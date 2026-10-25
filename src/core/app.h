// ============================================================================
// Class: App
// Project: LenteUV Image Viewer
// Author: xPhang
// Based on: Sergkruma's image viewer by Sergei Krumas (github.com/sergkrumas)
//           Fork extended with Direct2D/Direct3D 11, XMP tagging, Board page,
//           multi-language support and Windows shell integration.
// Description: App is the central coordinator for the LenteUV application,
//              managing the main event loop, routing system messages, and
//              page navigation across Viewer, Board and Waterfall pages.
// Responsibility: Orchestrate the Win32 window message loop and delegate
//                 actions to page components.
// ============================================================================

#pragma once

#include "common/types.h"
#include "core/settings.h"
#include "core/folder_data_manager.h"
#include "core/window.h"
#include "rendering/renderer.h"
#include "loader/image_loader.h"
#include "rendering/animation_controller.h"
#include "viewer/viewer_page.h"
#include "ui/thumbnail_strip.h"
#include "board/board_page.h"
#include "waterfall/waterfall_page.h"
#include "core/single_instance_ipc.h"
#include "metadata/xmp_tag_manager.h"
#include "ui/options_menu.h"
#include "ui/tagging_modal.h"
#include "i18n/i18n_engine.h"

namespace kv {

// Persisted mode IDs: leave the retired value 4 unassigned; never compact remaining IDs.
enum class Page {
    Start = 1,
    Viewer = 2,
    Board = 3,
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

// Startup outcome: transport failure is distinct from normal forwarding and subsystem failure.
enum class InitResult {
    Success,
    AlreadyRunning,
    Failed,
    ForwardFailed
};

// ─── Application Class ──────────────────────────────────────────────────────

class App {
    friend class AsyncPreviewTests;
    friend class StartupCacheTests; // Exercises production async generations/cache with isolated data. // Deterministic callback delivery tests; no public test API.
    friend class IpcDiagnosticTests; // Observes applied image state through the real IPC handler.
public:
    // Binds folder/selection aliases to this App's owned shared folder service.
    App();
    // UI: stops IPC, joins Board/tag searches, then stops loader before destroying their dependencies.
    ~App();

    // Non-copyable
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    /// Parse command-line arguments into an AppConfig
    static AppConfig ParseCommandLine(int argc, wchar_t* argv[]);

    // UI: resolve IPC with a hidden HWND; after WIC, queue CPU decode/enumeration before graphics.
    // Retain reentrant paths until pages are ready; all results wait for normal main-thread delivery.
    // Forwarding exits skip services; partial failure releases retained paths with the callback/App.
    // Installs terminal close handling that stops IPC before Window destroys its HWND; hiding keeps services alive.
    InitResult Initialize(const AppConfig& config);

    // Reports the last forwarding open/write error for the startup failure dialog; no application ACK.
    DWORD GetIpcSendError() const { return m_ipc.GetDiagnostics().sendError; }

    // Runs UI/render; on quit stops IPC and joins scanners before loader and dependency teardown.
    int Run();

private:
    // UI thread: validates and applies an IPC path, then restores the window as in normal external opening.
    void OnIpcOpenPath(const std::wstring& rawPath);
    // ── Event Handlers ────────────────────────────────────────────────────
    void OnPaint();
    void OnResize(uint32_t width, uint32_t height);
    // UI: tracks radial hover/tooltip, then delegates movement to the active page.
    void OnMouseMove(float x, float y, KeyModifiers mods);
    // UI: dispatches overlays/radial/page clicks; the reserved radial slot consumes without action.
    void OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods);
    // UI: consumes radial releases and routes remaining releases to the active page.
    void OnMouseUp(MouseButton btn, float x, float y, KeyModifiers mods);
    // UI: routes scroll through overlays and the active page.
    void OnMouseWheel(float delta, float x, float y, KeyModifiers mods);
    // UI: delegates double-click gestures to the active page and viewer strip.
    void OnMouseDoubleClick(MouseButton btn, float x, float y, KeyModifiers mods);
    // UI: offers keys to editors/pages before global shortcuts; NUM2/2 have no navigation action.
    void OnKeyDown(uint32_t vk, KeyModifiers mods);
    void OnChar(wchar_t ch);
    // UI: loads dropped content using the existing preference; Board retains its own drop path.
    void OnFileDrop(std::vector<std::wstring> files);
    void OnMouseLeave();

    // ── Rendering ─────────────────────────────────────────────────────────
    // UI: updates active consumers, delivers pending textures, draws overlays and presents.
    void RenderFrame();
    // UI: draws Viewer, Board or Waterfall using renderer-owned resources.
    void DrawViewerContent();
    void DrawImageFitted(ID2D1Bitmap1* bitmap);
    void DrawCheckerboard(const RectF& rect);
    void DrawOverlayInfo();
    void DrawCloseButton();

    // ── Image Management ──────────────────────────────────────────────────
    // UI: queues foreground CPU decode; replaces the prior lease and leaves its displayed image intact until success.
    void LoadImage(const std::wstring& path);
    // UI admission before renderer initialization is allowed: CPU result waits for normal UI delivery.
    void BeginViewerLoad(const std::wstring& path);
    // UI after page initialization: queues folder data without delaying an already decoding original.
    void PrepareImageFolder(const std::wstring& path);
    // UI: commits a current CPU/GPU result without selecting a page or applying obsolete work.
    void ApplyLoadedImage(const std::wstring& path, const ViewerLoadResult& loaded);
    // UI: reconciles the latest requested image with a newly available folder snapshot.
    void OnFolderReady();
    bool m_asyncLoading = false;
    ImageLoadLifetime m_imageLoadLifetime;
    std::wstring m_requestedImagePath;
    uint64_t m_imageGeneration = 0, m_presentedGeneration = 0;
    bool m_originalFramePending = false;
    bool m_originalDrawnThisFrame = false;
    bool m_stripReadyDrawnThisFrame = false, m_waterfallReadyDrawnThisFrame = false;
    UINT m_originalPresentId = 0;
    uint64_t m_originalPresentGeneration = 0;
    // UI: queues/selects a shared folder; its snapshot later synchronizes navigation and optional image loading.
    void LoadFolder(const std::wstring& folderPath, bool loadImage = true);
    void NavigateImage(int direction); // +1 = next, -1 = previous
    // Persists the sort mode, reorders shared folders and restores the active image's selection.
    void ApplySortMode(int mode);
    // Removes a path from shared lists, cancels its hover, invalidates preview layouts and updates image/selection.
    void DeleteImage(const std::wstring& path);
    void AdjustWindowToImage();

    // ── Zoom / Pan ────────────────────────────────────────────────────────
    void ZoomToPoint(float factor, float screenX, float screenY);
    void ResetView();
    // UI: switches mode, cancels departing consumers' work and preserves ready caches/gestures.
    void SetPage(Page page);
    // UI: drops Waterfall hover lease/results on folder/page changes, preventing late same-path animation reuse.
    void CancelHoverLoads();
    RectF CalculateImageRect() const;

    // ── Subsystems ────────────────────────────────────────────────────────
    friend class ViewerPage;
    friend class ThumbnailStrip;
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
    FolderDataManager   m_folderData;
    ViewerPage          m_viewerPage;
    ThumbnailStrip      m_thumbnailStrip;
    BoardPage           m_boardPage;
    WaterfallPage       m_waterfallPage;
    SingleInstanceIpc   m_ipc;
    XmpTagManager       m_tagManager;
    OptionsMenu         m_optionsMenu;
    TaggingModal        m_taggingModal;

    // ── Routing & Folders State ───────────────────────────────────────────
    Page                              m_currentPage = Page::Viewer;
    // Aliases into m_folderData; the service is declared first and owns this state.
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
    // UI: draws three mode targets and a gray reserved slot with an informational tooltip.
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
