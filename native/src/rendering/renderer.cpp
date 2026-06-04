// ============================================================================
// Class: Renderer
// Description: Renderer wraps Direct2D and Direct3D 11 initialization and provides hardware-accelerated drawing APIs for [IMAGE_VIEWER_APP_NAME].
// Responsibility: DXGI swap-chains, DirectComposition target blending, and Direct2D primitive draw calls.
// ============================================================================
#include "common/pch.h"
#include "renderer.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dcomp.lib")

namespace kv {

bool Renderer::Initialize(HWND hwnd, bool translucent) {
    m_hwnd = hwnd;
    m_translucent = translucent;

    RECT rc;
    GetClientRect(hwnd, &rc);
    m_width  = static_cast<uint32_t>(rc.right);
    m_height = static_cast<uint32_t>(rc.bottom);

    if (!CreateDeviceResources(hwnd, translucent)) {
        LogError(L"Failed to create device resources");
        return false;
    }

    m_initialized = true;
    LogInfo(std::format(L"Renderer initialized: {}x{} (translucent={})", m_width, m_height, translucent));
    return true;
}

bool Renderer::CreateDeviceResources(HWND hwnd, bool translucent) {
    UINT d3dFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    d3dFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    D3D_FEATURE_LEVEL achievedLevel;
    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        d3dFlags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &m_d3dDevice,
        &achievedLevel,
        &m_d3dContext
    );

    if (FAILED(hr) && (d3dFlags & D3D11_CREATE_DEVICE_DEBUG)) {
        d3dFlags &= ~D3D11_CREATE_DEVICE_DEBUG;
        hr = D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, d3dFlags,
            featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION,
            &m_d3dDevice, &achievedLevel, &m_d3dContext
        );
    }
    KV_HR_RETURN(hr, L"D3D11CreateDevice");

    LogInfo(std::format(L"D3D11 device created (feature level: 0x{:X})",
        static_cast<unsigned>(achievedLevel)));

    D2D1_FACTORY_OPTIONS factoryOptions = {};
#ifdef _DEBUG
    factoryOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

    hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        __uuidof(ID2D1Factory3),
        &factoryOptions,
        reinterpret_cast<void**>(m_d2dFactory.ReleaseAndGetAddressOf())
    );
    KV_HR_RETURN(hr, L"D2D1CreateFactory");

    ComPtr<IDXGIDevice> dxgiDevice;
    KV_HR_RETURN(m_d3dDevice.As(&dxgiDevice), L"QueryInterface(IDXGIDevice)");

    hr = m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice);
    KV_HR_RETURN(hr, L"ID2D1Factory3::CreateDevice");

    hr = m_d2dDevice->CreateDeviceContext(
        D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
        &m_d2dContext
    );
    KV_HR_RETURN(hr, L"ID2D1Device::CreateDeviceContext");

    m_d2dContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory3),
        reinterpret_cast<IUnknown**>(m_dwriteFactory.ReleaseAndGetAddressOf())
    );
    KV_HR_RETURN(hr, L"DWriteCreateFactory");

    if (!CreateSwapChain(hwnd, translucent)) return false;
    if (translucent) {
        if (!SetupDirectComposition(hwnd)) return false;
    }

    if (!CreateRenderTarget()) return false;

    hr = m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 1), &m_brush);
    KV_HR_RETURN(hr, L"CreateSolidColorBrush");

    m_transformStack.clear();
    m_transformStack.push_back(D2D1::Matrix3x2F::Identity());

    return true;
}

bool Renderer::CreateSwapChain(HWND hwnd, bool translucent) {
    ComPtr<IDXGIDevice> dxgiDevice;
    KV_HR_RETURN(m_d3dDevice.As(&dxgiDevice), L"QueryInterface(IDXGIDevice)");

    ComPtr<IDXGIAdapter> adapter;
    KV_HR_RETURN(dxgiDevice->GetAdapter(&adapter), L"GetAdapter");

    ComPtr<IDXGIFactory2> dxgiFactory;
    KV_HR_RETURN(adapter->GetParent(IID_PPV_ARGS(&dxgiFactory)), L"GetParent(IDXGIFactory2)");

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width       = m_width;
    desc.Height      = m_height;
    desc.Format      = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc  = { 1, 0 };
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.Scaling     = DXGI_SCALING_STRETCH;
    desc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.AlphaMode   = translucent
        ? DXGI_ALPHA_MODE_PREMULTIPLIED
        : DXGI_ALPHA_MODE_IGNORE;

    if (translucent) {
        HRESULT hr = dxgiFactory->CreateSwapChainForComposition(
            m_d3dDevice.Get(), &desc, nullptr, &m_swapChain);
        KV_HR_RETURN(hr, L"CreateSwapChainForComposition");
    } else {
        HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(
            m_d3dDevice.Get(), hwnd, &desc, nullptr, nullptr, &m_swapChain);
        KV_HR_RETURN(hr, L"CreateSwapChainForHwnd");
    }

    LogInfo(L"Swap chain created (Flip-Sequential, premultiplied alpha)");
    return true;
}

bool Renderer::SetupDirectComposition(HWND hwnd) {
    ComPtr<IDXGIDevice> dxgiDevice;
    KV_HR_RETURN(m_d3dDevice.As(&dxgiDevice), L"QueryInterface(IDXGIDevice)");

    HRESULT hr = DCompositionCreateDevice(
        dxgiDevice.Get(),
        IID_PPV_ARGS(&m_dcompDevice)
    );
    KV_HR_RETURN(hr, L"DCompositionCreateDevice");

    hr = m_dcompDevice->CreateTargetForHwnd(hwnd, TRUE, &m_dcompTarget);
    KV_HR_RETURN(hr, L"CreateTargetForHwnd");

    hr = m_dcompDevice->CreateVisual(&m_dcompVisual);
    KV_HR_RETURN(hr, L"CreateVisual");

    hr = m_dcompVisual->SetContent(m_swapChain.Get());
    KV_HR_RETURN(hr, L"SetContent(swapChain)");

    hr = m_dcompTarget->SetRoot(m_dcompVisual.Get());
    KV_HR_RETURN(hr, L"SetRoot");

    hr = m_dcompDevice->Commit();
    KV_HR_RETURN(hr, L"DCompositionDevice::Commit");

    LogInfo(L"DirectComposition configured for per-pixel alpha transparency");
    return true;
}

bool Renderer::CreateRenderTarget() {
    ComPtr<IDXGISurface> dxgiSurface;
    KV_HR_RETURN(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiSurface)),
        L"GetBuffer(0)");

    D2D1_BITMAP_PROPERTIES1 bitmapProps = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                          m_translucent ? D2D1_ALPHA_MODE_PREMULTIPLIED
                                        : D2D1_ALPHA_MODE_IGNORE)
    );

    HRESULT hr = m_d2dContext->CreateBitmapFromDxgiSurface(
        dxgiSurface.Get(), &bitmapProps, &m_renderTarget);
    KV_HR_RETURN(hr, L"CreateBitmapFromDxgiSurface");

    m_d2dContext->SetTarget(m_renderTarget.Get());

    return true;
}

void Renderer::Resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return;
    if (width == m_width && height == m_height) return;

    m_width = width;
    m_height = height;

    m_d2dContext->SetTarget(nullptr);
    m_renderTarget.Reset();

    HRESULT hr = m_swapChain->ResizeBuffers(
        0,
        width,
        height,
        DXGI_FORMAT_UNKNOWN,
        0
    );

    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        HandleDeviceLost(m_hwnd, m_translucent);
        return;
    }

    if (!HrCheck(hr, L"ResizeBuffers")) return;

    CreateRenderTarget();

    LogDebug(std::format(L"Renderer resized to {}x{}", width, height));
}

void Renderer::BeginFrame() {
    m_d2dContext->BeginDraw();
    m_transformStack.clear();
    m_transformStack.push_back(D2D1::Matrix3x2F::Identity());
    m_d2dContext->SetTransform(D2D1::Matrix3x2F::Identity());
    m_clipDepth = 0;
}

void Renderer::EndFrame() {
    while (m_clipDepth > 0) {
        m_d2dContext->PopAxisAlignedClip();
        --m_clipDepth;
    }

    HRESULT hr = m_d2dContext->EndDraw();

    if (hr == D2DERR_RECREATE_TARGET) {
        HandleDeviceLost(m_hwnd, m_translucent);
        return;
    }

    DXGI_PRESENT_PARAMETERS params = {};
    hr = m_swapChain->Present1(1, 0, &params);

    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        HandleDeviceLost(m_hwnd, m_translucent);
    }
}

void Renderer::Clear(const Color& color) {
    Color c = m_translucent ? color.Premultiplied() : color;
    m_d2dContext->Clear(c.ToD2D());
}

ID2D1SolidColorBrush* Renderer::GetBrush(const Color& color) {
    m_brush->SetColor(color.ToD2D());
    return m_brush.Get();
}

void Renderer::DrawBitmap(
    ID2D1Bitmap1* bitmap, const RectF& destRect,
    float opacity, BitmapInterpolation interpolation,
    const RectF* srcRect)
{
    if (!bitmap) return;

    D2D1_INTERPOLATION_MODE mode;
    switch (interpolation) {
        case BitmapInterpolation::NearestNeighbor:
            mode = D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR; break;
        case BitmapInterpolation::HighQuality:
            mode = D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC; break;
        default:
            mode = D2D1_INTERPOLATION_MODE_LINEAR; break;
    }

    D2D1_RECT_F sRect;
    const D2D1_RECT_F* pSRect = nullptr;
    if (srcRect) {
        sRect = srcRect->ToD2D();
        pSRect = &sRect;
    }

    m_d2dContext->DrawBitmap(
        bitmap,
        destRect.ToD2D(),
        opacity,
        mode,
        pSRect
    );
}

void Renderer::FillRect(const RectF& rect, const Color& color) {
    m_d2dContext->FillRectangle(rect.ToD2D(), GetBrush(color));
}

void Renderer::DrawRect(const RectF& rect, const Color& color, float strokeWidth) {
    m_d2dContext->DrawRectangle(rect.ToD2D(), GetBrush(color), strokeWidth);
}

void Renderer::FillRoundedRect(const RectF& rect, float rx, float ry, const Color& color) {
    D2D1_ROUNDED_RECT rr = { rect.ToD2D(), rx, ry };
    m_d2dContext->FillRoundedRectangle(rr, GetBrush(color));
}

void Renderer::DrawRoundedRect(const RectF& rect, float rx, float ry,
                                const Color& color, float strokeWidth) {
    D2D1_ROUNDED_RECT rr = { rect.ToD2D(), rx, ry };
    m_d2dContext->DrawRoundedRectangle(rr, GetBrush(color), strokeWidth);
}

void Renderer::FillEllipse(const PointF& center, float rx, float ry, const Color& color) {
    D2D1_ELLIPSE e = { center.ToD2D(), rx, ry };
    m_d2dContext->FillEllipse(e, GetBrush(color));
}

void Renderer::DrawEllipse(const PointF& center, float rx, float ry,
                            const Color& color, float strokeWidth) {
    D2D1_ELLIPSE e = { center.ToD2D(), rx, ry };
    m_d2dContext->DrawEllipse(e, GetBrush(color), strokeWidth);
}

void Renderer::DrawLine(const PointF& p1, const PointF& p2,
                         const Color& color, float strokeWidth) {
    m_d2dContext->DrawLine(p1.ToD2D(), p2.ToD2D(), GetBrush(color), strokeWidth);
}

void Renderer::DrawText(
    std::wstring_view text, const RectF& layoutRect, const Color& color,
    float fontSize, TextAlign hAlign, TextVAlign vAlign, const wchar_t* fontFamily)
{
    auto textFormat = CreateTextFormat(fontFamily, fontSize);
    if (!textFormat) return;

    switch (hAlign) {
        case TextAlign::Left:   textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING); break;
        case TextAlign::Center: textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER); break;
        case TextAlign::Right:  textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING); break;
    }
    switch (vAlign) {
        case TextVAlign::Top:    textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR); break;
        case TextVAlign::Center: textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER); break;
        case TextVAlign::Bottom: textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR); break;
    }

    m_d2dContext->DrawText(
        text.data(),
        static_cast<UINT32>(text.size()),
        textFormat.Get(),
        layoutRect.ToD2D(),
        GetBrush(color)
    );
}

PointF Renderer::MeasureText(
    std::wstring_view text, float fontSize,
    const wchar_t* fontFamily)
{
    auto textFormat = CreateTextFormat(fontFamily, fontSize);
    if (!textFormat) return { 0.0f, 0.0f };

    std::wstring measureText(text);
    for (auto& c : measureText) {
        if (c == L' ') c = L'\u00A0';
    }

    ComPtr<IDWriteTextLayout> textLayout;
    HRESULT hr = m_dwriteFactory->CreateTextLayout(
        measureText.data(),
        static_cast<UINT32>(measureText.size()),
        textFormat.Get(),
        10000.0f, // Max width
        1000.0f,  // Max height
        &textLayout
    );
    if (FAILED(hr)) return { 0.0f, 0.0f };

    DWRITE_TEXT_METRICS metrics;
    textLayout->GetMetrics(&metrics);
    return { metrics.width, metrics.height };
}

void Renderer::PushTransform(const D2D1_MATRIX_3X2_F& transform) {
    auto combined = transform * m_transformStack.back();
    m_transformStack.push_back(combined);
    m_d2dContext->SetTransform(combined);
}

void Renderer::PopTransform() {
    if (m_transformStack.size() > 1) {
        m_transformStack.pop_back();
        m_d2dContext->SetTransform(m_transformStack.back());
    }
}

void Renderer::PushTranslation(float x, float y) {
    PushTransform(D2D1::Matrix3x2F::Translation(x, y));
}

void Renderer::PushScale(float sx, float sy, const PointF& center) {
    PushTransform(D2D1::Matrix3x2F::Scale(sx, sy, center.ToD2D()));
}

void Renderer::PushRotation(float angleDegrees, const PointF& center) {
    PushTransform(D2D1::Matrix3x2F::Rotation(angleDegrees, center.ToD2D()));
}

void Renderer::ResetTransform() {
    m_transformStack.clear();
    m_transformStack.push_back(D2D1::Matrix3x2F::Identity());
    m_d2dContext->SetTransform(D2D1::Matrix3x2F::Identity());
}

void Renderer::PushClip(const RectF& rect) {
    m_d2dContext->PushAxisAlignedClip(rect.ToD2D(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    ++m_clipDepth;
}

void Renderer::PopClip() {
    if (m_clipDepth > 0) {
        m_d2dContext->PopAxisAlignedClip();
        --m_clipDepth;
    }
}

ComPtr<ID2D1Bitmap1> Renderer::CreateBitmapFromWIC(IWICBitmapSource* wicBitmap) {
    if (!wicBitmap) return nullptr;

    ComPtr<IWICImagingFactory> wicFactory;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&wicFactory));
    if (FAILED(hr)) return nullptr;

    ComPtr<IWICFormatConverter> converter;
    hr = wicFactory->CreateFormatConverter(&converter);
    if (FAILED(hr)) return nullptr;

    hr = converter->Initialize(
        wicBitmap,
        GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0,
        WICBitmapPaletteTypeMedianCut
    );
    if (FAILED(hr)) return nullptr;

    ComPtr<ID2D1Bitmap1> bitmap;
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );

    hr = m_d2dContext->CreateBitmapFromWicBitmap(converter.Get(), &props, &bitmap);
    if (FAILED(hr)) {
        LogError(std::format(L"CreateBitmapFromWicBitmap failed: 0x{:08X}",
            static_cast<unsigned>(hr)));
        return nullptr;
    }

    return bitmap;
}

ComPtr<ID2D1Bitmap1> Renderer::CreateBitmapFromMemory(
    const uint8_t* data, uint32_t width, uint32_t height, uint32_t stride)
{
    if (!data || width == 0 || height == 0) return nullptr;

    if (stride == 0) stride = width * 4;

    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );

    ComPtr<ID2D1Bitmap1> bitmap;
    HRESULT hr = m_d2dContext->CreateBitmap(
        D2D1::SizeU(width, height),
        data, stride,
        props, &bitmap
    );

    if (FAILED(hr)) {
        LogError(std::format(L"CreateBitmap failed: 0x{:08X}", static_cast<unsigned>(hr)));
        return nullptr;
    }

    return bitmap;
}

ComPtr<IDWriteTextFormat> Renderer::CreateTextFormat(
    const wchar_t* fontFamily, float fontSize,
    DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STYLE style)
{
    ComPtr<IDWriteTextFormat> format;
    HRESULT hr = m_dwriteFactory->CreateTextFormat(
        fontFamily,
        nullptr,
        weight,
        style,
        DWRITE_FONT_STRETCH_NORMAL,
        fontSize,
        L"en-us",
        &format
    );

    if (FAILED(hr)) {
        LogError(std::format(L"CreateTextFormat failed for '{}': 0x{:08X}",
            fontFamily, static_cast<unsigned>(hr)));
        return nullptr;
    }

    return format;
}

SizeU Renderer::GetTargetSize() const {
    return { m_width, m_height };
}

void Renderer::DiscardDeviceResources() {
    m_renderTarget.Reset();
    m_brush.Reset();
    m_d2dContext.Reset();
    m_d2dDevice.Reset();
    m_d2dFactory.Reset();
    m_dwriteFactory.Reset();
    m_swapChain.Reset();
    m_dcompVisual.Reset();
    m_dcompTarget.Reset();
    m_dcompDevice.Reset();
    m_d3dContext.Reset();
    m_d3dDevice.Reset();
}

bool Renderer::HandleDeviceLost(HWND hwnd, bool translucent) {
    LogWarn(L"GPU device lost — recreating all resources");
    DiscardDeviceResources();
    return CreateDeviceResources(hwnd, translucent);
}

} // namespace kv
