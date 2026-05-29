// ============================================================================
// image_loader.cpp — WIC-Based Image Loading Implementation
// Krumassan Image Viewer — Native C++20
//
// WIC handles format detection and decoding natively for:
//   JPEG, PNG, GIF (animated), TIFF, BMP, ICO, JPEG-XR/HD Photo
// With Windows 10+ codecs:
//   WebP, AVIF, HEIF/HEIC, SVG (via installed codecs)
//
// Thread safety model:
//   - WIC factory is thread-safe (created with CLSCTX_INPROC_SERVER)
//   - Each decoder is created per-file on its owning thread
//   - Decoded pixel data (CPU memory) is safe to pass between threads
//   - ID2D1Bitmap creation is restricted to the render thread
// ============================================================================

#include "pch.h"
#include "image_loader.h"

namespace kv {

// ─── Supported Extensions ────────────────────────────────────────────────────

static const std::vector<std::wstring> kSupportedExtensions = {
    L".jpg", L".jpeg", L".jfif", L".png", L".apng",
    L".gif", L".bmp", L".tif", L".tiff", L".tga",
    L".ico", L".svg", L".svgz",
    L".webp", L".avif", L".avf",
    L".heif", L".heic",
    L".jxr", L".wdp", L".hdp"  // JPEG XR / HD Photo (WIC native)
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
    // Create WIC factory (thread-safe)
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&m_wicFactory)
    );
    KV_HR_RETURN(hr, L"CoCreateInstance(WICImagingFactory2)");

    // Start worker threads for async loading
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
    // Signal workers to stop
    {
        std::lock_guard lock(m_queueMutex);
        m_shutdown = true;
    }
    m_queueCV.notify_all();

    // Wait for workers to finish
    for (auto& w : m_workers) {
        if (w.joinable()) w.join();
    }
    m_workers.clear();
}

// ─── Thread Pool ─────────────────────────────────────────────────────────────

void ImageLoader::WorkerLoop() {
    // Each worker thread needs its own COM initialization
    ScopedComInit com;

    while (true) {
        std::function<void()> task;
        {
            std::unique_lock lock(m_queueMutex);
            m_queueCV.wait(lock, [this] { return m_shutdown || !m_taskQueue.empty(); });

            if (m_shutdown && m_taskQueue.empty()) return;

            task = std::move(m_taskQueue.front());
            m_taskQueue.pop();
        }
        task();
    }
}

void ImageLoader::EnqueueTask(std::function<void()> task) {
    {
        std::lock_guard lock(m_queueMutex);
        m_taskQueue.push(std::move(task));
    }
    m_queueCV.notify_one();
}

// ─── WIC Decoder Creation ────────────────────────────────────────────────────

ComPtr<IWICBitmapDecoder> ImageLoader::CreateDecoder(const std::wstring& path) {
    ComPtr<IWICBitmapDecoder> decoder;
    HRESULT hr = m_wicFactory->CreateDecoderFromFilename(
        path.c_str(),
        nullptr,                        // vendor GUID (null = any)
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
    // Get frame dimensions
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

    // Convert to 32bpp BGRA premultiplied alpha (Direct2D's native format)
    ComPtr<IWICFormatConverter> converter;
    KV_HR_RETURN(m_wicFactory->CreateFormatConverter(&converter), L"CreateFormatConverter");

    HRESULT hr = converter->Initialize(
        source.Get(),
        GUID_WICPixelFormat32bppPBGRA,   // premultiplied BGRA
        WICBitmapDitherTypeNone,
        nullptr, 0.0,
        WICBitmapPaletteTypeMedianCut
    );
    KV_HR_RETURN(hr, L"FormatConverter::Initialize");

    // Check if source has alpha channel
    WICPixelFormatGUID sourceFormat;
    frame->GetPixelFormat(&sourceFormat);
    out.hasAlpha = (sourceFormat != GUID_WICPixelFormat24bppBGR &&
                    sourceFormat != GUID_WICPixelFormat24bppRGB &&
                    sourceFormat != GUID_WICPixelFormat8bppGray);

    // Copy pixels to our buffer
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

    // Get the first frame
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
        // Not animated or single frame — fall back to static loading
        return nullptr;
    }

    auto anim = std::make_shared<AnimatedImageData>();
    anim->filePath = path;
    anim->frames.reserve(frameCount);

    for (UINT i = 0; i < frameCount; ++i) {
        ComPtr<IWICBitmapFrameDecode> frame;
        hr = decoder->GetFrame(i, &frame);
        if (FAILED(hr)) continue;

        AnimationFrame af;

        // Decode the frame pixels
        UINT w = 0, h = 0;
        frame->GetSize(&w, &h);

        ComPtr<IWICFormatConverter> converter;
        if (FAILED(m_wicFactory->CreateFormatConverter(&converter))) continue;
        if (FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut)))
            continue;

        af.width = w;
        af.height = h;
        uint32_t stride = w * 4;
        af.pixels.resize(static_cast<size_t>(stride) * h);
        if (FAILED(converter->CopyPixels(nullptr, stride, static_cast<UINT>(af.pixels.size()),
                af.pixels.data())))
            continue;

        // Get frame delay from metadata
        ComPtr<IWICMetadataQueryReader> metadata;
        if (SUCCEEDED(frame->GetMetadataQueryReader(&metadata))) {
            PROPVARIANT delay;
            PropVariantInit(&delay);

            // GIF frame delay: "/grctlext/Delay" (in 1/100th seconds)
            if (SUCCEEDED(metadata->GetMetadataByName(L"/grctlext/Delay", &delay))) {
                if (delay.vt == VT_UI2) {
                    af.delayMs = static_cast<double>(delay.uiVal) * 10.0;
                }
            }
            // WebP frame delay: "/imgdesc/Delay" or similar
            // APNG frame delay: varies by codec

            PropVariantClear(&delay);
        }

        // Clamp minimum delay (some GIFs have 0 or 10ms delays)
        if (af.delayMs < 20.0) af.delayMs = 100.0;

        anim->totalDurationMs += af.delayMs;
        anim->frames.push_back(std::move(af));
    }

    // Get loop count from metadata
    ComPtr<IWICMetadataQueryReader> globalMeta;
    if (SUCCEEDED(decoder->GetMetadataQueryReader(&globalMeta))) {
        PROPVARIANT loops;
        PropVariantInit(&loops);
        // GIF: "/appext/application" for NETSCAPE2.0 extension
        if (SUCCEEDED(globalMeta->GetMetadataByName(L"/appext/Data", &loops))) {
            // The loop count is typically in the application extension data
            // For simplicity, default to infinite loop
            anim->loopCount = 0;
        }
        PropVariantClear(&loops);
    }

    LogInfo(std::format(L"Loaded animated: {} ({} frames, {:.0f}ms total)",
        path, anim->frames.size(), anim->totalDurationMs));

    return anim;
}

// ─── Asynchronous Loading ────────────────────────────────────────────────────

void ImageLoader::LoadAsync(const std::wstring& path, ImageLoadCallback callback, uint32_t targetWidth, uint32_t targetHeight) {
    // Capture by value for the background thread
    EnqueueTask([this, path, callback, targetWidth, targetHeight]() {
        auto image = DecodeImage(path, targetWidth, targetHeight);

        if (image) {
            // Queue the decoded image for GPU texture creation on the main thread
            std::lock_guard lock(m_pendingMutex);
            m_pendingTextures.push_back({ image, callback });
        } else if (callback) {
            // Still notify the callback of failure
            std::lock_guard lock(m_pendingMutex);
            m_pendingTextures.push_back({ nullptr, callback });
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

// ─── Process Pending Textures (Main Thread) ──────────────────────────────────
// This is called once per frame on the main thread. It creates GPU textures
// from the pixel data that background threads have decoded.
//
// This is the critical pattern that prevents the thumbnail loading failures
// from the Python version: GPU resource creation is isolated to one thread.

void ImageLoader::ProcessPendingTextures(Renderer& renderer) {
    std::vector<PendingTexture> pending;
    std::vector<PendingAnimatedTexture> pendingAnimated;
    {
        std::lock_guard lock(m_pendingMutex);
        pending.swap(m_pendingTextures);
        pendingAnimated.swap(m_pendingAnimatedTextures);
    }

    for (auto& pt : pending) {
        if (pt.image && pt.image->IsValid() && !pt.image->HasGpuBitmap()) {
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

// ─── GPU Upload ──────────────────────────────────────────────────────────────

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
        if (frame.gpuBitmap) continue;  // already uploaded
        frame.gpuBitmap = renderer.CreateBitmapFromMemory(
            frame.pixels.data(),
            frame.width,
            frame.height,
            frame.width * 4
        );
    }
}

} // namespace kv
