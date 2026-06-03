// ============================================================================
// Class: BoardSelectionManager
// Description: BoardSelectionManager controls picking, rectangular box selection, and translation/scaling of elements on the canvas of [IMAGE_VIEWER_APP_NAME].
// Responsibility: Manage rubber-band selection, drag translations of selected item sets, handle scaling vectors from TL/TR/BL/BR control boundaries.
// ============================================================================
#include "common/pch.h"
#include "board_selection_manager.h"
#include <algorithm>
#include <cmath>

namespace kv {

RectF BoardSelectionManager::GetSelectionBoundingBox(const std::vector<BoardItem>& items) const {
    float x1 = 1e9f, y1 = 1e9f, x2 = -1e9f, y2 = -1e9f;
    bool anySelected = false;

    for (const auto& item : items) {
        if (item.selected && item.type != BoardItemType::Link) {
            RectF r = item.GetWorldRect();
            if (r.left < x1) x1 = r.left;
            if (r.top < y1) y1 = r.top;
            if (r.right > x2) x2 = r.right;
            if (r.bottom > y2) y2 = r.bottom;
            anySelected = true;
        }
    }

    if (!anySelected) return {};
    return RectF(x1, y1, x2, y2);
}

void BoardSelectionManager::StartDragging(const PointF& mouseWorld, const std::vector<BoardItem>& items) {
    m_itemDragging = true;
    m_dragStartMouseWorld = mouseWorld;
    m_dragStartItemPositions.clear();
    for (const auto& item : items) {
        if (item.selected && item.type != BoardItemType::Link) {
            m_dragStartItemPositions.push_back(item.position);
        }
    }
}

void BoardSelectionManager::UpdateDragging(const PointF& mouseWorld, std::vector<BoardItem>& items) {
    if (!m_itemDragging) return;

    float dx = mouseWorld.x - m_dragStartMouseWorld.x;
    float dy = mouseWorld.y - m_dragStartMouseWorld.y;

    int selIdx = 0;
    for (auto& item : items) {
        if (item.selected && item.type != BoardItemType::Link) {
            if (selIdx < static_cast<int>(m_dragStartItemPositions.size())) {
                item.position.x = m_dragStartItemPositions[selIdx].x + dx;
                item.position.y = m_dragStartItemPositions[selIdx].y + dy;
                selIdx++;
            }
        }
    }
}

void BoardSelectionManager::StartScaling(int handleIdx, const RectF& selBox, const std::vector<BoardItem>& items) {
    m_itemScaling = true;
    m_scaleHandleIdx = handleIdx;
    m_scaleStartSelectionBox = selBox;
    m_scaleStartItemPositions.clear();
    m_scaleStartItemScales.clear();
    for (const auto& item : items) {
        if (item.selected && item.type != BoardItemType::Link) {
            m_scaleStartItemPositions.push_back(item.position);
            m_scaleStartItemScales.push_back({ item.scaleX, item.scaleY });
        }
    }
}

void BoardSelectionManager::UpdateScaling(const PointF& mouseWorld, std::vector<BoardItem>& items, bool shiftPressed) {
    if (!m_itemScaling || m_scaleHandleIdx < 0) return;

    PointF pivot = {};
    PointF initialCorner = {};
    
    if (m_scaleHandleIdx == 0) { // TL
        pivot = m_scaleStartSelectionBox.BottomRight();
        initialCorner = m_scaleStartSelectionBox.TopLeft();
    } else if (m_scaleHandleIdx == 1) { // TR
        pivot = m_scaleStartSelectionBox.BottomLeft();
        initialCorner = m_scaleStartSelectionBox.TopRight();
    } else if (m_scaleHandleIdx == 2) { // BR
        pivot = m_scaleStartSelectionBox.TopLeft();
        initialCorner = m_scaleStartSelectionBox.BottomRight();
    } else if (m_scaleHandleIdx == 3) { // BL
        pivot = m_scaleStartSelectionBox.TopRight();
        initialCorner = m_scaleStartSelectionBox.BottomLeft();
    }

    PointF initialVec = { initialCorner.x - pivot.x, initialCorner.y - pivot.y };
    PointF currentVec = { mouseWorld.x - pivot.x, mouseWorld.y - pivot.y };

    float scaleFactorX = (std::abs(initialVec.x) > 0.01f) ? (currentVec.x / initialVec.x) : 1.0f;
    float scaleFactorY = (std::abs(initialVec.y) > 0.01f) ? (currentVec.y / initialVec.y) : 1.0f;

    // Check if there is an image or a node in the selected items to enforce aspect-ratio preservation
    bool hasImageOrNode = false;
    for (const auto& item : items) {
        if (item.selected && (item.type == BoardItemType::Image || item.type == BoardItemType::Node)) {
            hasImageOrNode = true;
            break;
        }
    }

    if (hasImageOrNode || shiftPressed) {
        float sX = std::abs(scaleFactorX);
        float sY = std::abs(scaleFactorY);
        float uniformScale = (std::abs(sX - 1.0f) > std::abs(sY - 1.0f)) ? sX : sY;
        scaleFactorX = (scaleFactorX < 0.0f) ? -uniformScale : uniformScale;
        scaleFactorY = (scaleFactorY < 0.0f) ? -uniformScale : uniformScale;
    }

    // Apply scale factors to each selected item
    int selIdx = 0;
    for (auto& item : items) {
        if (item.selected && item.type != BoardItemType::Link) {
            if (selIdx < static_cast<int>(m_scaleStartItemPositions.size())) {
                PointF startPos = m_scaleStartItemPositions[selIdx];
                item.position.x = pivot.x + (startPos.x - pivot.x) * scaleFactorX;
                item.position.y = pivot.y + (startPos.y - pivot.y) * scaleFactorY;
                item.scaleX = m_scaleStartItemScales[selIdx].first * std::abs(scaleFactorX);
                item.scaleY = m_scaleStartItemScales[selIdx].second * std::abs(scaleFactorY);
                selIdx++;
            }
        }
    }
}

void BoardSelectionManager::StartBoxSelection(const PointF& mouseScreen) {
    m_boxSelecting = true;
    m_boxSelectStart = mouseScreen;
    m_boxSelectEnd = mouseScreen;
}

void BoardSelectionManager::UpdateBoxSelection(const PointF& mouseScreen, const PointF& startW, const PointF& endW, std::vector<BoardItem>& items) {
    if (!m_boxSelecting) return;

    m_boxSelectEnd = mouseScreen;
    
    float x1 = std::min(startW.x, endW.x);
    float y1 = std::min(startW.y, endW.y);
    float x2 = std::max(startW.x, endW.x);
    float y2 = std::max(startW.y, endW.y);
    RectF selWorldBox = RectF::FromXYWH(x1, y1, x2 - x1, y2 - y1);

    for (auto& item : items) {
        if (item.type == BoardItemType::Link) continue;
        RectF itemBox = item.GetWorldRect();
        item.selected = itemBox.Intersects(selWorldBox);
    }
}

} // namespace kv
