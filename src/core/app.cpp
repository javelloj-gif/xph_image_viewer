// ============================================================================
// Class: App
// Description: App is the central coordinator for the [IMAGE_VIEWER_APP_NAME] application, managing the main event loop, routing system messages, and page navigation.
// Responsibility: Orchestrate the Win32 window message loop and delegate actions to page components.
// ============================================================================
#include "common/pch.h"
#include "app.h"
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
    m_window.events.onIpcOpenPath = {};
    m_ipc.Shutdown();
    // Join producers while their page, metadata cache, WIC factory and window still exist.
    m_boardPage.m_tagScanner->CancelScan();
    m_tagManager.CancelSearch();
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
    wndConfig.title       = L"LenteUV";
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

    m_tagManager.Initialize(m_imageLoader.GetWicFactory());
    trace("Tags loaded");
    m_optionsMenu.Initialize(this);
    trace("OptionsMenu initialized");
    m_taggingModal.Initialize(this);
    trace("TaggingModal initialized");

    // Register system tray icon
    ShellIntegrator::RegisterTrayIcon(
        m_window.GetHwnd(),
        LoadIconW(nullptr, IDI_APPLICATION),
        L"LenteUV"
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
    if (!m_renderer.Initialize(m_window.GetHwnd(), config.translucent)) {
        LogError(L"Failed to initialize renderer");
        return InitResult::Failed;
    }

    // ── Show Window ───────────────────────────────────────────────────────
    if (!m_settings.hide_on_app_start) {
        int cmdShow = SW_SHOW;
        if (config.startMaximized) cmdShow = SW_SHOWMAXIMIZED;
        m_window.Show(cmdShow);
    }

    if (config.startFullscreen || m_settings.show_fullscreen) {
        m_window.SetFullscreen(true);
    }

    m_viewerPage.Initialize(this);
    m_thumbnailStrip.Initialize(this);
    m_libraryPage.Initialize(this);
    m_boardPage.Initialize(this);
    m_waterfallPage.Initialize(this);

    SetPage(static_cast<Page>(m_settings.page_on_app_launch_from_explorer));

    // ── Load Initial Content ──────────────────────────────────────────────
    bool loadSessionImage = config.initialFilePath.empty() && config.initialFolderPath.empty();
    m_folderData.LoadSession(loadSessionImage);

    if (!config.initialFilePath.empty()) {
        LoadImage(config.initialFilePath);
    } else if (!config.initialFolderPath.empty()) {
        LoadFolder(config.initialFolderPath);
    } else if (m_folders.empty()) {
        // Try loading current directory if nothing else is specified or restored by session
        LoadFolder(std::filesystem::current_path().wstring());
    }

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
        SetPage(Page::Library);
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
// Uses a game-loop style approach: process messages, then render continuously.
// This ensures smooth animations and responsive zoom/pan.

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

        // Render a frame
        RenderFrame();

        // If no animation is playing and no image is being loaded,
        // yield CPU time to other processes
        bool needsContinuousRender = m_animations.HasActiveAnimations()
                                   || m_thumbnailStrip.HasActiveAnimations()
                                   || (m_currentPage == Page::Viewer && m_viewerPage.HasActiveAnimations())
                                   || (m_animatedImage && !m_frameSequencer.IsPaused())
                                   || (m_currentPage == Page::Library && m_libraryPage.m_hoveredOriginalAnim && !m_libraryPage.m_hoveredAnimSequencer.IsPaused())
                                   || (m_currentPage == Page::Board && m_boardPage.m_editor.m_editingNoteIdx != -1)
                                   || (m_currentPage == Page::Waterfall && (std::abs(m_waterfallPage.m_targetScrollOffset - m_waterfallPage.m_scrollOffset) > 0.1f || m_waterfallPage.m_waterfallBlockActiveItem || (m_waterfallPage.m_hoveredOriginalAnim && !m_waterfallPage.m_hoveredAnimSequencer.IsPaused())))
                                   || (m_radialMenuProgress > 0.0f && m_radialMenuProgress < 1.0f);

        if (!needsContinuousRender) {
            // Wait for next message or 16ms timeout (~60fps max idle rate)
            MsgWaitForMultipleObjects(0, nullptr, FALSE, 16, QS_ALLINPUT);
        }
    }
}

// ─── Render Frame ────────────────────────────────────────────────────────────

void App::RenderFrame() {
    // Guard against rendering before the window has a valid size
    auto targetSize = m_renderer.GetTargetSize();
    if (targetSize.width == 0 || targetSize.height == 0) return;
    if (!m_renderer.IsInitialized()) return;

    double dt = m_frameTimer.Tick();
    // Clamp dt to avoid huge jumps after loading stalls
    dt = std::min(dt, 0.1);

    // Update radial menu animation progress
    if (m_radialMenuVisible) {
        if (m_radialMenuProgress < 1.0f) {
            m_radialMenuProgress = std::min(m_radialMenuProgress + static_cast<float>(dt) * 5.0f, 1.0f);
            m_window.RequestRedraw();
        }
    } else {
        if (m_radialMenuProgress > 0.0f) {
            m_radialMenuProgress = std::max(m_radialMenuProgress - static_cast<float>(dt) * 5.0f, 0.0f);
            m_window.RequestRedraw();
        }
    }

    // Update animations
    m_animations.Update(dt);

    // Update page animations
    if (m_currentPage == Page::Viewer) {
        m_viewerPage.Update(dt);
        m_thumbnailStrip.Update(dt);
    } else if (m_currentPage == Page::Library) {
        m_libraryPage.Update(dt);
    } else if (m_currentPage == Page::Board) {
        m_boardPage.Update(dt);
    } else if (m_currentPage == Page::Waterfall) {
        m_waterfallPage.Update(dt);
    }

    // Update animated image playback
    if (m_animatedImage && !m_frameSequencer.IsPaused()) {
        if (m_frameSequencer.Update(dt)) {
            m_window.RequestRedraw();
        }
    }

    // Process any pending GPU texture uploads from background threads
    m_imageLoader.ProcessPendingTextures(m_renderer);

    // FPS counter
    m_fpsAccumulator += dt;
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
    DrawViewerContent();
    DrawCloseButton();
    if (m_optionsMenu.IsVisible()) {
        m_optionsMenu.Draw(m_renderer);
    }
    if (m_taggingModal.IsVisible()) {
        m_taggingModal.Draw(m_renderer);
    }
    if (m_radialMenuProgress > 0.0f) {
        DrawRadialMenu();
    }

    m_renderer.EndFrame();
}

// ─── Draw Viewer Content ─────────────────────────────────────────────────────

void App::DrawViewerContent() {
    if (m_currentPage == Page::Viewer) {
        m_viewerPage.Draw(m_renderer);
        m_thumbnailStrip.Draw(m_renderer);
    } else if (m_currentPage == Page::Library) {
        m_libraryPage.Draw(m_renderer);
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

    Color btnColor = m_settings.desaturated_corner_buttons_and_corner_menus
                     ? Color::FromRGBA(80, 80, 80, 180)
                     : Color::FromRGBA(200, 50, 50, 180);
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

    LogInfo(std::format(L"Loading image: {}", path));

    const auto loaded = m_imageLoader.LoadForViewerSync(path);
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
            m_window.SetTitle(std::format(L"LenteUV — {} ({} frames)",
                fs::path(path).filename().wstring(), anim->FrameCount()));
            
            m_folderData.AddToHistory(path);
            m_window.RequestRedraw();
            return;
        }
    }

    // Load as static image
    auto image = loaded.image;
    if (image && image->IsValid()) {
        ImageLoader::UploadToGpu(*image, m_renderer);
        m_currentImage = image;
        m_animatedImage = nullptr;
        m_frameSequencer.Reset();
        ResetView();
        m_window.SetTitle(std::format(L"LenteUV — {} ({}×{})",
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
        m_fileList.clear();
        LoadFolder(fs::path(path).parent_path().wstring(), false);
        it = std::find(m_fileList.begin(), m_fileList.end(), path);
        if (it != m_fileList.end()) {
            m_currentIndex = static_cast<int>(std::distance(m_fileList.begin(), it));
        }
    }

    if (m_currentFolderIdx >= 0 && m_currentFolderIdx < static_cast<int>(m_folders.size())) {
        m_folders[m_currentFolderIdx].currentIndex = m_currentIndex;
    }

    m_thumbnailStrip.SyncSelection(m_currentIndex);
    m_libraryPage.SyncCurrentFolderWithApp();
    if (!m_window.IsFullscreen()) {
        AdjustWindowToImage();
    }
    m_window.RequestRedraw();
}

void App::LoadFolder(const std::wstring& folderPath, bool loadImage) {
    m_folderData.AddFolder(folderPath, false, loadImage);

    m_thumbnailStrip.SetFileList(m_fileList);
    m_thumbnailStrip.SyncSelection(m_currentIndex);
    m_libraryPage.SyncCurrentFolderWithApp();
}

void App::ApplySortMode(int mode) {
    m_settings.sort_mode = mode;
    m_settings.Save();

    for (size_t i = 0; i < m_folders.size(); ++i) {
        m_folderData.SortFolder(static_cast<int>(i), static_cast<FolderDataManager::SortMode>(mode));
    }

    if (m_currentFolderIdx >= 0 && m_currentFolderIdx < static_cast<int>(m_folders.size())) {
        auto& folder = m_folders[m_currentFolderIdx];
        m_fileList = folder.filePaths;

        std::wstring currentPath = L"";
        if (m_currentImage) {
            currentPath = m_currentImage->filePath;
        } else if (m_animatedImage) {
            currentPath = m_animatedImage->filePath;
        }

        if (!currentPath.empty()) {
            auto it = std::find(m_fileList.begin(), m_fileList.end(), currentPath);
            if (it != m_fileList.end()) {
                m_currentIndex = static_cast<int>(std::distance(m_fileList.begin(), it));
                folder.currentIndex = m_currentIndex;
            }
        }
    }

    m_thumbnailStrip.SetFileList(m_fileList);
    m_thumbnailStrip.SyncSelection(m_currentIndex);

    m_libraryPage.CalculateLayout();
    m_waterfallPage.CalculateLayout();

    m_window.RequestRedraw();
}

void App::DeleteImage(const std::wstring& path) {
    if (m_libraryPage.m_hoveredOriginalPath == path || m_waterfallPage.m_hoveredOriginalPath == path) CancelHoverLoads();
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
            LoadImage(m_fileList[m_currentIndex]);
        } else {
            m_currentIndex = -1;
            ResetView();
            m_window.SetTitle(L"LenteUV");
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

    m_libraryPage.CalculateLayout();
    m_waterfallPage.CalculateLayout();

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
    m_libraryPage.m_hoverLoadLifetime.reset();
    m_libraryPage.m_hoveredOriginalPath.clear();
    m_libraryPage.m_hoveredPreviewIdx = -1;
    m_libraryPage.m_hoveredOriginalImage.reset();
    m_libraryPage.m_hoveredOriginalAnim.reset();
    m_libraryPage.m_hoveredOriginalLoading = false;
    m_waterfallPage.m_hoverLoadLifetime.reset();
    m_waterfallPage.m_hoveredOriginalPath.clear();
    m_waterfallPage.m_hoveredPreviewIdx = -1;
    m_waterfallPage.m_hoveredOriginalAnim.reset();
    m_waterfallPage.m_hoveredOriginalLoading = false;
}

void App::SetPage(Page page) {
    if (page != m_currentPage) {
        CancelHoverLoads();
        if (page != Page::Library && page != Page::Waterfall) m_folderData.CancelPreviewLoads();
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
    } else if (page == Page::Library) {
        m_libraryPage.SyncCurrentFolderWithApp();
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
    m_mouseX = x;
    m_mouseY = y;
    if (m_taggingModal.IsVisible()) {
        if (m_taggingModal.OnMouseMove(x, y, mods)) return;
    }
    if (m_optionsMenu.IsVisible()) {
        if (m_optionsMenu.OnMouseMove(x, y, mods)) return;
    }

    if (m_window.IsFullscreen()) {
        auto size = m_renderer.GetTargetSize().ToFloat();
        if (x < 100.0f && y < 100.0f) {
            if (!m_radialMenuVisible) {
                m_radialMenuVisible = true;
                m_window.RequestRedraw();
            }
        } else if (m_radialMenuVisible) {
            bool inMenuArea = (x < 200.0f && y < 200.0f);
            bool inTravelBand = (y < 80.0f);
            if (!inMenuArea && !inTravelBand) {
                m_radialMenuVisible = false;
                m_window.RequestRedraw();
            }
        }
    } else {
        if (m_radialMenuVisible) {
            m_radialMenuVisible = false;
            m_window.RequestRedraw();
        }
    }

    if (m_radialMenuProgress > 0.1f) {
        int oldHovered = m_radialMenuHoveredIdx;
        m_radialMenuHoveredIdx = -1;
        for (int i = 0; i < 4; ++i) {
            PointF btnCenter = GetRadialButtonCenter(i, m_radialMenuProgress);
            float dx = x - btnCenter.x;
            float dy = y - btnCenter.y;
            float distSq = dx * dx + dy * dy;
            if (distSq <= 26.0f * 26.0f) {
                m_radialMenuHoveredIdx = i;
                break;
            }
        }
        if (m_radialMenuHoveredIdx != oldHovered) {
            m_window.RequestRedraw();
        }
        if (x < 200.0f && y < 200.0f) {
            return; // Intercept mouse move so background doesn't hover
        }
    }

    if (m_currentPage == Page::Library) {
        if (m_libraryPage.OnMouseMove(x, y, mods)) return;
    } else if (m_currentPage == Page::Board) {
        if (m_boardPage.OnMouseMove(x, y, mods)) return;
    } else if (m_currentPage == Page::Waterfall) {
        if (m_waterfallPage.OnMouseMove(x, y, mods)) return;
    } else {
        if (m_thumbnailStrip.HandleMouseMove(x, y, mods)) return;
        m_viewerPage.OnMouseMove(x, y, mods);
    }
}

void App::OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods) {
    m_mouseX = x;
    m_mouseY = y;

    if (m_taggingModal.IsVisible()) {
        if (m_taggingModal.OnMouseDown(btn, x, y, mods)) return;
    }
    if (m_optionsMenu.IsVisible()) {
        if (m_optionsMenu.OnMouseDown(btn, x, y, mods)) return;
    }

    if (m_radialMenuProgress > 0.5f) {
        if (btn == MouseButton::Left && m_radialMenuHoveredIdx != -1) {
            Page targetPage;
            if (m_radialMenuHoveredIdx == 0) targetPage = Page::Viewer;
            else if (m_radialMenuHoveredIdx == 1) targetPage = Page::Library;
            else if (m_radialMenuHoveredIdx == 2) targetPage = Page::Waterfall;
            else targetPage = Page::Board;

            SetPage(targetPage);
            m_radialMenuVisible = false;
            m_radialMenuProgress = 0.0f;
            m_radialMenuHoveredIdx = -1;
            m_window.RequestRedraw();
            return;
        }
        if (x < 200.0f && y < 200.0f) {
            return; // Eat the click
        }
    }

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

    if (m_currentPage == Page::Library) {
        if (m_libraryPage.OnMouseDown(btn, x, y, mods)) return;
    } else if (m_currentPage == Page::Board) {
        if (m_boardPage.OnMouseDown(btn, x, y, mods)) return;
    } else if (m_currentPage == Page::Waterfall) {
        if (m_waterfallPage.OnMouseDown(btn, x, y, mods)) return;
    } else {
        if (m_thumbnailStrip.HandleMouseDown(btn, x, y, mods)) return;
        m_viewerPage.OnMouseDown(btn, x, y, mods);
    }
}

void App::OnMouseUp(MouseButton btn, float x, float y, KeyModifiers mods) {
    m_mouseX = x;
    m_mouseY = y;
    if (m_taggingModal.IsVisible()) {
        if (m_taggingModal.OnMouseUp(btn, x, y, mods)) return;
    }
    if (m_optionsMenu.IsVisible()) {
        if (m_optionsMenu.OnMouseUp(btn, x, y, mods)) return;
    }

    if (m_radialMenuProgress > 0.5f) {
        if (x < 200.0f && y < 200.0f) {
            return; // Eat mouse up
        }
    }

    if (m_currentPage == Page::Library) {
        if (m_libraryPage.OnMouseUp(btn, x, y, mods)) return;
    } else if (m_currentPage == Page::Board) {
        if (m_boardPage.OnMouseUp(btn, x, y, mods)) return;
    } else if (m_currentPage == Page::Waterfall) {
        if (m_waterfallPage.OnMouseUp(btn, x, y, mods)) return;
    } else {
        if (m_thumbnailStrip.HandleMouseUp(btn, x, y, mods)) return;
        m_viewerPage.OnMouseUp(btn, x, y, mods);
    }
}

void App::OnMouseWheel(float delta, float x, float y, KeyModifiers mods) {
    m_mouseX = x;
    m_mouseY = y;
    if (m_taggingModal.IsVisible()) {
        if (m_taggingModal.OnMouseWheel(delta, x, y, mods)) return;
    }
    if (m_currentPage == Page::Library) {
        if (m_libraryPage.OnMouseWheel(delta, x, y, mods)) return;
    } else if (m_currentPage == Page::Board) {
        if (m_boardPage.OnMouseWheel(delta, x, y, mods)) return;
    } else if (m_currentPage == Page::Waterfall) {
        if (m_waterfallPage.OnMouseWheel(delta, x, y, mods)) return;
    } else {
        if (m_thumbnailStrip.HandleMouseWheel(delta, x, y, mods)) return;
        m_viewerPage.OnMouseWheel(delta, x, y, mods);
    }
}

void App::OnMouseDoubleClick(MouseButton btn, float x, float y, KeyModifiers mods) {
    m_mouseX = x;
    m_mouseY = y;
    if (m_currentPage == Page::Library) {
        if (m_libraryPage.OnMouseDoubleClick(btn, x, y, mods)) return;
    } else if (m_currentPage == Page::Board) {
        if (m_boardPage.OnMouseDoubleClick(btn, x, y, mods)) return;
    } else if (m_currentPage == Page::Waterfall) {
        if (m_waterfallPage.OnMouseDoubleClick(btn, x, y, mods)) return;
    } else {
        if (m_thumbnailStrip.HandleMouseDoubleClick(btn, x, y, mods)) return;
        m_viewerPage.OnMouseDoubleClick(btn, x, y, mods);
    }
}

void App::OnKeyDown(uint32_t vk, KeyModifiers mods) {
    if (m_taggingModal.IsVisible()) {
        if (m_taggingModal.OnKeyDown(vk, mods)) return;
    }
    if (m_optionsMenu.IsVisible()) {
        if (m_optionsMenu.OnKeyDown(vk, mods)) return;
    }
    if (m_currentPage == Page::Viewer) {
        if (m_thumbnailStrip.IsMouseOver(m_mouseX, m_mouseY) && m_thumbnailStrip.HandleKeyDown(vk, mods)) {
            return;
        }
        if (m_viewerPage.OnKeyDown(vk, mods)) {
            return;
        }
    } else if (m_currentPage == Page::Library) {
        if (m_libraryPage.OnKeyDown(vk, mods)) {
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
            } else if (m_currentPage == Page::Library) {
                if (m_libraryPage.m_hoveredPreviewIdx != -1 && m_currentFolderIdx >= 0) {
                    auto& folder = m_folders[m_currentFolderIdx];
                    if (m_libraryPage.m_hoveredPreviewIdx < static_cast<int>(folder.filePaths.size())) {
                        targetPath = folder.filePaths[m_libraryPage.m_hoveredPreviewIdx];
                    }
                }
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
                    L"¿Estás seguro de que deseas enviar esta imagen a la Papelera de reciclaje?",
                    L"Eliminar Imagen",
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
            SetPage(Page::Library);
            break;
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
                } else if (m_currentPage == Page::Library) {
                    if (m_libraryPage.m_hoveredPreviewIdx != -1) {
                        selectedPath = m_libraryPage.m_hoveredOriginalPath;
                    }
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
    if (m_taggingModal.IsVisible()) {
        m_taggingModal.OnChar(ch);
        return;
    }
    if (m_currentPage == Page::Board) {
        m_boardPage.OnChar(ch);
    }
}

void App::OnFileDrop(std::vector<std::wstring> files) {
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
    if (m_currentPage == Page::Viewer) {
        m_thumbnailStrip.HandleMouseLeave();
    }
}

PointF App::GetRadialButtonCenter(int idx, float progress) {
    float startAngle = 0.0f; // 0 degrees
    float stepAngle = 3.14159265f / 6.0f; // 30 degrees (pi / 6)
    float angle = startAngle + idx * stepAngle;
    
    // Distance from the hub center (50, 50)
    float distance = 95.0f * progress;
    
    float x = 50.0f + distance * cos(angle);
    float y = 50.0f + distance * sin(angle);
    return PointF{ x, y };
}

void App::DrawRadialMenu() {
    if (m_radialMenuProgress <= 0.0f) return;

    PointF hubCenter(50.0f, 50.0f);
    float progress = m_radialMenuProgress;

    // Draw connection lines to each active button center
    for (int i = 0; i < 4; ++i) {
        PointF btnCenter = GetRadialButtonCenter(i, progress);
        Color lineCol = (m_radialMenuHoveredIdx == i) 
            ? Color::FromHex(0x00ffff, 0.4f * progress) 
            : Color::FromHex(0x00ff88, 0.2f * progress);
        m_renderer.DrawLine(hubCenter, btnCenter, lineCol, 1.5f);
    }

    // Draw Hub
    // Glowing ring
    m_renderer.DrawEllipse(hubCenter, 40.0f, 40.0f, Color::FromHex(0x00ffff, 0.3f * progress), 2.0f);
    // Inner fill
    m_renderer.FillEllipse(hubCenter, 36.0f, 36.0f, Color::FromRGBA(15, 20, 25, static_cast<uint8_t>(220.0f * progress)));
    // Border ring
    m_renderer.DrawEllipse(hubCenter, 36.0f, 36.0f, Color::FromHex(0x00ff88, 0.6f * progress), 1.0f);
    // Hub Label
    m_renderer.DrawText(L"KV", RectF::FromXYWH(10.0f, 10.0f, 80.0f, 80.0f), Color::FromHex(0x00ffff, 0.8f * progress), 14.0f, TextAlign::Center, TextVAlign::Center);

    // Draw Buttons
    const wchar_t* labels[] = { L"VIS", L"LIB", L"WAT", L"BRD" };
    Page pages[] = { Page::Viewer, Page::Library, Page::Waterfall, Page::Board };

    for (int i = 0; i < 4; ++i) {
        PointF btnCenter = GetRadialButtonCenter(i, progress);
        bool hovered = (m_radialMenuHoveredIdx == i);
        bool isActive = (m_currentPage == pages[i]);

        Color ringColor;
        Color fillColor;
        Color textColor;

        if (hovered) {
            ringColor = Color::FromHex(0x00ffff, 0.9f * progress);
            fillColor = Color::FromHex(0x00ffff, 0.25f * progress);
            textColor = Color::White(0.95f * progress);
        } else if (isActive) {
            ringColor = Color::FromHex(0x00ff88, 0.7f * progress);
            fillColor = Color::FromHex(0x00ff88, 0.15f * progress);
            textColor = Color::FromHex(0x00ff88, 0.9f * progress);
        } else {
            ringColor = Color::White(0.3f * progress);
            fillColor = Color::FromRGBA(10, 10, 10, static_cast<uint8_t>(180.0f * progress));
            textColor = Color::White(0.6f * progress);
        }

        // Fill background
        m_renderer.FillEllipse(btnCenter, 26.0f, 26.0f, fillColor);
        // Draw ring
        m_renderer.DrawEllipse(btnCenter, 26.0f, 26.0f, ringColor, hovered ? 2.0f : 1.0f);
        // Draw Text
        RectF textRect = RectF::FromXYWH(btnCenter.x - 26.0f, btnCenter.y - 26.0f, 52.0f, 52.0f);
        m_renderer.DrawText(labels[i], textRect, textColor, 11.0f, TextAlign::Center, TextVAlign::Center);
    }

    // Draw Tooltip / Label
    if (m_radialMenuHoveredIdx != -1) {
        const wchar_t* modeNames[] = {
            L"Viewer Mode (num1)",
            L"Library Mode (num2)",
            L"Waterfall Mode (num3)",
            L"Board Mode (num4)"
        };
        RectF tooltipRect = RectF::FromXYWH(15.0f, 155.0f, 220.0f, 25.0f);
        m_renderer.DrawText(modeNames[m_radialMenuHoveredIdx], tooltipRect, Color::FromHex(0x00ffff, 0.9f * progress), 12.0f, TextAlign::Left, TextVAlign::Center);
    }
}

} // namespace kv
