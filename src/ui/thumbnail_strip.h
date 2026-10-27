// ============================================================================
// Class: ThumbnailStrip
// Description: ThumbnailStrip manages the reorderable row of image previews displayed at the bottom of the screen in Fotassa.
// Responsibility: Draw selection markers, re-ordering layout guides, and handle thumbnail click events.
// ============================================================================
#pragma once

#include "common/types.h"
#include "common/image_request.h"
#include "rendering/renderer.h"
#include "loader/image_loader.h"
#include <utility>

namespace kv {

class App; // Forward declaration

// UI-thread thumbnail cache entry; pending request identity travels with retained/moved entries.
struct ThumbnailInfo {
    std::wstring         path;
    ComPtr<ID2D1Bitmap1> texture;
    bool                 isLoading = false;
    bool                 loadFailed = false;
    ImageRequestToken    pendingRequest;
    ImageLoadLifetime loadLifetime; // UI-only lease; removal cancels CPU and delivery without owning callbacks.
};

class ThumbnailStrip {
    friend class StartupCacheTests; // Exercises production async generations/cache with isolated data.
    friend class App;
    friend class AsyncPreviewTests; // Exercises real requests and callbacks without visible UI.
public:
    ThumbnailStrip() = default;
    ~ThumbnailStrip() = default;

    // Non-copyable
    ThumbnailStrip(const ThumbnailStrip&) = delete;
    ThumbnailStrip& operator=(const ThumbnailStrip&) = delete;

    /// Initialize with the parent App context
    void Initialize(App* app);

    // UI: advances animations/autohide, invalidates visible transitions and requests thumbnails in range.
    void Update(double dt);

    /// UI thread: draws current-image emphasis, selection checks and reflected previews in the shared clip.
    void Draw(Renderer& renderer);

    /// UI thread: rebuilds via a temporary occurrence index; retains each old entry/request once and resets selection.
    void SetFileList(const std::vector<std::wstring>& fileList);
    // UI: cancels requests on page exit while retaining ready textures and allowing later retries.
    void CancelPendingLoads();

    /// UI thread: refreshes presented identity and centers navigation; preserves selection during gestures.
    void SyncSelection(int currentIndex);

    /// Returns true if the cursor is within the panel boundaries
    bool IsMouseOver(float x, float y) const;

    // UI-only client-pixel geometry, converted locally from DIP using the owning window DPI.
    RectF GetBounds() const;
    // UI-only: client center of the presented occurrence, or client center if none is present.
    float GetActiveCenter(float windowWidth) const;

    // ── Input Handling ──────────────────────────────────────────────────────
    // UI thread: route pointer gestures against the same visible geometry used for rendering.
    bool HandleMouseMove(float x, float y, KeyModifiers mods);
    bool HandleMouseDown(MouseButton btn, float x, float y, KeyModifiers mods);
    bool HandleMouseUp(MouseButton btn, float x, float y, KeyModifiers mods);
    bool HandleMouseWheel(float delta, float x, float y, KeyModifiers mods);
    bool HandleMouseDoubleClick(MouseButton btn, float x, float y, KeyModifiers mods);
    void HandleMouseLeave();
    // UI: abandons an unfinished drag/selection without reordering or discarding cached textures.
    void CancelPointerGesture();
    // UI thread: pages the strip within its geometry-derived scroll limits.
    bool HandleKeyDown(uint32_t vk, KeyModifiers mods);

    /// Returns true if scroll or opacity transitions are running
    bool HasActiveAnimations() const;

    /// Get current opacity of the thumbnail strip
    float GetOpacity() const { return m_opacity; }

    // ── Layout Constants ────────────────────────────────────────────────────
    static constexpr float THUMBNAIL_WIDTH = 44.0f;
    static constexpr float ACTIVE_SCALE = 1.3f;
    static constexpr float THUMBNAIL_GAP = 3.0f; // DIP; shared draw/hit/scroll geometry.

private:
    // ── Private Helpers ────────────────────────────────────────────────────
    // UI-only: common monotone geometry for drawing, visibility, selection and pointer input.
    RectF GetThumbnailRect(int index, float windowWidth, float windowHeight) const;
    RectF GetThumbnailRect(int index, float windowWidth, float windowHeight, float scrollX) const;
    // UI-only: selects one occurrence of the successfully applied original, never a pending path.
    void RefreshActiveIndex(int preferredIndex);
    // UI-only: common centers/scroll limits and binary hit/insertion lookup; no resource ownership.
    float GetCenteredScroll(int index) const;
    std::pair<float, float> GetScrollLimits() const;
    int HitTest(float x, float y) const;
    int GetInsertionIndex(float x, float windowWidth) const;
    // UI-only DIP-to-client conversion; renderer and window inputs remain in their existing pixel space.
    float GetUiScale() const;
    // Returns a half-open candidate interval with inclusive edge visibility; invalid geometry falls back to all items.
    std::pair<size_t, size_t> GetVisibleRange(float windowWidth) const;
    // Same inclusive geometry for a destination offset, without mutating animated UI state.
    std::pair<size_t, size_t> GetVisibleRange(float windowWidth, float scrollX) const;
    // UI-only preferred base size converted from DIP; invalid preferences use the default.
    float GetThumbnailWidth() const;
    void DrawMirror(Renderer& renderer, ID2D1Bitmap1* bitmap, const RectF& rect, float opacity);
    // Requests a thumbnail with unique identity and a pre-upload UI validity check; completion still checks identity.
    // Destination requests precede intermediate visible positions without changing the scroll animation.
    void LoadThumbnail(int index, bool destination = false);
    // UI-only read: matches token and exact path without consuming loading state or retaining an item address.
    bool IsThumbnailRequestCurrent(const ImageRequestToken& request) const;
    // Applies a completion once to the current token/path match; removed requests do nothing.
    void CompleteThumbnail(const ImageRequestToken& request, std::shared_ptr<ImageData> image);
    // UI thread: reorders selected occurrences, retaining presented identity and cached previews.
    void Rearrange(int insertIndex);
    void ShowContextMenu(float screenX, float screenY);

    App*                         m_app = nullptr;
    std::vector<ThumbnailInfo>   m_thumbnails;
    std::vector<bool>            m_selected; // parallel to m_thumbnails

    // ── Scroll & Opacity Transition State ──────────────────────────────────
    float m_layoutWidth = 0.0f; // Last pixel width used to recenter after preferred-size/DPI changes.
    float m_scrollX = 0.0f;
    float m_targetScrollX = 0.0f;
    // Diagnostic stability of the actual float position, including representable lerp plateaus.
    unsigned m_scrollStableFrames = 0;
    float m_opacity = 0.0f;
    float m_targetOpacity = 0.0f;

    // ── Drag & Selection Interaction State ─────────────────────────────────
    bool   m_draggingThumbnails = false;
    bool   m_groupSelecting = false;
    PointF m_selectionStart = {};
    RectF  m_selectionRect = {};
    PointF m_dragStartPos = {};
    bool   m_hasDragged = false;

    int    m_activeIndex = -1; // Presented occurrence; independent of hover and multi-selection.
    int    m_hoveredIndex = -1;
    int    m_insertIndex = -1; // -1 if not showing insertion cursor
    double m_mouseMoveTimer = 0.0;
};

} // namespace kv
