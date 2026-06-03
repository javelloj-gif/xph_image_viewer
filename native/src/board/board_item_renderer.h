// ============================================================================
// Class: BoardItemRenderer
// Description: BoardItemRenderer draws nodes, image frames, and connection links on the virtual canvas of [IMAGE_VIEWER_APP_NAME].
// Responsibility: Direct2D drawing logic for nodes, connection links, editing status, and textured images.
// ============================================================================
#pragma once

#include "board_item.h"
#include "rendering/renderer.h"
#include <vector>

namespace kv {

class BoardItemRenderer {
public:
    static void DrawLinks(
        Renderer& renderer,
        const std::vector<BoardItem>& items,
        float zoom
    );

    static void DrawItems(
        Renderer& renderer,
        std::vector<BoardItem>& items,
        int editingNoteIdx,
        bool cursorBlinkState,
        float zoom,
        const PointF& minWorld,
        const PointF& maxWorld,
        std::function<void(BoardItem&)> loadTextureCallback
    );

    static void DrawActiveLinkFeedback(
        Renderer& renderer,
        const std::vector<BoardItem>& items,
        int linkingFromIdx,
        const PointF& linkingCurrentMouseWorld,
        float zoom
    );
};

} // namespace kv
