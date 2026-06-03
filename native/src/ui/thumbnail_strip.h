// ============================================================================
// Class: ThumbnailStrip
// Description: ThumbnailStrip manages the reorderable row of image previews displayed at the bottom of the screen in [IMAGE_VIEWER_APP_NAME].
// Responsibility: Draw selection markers, re-ordering layout guides, and handle thumbnail click events.
// ============================================================================
#pragma once

#include "common/types.h"
#include "rendering/renderer.h"
#include "loader/image_loader.h"

namespace kv {

class App; // Forward declaration

struct ThumbnailInfo {
    std::wstring         path;
    ComPtr<ID2D1Bitmap1> texture;
    bool                 isLoading = false;
    bool                 loadFailed = false;
};

class ThumbnailStrip {
public:
    ThumbnailStrip() = default;
    ~ThumbnailStrip() = default;

    // Non-copyable
    ThumbnailStrip(const ThumbnailStrip&) = delete;
    ThumbnailStrip& operator=(const ThumbnailStrip&) = delete;

    /// Initialize with the parent App context
    void Initialize(App* app);

    /// Update animations and check pre-fetching
    void Update(double dt);

    /// Draw the thumbnail strip
    void Draw(Renderer& renderer);

    /// Rebuild the thumbnail cache when the file list changes
    void SetFileList(const std::vector<std::wstring>& fileList);

    /// Sync the selection to the active index when not dragging/multi-selecting
    void SyncSelection(int currentIndex);

    /// Returns true if the cursor is within the panel boundaries
    bool IsMouseOver(float x, float y) const;

    // ── Input Handling ──────────────────────────────────────────────────────
    bool HandleMouseMove(float x, float y, KeyModifiers mods);
    bool HandleMouseDown(MouseButton btn, float x, float y, KeyModifiers mods);
    bool HandleMouseUp(MouseButton btn, float x, float y, KeyModifiers mods);
    bool HandleMouseWheel(float delta, float x, float y, KeyModifiers mods);
    bool HandleMouseDoubleClick(MouseButton btn, float x, float y, KeyModifiers mods);
    void HandleMouseLeave();

    /// Returns true if scroll or opacity transitions are running
    bool HasActiveAnimations() const;

    /// Get current opacity of the thumbnail strip
    float GetOpacity() const { return m_opacity; }

    // ── Layout Constants ────────────────────────────────────────────────────
    static constexpr float THUMBNAIL_WIDTH = 60.0f;
    static constexpr float AUGMENTED_THUBNAIL_INCREMENT = 24.0f;
    static constexpr float HEIGHT = 120.0f;

private:
    // ── Private Helpers ────────────────────────────────────────────────────
    RectF GetThumbnailRect(int index, float windowWidth, float windowHeight) const;
    float GetThumbnailWidth() const;
    void DrawMirror(Renderer& renderer, ID2D1Bitmap1* bitmap, const RectF& rect, float opacity);
    void LoadThumbnail(int index);
    void Rearrange(int insertIndex);
    void ShowContextMenu(float screenX, float screenY);

    App*                         m_app = nullptr;
    std::vector<ThumbnailInfo>   m_thumbnails;
    std::vector<bool>            m_selected; // parallel to m_thumbnails

    // ── Scroll & Opacity Transition State ──────────────────────────────────
    float m_scrollX = 0.0f;
    float m_targetScrollX = 0.0f;
    float m_opacity = 0.0f;
    float m_targetOpacity = 0.0f;

    // ── Drag & Selection Interaction State ─────────────────────────────────
    bool   m_draggingThumbnails = false;
    bool   m_groupSelecting = false;
    PointF m_selectionStart = {};
    RectF  m_selectionRect = {};
    PointF m_dragStartPos = {};
    bool   m_hasDragged = false;

    int    m_hoveredIndex = -1;
    int    m_insertIndex = -1; // -1 if not showing insertion cursor
    double m_mouseMoveTimer = 0.0;
};

} // namespace kv
