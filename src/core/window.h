// ============================================================================
// Class: Window
// Description: Window encapsulates the Win32 window creation and message dispatching for [IMAGE_VIEWER_APP_NAME], providing support for transparent overlays.
// Responsibility: Handle low-level Win32 window creation, Custom Hit-Testing, drag-and-drop registration, and DPI scaling events.
// ============================================================================
#pragma once

#include "common/types.h"

namespace kv {

// ─── Window Configuration ────────────────────────────────────────────────────

// Nonpersisted creation parameters; Window owns the resulting HWND on its creating UI thread.
struct WindowConfig {
    std::wstring title       = L"Image Viewer";
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
    std::function<void()>                                                 onMouseLeave;

    // Keyboard
    std::function<void(uint32_t vk, KeyModifiers)>    onKeyDown;
    std::function<void(uint32_t vk, KeyModifiers)>    onKeyUp;
    std::function<void(wchar_t ch)>                   onChar;

    // File drop
    std::function<void(std::vector<std::wstring>)>    onFileDrop;

    // IPC open path
    std::function<void(const std::wstring&)>          onIpcOpenPath;

    // Tray and Command
    std::function<void(LPARAM)>                       onTrayIconEvent;
    std::function<void(uint16_t)>                     onCommand;

    // UI: true keeps custom viewer/overlay surfaces in client input instead of native caption gestures.
    std::function<bool(float, float)> onClientHitTest;

    // UI: lets the owner publish its custom client accessibility object; zero falls back to Win32.
    std::function<LRESULT(WPARAM, LPARAM)> onGetObject;

    // Close request: return true to allow close, false to cancel
    std::function<bool()>                             onCloseRequest;
};

// ─── Window Class ────────────────────────────────────────────────────────────

class Window {
    friend class StartupCacheTests; // Isolated WARP async tests suppress real-window sizing.
    friend class AsyncPreviewTests; // Owns a message-only HWND in callback tests.
    friend class IpcDiagnosticTests; // Routes message-only receiver traffic through the real handler.
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
    // UI: reports alpha composition for transparent exterior viewer gestures.
    bool IsTranslucent() const { return m_translucent; }
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
    // UI: translates native messages into owner callbacks, including custom client accessibility.
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp);

    // ── Hit Testing (frameless window) ────────────────────────────────────
    // UI: preserves resize borders, then offers custom surfaces before the native caption band.
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
