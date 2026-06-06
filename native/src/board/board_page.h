// ============================================================================
// Class: BoardPage
// Description: BoardPage coordinates the virtual infinite workspace UI page in [IMAGE_VIEWER_APP_NAME].
// Responsibility: Act as main page handler receiving mouse and keyboard inputs, forwarding events to selection, text editor, canvas, and file loaders.
// ============================================================================
#pragma once

#include "common/types.h"
#include "board_item.h"
#include "board_canvas.h"
#include "board_selection_manager.h"
#include "board_text_editor.h"

namespace kv {

class App; // Forward declaration

class BoardPage {
public:
    friend class App;

    BoardPage() = default;
    ~BoardPage();

    BoardPage(const BoardPage&) = delete;
    BoardPage& operator=(const BoardPage&) = delete;

    /// Initialize the page with parent App context
    void Initialize(App* app);

    /// Update animations, timers and blinking cursor states
    void Update(double dt);

    /// Draw the 2D canvas, board items, selection boxes, grids, and edit overlays
    void Draw(Renderer& renderer);

    // ── Input Event Routing ────────────────────────────────────────────────
    bool OnMouseMove(float x, float y, KeyModifiers mods);
    bool OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods);
    bool OnMouseUp(MouseButton btn, float x, float y, KeyModifiers mods);
    bool OnMouseWheel(float delta, float x, float y, KeyModifiers mods);
    bool OnMouseDoubleClick(MouseButton btn, float x, float y, KeyModifiers mods);
    bool OnKeyDown(uint32_t vk, KeyModifiers mods);
    void OnChar(wchar_t ch);
    void OnFileDrop(const std::wstring& path, float screenX, float screenY);
    void OnEnterPage();

    // Coordinate conversions
    PointF ScreenToWorld(const PointF& screenPt) const;
    PointF WorldToScreen(const PointF& worldPt) const;
    RectF WorldToScreen(const RectF& worldRect) const;

    // Save/Load layouts
    bool SaveBoard(const std::wstring& filePath);
    bool LoadBoard(const std::wstring& filePath);

    static constexpr size_t MAX_BOARD_TEXTURES = 150; // Max loaded textures in GPU VRAM
    void ListCurrentDirectoryImages();
    void EvictFarBoardTextures();

private:
    void ResetView();
    void LoadItemTexture(BoardItem& item);
    void DeleteSelectedItems();
    void DeleteItemsByIndices(const std::vector<int>& indicesToDelete);
    void AddNoteAt(const PointF& worldPos, const std::wstring& text = L"Note");
    void AddNodeAt(const PointF& worldPos, const std::wstring& label = L"Node");
    void StartLinking(int fromIdx);
    float DistanceToLineSegment(const PointF& p, const PointF& a, const PointF& b) const;

    // Helpers
    PointF FindSpiralPosition(const PointF& center, float width, float height, float margin, const std::vector<BoardItem>& extraItems);
    RectF GetSearchModalBounds() const;
    void UpdateFilteredSuggestions();
    void SelectSuggestion(const std::wstring& suggestion);
    void DrawSearchModal(Renderer& renderer);
    void DrawScanProgressBar(Renderer& renderer);
    void DrawToastNotification(Renderer& renderer);
    void LoadSuggestionsFromDirectory(const std::filesystem::path& activeDir);
    void StartDirectoryTagScan(const std::filesystem::path& activeDir);
    void PerformTagQueryAndInsert();
    std::filesystem::path GetActiveDirectory() const;

    App* m_app = nullptr;

    // Subsystem components
    BoardCanvas m_canvas;
    BoardSelectionManager m_selection;
    BoardTextEditor m_editor;

    // Items list
    std::vector<BoardItem> m_items;

    // Dynamic Connection/Link-creation mode
    bool m_linkingMode = false;
    int m_linkingFromIdx = -1;
    PointF m_linkingCurrentMousePos = {}; // Screen coordinates

    // Search Modal State
    bool m_searchVisible = false;
    std::wstring m_searchText;
    std::vector<std::wstring> m_suggestions;
    std::vector<std::wstring> m_filteredSuggestions;
    int m_hoveredSuggestionIdx = -1;
    int m_hoveredSearchButton = 0; // 0 = None, 1 = Search, 2 = Close

    // Tag Scanning / Indexing State (CTRL+L)
    bool m_scanningTags = false;
    std::atomic<size_t> m_scanCurrent{ 0 };
    size_t m_scanTotal = 0;
    std::wstring m_scanDir;
    std::thread m_scanThread;

    // Toast notification
    std::wstring m_toastMessage;
    double m_toastTimeRemaining = 0.0;
};

} // namespace kv
