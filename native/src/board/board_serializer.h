// ============================================================================
// Class: BoardSerializer
// Description: BoardSerializer handles saving and reading canvas layouts using the custom binary format of [IMAGE_VIEWER_APP_NAME].
// Responsibility: Serialize/deserialize `BoardItem` structures to `.board` layout files.
// ============================================================================
#pragma once

#include "board_item.h"
#include <vector>
#include <string>
#include <functional>

namespace kv {

class BoardSerializer {
public:
    static bool SaveBoard(
        const std::wstring& filePath,
        float zoom,
        const PointF& pan,
        const std::vector<BoardItem>& items
    );

    static bool LoadBoard(
        const std::wstring& filePath,
        float& outZoom,
        PointF& outPan,
        std::vector<BoardItem>& outItems,
        std::function<void(BoardItem&)> loadTextureCallback
    );
};

} // namespace kv
