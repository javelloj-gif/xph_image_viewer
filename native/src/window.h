// ============================================================================
// window.h — Win32 Frameless Window with Translucent Background
// Krumassan Image Viewer — Native C++20
//
// Provides a frameless, DPI-aware, translucent Win32 window suitable for
// DirectComposition-based rendering. Key features:
//   - WS_POPUP frameless window with custom hit-testing for resize/move
//   - Per-monitor DPI awareness (WM_DPICHANGED)
//   - Translucent background support (WS_EX_NOREDIRECTIONBITMAP)
//   - Fullscreen toggle
//   - File drag-and-drop
//   - Event callbacks via std::function (no virtual dispatch overhead)
//
// Lesson from Python optimization failure:
//   The QOpenGLWidget approach lost the translucent background effect.
//   Here we use WS_EX_NOREDIRECTIONBITMAP + DirectComposition, which
//   provides true per-pixel alpha transparency without DWM hacks.
// ============================================================================
#pragma once

#include "types.h"

namespace kv {

// ─── Window Configuration ────────────────────────────────────────────────────

struct WindowConfig {
    std::wstring title       = L"Krumassan Image Viewer";
    int          width       = 1280;
    int          height      = 800;
    bool         frameless   = true;
    bool         translucent = true;    // per-pixel alpha via DirectComposition
    bool         topmost     = false;
    bool         acceptDrops = true;
    HICON        icon        = nullptr; // application icon
};

// ─── Window Events ───────────────────────────────────────────────────────────
// All callbacks are optional. Set only the ones you need.

struct WindowEvents {
    // Rendering
    std::function<void()>                             onPaint;

    // Resize / DPI
    std::function<void(uint32_t w, uint32_t h)>       onResize;
    std::function<void(uint32_t dpi)>                  onDpiChanged;

    // Mouse
    std::function<void(float x, float y, KeyModifiers)>                   onMouseMove;
    std::function<void(MouseButton, float x, float y, KeyModifiers)>      onMouseDown;
    std::function<void(MouseButton, float x, float y, KeyModifiers)>      onMouseUp;
    std::function<void(float delta, float x, float y, KeyModifiers)>      onMouseWheel;
    std::function<void(MouseButton, float x, float y, KeyModifiers)>      onMouseDoubleClick;

    // Keyboard
    std::function<void(uint32_t vk, KeyModifiers)>    onKeyDown;
    std::function<void(uint32_t vk, KeyModifiers)>    onKeyUp;
    std::function<void(wchar_t ch)>                   onChar;

    // File drop
    std::function<void(std::vector<std::wstring>)>    onFileDrop;

    // Close request: return true to allow close, false to cancel
    std::function<bool()>                             onCloseRequest;
};

// ─── Window Class ────────────────────────────────────────────────────────────

class Window {
public:
    Window() = default;
    ~Window();

    // Non-copyable, non-movable (HWND lifetime is tied to this object)
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // ── Lifecycle ──────────────────────────────────────────────────────────
    bool Create(const WindowConfig& config);
    void Show(int cmdShow = SW_SHOW);
    int  RunMessageLoop();
    void Close();

    // ── Accessors ──────────────────────────────────────────────────────────
    HWND     GetHwnd()       const { return m_hwnd; }
    SizeU    GetClientSize() const;
    uint32_t GetDpi()        const { return m_dpi; }
    float    GetDpiScale()   const { return DpiScale(m_dpi); }
    bool     IsFullscreen()  const { return m_fullscreen; }
    bool     IsFrameless()   const { return m_frameless; }
    bool     CheckMinimized()  const;
    bool     CheckMaximized()  const;

    // ── Window State ───────────────────────────────────────────────────────
    void SetTitle(const std::wstring& title);
    void SetFullscreen(bool fullscreen);
    void ToggleFullscreen() { SetFullscreen(!m_fullscreen); }
    void Minimize();
    void Maximize();
    void Restore();
    void RequestRedraw();

    // ── Events ─────────────────────────────────────────────────────────────
    WindowEvents events;

private:
    // ── Win32 Message Handling ─────────────────────────────────────────────
    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp);

    // ── Hit Testing (frameless window) ────────────────────────────────────
    LRESULT HitTest(int x, int y) const;

    // ── Internal Helpers ──────────────────────────────────────────────────
    void UpdateDpi(uint32_t newDpi);
    void SaveWindowPlacement();
    void RestoreWindowPlacement();

    // ── Members ───────────────────────────────────────────────────────────
    HWND              m_hwnd       = nullptr;
    uint32_t          m_dpi        = 96;
    bool              m_frameless  = true;
    bool              m_translucent = true;
    bool              m_fullscreen = false;
    int               m_resizeBorderWidth = 6; // pixels for resize hit zone

    // Saved placement for fullscreen toggle
    WINDOWPLACEMENT   m_savedPlacement{};
    bool              m_hasValidPlacement = false;
};

} // namespace kv
