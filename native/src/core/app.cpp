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
#include <fstream>

namespace kv {

App::App()
    : m_folders(m_libraryData.m_folders),
      m_currentFolderIdx(m_libraryData.m_currentFolderIdx)
{
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
        std::ofstream dbg("debug_ipc.txt", std::ios::app);
        dbg << "[APP] Initialize step: " << msg << std::endl;
        dbg.close();
    };
    trace("Start");

    // Load Settings
    m_settings.Load();
    trace("Settings loaded");
    I18nEngine::Initialize(m_settings.ui_lang);
    trace("I18n initialized");

    // Initialize Library Data
    m_libraryData.Initialize(this);
    trace("FolderDataManager initialized");
    m_libraryData.LoadFavorites();
    trace("Favorites loaded");
    m_libraryData.LoadSession();
    trace("Session loaded");
    m_commentsManager.Load();
    trace("Comments loaded");
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

    // ── Create Window ─────────────────────────────────────────────────────
    WindowConfig wndConfig;
    wndConfig.title       = L"Krumassan Image Viewer";
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

    // Enforce Single Instance
    if (m_settings.single_instance && !config.multiple) {
        std::wstring initialPath = config.initialFilePath.empty() ? config.initialFolderPath : config.initialFilePath;
        if (!m_ipc.Initialize(m_window.GetHwnd(), initialPath)) {
            // Exit process immediately since it has been handled by existing instance
            return InitResult::AlreadyRunning;
        }
    }

    // Register system tray icon
    ShellIntegrator::RegisterTrayIcon(
        m_window.GetHwnd(),
        LoadIconW(nullptr, IDI_APPLICATION),
        L"Krumassan Image Viewer"
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
        return true;
    };
    m_window.events.onIpcOpenPath = [this](const std::wstring& path) {
        LoadImage(path);
        if (m_window.IsFullscreen()) {
            m_window.SetFullscreen(true);
        } else {
            m_window.Show();
        }
        SetForegroundWindow(m_window.GetHwnd());
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
    if (!config.initialFilePath.empty()) {
        LoadImage(config.initialFilePath);
    } else if (!config.initialFolderPath.empty()) {
        LoadFolder(config.initialFolderPath);
    } else if (m_folders.empty()) {
        // Try loading current directory if nothing else is specified or restored by session
        LoadFolder(std::filesystem::current_path().wstring());
    }

    m_frameTimer.Reset();
    LogInfo(L"Application initialized successfully");
    return InitResult::Success;
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
                // Shutdown IPC server first (poison pill, guaranteed no deadlock).
                m_ipc.Shutdown();
                // Shutdown image loader thread pool.
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
                                   || (m_currentPage == Page::Waterfall && (std::abs(m_waterfallPage.m_targetScrollOffset - m_waterfallPage.m_scrollOffset) > 0.1f || m_waterfallPage.m_waterfallBlockActiveItem || (m_waterfallPage.m_hoveredOriginalAnim && !m_waterfallPage.m_hoveredAnimSequencer.IsPaused())));

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
    float scaleX = screenSize.width / imgW;
    float scaleY = screenSize.height / imgH;
    float scale = std::min(scaleX, scaleY);

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

    // Check if animated
    if (ImageLoader::IsAnimatedFormat(path)) {
        auto anim = m_imageLoader.LoadAnimatedSync(path);
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
            m_window.SetTitle(std::format(L"Krumassan Viewer — {} ({} frames)",
                fs::path(path).filename().wstring(), anim->FrameCount()));
            
            m_libraryData.AddToHistory(path);
            m_window.RequestRedraw();
            return;
        }
    }

    // Load as static image
    auto image = m_imageLoader.LoadSync(path);
    if (image && image->IsValid()) {
        ImageLoader::UploadToGpu(*image, m_renderer);
        m_currentImage = image;
        m_animatedImage = nullptr;
        m_frameSequencer.Reset();
        ResetView();
        m_window.SetTitle(std::format(L"Krumassan Viewer — {} ({}×{})",
            fs::path(path).filename().wstring(), image->width, image->height));

        m_libraryData.AddToHistory(path);
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
    m_window.RequestRedraw();
}

void App::LoadFolder(const std::wstring& folderPath, bool loadImage) {
    m_libraryData.AddFolder(folderPath, false, loadImage);

    m_thumbnailStrip.SetFileList(m_fileList);
    m_thumbnailStrip.SyncSelection(m_currentIndex);
    m_libraryPage.SyncCurrentFolderWithApp();
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

void App::SetPage(Page page) {
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
        if (m_viewerPage.OnKeyDown(vk, mods)) {
            return;
        }
    } else if (m_currentPage == Page::Library) {
        m_libraryPage.OnKeyDown(vk, mods);
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
        case '2':
            SetPage(Page::Viewer);
            break;
        case '3':
            SetPage(Page::Board);
            break;
        case '4':
            SetPage(Page::Library);
            break;
        case '5':
            SetPage(Page::Waterfall);
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
                            selectedPath = item.filePath;
                            break;
                        }
                    }
                }
                if (!selectedPath.empty() && ImageLoader::IsSupported(selectedPath)) {
                    m_taggingModal.Show(selectedPath);
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

} // namespace kv
