// ============================================================================
// library_page.h — Library Page (Folders List & Previews Grid) Component
// Krumassan Image Viewer — Native C++20
// ============================================================================
#pragma once

#include "types.h"
#include "renderer.h"
#include "image_loader.h"
#include "animation.h"
#include "library_data.h"

namespace kv {

class App; // Forward declaration

class LibraryPage {
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
    bool OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods);
    bool OnMouseUp(MouseButton btn, float x, float y, KeyModifiers mods);
    bool OnMouseWheel(float delta, float x, float y, KeyModifiers mods);
    bool OnMouseDoubleClick(MouseButton btn, float x, float y, KeyModifiers mods);
    void OnKeyDown(uint32_t vk, KeyModifiers mods);

    // ── Public Helpers ──────────────────────────────────────────────────────
    void AddFolder(const std::wstring& path);
    void SetCurrentFolder(int index);
    void SyncCurrentFolderWithApp();

private:
    void CalculateLayout();
    void DrawFoldersList(Renderer& renderer, const RectF& rect);
    void DrawPreviewsGrid(Renderer& renderer, const RectF& rect);
    void DrawScrollbar(Renderer& renderer, const RectF& trackRect, float contentHeight, float viewHeight, float scrollOffset, float opacity);
    void LoadPreviewTexture(int folderIdx, int previewIdx);
    void DrawCheckerboard(Renderer& renderer, const RectF& rect);

    App* m_app = nullptr;

    // Scroll offsets (in pixels)
    float m_foldersScrollY = 0.0f;
    float m_targetFoldersScrollY = 0.0f;

    // Selection/Hover state
    int m_hoveredFolderIdx = -1;
    int m_hoveredPreviewIdx = -1;

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
