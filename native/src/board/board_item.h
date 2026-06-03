// ============================================================================
// Struct: BoardItem
// Description: BoardItem represents the structural state and geometry of a single element (image, label, link, or note) on the canvas of [IMAGE_VIEWER_APP_NAME].
// Responsibility: Data container holding type, bounds, colors, link anchors, texture loaders, and visibility flags.
// ============================================================================
#pragma once

#include "common/types.h"
#include "loader/image_loader.h"
#include <string>
#include <memory>

namespace kv {

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

} // namespace kv
