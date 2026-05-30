// ============================================================================
// app.h — Application Orchestrator
// Krumassan Image Viewer — Native C++20
//
// The App class owns and coordinates all subsystems:
//   - Window (Win32 frameless window)
//   - Renderer (D2D/D3D11 GPU rendering)
//   - ImageLoader (WIC async image loading)
//   - AnimationManager (property animations)
//   - FrameSequencer (animated image playback)
//
// It wires events between subsystems and implements the main render loop.
// In Phase 1, it provides a basic image viewer with zoom/pan functionality
// to demonstrate the full pipeline working end-to-end.
// ============================================================================
#pragma once

#include "types.h"
#include "settings.h"
#include "library_data.h"
#include "window.h"
#include "renderer.h"
#include "image_loader.h"
#include "animation.h"
#include "viewer_page.h"
#include "thumbnail_strip.h"
#include "library_page.h"
#include "board_page.h"
#include "waterfall_page.h"

namespace kv {

enum class Page {
    Start = 1,
    Viewer = 2,
    Board = 3,
    Library = 4,
    Waterfall = 5
};

// ─── Application Configuration ───────────────────────────────────────────────

struct AppConfig {
    std::wstring initialFilePath;   // image file to open at startup
    std::wstring initialFolderPath; // folder to browse at startup
    bool         frameless   = true;
    bool         translucent = true;
    bool         startMaximized = false;
    bool         startFullscreen = false;
};

// ─── Application Class ──────────────────────────────────────────────────────

class App {
public:
    App();
    ~App() = default;

    // Non-copyable
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    /// Parse command-line arguments into an AppConfig
    static AppConfig ParseCommandLine(int argc, wchar_t* argv[]);

    /// Initialize all subsystems: window, renderer, image loader, animations
    bool Initialize(const AppConfig& config);

    /// Run the main application loop. Returns exit code.
    int Run();

private:
    // ── Event Handlers ────────────────────────────────────────────────────
    void OnPaint();
    void OnResize(uint32_t width, uint32_t height);
    void OnMouseMove(float x, float y, KeyModifiers mods);
    void OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods);
    void OnMouseUp(MouseButton btn, float x, float y, KeyModifiers mods);
    void OnMouseWheel(float delta, float x, float y, KeyModifiers mods);
    void OnMouseDoubleClick(MouseButton btn, float x, float y, KeyModifiers mods);
    void OnKeyDown(uint32_t vk, KeyModifiers mods);
    void OnChar(wchar_t ch);
    void OnFileDrop(std::vector<std::wstring> files);

    // ── Rendering ─────────────────────────────────────────────────────────
    void RenderFrame();
    void DrawViewerContent();
    void DrawImageFitted(ID2D1Bitmap1* bitmap);
    void DrawCheckerboard(const RectF& rect);
    void DrawOverlayInfo();
    void DrawCloseButton();

    // ── Image Management ──────────────────────────────────────────────────
    void LoadImage(const std::wstring& path);
    void LoadFolder(const std::wstring& folderPath);
    void NavigateImage(int direction); // +1 = next, -1 = previous

    // ── Zoom / Pan ────────────────────────────────────────────────────────
    void ZoomToPoint(float factor, float screenX, float screenY);
    void ResetView();
    void SetPage(Page page);
    RectF CalculateImageRect() const;

    // ── Subsystems ────────────────────────────────────────────────────────
    friend class ViewerPage;
    friend class ThumbnailStrip;
    friend class LibraryPage;
    friend class BoardPage;
    friend class WaterfallPage;
    friend class LibraryData;
    friend class Settings;
    Window              m_window;
    Renderer            m_renderer;
    ImageLoader         m_imageLoader;
    AnimationManager    m_animations;
    Timer               m_frameTimer;
    Settings            m_settings;
    LibraryData         m_libraryData;
    ViewerPage          m_viewerPage;
    ThumbnailStrip      m_thumbnailStrip;
    LibraryPage         m_libraryPage;
    BoardPage           m_boardPage;
    WaterfallPage       m_waterfallPage;

    // ── Routing & Folders State ───────────────────────────────────────────
    Page                              m_currentPage = Page::Viewer;
    std::vector<FolderInfo>&          m_folders;
    int&                              m_currentFolderIdx;

    // ── Image State ───────────────────────────────────────────────────────
    std::shared_ptr<ImageData>        m_currentImage;
    std::shared_ptr<AnimatedImageData> m_animatedImage;
    FrameSequencer                    m_frameSequencer;
    std::vector<std::wstring>         m_fileList;     // files in current folder
    int                               m_currentIndex = -1;

    // ── View Transform (zoom / pan) ───────────────────────────────────────
    float    m_zoom    = 1.0f;
    PointF   m_pan     = {};   // translation offset
    float    m_rotation = 0.0f; // degrees

    // ── Interaction State ─────────────────────────────────────────────────
    bool     m_dragging = false;
    PointF   m_dragStart;
    PointF   m_panStart;
    float    m_mouseX = 0.0f;
    float    m_mouseY = 0.0f;

    // ── Cached Values ─────────────────────────────────────────────────────
    ComPtr<ID2D1Bitmap1>  m_checkerboardBitmap;  // cached checkerboard pattern
    float                 m_backgroundOpacity = 0.85f;

    // ── FPS Counter ───────────────────────────────────────────────────────
    double   m_fpsAccumulator = 0;
    int      m_fpsFrameCount = 0;
    double   m_currentFps = 0;
};

} // namespace kv
