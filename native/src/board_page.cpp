// ============================================================================
// board_page.cpp — Board Page Component Implementation
// Krumassan Image Viewer — Native C++20
// ============================================================================
#include "pch.h"
#include "board_page.h"
#include "app.h"
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace kv {

// ─── Initialize ──────────────────────────────────────────────────────────────
void BoardPage::Initialize(App* app) {
    m_app = app;
    ResetView();

    // Create some default items if no board file exists
    // We add a welcome note and node
    AddNoteAt({ 0.0f, -100.0f }, L"Welcome to Krumassan Board!\n\n- Drag & Drop images/folders\n- Mouse Wheel to Zoom\n- Middle-click Drag to Pan\n- Left-click Drag to Box Select\n- Double-click notes to edit\n- Right-click to Add Note/Node or Link items");
    
    // Add default image placeholder if there is an image in app filelist
    if (!m_app->m_fileList.empty()) {
        BoardItem imgItem;
        imgItem.type = BoardItemType::Image;
        imgItem.position = { 250.0f, 50.0f };
        imgItem.filePath = m_app->m_fileList[0];
        imgItem.label = std::filesystem::path(imgItem.filePath).filename().wstring();
        imgItem.width = 300.0f;
        imgItem.height = 200.0f;
        m_items.push_back(imgItem);
        LoadItemTexture(m_items.back());
    }

    // Try loading saved board from parent folder of active image if possible
    if (!m_app->m_fileList.empty()) {
        namespace fs = std::filesystem;
        fs::path activePath(m_app->m_fileList[0]);
        auto boardPath = (activePath.parent_path() / L"!board.kbrd").wstring();
        if (fs::exists(boardPath)) {
            LoadBoard(boardPath);
        }
    }
}

void BoardPage::OnEnterPage() {
    std::wstring activePath;
    if (m_app->m_currentImage) {
        activePath = m_app->m_currentImage->filePath;
    } else if (m_app->m_animatedImage) {
        activePath = m_app->m_animatedImage->filePath;
    } else if (!m_app->m_fileList.empty() && m_app->m_currentIndex >= 0 && m_app->m_currentIndex < static_cast<int>(m_app->m_fileList.size())) {
        activePath = m_app->m_fileList[m_app->m_currentIndex];
    }

    if (!activePath.empty()) {
        bool alreadyExists = false;
        for (const auto& item : m_items) {
            if (item.type == BoardItemType::Image && item.filePath == activePath) {
                alreadyExists = true;
                break;
            }
        }

        if (!alreadyExists) {
            auto targetSize = m_app->m_renderer.GetTargetSize().ToFloat();
            PointF screenCenter = { targetSize.width * 0.5f, targetSize.height * 0.5f };
            PointF worldCenter = ScreenToWorld(screenCenter);

            BoardItem imgItem;
            imgItem.type = BoardItemType::Image;
            imgItem.position = worldCenter;
            imgItem.filePath = activePath;
            imgItem.label = std::filesystem::path(imgItem.filePath).filename().wstring();
            imgItem.width = 400.0f;
            imgItem.height = 300.0f;
            m_items.push_back(imgItem);
            LoadItemTexture(m_items.back());
        }
    }
}

// ─── Update ──────────────────────────────────────────────────────────────────
void BoardPage::Update(double dt) {
    // Blink cursor for text editing
    if (m_editingNoteIdx != -1) {
        m_cursorBlinkTime += dt;
        if (m_cursorBlinkTime >= 0.5) {
            m_cursorBlinkState = !m_cursorBlinkState;
            m_cursorBlinkTime = 0.0;
            m_app->m_window.RequestRedraw();
        }
    }
}

// ─── Draw ────────────────────────────────────────────────────────────────────
void BoardPage::Draw(Renderer& renderer) {
    auto size = renderer.GetTargetSize().ToFloat();

    // 1. Draw Checkerboard background across the whole window
    DrawCheckerboard(renderer, RectF::FromXYWH(0, 0, size.width, size.height));

    // 2. Apply Camera viewport transforms (Translation & Scale)
    renderer.PushTranslation(m_pan.x, m_pan.y);
    renderer.PushScale(m_zoom, m_zoom);

    // Render Grid Lines for canvas texture
    Color gridColor = Color::FromRGBA(255, 255, 255, 10);
    float gridSize = 100.0f;
    PointF minWorld = ScreenToWorld({ 0, 0 });
    PointF maxWorld = ScreenToWorld({ size.width, size.height });

    float startX = std::floor(minWorld.x / gridSize) * gridSize;
    float endX = std::ceil(maxWorld.x / gridSize) * gridSize;
    float startY = std::floor(minWorld.y / gridSize) * gridSize;
    float endY = std::ceil(maxWorld.y / gridSize) * gridSize;

    for (float x = startX; x <= endX; x += gridSize) {
        renderer.DrawLine({ x, minWorld.y }, { x, maxWorld.y }, gridColor, 1.0f / m_zoom);
    }
    for (float y = startY; y <= endY; y += gridSize) {
        renderer.DrawLine({ minWorld.x, y }, { maxWorld.x, y }, gridColor, 1.0f / m_zoom);
    }

    // 3. Draw Connections / Links (draw behind items)
    for (const auto& item : m_items) {
        if (item.type == BoardItemType::Link && item.fromItemIdx >= 0 && item.fromItemIdx < static_cast<int>(m_items.size()) &&
            item.toItemIdx >= 0 && item.toItemIdx < static_cast<int>(m_items.size())) {
            
            PointF p1 = m_items[item.fromItemIdx].position;
            PointF p2 = m_items[item.toItemIdx].position;

            Color linkCol = item.selected ? Color::FromHex(0xFF9800) : Color::FromHex(0x9E9E9E);
            renderer.DrawLine(p1, p2, linkCol, item.linkWidth / m_zoom);

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
                    renderer.DrawLine(p2, arrowP1, linkCol, item.linkWidth / m_zoom);
                    renderer.DrawLine(p2, arrowP2, linkCol, item.linkWidth / m_zoom);
                }
            }
        }
    }

    // 4. Draw Board Items
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        auto& item = m_items[i];
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
            renderer.DrawRoundedRect(itemRect, 10.0f, 10.0f, borderCol, 2.0f / m_zoom);
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
            
            if (m_editingNoteIdx == i) {
                // Draw edit outline
                renderer.DrawRoundedRect(itemRect, 8.0f, 8.0f, Color::FromHex(0x2196F3), 1.5f);
                if (m_cursorBlinkState) {
                    displayText += L"|";
                }
            }

            renderer.DrawText(displayText, paddingRect, textCol, 13.0f, TextAlign::Left, TextVAlign::Top);
            continue;
        }

        // Draw Node item (circle)
        if (item.type == BoardItemType::Node) {
            Color circleCol = item.selected ? Color::FromHex(0xFF9800) : Color::FromHex(0x4CAF50);
            if (m_editingNoteIdx == i) {
                circleCol = Color::FromHex(0x2196F3);
            }
            renderer.FillEllipse(item.position, itemRect.Width() * 0.5f, itemRect.Height() * 0.5f, Color::FromRGBA(0, 0, 0, 150));
            renderer.DrawEllipse(item.position, itemRect.Width() * 0.5f, itemRect.Height() * 0.5f, circleCol, 2.0f);
            
            // Draw label in center
            std::wstring displayText = item.label;
            if (m_editingNoteIdx == i && m_cursorBlinkState) {
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
                    renderer.DrawRect(itemRect, Color::FromHex(0xFF9800), 2.0f / m_zoom);
                }
            } else {
                // Placeholder loading
                renderer.FillRect(itemRect, Color::FromRGBA(25, 25, 25, 180));
                renderer.DrawRect(itemRect, item.selected ? Color::FromHex(0xFF9800) : Color::White(0.1f), 1.0f / m_zoom);
                
                std::wstring statusText = item.isLoading ? L"LOADING..." : (item.loadFailed ? L"LOAD FAILED" : L"QUEUED");
                renderer.DrawText(statusText, itemRect, Color::White(0.4f), 12.0f, TextAlign::Center, TextVAlign::Center);
                
                if (!item.isLoading && !item.loadFailed) {
                    LoadItemTexture(item);
                }
            }

            // Draw label under the image
            RectF labelRect = RectF::FromXYWH(itemRect.left, itemRect.bottom + 5.0f, itemRect.Width(), 20.0f);
            renderer.DrawText(item.label, labelRect, Color::White(0.5f), 11.0f, TextAlign::Center, TextVAlign::Top);
        }
    }

    // 5. Draw active linking feedback line (world coordinates)
    if (m_linkingMode && m_linkingFromIdx >= 0 && m_linkingFromIdx < static_cast<int>(m_items.size())) {
        PointF p1 = m_items[m_linkingFromIdx].position;
        PointF p2 = ScreenToWorld(m_linkingCurrentMousePos);
        renderer.DrawLine(p1, p2, Color::FromHex(0x2196F3), 2.0f / m_zoom);
    }

    renderer.PopTransform(); // Pop scale
    renderer.PopTransform(); // Pop translation

    // 6. Draw Screen-Space Selection Box Overlays
    RectF selBox = GetSelectionBoundingBox();
    if (selBox.Width() > 0.0f) {
        RectF selScreenBox = WorldToScreen(selBox);
        renderer.DrawRect(selScreenBox, Color::FromHex(0xFF9800), 1.0f);

        // Draw 4 corner handles
        float handleSize = 8.0f;
        Color handleColor = Color::FromHex(0xFF9800);
        renderer.FillRect(RectF::FromXYWH(selScreenBox.left - handleSize * 0.5f, selScreenBox.top - handleSize * 0.5f, handleSize, handleSize), handleColor);
        renderer.FillRect(RectF::FromXYWH(selScreenBox.right - handleSize * 0.5f, selScreenBox.top - handleSize * 0.5f, handleSize, handleSize), handleColor);
        renderer.FillRect(RectF::FromXYWH(selScreenBox.right - handleSize * 0.5f, selScreenBox.bottom - handleSize * 0.5f, handleSize, handleSize), handleColor);
        renderer.FillRect(RectF::FromXYWH(selScreenBox.left - handleSize * 0.5f, selScreenBox.bottom - handleSize * 0.5f, handleSize, handleSize), handleColor);
    }

    // 7. Draw screen space rubber-band box selection rectangle
    if (m_boxSelecting) {
        float x1 = std::min(m_boxSelectStart.x, m_boxSelectEnd.x);
        float y1 = std::min(m_boxSelectStart.y, m_boxSelectEnd.y);
        float x2 = std::max(m_boxSelectStart.x, m_boxSelectEnd.x);
        float y2 = std::max(m_boxSelectStart.y, m_boxSelectEnd.y);
        RectF selectionRect = RectF::FromXYWH(x1, y1, x2 - x1, y2 - y1);
        renderer.DrawRect(selectionRect, Color::White(0.8f), 1.0f);
        renderer.FillRect(selectionRect, Color::FromRGBA(255, 255, 255, 20));
    }

    // 8. Help Overlay info text in floating island (bottom-center)
    float islandWidth = 850.0f;
    float islandHeight = 35.0f;
    float islandX = (size.width - islandWidth) * 0.5f;
    float islandY = size.height - islandHeight - 20.0f;

    RectF islandRect = RectF::FromXYWH(islandX, islandY, islandWidth, islandHeight);
    renderer.FillRoundedRect(islandRect, 8.0f, 8.0f, Color::FromHex(0x1E1E1E, 0.85f));
    renderer.DrawRoundedRect(islandRect, 8.0f, 8.0f, Color::FromHex(0xFF9800, 0.5f), 1.0f);

    RectF textRect = RectF::FromXYWH(islandX + 15.0f, islandY + 5.0f, islandWidth - 30.0f, islandHeight - 10.0f);
    renderer.DrawText(L"Board Mode | Scroll: Zoom | Middle Drag: Pan | Left Drag: Move/Select | Ctrl+Click: Link | Right-Click: Menu | Delete: Remove | Ctrl+S: Save", 
                      textRect, Color::White(0.9f), 11.0f, TextAlign::Center, TextVAlign::Center);
}

// ─── Input Routing ────────────────────────────────────────────────────────────

bool BoardPage::OnMouseMove(float x, float y, KeyModifiers mods) {
    m_linkingCurrentMousePos = { x, y };

    // 1. Camera Panning (Middle click drag)
    if (m_cameraPanning) {
        m_pan.x = m_panStart.x + (x - m_mouseDragStart.x);
        m_pan.y = m_panStart.y + (y - m_mouseDragStart.y);
        m_app->m_window.RequestRedraw();
        return true;
    }

    // 2. Selection Box Scaling (Corner handles drag)
    if (m_itemScaling && m_scaleHandleIdx >= 0) {
        PointF mouseWorld = ScreenToWorld({ x, y });
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
        for (const auto& item : m_items) {
            if (item.selected && (item.type == BoardItemType::Image || item.type == BoardItemType::Node)) {
                hasImageOrNode = true;
                break;
            }
        }

        if (hasImageOrNode || mods.shift) {
            float sX = std::abs(scaleFactorX);
            float sY = std::abs(scaleFactorY);
            float uniformScale = (std::abs(sX - 1.0f) > std::abs(sY - 1.0f)) ? sX : sY;
            scaleFactorX = (scaleFactorX < 0.0f) ? -uniformScale : uniformScale;
            scaleFactorY = (scaleFactorY < 0.0f) ? -uniformScale : uniformScale;
        }

        // Apply scale factors to each selected item
        int selIdx = 0;
        for (auto& item : m_items) {
            if (item.selected && item.type != BoardItemType::Link) {
                PointF startPos = m_scaleStartItemPositions[selIdx];
                item.position.x = pivot.x + (startPos.x - pivot.x) * scaleFactorX;
                item.position.y = pivot.y + (startPos.y - pivot.y) * scaleFactorY;
                item.scaleX = m_scaleStartItemScales[selIdx].first * std::abs(scaleFactorX);
                item.scaleY = m_scaleStartItemScales[selIdx].second * std::abs(scaleFactorY);
                selIdx++;
            }
        }
        m_app->m_window.RequestRedraw();
        return true;
    }

    // 3. Item Dragging (Left click translation)
    if (m_itemDragging) {
        PointF mouseWorld = ScreenToWorld({ x, y });
        float dx = mouseWorld.x - m_dragStartMouseWorld.x;
        float dy = mouseWorld.y - m_dragStartMouseWorld.y;

        int selIdx = 0;
        for (auto& item : m_items) {
            if (item.selected && item.type != BoardItemType::Link) {
                item.position.x = m_dragStartItemPositions[selIdx].x + dx;
                item.position.y = m_dragStartItemPositions[selIdx].y + dy;
                selIdx++;
            }
        }
        m_app->m_window.RequestRedraw();
        return true;
    }

    // 4. Box Selecting (Rubber-band drag)
    if (m_boxSelecting) {
        m_boxSelectEnd = { x, y };
        
        // Check intersection of world bounding box with all items
        PointF startW = ScreenToWorld(m_boxSelectStart);
        PointF endW = ScreenToWorld(m_boxSelectEnd);
        float x1 = std::min(startW.x, endW.x);
        float y1 = std::min(startW.y, endW.y);
        float x2 = std::max(startW.x, endW.x);
        float y2 = std::max(startW.y, endW.y);
        RectF selWorldBox = RectF::FromXYWH(x1, y1, x2 - x1, y2 - y1);

        for (auto& item : m_items) {
            if (item.type == BoardItemType::Link) continue;
            RectF itemBox = item.GetWorldRect();
            item.selected = itemBox.Intersects(selWorldBox);
        }
        m_app->m_window.RequestRedraw();
        return true;
    }

    return false;
}

bool BoardPage::OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods) {
    PointF worldPos = ScreenToWorld({ x, y });

    // 1. Middle mouse pan
    if (btn == MouseButton::Middle) {
        m_cameraPanning = true;
        m_mouseDragStart = { x, y };
        m_panStart = m_pan;
        return true;
    }

    // 2. Left click interaction
    if (btn == MouseButton::Left) {
        // Exit text editing if clicked elsewhere
        if (m_editingNoteIdx != -1) {
            RectF editBox = m_items[m_editingNoteIdx].GetWorldRect();
            if (!editBox.Contains(worldPos.x, worldPos.y)) {
                m_editingNoteIdx = -1;
                m_app->m_window.RequestRedraw();
            }
        }

        // Check if clicked over scaling handles
        RectF selBox = GetSelectionBoundingBox();
        if (selBox.Width() > 0.0f) {
            RectF selScreenBox = WorldToScreen(selBox);
            float handleSize = 10.0f;
            RectF handles[4] = {
                RectF::Centered(selScreenBox.left, selScreenBox.top, handleSize, handleSize),
                RectF::Centered(selScreenBox.right, selScreenBox.top, handleSize, handleSize),
                RectF::Centered(selScreenBox.right, selScreenBox.bottom, handleSize, handleSize),
                RectF::Centered(selScreenBox.left, selScreenBox.bottom, handleSize, handleSize)
            };

            for (int hIdx = 0; hIdx < 4; ++hIdx) {
                if (handles[hIdx].Contains(x, y)) {
                    m_itemScaling = true;
                    m_scaleHandleIdx = hIdx;
                    m_scaleStartSelectionBox = selBox;
                    m_scaleStartItemPositions.clear();
                    m_scaleStartItemScales.clear();
                    for (const auto& item : m_items) {
                        if (item.selected && item.type != BoardItemType::Link) {
                            m_scaleStartItemPositions.push_back(item.position);
                            m_scaleStartItemScales.push_back({ item.scaleX, item.scaleY });
                        }
                    }
                    return true;
                }
            }
        }

        // Check if clicked over any Board Item (highest index/z-order first)
        int clickedIdx = -1;
        for (int i = static_cast<int>(m_items.size()) - 1; i >= 0; --i) {
            if (m_items[i].type == BoardItemType::Link) continue;
            RectF rect = m_items[i].GetWorldRect();
            if (rect.Contains(worldPos.x, worldPos.y)) {
                clickedIdx = i;
                break;
            }
        }

        if (clickedIdx == -1) {
            // Check if clicked over any Link segment
            float minDistance = 8.0f / m_zoom; // 8 screen-pixels tolerance
            for (int i = static_cast<int>(m_items.size()) - 1; i >= 0; --i) {
                auto& item = m_items[i];
                if (item.type == BoardItemType::Link && item.fromItemIdx >= 0 && item.fromItemIdx < static_cast<int>(m_items.size()) &&
                    item.toItemIdx >= 0 && item.toItemIdx < static_cast<int>(m_items.size())) {
                    PointF p1 = m_items[item.fromItemIdx].position;
                    PointF p2 = m_items[item.toItemIdx].position;
                    if (DistanceToLineSegment(worldPos, p1, p2) < minDistance) {
                        clickedIdx = i;
                        break;
                    }
                }
            }
        }

        if (clickedIdx != -1) {
            // Clicked on an item
            auto& clickedItem = m_items[clickedIdx];

            // Ctrl modifier activates linking or multiselect toggle
            if (mods.ctrl) {
                StartLinking(clickedIdx);
                return true;
            }

            if (!clickedItem.selected) {
                if (!mods.shift) {
                    // Deselect others
                    for (auto& item : m_items) item.selected = false;
                }
                clickedItem.selected = true;
            } else if (mods.shift) {
                // Deselect on shift click if already selected
                clickedItem.selected = false;
            }

            // Start translation dragging
            m_itemDragging = true;
            m_dragStartMouseWorld = worldPos;
            m_dragStartItemPositions.clear();
            for (const auto& item : m_items) {
                if (item.selected && item.type != BoardItemType::Link) {
                    m_dragStartItemPositions.push_back(item.position);
                }
            }
            m_app->m_window.RequestRedraw();
            return true;
        } else {
            // Clicked empty canvas space
            if (!mods.shift) {
                for (auto& item : m_items) item.selected = false;
            }
            m_boxSelecting = true;
            m_boxSelectStart = { x, y };
            m_boxSelectEnd = { x, y };
            m_app->m_window.RequestRedraw();
            return true;
        }
    }

    return false;
}

bool BoardPage::OnMouseUp(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (btn == MouseButton::Middle) {
        m_cameraPanning = false;
        return true;
    }

    if (btn == MouseButton::Right) {
        POINT pt;
        if (GetCursorPos(&pt)) {
            PointF worldPos = ScreenToWorld({ x, y });
            int clickedIdx = -1;
            for (int i = static_cast<int>(m_items.size()) - 1; i >= 0; --i) {
                if (m_items[i].type == BoardItemType::Link) continue;
                if (m_items[i].GetWorldRect().Contains(worldPos.x, worldPos.y)) {
                    clickedIdx = i;
                    break;
                }
            }

            if (clickedIdx == -1) {
                // Check if clicked over any Link
                float minDistance = 8.0f / m_zoom;
                for (int i = static_cast<int>(m_items.size()) - 1; i >= 0; --i) {
                    auto& item = m_items[i];
                    if (item.type == BoardItemType::Link && item.fromItemIdx >= 0 && item.fromItemIdx < static_cast<int>(m_items.size()) &&
                        item.toItemIdx >= 0 && item.toItemIdx < static_cast<int>(m_items.size())) {
                        PointF p1 = m_items[item.fromItemIdx].position;
                        PointF p2 = m_items[item.toItemIdx].position;
                        if (DistanceToLineSegment(worldPos, p1, p2) < minDistance) {
                            clickedIdx = i;
                            break;
                        }
                    }
                }
            }

            HMENU hMenu = CreatePopupMenu();
            if (clickedIdx != -1) {
                if (m_items[clickedIdx].type == BoardItemType::Link) {
                    AppendMenuW(hMenu, MF_STRING, 5, L"Delete Link");
                } else {
                    AppendMenuW(hMenu, MF_STRING, 1, L"Link this item");
                    AppendMenuW(hMenu, MF_STRING, 2, L"Delete item");
                }
            } else {
                AppendMenuW(hMenu, MF_STRING, 3, L"Add Note");
                AppendMenuW(hMenu, MF_STRING, 4, L"Add Node");
            }

            int selected = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                                          pt.x, pt.y, 0, m_app->m_window.GetHwnd(), nullptr);
            DestroyMenu(hMenu);

            if (selected == 1) {
                StartLinking(clickedIdx);
            } else if (selected == 2) {
                for (auto& item : m_items) item.selected = false;
                m_items[clickedIdx].selected = true;
                DeleteSelectedItems();
            } else if (selected == 3) {
                AddNoteAt(worldPos, L"Note");
                m_app->m_window.RequestRedraw();
            } else if (selected == 4) {
                AddNodeAt(worldPos, L"Node");
                m_app->m_window.RequestRedraw();
            } else if (selected == 5) {
                for (auto& item : m_items) item.selected = false;
                m_items[clickedIdx].selected = true;
                DeleteSelectedItems();
            }
        }
        return true;
    }

    if (btn == MouseButton::Left) {
        if (m_itemScaling) {
            m_itemScaling = false;
            m_scaleHandleIdx = -1;
            return true;
        }
        if (m_itemDragging) {
            m_itemDragging = false;
            return true;
        }
        if (m_boxSelecting) {
            m_boxSelecting = false;
            m_app->m_window.RequestRedraw();
            return true;
        }
        if (m_linkingMode) {
            // Finish linking if released over another item
            PointF worldPos = ScreenToWorld({ x, y });
            int targetIdx = -1;
            for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
                if (i == m_linkingFromIdx || m_items[i].type == BoardItemType::Link) continue;
                if (m_items[i].GetWorldRect().Contains(worldPos.x, worldPos.y)) {
                    targetIdx = i;
                    break;
                }
            }

            if (targetIdx != -1) {
                // Create link item
                BoardItem linkItem;
                linkItem.type = BoardItemType::Link;
                linkItem.fromItemIdx = m_linkingFromIdx;
                linkItem.toItemIdx = targetIdx;
                linkItem.linkWidth = 2.0f;
                linkItem.isDirectional = true;
                m_items.push_back(linkItem);
            }

            m_linkingMode = false;
            m_linkingFromIdx = -1;
            m_app->m_window.RequestRedraw();
            return true;
        }
    }

    return false;
}

bool BoardPage::OnMouseWheel(float delta, float x, float y, KeyModifiers mods) {
    // Zoom canvas centering on mouse cursor
    float factor = (delta > 0) ? 1.07f : (1.0f / 1.07f);
    float oldZoom = m_zoom;
    
    m_zoom = m_zoom * factor;
    m_zoom = std::clamp(m_zoom, 0.05f, 50.0f); // Board zoom limit: 5% to 5000%
    
    float actualFactor = m_zoom / oldZoom;
    
    // Zoom anchor centering
    m_pan.x = x - (x - m_pan.x) * actualFactor;
    m_pan.y = y - (y - m_pan.y) * actualFactor;

    m_app->m_window.RequestRedraw();
    return true;
}

bool BoardPage::OnMouseDoubleClick(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (btn != MouseButton::Left) return false;

    PointF worldPos = ScreenToWorld({ x, y });

    // Check if double clicked over image to open in viewer
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        if (m_items[i].type == BoardItemType::Image && m_items[i].GetWorldRect().Contains(worldPos.x, worldPos.y)) {
            if (!m_items[i].filePath.empty()) {
                m_app->m_fileList.clear();
                m_app->m_currentIndex = -1;
                m_app->LoadImage(m_items[i].filePath);
                m_app->SetPage(Page::Viewer);
                return true;
            }
        }
    }

    // Check if double clicked over note to edit
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        if (m_items[i].type == BoardItemType::Note && m_items[i].GetWorldRect().Contains(worldPos.x, worldPos.y)) {
            m_editingNoteIdx = i;
            m_cursorBlinkState = true;
            m_cursorBlinkTime = 0.0;
            m_app->m_window.RequestRedraw();
            return true;
        }
    }

    // Check if double clicked over node to edit
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        if (m_items[i].type == BoardItemType::Node && m_items[i].GetWorldRect().Contains(worldPos.x, worldPos.y)) {
            m_editingNoteIdx = i;
            m_cursorBlinkState = true;
            m_cursorBlinkTime = 0.0;
            m_app->m_window.RequestRedraw();
            return true;
        }
    }

    // Double click canvas to reset view
    ResetView();
    m_app->m_window.RequestRedraw();
    return true;
}

bool BoardPage::OnKeyDown(uint32_t vk, KeyModifiers mods) {
    // 1. Text editing keys routing
    if (m_editingNoteIdx != -1) {
        if (vk == VK_ESCAPE) {
            m_editingNoteIdx = -1;
            m_app->m_window.RequestRedraw();
        }
        return true;
    }

    // 2. Canvas standard shortcuts
    switch (vk) {
        case VK_DELETE:
            DeleteSelectedItems();
            return true;
        case VK_ESCAPE:
            for (auto& item : m_items) item.selected = false;
            m_app->m_window.RequestRedraw();
            return true;
        case 'S':
            if (mods.ctrl) {
                // Save board layout
                if (!m_app->m_fileList.empty()) {
                    namespace fs = std::filesystem;
                    fs::path activePath(m_app->m_fileList[0]);
                    auto boardPath = (activePath.parent_path() / L"!board.kbrd").wstring();
                    SaveBoard(boardPath);
                }
                return true;
            }
            break;
    }
    return false;
}

void BoardPage::OnChar(wchar_t ch) {
    if (m_editingNoteIdx == -1) return;

    auto& item = m_items[m_editingNoteIdx];
    if (ch == L'\b') {
        if (item.type == BoardItemType::Note) {
            if (!item.plainText.empty()) {
                item.plainText.pop_back();
                m_app->m_window.RequestRedraw();
            }
        } else if (item.type == BoardItemType::Node) {
            if (!item.label.empty()) {
                item.label.pop_back();
                UpdateNodeSize(item);
                m_app->m_window.RequestRedraw();
            }
        }
    } else if (ch == L'\r' || ch == L'\n') {
        if (item.type == BoardItemType::Note) {
            item.plainText.push_back(L'\n');
            m_app->m_window.RequestRedraw();
        } else if (item.type == BoardItemType::Node) {
            m_editingNoteIdx = -1;
            m_app->m_window.RequestRedraw();
        }
    } else if (ch == 27) {
        m_editingNoteIdx = -1;
        m_app->m_window.RequestRedraw();
    } else if (ch >= 32) {
        if (item.type == BoardItemType::Note) {
            item.plainText.push_back(ch);
            m_app->m_window.RequestRedraw();
        } else if (item.type == BoardItemType::Node) {
            item.label.push_back(ch);
            UpdateNodeSize(item);
            m_app->m_window.RequestRedraw();
        }
    }
}

void BoardPage::OnFileDrop(const std::wstring& path, float screenX, float screenY) {
    PointF worldPos = ScreenToWorld({ screenX, screenY });
    namespace fs = std::filesystem;

    if (!fs::exists(path)) return;

    if (fs::is_directory(path)) {
        // Create folder item
        BoardItem folderItem;
        folderItem.type = BoardItemType::Folder;
        folderItem.position = worldPos;
        folderItem.filePath = path;
        folderItem.label = fs::path(path).filename().wstring();
        folderItem.width = 300.0f;
        folderItem.height = 200.0f;
        m_items.push_back(folderItem);
    } else if (ImageLoader::IsSupported(path)) {
        // Create image item
        BoardItem imgItem;
        imgItem.type = BoardItemType::Image;
        imgItem.position = worldPos;
        imgItem.filePath = path;
        imgItem.label = fs::path(path).filename().wstring();
        imgItem.width = 400.0f;
        imgItem.height = 300.0f;
        m_items.push_back(imgItem);
        LoadItemTexture(m_items.back());
    }

    m_app->m_window.RequestRedraw();
}

// ─── Save / Load ─────────────────────────────────────────────────────────────

bool BoardPage::SaveBoard(const std::wstring& filePath) {
    std::ofstream out(filePath, std::ios::binary);
    if (!out.is_open()) return false;

    // Header signature KBRD version 1
    char sig[4] = { 'K', 'B', 'R', 'D' };
    out.write(sig, 4);
    int32_t version = 1;
    out.write(reinterpret_cast<char*>(&version), 4);

    // Save camera transforms
    out.write(reinterpret_cast<char*>(&m_zoom), 4);
    out.write(reinterpret_cast<char*>(&m_pan.x), 4);
    out.write(reinterpret_cast<char*>(&m_pan.y), 4);

    // Number of items
    int32_t numItems = static_cast<int32_t>(m_items.size());
    out.write(reinterpret_cast<char*>(&numItems), 4);

    for (const auto& item : m_items) {
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

bool BoardPage::LoadBoard(const std::wstring& filePath) {
    std::ifstream in(filePath, std::ios::binary);
    if (!in.is_open()) return false;

    char sig[4];
    in.read(sig, 4);
    if (sig[0] != 'K' || sig[1] != 'B' || sig[2] != 'R' || sig[3] != 'D') return false;

    int32_t version = 0;
    in.read(reinterpret_cast<char*>(&version), 4);
    if (version != 1) return false;

    in.read(reinterpret_cast<char*>(&m_zoom), 4);
    in.read(reinterpret_cast<char*>(&m_pan.x), 4);
    in.read(reinterpret_cast<char*>(&m_pan.y), 4);

    int32_t numItems = 0;
    in.read(reinterpret_cast<char*>(&numItems), 4);

    m_items.clear();
    m_items.reserve(numItems);

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
            UpdateNodeSize(item);
        }

        m_items.push_back(item);
    }

    in.close();

    // Trigger image texture reloading
    for (auto& item : m_items) {
        if (item.type == BoardItemType::Image) {
            LoadItemTexture(item);
        }
    }

    m_app->m_window.RequestRedraw();
    return true;
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

PointF BoardPage::ScreenToWorld(const PointF& screenPt) const {
    PointF worldPt;
    worldPt.x = (screenPt.x - m_pan.x) / m_zoom;
    worldPt.y = (screenPt.y - m_pan.y) / m_zoom;
    return worldPt;
}

PointF BoardPage::WorldToScreen(const PointF& worldPt) const {
    PointF screenPt;
    screenPt.x = worldPt.x * m_zoom + m_pan.x;
    screenPt.y = worldPt.y * m_zoom + m_pan.y;
    return screenPt;
}

RectF BoardPage::WorldToScreen(const RectF& worldRect) const {
    PointF tl = WorldToScreen(worldRect.TopLeft());
    PointF br = WorldToScreen(worldRect.BottomRight());
    return RectF(tl.x, tl.y, br.x, br.y);
}

void BoardPage::ResetView() {
    m_zoom = 1.0f;
    m_pan = { 0.0f, 0.0f };
    m_cameraPanning = false;
    m_boxSelecting = false;
    m_itemDragging = false;
    m_itemScaling = false;
    m_linkingMode = false;
    m_editingNoteIdx = -1;
}

void BoardPage::LoadItemTexture(BoardItem& item) {
    if (item.texture || item.isLoading || item.loadFailed) return;
    if (item.filePath.empty()) return;

    item.isLoading = true;
    std::wstring path = item.filePath;
    
    m_app->m_imageLoader.LoadAsync(path, [this, path](std::shared_ptr<ImageData> img) {
        // Callback triggers on the main thread inside ProcessPendingTextures
        for (auto& it : m_items) {
            if (it.filePath == path) {
                it.isLoading = false;
                if (img) {
                    it.texture = img;
                    if (img->width > 0 && img->height > 0) {
                        float aspect = static_cast<float>(img->height) / static_cast<float>(img->width);
                        it.height = it.width * aspect;
                    }
                } else {
                    it.loadFailed = true;
                }
                break;
            }
        }
        m_app->m_window.RequestRedraw();
    }, 400, 400); // Decode at max 400x400 for grid performance
}

RectF BoardPage::GetSelectionBoundingBox() const {
    float x1 = 1e9f, y1 = 1e9f, x2 = -1e9f, y2 = -1e9f;
    bool anySelected = false;

    for (const auto& item : m_items) {
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

void BoardPage::DrawCheckerboard(Renderer& renderer, const RectF& rect) {
    if (!m_checkerboardBitmap) {
        constexpr int size = 20;
        constexpr int total = size * 2;
        std::vector<uint8_t> pixels(total * total * 4);

        const uint8_t lightGray = 30; // Dark cyberpunk colors
        const uint8_t darkGray  = 15;

        for (int y = 0; y < total; ++y) {
            for (int x = 0; x < total; ++x) {
                int idx = (y * total + x) * 4;
                bool isLight = ((x / size) + (y / size)) % 2 == 0;
                uint8_t c = isLight ? lightGray : darkGray;
                pixels[idx + 0] = c;
                pixels[idx + 1] = c;
                pixels[idx + 2] = c;
                pixels[idx + 3] = 255;
            }
        }

        m_checkerboardBitmap = renderer.CreateBitmapFromMemory(pixels.data(), total, total, total * 4);
    }

    if (!m_checkerboardBitmap) return;

    renderer.PushClip(rect);
    float tileW = 40.0f, tileH = 40.0f;
    for (float y = rect.top; y < rect.bottom; y += tileH) {
        for (float x = rect.left; x < rect.right; x += tileW) {
            RectF tile = RectF::FromXYWH(x, y, tileW, tileH);
            renderer.DrawBitmap(m_checkerboardBitmap.Get(), tile, 1.0f, BitmapInterpolation::NearestNeighbor);
        }
    }
    renderer.PopClip();
}

void BoardPage::DeleteSelectedItems() {
    // Collect selected indices
    std::vector<int> selectedIndices;
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        if (m_items[i].selected) {
            selectedIndices.push_back(i);
        }
    }

    if (selectedIndices.empty()) return;

    // Erase items from vector (back to front to preserve index values during deletion)
    std::vector<BoardItem> remainingItems;
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        if (m_items[i].selected) {
            if (i == m_editingNoteIdx) m_editingNoteIdx = -1;
            continue;
        }

        // Adjust Link connection indexes when preceding items are deleted
        if (m_items[i].type == BoardItemType::Link) {
            auto& link = m_items[i];
            
            // If from or to item is deleted, delete the link too
            bool fromDeleted = std::find(selectedIndices.begin(), selectedIndices.end(), link.fromItemIdx) != selectedIndices.end();
            bool toDeleted = std::find(selectedIndices.begin(), selectedIndices.end(), link.toItemIdx) != selectedIndices.end();
            if (fromDeleted || toDeleted) {
                continue;
            }

            // Shift indices down
            int shiftFrom = 0;
            int shiftTo = 0;
            for (int delIdx : selectedIndices) {
                if (delIdx < link.fromItemIdx) shiftFrom++;
                if (delIdx < link.toItemIdx) shiftTo++;
            }
            link.fromItemIdx -= shiftFrom;
            link.toItemIdx -= shiftTo;
        }

        remainingItems.push_back(m_items[i]);
    }

    m_items = std::move(remainingItems);
    m_app->m_window.RequestRedraw();
}

void BoardPage::AddNoteAt(const PointF& worldPos, const std::wstring& text) {
    BoardItem note;
    note.type = BoardItemType::Note;
    note.position = worldPos;
    note.plainText = text;
    note.width = 220.0f;
    note.height = 180.0f;
    m_items.push_back(note);
}

void BoardPage::AddNodeAt(const PointF& worldPos, const std::wstring& label) {
    BoardItem node;
    node.type = BoardItemType::Node;
    node.position = worldPos;
    node.label = label;
    UpdateNodeSize(node);
    m_items.push_back(node);
}

void BoardPage::StartLinking(int fromIdx) {
    m_linkingMode = true;
    m_linkingFromIdx = fromIdx;
    m_linkingCurrentMousePos = { m_app->m_mouseX, m_app->m_mouseY };
}

float BoardPage::DistanceToLineSegment(const PointF& p, const PointF& a, const PointF& b) const {
    float l2 = (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y);
    if (l2 == 0.0f) {
        return std::sqrt((p.x - a.x) * (p.x - a.x) + (p.y - a.y) * (p.y - a.y));
    }
    
    // Project p onto segment ab
    float t = ((p.x - a.x) * (b.x - a.x) + (p.y - a.y) * (b.y - a.y)) / l2;
    t = std::clamp(t, 0.0f, 1.0f);
    
    PointF projection = { a.x + t * (b.x - a.x), a.y + t * (b.y - a.y) };
    return std::sqrt((p.x - projection.x) * (p.x - projection.x) + (p.y - projection.y) * (p.y - projection.y));
}

void BoardPage::UpdateNodeSize(BoardItem& item) {
    if (item.type != BoardItemType::Node) return;
    // Estimate width based on label length.
    // Standard font size is 13.f. Average character width is around 8.f pixels.
    float textWidth = static_cast<float>(item.label.size()) * 8.0f;
    item.width = std::max(80.0f, textWidth + 30.0f);
    item.height = 80.0f;
}

} // namespace kv
