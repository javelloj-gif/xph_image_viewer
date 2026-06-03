// ============================================================================
// Class: BoardCanvas
// Description: BoardCanvas controls the viewport transformations, coordinate projections, and background grid patterns for the board interface of [IMAGE_VIEWER_APP_NAME].
// Responsibility: Keep zoom and pan states, convert screen/world coordinate spaces, and draw checkerboard or alignment grids.
// ============================================================================
#pragma once

#include "common/types.h"
#include "rendering/renderer.h"

namespace kv {

class BoardCanvas {
public:
    BoardCanvas() = default;
    ~BoardCanvas() = default;

    void ResetView();

    PointF ScreenToWorld(const PointF& screenPt) const;
    PointF WorldToScreen(const PointF& worldPt) const;
    RectF WorldToScreen(const RectF& worldRect) const;

    void DrawCheckerboard(Renderer& renderer, const RectF& rect);
    void DrawGrid(Renderer& renderer, const SizeF& size);

    // Canvas transformations
    float m_zoom = 1.0f;
    PointF m_pan = {0.0f, 0.0f};

    // Camera panning
    bool m_cameraPanning = false;
    PointF m_panStart = {};
    PointF m_mouseDragStart = {};

private:
    ComPtr<ID2D1Bitmap1> m_checkerboardBitmap;
};

} // namespace kv
