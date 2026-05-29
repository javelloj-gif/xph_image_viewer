// ============================================================================
// board_page.h — Board Page (PureRef/Miro clone) Component
// Krumassan Image Viewer — Native C++20
// ============================================================================
#pragma once

#include "types.h"
#include "renderer.h"
#include "image_loader.h"
#include <vector>
#include <string>
#include <memory>

namespace kv {

class App; // Forward declaration

enum class BoardItemType {
    Undefined = 0,
    Image = 1,
    AV = 2,
    Folder = 3,
    Group = 4,
    Node = 5,
    Link = 6,
    Frame = 7,
    Note = 8,
    EditingTable = 9
};

struct BoardItem {
    BoardItemType type = BoardItemType::Undefined;
    PointF position = {0.0f, 0.0f}; // Center position in world space
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float rotation = 0.0f;          // Degrees
    float width = 200.0f;           // Local/Unscaled width
    float height = 200.0f;          // Local/Unscaled height
    bool visible = true;
    bool selected = false;
    std::wstring label;
    std::wstring status;
    std::wstring filePath;          // Source image/folder path
    std::wstring plainText;         // note content

    // Cached GPU Texture
    std::shared_ptr<ImageData> texture = nullptr;
    bool isLoading = false;
    bool loadFailed = false;

    // Link details
    int fromItemIdx = -1;
    int toItemIdx = -1;
    float linkWidth = 2.0f;
    bool isDirectional = true;

    // Frame/Note style
    Color frameColor = Color::White();

    // Helpers
    RectF GetLocalRect() const {
        return RectF::Centered(0, 0, width, height);
    }

    RectF GetWorldRect() const {
        float w = width * scaleX;
        float h = height * scaleY;
        return RectF::Centered(position.x, position.y, w, h);
    }
};

class BoardPage {
public:
    friend class App;

    BoardPage() = default;
    ~BoardPage() = default;

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

    // ── View Coordinate Mapping ───────────────────────────────────────────
    PointF ScreenToWorld(const PointF& screenPt) const;
    PointF WorldToScreen(const PointF& worldPt) const;
    RectF WorldToScreen(const RectF& worldRect) const;

    // ── Save/Load layout ──────────────────────────────────────────────────
    bool SaveBoard(const std::wstring& filePath);
    bool LoadBoard(const std::wstring& filePath);

private:
    void ResetView();
    void LoadItemTexture(BoardItem& item);
    RectF GetSelectionBoundingBox() const;
    void DrawCheckerboard(Renderer& renderer, const RectF& rect);
    void DeleteSelectedItems();
    void AddNoteAt(const PointF& worldPos, const std::wstring& text = L"Note");
    void AddNodeAt(const PointF& worldPos, const std::wstring& label = L"Node");
    void StartLinking(int fromIdx);
    float DistanceToLineSegment(const PointF& p, const PointF& a, const PointF& b) const;
    void UpdateNodeSize(BoardItem& item);

    App* m_app = nullptr;

    // Canvas transformations
    float m_zoom = 1.0f;
    PointF m_pan = {0.0f, 0.0f};

    // Items list
    std::vector<BoardItem> m_items;

    // Camera panning
    bool m_cameraPanning = false;
    PointF m_panStart = {};
    PointF m_mouseDragStart = {};

    // Box/Rubber-band selection
    bool m_boxSelecting = false;
    PointF m_boxSelectStart = {};
    PointF m_boxSelectEnd = {};

    // Item translation/dragging
    bool m_itemDragging = false;
    PointF m_dragStartMouseWorld = {};
    std::vector<PointF> m_dragStartItemPositions;

    // Bounding Box Scaling
    bool m_itemScaling = false;
    int m_scaleHandleIdx = -1; // 0=TL, 1=TR, 2=BR, 3=BL
    RectF m_scaleStartSelectionBox = {};
    std::vector<PointF> m_scaleStartItemPositions;
    std::vector<std::pair<float, float>> m_scaleStartItemScales;

    // Dynamic Connection/Link-creation mode
    bool m_linkingMode = false;
    int m_linkingFromIdx = -1;
    PointF m_linkingCurrentMousePos = {}; // Screen coordinates

    // Text editing note state
    int m_editingNoteIdx = -1;
    bool m_cursorBlinkState = false;
    double m_cursorBlinkTime = 0.0;

    // Background checkerboard
    ComPtr<ID2D1Bitmap1> m_checkerboardBitmap;
};

} // namespace kv
