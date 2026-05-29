// ============================================================================
// image_loader.h — WIC-Based Image Loading with Async Support
// Krumassan Image Viewer — Native C++20
//
// Loads images using Windows Imaging Component (WIC), which provides native
// hardware-accelerated decoding for JPEG, PNG, TIFF, BMP, GIF, WebP, AVIF,
// HEIF, ICO, and more.
//
// Threading model (lesson from failed Python optimization):
//   - WIC decoding runs on background threads (true parallelism, no GIL)
//   - Decoded pixel data is passed to the main thread via a thread-safe queue
//   - GPU texture creation (ID2D1Bitmap) happens ONLY on the main thread
//   - This prevents the driver context issues that caused thumbnail loading
//     failures in the Python QThread approach
//
// Usage:
//   1. Call LoadSync() for immediate single-image loading (viewer page)
//   2. Call LoadAsync() for non-blocking thumbnail generation
//   3. Call ProcessPendingTextures() each frame on the main thread
// ============================================================================
#pragma once

#include "types.h"
#include "renderer.h"

namespace kv {

// ─── Image Data ──────────────────────────────────────────────────────────────

struct ImageData {
    uint32_t                width  = 0;
    uint32_t                height = 0;
    std::vector<uint8_t>    pixels;     // BGRA premultiplied, 32bpp
    ComPtr<ID2D1Bitmap1>    gpuBitmap;  // Created on main thread only
    std::wstring            filePath;
    bool                    hasAlpha = false;

    bool IsValid() const { return width > 0 && height > 0 && !pixels.empty(); }
    bool HasGpuBitmap() const { return gpuBitmap != nullptr; }
};

// ─── Animated Image Data ─────────────────────────────────────────────────────

struct AnimationFrame {
    std::vector<uint8_t>    pixels;       // BGRA premultiplied, 32bpp
    ComPtr<ID2D1Bitmap1>    gpuBitmap;    // Created on main thread
    uint32_t                width  = 0;
    uint32_t                height = 0;
    double                  delayMs = 100; // frame delay in milliseconds
};

struct AnimatedImageData {
    std::vector<AnimationFrame> frames;
    uint32_t                    loopCount = 0; // 0 = infinite
    double                      totalDurationMs = 0;
    std::wstring                filePath;

    bool IsValid() const { return !frames.empty(); }
    size_t FrameCount() const { return frames.size(); }
};

// ─── Async Load Request ──────────────────────────────────────────────────────

using ImageLoadCallback = std::function<void(std::shared_ptr<ImageData>)>;

// ─── Image Loader Class ──────────────────────────────────────────────────────

class ImageLoader {
public:
    ImageLoader() = default;
    ~ImageLoader();

    // Non-copyable
    ImageLoader(const ImageLoader&) = delete;
    ImageLoader& operator=(const ImageLoader&) = delete;

    /// Initialize WIC factory and thread pool
    bool Initialize(uint32_t threadCount = 4);

    /// Shutdown the thread pool and release resources
    void Shutdown();

    // ── Synchronous Loading ────────────────────────────────────────────────
    // Blocks until the image is fully decoded. Use for viewer page.

    /// Load a single image. Returns pixel data; gpuBitmap is NOT created yet.
    /// Call renderer.CreateBitmapFromMemory() or UploadToGpu() separately.
    [[nodiscard]]
    std::shared_ptr<ImageData> LoadSync(const std::wstring& path, uint32_t targetWidth = 0, uint32_t targetHeight = 0);

    /// Load an animated image (GIF, animated WebP, APNG).
    /// Frame GPU bitmaps are NOT created; call UploadAnimationToGpu().
    [[nodiscard]]
    std::shared_ptr<AnimatedImageData> LoadAnimatedSync(const std::wstring& path);

    // ── Asynchronous Loading ───────────────────────────────────────────────
    // Decoding runs on background threads. Callback fires when pixel data
    // is ready (still on the background thread — no GPU ops here).

    void LoadAsync(const std::wstring& path, ImageLoadCallback callback, uint32_t targetWidth = 0, uint32_t targetHeight = 0);

    /// Process completed async loads: creates GPU textures on the main thread.
    /// Call this once per frame from the main/render thread.
    void ProcessPendingTextures(Renderer& renderer);

    // ── GPU Upload ─────────────────────────────────────────────────────────
    // Must be called on the main/render thread only.

    /// Upload decoded pixel data to GPU, creating the ID2D1Bitmap.
    static void UploadToGpu(ImageData& image, Renderer& renderer);

    /// Upload all frames of an animated image to GPU.
    static void UploadAnimationToGpu(AnimatedImageData& anim, Renderer& renderer);

    // ── Format Support ─────────────────────────────────────────────────────

    /// Returns true if the file extension is a supported image format.
    static bool IsSupported(const std::wstring& path);

    /// Returns true if the file is potentially animated (GIF, WebP, APNG).
    static bool IsAnimatedFormat(const std::wstring& path);

    /// Get all supported file extensions (with dot, lowercase).
    static const std::vector<std::wstring>& GetSupportedExtensions();

private:
    // ── WIC Decoding (runs on any thread) ─────────────────────────────────
    std::shared_ptr<ImageData> DecodeImage(const std::wstring& path, uint32_t targetWidth = 0, uint32_t targetHeight = 0);
    std::shared_ptr<AnimatedImageData> DecodeAnimatedImage(const std::wstring& path);

    // ── WIC Helpers ───────────────────────────────────────────────────────
    ComPtr<IWICBitmapDecoder> CreateDecoder(const std::wstring& path);
    bool DecodeFrame(IWICBitmapFrameDecode* frame, ImageData& out, uint32_t targetWidth = 0, uint32_t targetHeight = 0);

    // ── Pending Texture Queue ─────────────────────────────────────────────
    struct PendingTexture {
        std::shared_ptr<ImageData> image;
        ImageLoadCallback callback;
    };

    std::mutex                     m_pendingMutex;
    std::vector<PendingTexture>    m_pendingTextures;

    // ── Thread Pool ───────────────────────────────────────────────────────
    std::vector<std::thread>       m_workers;
    std::queue<std::function<void()>> m_taskQueue;
    std::mutex                     m_queueMutex;
    std::condition_variable        m_queueCV;
    std::atomic<bool>              m_shutdown = false;

    void WorkerLoop();
    void EnqueueTask(std::function<void()> task);

    // ── WIC Factory ───────────────────────────────────────────────────────
    ComPtr<IWICImagingFactory2>    m_wicFactory;
};

} // namespace kv
