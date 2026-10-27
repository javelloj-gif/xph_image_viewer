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
#include <memory>
#include <vector>
#include <string>
#include <filesystem>

namespace kv {

class App; // Forward declaration
class BoardSearchManager;
class BoardTagScanner;
class BoardLayoutHelper;

class BoardPage {
public:
    friend class App;
    friend class AsyncPreviewTests; // Exercises producer join order before App dependency teardown.
    friend class BoardSearchManager;
    friend class BoardTagScanner;
    friend class BoardLayoutHelper;

    BoardPage();
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
    // UI: prepares the workspace once and incorporates the active image without reloading unsaved edits.
    void OnEnterPage();
    // UI: prepares the working board once, without navigating or overwriting later in-memory edits.
    void PrepareWorkspace();
    // UI: adds a supported path once at the camera center; queues texture loading, never writes the source or KBRD.
    bool AddImageToWorkspace(const std::wstring& path);

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
    // UI: loads one item's transient identity/lease; removal cancels it and duplicate paths complete independently.
    void LoadItemTexture(BoardItem& item);
    void DeleteSelectedItems();
    void DeleteItemsByIndices(const std::vector<int>& indicesToDelete);
    void AddNoteAt(const PointF& worldPos, const std::wstring& text = L"Note");
    void AddNodeAt(const PointF& worldPos, const std::wstring& label = L"Node");
    void StartLinking(int fromIdx);

    // Helpers
    void DrawToastNotification(Renderer& renderer);
    // Uses App's active files, then shared selected folder, then the working directory.
    std::filesystem::path GetActiveDirectory() const;

    App* m_app = nullptr;

    // Subsystem components
    BoardCanvas m_canvas;
    BoardSelectionManager m_selection;
    BoardTextEditor m_editor;

    // Submódulos refactorizados
    std::unique_ptr<BoardSearchManager> m_search;
    std::unique_ptr<BoardTagScanner> m_tagScanner;
    std::unique_ptr<BoardLayoutHelper> m_layoutHelper;

    // Items list
    std::vector<BoardItem> m_items;
    bool m_workspacePrepared=false;

    // Dynamic Connection/Link-creation mode
    bool m_linkingMode = false;
    int m_linkingFromIdx = -1;
    PointF m_linkingCurrentMousePos = {}; // Screen coordinates

    // Toast notification
    std::wstring m_toastMessage;
    double m_toastTimeRemaining = 0.0;
};

} // namespace kv

