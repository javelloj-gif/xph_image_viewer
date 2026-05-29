// ============================================================================
// waterfall_page.h — Waterfall Page (Masonry layout) Component
// Krumassan Image Viewer — Native C++20
// ============================================================================
#pragma once

#include "types.h"
#include "renderer.h"
#include "image_loader.h"
#include "animation.h"
#include <vector>
#include <string>
#include <memory>

namespace kv {

class App; // Forward declaration

class WaterfallPage {
public:
    friend class App;

    WaterfallPage() = default;
    ~WaterfallPage() = default;

    WaterfallPage(const WaterfallPage&) = delete;
    WaterfallPage& operator=(const WaterfallPage&) = delete;

    /// Initialize the page with the parent App context
    void Initialize(App* app);

    /// Update animations and scroll offsets
    void Update(double dt);

    /// Draw the waterfall page layout (masonry previews grid, scrollbars, hovered item, modal viewer)
    void Draw(Renderer& renderer);

    // ── Input Event Routing ────────────────────────────────────────────────
    bool OnMouseMove(float x, float y, KeyModifiers mods);
    bool OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods);
    bool OnMouseUp(MouseButton btn, float x, float y, KeyModifiers mods);
    bool OnMouseWheel(float delta, float x, float y, KeyModifiers mods);
    bool OnMouseDoubleClick(MouseButton btn, float x, float y, KeyModifiers mods);
    bool OnKeyDown(uint32_t vk, KeyModifiers mods);

private:
    void CalculateLayout();
    void DrawPreviewsGrid(Renderer& renderer, const RectF& rect);
    void DrawScrollbars(Renderer& renderer, const RectF& rect);
    void DrawScrollbar(Renderer& renderer, const RectF& trackRect, float contentHeight, float viewHeight, float scrollOffset, float opacity);
    void LoadPreviewTexture(int folderIdx, int previewIdx);
    void DrawCheckerboard(Renderer& renderer, const RectF& rect);
    
    // Spotlight/Enlarged rect helper
    RectF EnlargeActiveItemRect(const RectF& rect) const;

    App* m_app = nullptr;

    // Grid scroll offset
    float m_scrollOffset = 0.0f;
    float m_targetScrollOffset = 0.0f;

    // Masonry column parameters
    int m_columnsNumber = 0; // 0 = auto-calculate based on width, or positive value
    float m_gridSpacing = 15.0f;
    float m_cornerRadius = 20.0f;

    // Bounding boxes computed in CalculateLayout
    std::vector<RectF> m_previewRects;

    // Hover state
    int m_hoveredPreviewIdx = -1;
    bool m_waterfallBlockActiveItem = false;
    double m_unblockTimer = 0.0;

    // Hovered animated file caching (same as library page)
    std::wstring m_hoveredOriginalPath;
    std::shared_ptr<AnimatedImageData> m_hoveredOriginalAnim;
    FrameSequencer m_hoveredAnimSequencer;
    bool m_hoveredOriginalLoading = false;

    // Modal Viewer mode
    bool m_viewerModal = false;

    // Scrollbar capture state
    bool m_scrollbarCaptured = false;
    float m_dragStartY = 0.0f;
    float m_dragStartScrollOffset = 0.0f;

    // Background checkerboard pattern
    ComPtr<ID2D1Bitmap1> m_checkerboardBitmap;
};

} // namespace kv
