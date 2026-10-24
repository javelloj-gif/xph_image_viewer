// ============================================================================
// Class: WaterfallPage
// Description: WaterfallPage implements the virtual scrolling masonry grid layout for browsing folder contents in [IMAGE_VIEWER_APP_NAME].
// Responsibility: Virtual scroll column coordinate maps and modal image viewers.
// ============================================================================
#pragma once

#include "common/types.h"
#include "rendering/renderer.h"
#include "loader/image_loader.h"
#include "rendering/animation_controller.h"
#include "loader/frame_sequencer.h"
#include <vector>
#include <string>
#include <memory>

namespace kv {

class App; // Forward declaration

class WaterfallPage {
    friend class AsyncPreviewTests; // Exercises shared preview completion after list mutations.
public:
    friend class App;
    friend class ViewerPage;

    WaterfallPage() = default;
    ~WaterfallPage() = default;

    WaterfallPage(const WaterfallPage&) = delete;
    WaterfallPage& operator=(const WaterfallPage&) = delete;

    /// Initialize the page with the parent App context
    void Initialize(App* app);

    /// Update any continuous animations or states
    void Update(double dt);

    /// Draw the waterfall page layout (masonry previews grid, scrollbars, hovered item, modal viewer)
    void Draw(Renderer& renderer);

    // ── Input Event Routing ────────────────────────────────────────────────
    // Routes modal/scroll/hover on UI; replacing hover cancels the preceding animation lease.
    bool OnMouseMove(float x, float y, KeyModifiers mods);
    bool OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods);
    bool OnMouseUp(MouseButton btn, float x, float y, KeyModifiers mods);
    bool OnMouseWheel(float delta, float x, float y, KeyModifiers mods);
    bool OnMouseDoubleClick(MouseButton btn, float x, float y, KeyModifiers mods);
    bool OnKeyDown(uint32_t vk, KeyModifiers mods);

private:
    // UI thread: reuses base rects until model/selection/width/count or column/spacing parameters change.
    void CalculateLayout();
    // UI: draws/requests visible previews and reports readiness after two stable geometry frames.
    void DrawPreviewsGrid(Renderer& renderer, const RectF& rect);
    void DrawScrollbars(Renderer& renderer, const RectF& rect);
    void DrawScrollbar(Renderer& renderer, const RectF& trackRect, float contentHeight, float viewHeight, float scrollOffset, float opacity);
    // Requests shared previews; UI checks token/path before upload and again at completion, never captured indices.
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
    // Fixed-size cache key only; the existing rect vector remains the sole geometry storage.
    bool m_layoutValid = false;
    bool m_readyLastFrame = false;
    uint64_t m_readyRevision = 0;
    uint64_t m_layoutRevision = 0;
    int m_layoutFolder = -1;
    float m_layoutWidth = 0.0f;
    size_t m_layoutCount = 0;
    int m_layoutColumns = 0;
    float m_layoutSpacing = 0.0f;

    // Hover state
    int m_hoveredPreviewIdx = -1;
    bool m_waterfallBlockActiveItem = false;
    double m_unblockTimer = 0.0;

    // Hovered animated file caching for the Waterfall grid
    std::wstring m_hoveredOriginalPath;
    ImageLoadLifetime m_hoverLoadLifetime; // Changed on hover/folder/page exit; loader holds only a weak lease.
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
