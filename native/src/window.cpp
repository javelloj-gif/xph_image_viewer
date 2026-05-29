// ============================================================================
// window.cpp — Win32 Frameless Window Implementation
// Krumassan Image Viewer — Native C++20
// ============================================================================

#include "pch.h"
#include "window.h"

namespace kv {

// ─── Constants ───────────────────────────────────────────────────────────────

static constexpr const wchar_t* kWindowClassName = L"KrumassanViewerWindow";
static bool sWindowClassRegistered = false;

// ─── Destructor ──────────────────────────────────────────────────────────────

Window::~Window() {
    if (m_hwnd) {
        // Remove the stored pointer before destroying to prevent dangling access
        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, 0);
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

// ─── Create ──────────────────────────────────────────────────────────────────

bool Window::Create(const WindowConfig& config) {
    m_frameless   = config.frameless;
    m_translucent = config.translucent;

    // Register window class (once per process)
    if (!sWindowClassRegistered) {
        WNDCLASSEXW wc = {};
        wc.cbSize        = sizeof(wc);
        wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc   = StaticWndProc;
        wc.hInstance     = GetModuleHandleW(nullptr);
        wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;  // We handle all painting
        wc.lpszClassName = kWindowClassName;
        wc.hIcon         = config.icon;
        wc.hIconSm       = config.icon;

        if (!RegisterClassExW(&wc)) {
            LogError(L"RegisterClassExW failed");
            return false;
        }
        sWindowClassRegistered = true;
    }

    // ── Window Style ──────────────────────────────────────────────────────
    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exStyle = WS_EX_APPWINDOW;

    if (m_frameless) {
        // WS_POPUP for frameless, but keep THICKFRAME for system snap/resize
        // and MAXIMIZEBOX for double-click-titlebar maximize
        style = WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
    }

    if (m_translucent) {
        // WS_EX_NOREDIRECTIONBITMAP tells Windows not to allocate a
        // redirection surface for this window. Combined with DirectComposition,
        // this enables true per-pixel alpha transparency.
        // This is the correct approach — the Python QOpenGLWidget attempt
        // lost transparency because Qt's compositor couldn't handle it.
        exStyle |= WS_EX_NOREDIRECTIONBITMAP;
    }

    // ── DPI-aware initial size ────────────────────────────────────────────
    // Get the system DPI for initial sizing
    m_dpi = GetDpiForSystem();

    int scaledWidth  = DpiScaleInt(config.width, m_dpi);
    int scaledHeight = DpiScaleInt(config.height, m_dpi);

    // Center on primary monitor
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenW - scaledWidth) / 2;
    int y = (screenH - scaledHeight) / 2;

    // ── Create Window ─────────────────────────────────────────────────────
    // Pass 'this' through lpParam so StaticWndProc can store it during WM_CREATE
    m_hwnd = CreateWindowExW(
        exStyle,
        kWindowClassName,
        config.title.c_str(),
        style,
        x, y, scaledWidth, scaledHeight,
        nullptr,        // parent
        nullptr,        // menu
        GetModuleHandleW(nullptr),
        this            // lpParam → available in WM_CREATE
    );

    if (!m_hwnd) {
        LogError(std::format(L"CreateWindowExW failed: {}", GetLastError()));
        return false;
    }

    // ── DWM: Extend frame for translucent background ──────────────────────
    // Commented out: When using DirectComposition with WS_EX_NOREDIRECTIONBITMAP,
    // extending the frame with DwmExtendFrameIntoClientArea can cause the DWM
    // to paint an opaque frame background, preventing true alpha transparency.
    /*
    if (m_translucent) {
        MARGINS margins = { -1, -1, -1, -1 };
        DwmExtendFrameIntoClientArea(m_hwnd, &margins);
    }
    */

    // ── File Drag-and-Drop ────────────────────────────────────────────────
    if (config.acceptDrops) {
        DragAcceptFiles(m_hwnd, TRUE);
        // For elevated processes, allow drag from non-elevated Explorer
        ChangeWindowMessageFilterEx(m_hwnd, WM_DROPFILES, MSGFLT_ALLOW, nullptr);
        ChangeWindowMessageFilterEx(m_hwnd, WM_COPYDATA,  MSGFLT_ALLOW, nullptr);
        ChangeWindowMessageFilterEx(m_hwnd, 0x0049 /*WM_COPYGLOBALDATA*/, MSGFLT_ALLOW, nullptr);
    }

    LogInfo(std::format(L"Window created: {}x{} @ {}dpi (frameless={}, translucent={})",
        scaledWidth, scaledHeight, m_dpi, m_frameless, m_translucent));

    return true;
}

// ─── Show / Close ────────────────────────────────────────────────────────────

void Window::Show(int cmdShow) {
    ShowWindow(m_hwnd, cmdShow);
    UpdateWindow(m_hwnd);
}

void Window::Close() {
    if (m_hwnd) {
        PostMessageW(m_hwnd, WM_CLOSE, 0, 0);
    }
}

// ─── Message Loop ────────────────────────────────────────────────────────────

int Window::RunMessageLoop() {
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

// ─── Accessors ───────────────────────────────────────────────────────────────

SizeU Window::GetClientSize() const {
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    return { static_cast<uint32_t>(rc.right), static_cast<uint32_t>(rc.bottom) };
}

bool Window::CheckMinimized() const { return ::IsIconic(m_hwnd) != 0; }
bool Window::CheckMaximized() const { return ::IsZoomed(m_hwnd) != 0; }

void Window::SetTitle(const std::wstring& title) {
    SetWindowTextW(m_hwnd, title.c_str());
}

void Window::RequestRedraw() {
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

// ─── Fullscreen Toggle ───────────────────────────────────────────────────────

void Window::SetFullscreen(bool fullscreen) {
    if (m_fullscreen == fullscreen) return;

    if (fullscreen) {
        // Save current window placement for restoration
        SaveWindowPlacement();

        // Get the monitor where the window currently resides
        HMONITOR hMon = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfoW(hMon, &mi);

        // Remove window decorations and fill the monitor
        SetWindowLongW(m_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(m_hwnd, HWND_TOP,
            mi.rcMonitor.left, mi.rcMonitor.top,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
    } else {
        // Restore previous style and placement
        DWORD style = WS_OVERLAPPEDWINDOW;
        if (m_frameless) {
            style = WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
        }
        SetWindowLongW(m_hwnd, GWL_STYLE, style | WS_VISIBLE);
        RestoreWindowPlacement();
    }

    m_fullscreen = fullscreen;
}

void Window::Minimize() { ShowWindow(m_hwnd, SW_MINIMIZE); }
void Window::Maximize() { ShowWindow(m_hwnd, SW_MAXIMIZE); }
void Window::Restore()  { ShowWindow(m_hwnd, SW_RESTORE); }

// ─── Window Placement Persistence ────────────────────────────────────────────

void Window::SaveWindowPlacement() {
    m_savedPlacement.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(m_hwnd, &m_savedPlacement);
    m_hasValidPlacement = true;
}

void Window::RestoreWindowPlacement() {
    if (m_hasValidPlacement) {
        SetWindowPlacement(m_hwnd, &m_savedPlacement);
        SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
}

// ─── DPI Update ──────────────────────────────────────────────────────────────

void Window::UpdateDpi(uint32_t newDpi) {
    if (m_dpi == newDpi) return;
    uint32_t oldDpi = m_dpi;
    m_dpi = newDpi;

    // Scale the resize border width
    m_resizeBorderWidth = DpiScaleInt(6, m_dpi);

    LogInfo(std::format(L"DPI changed: {} → {}", oldDpi, newDpi));

    if (events.onDpiChanged) {
        events.onDpiChanged(newDpi);
    }
}

// ─── Static WndProc (thunk to instance method) ──────────────────────────────

LRESULT CALLBACK Window::StaticWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    Window* self = nullptr;

    if (msg == WM_CREATE) {
        // Store the Window pointer from CreateWindowEx's lpParam
        auto cs = reinterpret_cast<CREATESTRUCT*>(lp);
        self = static_cast<Window*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;

        // Get actual DPI for this window's monitor
        self->m_dpi = GetDpiForWindow(hwnd);
        self->m_resizeBorderWidth = DpiScaleInt(6, self->m_dpi);
    } else {
        self = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self) {
        return self->HandleMessage(msg, wp, lp);
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ─── Hit Test (frameless resize/move) ────────────────────────────────────────

LRESULT Window::HitTest(int x, int y) const {
    RECT rc;
    GetWindowRect(m_hwnd, &rc);

    int border = m_resizeBorderWidth;

    // Check corners first (they overlap edges)
    bool atLeft   = (x - rc.left)   < border;
    bool atRight  = (rc.right - x)  < border;
    bool atTop    = (y - rc.top)    < border;
    bool atBottom = (rc.bottom - y) < border;

    if (atTop && atLeft)     return HTTOPLEFT;
    if (atTop && atRight)    return HTTOPRIGHT;
    if (atBottom && atLeft)  return HTBOTTOMLEFT;
    if (atBottom && atRight) return HTBOTTOMRIGHT;
    if (atLeft)              return HTLEFT;
    if (atRight)             return HTRIGHT;
    if (atTop)               return HTTOP;
    if (atBottom)            return HTBOTTOM;

    // Title bar area: top 32 DPI-scaled pixels act as drag handle
    // (unless the mouse is over a UI control — the app layer can override)
    int titleBarHeight = DpiScaleInt(32, m_dpi);
    if ((y - rc.top) < titleBarHeight) {
        return HTCAPTION;
    }

    return HTCLIENT;
}

// ─── Instance Message Handler ────────────────────────────────────────────────

LRESULT Window::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {

    // ── Painting ──────────────────────────────────────────────────────────
    case WM_PAINT: {
        // We don't use BeginPaint/EndPaint because Direct2D handles
        // the presentation via the swap chain. Just validate the region.
        ValidateRect(m_hwnd, nullptr);
        if (events.onPaint) {
            events.onPaint();
        }
        return 0;
    }

    // ── Erase Background (skip — Direct2D handles this) ──────────────────
    case WM_ERASEBKGND:
        return 1; // "We handled it" — prevents GDI from clearing the window

    // ── Size Changes ─────────────────────────────────────────────────────
    case WM_SIZE: {
        if (wp == SIZE_MINIMIZED) return 0;
        uint32_t w = LOWORD(lp);
        uint32_t h = HIWORD(lp);
        if (w > 0 && h > 0 && events.onResize) {
            events.onResize(w, h);
        }
        return 0;
    }

    // ── DPI Change (per-monitor DPI awareness v2) ────────────────────────
    case WM_DPICHANGED: {
        uint32_t newDpi = HIWORD(wp);
        UpdateDpi(newDpi);

        // Windows provides the suggested new window rect in lParam
        auto suggestedRect = reinterpret_cast<const RECT*>(lp);
        SetWindowPos(m_hwnd, nullptr,
            suggestedRect->left, suggestedRect->top,
            suggestedRect->right - suggestedRect->left,
            suggestedRect->bottom - suggestedRect->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }

    // ── Hit Testing (frameless window) ───────────────────────────────────
    case WM_NCHITTEST: {
        if (m_frameless && !m_fullscreen) {
            return HitTest(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        }
        break;
    }

    // ── Remove non-client area for frameless window ──────────────────────
    case WM_NCCALCSIZE: {
        if (m_frameless && wp == TRUE) {
            // Return 0 to make the entire window the client area
            // This removes the title bar and borders
            return 0;
        }
        break;
    }

    // ── Mouse Events ─────────────────────────────────────────────────────
    case WM_MOUSEMOVE: {
        float x = static_cast<float>(GET_X_LPARAM(lp));
        float y = static_cast<float>(GET_Y_LPARAM(lp));
        if (events.onMouseMove) {
            events.onMouseMove(x, y, KeyModifiers::Current());
        }
        return 0;
    }

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN: {
        SetCapture(m_hwnd);
        float x = static_cast<float>(GET_X_LPARAM(lp));
        float y = static_cast<float>(GET_Y_LPARAM(lp));
        MouseButton btn = MouseButton::Left;
        if (msg == WM_RBUTTONDOWN) btn = MouseButton::Right;
        if (msg == WM_MBUTTONDOWN) btn = MouseButton::Middle;
        if (events.onMouseDown) {
            events.onMouseDown(btn, x, y, KeyModifiers::Current());
        }
        return 0;
    }

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP: {
        ReleaseCapture();
        float x = static_cast<float>(GET_X_LPARAM(lp));
        float y = static_cast<float>(GET_Y_LPARAM(lp));
        MouseButton btn = MouseButton::Left;
        if (msg == WM_RBUTTONUP) btn = MouseButton::Right;
        if (msg == WM_MBUTTONUP) btn = MouseButton::Middle;
        if (events.onMouseUp) {
            events.onMouseUp(btn, x, y, KeyModifiers::Current());
        }
        return 0;
    }

    case WM_MOUSEWHEEL: {
        float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wp)) / WHEEL_DELTA;
        // WM_MOUSEWHEEL coordinates are screen-relative, convert to client
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        ScreenToClient(m_hwnd, &pt);
        if (events.onMouseWheel) {
            events.onMouseWheel(delta, static_cast<float>(pt.x), static_cast<float>(pt.y),
                                KeyModifiers::Current());
        }
        return 0;
    }

    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK: {
        float x = static_cast<float>(GET_X_LPARAM(lp));
        float y = static_cast<float>(GET_Y_LPARAM(lp));
        MouseButton btn = MouseButton::Left;
        if (msg == WM_RBUTTONDBLCLK) btn = MouseButton::Right;
        if (msg == WM_MBUTTONDBLCLK) btn = MouseButton::Middle;
        if (events.onMouseDoubleClick) {
            events.onMouseDoubleClick(btn, x, y, KeyModifiers::Current());
        }
        return 0;
    }

    // ── Keyboard Events ──────────────────────────────────────────────────
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        // Bit 30 of lParam: 1 = key was already down (auto-repeat)
        if (events.onKeyDown) {
            events.onKeyDown(static_cast<uint32_t>(wp), KeyModifiers::Current());
        }
        return 0;
    }

    case WM_KEYUP:
    case WM_SYSKEYUP: {
        if (events.onKeyUp) {
            events.onKeyUp(static_cast<uint32_t>(wp), KeyModifiers::Current());
        }
        return 0;
    }

    case WM_CHAR: {
        wchar_t ch = static_cast<wchar_t>(wp);
        if (events.onChar) {
            events.onChar(ch);
        }
        return 0;
    }

    // ── File Drop ────────────────────────────────────────────────────────
    case WM_DROPFILES: {
        HDROP hDrop = reinterpret_cast<HDROP>(wp);
        UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
        std::vector<std::wstring> files;
        files.reserve(fileCount);

        for (UINT i = 0; i < fileCount; ++i) {
            UINT len = DragQueryFileW(hDrop, i, nullptr, 0);
            std::wstring path(len, L'\0');
            DragQueryFileW(hDrop, i, path.data(), len + 1);
            files.push_back(std::move(path));
        }

        DragFinish(hDrop);

        if (events.onFileDrop && !files.empty()) {
            events.onFileDrop(std::move(files));
        }
        return 0;
    }

    // ── Close ────────────────────────────────────────────────────────────
    case WM_CLOSE: {
        if (events.onCloseRequest) {
            if (!events.onCloseRequest()) {
                return 0; // Close cancelled by the application
            }
        }
        DestroyWindow(m_hwnd);
        return 0;
    }

    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }

    } // switch

    return DefWindowProcW(m_hwnd, msg, wp, lp);
}

} // namespace kv
