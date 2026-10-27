// ============================================================================
// Class: BoardItemRenderer
// Description: BoardItemRenderer draws nodes, image frames, and connection links on the virtual canvas of Fotassa.
// Responsibility: Direct2D drawing logic for nodes, connection links, editing status, and textured images.
// ============================================================================
#include "common/pch.h"
#include "i18n/i18n_engine.h"
#include "board_item_renderer.h"
#include <cmath>
#include <algorithm>

namespace kv {

void BoardItemRenderer::DrawLinks(
    Renderer& renderer,
    const std::vector<BoardItem>& items,
    float zoom
) {
    for (const auto& item : items) {
        if (item.type == BoardItemType::Link && item.fromItemIdx >= 0 && item.fromItemIdx < static_cast<int>(items.size()) &&
            item.toItemIdx >= 0 && item.toItemIdx < static_cast<int>(items.size())) {
            
            PointF p1 = items[item.fromItemIdx].position;
            PointF p2 = items[item.toItemIdx].position;

            Color linkCol = item.selected ? Color::FromHex(0xFF9800) : Color::FromHex(0x9E9E9E);
            renderer.DrawLine(p1, p2, linkCol, item.linkWidth / zoom);

            if (item.isDirectional) {
                // Draw arrow tip at p2
                float dx = p2.x - p1.x;
                float dy = p2.y - p1.y;
                float len = std::sqrt(dx * dx + dy * dy);
                if (len > 10.0f) {
                    dx /= len;
                    dy /= len;
                    float arrowSize = 15.0f;
                    PointF arrowP1 = { p2.x - dx * arrowSize + dy * arrowSize * 0.5f, p2.y - dy * arrowSize - dx * arrowSize * 0.5f };
                    PointF arrowP2 = { p2.x - dx * arrowSize - dy * arrowSize * 0.5f, p2.y - dy * arrowSize + dx * arrowSize * 0.5f };
                    renderer.DrawLine(p2, arrowP1, linkCol, item.linkWidth / zoom);
                    renderer.DrawLine(p2, arrowP2, linkCol, item.linkWidth / zoom);
                }
            }
        }
    }
}

void BoardItemRenderer::DrawItems(
    Renderer& renderer,
    std::vector<BoardItem>& items,
    int editingNoteIdx,
    bool cursorBlinkState,
    float zoom,
    const PointF& minWorld,
    const PointF& maxWorld,
    std::function<void(BoardItem&)> loadTextureCallback
) {
    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        auto& item = items[i];
        if (!item.visible || item.type == BoardItemType::Link) continue;

        RectF itemRect = item.GetWorldRect();

        // Frustum Culling
        if (itemRect.right < minWorld.x || itemRect.left > maxWorld.x ||
            itemRect.bottom < minWorld.y || itemRect.top > maxWorld.y) {
            continue;
        }

        // Draw Frame item (draw behind other elements)
        if (item.type == BoardItemType::Frame) {
            Color borderCol = item.selected ? Color::FromHex(0xFF9800) : item.frameColor;
            renderer.DrawRoundedRect(itemRect, 10.0f, 10.0f, borderCol, 2.0f / zoom);
            renderer.FillRoundedRect(itemRect, 10.0f, 10.0f, Color::FromRGBA(255, 255, 255, 5));
            
            // Draw label
            RectF textRect = RectF::FromXYWH(itemRect.left + 15.0f, itemRect.top + 10.0f, itemRect.Width() - 30.0f, 30.0f);
            renderer.DrawText(item.label, textRect, borderCol, 18.0f, TextAlign::Left, TextVAlign::Center);
            continue;
        }

        // Draw Note item (sticky note)
        if (item.type == BoardItemType::Note) {
            Color bgCol = Color::FromHex(0x2C2C2C);
            Color textCol = Color::White(0.9f);
            if (item.selected) {
                bgCol = Color::FromHex(0x3E3E3E);
            }

            renderer.FillRoundedRect(itemRect, 8.0f, 8.0f, bgCol);
            renderer.DrawRoundedRect(itemRect, 8.0f, 8.0f, item.selected ? Color::FromHex(0xFF9800) : Color::FromHex(0x444444), 1.0f);

            // Draw note plain text
            RectF paddingRect = RectF::FromXYWH(itemRect.left + 12.0f, itemRect.top + 12.0f, itemRect.Width() - 24.0f, itemRect.Height() - 24.0f);
            std::wstring displayText = item.plainText;
            
            if (editingNoteIdx == i) {
                // Draw edit outline
                renderer.DrawRoundedRect(itemRect, 8.0f, 8.0f, Color::FromHex(0x2196F3), 1.5f);
                if (cursorBlinkState) {
                    displayText += L"|";
                }
            }

            renderer.DrawText(displayText, paddingRect, textCol, 13.0f, TextAlign::Left, TextVAlign::Top);
            continue;
        }

        // Draw Node item (circle)
        if (item.type == BoardItemType::Node) {
            Color circleCol = item.selected ? Color::FromHex(0xFF9800) : Color::FromHex(0x4CAF50);
            if (editingNoteIdx == i) {
                circleCol = Color::FromHex(0x2196F3);
            }
            renderer.FillEllipse(item.position, itemRect.Width() * 0.5f, itemRect.Height() * 0.5f, Color::FromRGBA(0, 0, 0, 150));
            renderer.DrawEllipse(item.position, itemRect.Width() * 0.5f, itemRect.Height() * 0.5f, circleCol, 2.0f);
            
            // Draw label in center
            std::wstring displayText = item.label;
            if (editingNoteIdx == i && cursorBlinkState) {
                displayText += L"|";
            }
            float fontSize = 13.0f * std::max(0.1f, item.scaleX);
            renderer.DrawText(displayText, itemRect, circleCol, fontSize, TextAlign::Center, TextVAlign::Center);
            continue;
        }

        // Draw Image item
        if (item.type == BoardItemType::Image) {
            if (item.texture && item.texture->HasGpuBitmap()) {
                renderer.DrawBitmap(item.texture->gpuBitmap.Get(), itemRect, 1.0f, BitmapInterpolation::HighQuality);
                if (item.selected) {
                    renderer.DrawRect(itemRect, Color::FromHex(0xFF9800), 2.0f / zoom);
                }
            } else {
                // Placeholder loading
                renderer.FillRect(itemRect, Color::FromRGBA(25, 25, 25, 180));
                renderer.DrawRect(itemRect, item.selected ? Color::FromHex(0xFF9800) : Color::White(0.1f), 1.0f / zoom);
                
                std::wstring statusText = item.isLoading ? _(L"Loading...") : (item.loadFailed ? _(L"Load failed") : _(L"Queued"));
                renderer.DrawText(statusText, itemRect, Color::White(0.4f), 12.0f, TextAlign::Center, TextVAlign::Center);
                
                if (!item.isLoading && !item.loadFailed && loadTextureCallback) {
                    loadTextureCallback(item);
                }
            }

            // Draw label under the image
            RectF labelRect = RectF::FromXYWH(itemRect.left, itemRect.bottom + 5.0f, itemRect.Width(), 20.0f);
            renderer.DrawText(item.label, labelRect, Color::White(0.5f), 11.0f, TextAlign::Center, TextVAlign::Top);
        }
    }
}

void BoardItemRenderer::DrawActiveLinkFeedback(
    Renderer& renderer,
    const std::vector<BoardItem>& items,
    int linkingFromIdx,
    const PointF& linkingCurrentMouseWorld,
    float zoom
) {
    if (linkingFromIdx >= 0 && linkingFromIdx < static_cast<int>(items.size())) {
        PointF p1 = items[linkingFromIdx].position;
        renderer.DrawLine(p1, linkingCurrentMouseWorld, Color::FromHex(0x2196F3), 2.0f / zoom);
    }
}

} // namespace kv
