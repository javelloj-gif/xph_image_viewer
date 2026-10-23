// ============================================================================
// Class: ImageLoader
// Description: ImageLoader implements multi-threaded image decoding using Windows Imaging Component (WIC) for [IMAGE_VIEWER_APP_NAME].
// Responsibility: Decode pixel files asynchronously, queuing raw bytes for main thread GPU texture uploading.
// ============================================================================
#include "common/pch.h"
#include "image_loader.h"

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
    std::queue<std::function<void()>> discarded;
    {
        std::lock_guard lock(m_queueMutex);
        m_shutdown = true;
        discarded.swap(m_taskQueue);
    }
    m_queueCV.notify_all();

    for (auto& w : m_workers) {
        if (w.joinable()) w.join();
    }
    m_workers.clear();
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
        std::function<void()> task;
        {
            std::unique_lock lock(m_queueMutex);
            m_queueCV.wait(lock, [this] { return m_shutdown || !m_taskQueue.empty(); });

            if (m_shutdown) return;

            task = std::move(m_taskQueue.front());
            m_taskQueue.pop();
        }
        task();
    }
}

void ImageLoader::EnqueueTask(std::function<void()> task) {
    {
        std::lock_guard lock(m_queueMutex);
        if (m_shutdown) return;
        m_taskQueue.push(std::move(task));
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

bool ImageLoader::DecodeFrame(IWICBitmapFrameDecode* frame, ImageData& out, uint32_t targetWidth, uint32_t targetHeight) {
    UINT width = 0, height = 0;
    KV_HR_RETURN(frame->GetSize(&width, &height), L"GetSize");

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
        HRESULT hr = scaler->Initialize(frame, actualWidth, actualHeight, WICBitmapInterpolationModeFant);
        KV_HR_RETURN(hr, L"BitmapScaler::Initialize");
        source = scaler;
    }

    ComPtr<IWICFormatConverter> converter;
    KV_HR_RETURN(m_wicFactory->CreateFormatConverter(&converter), L"CreateFormatConverter");

    HRESULT hr = converter->Initialize(
        source.Get(),
        GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone,
        nullptr, 0.0,
        WICBitmapPaletteTypeMedianCut
    );
    KV_HR_RETURN(hr, L"FormatConverter::Initialize");

    WICPixelFormatGUID sourceFormat;
    frame->GetPixelFormat(&sourceFormat);
    out.hasAlpha = (sourceFormat != GUID_WICPixelFormat24bppBGR &&
                    sourceFormat != GUID_WICPixelFormat24bppRGB &&
                    sourceFormat != GUID_WICPixelFormat8bppGray);

    uint32_t stride = actualWidth * 4;
    out.width  = actualWidth;
    out.height = actualHeight;
    out.pixels.resize(static_cast<size_t>(stride) * actualHeight);

    hr = converter->CopyPixels(nullptr, stride, static_cast<UINT>(out.pixels.size()), out.pixels.data());
    KV_HR_RETURN(hr, L"CopyPixels");

    return true;
}

// ─── Synchronous Loading ─────────────────────────────────────────────────────

std::shared_ptr<ImageData> ImageLoader::LoadSync(const std::wstring& path, uint32_t targetWidth, uint32_t targetHeight) {
    return DecodeImage(path, targetWidth, targetHeight);
}

std::shared_ptr<ImageData> ImageLoader::DecodeImage(const std::wstring& path, uint32_t targetWidth, uint32_t targetHeight) {
    auto decoder = CreateDecoder(path);
    if (!decoder) return nullptr;
    return DecodeImage(decoder.Get(), path, targetWidth, targetHeight);
}

std::shared_ptr<ImageData> ImageLoader::DecodeImage(
    IWICBitmapDecoder* decoder, const std::wstring& path, uint32_t targetWidth, uint32_t targetHeight) {

    ComPtr<IWICBitmapFrameDecode> frame;
    HRESULT hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) {
        LogError(std::format(L"GetFrame(0) failed for '{}': 0x{:08X}",
            path, static_cast<unsigned>(hr)));
        return nullptr;
    }

    auto image = std::make_shared<ImageData>();
    image->filePath = path;

    if (!DecodeFrame(frame.Get(), *image, targetWidth, targetHeight)) {
        return nullptr;
    }

    LogDebug(std::format(L"Loaded: {} ({}x{}, {} bytes)",
        path, image->width, image->height, image->pixels.size()));

    return image;
}

// ─── Animated Image Loading ──────────────────────────────────────────────────

std::shared_ptr<AnimatedImageData> ImageLoader::LoadAnimatedSync(const std::wstring& path) {
    return DecodeAnimatedImage(path);
}

std::shared_ptr<AnimatedImageData> ImageLoader::DecodeAnimatedImage(const std::wstring& path) {
    auto decoder = CreateDecoder(path);
    if (!decoder) return nullptr;

    UINT frameCount = 0;
    HRESULT hr = decoder->GetFrameCount(&frameCount);
    if (FAILED(hr) || frameCount <= 1) {
        return nullptr;
    }
    return DecodeAnimatedFrames(decoder.Get(), path, frameCount);
}

ViewerLoadResult ImageLoader::LoadForViewerSync(const std::wstring& path) {
    if (!IsAnimatedFormat(path)) return {LoadSync(path), nullptr};
    {
        auto decoder = CreateDecoder(path);
        if (decoder) {
            UINT frameCount = 0;
            if (SUCCEEDED(decoder->GetFrameCount(&frameCount))) {
                if (frameCount > 1) {
                    auto animation = DecodeAnimatedFrames(decoder.Get(), path, frameCount);
                    if (animation && animation->IsValid()) return {nullptr, std::move(animation)};
                } else {
                    auto image = DecodeImage(decoder.Get(), path, 0, 0);
                    if (image && image->IsValid()) return {std::move(image), nullptr};
                }
            }
        }
    } // Release failed attempt (including file handle) before preserving the legacy fresh static fallback.
    return {LoadSync(path), nullptr};
}

std::shared_ptr<AnimatedImageData> ImageLoader::DecodeAnimatedFrames(
    IWICBitmapDecoder* decoder, const std::wstring& path, UINT frameCount) {
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

    std::vector<uint32_t> canvas(static_cast<size_t>(globalW) * globalH, 0);
    std::vector<uint32_t> prevCanvas = canvas;

    uint8_t prevDisposal = 0;
    UINT prevLeft = 0, prevTop = 0, prevW = 0, prevH = 0;

    for (UINT i = 0; i < frameCount; ++i) {
        ComPtr<IWICBitmapFrameDecode> frame;
        hr = decoder->GetFrame(i, &frame);
        if (FAILED(hr)) continue;

        UINT localW = 0, localH = 0;
        frame->GetSize(&localW, &localH);

        ComPtr<IWICFormatConverter> converter;
        if (FAILED(m_wicFactory->CreateFormatConverter(&converter))) continue;
        if (FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut)))
            continue;

        uint32_t stride = localW * 4;
        std::vector<uint8_t> localPixels(static_cast<size_t>(stride) * localH);
        if (FAILED(converter->CopyPixels(nullptr, stride, static_cast<UINT>(localPixels.size()),
                localPixels.data())))
            continue;

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

    return anim;
}

// ─── Asynchronous Loading ────────────────────────────────────────────────────

void ImageLoader::LoadAsync(const std::wstring& path, ImageLoadCallback callback, uint32_t targetWidth, uint32_t targetHeight, ImageUploadPredicate isCurrent) {
    EnqueueTask([this, path, callback, targetWidth, targetHeight, isCurrent]() {
        auto image = DecodeImage(path, targetWidth, targetHeight);

        if (image) {
            std::lock_guard lock(m_pendingMutex);
            m_pendingTextures.push_back({ image, callback, isCurrent });
        } else if (callback) {
            std::lock_guard lock(m_pendingMutex);
            m_pendingTextures.push_back({ nullptr, callback, isCurrent });
        }
    });
}

void ImageLoader::LoadAnimatedAsync(const std::wstring& path, AnimatedImageLoadCallback callback) {
    EnqueueTask([this, path, callback]() {
        auto anim = DecodeAnimatedImage(path);

        if (anim) {
            std::lock_guard lock(m_pendingMutex);
            m_pendingAnimatedTextures.push_back({ anim, callback });
        } else if (callback) {
            std::lock_guard lock(m_pendingMutex);
            m_pendingAnimatedTextures.push_back({ nullptr, callback });
        }
    });
}

void ImageLoader::ProcessPendingTextures(Renderer& renderer) {
    std::vector<PendingTexture> pending;
    std::vector<PendingAnimatedTexture> pendingAnimated;
    {
        std::lock_guard lock(m_pendingMutex);
        pending.swap(m_pendingTextures);
        pendingAnimated.swap(m_pendingAnimatedTextures);
    }

    for (auto& pt : pending) {
        const bool isCurrent = !pt.isCurrent || pt.isCurrent();
        if (isCurrent && pt.image && pt.image->IsValid() && !pt.image->HasGpuBitmap()) {
            UploadToGpu(*pt.image, renderer);
        }
        if (pt.callback) {
            pt.callback(pt.image);
        }
    }

    for (auto& pa : pendingAnimated) {
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
