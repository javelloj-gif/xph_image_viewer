// ============================================================================
// Class: ImageLoader
// Description: ImageLoader implements multi-threaded image decoding using Windows Imaging Component (WIC) for [IMAGE_VIEWER_APP_NAME].
// Responsibility: Decode pixel files asynchronously, queuing raw bytes for main thread GPU texture uploading.
// ============================================================================
#pragma once

#include "common/types.h"
#include "common/image_request.h"
#include "rendering/renderer.h"

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

// Synchronous viewer result: at most one CPU candidate; validity is checked by App before upload.
struct ViewerLoadResult {
    std::shared_ptr<ImageData> image;
    std::shared_ptr<AnimatedImageData> animation;
};

// ─── Async Load Request ──────────────────────────────────────────────────────

using ImageLoadCallback = std::function<void(std::shared_ptr<ImageData>)>;
// UI-only, read-only validity check: must not throw or pump messages. False skips upload, not completion.
using ImageUploadPredicate = std::function<bool()>;
using AnimatedImageLoadCallback = std::function<void(std::shared_ptr<AnimatedImageData>)>;

// ─── Image Loader Class ──────────────────────────────────────────────────────

// Owns CPU workers and pending UI deliveries; terminal shutdown cancels unstarted work, never executing UI callbacks.
class ImageLoader {
    friend class ShutdownTests; // Controls task boundaries and inspects released queues in isolated processes.
    friend class AsyncPreviewTests; // Controls existing queues with zero workers in tests.
    friend class ImageDecodeTests; // Supplies a forwarding WIC factory for decoder counts/failure controls.
public:
    ImageLoader() = default;
    ~ImageLoader();

    // Non-copyable
    ImageLoader(const ImageLoader&) = delete;
    ImageLoader& operator=(const ImageLoader&) = delete;

    // Owner/UI thread: opens admission after WIC succeeds; only fresh or fully stopped instances may initialize.
    bool Initialize(uint32_t threadCount = 4);

    // Owner/UI thread, non-reentrant: rejects new work, discards queued tasks, joins active decodes,
    // then releases pending results without callbacks. Active tasks stop at the next cooperative boundary.
    // WIC already executing must return before join; codecs are never forcibly terminated.
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

    // Blocks on CPU using the existing extension policy; shares a successful decoder and releases it before fresh static fallback.
    // Returns animation only when valid; does not upload, enqueue work or change any viewer state.
    [[nodiscard]] ViewerLoadResult LoadForViewerSync(const std::wstring& path);

    // ── Asynchronous Loading ───────────────────────────────────────────────
    // Decoding runs on background threads. Callback fires when pixel data
    // is ready through ProcessPendingTextures() on the main/render thread,
    // after GPU upload (unless the optional predicate rejects it). Workers only decode and enqueue results.

    // While running, carries UI validity to delivery; callback receives the original CPU result, including failure.
    // Before initialization/after stop, submissions are ignored; shutdown cancels undelivered callbacks.
    // Captured consumers must outlive delivery, just as for the completion callback.
    // Optional weak lease cancels CPU work and delivery when its UI owner is removed; never capture the lease in callback.
    void LoadAsync(const std::wstring& path, ImageLoadCallback callback, uint32_t targetWidth = 0, uint32_t targetHeight = 0, ImageUploadPredicate isCurrent = {}, std::weak_ptr<const bool> lifetime = {});
    // Same lease contract for animation; cancellation discards partial frames without callback.
    void LoadAnimatedAsync(const std::wstring& path, AnimatedImageLoadCallback callback, std::weak_ptr<const bool> lifetime = {});

    // UI thread: removes obsolete queued requests and frees their captures outside locks; active calls observe cancellation.
    void CancelObsoleteRequests();

    /// UI: prunes obsolete tasks; canceled leases skip predicate/upload/callback. Validity-only rejection still delivers CPU result.
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

    /// Get the WIC factory pointer
    [[nodiscard]] IWICImagingFactory2* GetWicFactory() const { return m_wicFactory.Get(); }

private:
    // ── WIC Decoding (runs on any thread) ─────────────────────────────────
    // Opens one decoder and reads static frame 0 at the requested CPU size; no GPU work.
    std::shared_ptr<ImageData> DecodeImage(const std::wstring& path, uint32_t targetWidth = 0, uint32_t targetHeight = 0, ImageCancellation cancel = {});
    // Borrows a caller-owned decoder for frame 0; preserves the same scaling/conversion/error path.
    std::shared_ptr<ImageData> DecodeImage(IWICBitmapDecoder* decoder, const std::wstring& path, uint32_t targetWidth, uint32_t targetHeight, ImageCancellation cancel = {});
    // Opens a decoder and attempts animation only; preserves legacy invalid/empty-result behavior.
    std::shared_ptr<AnimatedImageData> DecodeAnimatedImage(const std::wstring& path, ImageCancellation cancel = {});
    // Borrows a decoder with count >1 and composes CPU frames using the existing GIF-oriented metadata/disposal rules.
    std::shared_ptr<AnimatedImageData> DecodeAnimatedFrames(IWICBitmapDecoder* decoder, const std::wstring& path, UINT frameCount, ImageCancellation cancel = {});

    // ── WIC Helpers ───────────────────────────────────────────────────────
    ComPtr<IWICBitmapDecoder> CreateDecoder(const std::wstring& path);
    // Converts one frame with checks between WIC stages; no partial result escapes after cancellation.
    bool DecodeFrame(IWICBitmapFrameDecode* frame, ImageData& out, uint32_t targetWidth = 0, uint32_t targetHeight = 0, ImageCancellation cancel = {});

    // ── Pending Texture Queue ─────────────────────────────────────────────
    // Owns CPU result and UI functions across worker-to-UI delivery; validity never runs on workers.
    struct PendingTexture {
        std::shared_ptr<ImageData> image;
        ImageLoadCallback callback;
        ImageUploadPredicate isCurrent;
        ImageCancellation cancel;
    };

    struct PendingAnimatedTexture {
        std::shared_ptr<AnimatedImageData> anim;
        AnimatedImageLoadCallback callback;
        ImageCancellation cancel;
    };

    std::mutex                     m_pendingMutex;
    std::vector<PendingTexture>    m_pendingTextures;
    std::vector<PendingAnimatedTexture> m_pendingAnimatedTextures;

    // ── Thread Pool ───────────────────────────────────────────────────────
    std::vector<std::thread>       m_workers;
    // Retains CPU work and its cancellation snapshot, allowing physical queue removal without calling UI predicates.
    struct DecodeTask {
        std::function<void()> work;
        ImageCancellation cancel;
        // Allows controlled delivery tests and workers to execute only still-current work.
        void operator()() const { if (!cancel.IsCancelled()) work(); }
    };
    std::queue<DecodeTask> m_taskQueue;
    std::mutex                     m_queueMutex;
    std::condition_variable        m_queueCV;
    std::atomic<bool>              m_shutdown = true;
    std::stop_source m_stopSource; // Replaced only after all workers join; task snapshots retain old stop state.

    // Worker-only: takes tasks until stopped; completes a task already taken, never drains queued work after stop.
    void WorkerLoop();
    // Serializes admission with shutdown; rejected/canceled tasks never invoke completion callbacks.
    void EnqueueTask(std::function<void()> task, ImageCancellation cancel = {});

    // ── WIC Factory ───────────────────────────────────────────────────────
    ComPtr<IWICImagingFactory2>    m_wicFactory;
};

} // namespace kv
