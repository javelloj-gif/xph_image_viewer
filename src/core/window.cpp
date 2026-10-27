// ============================================================================
// Class: Window
// Description: Window encapsulates the Win32 window creation and message dispatching for Fotassa, providing support for transparent overlays.
// Responsibility: Handle low-level Win32 window creation, Custom Hit-Testing, drag-and-drop registration, and DPI scaling events.
// ============================================================================
#include "common/pch.h"
#include "window.h"

namespace kv {

static constexpr const wchar_t* kWindowClassName = L"LenteUVWindow";
static bool sWindowClassRegistered = false;

Window::~Window() {
    if (m_hwnd) {
        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, 0);
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

bool Window::Create(const WindowConfig& config) {
    m_frameless   = config.frameless;
    m_translucent = config.translucent;

    if (!sWindowClassRegistered) {
        WNDCLASSEXW wc = {};
        wc.cbSize        = sizeof(wc);
        wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc   = StaticWndProc;
        wc.hInstance     = GetModuleHandleW(nullptr);
        wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = kWindowClassName;
        wc.hIcon         = config.icon;
        wc.hIconSm       = config.icon;

        if (!RegisterClassExW(&wc)) {
            LogError(L"RegisterClassExW failed");
            return false;
        }
        sWindowClassRegistered = true;
    }

    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exStyle = WS_EX_APPWINDOW;

    if (m_frameless) {
        style = WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
    }

    if (m_translucent) {
        exStyle |= WS_EX_NOREDIRECTIONBITMAP;
    }

    m_dpi = GetDpiForSystem();

    int scaledWidth  = DpiScaleInt(config.width, m_dpi);
    int scaledHeight = DpiScaleInt(config.height, m_dpi);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenW - scaledWidth) / 2;
    int y = (screenH - scaledHeight) / 2;

    m_hwnd = CreateWindowExW(
        exStyle,
        kWindowClassName,
        config.title.c_str(),
        style,
        x, y, scaledWidth, scaledHeight,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        this
    );

    if (!m_hwnd) {
        LogError(std::format(L"CreateWindowExW failed: {}", GetLastError()));
        return false;
    }

    if (config.acceptDrops) {
        DragAcceptFiles(m_hwnd, TRUE);
        ChangeWindowMessageFilterEx(m_hwnd, WM_DROPFILES, MSGFLT_ALLOW, nullptr);
        ChangeWindowMessageFilterEx(m_hwnd, WM_COPYDATA,  MSGFLT_ALLOW, nullptr);
        ChangeWindowMessageFilterEx(m_hwnd, 0x0049, MSGFLT_ALLOW, nullptr);
    }

    LogInfo(std::format(L"Window created: {}x{} @ {}dpi (frameless={}, translucent={})",
        scaledWidth, scaledHeight, m_dpi, m_frameless, m_translucent));

    return true;
}

void Window::Show(int cmdShow) {
    ShowWindow(m_hwnd, cmdShow);
    UpdateWindow(m_hwnd);
}

void Window::Close() {
    if (m_hwnd) {
        PostMessageW(m_hwnd, WM_CLOSE, 0, 0);
    }
}

int Window::RunMessageLoop() {
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

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
    m_redrawPending.store(true, std::memory_order_relaxed);
    if (m_hwnd) InvalidateRect(m_hwnd, nullptr, FALSE);
}

bool Window::ConsumeRedrawRequest() {
    if (!IsWindowVisible(m_hwnd) || CheckMinimized()) return false;
    if (!m_redrawPending.exchange(false, std::memory_order_relaxed)) return false;
    ValidateRect(m_hwnd, nullptr);
    return true;
}

void Window::SetFullscreen(bool fullscreen) {
    if (m_fullscreen == fullscreen) return;

    m_fullscreen = fullscreen;

    if (fullscreen) {
        SaveWindowPlacement();

        HMONITOR hMon = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfoW(hMon, &mi);

        SetWindowLongW(m_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(m_hwnd, HWND_TOP,
            mi.rcMonitor.left, mi.rcMonitor.top,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
    } else {
        DWORD style = WS_OVERLAPPEDWINDOW;
        if (m_frameless) {
            style = WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
        }
        SetWindowLongW(m_hwnd, GWL_STYLE, style | WS_VISIBLE);
        RestoreWindowPlacement();
    }
}

void Window::Minimize() { ShowWindow(m_hwnd, SW_MINIMIZE); }
void Window::Maximize() { ShowWindow(m_hwnd, SW_MAXIMIZE); }
void Window::Restore()  { ShowWindow(m_hwnd, SW_RESTORE); }

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

void Window::UpdateDpi(uint32_t newDpi) {
    if (m_dpi == newDpi) return;
    uint32_t oldDpi = m_dpi;
    m_dpi = newDpi;

    m_resizeBorderWidth = DpiScaleInt(6, m_dpi);

    LogInfo(std::format(L"DPI changed: {} → {}", oldDpi, newDpi));

    if (events.onDpiChanged) {
        events.onDpiChanged(newDpi);
    }
}

LRESULT CALLBACK Window::StaticWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    Window* self = nullptr;

    if (msg == WM_CREATE) {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lp);
        self = static_cast<Window*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;

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

LRESULT Window::HitTest(int x, int y) const {
    RECT rc;
    GetWindowRect(m_hwnd, &rc);

    int border = m_resizeBorderWidth;

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

    int localX = x - rc.left;
    int localY = y - rc.top;
    int width = rc.right - rc.left;
    if (localX >= (width - 40) && localX <= (width - 8) &&
        localY >= 8 && localY <= 40) {
        return HTCLIENT;
    }

    if (events.onClientHitTest && events.onClientHitTest(static_cast<float>(localX), static_cast<float>(localY))) return HTCLIENT;

    int titleBarHeight = DpiScaleInt(32, m_dpi);
    if ((y - rc.top) < titleBarHeight) {
        return HTCAPTION;
    }

    return HTCLIENT;
}

LRESULT Window::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    // Input can change hover/editor state even when its handler does not invalidate explicitly.
    if ((msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) ||
        (msg >= WM_KEYFIRST && msg <= WM_KEYLAST) || msg == WM_MOUSELEAVE ||
        msg == WM_COMMAND || msg == WM_DROPFILES || msg == WM_SHOWWINDOW ||
        msg == WM_SIZE || msg == WM_DPICHANGED || msg == WM_ACTIVATE ||
        msg == WM_SETFOCUS || msg == WM_KILLFOCUS) {
        m_redrawPending.store(true, std::memory_order_relaxed);
    }
    switch (msg) {

    case WM_GETOBJECT:
        if (events.onGetObject) { const auto result = events.onGetObject(wp, lp); if (result) return result; }
        break;

    case WM_PAINT: {
        m_redrawPending.store(true, std::memory_order_relaxed);
        ValidateRect(m_hwnd, nullptr);
        if (events.onPaint) {
            events.onPaint();
        }
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_SIZE: {
        if (wp == SIZE_MINIMIZED) return 0;
        uint32_t w = LOWORD(lp);
        uint32_t h = HIWORD(lp);
        if (w > 0 && h > 0 && events.onResize) {
            events.onResize(w, h);
        }
        return 0;
    }

    case WM_DPICHANGED: {
        uint32_t newDpi = HIWORD(wp);
        UpdateDpi(newDpi);

        auto suggestedRect = reinterpret_cast<const RECT*>(lp);
        SetWindowPos(m_hwnd, nullptr,
            suggestedRect->left, suggestedRect->top,
            suggestedRect->right - suggestedRect->left,
            suggestedRect->bottom - suggestedRect->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }

    case WM_NCHITTEST: {
        if (m_frameless && !m_fullscreen) {
            return HitTest(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        }
        break;
    }

    case WM_NCCALCSIZE: {
        if (m_frameless && wp == TRUE) {
            return 0;
        }
        break;
    }

    case WM_MOUSEMOVE: {
        TRACKMOUSEEVENT tme = {};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hwnd;
        TrackMouseEvent(&tme);

        float x = static_cast<float>(GET_X_LPARAM(lp));
        float y = static_cast<float>(GET_Y_LPARAM(lp));
        if (events.onMouseMove) {
            events.onMouseMove(x, y, KeyModifiers::Current());
        }
        return 0;
    }

    case WM_MOUSELEAVE: {
        if (events.onMouseLeave) {
            events.onMouseLeave();
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

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        if (msg == WM_SYSKEYDOWN && wp == VK_F4) {
            break; // Dejar fluir a DefWindowProcW para Alt+F4
        }
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

    case WM_USER + 4501: { // WM_IPC_OPEN_PATH
        std::wstring* pPath = reinterpret_cast<std::wstring*>(wp);
        if (pPath) {
            if (events.onIpcOpenPath) {
                events.onIpcOpenPath(*pPath);
            }
            delete pPath;
        }
        return 0;
    }

    case WM_USER + 4502: { // WM_TRAY_ICON
        if (events.onTrayIconEvent) {
            events.onTrayIconEvent(lp);
        }
        return 0;
    }

    case WM_COMMAND: {
        if (events.onCommand) {
            events.onCommand(LOWORD(wp));
        }
        return 0;
    }

    case WM_CLOSE: {
        if (events.onCloseRequest) {
            if (!events.onCloseRequest()) {
                return 0;
            }
        }
        DestroyWindow(m_hwnd);
        return 0;
    }

    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }

    }

    return DefWindowProcW(m_hwnd, msg, wp, lp);
}

} // namespace kv
