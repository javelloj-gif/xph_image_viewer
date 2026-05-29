// ============================================================================
// viewer_page.h — Viewer Page (Picasa clone) Component
// Krumassan Image Viewer — Native C++20
// ============================================================================
#pragma once

#include "types.h"
#include "renderer.h"
#include "image_loader.h"
#include "animation.h"

namespace kv {

class App; // Forward declaration

class ViewerPage {
public:
    friend class App;
    ViewerPage() = default;
    ~ViewerPage() = default;

    // Non-copyable
    ViewerPage(const ViewerPage&) = delete;
    ViewerPage& operator=(const ViewerPage&) = delete;

    /// Initialize the page with the parent App context
    void Initialize(App* app);

    /// Update any continuous animations or states
    void Update(double dt);

    /// Draw the viewer page contents (image, checkerboard, overlays, magnifier)
    void Draw(Renderer& renderer);

    // ── Input Event Routing ────────────────────────────────────────────────
    void OnMouseMove(float x, float y, KeyModifiers mods);
    void OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods);
    void OnMouseUp(MouseButton btn, float x, float y, KeyModifiers mods);
    void OnMouseWheel(float delta, float x, float y, KeyModifiers mods);
    void OnMouseDoubleClick(MouseButton btn, float x, float y, KeyModifiers mods);
    void OnKeyDown(uint32_t vk, KeyModifiers mods);

    // ── View Control ───────────────────────────────────────────────────────
    void ResetView();
    void Toggle1To1();
    void ZoomToPoint(float factor, float screenX, float screenY);
    RectF CalculateImageRect() const;
    RectF GetImageTransformedRect() const;
    bool IsPointOverImage(float x, float y) const;

private:
    // ── Drawing Helpers ────────────────────────────────────────────────────
    void DrawImageFitted(Renderer& renderer, ID2D1Bitmap1* bitmap);
    void DrawCheckerboard(Renderer& renderer, const RectF& rect);
    void DrawCyberpunkCorners(Renderer& renderer, const RectF& rect);
    void DrawThirds(Renderer& renderer, const RectF& rect);
    void DrawOverlayInfo(Renderer& renderer);
    void DrawProgressBar(Renderer& renderer, const RectF& rect);
    void DrawRegionZoom(Renderer& renderer);

    // ── Region Zoom (Ctrl+Drag Magnifier) ──────────────────────────────────
    void StartRegionZoom(float startX, float startY);
    void UpdateRegionZoom(float curX, float curY);
    void FinishRegionZoom(float endX, float endY);
    void CancelRegionZoom();

    // ── Pointer to Parent Application ──────────────────────────────────────
    App* m_app = nullptr;

    // ── View Transform State ───────────────────────────────────────────────
    float  m_zoom = 1.0f;
    PointF m_pan = {};          // current translation offset
    float  m_rotation = 0.0f;   // degrees

    // ── Smooth Zoom Target State ───────────────────────────────────────────
    float  m_targetZoom = 1.0f;
    PointF m_zoomAnchor = {};   // screen space coordinate of zoom anchor

    // ── Drag & Pan Interaction State ───────────────────────────────────────
    bool   m_dragging = false;
    PointF m_dragStart = {};
    PointF m_panStart = {};

    // ── Region Zoom State ──────────────────────────────────────────────────
    bool   m_regionZoomActive = false;
    PointF m_regionStartPoint = {};
    PointF m_regionEndPoint = {};
    bool   m_regionZoomDefined = false;
    
    // Animating region zoom properties
    float  m_origZoom = 1.0f;
    PointF m_origPan = {};
    PointF m_targetPan = {};
    float  m_regionStageFactor = 0.0f; // 0.0 to 1.0 spotlight animation progress
    float  m_zoomLevelText = 1.0f;     // calculated zoom target for text overlay
    RectF  m_inputRect = {};           // selected screen area
    RectF  m_projectedRect = {};       // selected area projected to full screen
    RectF  m_inputRectAnimated = {};   // animated spotlight bounding box

    // ── Cached Graphics Assets ─────────────────────────────────────────────
    ComPtr<ID2D1Bitmap1> m_checkerboardBitmap;

    // ── Page View Overlay Settings ─────────────────────────────────────────
    bool   m_showCyberpunk = true;
    bool   m_showThirds = false;
    bool   m_showMetadata = true;
    bool   m_invertImage = false;
    bool   m_pixelatedZoom = false; // smooth (Linear/HighQuality) at all times if false
};

} // namespace kv
