// ============================================================================
// Class: BoardSelectionManager
// Description: BoardSelectionManager controls picking, rectangular box selection, and translation/scaling of elements on the canvas of Fotassa.
// Responsibility: Manage rubber-band selection, drag translations of selected item sets, handle scaling vectors from TL/TR/BL/BR control boundaries.
// ============================================================================
#pragma once

#include "board_item.h"
#include "rendering/renderer.h"
#include <vector>

namespace kv {

class BoardSelectionManager {
public:
    BoardSelectionManager() = default;
    ~BoardSelectionManager() = default;

    RectF GetSelectionBoundingBox(const std::vector<BoardItem>& items) const;

    void StartDragging(const PointF& mouseWorld, const std::vector<BoardItem>& items);
    void UpdateDragging(const PointF& mouseWorld, std::vector<BoardItem>& items);

    void StartScaling(int handleIdx, const RectF& selBox, const std::vector<BoardItem>& items);
    void UpdateScaling(const PointF& mouseWorld, std::vector<BoardItem>& items, bool shiftPressed);

    void StartBoxSelection(const PointF& mouseScreen);
    void UpdateBoxSelection(const PointF& mouseScreen, const PointF& startW, const PointF& endW, std::vector<BoardItem>& items);

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
};

} // namespace kv
