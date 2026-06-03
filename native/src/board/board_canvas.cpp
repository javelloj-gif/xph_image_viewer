// ============================================================================
// Class: BoardCanvas
// Description: BoardCanvas controls the viewport transformations, coordinate projections, and background grid patterns for the board interface of [IMAGE_VIEWER_APP_NAME].
// Responsibility: Keep zoom and pan states, convert screen/world coordinate spaces, and draw checkerboard or alignment grids.
// ============================================================================
#include "common/pch.h"
#include "board_canvas.h"
#include <algorithm>
#include <vector>
#include <cmath>

namespace kv {

void BoardCanvas::ResetView() {
    m_zoom = 1.0f;
    m_pan = { 0.0f, 0.0f };
    m_cameraPanning = false;
}

PointF BoardCanvas::ScreenToWorld(const PointF& screenPt) const {
    PointF worldPt;
    worldPt.x = (screenPt.x - m_pan.x) / m_zoom;
    worldPt.y = (screenPt.y - m_pan.y) / m_zoom;
    return worldPt;
}

PointF BoardCanvas::WorldToScreen(const PointF& worldPt) const {
    PointF screenPt;
    screenPt.x = worldPt.x * m_zoom + m_pan.x;
    screenPt.y = worldPt.y * m_zoom + m_pan.y;
    return screenPt;
}

RectF BoardCanvas::WorldToScreen(const RectF& worldRect) const {
    PointF tl = WorldToScreen(worldRect.TopLeft());
    PointF br = WorldToScreen(worldRect.BottomRight());
    return RectF(tl.x, tl.y, br.x, br.y);
}

void BoardCanvas::DrawCheckerboard(Renderer& renderer, const RectF& rect) {
    if (!m_checkerboardBitmap) {
        constexpr int size = 20;
        constexpr int total = size * 2;
        std::vector<uint8_t> pixels(total * total * 4);

        const uint8_t lightGray = 30; // Dark cyberpunk colors
        const uint8_t darkGray  = 15;

        for (int y = 0; y < total; ++y) {
            for (int x = 0; x < total; ++x) {
                int idx = (y * total + x) * 4;
                bool isLight = ((x / size) + (y / size)) % 2 == 0;
                uint8_t c = isLight ? lightGray : darkGray;
                pixels[idx + 0] = c;
                pixels[idx + 1] = c;
                pixels[idx + 2] = c;
                pixels[idx + 3] = 255;
            }
        }

        m_checkerboardBitmap = renderer.CreateBitmapFromMemory(pixels.data(), total, total, total * 4);
    }

    if (!m_checkerboardBitmap) return;

    renderer.PushClip(rect);
    float tileW = 40.0f, tileH = 40.0f;
    for (float y = rect.top; y < rect.bottom; y += tileH) {
        for (float x = rect.left; x < rect.right; x += tileW) {
            RectF tile = RectF::FromXYWH(x, y, tileW, tileH);
            renderer.DrawBitmap(m_checkerboardBitmap.Get(), tile, 1.0f, BitmapInterpolation::NearestNeighbor);
        }
    }
    renderer.PopClip();
}

void BoardCanvas::DrawGrid(Renderer& renderer, const SizeF& size) {
    Color gridColor = Color::FromRGBA(255, 255, 255, 10);
    float gridSize = 100.0f;
    PointF minWorld = ScreenToWorld({ 0, 0 });
    PointF maxWorld = ScreenToWorld({ size.width, size.height });

    float startX = std::floor(minWorld.x / gridSize) * gridSize;
    float endX = std::ceil(maxWorld.x / gridSize) * gridSize;
    float startY = std::floor(minWorld.y / gridSize) * gridSize;
    float endY = std::ceil(maxWorld.y / gridSize) * gridSize;

    for (float x = startX; x <= endX; x += gridSize) {
        renderer.DrawLine({ x, minWorld.y }, { x, maxWorld.y }, gridColor, 1.0f / m_zoom);
    }
    for (float y = startY; y <= endY; y += gridSize) {
        renderer.DrawLine({ minWorld.x, y }, { maxWorld.x, y }, gridColor, 1.0f / m_zoom);
    }
}

} // namespace kv
