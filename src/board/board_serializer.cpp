// ============================================================================
// Class: BoardSerializer
// Description: BoardSerializer handles saving and reading canvas layouts using the custom binary format of Fotassa.
// Responsibility: Serialize/deserialize `BoardItem` structures to `.board` layout files.
// ============================================================================
#include "common/pch.h"
#include "board_serializer.h"
#include "board_text_editor.h"
#include <fstream>
#include <cstdint>

namespace kv {

bool BoardSerializer::SaveBoard(
    const std::wstring& filePath,
    float zoom,
    const PointF& pan,
    const std::vector<BoardItem>& items
) {
    std::ofstream out(filePath, std::ios::binary);
    if (!out.is_open()) return false;

    // Header signature KBRD version 1
    char sig[4] = { 'K', 'B', 'R', 'D' };
    out.write(sig, 4);
    int32_t version = 1;
    out.write(reinterpret_cast<char*>(&version), 4);

    // Save camera transforms
    out.write(reinterpret_cast<const char*>(&zoom), 4);
    out.write(reinterpret_cast<const char*>(&pan.x), 4);
    out.write(reinterpret_cast<const char*>(&pan.y), 4);

    // Number of items
    int32_t numItems = static_cast<int32_t>(items.size());
    out.write(reinterpret_cast<char*>(&numItems), 4);

    for (const auto& item : items) {
        int32_t type = static_cast<int32_t>(item.type);
        out.write(reinterpret_cast<char*>(&type), 4);
        out.write(reinterpret_cast<const char*>(&item.position.x), 4);
        out.write(reinterpret_cast<const char*>(&item.position.y), 4);
        out.write(reinterpret_cast<const char*>(&item.scaleX), 4);
        out.write(reinterpret_cast<const char*>(&item.scaleY), 4);
        out.write(reinterpret_cast<const char*>(&item.rotation), 4);
        out.write(reinterpret_cast<const char*>(&item.width), 4);
        out.write(reinterpret_cast<const char*>(&item.height), 4);
        
        bool visible = item.visible;
        out.write(reinterpret_cast<char*>(&visible), 1);

        out.write(reinterpret_cast<const char*>(&item.fromItemIdx), 4);
        out.write(reinterpret_cast<const char*>(&item.toItemIdx), 4);
        out.write(reinterpret_cast<const char*>(&item.linkWidth), 4);

        bool isDir = item.isDirectional;
        out.write(reinterpret_cast<char*>(&isDir), 1);

        out.write(reinterpret_cast<const char*>(&item.frameColor.r), 4);
        out.write(reinterpret_cast<const char*>(&item.frameColor.g), 4);
        out.write(reinterpret_cast<const char*>(&item.frameColor.b), 4);
        out.write(reinterpret_cast<const char*>(&item.frameColor.a), 4);

        // Strings
        auto writeString = [&out](const std::wstring& str) {
            int32_t len = static_cast<int32_t>(str.size());
            out.write(reinterpret_cast<char*>(&len), 4);
            if (len > 0) {
                out.write(reinterpret_cast<const char*>(str.data()), len * sizeof(wchar_t));
            }
        };

        writeString(item.label);
        writeString(item.status);
        writeString(item.filePath);
        writeString(item.plainText);
    }

    out.close();
    return true;
}

bool BoardSerializer::LoadBoard(
    const std::wstring& filePath,
    float& outZoom,
    PointF& outPan,
    std::vector<BoardItem>& outItems,
    std::function<void(BoardItem&)> loadTextureCallback
) {
    std::ifstream in(filePath, std::ios::binary);
    if (!in.is_open()) return false;

    char sig[4];
    in.read(sig, 4);
    if (sig[0] != 'K' || sig[1] != 'B' || sig[2] != 'R' || sig[3] != 'D') return false;

    int32_t version = 0;
    in.read(reinterpret_cast<char*>(&version), 4);
    if (version != 1) return false;

    in.read(reinterpret_cast<char*>(&outZoom), 4);
    in.read(reinterpret_cast<char*>(&outPan.x), 4);
    in.read(reinterpret_cast<char*>(&outPan.y), 4);

    int32_t numItems = 0;
    in.read(reinterpret_cast<char*>(&numItems), 4);

    outItems.clear();
    outItems.reserve(numItems);

    for (int i = 0; i < numItems; ++i) {
        BoardItem item;
        int32_t type = 0;
        in.read(reinterpret_cast<char*>(&type), 4);
        item.type = static_cast<BoardItemType>(type);

        in.read(reinterpret_cast<char*>(&item.position.x), 4);
        in.read(reinterpret_cast<char*>(&item.position.y), 4);
        in.read(reinterpret_cast<char*>(&item.scaleX), 4);
        in.read(reinterpret_cast<char*>(&item.scaleY), 4);
        in.read(reinterpret_cast<char*>(&item.rotation), 4);
        in.read(reinterpret_cast<char*>(&item.width), 4);
        in.read(reinterpret_cast<char*>(&item.height), 4);

        bool visible = true;
        in.read(reinterpret_cast<char*>(&visible), 1);
        item.visible = visible;

        in.read(reinterpret_cast<char*>(&item.fromItemIdx), 4);
        in.read(reinterpret_cast<char*>(&item.toItemIdx), 4);
        in.read(reinterpret_cast<char*>(&item.linkWidth), 4);

        bool isDir = true;
        in.read(reinterpret_cast<char*>(&isDir), 1);
        item.isDirectional = isDir;

        in.read(reinterpret_cast<char*>(&item.frameColor.r), 4);
        in.read(reinterpret_cast<char*>(&item.frameColor.g), 4);
        in.read(reinterpret_cast<char*>(&item.frameColor.b), 4);
        in.read(reinterpret_cast<char*>(&item.frameColor.a), 4);

        auto readString = [&in]() {
            int32_t len = 0;
            in.read(reinterpret_cast<char*>(&len), 4);
            if (len <= 0) return std::wstring();
            std::wstring str;
            str.resize(len);
            in.read(reinterpret_cast<char*>(&str[0]), len * sizeof(wchar_t));
            return str;
        };

        item.label = readString();
        item.status = readString();
        item.filePath = readString();
        item.plainText = readString();

        if (item.type == BoardItemType::Node) {
            BoardTextEditor::UpdateNodeSize(item);
        }

        outItems.push_back(item);
    }

    in.close();

    // Trigger image texture reloading
    for (auto& item : outItems) {
        if (item.type == BoardItemType::Image && loadTextureCallback) {
            loadTextureCallback(item);
        }
    }

    return true;
}

} // namespace kv
