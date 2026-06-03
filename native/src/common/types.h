// ============================================================================
// Class: Types
// Description: Types defines the core primitives, vectors, and logging macro functions shared across modules in [IMAGE_VIEWER_APP_NAME].
// Responsibility: Colors, geometric rectangles, high-resolution clocks, and file log writers.
// ============================================================================
#pragma once

#include "common/pch.h"
#include <share.h>

namespace kv {

// ─── COM Smart Pointer Alias ─────────────────────────────────────────────────
template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

// ─── Logging ─────────────────────────────────────────────────────────────────
// Simple debug logging. In Release builds, Info/Debug are compiled out.
// All log messages go to OutputDebugString (visible in VS Output window).

enum class LogLevel { Debug, Info, Warn, Error };

namespace detail {

inline void LogImpl(LogLevel level, std::wstring_view message) {
    const wchar_t* prefix = L"";
    switch (level) {
        case LogLevel::Debug: prefix = L"[DEBUG] "; break;
        case LogLevel::Info:  prefix = L"[INFO]  "; break;
        case LogLevel::Warn:  prefix = L"[WARN]  "; break;
        case LogLevel::Error: prefix = L"[ERROR] "; break;
    }
    std::wstring full = std::wstring(prefix) + std::wstring(message) + L"\n";
    OutputDebugStringW(full.c_str());

    // Also write to a log file next to the executable for post-mortem diagnostics
    static std::once_flag s_initFlag;
    static FILE* s_logFile = nullptr;
    std::call_once(s_initFlag, []() {
        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        // Replace filename with kv_log.txt
        wchar_t* lastSlash = wcsrchr(exePath, L'\\');
        if (lastSlash) *(lastSlash + 1) = L'\0';
        wcscat_s(exePath, L"kv_log.txt");
        s_logFile = _wfsopen(exePath, L"a,ccs=UTF-8", _SH_DENYNO);
    });
    if (s_logFile) {
        fwprintf(s_logFile, L"%s", full.c_str());
        fflush(s_logFile);
    }
}

} // namespace detail


inline void LogDebug(std::wstring_view msg) {
#ifdef _DEBUG
    detail::LogImpl(LogLevel::Debug, msg);
#endif
}
inline void LogInfo(std::wstring_view msg)  { detail::LogImpl(LogLevel::Info, msg); }
inline void LogWarn(std::wstring_view msg)  { detail::LogImpl(LogLevel::Warn, msg); }
inline void LogError(std::wstring_view msg) { detail::LogImpl(LogLevel::Error, msg); }

// ─── HRESULT Helpers ─────────────────────────────────────────────────────────
// Check HRESULT and log on failure. Returns false if FAILED.

inline bool HrCheck(HRESULT hr, const wchar_t* context = L"") {
    if (FAILED(hr)) {
        LogError(std::format(L"{} failed with HRESULT 0x{:08X}", context, static_cast<unsigned>(hr)));
        return false;
    }
    return true;
}

// Macro version that returns false from the calling function on failure.
// Use in initialization functions that return bool.
#define KV_HR_RETURN(hr, ctx) \
    do { if (!::kv::HrCheck((hr), (ctx))) return false; } while (0)

// Macro that returns the HRESULT directly (for functions returning HRESULT).
#define KV_HR_FAIL(hr, ctx) \
    do { HRESULT _hr = (hr); if (FAILED(_hr)) { ::kv::LogError(std::format(L"{} failed: 0x{:08X}", ctx, static_cast<unsigned>(_hr))); return _hr; } } while (0)

// ─── Geometric Types ─────────────────────────────────────────────────────────

struct PointF {
    float x = 0.0f;
    float y = 0.0f;

    constexpr PointF() = default;
    constexpr PointF(float x, float y) : x(x), y(y) {}

    // Convert to D2D point
    D2D1_POINT_2F ToD2D() const { return D2D1::Point2F(x, y); }

    PointF operator+(const PointF& o) const { return { x + o.x, y + o.y }; }
    PointF operator-(const PointF& o) const { return { x - o.x, y - o.y }; }
    PointF operator*(float s) const { return { x * s, y * s }; }
};

struct SizeF {
    float width  = 0.0f;
    float height = 0.0f;

    constexpr SizeF() = default;
    constexpr SizeF(float w, float h) : width(w), height(h) {}

    D2D1_SIZE_F ToD2D() const { return D2D1::SizeF(width, height); }
    constexpr bool IsEmpty() const { return width <= 0.0f || height <= 0.0f; }
};

struct SizeU {
    uint32_t width  = 0;
    uint32_t height = 0;

    constexpr SizeU() = default;
    constexpr SizeU(uint32_t w, uint32_t h) : width(w), height(h) {}

    D2D1_SIZE_U ToD2D() const { return D2D1::SizeU(width, height); }
    constexpr SizeF ToFloat() const {
        return { static_cast<float>(width), static_cast<float>(height) };
    }
};

struct RectF {
    float left   = 0.0f;
    float top    = 0.0f;
    float right  = 0.0f;
    float bottom = 0.0f;

    constexpr RectF() = default;
    constexpr RectF(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) {}

    // Factory: create from position + size
    static constexpr RectF FromXYWH(float x, float y, float w, float h) {
        return { x, y, x + w, y + h };
    }

    // Factory: centered at a point
    static constexpr RectF Centered(float cx, float cy, float w, float h) {
        return { cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2 };
    }

    constexpr float Width()  const { return right - left; }
    constexpr float Height() const { return bottom - top; }
    constexpr PointF Center() const { return { (left + right) / 2, (top + bottom) / 2 }; }
    constexpr bool Contains(float x, float y) const {
        return x >= left && x < right && y >= top && y < bottom;
    }
    constexpr bool Intersects(const RectF& o) const {
        return left < o.right && right > o.left && top < o.bottom && bottom > o.top;
    }
    constexpr PointF TopLeft()     const { return { left, top }; }
    constexpr PointF TopRight()    const { return { right, top }; }
    constexpr PointF BottomLeft()  const { return { left, bottom }; }
    constexpr PointF BottomRight() const { return { right, bottom }; }
    constexpr RectF Inset(float amount) const {
        return { left + amount, top + amount, right - amount, bottom - amount };
    }

    D2D1_RECT_F ToD2D() const { return D2D1::RectF(left, top, right, bottom); }
};

// ─── Color ───────────────────────────────────────────────────────────────────

struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    constexpr Color() = default;
    constexpr Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}

    // Construct from 0-255 integer values
    static constexpr Color FromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        return { r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
    }

    // Construct from hex (0xRRGGBB or 0xRRGGBBAA)
    static constexpr Color FromHex(uint32_t hex, float alpha = 1.0f) {
        return {
            ((hex >> 16) & 0xFF) / 255.0f,
            ((hex >> 8)  & 0xFF) / 255.0f,
            ((hex >> 0)  & 0xFF) / 255.0f,
            alpha
        };
    }

    // Pre-multiplied alpha (required for DirectComposition blending)
    constexpr Color Premultiplied() const { return { r * a, g * a, b * a, a }; }

    D2D1_COLOR_F ToD2D() const { return { r, g, b, a }; }

    // Common colors
    static constexpr Color Transparent() { return { 0, 0, 0, 0 }; }
    static constexpr Color Black(float a = 1) { return { 0, 0, 0, a }; }
    static constexpr Color White(float a = 1) { return { 1, 1, 1, a }; }
};

// ─── Mouse / Keyboard Input ─────────────────────────────────────────────────

enum class MouseButton { Left, Right, Middle, X1, X2 };

struct MouseState {
    PointF position;
    bool leftDown   = false;
    bool rightDown  = false;
    bool middleDown = false;
};

struct KeyModifiers {
    bool ctrl  = false;
    bool shift = false;
    bool alt   = false;

    static KeyModifiers Current() {
        return {
            (GetKeyState(VK_CONTROL) & 0x8000) != 0,
            (GetKeyState(VK_SHIFT)   & 0x8000) != 0,
            (GetKeyState(VK_MENU)    & 0x8000) != 0
        };
    }
};

// ─── Scoped COM Initializer ──────────────────────────────────────────────────
// RAII wrapper for CoInitializeEx / CoUninitialize.
// Must be created before any COM objects and destroyed after all are released.

class ScopedComInit {
public:
    ScopedComInit() {
        m_hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    }
    ~ScopedComInit() {
        if (SUCCEEDED(m_hr)) {
            CoUninitialize();
        }
    }
    bool IsValid() const { return SUCCEEDED(m_hr); }

    ScopedComInit(const ScopedComInit&) = delete;
    ScopedComInit& operator=(const ScopedComInit&) = delete;

private:
    HRESULT m_hr;
};

// ─── High-Resolution Timer ──────────────────────────────────────────────────

class Timer {
public:
    Timer() {
        QueryPerformanceFrequency(&m_frequency);
        Reset();
    }

    void Reset() {
        QueryPerformanceCounter(&m_start);
        m_last = m_start;
    }

    // Seconds since last call to Tick() (or Reset)
    double Tick() {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double delta = static_cast<double>(now.QuadPart - m_last.QuadPart)
                     / static_cast<double>(m_frequency.QuadPart);
        m_last = now;
        return delta;
    }

    // Seconds since Reset()
    double Elapsed() const {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return static_cast<double>(now.QuadPart - m_start.QuadPart)
             / static_cast<double>(m_frequency.QuadPart);
    }

private:
    LARGE_INTEGER m_frequency{};
    LARGE_INTEGER m_start{};
    LARGE_INTEGER m_last{};
};

// ─── DPI Helpers ─────────────────────────────────────────────────────────────

inline float DpiScale(uint32_t dpi) {
    return static_cast<float>(dpi) / 96.0f;
}

inline int DpiScaleInt(int value, uint32_t dpi) {
    return MulDiv(value, static_cast<int>(dpi), 96);
}

} // namespace kv
