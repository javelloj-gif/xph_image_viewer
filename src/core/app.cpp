// ============================================================================
// Class: App
// Description: App is the central coordinator for the Fotassa application, managing the main event loop, routing system messages, and page navigation.
// Responsibility: Orchestrate the Win32 window message loop and delegate actions to page components.
// ============================================================================
#include "common/pch.h"
#include "app.h"
#include "common/performance_trace.h"
#include "common/user_data_paths.h"
#include <algorithm>
#include <ranges>
#include "shell/shell_integrator.h"
#include "i18n/i18n_engine.h"
#include "board/board_tag_scanner.h"
#include <fstream>

namespace kv {

App::App()
    : m_folders(m_folderData.m_folders),
      m_currentFolderIdx(m_folderData.m_currentFolderIdx)
{
}

App::~App() {
    m_dockAccessibleState.reset();
    m_window.events.onIpcOpenPath = {};
    m_ipc.Shutdown();
    // Join producers while their page, metadata cache, WIC factory and window still exist.
    m_boardPage.m_tagScanner->CancelScan();
    m_tagManager.CancelSearch();
    m_folderData.StopScanning();
    m_imageLoadLifetime.reset();
    m_imageLoader.Shutdown();
}

// ─── Sorted natural image extensions for binary_search ──────────────────────

static bool IsImageExtension(const std::wstring& ext) {
    return ImageLoader::IsSupported(ext);
}

// ─── Parse Command Line ──────────────────────────────────────────────────────

AppConfig App::ParseCommandLine(int argc, wchar_t* argv[]) {
    AppConfig config;

    for (int i = 1; i < argc; ++i) {
        std::wstring arg = argv[i];

        if (arg == L"-frameless" || arg == L"--frameless") {
            config.frameless = true;
        } else if (arg == L"-windowed" || arg == L"--windowed") {
            config.frameless = false;
        } else if (arg == L"-opaque" || arg == L"--opaque") {
            config.translucent = false;
        } else if (arg == L"-maximized" || arg == L"--maximized") {
            config.startMaximized = true;
        } else if (arg == L"-fullscreen" || arg == L"--fullscreen") {
            config.startFullscreen = true;
        } else if (arg == L"-multiple" || arg == L"--multiple" || arg == L"-lite" || arg == L"--lite") {
            config.multiple = true;
        } else if (arg[0] != L'-') {
            // Treat as file or folder path
            namespace fs = std::filesystem;
            fs::path p(arg);
            if (fs::is_directory(p)) {
                config.initialFolderPath = p.wstring();
            } else if (fs::is_regular_file(p)) {
                config.initialFilePath = p.wstring();
            }
        }
    }

    return config;
}

// ─── Initialize ──────────────────────────────────────────────────────────────

InitResult App::Initialize(const AppConfig& config) {
    PerformanceSpan startupSpan("initialize.begin", "initialize.end");
    auto trace = [](std::string_view msg) {
        if (detail::g_enableDebugIpc) {
            std::ofstream dbg("debug_ipc.txt", std::ios::app);
            dbg << "[APP] Initialize step: " << msg << std::endl;
            dbg.close();
        }
    };
    trace("Start");

    // Load Settings
    m_settings.Load();
    trace("Settings loaded");
    I18nEngine::Initialize(m_settings.ui_lang);
    trace("I18n initialized");

    // ── Create Window ─────────────────────────────────────────────────────
    WindowConfig wndConfig;
    wndConfig.title       = L"Fotassa";
    wndConfig.width       = 1280;
    wndConfig.height      = 800;
    wndConfig.frameless   = config.frameless;
    wndConfig.translucent = config.translucent;

    trace("Calling m_window.Create");
    if (!m_window.Create(wndConfig)) {
        LogError(L"Failed to create window");
        trace("Failed to create window");
        return InitResult::Failed;
    }
    trace("Window created");

    // Win32/COM may dispatch during startup. Own early paths until every consumer is ready.
    auto startupPaths = std::make_shared<std::vector<std::wstring>>();
    m_window.events.onIpcOpenPath = [startupPaths](const std::wstring& path) {
        startupPaths->push_back(path);
    };

    // Enforce Single Instance
    if (m_settings.single_instance && !config.multiple) {
        std::wstring initialPath = config.initialFilePath.empty() ? config.initialFolderPath : config.initialFilePath;
        const auto ipcResult = m_ipc.Initialize(m_window.GetHwnd(), initialPath);
        if (ipcResult == IpcInitResult::ForwardFailed) {
            return InitResult::ForwardFailed;
        }
        if (ipcResult == IpcInitResult::Forwarded) {
            return InitResult::AlreadyRunning;
        }
    }

    m_folderData.Initialize(this);
    trace("FolderDataManager initialized");
    m_folderData.LoadFavorites();
    // Initialize Image Loader early to provide WIC Factory to TagManager
    if (!m_imageLoader.Initialize(4)) {
        LogError(L"Failed to initialize image loader");
        trace("Failed to initialize image loader");
        return InitResult::Failed;
    }
    trace("ImageLoader initialized");

    m_asyncLoading = true;
    m_folderData.EnableAsync();
    if (!config.initialFilePath.empty()) BeginViewerLoad(config.initialFilePath);
    try {
        auto cacheRoot = GetExecutableUserDataRoot() / L"cache" / L"thumbnails-v1";
        if (PerformanceTrace::Get().Enabled()) {
            const auto cacheNamespace = PerformanceTrace::Environment(L"IMAGE_VIEWER_CACHE_NAMESPACE");
            if (!cacheNamespace.empty() && cacheNamespace.find_first_not_of(L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_")==std::wstring::npos)
                cacheRoot /= cacheNamespace;
        }
        m_imageLoader.InitializeThumbnailCache(cacheRoot);
    } catch (...) { LogWarn(L"Thumbnail cache unavailable; source loading remains enabled"); }
    m_tagManager.Initialize(m_imageLoader.GetWicFactory());
    trace("Tags loaded");
    m_optionsMenu.Initialize(this);
    m_sortFilterModal.Initialize(this);
    trace("OptionsMenu initialized");
    m_taggingModal.Initialize(this);
    trace("TaggingModal initialized");

    // CPU enumeration overlaps graphics creation. No snapshots/textures are delivered here.
    m_viewerPage.Initialize(this);
    m_thumbnailStrip.Initialize(this);
    m_boardPage.Initialize(this);
    m_waterfallPage.Initialize(this);
    const bool loadSessionImage = config.initialFilePath.empty() && config.initialFolderPath.empty();
    const auto initialFolder = config.initialFilePath.empty() ? config.initialFolderPath
        : std::filesystem::path(config.initialFilePath).parent_path().wstring();
    m_folderData.LoadSession(loadSessionImage, initialFolder, config.initialFilePath.empty());
    if (initialFolder.empty() && m_folders.empty() && !m_folderData.HasPendingFolders()) {
        LoadFolder(std::filesystem::current_path().wstring());
    }

    // Register system tray icon
    ShellIntegrator::RegisterTrayIcon(
        m_window.GetHwnd(),
        LoadIconW(nullptr, IDI_APPLICATION),
        L"Fotassa"
    );

    // ── Wire Events ───────────────────────────────────────────────────────
    m_window.events.onPaint = [this]() { OnPaint(); };
    m_window.events.onResize = [this](uint32_t w, uint32_t h) { OnResize(w, h); };
    m_window.events.onMouseMove = [this](float x, float y, KeyModifiers m) { OnMouseMove(x, y, m); };
    m_window.events.onMouseDown = [this](MouseButton b, float x, float y, KeyModifiers m) { OnMouseDown(b, x, y, m); };
    m_window.events.onMouseUp = [this](MouseButton b, float x, float y, KeyModifiers m) { OnMouseUp(b, x, y, m); };
    m_window.events.onMouseWheel = [this](float d, float x, float y, KeyModifiers m) { OnMouseWheel(d, x, y, m); };
    m_window.events.onMouseDoubleClick = [this](MouseButton b, float x, float y, KeyModifiers m) { OnMouseDoubleClick(b, x, y, m); };
    m_window.events.onKeyDown = [this](uint32_t vk, KeyModifiers m) { OnKeyDown(vk, m); };
    m_window.events.onChar = [this](wchar_t ch) { OnChar(ch); };
    m_window.events.onFileDrop = [this](std::vector<std::wstring> f) { OnFileDrop(std::move(f)); };
    m_window.events.onClientHitTest = [this](float, float) { return IsViewerActive() || m_optionsMenu.IsVisible() || m_taggingModal.IsVisible(); };
    m_window.events.onGetObject = [this](WPARAM request, LPARAM objectId) { return GetDockAccessibleObject(request, objectId); };
    m_window.events.onMouseLeave = [this]() { OnMouseLeave(); };
    m_window.events.onCloseRequest = [this]() -> bool {
        if (m_settings.hide_to_tray_on_close && !m_isQuitting) {
            m_window.Show(SW_HIDE);
            return false;
        }
        // WM_CLOSE destroys the HWND before WM_QUIT; stop posts and release its payloads while it is alive.
        m_window.events.onIpcOpenPath = {};
        m_ipc.Shutdown();
        // Scanners can still invalidate this HWND; join them before Window handles WM_CLOSE destruction.
        m_boardPage.m_tagScanner->CancelScan();
        m_tagManager.CancelSearch();
        m_folderData.StopScanning();
        m_imageLoadLifetime.reset();
        return true;
    };
    m_window.events.onTrayIconEvent = [this](LPARAM lp) {
        if (lp == WM_LBUTTONDBLCLK) {
            m_window.Show();
            SetForegroundWindow(m_window.GetHwnd());
        } else if (lp == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            ShellIntegrator::ShowTrayContextMenu(m_window.GetHwnd(), pt);
        }
    };
    m_window.events.onCommand = [this](uint16_t id) {
        if (id == 1002) { // Quit from tray
            m_isQuitting = true;
            m_window.Close();
        }
    };

    // ── Initialize Renderer ───────────────────────────────────────────────
    PerformanceTrace::Event("renderer.initialize.begin");
    if (!m_renderer.Initialize(m_window.GetHwnd(), config.translucent)) {
        LogError(L"Failed to initialize renderer");
        return InitResult::Failed;
    }

    PerformanceTrace::Event("renderer.initialize.end");
    // ── Show Window ───────────────────────────────────────────────────────
    if (!m_settings.hide_on_app_start) {
        int cmdShow = SW_SHOW;
        if (config.startMaximized) cmdShow = SW_SHOWMAXIMIZED;
        m_window.Show(cmdShow);
        PerformanceTrace::Event("window.show");
    }

    if (config.startFullscreen || m_settings.show_fullscreen) {
        m_window.SetFullscreen(true);
    }

    SetPage(static_cast<Page>(m_settings.page_on_app_launch_from_explorer));

    m_frameTimer.Reset();
    m_window.events.onIpcOpenPath = [this](const std::wstring& rawPath) { OnIpcOpenPath(rawPath); };
    for (const auto& path : *startupPaths) OnIpcOpenPath(path);
    LogInfo(L"Application initialized successfully");
    return InitResult::Success;
}

void App::OnIpcOpenPath(const std::wstring& rawPath) {
    if (rawPath.empty()) return;

    namespace fs = std::filesystem;
    std::wstring path = fs::absolute(rawPath).lexically_normal().wstring();

    std::error_code ec;
    if (!fs::exists(path, ec)) return;

    if (fs::is_directory(path, ec)) {
        LoadFolder(path);
    } else if (fs::is_regular_file(path, ec)) {
        LoadImage(path);
        SetPage(Page::Viewer);
    } else {
        return;
    }

    if (IsIconic(m_window.GetHwnd())) {
        ShowWindow(m_window.GetHwnd(), SW_RESTORE);
    } else {
        m_window.Show();
    }

    if (m_window.IsFullscreen()) {
        m_window.SetFullscreen(true);
    } else {
        AdjustWindowToImage();
    }

    SetForegroundWindow(m_window.GetHwnd());
    m_window.RequestRedraw();
}

// ─── Main Loop ───────────────────────────────────────────────────────────────
// Polls deliveries/timers independently of drawing; visible transitions remain paced by Present.

int App::Run() {
    MSG msg = {};

    while (true) {
        // Process all pending messages without blocking
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                // ── Clean shutdown of background threads before returning ──
                // Stop new IPC deliveries before consumers are destroyed; discard undelivered payloads.
                m_window.events.onIpcOpenPath = {};
                m_ipc.Shutdown();
                m_boardPage.m_tagScanner->CancelScan();
                m_tagManager.CancelSearch();
                // Cancel queued decodes and wait for tasks already active; WIC itself is not interrupted.
                m_imageLoader.Shutdown();
                // Unregister tray icon.
                ShellIntegrator::UnregisterTrayIcon(m_window.GetHwnd());
                return static_cast<int>(msg.wParam);
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        if (!RenderFrame()) {
            // Existing producers publish on polling; keep their latency without redrawing stable/invisible content.
            MsgWaitForMultipleObjects(0, nullptr, FALSE, 16, QS_ALLINPUT);
        }
    }
}

// ─── Render Frame ────────────────────────────────────────────────────────────

bool App::RenderFrame() {
    // Guard against rendering before the window has a valid size
    auto targetSize = m_renderer.GetTargetSize();
    if (targetSize.width == 0 || targetSize.height == 0) return false;
    if (!m_renderer.IsInitialized()) return false;

    const auto folderRevision = m_folderData.GetPreviewLayoutRevision();
    m_folderData.ProcessPending();
    if (m_folderData.GetPreviewLayoutRevision() != folderRevision) m_window.RequestRedraw();
    double dt = m_frameTimer.Tick();
    // Clamp dt to avoid huge jumps after loading stalls
    dt = std::min(dt, 0.1);

    UpdateDock(dt);
    if (m_toastRemaining>0) { m_toastRemaining=std::max(0.0,m_toastRemaining-dt); m_window.RequestRedraw(); }

    // Update animations
    const bool animatedProperties = m_animations.HasActiveAnimations();
    m_animations.Update(dt);
    if (animatedProperties) m_window.RequestRedraw();

    // Update page animations
    if (m_currentPage == Page::Viewer) {
        m_viewerPage.Update(dt);

    } else if (m_currentPage == Page::Board) {
        m_boardPage.Update(dt);
    } else if (m_currentPage == Page::Waterfall) {
        m_waterfallPage.Update(dt);
    }

    if (IsViewerActive()) m_thumbnailStrip.Update(dt);

    // Update animated image playback
    if (m_animatedImage && !m_frameSequencer.IsPaused()) {
        const bool frameChanged = m_frameSequencer.Update(dt);
        // The visible progress bar advances between source frames; completed/background playback needs no drawing.
        if (IsViewerActive() && (frameChanged || (!m_frameSequencer.IsComplete() && m_frameSequencer.GetFrameCount() > 0))) {
            m_window.RequestRedraw();
        }
    }

    // Process any pending GPU texture uploads from background threads
    if (m_imageLoader.ProcessPendingTextures(m_renderer)) m_window.RequestRedraw();

    m_fpsAccumulator += dt;
    if (!m_window.ConsumeRedrawRequest()) {
        UpdateRenderDiagnostics();
        return false;
    }

    // FPS counter
    m_fpsFrameCount++;
    if (m_fpsAccumulator >= 1.0) {
        m_currentFps = m_fpsFrameCount / m_fpsAccumulator;
        m_fpsFrameCount = 0;
        m_fpsAccumulator = 0;
    }

    // Begin rendering
    m_renderer.BeginFrame();
    m_renderer.Clear(Color::Black(m_settings.window_opacity));

    // Draw content based on current state
    m_originalDrawnThisFrame=false;
    m_stripReadyDrawnThisFrame=false; m_waterfallReadyDrawnThisFrame=false;
    DrawViewerContent();
    if (m_currentPage!=Page::Board && m_fileList.empty() && m_currentFolderIdx>=0) {
        const auto size=m_renderer.GetTargetSize().ToFloat();
        m_renderer.DrawText(_(L"No matching images"),RectF::FromXYWH(16,110,size.width-32,34),Color::White(0.7f),14,TextAlign::Center,TextVAlign::Center);
    }
    DrawCloseButton();
    if (m_optionsMenu.IsVisible()) {
        m_optionsMenu.Draw(m_renderer);
    }
    if (m_taggingModal.IsVisible()) {
        m_taggingModal.Draw(m_renderer);
    }
    DrawDock();
    m_sortFilterModal.Draw(m_renderer);
    if (m_toastRemaining>0) {
        const auto size=m_renderer.GetTargetSize().ToFloat();
        auto r=RectF::FromXYWH(16,size.height*0.25f,std::max(0.0f,size.width-32),36);
        m_renderer.FillRoundedRect(r,6,6,Color::Black(0.7f));
        m_renderer.DrawText(_(L"Image sent to Board"),r,Color::White(),13,TextAlign::Center,TextVAlign::Center);
    }

    const auto presentResult=m_renderer.EndFrame();
    if (presentResult != S_OK) m_window.RequestRedraw();
    if (presentResult==S_OK) {
        if (m_stripReadyDrawnThisFrame) PerformanceTrace::Event("strip.viewport.submitted",m_imageGeneration);
        if (m_waterfallReadyDrawnThisFrame) PerformanceTrace::Event("waterfall.viewport.submitted",m_imageGeneration);
    }
    if (presentResult==S_OK && m_originalFramePending && m_originalDrawnThisFrame) {
        PerformanceTrace::Event("frame.original.submitted",m_presentedGeneration);
        if (PerformanceTrace::Get().Enabled()) {
            m_originalPresentId=m_renderer.GetLastPresentId();
            m_originalPresentGeneration=m_presentedGeneration;
            PerformanceTrace::Event("frame.original.present_id",m_presentedGeneration,m_originalPresentId);
        }
        m_originalFramePending=false;
        if (PerformanceTrace::Get().Enabled()) {
            const auto scenario=PerformanceTrace::Environment(L"IMAGE_VIEWER_PERF_SCENARIO");
            if (scenario==L"strip") m_thumbnailStrip.m_mouseMoveTimer=10.0;
            if (scenario==L"waterfall") { SetPage(Page::Waterfall); PerformanceTrace::Event("waterfall.demand",m_imageGeneration); }
        }
    }
    UpdateRenderDiagnostics();
    return true;
}

void App::UpdateRenderDiagnostics() {
    if (PerformanceTrace::Get().Enabled()) {
        DXGI_FRAME_STATISTICS stats{};
        if (m_originalPresentId && m_renderer.GetPresentationStatistics(stats) && stats.PresentCount==m_originalPresentId) {
            PerformanceTrace::Event("frame.original.display_confirmed",m_originalPresentGeneration,stats.PresentCount);
            m_originalPresentId=0;
        }
        if (PerformanceTrace::Get().ShouldExit(m_originalDrawnThisFrame && !m_originalFramePending,
            m_stripReadyDrawnThisFrame,m_waterfallReadyDrawnThisFrame)) {
            m_isQuitting = true; m_window.Close();
        }
    }
}

// ─── Draw Viewer Content ─────────────────────────────────────────────────────

bool App::IsViewerActive() const {
    return m_currentPage == Page::Viewer || (m_currentPage == Page::Waterfall && m_waterfallPage.m_viewerModal);
}

void App::DrawViewerComposition(Renderer& renderer) {
    m_viewerPage.Draw(renderer);
    m_thumbnailStrip.Draw(renderer);
}

void App::DrawViewerContent() {
    if (m_currentPage == Page::Viewer) {
        DrawViewerComposition(m_renderer);
    } else if (m_currentPage == Page::Board) {
        m_boardPage.Draw(m_renderer);
    } else if (m_currentPage == Page::Waterfall) {
        m_waterfallPage.Draw(m_renderer);
    }
}

// ─── Draw Image with Zoom/Pan/Rotate ─────────────────────────────────────────

void App::DrawImageFitted(ID2D1Bitmap1* bitmap) {
    if (!bitmap) return;

    auto imageRect = CalculateImageRect();

    // Apply zoom/pan/rotation transforms
    auto center = imageRect.Center();
    m_renderer.PushTranslation(m_pan.x, m_pan.y);
    m_renderer.PushScale(m_zoom, m_zoom, center);
    if (m_rotation != 0.0f) {
        m_renderer.PushRotation(m_rotation, center);
    }

    // Choose interpolation based on zoom level
    auto interp = (m_zoom > 4.0f)
        ? BitmapInterpolation::NearestNeighbor  // pixelated at high zoom
        : BitmapInterpolation::HighQuality;     // smooth at normal zoom

    m_renderer.DrawBitmap(bitmap, imageRect, 1.0f, interp);

    // Pop transforms (reverse order)
    if (m_rotation != 0.0f) m_renderer.PopTransform();
    m_renderer.PopTransform(); // scale
    m_renderer.PopTransform(); // translation
}

// ─── Calculate Image Rect (fit to window) ────────────────────────────────────

RectF App::CalculateImageRect() const {
    if (!m_currentImage && !m_animatedImage) return {};

    float imgW, imgH;
    if (m_animatedImage && !m_animatedImage->frames.empty()) {
        imgW = static_cast<float>(m_animatedImage->frames[0].width);
        imgH = static_cast<float>(m_animatedImage->frames[0].height);
    } else if (m_currentImage) {
        imgW = static_cast<float>(m_currentImage->width);
        imgH = static_cast<float>(m_currentImage->height);
    } else {
        return {};
    }

    auto screenSize = m_renderer.GetTargetSize().ToFloat();
    float scale = 1.0f;
    if (imgW > screenSize.width || imgH > screenSize.height) {
        float scaleX = screenSize.width / imgW;
        float scaleY = screenSize.height / imgH;
        scale = std::min(scaleX, scaleY);
    }

    float fitW = imgW * scale;
    float fitH = imgH * scale;
    float x = (screenSize.width - fitW) / 2.0f;
    float y = (screenSize.height - fitH) / 2.0f;

    return RectF::FromXYWH(x, y, fitW, fitH);
}

// ─── Checkerboard (cached — learned from Python optimization) ────────────────

void App::DrawCheckerboard(const RectF& rect) {
    // Create checkerboard pattern bitmap once
    if (!m_checkerboardBitmap) {
        constexpr int size = 20;  // 20x20 per square, 40x40 total
        constexpr int total = size * 2;
        std::vector<uint8_t> pixels(total * total * 4);

        const uint8_t lightGray = 200;
        const uint8_t darkGray  = 150;

        for (int y = 0; y < total; ++y) {
            for (int x = 0; x < total; ++x) {
                int idx = (y * total + x) * 4;
                bool isLight = ((x / size) + (y / size)) % 2 == 0;
                uint8_t c = isLight ? lightGray : darkGray;
                pixels[idx + 0] = c; // B
                pixels[idx + 1] = c; // G
                pixels[idx + 2] = c; // R
                pixels[idx + 3] = 255; // A
            }
        }

        m_checkerboardBitmap = m_renderer.CreateBitmapFromMemory(
            pixels.data(), total, total, total * 4);
    }

    if (!m_checkerboardBitmap) return;

    // Tile the checkerboard within the image rect
    m_renderer.PushClip(rect);
    float tileW = 40.0f, tileH = 40.0f;
    for (float y = rect.top; y < rect.bottom; y += tileH) {
        for (float x = rect.left; x < rect.right; x += tileW) {
            RectF tile = RectF::FromXYWH(x, y, tileW, tileH);
            m_renderer.DrawBitmap(m_checkerboardBitmap.Get(), tile, 1.0f,
                                  BitmapInterpolation::NearestNeighbor);
        }
    }
    m_renderer.PopClip();
}

// ─── Overlay Info ────────────────────────────────────────────────────────────

void App::DrawOverlayInfo() {
    auto size = m_renderer.GetTargetSize().ToFloat();

    // FPS counter and Opacity info (top-left)
    auto fpsText = std::format(L"FPS: {:.0f} | Opacity: {:.2f} (Use PageUp / PageDown to adjust)", m_currentFps, m_settings.window_opacity);
    RectF fpsRect = RectF::FromXYWH(10, 10, 500, 30);
    m_renderer.DrawText(fpsText, fpsRect, Color::FromRGBA(255, 255, 255, 180), 12.0f);

    // Image info (bottom-left)
    if (m_currentImage || m_animatedImage) {
        float fitW = CalculateImageRect().Width();
        float imgW = 1.0f;
        if (m_animatedImage && !m_animatedImage->frames.empty()) {
            imgW = static_cast<float>(m_animatedImage->frames[0].width);
        } else if (m_currentImage) {
            imgW = static_cast<float>(m_currentImage->width);
        }
        float displayZoom = (imgW > 0.0f && fitW > 0.0f) ? (m_zoom * fitW / imgW) : m_zoom;

        std::wstring info;
        if (m_animatedImage) {
            info = std::format(L"{} | Frame {}/{} | Zoom: {:.0f}%",
                std::filesystem::path(m_animatedImage->filePath).filename().wstring(),
                m_frameSequencer.GetCurrentFrame() + 1,
                m_frameSequencer.GetFrameCount(),
                displayZoom * 100.0f);
        } else if (m_currentImage) {
            info = std::format(L"{} | {}×{} | Zoom: {:.0f}%",
                std::filesystem::path(m_currentImage->filePath).filename().wstring(),
                m_currentImage->width, m_currentImage->height,
                displayZoom * 100.0f);
        }

        if (!m_fileList.empty()) {
            info += std::format(L" | [{}/{}]", m_currentIndex + 1, m_fileList.size());
        }

        RectF infoRect = RectF::FromXYWH(10, size.height - 35, size.width - 20, 25);

        // Semi-transparent background bar
        m_renderer.FillRoundedRect(
            RectF::FromXYWH(5, size.height - 40, size.width - 10, 35),
            6, 6, Color::Black(0.5f)
        );

        m_renderer.DrawText(info, infoRect, Color::White(0.9f), 13.0f,
                            TextAlign::Left, TextVAlign::Center);
    }
}

// ─── Close Button (top-right) ────────────────────────────────────────────────

void App::DrawCloseButton() {
    auto size = m_renderer.GetTargetSize().ToFloat();
    float btnSize = 32.0f;
    RectF btnRect = RectF::FromXYWH(size.width - btnSize - 8, 8, btnSize, btnSize);

    Color btnColor = Color::FromRGBA(200, 50, 50, 180);
    m_renderer.FillRoundedRect(btnRect, 6, 6, btnColor);

    // Draw X
    float margin = 10.0f;
    m_renderer.DrawLine(
        { btnRect.left + margin, btnRect.top + margin },
        { btnRect.right - margin, btnRect.bottom - margin },
        Color::White(), 2.0f
    );
    m_renderer.DrawLine(
        { btnRect.right - margin, btnRect.top + margin },
        { btnRect.left + margin, btnRect.bottom - margin },
        Color::White(), 2.0f
    );
}

// ─── Image Loading ───────────────────────────────────────────────────────────

void App::LoadImage(const std::wstring& path) {
    namespace fs = std::filesystem;

    PerformanceSpan imageSpan("image.load.begin", "image.load.end");
    LogInfo(std::format(L"Loading image: {}", path));

    if (m_asyncLoading) { BeginViewerLoad(path); PrepareImageFolder(path); return; }
    const auto loaded=m_imageLoader.LoadForViewerSync(path);
    ApplyLoadedImage(path,loaded);
}

void App::BeginViewerLoad(const std::wstring& path) {
    m_imageLoadLifetime=std::make_shared<const bool>(true);
    m_imageLoader.CancelObsoleteRequests();
    m_requestedImagePath=path;
    const auto generation=++m_imageGeneration;
    PerformanceTrace::Event("image.request",generation);
    m_imageLoader.LoadForViewerAsync(path,[this,path,generation](ViewerLoadResult result) {
        if (generation!=m_imageGeneration) return;
        const bool success=(result.image && result.image->IsValid() && result.image->HasGpuBitmap()) ||
            (result.animation && result.animation->IsValid() && result.animation->frames.front().gpuBitmap);
        if (success) { m_presentedGeneration=generation; m_originalFramePending=true; ApplyLoadedImage(path,result); }
        else PerformanceTrace::Event("image.failed",generation);
        if (generation==m_imageGeneration) m_imageLoadLifetime.reset();
    },m_imageLoadLifetime);
}
void App::PrepareImageFolder(const std::wstring& path) {
    if (std::find(m_fileList.begin(),m_fileList.end(),path)==m_fileList.end())
        LoadFolder(std::filesystem::path(path).parent_path().wstring(),false);
    else OnFolderReady();
}

void App::ApplyLoadedImage(const std::wstring& path, const ViewerLoadResult& loaded) {
    namespace fs=std::filesystem;
    {
        auto anim = loaded.animation;
        if (anim && anim->IsValid()) {
            // Upload all frames to GPU
            ImageLoader::UploadAnimationToGpu(*anim, m_renderer);

            m_animatedImage = anim;
            m_currentImage = nullptr;

            // Set up frame sequencer
            std::vector<double> delays;
            delays.reserve(anim->frames.size());
            for (const auto& f : anim->frames) delays.push_back(f.delayMs);
            m_frameSequencer.SetFrames(delays, anim->loopCount);

            ResetView();
            m_window.SetTitle(std::format(L"Fotassa — {} ({} frames)",
                fs::path(path).filename().wstring(), anim->FrameCount()));
            
            m_folderData.AddToHistory(path);
            m_thumbnailStrip.SyncSelection(m_currentIndex);
            m_window.RequestRedraw();
            return;
        }
    }

    // Load as static image
    auto image = loaded.image;
    if (image && image->IsValid()) {
        if (!image->HasGpuBitmap()) ImageLoader::UploadToGpu(*image, m_renderer);
        m_currentImage = image;
        m_animatedImage = nullptr;
        m_frameSequencer.Reset();
        ResetView();
        m_window.SetTitle(std::format(L"Fotassa — {} ({}×{})",
            fs::path(path).filename().wstring(), image->width, image->height));

        m_folderData.AddToHistory(path);
    } else {
        LogError(std::format(L"Failed to load: {}", path));
    }

    // Find current image in the list
    auto it = std::find(m_fileList.begin(), m_fileList.end(), path);
    if (it != m_fileList.end()) {
        m_currentIndex = static_cast<int>(std::distance(m_fileList.begin(), it));
    } else {
        // Build file list from the image's parent folder
        m_currentIndex=-1;
        if (!m_asyncLoading && m_currentFolderIdx<0) LoadFolder(fs::path(path).parent_path().wstring(), false);
        it = std::find(m_fileList.begin(), m_fileList.end(), path);
        if (it != m_fileList.end()) {
            m_currentIndex = static_cast<int>(std::distance(m_fileList.begin(), it));
        }
    }

    if (m_currentFolderIdx >= 0 && m_currentFolderIdx < static_cast<int>(m_folders.size())) {
        m_folders[m_currentFolderIdx].currentIndex = m_currentIndex;
    }

    m_thumbnailStrip.SyncSelection(m_currentIndex);
    if (!m_window.IsFullscreen()) {
        AdjustWindowToImage();
    }
    m_window.RequestRedraw();
}

void App::LoadFolder(const std::wstring& folderPath, bool loadImage) {
    if (m_asyncLoading && loadImage) { m_imageLoadLifetime.reset(); ++m_imageGeneration; m_requestedImagePath.clear(); }
    m_folderData.AddFolder(folderPath, false, loadImage);
    if (!m_asyncLoading) {
        m_thumbnailStrip.SetFileList(m_fileList);
        m_thumbnailStrip.SyncSelection(m_currentIndex);
    }
}


void App::OnFolderReady() {
    auto it=std::find(m_fileList.begin(),m_fileList.end(),m_requestedImagePath);
    if (!m_requestedImagePath.empty()) m_currentIndex=it==m_fileList.end() ? -1 : static_cast<int>(std::distance(m_fileList.begin(),it));
    if (m_currentFolderIdx>=0 && m_currentFolderIdx<static_cast<int>(m_folders.size())) m_folders[m_currentFolderIdx].currentIndex=m_currentIndex;
    m_thumbnailStrip.SyncSelection(m_currentIndex);
    if (!m_fileList.empty()) PerformanceTrace::Event("navigation.ready",m_imageGeneration,m_fileList.size());
}

void App::OnFileViewReady() { m_waterfallPage.CalculateLayout(); }

void App::ApplySortMode(int mode) { ApplyFileView(mode,m_settings.image_filter_mask); }
void App::ApplyFileView(int mode,int mask) {
    mode=std::clamp(mode,0,5); mask&=511;
    if (mode==m_settings.sort_mode && mask==m_settings.image_filter_mask) return;
    m_settings.sort_mode=mode; m_settings.image_filter_mask=mask; m_settings.Save();
    m_folderData.ApplyView(m_currentFolderIdx); m_window.RequestRedraw();
}

void App::DeleteImage(const std::wstring& path) {
    m_folderData.NotifyImageRemoved(path);
    if (m_asyncLoading && m_requestedImagePath==path) { m_imageLoadLifetime.reset(); ++m_imageGeneration; }
    if (m_waterfallPage.m_hoveredOriginalPath == path) CancelHoverLoads();
    for (auto& folder : m_folders) {
        auto itPath = std::find(folder.filePaths.begin(), folder.filePaths.end(), path);
        if (itPath != folder.filePaths.end()) {
            size_t idx = std::distance(folder.filePaths.begin(), itPath);
            folder.filePaths.erase(itPath);
            if (idx < folder.previews.size()) {
                folder.previews.erase(folder.previews.begin() + idx);
                m_folderData.NotifyPreviewLayoutChanged();
            }
            if (folder.currentIndex >= static_cast<int>(folder.filePaths.size())) {
                folder.currentIndex = std::max(0, static_cast<int>(folder.filePaths.size()) - 1);
            }
        }
    }

    bool wasActive = false;
    std::wstring activePath = L"";
    if (m_currentImage) {
        activePath = m_currentImage->filePath;
    } else if (m_animatedImage) {
        activePath = m_animatedImage->filePath;
    }

    if (activePath == path) {
        wasActive = true;
    }

    auto itFileList = std::find(m_fileList.begin(), m_fileList.end(), path);
    if (itFileList != m_fileList.end()) {
        m_fileList.erase(itFileList);
    }

    if (wasActive) {
        m_currentImage = nullptr;
        m_animatedImage = nullptr;
        m_frameSequencer.Reset();

        if (!m_fileList.empty()) {
            if (m_currentIndex >= static_cast<int>(m_fileList.size())) {
                m_currentIndex = 0;
            }
            if (m_currentIndex<0) m_currentIndex=0;
            LoadImage(m_fileList[m_currentIndex]);
        } else {
            m_currentIndex = -1;
            ResetView();
            m_window.SetTitle(L"Fotassa");
            m_waterfallPage.m_viewerModal = false;
        }
    } else {
        if (!m_fileList.empty() && !activePath.empty()) {
            auto it = std::find(m_fileList.begin(), m_fileList.end(), activePath);
            if (it != m_fileList.end()) {
                m_currentIndex = static_cast<int>(std::distance(m_fileList.begin(), it));
            } else {
                m_currentIndex = -1;
            }
        } else {
            m_currentIndex = -1;
        }
    }

    m_thumbnailStrip.SetFileList(m_fileList);
    m_thumbnailStrip.SyncSelection(m_currentIndex);

    m_waterfallPage.CalculateLayout();

    m_folderData.ApplyView(m_currentFolderIdx);
    m_window.RequestRedraw();
}

void App::AdjustWindowToImage() {
    if (!m_currentImage && !m_animatedImage) return;

    float imgW = 0.0f;
    float imgH = 0.0f;
    if (m_animatedImage && !m_animatedImage->frames.empty()) {
        imgW = static_cast<float>(m_animatedImage->frames[0].width);
        imgH = static_cast<float>(m_animatedImage->frames[0].height);
    } else if (m_currentImage) {
        imgW = static_cast<float>(m_currentImage->width);
        imgH = static_cast<float>(m_currentImage->height);
    }

    if (imgW <= 0.0f || imgH <= 0.0f) return;

    HMONITOR hMon = MonitorFromWindow(m_window.GetHwnd(), MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(hMon, &mi);

    int workAreaWidth = mi.rcWork.right - mi.rcWork.left;
    int workAreaHeight = mi.rcWork.bottom - mi.rcWork.top;

    // Minimum client size is 512x512
    int clientW = std::max(512, static_cast<int>(imgW));
    int clientH = std::max(512, static_cast<int>(imgH));
    float aspect = imgW / imgH;

    // Maximum client area calculation considering non-client area (borders, caption)
    int maxClientW = workAreaWidth;
    int maxClientH = workAreaHeight;
    if (!m_window.IsFrameless()) {
        DWORD style = GetWindowLongW(m_window.GetHwnd(), GWL_STYLE);
        DWORD exStyle = GetWindowLongW(m_window.GetHwnd(), GWL_EXSTYLE);
        RECT ncRect = { 0, 0, 0, 0 };
        AdjustWindowRectEx(&ncRect, style, FALSE, exStyle);
        int ncW = ncRect.right - ncRect.left;
        int ncH = ncRect.bottom - ncRect.top;
        maxClientW = std::max(100, workAreaWidth - ncW);
        maxClientH = std::max(100, workAreaHeight - ncH);
    }

    // Scale down client area preserving aspect ratio if it exceeds maximum client area
    if (clientW > maxClientW || clientH > maxClientH) {
        float maxAspect = static_cast<float>(maxClientW) / maxClientH;
        if (aspect > maxAspect) {
            clientW = maxClientW;
            clientH = static_cast<int>(clientW / aspect);
        } else {
            clientH = maxClientH;
            clientW = static_cast<int>(clientH * aspect);
        }
        clientW = std::max(512, clientW);
        clientH = std::max(512, clientH);
    }

    // Calculate window size from client size
    int targetW = clientW;
    int targetH = clientH;
    if (!m_window.IsFrameless()) {
        RECT rc = { 0, 0, clientW, clientH };
        DWORD style = GetWindowLongW(m_window.GetHwnd(), GWL_STYLE);
        DWORD exStyle = GetWindowLongW(m_window.GetHwnd(), GWL_EXSTYLE);
        AdjustWindowRectEx(&rc, style, FALSE, exStyle);
        targetW = rc.right - rc.left;
        targetH = rc.bottom - rc.top;
    }

    // Clamp final window size to work area bounds
    if (targetW > workAreaWidth) targetW = workAreaWidth;
    if (targetH > workAreaHeight) targetH = workAreaHeight;

    // Centering window in work area
    int targetX = mi.rcWork.left + (workAreaWidth - targetW) / 2;
    int targetY = mi.rcWork.top + (workAreaHeight - targetH) / 2;

    SetWindowPos(m_window.GetHwnd(), nullptr, targetX, targetY, targetW, targetH,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

void App::NavigateImage(int direction) {
    if (m_fileList.empty()) return;

    if (m_currentIndex==-1) m_currentIndex=direction<0 ? static_cast<int>(m_fileList.size()) : -1;
    m_currentIndex += direction;
    if (m_currentIndex < 0) m_currentIndex = static_cast<int>(m_fileList.size()) - 1;
    if (m_currentIndex >= static_cast<int>(m_fileList.size())) m_currentIndex = 0;

    if (m_currentFolderIdx >= 0 && m_currentFolderIdx < static_cast<int>(m_folders.size())) {
        m_folders[m_currentFolderIdx].currentIndex = m_currentIndex;
    }

    LoadImage(m_fileList[m_currentIndex]);
}

// ─── Zoom / Pan ──────────────────────────────────────────────────────────────

void App::ZoomToPoint(float factor, float screenX, float screenY) {
    m_viewerPage.ZoomToPoint(factor, screenX, screenY);
}

void App::ResetView() {
    m_viewerPage.ResetView();
}

void App::CancelHoverLoads() {
    m_waterfallPage.m_hoverLoadLifetime.reset();
    m_waterfallPage.m_hoveredOriginalPath.clear();
    m_waterfallPage.m_hoveredPreviewIdx = -1;
    m_waterfallPage.m_hoveredOriginalAnim.reset();
    m_waterfallPage.m_hoveredOriginalLoading = false;
}

void App::SetPage(Page page) {
    if (page != m_currentPage) {
        if (IsViewerActive()) { m_viewerPage.CancelPointerGesture(); m_thumbnailStrip.CancelPointerGesture(); }
        m_waterfallPage.m_viewerModal = false;
        m_dockFocus = m_dockHovered = -1; m_dockOpacity = 0; m_dockRemaining = 0;
        CancelHoverLoads();
        if (page != Page::Waterfall) m_folderData.CancelPreviewLoads();
        if (page != Page::Viewer) {
            m_thumbnailStrip.CancelPendingLoads();
        }
        if (m_currentPage == Page::Board) {
            for (auto& item : m_boardPage.m_items) {
                item.loadLifetime.reset();
                item.pendingRequest.reset();
                item.isLoading = false;
            }
            m_boardPage.m_tagScanner->CancelScan();
            m_tagManager.CancelSearch();
        }
    }
    m_currentPage = page;
    if (page == Page::Viewer) {
        m_thumbnailStrip.SetFileList(m_fileList);
        m_thumbnailStrip.SyncSelection(m_currentIndex);
    } else if (page == Page::Board) {
        m_boardPage.OnEnterPage();
    } else if (page == Page::Waterfall) {
        m_waterfallPage.CalculateLayout();
    }
    m_window.RequestRedraw();
}

// ─── Event Handlers ──────────────────────────────────────────────────────────

void App::OnPaint() {
}

void App::OnResize(uint32_t width, uint32_t height) {
    m_renderer.Resize(width, height);
    
    bool isFullscreen = m_window.IsFullscreen();
    bool wasFullscreen = m_wasFullscreen;
    m_wasFullscreen = isFullscreen;
    
    if (wasFullscreen && !isFullscreen) {
        AdjustWindowToImage();
    }
    if (m_optionsMenu.IsVisible()) {
        m_optionsMenu.BuildControls();
    }
    m_window.RequestRedraw();
}

void App::OnMouseMove(float x, float y, KeyModifiers mods) {
    if (m_sortFilterModal.OnMouseMove(x,y)) return;
    m_mouseX = x;
    m_mouseY = y;
    if (m_taggingModal.IsVisible()) {
        if (m_taggingModal.OnMouseMove(x, y, mods)) return;
    }
    if (m_optionsMenu.IsVisible()) {
        if (m_optionsMenu.OnMouseMove(x, y, mods)) return;
    }

    const bool overDockTooltip = m_dockOpacity > 0 && (m_dockHovered >= 0 || m_dockFocus >= 0) && GetDockTooltipRect().Contains(x, y);
    if (!overDockTooltip) m_dockHovered = HitDock(x, y);
    if (IsDockHit(x, y)) {
        m_window.RequestRedraw();
        return;
    }

    if (IsViewerActive() && m_thumbnailStrip.HandleMouseMove(x, y, mods)) return;
    if (m_currentPage == Page::Board) {
        if (m_boardPage.OnMouseMove(x, y, mods)) return;
    } else if (m_currentPage == Page::Waterfall) {
        if (m_waterfallPage.OnMouseMove(x, y, mods)) return;
    } else {
        m_viewerPage.OnMouseMove(x, y, mods);
    }
}

void App::OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (m_sortFilterModal.OnMouseDown(btn,x,y)) return;
    m_mouseX = x;
    m_mouseY = y;

    if (m_taggingModal.IsVisible()) {
        if (m_taggingModal.OnMouseDown(btn, x, y, mods)) return;
    }
    if (m_optionsMenu.IsVisible()) {
        if (m_optionsMenu.OnMouseDown(btn, x, y, mods)) return;
    }

    if (IsDockHit(x, y)) {
        m_dockFocus = -1;
        m_dockPressed = btn == MouseButton::Left ? HitDock(x, y) : -1;
        m_window.RequestRedraw();
        return;
    }
    m_dockFocus = -1;

    // Global close button check
    if (btn == MouseButton::Left) {
        auto size = m_renderer.GetTargetSize().ToFloat();
        RectF closeBtn = RectF::FromXYWH(size.width - 40.0f, 8.0f, 32.0f, 32.0f);
        if (closeBtn.Contains(x, y)) {
            m_isQuitting = true;
            m_window.Close();
            return;
        }
    }

    if (IsViewerActive() && m_thumbnailStrip.HandleMouseDown(btn, x, y, mods)) return;
    if (m_currentPage == Page::Board) {
        if (m_boardPage.OnMouseDown(btn, x, y, mods)) return;
    } else if (m_currentPage == Page::Waterfall) {
        if (m_waterfallPage.OnMouseDown(btn, x, y, mods)) return;
    } else {
        m_viewerPage.OnMouseDown(btn, x, y, mods);
    }
}

void App::OnMouseUp(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (m_sortFilterModal.IsVisible()) return;
    m_mouseX = x;
    m_mouseY = y;
    if (m_taggingModal.IsVisible()) {
        if (m_taggingModal.OnMouseUp(btn, x, y, mods)) return;
    }
    if (m_optionsMenu.IsVisible()) {
        if (m_optionsMenu.OnMouseUp(btn, x, y, mods)) return;
    }

    if (m_dockPressed >= 0) {
        const int pressed = m_dockPressed;
        m_dockPressed = -1;
        if (btn == MouseButton::Left && HitDock(x, y) == pressed) InvokeDock(pressed);
        return;
    }
    if (IsDockHit(x, y)) return;

    if (IsViewerActive() && m_thumbnailStrip.HandleMouseUp(btn, x, y, mods)) return;
    if (m_currentPage == Page::Board) {
        if (m_boardPage.OnMouseUp(btn, x, y, mods)) return;
    } else if (m_currentPage == Page::Waterfall) {
        if (m_waterfallPage.OnMouseUp(btn, x, y, mods)) return;
    } else {
        m_viewerPage.OnMouseUp(btn, x, y, mods);
    }
}

void App::OnMouseWheel(float delta, float x, float y, KeyModifiers mods) {
    if (m_sortFilterModal.IsVisible()) return;
    m_mouseX = x;
    m_mouseY = y;
    if (m_taggingModal.IsVisible()) {
        if (m_taggingModal.OnMouseWheel(delta, x, y, mods)) return;
    }
    if (m_optionsMenu.IsVisible()) return;
    if (IsDockHit(x, y)) return;
    if (IsViewerActive() && m_thumbnailStrip.HandleMouseWheel(delta, x, y, mods)) return;
    if (m_currentPage == Page::Board) {
        if (m_boardPage.OnMouseWheel(delta, x, y, mods)) return;
    } else if (m_currentPage == Page::Waterfall) {
        if (m_waterfallPage.OnMouseWheel(delta, x, y, mods)) return;
    } else {
        m_viewerPage.OnMouseWheel(delta, x, y, mods);
    }
}

void App::OnMouseDoubleClick(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (m_sortFilterModal.IsVisible()) return;
    m_mouseX = x;
    m_mouseY = y;
    if (m_taggingModal.IsVisible() || m_optionsMenu.IsVisible()) return;
    if (IsDockHit(x, y)) return;
    if (IsViewerActive() && m_thumbnailStrip.HandleMouseDoubleClick(btn, x, y, mods)) return;
    if (m_currentPage == Page::Board) {
        if (m_boardPage.OnMouseDoubleClick(btn, x, y, mods)) return;
    } else if (m_currentPage == Page::Waterfall) {
        if (m_waterfallPage.OnMouseDoubleClick(btn, x, y, mods)) return;
    } else {
        m_viewerPage.OnMouseDoubleClick(btn, x, y, mods);
    }
}

void App::OnKeyDown(uint32_t vk, KeyModifiers mods) {
    if (m_sortFilterModal.OnKeyDown(vk,mods)) return;
    if (m_taggingModal.IsVisible()) {
        if (m_taggingModal.OnKeyDown(vk, mods)) return;
    }
    if (m_optionsMenu.IsVisible()) {
        if (m_optionsMenu.OnKeyDown(vk, mods)) return;
    }
    if (HandleDockKey(vk, mods)) return;
    if (IsViewerActive() && m_thumbnailStrip.IsMouseOver(m_mouseX, m_mouseY) && m_thumbnailStrip.HandleKeyDown(vk, mods)) return;
    if (m_currentPage == Page::Viewer) {
        if (m_viewerPage.OnKeyDown(vk, mods)) {
            return;
        }
    } else if (m_currentPage == Page::Board) {
        if (m_boardPage.OnKeyDown(vk, mods)) {
            return;
        }
    } else if (m_currentPage == Page::Waterfall) {
        if (m_waterfallPage.OnKeyDown(vk, mods)) {
            return;
        }
    }

    switch (vk) {
        case VK_DELETE: {
            std::wstring targetPath = L"";
            if (m_currentPage == Page::Viewer) {
                if (m_currentImage) targetPath = m_currentImage->filePath;
                else if (m_animatedImage) targetPath = m_animatedImage->filePath;
            } else if (m_currentPage == Page::Waterfall) {
                if (m_waterfallPage.m_viewerModal) {
                    if (m_currentImage) targetPath = m_currentImage->filePath;
                    else if (m_animatedImage) targetPath = m_animatedImage->filePath;
                } else if (m_waterfallPage.m_hoveredPreviewIdx != -1 && m_currentFolderIdx >= 0) {
                    auto& folder = m_folders[m_currentFolderIdx];
                    if (m_waterfallPage.m_hoveredPreviewIdx < static_cast<int>(folder.filePaths.size())) {
                        targetPath = folder.filePaths[m_waterfallPage.m_hoveredPreviewIdx];
                    }
                }
            }

            if (!targetPath.empty()) {
                bool isFullscreen = m_window.IsFullscreen();
                int result = MessageBoxW(m_window.GetHwnd(),
                    _(L"Send this image to the Recycle Bin?").c_str(),
                    _(L"Delete image").c_str(),
                    MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1);
                
                if (isFullscreen && !m_window.IsFullscreen()) {
                    m_window.SetFullscreen(true);
                }

                if (result == IDYES) {
                    if (ShellIntegrator::SendFileToRecycleBin(targetPath)) {
                        DeleteImage(targetPath);
                    }
                }
            }
            break;
        }
        case VK_NUMPAD1:
            SetPage(Page::Viewer);
            break;
        case VK_NUMPAD2:
        case '2':
            break; // Reserved navigation keys; WM_CHAR remains available to editors.
        case VK_NUMPAD3:
        case '3':
            SetPage(Page::Waterfall);
            break;
        case VK_NUMPAD4:
        case '4':
            SetPage(Page::Board);
            break;


        case VK_ESCAPE:
            // Always quit on Escape — do not hide to tray.
            m_isQuitting = true;
            m_window.Close();
            break;

        case VK_LEFT:
        case VK_UP:
            NavigateImage(-1);
            break;

        case VK_RIGHT:
        case VK_DOWN:
            NavigateImage(1);
            break;

        case VK_HOME:
            if (!m_fileList.empty()) { m_currentIndex = -1; NavigateImage(1); }
            break;
        case VK_END:
            if (!m_fileList.empty()) { m_currentIndex = static_cast<int>(m_fileList.size()); NavigateImage(-1); }
            break;

        case 'F':
        case VK_F11:
            m_window.ToggleFullscreen();
            break;

        case VK_OEM_PLUS:
        case VK_ADD: {
            auto size = m_renderer.GetTargetSize().ToFloat();
            ZoomToPoint(1.07f, size.width / 2, size.height / 2);
            break;
        }

        case VK_OEM_MINUS:
        case VK_SUBTRACT: {
            auto size = m_renderer.GetTargetSize().ToFloat();
            ZoomToPoint(1.0f / 1.07f, size.width / 2, size.height / 2);
            break;
        }

        case 'O':
        case VK_OEM_3: // Tilde key '~'
            if (m_optionsMenu.IsVisible()) {
                m_optionsMenu.Hide();
            } else {
                m_optionsMenu.Show();
            }
            break;

        case 'T':
            if (m_taggingModal.IsVisible()) {
                m_taggingModal.Hide();
            } else {
                std::wstring selectedPath = L"";
                std::vector<std::wstring> boardSelectedPaths;
                if (m_currentPage == Page::Viewer) {
                    if (m_currentImage) selectedPath = m_currentImage->filePath;
                    else if (m_animatedImage) selectedPath = m_animatedImage->filePath;
                } else if (m_currentPage == Page::Waterfall) {
                    if (m_waterfallPage.m_viewerModal) {
                        if (m_currentImage) selectedPath = m_currentImage->filePath;
                        else if (m_animatedImage) selectedPath = m_animatedImage->filePath;
                    } else if (m_waterfallPage.m_hoveredPreviewIdx != -1) {
                        selectedPath = m_waterfallPage.m_hoveredOriginalPath;
                    }
                } else if (m_currentPage == Page::Board) {
                    for (const auto& item : m_boardPage.m_items) {
                        if (item.selected && item.type == BoardItemType::Image && !item.filePath.empty()) {
                            if (ImageLoader::IsSupported(item.filePath)) {
                                boardSelectedPaths.push_back(item.filePath);
                            }
                        }
                    }
                }
                
                if (m_currentPage == Page::Board) {
                    if (!boardSelectedPaths.empty()) {
                        m_taggingModal.Show(boardSelectedPaths);
                    }
                } else {
                    if (!selectedPath.empty() && ImageLoader::IsSupported(selectedPath)) {
                        m_taggingModal.Show(selectedPath);
                    }
                }
            }
            break;

        case VK_PRIOR: // Page Up
            m_settings.window_opacity = std::min(1.0f, m_settings.window_opacity + 0.05f);
            m_settings.Save();
            m_window.RequestRedraw();
            break;

        case VK_NEXT: // Page Down
            m_settings.window_opacity = std::max(0.0f, m_settings.window_opacity - 0.05f);
            m_settings.Save();
            m_window.RequestRedraw();
            break;

        case '0': case VK_NUMPAD0:
            ResetView();
            m_window.RequestRedraw();
            break;

        case 'R':
            m_viewerPage.m_rotation += mods.shift ? -90.0f : 90.0f;
            m_window.RequestRedraw();
            break;

        case VK_SPACE:
            if (m_animatedImage) {
                m_frameSequencer.TogglePause();
            }
            break;
    }
}

void App::OnChar(wchar_t ch) {
    if (m_sortFilterModal.IsVisible()) return;
    if (m_taggingModal.IsVisible()) {
        m_taggingModal.OnChar(ch);
        return;
    }
    if (m_currentPage == Page::Board) {
        m_boardPage.OnChar(ch);
    }
}

void App::OnFileDrop(std::vector<std::wstring> files) {
    if (m_sortFilterModal.IsVisible()) return;
    if (files.empty()) return;

    if (m_currentPage == Page::Board) {
        for (const auto& file : files) {
            m_boardPage.OnFileDrop(file, m_mouseX, m_mouseY);
        }
        return;
    }

    namespace fs = std::filesystem;

    if (fs::is_directory(files[0])) {
        LoadFolder(files[0]);
    } else {
        m_fileList.clear(); // will be rebuilt by LoadImage
        m_currentIndex = -1;
        LoadImage(files[0]);
    }

    SetPage(static_cast<Page>(m_settings.page_to_open_on_drag_and_drop));
}

void App::OnMouseLeave() {
    m_mouseX = m_mouseY = -1;
    m_dockHovered = -1;
    if (IsViewerActive()) {
        m_thumbnailStrip.HandleMouseLeave();
    }
}

bool App::HasDock() const { return m_currentPage==Page::Viewer || m_currentPage==Page::Waterfall || m_currentPage==Page::Board; }
App::DockAction App::GetDockAction(int index) const {
    const bool board=m_currentPage==Page::Board;
    const bool image=IsViewerActive() && (m_currentImage || m_animatedImage);
    const bool selection=board && std::any_of(m_boardPage.m_items.begin(),m_boardPage.m_items.end(),[](const BoardItem& item) {
        return item.selected && item.type==BoardItemType::Image && ImageLoader::IsSupported(item.filePath);
    });
    if (index==0) return m_currentPage==Page::Viewer ? DockAction{DockCommand::Waterfall,0,L"Waterfall (NUM3)",false} : DockAction{DockCommand::Viewer,6,L"Viewer (NUM1)",false};
    if (index==1) return board ? DockAction{DockCommand::Waterfall,0,L"Waterfall (NUM3)",false} : DockAction{DockCommand::Board,1,L"Board (NUM4)",false};
    if (index==2) return {DockCommand::Rotate,2,L"Rotate 90° (R)",!image};
    if (index==3) return {DockCommand::Tags,3,L"Tags (T)",!(image || selection)};
    if (index==4) return {DockCommand::SortFilter,4,L"Sort & Filter",board};
    return {DockCommand::Settings,5,L"Settings (O)",false};
}

LRESULT App::GetDockAccessibleObject(WPARAM request, LPARAM objectId) {
    if (static_cast<LONG>(objectId) != OBJID_CLIENT) return 0;
    if (!m_dockAccessibleState) {
        m_dockAccessibleState = std::make_shared<DockAccessibleState>();
        auto& state = *m_dockAccessibleState;
        state.hwnd = m_window.GetHwnd();
        state.visible = [this] { return HasDock() && m_dockOpacity > 0 && !m_optionsMenu.IsVisible() && !m_taggingModal.IsVisible() && !m_sortFilterModal.IsVisible(); };
        state.disabled = [this](int index) { return GetDockAction(index).disabled; };
        state.name = [this](int index) { return index<0 ? _(L"Navigation actions") : _(GetDockAction(index).name); };
        state.rect = [this](int index) { const auto r = index < 0 ? GetDockBounds() : GetDockButtonRect(index); return RECT{static_cast<LONG>(r.left),static_cast<LONG>(r.top),static_cast<LONG>(r.right),static_cast<LONG>(r.bottom)}; };
        state.invoke = [this](int index) { InvokeDock(index); };
    }
    auto* accessible = new DockAccessibility(m_dockAccessibleState);
    const auto result = LresultFromObject(IID_IAccessible, request, accessible);
    accessible->Release();
    return result;
}

RectF App::GetDockBounds() const {
    const float scale = m_window.GetDpi() / 96.0f;
    const float width = m_renderer.GetTargetSize().ToFloat().width;
    const int preferred=std::clamp(static_cast<int>((width / scale - 76) / 46),1,6);
    const int rows=std::max(1,static_cast<int>((m_renderer.GetTargetSize().ToFloat().height/scale-24)/46));
    const int fits=std::clamp(static_cast<int>((width/scale-12)/46),1,6);
    const int columns=std::min(fits,std::max(preferred,(6+rows-1)/rows));
    const float dockWidth = (12 + columns * 46 - 2) * scale;
    return RectF::FromXYWH((width - dockWidth) / 2, 12 * scale, dockWidth, (12 + ((6 + columns - 1) / columns) * 46 - 2) * scale);
}

RectF App::GetDockButtonRect(int index) const {
    const float scale = m_window.GetDpi() / 96.0f;
    const auto bounds = GetDockBounds();
    const int columns = std::clamp(static_cast<int>((bounds.Width() / scale - 10) / 46), 1, 6);
    return RectF::FromXYWH(bounds.left + (6 + index % columns * 46) * scale,
        bounds.top + (6 + index / columns * 46) * scale, 44 * scale, 44 * scale);
}

int App::HitDock(float x, float y) const {
    if (!HasDock() || m_dockOpacity <= 0) return -1;
    for (int index = 0; index < 6; ++index) if (GetDockButtonRect(index).Contains(x, y)) return index;
    return -1;
}

RectF App::GetDockTooltipRect() const {
    const float scale = m_window.GetDpi() / 96.0f;
    const auto bounds = GetDockBounds();
    const float width = std::max(0.0f, std::min(260 * scale, m_renderer.GetTargetSize().ToFloat().width - 24 * scale));
    const float left = std::clamp(bounds.Center().x - width / 2, 0.0f, std::max(0.0f, m_renderer.GetTargetSize().ToFloat().width - width));
    return RectF::FromXYWH(left, bounds.bottom + 6 * scale, width, 28 * scale);
}

bool App::IsDockHit(float x, float y) const {
    return HasDock() && m_dockOpacity > 0 && (GetDockBounds().Contains(x, y) || ((m_dockHovered >= 0 || m_dockFocus >= 0) && GetDockTooltipRect().Contains(x, y)));
}

void App::UpdateDock(double dt) {
    if (!HasDock()) {
        m_dockOpacity = 0; m_dockRemaining = 0; m_dockHovered = m_dockFocus = -1;
        return;
    }
    const auto bounds = GetDockBounds();
    const float scale = m_window.GetDpi() / 96.0f;
    const bool trigger = m_mouseX >= bounds.left && m_mouseX <= bounds.right && m_mouseY >= 0 && m_mouseY <= 48 * scale;
    const bool retained = trigger || IsDockHit(m_mouseX, m_mouseY) || m_dockFocus >= 0 || m_optionsMenu.IsVisible() || m_taggingModal.IsVisible() || m_sortFilterModal.IsVisible() || m_dockPressed >= 0;
    m_dockRemaining = retained ? 0.9 : std::max(0.0, m_dockRemaining - dt);
    const float target = m_dockRemaining > 0 ? 1.0f : 0.0f;
    const float previous = m_dockOpacity;
    if (!m_settings.effects) m_dockOpacity = target;
    else if (target > m_dockOpacity) m_dockOpacity = std::min(target, m_dockOpacity + static_cast<float>(dt / 0.18));
    else m_dockOpacity = std::max(target, m_dockOpacity - static_cast<float>(dt / 0.18));
    if (previous != m_dockOpacity) m_window.RequestRedraw();
}

void App::InvokeDock(int index) {
    if (index<0 || index>=6 || GetDockAction(index).disabled) return;
    switch (GetDockAction(index).command) {
        case DockCommand::Viewer: SetPage(Page::Viewer); break;
        case DockCommand::Waterfall: SetPage(Page::Waterfall); break;
        case DockCommand::Board: SetPage(Page::Board); break;
        case DockCommand::Rotate: OnKeyDown('R',{}); break;
        case DockCommand::Tags: OnKeyDown('T',{}); break;
        case DockCommand::SortFilter: m_sortFilterModal.Show(); break;
        case DockCommand::Settings: OnKeyDown('O',{}); break;
    }
    m_window.RequestRedraw();
}

bool App::HandleDockKey(uint32_t vk, KeyModifiers mods) {
    if (!HasDock() || mods.ctrl || mods.alt) return false;
    if (vk == VK_TAB) {
        m_dockFocus = m_dockFocus < 0 ? (mods.shift ? 5 : 0) : (m_dockFocus + (mods.shift ? 5 : 1)) % 6;
        m_dockRemaining = 0.9; m_dockOpacity = 1; m_window.RequestRedraw();
        return true;
    }
    if (m_dockFocus < 0) return false;
    if (vk == VK_ESCAPE) { m_dockFocus = -1; m_window.RequestRedraw(); return true; }
    if (vk == VK_LEFT || vk == VK_RIGHT) {
        m_dockFocus = (m_dockFocus + (vk == VK_LEFT ? 5 : 1)) % 6;
        m_window.RequestRedraw(); return true;
    }
    if (vk == VK_RETURN || vk == VK_SPACE) { InvokeDock(m_dockFocus); return true; }
    return false;
}

void App::DrawDock() {
    if (!HasDock() || m_dockOpacity <= 0 || m_optionsMenu.IsVisible() || m_taggingModal.IsVisible() || m_sortFilterModal.IsVisible()) return;
    const float scale = m_window.GetDpi() / 96.0f;
    const auto bounds = GetDockBounds();
    const auto alpha = m_dockOpacity;
    m_renderer.FillRoundedRect(bounds, 8 * scale, 8 * scale, Color::FromRGBA(26, 33, 36, static_cast<uint8_t>(235 * alpha)));
    m_renderer.DrawRoundedRect(bounds, 8 * scale, 8 * scale, Color::White(0.12f * alpha), scale);
    for (int index = 0; index < 6; ++index) {
        const auto cell = GetDockButtonRect(index);
        const auto action=GetDockAction(index);
        const bool disabled=action.disabled;
        const bool hovered = m_dockHovered == index || m_dockFocus == index;
        const bool active = (index == 4 && m_sortFilterModal.IsVisible()) || (index == 3 && m_taggingModal.IsVisible()) || (index == 5 && m_optionsMenu.IsVisible());
        const auto color = disabled ? Color::White(0.36f * alpha) : (active || hovered) ? Color::FromHex(0x53deee, alpha) : Color::FromHex(0xe9edef, alpha);
        if (!disabled && (hovered || active)) {
            m_renderer.FillRoundedRect(cell, 7 * scale, 7 * scale, Color::FromHex(0x53deee, (active ? 0.13f : 0.10f) * alpha));
        }
        if (m_dockFocus == index) m_renderer.DrawRoundedRect(cell, 7 * scale, 7 * scale, color, scale);
        const auto center = cell.Center();
        m_dockIcons.Draw(m_renderer, action.glyph, RectF::FromXYWH(center.x - 14 * scale, center.y - 14 * scale, 28 * scale, 28 * scale), color);
        if (index < 5) {
            const auto next = GetDockButtonRect(index + 1);
            if (next.top == cell.top) {
                const float x = (cell.right + next.left) / 2;
                m_renderer.DrawLine({x, cell.top + 4 * scale}, {x, cell.bottom - 4 * scale}, Color::White(0.10f * alpha), scale);
            }
        }
    }
    const int named = m_dockFocus >= 0 ? m_dockFocus : m_dockHovered;
    if (named >= 0) {
        const auto tooltip = GetDockTooltipRect();
        m_renderer.FillRoundedRect(tooltip,4*scale,4*scale,Color::Black(0.6f*alpha));
        m_renderer.DrawRoundedRect(tooltip,4*scale,4*scale,Color::White(0.15f*alpha),scale);
        m_renderer.DrawText(_(GetDockAction(named).name),tooltip,Color::White(alpha),12*scale,TextAlign::Center,TextVAlign::Center);
    }
}

} // namespace kv
