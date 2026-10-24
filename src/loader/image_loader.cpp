// ============================================================================
// Class: ImageLoader
// Description: ImageLoader implements multi-threaded image decoding using Windows Imaging Component (WIC) for [IMAGE_VIEWER_APP_NAME].
// Responsibility: Decode pixel files asynchronously, queuing raw bytes for main thread GPU texture uploading.
// ============================================================================
#include "common/pch.h"
#include "image_loader.h"
#include "common/performance_trace.h"

namespace kv {

// ─── Supported Extensions ────────────────────────────────────────────────────

static const std::vector<std::wstring> kSupportedExtensions = {
    L".jpg", L".jpeg", L".jfif", L".png", L".apng",
    L".gif", L".bmp", L".tif", L".tiff", L".tga",
    L".ico", L".svg", L".svgz",
    L".webp", L".avif", L".avf",
    L".heif", L".heic",
    L".jxr", L".wdp", L".hdp"
};

static const std::vector<std::wstring> kAnimatedExtensions = {
    L".gif", L".webp", L".apng", L".png"
};

const std::vector<std::wstring>& ImageLoader::GetSupportedExtensions() {
    return kSupportedExtensions;
}

bool ImageLoader::IsSupported(const std::wstring& path) {
    std::filesystem::path p(path);
    auto ext = p.extension().wstring();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    return std::find(kSupportedExtensions.begin(), kSupportedExtensions.end(), ext)
           != kSupportedExtensions.end();
}

bool ImageLoader::IsAnimatedFormat(const std::wstring& path) {
    std::filesystem::path p(path);
    auto ext = p.extension().wstring();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    return std::find(kAnimatedExtensions.begin(), kAnimatedExtensions.end(), ext)
           != kAnimatedExtensions.end();
}

// ─── Initialize / Shutdown ───────────────────────────────────────────────────

bool ImageLoader::Initialize(uint32_t threadCount) {
    if (!m_shutdown.load()) return false;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&m_wicFactory)
    );
    KV_HR_RETURN(hr, L"CoCreateInstance(WICImagingFactory2)");

    m_stopSource = std::stop_source{};
    m_shutdown = false;
    m_workers.reserve(threadCount);
    for (uint32_t i = 0; i < threadCount; ++i) {
        m_workers.emplace_back(&ImageLoader::WorkerLoop, this);
    }

    LogInfo(std::format(L"ImageLoader initialized with {} worker threads", threadCount));
    return true;
}

ImageLoader::~ImageLoader() {
    Shutdown();
}

void ImageLoader::Shutdown() {
    std::queue<DecodeTask> discarded, discardedForeground, discardedDestination;
    {
        std::lock_guard lock(m_queueMutex);
        m_shutdown = true;
        m_stopSource.request_stop();
        discarded.swap(m_taskQueue);
        discardedForeground.swap(m_foregroundQueue); discardedDestination.swap(m_destinationQueue);
    }
    m_queueCV.notify_all();

    for (auto& w : m_workers) {
        if (w.joinable()) w.join();
    }
    m_workers.clear();
    m_thumbnailCache.Shutdown();
    m_pendingViewers.clear();
    std::vector<PendingTexture> pending;
    std::vector<PendingAnimatedTexture> animated;
    {
        std::lock_guard lock(m_pendingMutex);
        pending.swap(m_pendingTextures);
        animated.swap(m_pendingAnimatedTextures);
    }
    // Captured owners and GPU resources are released on the calling thread, outside queue locks.
    m_wicFactory.Reset();
}

// ─── Thread Pool ─────────────────────────────────────────────────────────────

void ImageLoader::WorkerLoop() {
    ScopedComInit com;

    while (true) {
        DecodeTask task;
        {
            std::unique_lock lock(m_queueMutex);
            m_queueCV.wait(lock, [this] {
                return m_shutdown || !m_foregroundQueue.empty() ||
                    (m_activeThumbnails<3 && !m_destinationQueue.empty()) ||
                    (!m_taskQueue.empty() && (!m_taskQueue.front().thumbnail || m_activeThumbnails<3));
            });
            if (m_shutdown) return;
            auto& queue=!m_foregroundQueue.empty() ? m_foregroundQueue :
                (m_activeThumbnails<3 && !m_destinationQueue.empty()) ? m_destinationQueue : m_taskQueue;
            task=std::move(queue.front()); queue.pop();
            if (task.thumbnail) ++m_activeThumbnails;
        }
        PerformanceTrace::Event("work.started", task.traceId);
        try { task(); } catch (...) { LogError(L"Background image request failed"); }
        PerformanceTrace::Event("work.finished", task.traceId);
        { std::lock_guard lock(m_queueMutex); if (task.thumbnail) --m_activeThumbnails; }
        m_queueCV.notify_all();
    }
}

void ImageLoader::EnqueueTask(std::function<void()> task, ImageCancellation cancel, int priority, bool thumbnail) {
    {
        std::lock_guard lock(m_queueMutex);
        if (m_shutdown) return;
        const auto id = ++m_nextTaskId;
        PerformanceTrace::Event("work.queued", id);
        auto& queue=priority==0 ? m_foregroundQueue : priority==1 ? m_destinationQueue : m_taskQueue;
        queue.push({std::move(task), std::move(cancel), priority, thumbnail, id});
    }
    m_queueCV.notify_one();
}

// ─── WIC Decoder Creation ────────────────────────────────────────────────────

ComPtr<IWICBitmapDecoder> ImageLoader::CreateDecoder(const std::wstring& path) {
    ComPtr<IWICBitmapDecoder> decoder;
    HRESULT hr = m_wicFactory->CreateDecoderFromFilename(
        path.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnDemand,
        &decoder
    );

    if (FAILED(hr)) {
        LogError(std::format(L"Failed to create decoder for '{}': 0x{:08X}",
            path, static_cast<unsigned>(hr)));
        return nullptr;
    }

    return decoder;
}

// ─── Decode Single Frame ─────────────────────────────────────────────────────

bool ImageLoader::DecodeFrame(IWICBitmapFrameDecode* frame, ImageData& out, uint32_t targetWidth, uint32_t targetHeight, ImageCancellation cancel) {
    if (cancel.IsCancelled()) return false;
    UINT width = 0, height = 0;
    KV_HR_RETURN(frame->GetSize(&width, &height), L"GetSize");
    if (cancel.IsCancelled()) return false;

    UINT actualWidth = width;
    UINT actualHeight = height;
    ComPtr<IWICBitmapSource> source = frame;

    if (targetWidth > 0 && targetHeight > 0) {
        float scaleX = static_cast<float>(targetWidth) / width;
        float scaleY = static_cast<float>(targetHeight) / height;
        float scale = std::min(scaleX, scaleY);
        actualWidth = static_cast<UINT>(width * scale);
        actualHeight = static_cast<UINT>(height * scale);
        if (actualWidth == 0) actualWidth = 1;
        if (actualHeight == 0) actualHeight = 1;

        ComPtr<IWICBitmapScaler> scaler;
        KV_HR_RETURN(m_wicFactory->CreateBitmapScaler(&scaler), L"CreateBitmapScaler");
        if (cancel.IsCancelled()) return false;
        HRESULT hr = scaler->Initialize(frame, actualWidth, actualHeight, WICBitmapInterpolationModeFant);
        KV_HR_RETURN(hr, L"BitmapScaler::Initialize");
        if (cancel.IsCancelled()) return false;
        source = scaler;
    }

    ComPtr<IWICFormatConverter> converter;
    KV_HR_RETURN(m_wicFactory->CreateFormatConverter(&converter), L"CreateFormatConverter");
    if (cancel.IsCancelled()) return false;

    HRESULT hr = converter->Initialize(
        source.Get(),
        GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone,
        nullptr, 0.0,
        WICBitmapPaletteTypeMedianCut
    );
    KV_HR_RETURN(hr, L"FormatConverter::Initialize");
    if (cancel.IsCancelled()) return false;

    WICPixelFormatGUID sourceFormat;
    frame->GetPixelFormat(&sourceFormat);
    if (cancel.IsCancelled()) return false;
    out.hasAlpha = (sourceFormat != GUID_WICPixelFormat24bppBGR &&
                    sourceFormat != GUID_WICPixelFormat24bppRGB &&
                    sourceFormat != GUID_WICPixelFormat8bppGray);

    uint32_t stride = actualWidth * 4;
    out.width  = actualWidth;
    out.height = actualHeight;
    out.pixels.resize(static_cast<size_t>(stride) * actualHeight);
    if (cancel.IsCancelled()) return false;

    hr = converter->CopyPixels(nullptr, stride, static_cast<UINT>(out.pixels.size()), out.pixels.data());
    KV_HR_RETURN(hr, L"CopyPixels");

    return !cancel.IsCancelled();
}

// ─── Synchronous Loading ─────────────────────────────────────────────────────

std::shared_ptr<ImageData> ImageLoader::LoadSync(const std::wstring& path, uint32_t targetWidth, uint32_t targetHeight) {
    return DecodeImage(path, targetWidth, targetHeight);
}

std::shared_ptr<ImageData> ImageLoader::DecodeImage(const std::wstring& path, uint32_t targetWidth, uint32_t targetHeight, ImageCancellation cancel) {
    if (cancel.IsCancelled()) return nullptr;
    PerformanceSpan decodeSpan("decode.begin", "decode.end");
    auto decoder = CreateDecoder(path);
    if (!decoder || cancel.IsCancelled()) return nullptr;
    return DecodeImage(decoder.Get(), path, targetWidth, targetHeight, cancel);
}

std::shared_ptr<ImageData> ImageLoader::DecodeImage(
    IWICBitmapDecoder* decoder, const std::wstring& path, uint32_t targetWidth, uint32_t targetHeight, ImageCancellation cancel) {
    if (cancel.IsCancelled()) return nullptr;

    ComPtr<IWICBitmapFrameDecode> frame;
    HRESULT hr = decoder->GetFrame(0, &frame);
    if (cancel.IsCancelled()) return nullptr;
    if (FAILED(hr)) {
        LogError(std::format(L"GetFrame(0) failed for '{}': 0x{:08X}",
            path, static_cast<unsigned>(hr)));
        return nullptr;
    }

    auto image = std::make_shared<ImageData>();
    image->filePath = path;

    if (!DecodeFrame(frame.Get(), *image, targetWidth, targetHeight, cancel)) {
        return nullptr;
    }

    LogDebug(std::format(L"Loaded: {} ({}x{}, {} bytes)",
        path, image->width, image->height, image->pixels.size()));

    return image;
}


void ImageLoader::InitializeThumbnailCache(const std::filesystem::path& root) { m_thumbnailCache.Initialize(root); }

void ImageLoader::LoadForViewerAsync(const std::wstring& path, std::function<void(ViewerLoadResult)> callback, std::weak_ptr<const bool> lifetime) {
    const ImageCancellation cancel(lifetime, m_stopSource.get_token());
    EnqueueTask([this, path, callback=std::move(callback), cancel] {
        auto result = LoadForViewerSync(path, cancel);
        if (cancel.IsCancelled()) return;
        std::lock_guard lock(m_pendingMutex);
        if (!cancel.IsCancelled()) m_pendingViewers.push_back({std::move(result), callback, cancel});
    }, cancel, 0);
}

void ImageLoader::LoadThumbnailAsync(const std::wstring& path, ImageLoadCallback callback, uint32_t size,
    ImageUploadPredicate isCurrent, std::weak_ptr<const bool> lifetime, bool destination) {
    const ImageCancellation cancel(lifetime, m_stopSource.get_token());
    EnqueueTask([this,path,callback=std::move(callback),size,isCurrent=std::move(isCurrent),cancel] {
        if (cancel.IsCancelled()) return;
        std::wstring signature;
        try { signature = ThumbnailCache::Signature(path); } catch (...) { }
        std::shared_ptr<ImageData> image;
        try { image=m_thumbnailCache.Read(path,signature,size); } catch (...) { }
        bool hit = image != nullptr;
        if (!image) image = DecodeImage(path, size, size, cancel);
        if (cancel.IsCancelled()) return;
        if (image && !signature.empty()) {
            std::wstring after;
            try { after = ThumbnailCache::Signature(path); } catch (...) { }
            if (signature != after) {
                PerformanceTrace::Event("cache.invalidated");
                signature=after; hit=false;
                image=signature.empty() ? nullptr : DecodeImage(path,size,size,cancel);
                try { if (signature!=ThumbnailCache::Signature(path)) image.reset(); } catch (...) { image.reset(); }
            }
        }
        // Publish before optional compression/writing; only CPU bytes go to the cache writer.
        { std::lock_guard lock(m_pendingMutex); m_pendingTextures.push_back({image,callback,isCurrent,cancel}); }
        if (image && !hit && !cancel.IsCancelled()) m_thumbnailCache.Store(signature,size,*image);
    }, cancel, destination ? 1 : 2, true);
}

// ─── Animated Image Loading ──────────────────────────────────────────────────

std::shared_ptr<AnimatedImageData> ImageLoader::LoadAnimatedSync(const std::wstring& path) {
    return DecodeAnimatedImage(path);
}

std::shared_ptr<AnimatedImageData> ImageLoader::DecodeAnimatedImage(const std::wstring& path, ImageCancellation cancel) {
    if (cancel.IsCancelled()) return nullptr;
    auto decoder = CreateDecoder(path);
    if (!decoder || cancel.IsCancelled()) return nullptr;

    UINT frameCount = 0;
    HRESULT hr = decoder->GetFrameCount(&frameCount);
    if (FAILED(hr) || frameCount <= 1 || cancel.IsCancelled()) {
        return nullptr;
    }
    return DecodeAnimatedFrames(decoder.Get(), path, frameCount, cancel);
}

ViewerLoadResult ImageLoader::LoadForViewerSync(const std::wstring& path, ImageCancellation cancel) {
    PerformanceSpan span("viewer.decode.begin", "viewer.decode.end");
    if (cancel.IsCancelled()) return {};
    if (!IsAnimatedFormat(path)) return {DecodeImage(path, 0, 0, cancel), nullptr};
    {
        auto decoder = CreateDecoder(path);
        if (decoder) {
            UINT frameCount = 0;
            if (SUCCEEDED(decoder->GetFrameCount(&frameCount))) {
                if (frameCount > 1) {
                    auto animation = DecodeAnimatedFrames(decoder.Get(), path, frameCount, cancel);
                    if (animation && animation->IsValid()) return {nullptr, std::move(animation)};
                } else {
                    auto image = DecodeImage(decoder.Get(), path, 0, 0, cancel);
                    if (image && image->IsValid()) return {std::move(image), nullptr};
                }
            }
        }
    } // Release failed attempt (including file handle) before preserving the legacy fresh static fallback.
    return {DecodeImage(path, 0, 0, cancel), nullptr};
}

std::shared_ptr<AnimatedImageData> ImageLoader::DecodeAnimatedFrames(
    IWICBitmapDecoder* decoder, const std::wstring& path, UINT frameCount, ImageCancellation cancel) {
    if (cancel.IsCancelled()) return nullptr;
    HRESULT hr = S_OK;

    auto anim = std::make_shared<AnimatedImageData>();
    anim->filePath = path;
    anim->frames.reserve(frameCount);

    UINT globalW = 0;
    UINT globalH = 0;
    ComPtr<IWICMetadataQueryReader> globalMeta;
    if (SUCCEEDED(decoder->GetMetadataQueryReader(&globalMeta))) {
        PROPVARIANT varW, varH;
        PropVariantInit(&varW);
        PropVariantInit(&varH);
        if (SUCCEEDED(globalMeta->GetMetadataByName(L"/logscrdesc/Width", &varW)) && varW.vt == VT_UI2) {
            globalW = varW.uiVal;
        }
        if (SUCCEEDED(globalMeta->GetMetadataByName(L"/logscrdesc/Height", &varH)) && varH.vt == VT_UI2) {
            globalH = varH.uiVal;
        }
        PropVariantClear(&varW);
        PropVariantClear(&varH);

        PROPVARIANT loops;
        PropVariantInit(&loops);
        if (SUCCEEDED(globalMeta->GetMetadataByName(L"/appext/Data", &loops))) {
            anim->loopCount = 0;
        }
        PropVariantClear(&loops);
    }

    if (globalW == 0 || globalH == 0) {
        ComPtr<IWICBitmapFrameDecode> firstFrame;
        if (SUCCEEDED(decoder->GetFrame(0, &firstFrame))) {
            firstFrame->GetSize(&globalW, &globalH);
        }
    }
    if (globalW == 0 || globalH == 0) {
        globalW = 1;
        globalH = 1;
    }

    if (cancel.IsCancelled()) return nullptr;
    std::vector<uint32_t> canvas(static_cast<size_t>(globalW) * globalH, 0);
    std::vector<uint32_t> prevCanvas = canvas;

    uint8_t prevDisposal = 0;
    UINT prevLeft = 0, prevTop = 0, prevW = 0, prevH = 0;

    for (UINT i = 0; i < frameCount; ++i) {
        if (cancel.IsCancelled()) return nullptr;
        ComPtr<IWICBitmapFrameDecode> frame;
        hr = decoder->GetFrame(i, &frame);
        if (cancel.IsCancelled()) return nullptr;
        if (FAILED(hr)) continue;

        UINT localW = 0, localH = 0;
        frame->GetSize(&localW, &localH);
        if (cancel.IsCancelled()) return nullptr;

        ComPtr<IWICFormatConverter> converter;
        if (FAILED(m_wicFactory->CreateFormatConverter(&converter))) continue;
        if (cancel.IsCancelled()) return nullptr;
        if (FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut)))
            continue;

        if (cancel.IsCancelled()) return nullptr;
        uint32_t stride = localW * 4;
        std::vector<uint8_t> localPixels(static_cast<size_t>(stride) * localH);
        if (FAILED(converter->CopyPixels(nullptr, stride, static_cast<UINT>(localPixels.size()),
                localPixels.data())))
            continue;

        if (cancel.IsCancelled()) return nullptr;
        UINT left = 0, top = 0;
        uint8_t disposal = 0;
        double delayMs = 100.0;

        ComPtr<IWICMetadataQueryReader> metadata;
        if (SUCCEEDED(frame->GetMetadataQueryReader(&metadata))) {
            PROPVARIANT val;
            PropVariantInit(&val);

            if (SUCCEEDED(metadata->GetMetadataByName(L"/imgdesc/Left", &val)) && val.vt == VT_UI2) {
                left = val.uiVal;
            }
            PropVariantClear(&val);

            if (SUCCEEDED(metadata->GetMetadataByName(L"/imgdesc/Top", &val)) && val.vt == VT_UI2) {
                top = val.uiVal;
            }
            PropVariantClear(&val);

            if (SUCCEEDED(metadata->GetMetadataByName(L"/grctlext/Disposal", &val)) && val.vt == VT_UI1) {
                disposal = val.bVal;
            }
            PropVariantClear(&val);

            if (SUCCEEDED(metadata->GetMetadataByName(L"/grctlext/Delay", &val))) {
                if (val.vt == VT_UI2) {
                    delayMs = static_cast<double>(val.uiVal) * 10.0;
                }
            }
            PropVariantClear(&val);
        }

        if (delayMs < 20.0) delayMs = 100.0;

        if (i > 0) {
            if (prevDisposal == 2) {
                for (UINT dy = 0; dy < prevH; ++dy) {
                    if (cancel.IsCancelled()) return nullptr;
                    for (UINT dx = 0; dx < prevW; ++dx) {
                        UINT tx = prevLeft + dx;
                        UINT ty = prevTop + dy;
                        if (tx < globalW && ty < globalH) {
                            canvas[static_cast<size_t>(ty) * globalW + tx] = 0;
                        }
                    }
                }
            } else if (prevDisposal == 3) {
                canvas = prevCanvas;
            }
        }

        prevCanvas = canvas;

        for (UINT dy = 0; dy < localH; ++dy) {
            if (cancel.IsCancelled()) return nullptr;
            for (UINT dx = 0; dx < localW; ++dx) {
                UINT tx = left + dx;
                UINT ty = top + dy;
                if (tx >= globalW || ty >= globalH) continue;

                size_t srcIdx = (static_cast<size_t>(dy) * localW + dx) * 4;
                size_t destIdx = static_cast<size_t>(ty) * globalW + tx;

                uint8_t sb = localPixels[srcIdx + 0];
                uint8_t sg = localPixels[srcIdx + 1];
                uint8_t sr = localPixels[srcIdx + 2];
                uint8_t sa = localPixels[srcIdx + 3];

                if (sa == 255) {
                    canvas[destIdx] = sb | (sg << 8) | (sr << 16) | (sa << 24);
                } else if (sa > 0) {
                    uint32_t dest = canvas[destIdx];
                    uint8_t db = dest & 0xFF;
                    uint8_t dg = (dest >> 8) & 0xFF;
                    uint8_t dr = (dest >> 16) & 0xFF;
                    uint8_t da = (dest >> 24) & 0xFF;

                    float alphaF = sa / 255.0f;
                    uint8_t rb = sb + static_cast<uint8_t>(db * (1.0f - alphaF));
                    uint8_t rg = sg + static_cast<uint8_t>(dg * (1.0f - alphaF));
                    uint8_t rr = sr + static_cast<uint8_t>(dr * (1.0f - alphaF));
                    uint8_t ra = sa + static_cast<uint8_t>(da * (1.0f - alphaF));

                    canvas[destIdx] = rb | (rg << 8) | (rr << 16) | (ra << 24);
                }
            }
        }

        AnimationFrame af;
        af.width = globalW;
        af.height = globalH;
        af.delayMs = delayMs;
        uint32_t globalStride = globalW * 4;
        af.pixels.resize(static_cast<size_t>(globalStride) * globalH);
        std::memcpy(af.pixels.data(), canvas.data(), af.pixels.size());

        anim->totalDurationMs += af.delayMs;
        anim->frames.push_back(std::move(af));

        prevDisposal = disposal;
        prevLeft = left;
        prevTop = top;
        prevW = localW;
        prevH = localH;
    }

    LogInfo(std::format(L"Loaded animated: {} ({} frames, {:.0f}ms total)",
        path, anim->frames.size(), anim->totalDurationMs));

    return cancel.IsCancelled() ? nullptr : anim;
}

// ─── Asynchronous Loading ────────────────────────────────────────────────────

void ImageLoader::LoadAsync(const std::wstring& path, ImageLoadCallback callback, uint32_t targetWidth, uint32_t targetHeight, ImageUploadPredicate isCurrent, std::weak_ptr<const bool> lifetime) {
    const ImageCancellation cancel(lifetime, m_stopSource.get_token());
    EnqueueTask([this, path, callback, targetWidth, targetHeight, isCurrent, cancel]() {
        auto image = DecodeImage(path, targetWidth, targetHeight, cancel);
        if (cancel.IsCancelled()) return;

        if (image) {
            std::lock_guard lock(m_pendingMutex);
            m_pendingTextures.push_back({ image, callback, isCurrent, cancel });
        } else if (callback) {
            std::lock_guard lock(m_pendingMutex);
            m_pendingTextures.push_back({ nullptr, callback, isCurrent, cancel });
        }
    }, cancel);
}

void ImageLoader::LoadAnimatedAsync(const std::wstring& path, AnimatedImageLoadCallback callback, std::weak_ptr<const bool> lifetime) {
    const ImageCancellation cancel(lifetime, m_stopSource.get_token());
    EnqueueTask([this, path, callback, cancel]() {
        auto anim = DecodeAnimatedImage(path, cancel);
        if (cancel.IsCancelled()) return;

        if (anim) {
            std::lock_guard lock(m_pendingMutex);
            m_pendingAnimatedTextures.push_back({ anim, callback, cancel });
        } else if (callback) {
            std::lock_guard lock(m_pendingMutex);
            m_pendingAnimatedTextures.push_back({ nullptr, callback, cancel });
        }
    }, cancel);
}

void ImageLoader::CancelObsoleteRequests() {
    std::vector<DecodeTask> discarded;
    {
        std::lock_guard lock(m_queueMutex);
        for (auto* queue : {&m_foregroundQueue,&m_destinationQueue,&m_taskQueue}) {
            if (queue->empty()) continue; // No temporary deque allocation on the idle render path.
            std::queue<DecodeTask> retained;
            while (!queue->empty()) {
                auto task=std::move(queue->front()); queue->pop();
                if (task.cancel.IsCancelled()) discarded.push_back(std::move(task));
                else retained.push(std::move(task));
            }
            queue->swap(retained);
        }
    }
    if (!discarded.empty()) m_queueCV.notify_all();
    // Repeated opens must not retain full-resolution CPU results until a later render frame.
    std::vector<PendingViewer> obsolete;
    {
        std::lock_guard lock(m_pendingMutex);
        std::erase_if(m_pendingViewers,[&](PendingViewer& viewer) {
            if (!viewer.cancel.IsCancelled()) return false;
            obsolete.push_back(std::move(viewer)); return true;
        });
    }
    // Canceled captures and CPU buffers are released on UI, outside both queue locks.
}

void ImageLoader::ProcessPendingTextures(Renderer& renderer) {
    CancelObsoleteRequests();
    std::vector<PendingViewer> viewers;
    { std::lock_guard lock(m_pendingMutex); viewers.swap(m_pendingViewers); }
    for (auto& viewer : viewers) {
        if (viewer.cancel.IsCancelled()) continue;
        if (viewer.result.image) UploadToGpu(*viewer.result.image, renderer);
        if (viewer.result.animation) UploadAnimationToGpu(*viewer.result.animation, renderer);
        if (!viewer.cancel.IsCancelled() && viewer.callback) viewer.callback(std::move(viewer.result));
    }
    std::vector<PendingTexture> pending;
    std::vector<PendingAnimatedTexture> pendingAnimated;
    {
        std::lock_guard lock(m_pendingMutex);
        pending.swap(m_pendingTextures);
        pendingAnimated.swap(m_pendingAnimatedTextures);
    }

    for (auto& pt : pending) {
        if (pt.cancel.IsCancelled()) continue;
        const bool isCurrent = !pt.isCurrent || pt.isCurrent();
        if (pt.cancel.IsCancelled()) continue;
        if (isCurrent && pt.image && pt.image->IsValid() && !pt.image->HasGpuBitmap()) {
            UploadToGpu(*pt.image, renderer);
        }
        if (pt.callback) {
            pt.callback(pt.image);
        }
    }

    for (auto& pa : pendingAnimated) {
        if (pa.cancel.IsCancelled()) continue;
        if (pa.anim && pa.anim->IsValid()) {
            UploadAnimationToGpu(*pa.anim, renderer);
        }
        if (pa.callback) {
            pa.callback(pa.anim);
        }
    }
}

void ImageLoader::UploadToGpu(ImageData& image, Renderer& renderer) {
    if (!image.IsValid()) return;
    PerformanceSpan uploadSpan("upload.begin", "upload.end");

    image.gpuBitmap = renderer.CreateBitmapFromMemory(
        image.pixels.data(),
        image.width,
        image.height,
        image.width * 4
    );

    if (!image.gpuBitmap) {
        LogError(std::format(L"Failed to upload image to GPU: {}",
            image.filePath));
    }
}

void ImageLoader::UploadAnimationToGpu(AnimatedImageData& anim, Renderer& renderer) {
    for (auto& frame : anim.frames) {
        if (frame.gpuBitmap) continue;
        frame.gpuBitmap = renderer.CreateBitmapFromMemory(
            frame.pixels.data(),
            frame.width,
            frame.height,
            frame.width * 4
        );
    }
}

} // namespace kv
