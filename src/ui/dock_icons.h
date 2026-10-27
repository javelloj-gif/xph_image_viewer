#pragma once
#include "rendering/renderer.h"
#include <array>

namespace kv {
// UI-thread vector artwork traced from the owner's Outline reference: CPU geometries borrow no image/loader state.
// Resources belong to the current D2D factory and are rebuilt only when that factory changes.
class DockIcons {
    friend class AsyncPreviewTests;
public:
    // UI/render thread: draws a centered 32-unit reference glyph in the supplied client rectangle; restores transform.
    // Returns false when the context/factory cannot provide resources; the next draw may retry.
    bool Draw(Renderer& renderer, int index, const RectF& bounds, const Color& color);
private:
    // UI: creates seven outline/fill pairs and a round-cap/round-join style, replacing stale factory resources.
    bool EnsureResources(ID2D1Factory* factory);
    ComPtr<ID2D1Factory> m_factory;
    ComPtr<ID2D1StrokeStyle> m_stroke;
    std::array<ComPtr<ID2D1PathGeometry>, 7> m_outlines;
    std::array<ComPtr<ID2D1PathGeometry>, 7> m_fills;
};
} // namespace kv
