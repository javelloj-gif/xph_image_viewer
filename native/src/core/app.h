// ============================================================================
// Class: App
// Description: App is the central coordinator for the [IMAGE_VIEWER_APP_NAME] application, managing the main event loop, routing system messages, and page navigation.
// Responsibility: Orchestrate the Win32 window message loop and delegate actions to page components.
// ============================================================================
#pragma once

#include "common/types.h"
#include "core/settings.h"
#include "library/folder_data_manager.h"
#include "core/window.h"
#include "rendering/renderer.h"
#include "loader/image_loader.h"
#include "rendering/animation_controller.h"
#include "viewer/viewer_page.h"
#include "ui/thumbnail_strip.h"
#include "library/library_page.h"
#include "board/board_page.h"
#include "waterfall/waterfall_page.h"
#include "core/single_instance_ipc.h"
#include "metadata/xmp_tag_manager.h"
#include "ui/options_menu.h"
#include "ui/tagging_modal.h"
#include "i18n/i18n_engine.h"

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
    bool         multiple = false;
};

enum class InitResult {
    Success,
    AlreadyRunning,
    Failed
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
    InitResult Initialize(const AppConfig& config);

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
    void OnMouseLeave();

    // ── Rendering ─────────────────────────────────────────────────────────
    void RenderFrame();
    void DrawViewerContent();
    void DrawImageFitted(ID2D1Bitmap1* bitmap);
    void DrawCheckerboard(const RectF& rect);
    void DrawOverlayInfo();
    void DrawCloseButton();

    // ── Image Management ──────────────────────────────────────────────────
    void LoadImage(const std::wstring& path);
    void LoadFolder(const std::wstring& folderPath, bool loadImage = true);
    void NavigateImage(int direction); // +1 = next, -1 = previous
    void ApplySortMode(int mode);
    void DeleteImage(const std::wstring& path);
    void AdjustWindowToImage();

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
    friend class BoardSearchManager;
    friend class BoardTagScanner;
    friend class BoardLayoutHelper;
    friend class WaterfallPage;
    friend class FolderDataManager;
    friend class Settings;
    friend class OptionsMenu;
    friend class TaggingModal;
    Window              m_window;
    Renderer            m_renderer;
    ImageLoader         m_imageLoader;
    AnimationController m_animations;
    Timer               m_frameTimer;
    Settings            m_settings;
    FolderDataManager   m_libraryData;
    ViewerPage          m_viewerPage;
    ThumbnailStrip      m_thumbnailStrip;
    LibraryPage         m_libraryPage;
    BoardPage           m_boardPage;
    WaterfallPage       m_waterfallPage;
    SingleInstanceIpc   m_ipc;
    XmpTagManager       m_tagManager;
    OptionsMenu         m_optionsMenu;
    TaggingModal        m_taggingModal;

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
    bool     m_wasFullscreen = false;

    // ── Radial Menu fullscreen state ──────────────────────────────────────
    bool    m_radialMenuVisible = false;
    float   m_radialMenuProgress = 0.0f; // Animated 0.0 to 1.0
    int     m_radialMenuHoveredIdx = -1;  // -1 if none, 0 to 3
    void    DrawRadialMenu();
    PointF  GetRadialButtonCenter(int idx, float progress);

    // ── Cached Values ─────────────────────────────────────────────────────
    ComPtr<ID2D1Bitmap1>  m_checkerboardBitmap;  // cached checkerboard pattern

    // ── FPS Counter ───────────────────────────────────────────────────────
    double   m_fpsAccumulator = 0;
    int      m_fpsFrameCount = 0;
    double   m_currentFps = 0;
    bool     m_isQuitting = false;
};

} // namespace kv
