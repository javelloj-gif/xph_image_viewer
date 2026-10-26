// ============================================================================
// Class: ViewerPage
// Description: ViewerPage implements the single-image display screen with advanced zoom, pan, and visual grids for [IMAGE_VIEWER_APP_NAME].
// Responsibility: Handle fit-to-screen transforms, thirds divisions, and Ctrl+drag spotlights.
// ============================================================================
#pragma once

#include "common/types.h"
#include "rendering/renderer.h"
#include "loader/image_loader.h"
#include "rendering/animation_controller.h"

namespace kv {

class App; // Forward declaration

class ViewerPage {
    friend class AsyncPreviewTests; // Checks overlay pixels without a visible window or timing changes.
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
    // UI thread: toggles fullscreen on left double click after parent controls have consumed input.
    void OnMouseDoubleClick(MouseButton btn, float x, float y, KeyModifiers mods);
    // UI thread: handles viewer transforms, overlays, animation and navigation keys.
    bool OnKeyDown(uint32_t vk, KeyModifiers mods);

    // ── View Control ───────────────────────────────────────────────────────
    void ResetView();
    void Toggle1To1();
    void ZoomToPoint(float factor, float screenX, float screenY);
    RectF CalculateImageRect() const;
    RectF GetImageTransformedRect() const;
    bool IsPointOverImage(float x, float y) const;
    bool HasActiveAnimations() const;
    float GetDisplayZoom() const;
    float GetDisplayZoomForValue(float zoomValue) const;

private:
    // ── Drawing Helpers ────────────────────────────────────────────────────
    // UI thread: draws transformed image, retained thirds, metadata and animation progress.
    void DrawImageFitted(Renderer& renderer, ID2D1Bitmap1* bitmap);
    void DrawCheckerboard(Renderer& renderer, const RectF& rect);
    void DrawThirds(Renderer& renderer, const RectF& rect);
    // UI thread: prepares/draws metadata only at positive fade opacity; does not advance animation state.
    void DrawOverlayInfo(Renderer& renderer);
    void DrawProgressBar(Renderer& renderer, const RectF& rect);
    void DrawRegionZoom(Renderer& renderer);

    // ── Region Zoom (Ctrl+Drag Magnifier) ──────────────────────────────────
    void StartRegionZoom(float startX, float startY);
    void UpdateRegionZoom(float curX, float curY);
    void FinishRegionZoom(float endX, float endY);
    void CancelRegionZoom();
    float GetDefaultZoom() const;

    // ── Context Menu (right-click in windowed mode) ────────────────────────
    // UI thread: presents native viewer actions and applies the chosen command.
    void ShowContextMenu();

    // ── Pointer to Parent Application ──────────────────────────────────────
    App* m_app = nullptr;

    // ── View Transform State ───────────────────────────────────────────────
    float  m_zoom = 1.0f;
    PointF m_pan = {};          // current translation offset
    float  m_rotation = 0.0f;   // degrees

    // ── Smooth Zoom Target State ───────────────────────────────────────────
    float  m_targetZoom = 1.0f;
    PointF m_zoomAnchor = {};   // screen space coordinate of zoom anchor

    // ── Left-Click Drag State ──────────────────────────────────────────────
    // En modo ventana: si el cursor se mueve más allá del umbral antes de soltar,
    // el gesto se convierte en un drag de título nativo (WM_NCLBUTTONDOWN/HTCAPTION).
    // En modo pantalla completa: el movimiento panea directamente m_pan.
    bool   m_leftButtonDown = false;
    PointF m_leftButtonStart = {};       // posición inicial del cursor (espacio cliente)
    double m_leftButtonDownTime = 0.0;   // sello de tiempo para diagnóstico
    float  m_leftButtonDownX = 0.0f;
    float  m_leftButtonDownY = 0.0f;
    PointF m_panAtDragStart = {};        // valor de m_pan al pulsar (para paneo fullscreen)

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
    bool   m_pixelatedZoom = false; // smooth (Linear/HighQuality) at all times if false

    // ── Zoom Text Overlay State ────────────────────────────────────────────
    float  m_zoomTextOpacity = 0.0f;
    double m_zoomTextTimer = 0.0;
    float  m_lastZoom = 1.0f;

    // ── Metadata Overlay State ─────────────────────────────────────────────
    float  m_metadataOpacity = 1.0f;
    double m_mouseInactivityTimer = 0.0;
};

} // namespace kv
