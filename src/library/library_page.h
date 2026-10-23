// ============================================================================
// Class: LibraryPage
// Description: LibraryPage implements the panel view for browsing directory collections in [IMAGE_VIEWER_APP_NAME].
// Responsibility: Render left-hand scrollable folder entries and right-hand preview cards.
// ============================================================================
#pragma once

#include "common/types.h"
#include "rendering/renderer.h"
#include "loader/image_loader.h"
#include "rendering/animation_controller.h"
#include "core/folder_data_manager.h"
#include "loader/frame_sequencer.h"

namespace kv {

class App; // Forward declaration

class LibraryPage {
    friend class AsyncPreviewTests; // Exercises shared preview completion after list mutations.
public:
    friend class App;
    LibraryPage() = default;
    ~LibraryPage() = default;

    LibraryPage(const LibraryPage&) = delete;
    LibraryPage& operator=(const LibraryPage&) = delete;

    /// Initialize the page with the parent App context
    void Initialize(App* app);

    /// Update animations and scroll offsets
    void Update(double dt);

    /// Draw the library page layout (folders list, dividing line, previews grid, hovered original)
    void Draw(Renderer& renderer);

    // ── Input Event Routing ────────────────────────────────────────────────
    bool OnMouseMove(float x, float y, KeyModifiers mods);
    // Handles folder/preview selection and context actions, including removal from the shared service.
    bool OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods);
    bool OnMouseUp(MouseButton btn, float x, float y, KeyModifiers mods);
    bool OnMouseWheel(float delta, float x, float y, KeyModifiers mods);
    bool OnMouseDoubleClick(MouseButton btn, float x, float y, KeyModifiers mods);
    bool OnKeyDown(uint32_t vk, KeyModifiers mods);

    // ── Public Helpers ──────────────────────────────────────────────────────
    // UI thread: scans a new folder, appends previews and invalidates both layouts; ignores duplicates/empty scans.
    void AddFolder(const std::wstring& path);
    // Selects through App's shared folder service, loads the image and updates Library layout.
    void SetCurrentFolder(int index);
    void SyncCurrentFolderWithApp();

private:
    // UI thread: reuses existing preview rects until model revision, selection, count or target width changes.
    void CalculateLayout();
    void DrawFoldersList(Renderer& renderer, const RectF& rect);
    void DrawPreviewsGrid(Renderer& renderer, const RectF& rect);
    void DrawScrollbar(Renderer& renderer, const RectF& trackRect, float contentHeight, float viewHeight, float scrollOffset, float opacity);
    // Requests shared previews; UI checks token/path before upload and again at completion, never captured indices.
    void LoadPreviewTexture(int folderIdx, int previewIdx);
    void DrawCheckerboard(Renderer& renderer, const RectF& rect);

    App* m_app = nullptr;

    // Fixed-size last-layout key; rects remain in the shared previews, with no additional per-item cache.
    bool m_layoutValid = false;
    uint64_t m_layoutRevision = 0;
    int m_layoutFolder = -1;
    float m_layoutWidth = 0.0f;
    size_t m_layoutCount = 0;

    // Scroll offsets (in pixels)
    float m_foldersScrollY = 0.0f;
    float m_targetFoldersScrollY = 0.0f;

    // Selection/Hover state
    int m_hoveredFolderIdx = -1;
    int m_hoveredPreviewIdx = -1;
    bool m_hoveredCloseBtn = false;

    // Hovered full-size preview
    std::wstring m_hoveredOriginalPath;
    std::shared_ptr<ImageData> m_hoveredOriginalImage;
    std::shared_ptr<AnimatedImageData> m_hoveredOriginalAnim;
    FrameSequencer m_hoveredAnimSequencer;
    bool m_hoveredOriginalLoading = false;

    // Scrollbar state
    enum class ScrollCapture { None, Folders, Previews };
    ScrollCapture m_scrollCapture = ScrollCapture::None;
    float m_dragStartY = 0.0f;
    float m_dragStartScrollY = 0.0f;

    // Layout constants
    static constexpr float CENTER_OFFSET = 50.0f;
    static constexpr float FOLDER_ITEM_HEIGHT = 120.0f;
    static constexpr float PREVIEW_WIDTH = 180.0f;
    static constexpr float PREVIEW_GAP = 15.0f;
    static constexpr float SCROLLBAR_WIDTH = 10.0f;
    static constexpr float SCROLLBAR_MIN_HEIGHT = 40.0f;

    // Background checkerboard
    ComPtr<ID2D1Bitmap1> m_checkerboardBitmap;
};

} // namespace kv
