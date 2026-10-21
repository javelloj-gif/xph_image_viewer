// ============================================================================
// Class: Renderer
// Description: Renderer wraps Direct2D and Direct3D 11 initialization and provides hardware-accelerated drawing APIs for [IMAGE_VIEWER_APP_NAME].
// Responsibility: DXGI swap-chains, DirectComposition target blending, and Direct2D primitive draw calls.
// ============================================================================
#pragma once

#include "common/types.h"

namespace kv {

// ─── Text Alignment ──────────────────────────────────────────────────────────

enum class TextAlign { Left, Center, Right };
enum class TextVAlign { Top, Center, Bottom };

// ─── Bitmap Interpolation ────────────────────────────────────────────────────

enum class BitmapInterpolation {
    NearestNeighbor,   // Fast, pixelated at low zoom
    Linear,            // Smooth, default for most uses
    HighQuality        // Best quality, uses multi-sample filtering
};

// ─── Renderer Class ──────────────────────────────────────────────────────────

class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    // Non-copyable
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // ── Lifecycle ──────────────────────────────────────────────────────────

    /// Initialize the entire rendering pipeline for the given window.
    /// Creates D3D11 device, DXGI swap chain, D2D device context,
    /// DirectWrite factory, and DirectComposition binding.
    bool Initialize(HWND hwnd, bool translucent = true);

    /// Resize the swap chain when the window size changes.
    /// Must be called from WM_SIZE handler.
    void Resize(uint32_t width, uint32_t height);

    // ── Frame Management ───────────────────────────────────────────────────

    /// Begin a new frame. Acquires the render target and sets it as current.
    void BeginFrame();

    /// End the frame and present to screen. Calls IDXGISwapChain::Present.
    void EndFrame();

    /// Clear the render target with the given color.
    /// For translucent windows, use Color::Transparent() or Color::Black(alpha).
    void Clear(const Color& color);

    // ── Drawing Primitives ─────────────────────────────────────────────────

    void DrawBitmap(
        ID2D1Bitmap1* bitmap,
        const RectF& destRect,
        float opacity = 1.0f,
        BitmapInterpolation interpolation = BitmapInterpolation::Linear,
        const RectF* srcRect = nullptr
    );

    void FillRect(const RectF& rect, const Color& color);
    void DrawRect(const RectF& rect, const Color& color, float strokeWidth = 1.0f);

    void FillRoundedRect(const RectF& rect, float radiusX, float radiusY, const Color& color);
    void DrawRoundedRect(const RectF& rect, float radiusX, float radiusY,
                         const Color& color, float strokeWidth = 1.0f);

    void FillEllipse(const PointF& center, float radiusX, float radiusY, const Color& color);
    void DrawEllipse(const PointF& center, float radiusX, float radiusY,
                     const Color& color, float strokeWidth = 1.0f);

    void DrawLine(const PointF& p1, const PointF& p2,
                  const Color& color, float strokeWidth = 1.0f);

    // ── Text Rendering ─────────────────────────────────────────────────────

    void DrawText(
        std::wstring_view text,
        const RectF& layoutRect,
        const Color& color,
        float fontSize = 14.0f,
        TextAlign hAlign = TextAlign::Left,
        TextVAlign vAlign = TextVAlign::Top,
        const wchar_t* fontFamily = L"Segoe UI"
    );

    /// Measure text dimensions.
    PointF MeasureText(
        std::wstring_view text,
        float fontSize = 14.0f,
        const wchar_t* fontFamily = L"Segoe UI"
    );

    // ── Transform Stack ────────────────────────────────────────────────────
    // Transforms are concatenated (multiplied). Push/Pop to nest transforms.
    // This mirrors the Python BoardItem transform stack.

    void PushTransform(const D2D1_MATRIX_3X2_F& transform);
    void PopTransform();

    /// Convenience: push a translation transform
    void PushTranslation(float x, float y);

    /// Convenience: push a scale transform around a center point
    void PushScale(float sx, float sy, const PointF& center = {});

    /// Convenience: push a rotation transform around a center point (degrees)
    void PushRotation(float angleDegrees, const PointF& center = {});

    /// Reset the transform stack to identity
    void ResetTransform();

    // ── Clip Stack ─────────────────────────────────────────────────────────

    void PushClip(const RectF& rect);
    void PopClip();

    // ── Resource Creation ──────────────────────────────────────────────────

    /// Create a GPU bitmap from a WIC bitmap source.
    /// This must be called on the main/render thread only.
    [[nodiscard]]
    ComPtr<ID2D1Bitmap1> CreateBitmapFromWIC(IWICBitmapSource* wicBitmap);

    /// Create a GPU bitmap from raw pixel data (BGRA, 32bpp, premultiplied alpha).
    [[nodiscard]]
    ComPtr<ID2D1Bitmap1> CreateBitmapFromMemory(
        const uint8_t* data, uint32_t width, uint32_t height, uint32_t stride = 0
    );

    /// Create a text format for custom text rendering.
    [[nodiscard]]
    ComPtr<IDWriteTextFormat> CreateTextFormat(
        const wchar_t* fontFamily, float fontSize,
        DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_REGULAR,
        DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL
    );

    // ── Accessors ──────────────────────────────────────────────────────────

    ID2D1DeviceContext* GetD2DContext() const { return m_d2dContext.Get(); }
    ID3D11Device*       GetD3DDevice()  const { return m_d3dDevice.Get(); }
    SizeU               GetTargetSize() const;
    bool                IsInitialized() const { return m_initialized; }

    /// Get or create a solid color brush (cached for the current frame).
    ID2D1SolidColorBrush* GetBrush(const Color& color);

private:
    // ── Internal Setup ─────────────────────────────────────────────────────
    bool CreateDeviceResources(HWND hwnd, bool translucent);
    bool CreateSwapChain(HWND hwnd, bool translucent);
    bool CreateRenderTarget();
    bool SetupDirectComposition(HWND hwnd);
    void DiscardDeviceResources();

    /// Handle device lost: recreate all device-dependent resources.
    bool HandleDeviceLost(HWND hwnd, bool translucent);

    // ── Device Resources ───────────────────────────────────────────────────
    ComPtr<ID3D11Device>            m_d3dDevice;
    ComPtr<ID3D11DeviceContext>     m_d3dContext;
    ComPtr<IDXGISwapChain1>         m_swapChain;
    ComPtr<ID2D1Factory3>           m_d2dFactory;
    ComPtr<ID2D1Device2>            m_d2dDevice;
    ComPtr<ID2D1DeviceContext2>     m_d2dContext;
    ComPtr<ID2D1Bitmap1>            m_renderTarget;
    ComPtr<IDWriteFactory3>         m_dwriteFactory;

    // DirectComposition (per-pixel alpha transparency)
    ComPtr<IDCompositionDevice>     m_dcompDevice;
    ComPtr<IDCompositionTarget>     m_dcompTarget;
    ComPtr<IDCompositionVisual>     m_dcompVisual;

    // Reusable brush (color is set per-draw call)
    ComPtr<ID2D1SolidColorBrush>    m_brush;

    // Transform stack
    std::vector<D2D1_MATRIX_3X2_F>  m_transformStack;

    // Clip stack depth (for matching Push/Pop)
    uint32_t m_clipDepth = 0;

    // State
    bool     m_initialized = false;
    bool     m_translucent = true;
    HWND     m_hwnd = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace kv
