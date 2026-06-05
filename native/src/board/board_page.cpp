// ============================================================================
// Class: BoardPage
// Description: BoardPage coordinates the virtual infinite workspace UI page in [IMAGE_VIEWER_APP_NAME].
// Responsibility: Act as main page handler receiving mouse and keyboard inputs, forwarding events to selection, text editor, canvas, and file loaders.
// ============================================================================
#include "common/pch.h"
#include "board_page.h"
#include "board_item_renderer.h"
#include "board_serializer.h"
#include "core/app.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <thread>

namespace kv {

BoardPage::~BoardPage() {
    if (m_scanThread.joinable()) {
        m_scanThread.detach();
    }
}

void BoardPage::Initialize(App* app) {
    m_app = app;
    ResetView();

    AddNoteAt({ 0.0f, -100.0f }, L"Welcome to Board Workspace!\n\n- Drag & Drop images/folders\n- Mouse Wheel to Zoom\n- Middle-click Drag to Pan\n- Left-click Drag to Box Select\n- Double-click notes to edit\n- Right-click to Add Note/Node or Link items");
    
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

void BoardPage::Update(double dt) {
    m_editor.UpdateCursor(dt, [this]() { m_app->m_window.RequestRedraw(); });
    if (m_toastTimeRemaining > 0.0) {
        m_toastTimeRemaining -= dt;
        if (m_toastTimeRemaining <= 0.0) {
            m_toastTimeRemaining = 0.0;
        }
        m_app->m_window.RequestRedraw();
    }
}

void BoardPage::Draw(Renderer& renderer) {
    auto size = renderer.GetTargetSize().ToFloat();

    m_canvas.DrawCheckerboard(renderer, RectF::FromXYWH(0, 0, size.width, size.height));

    renderer.PushTranslation(m_canvas.m_pan.x, m_canvas.m_pan.y);
    renderer.PushScale(m_canvas.m_zoom, m_canvas.m_zoom);

    m_canvas.DrawGrid(renderer, size);

    BoardItemRenderer::DrawLinks(renderer, m_items, m_canvas.m_zoom);

    PointF minWorld = ScreenToWorld({ 0, 0 });
    PointF maxWorld = ScreenToWorld({ size.width, size.height });

    BoardItemRenderer::DrawItems(
        renderer,
        m_items,
        m_editor.GetEditingIndex(),
        m_editor.m_cursorBlinkState,
        m_canvas.m_zoom,
        minWorld,
        maxWorld,
        [this](BoardItem& item) { LoadItemTexture(item); }
    );

    if (m_linkingMode) {
        PointF currentMouseWorld = ScreenToWorld(m_linkingCurrentMousePos);
        BoardItemRenderer::DrawActiveLinkFeedback(renderer, m_items, m_linkingFromIdx, currentMouseWorld, m_canvas.m_zoom);
    }

    renderer.PopTransform(); // Pop scale
    renderer.PopTransform(); // Pop translation

    RectF selBox = m_selection.GetSelectionBoundingBox(m_items);
    if (selBox.Width() > 0.0f) {
        RectF selScreenBox = WorldToScreen(selBox);
        renderer.DrawRect(selScreenBox, Color::FromHex(0xFF9800), 1.0f);

        float handleSize = 8.0f;
        Color handleColor = Color::FromHex(0xFF9800);
        renderer.FillRect(RectF::FromXYWH(selScreenBox.left - handleSize * 0.5f, selScreenBox.top - handleSize * 0.5f, handleSize, handleSize), handleColor);
        renderer.FillRect(RectF::FromXYWH(selScreenBox.right - handleSize * 0.5f, selScreenBox.top - handleSize * 0.5f, handleSize, handleSize), handleColor);
        renderer.FillRect(RectF::FromXYWH(selScreenBox.right - handleSize * 0.5f, selScreenBox.bottom - handleSize * 0.5f, handleSize, handleSize), handleColor);
        renderer.FillRect(RectF::FromXYWH(selScreenBox.left - handleSize * 0.5f, selScreenBox.bottom - handleSize * 0.5f, handleSize, handleSize), handleColor);
    }

    if (m_selection.m_boxSelecting) {
        float x1 = std::min(m_selection.m_boxSelectStart.x, m_selection.m_boxSelectEnd.x);
        float y1 = std::min(m_selection.m_boxSelectStart.y, m_selection.m_boxSelectEnd.y);
        float x2 = std::max(m_selection.m_boxSelectStart.x, m_selection.m_boxSelectEnd.x);
        float y2 = std::max(m_selection.m_boxSelectStart.y, m_selection.m_boxSelectEnd.y);
        RectF selectionRect = RectF::FromXYWH(x1, y1, x2 - x1, y2 - y1);
        renderer.DrawRect(selectionRect, Color::White(0.8f), 1.0f);
        renderer.FillRect(selectionRect, Color::FromRGBA(255, 255, 255, 20));
    }

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

    DrawSearchModal(renderer);
    DrawScanProgressBar(renderer);
    DrawToastNotification(renderer);
}

bool BoardPage::OnMouseMove(float x, float y, KeyModifiers mods) {
    if (m_scanningTags) {
        return true;
    }
    if (m_searchVisible) {
        RectF bounds = GetSearchModalBounds();
        m_hoveredSuggestionIdx = -1;
        m_hoveredSearchButton = 0;
        
        if (bounds.Contains(x, y)) {
            RectF searchBtnRect = RectF::FromXYWH(bounds.right - 285.0f, bounds.bottom - 52.0f, 120.0f, 32.0f);
            if (searchBtnRect.Contains(x, y)) {
                m_hoveredSearchButton = 1;
            }
            
            RectF closeBtnRect = RectF::FromXYWH(bounds.right - 150.0f, bounds.bottom - 52.0f, 120.0f, 32.0f);
            if (closeBtnRect.Contains(x, y)) {
                m_hoveredSearchButton = 2;
            }
            
            RectF suggestionsArea = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 110.0f, bounds.Width() - 60.0f, 180.0f);
            if (suggestionsArea.Contains(x, y)) {
                float sX = suggestionsArea.left + 8.0f;
                float sY = suggestionsArea.top + 8.0f;
                float pillH = 28.0f;
                float spacingX = 6.0f;
                float spacingY = 6.0f;
                
                for (size_t i = 0; i < m_filteredSuggestions.size(); ++i) {
                    std::wstring tagStr = m_filteredSuggestions[i];
                    float textW = m_app->m_renderer.MeasureText(tagStr, 14.0f).x;
                    float pillW = textW + 22.0f;
                    
                    if (sX + pillW > suggestionsArea.right - 8.0f) {
                        sX = suggestionsArea.left + 8.0f;
                        sY += pillH + spacingY;
                    }
                    
                    RectF pillRect = RectF::FromXYWH(sX, sY, pillW, pillH);
                    if (pillRect.Contains(x, y)) {
                        m_hoveredSuggestionIdx = static_cast<int>(i);
                        break;
                    }
                    
                    sX += pillW + spacingX;
                }
            }
        }
        m_app->m_window.RequestRedraw();
        return true;
    }

    m_linkingCurrentMousePos = { x, y };

    if (m_canvas.m_cameraPanning) {
        m_canvas.m_pan.x = m_canvas.m_panStart.x + (x - m_canvas.m_mouseDragStart.x);
        m_canvas.m_pan.y = m_canvas.m_panStart.y + (y - m_canvas.m_mouseDragStart.y);
        m_app->m_window.RequestRedraw();
        return true;
    }

    if (m_selection.m_itemScaling) {
        PointF mouseWorld = ScreenToWorld({ x, y });
        m_selection.UpdateScaling(mouseWorld, m_items, mods.shift);
        m_app->m_window.RequestRedraw();
        return true;
    }

    if (m_selection.m_itemDragging) {
        PointF mouseWorld = ScreenToWorld({ x, y });
        m_selection.UpdateDragging(mouseWorld, m_items);
        m_app->m_window.RequestRedraw();
        return true;
    }

    if (m_selection.m_boxSelecting) {
        PointF startW = ScreenToWorld(m_selection.m_boxSelectStart);
        PointF endW = ScreenToWorld({ x, y });
        m_selection.UpdateBoxSelection({ x, y }, startW, endW, m_items);
        m_app->m_window.RequestRedraw();
        return true;
    }

    return false;
}

bool BoardPage::OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (m_scanningTags) {
        return true;
    }
    if (m_searchVisible) {
        RectF bounds = GetSearchModalBounds();
        if (btn == MouseButton::Left) {
            if (!bounds.Contains(x, y)) {
                m_searchVisible = false;
                m_app->m_window.RequestRedraw();
                return true;
            }
            
            RectF searchBtnRect = RectF::FromXYWH(bounds.right - 285.0f, bounds.bottom - 52.0f, 120.0f, 32.0f);
            if (searchBtnRect.Contains(x, y)) {
                PerformTagQueryAndInsert();
                m_searchVisible = false;
                m_app->m_window.RequestRedraw();
                return true;
            }
            
            RectF closeBtnRect = RectF::FromXYWH(bounds.right - 150.0f, bounds.bottom - 52.0f, 120.0f, 32.0f);
            if (closeBtnRect.Contains(x, y)) {
                m_searchVisible = false;
                m_app->m_window.RequestRedraw();
                return true;
            }
            
            RectF suggestionsArea = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 110.0f, bounds.Width() - 60.0f, 180.0f);
            if (suggestionsArea.Contains(x, y)) {
                float sX = suggestionsArea.left + 8.0f;
                float sY = suggestionsArea.top + 8.0f;
                float pillH = 28.0f;
                float spacingX = 6.0f;
                float spacingY = 6.0f;
                
                for (size_t i = 0; i < m_filteredSuggestions.size(); ++i) {
                    std::wstring tagStr = m_filteredSuggestions[i];
                    float textW = m_app->m_renderer.MeasureText(tagStr, 14.0f).x;
                    float pillW = textW + 22.0f;
                    
                    if (sX + pillW > suggestionsArea.right - 8.0f) {
                        sX = suggestionsArea.left + 8.0f;
                        sY += pillH + spacingY;
                    }
                    
                    RectF pillRect = RectF::FromXYWH(sX, sY, pillW, pillH);
                    if (pillRect.Contains(x, y)) {
                        SelectSuggestion(tagStr);
                        m_app->m_window.RequestRedraw();
                        return true;
                    }
                    
                    sX += pillW + spacingX;
                }
            }
        }
        return true;
    }

    PointF worldPos = ScreenToWorld({ x, y });

    if (btn == MouseButton::Middle) {
        m_canvas.m_cameraPanning = true;
        m_canvas.m_mouseDragStart = { x, y };
        m_canvas.m_panStart = m_canvas.m_pan;
        return true;
    }

    if (btn == MouseButton::Left) {
        if (m_editor.IsEditing()) {
            RectF editBox = m_items[m_editor.GetEditingIndex()].GetWorldRect();
            if (!editBox.Contains(worldPos.x, worldPos.y)) {
                m_editor.StopEditing();
                m_app->m_window.RequestRedraw();
            }
        }

        RectF selBox = m_selection.GetSelectionBoundingBox(m_items);
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
                    m_selection.StartScaling(hIdx, selBox, m_items);
                    return true;
                }
            }
        }

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
            float minDistance = 8.0f / m_canvas.m_zoom;
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
            auto& clickedItem = m_items[clickedIdx];

            if (mods.ctrl) {
                StartLinking(clickedIdx);
                return true;
            }

            if (!clickedItem.selected) {
                if (!mods.shift) {
                    for (auto& item : m_items) item.selected = false;
                }
                clickedItem.selected = true;
            } else if (mods.shift) {
                clickedItem.selected = false;
            }

            m_selection.StartDragging(worldPos, m_items);
            m_app->m_window.RequestRedraw();
            return true;
        } else {
            if (!mods.shift) {
                for (auto& item : m_items) item.selected = false;
            }
            m_selection.StartBoxSelection({ x, y });
            m_app->m_window.RequestRedraw();
            return true;
        }
    }

    return false;
}

bool BoardPage::OnMouseUp(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (m_scanningTags) {
        return true;
    }
    if (m_searchVisible) {
        return true;
    }
    if (btn == MouseButton::Middle) {
        m_canvas.m_cameraPanning = false;
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
                float minDistance = 8.0f / m_canvas.m_zoom;
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
                AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
                AppendMenuW(hMenu, MF_STRING, 6, L"Settings...");
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
            } else if (selected == 6) {
                m_app->m_optionsMenu.Show();
            }
        }
        return true;
    }

    if (btn == MouseButton::Left) {
        if (m_selection.m_itemScaling) {
            m_selection.m_itemScaling = false;
            m_selection.m_scaleHandleIdx = -1;
            return true;
        }
        if (m_selection.m_itemDragging) {
            m_selection.m_itemDragging = false;
            return true;
        }
        if (m_selection.m_boxSelecting) {
            m_selection.m_boxSelecting = false;
            m_app->m_window.RequestRedraw();
            return true;
        }
        if (m_linkingMode) {
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
    if (m_scanningTags) {
        return true;
    }
    if (m_searchVisible) {
        return true;
    }
    float factor = (delta > 0) ? 1.07f : (1.0f / 1.07f);
    float oldZoom = m_canvas.m_zoom;
    
    m_canvas.m_zoom = m_canvas.m_zoom * factor;
    m_canvas.m_zoom = std::clamp(m_canvas.m_zoom, 0.05f, 50.0f);
    
    float actualFactor = m_canvas.m_zoom / oldZoom;
    
    m_canvas.m_pan.x = x - (x - m_canvas.m_pan.x) * actualFactor;
    m_canvas.m_pan.y = y - (y - m_canvas.m_pan.y) * actualFactor;

    m_app->m_window.RequestRedraw();
    return true;
}

bool BoardPage::OnMouseDoubleClick(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (m_scanningTags) {
        return true;
    }
    if (m_searchVisible) {
        return true;
    }
    if (btn != MouseButton::Left) return false;

    PointF worldPos = ScreenToWorld({ x, y });

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

    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        if ((m_items[i].type == BoardItemType::Note || m_items[i].type == BoardItemType::Node) && 
            m_items[i].GetWorldRect().Contains(worldPos.x, worldPos.y)) {
            m_editor.StartEditing(i);
            m_app->m_window.RequestRedraw();
            return true;
        }
    }

    ResetView();
    m_app->m_window.RequestRedraw();
    return true;
}

bool BoardPage::OnKeyDown(uint32_t vk, KeyModifiers mods) {
    if (m_scanningTags) {
        if (vk == VK_ESCAPE) {
            return false;
        }
        return true;
    }

    if (vk == 'F' && mods.ctrl) {
        if (!m_editor.IsEditing()) {
            m_searchVisible = !m_searchVisible;
            if (m_searchVisible) {
                LoadSuggestionsFromDirectory(GetActiveDirectory());
                m_searchText.clear();
                m_filteredSuggestions = m_suggestions;
                m_hoveredSuggestionIdx = -1;
                m_hoveredSearchButton = 0;
            }
            m_app->m_window.RequestRedraw();
            return true;
        }
    }

    if (vk == 'L' && mods.ctrl) {
        if (!m_editor.IsEditing()) {
            std::filesystem::path activeDir = GetActiveDirectory();
            std::filesystem::path jsonPath = activeDir / "xmpTags.json";
            if (std::filesystem::exists(jsonPath)) {
                m_toastMessage = L"Tags file already exists";
                m_toastTimeRemaining = 2.5;
                m_app->m_window.RequestRedraw();
            } else {
                StartDirectoryTagScan(activeDir);
            }
            return true;
        }
    }

    if (m_searchVisible) {
        if (vk == VK_ESCAPE) {
            m_searchVisible = false;
            m_app->m_window.RequestRedraw();
            return true;
        }
        if (vk == VK_RETURN) {
            PerformTagQueryAndInsert();
            m_searchVisible = false;
            m_app->m_window.RequestRedraw();
            return true;
        }
        return true;
    }

    if (m_editor.HandleKeyDown(vk, [this]() { m_app->m_window.RequestRedraw(); })) {
        return true;
    }

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
    if (m_scanningTags) {
        return;
    }
    if (m_searchVisible) {
        if (ch == L'\r' || ch == L'\n') {
            return;
        }
        if (ch == L'\b') {
            if (!m_searchText.empty()) {
                m_searchText.pop_back();
                UpdateFilteredSuggestions();
                m_app->m_window.RequestRedraw();
            }
            return;
        }
        if (ch == 27) {
            return;
        }
        if (ch >= 32) {
            m_searchText += ch;
            UpdateFilteredSuggestions();
            m_app->m_window.RequestRedraw();
        }
        return;
    }
    m_editor.HandleChar(ch, m_items, [this]() { m_app->m_window.RequestRedraw(); });
}

void BoardPage::OnFileDrop(const std::wstring& path, float screenX, float screenY) {
    if (m_scanningTags || m_searchVisible) {
        return;
    }
    PointF worldPos = ScreenToWorld({ screenX, screenY });
    namespace fs = std::filesystem;

    if (!fs::exists(path)) return;

    if (fs::is_directory(path)) {
        BoardItem folderItem;
        folderItem.type = BoardItemType::Folder;
        folderItem.position = worldPos;
        folderItem.filePath = path;
        folderItem.label = fs::path(path).filename().wstring();
        folderItem.width = 300.0f;
        folderItem.height = 200.0f;
        m_items.push_back(folderItem);
    } else if (ImageLoader::IsSupported(path)) {
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

bool BoardPage::SaveBoard(const std::wstring& filePath) {
    return BoardSerializer::SaveBoard(filePath, m_canvas.m_zoom, m_canvas.m_pan, m_items);
}

bool BoardPage::LoadBoard(const std::wstring& filePath) {
    return BoardSerializer::LoadBoard(
        filePath,
        m_canvas.m_zoom,
        m_canvas.m_pan,
        m_items,
        [this](BoardItem& item) { LoadItemTexture(item); }
    );
}

PointF BoardPage::ScreenToWorld(const PointF& screenPt) const {
    return m_canvas.ScreenToWorld(screenPt);
}

PointF BoardPage::WorldToScreen(const PointF& worldPt) const {
    return m_canvas.WorldToScreen(worldPt);
}

RectF BoardPage::WorldToScreen(const RectF& worldRect) const {
    return m_canvas.WorldToScreen(worldRect);
}

void BoardPage::ResetView() {
    m_canvas.ResetView();
    m_selection.m_itemScaling = false;
    m_selection.m_itemDragging = false;
    m_selection.m_boxSelecting = false;
    m_linkingMode = false;
    m_linkingFromIdx = -1;
    m_editor.StopEditing();
}

void BoardPage::LoadItemTexture(BoardItem& item) {
    if (item.texture || item.isLoading || item.loadFailed) return;
    if (item.filePath.empty()) return;

    item.isLoading = true;
    std::wstring path = item.filePath;
    
    m_app->m_imageLoader.LoadAsync(path, [this, path](std::shared_ptr<ImageData> img) {
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
    }, 400, 400);
}

void BoardPage::DeleteSelectedItems() {
    std::vector<int> selectedIndices;
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        if (m_items[i].selected) {
            selectedIndices.push_back(i);
        }
    }

    if (selectedIndices.empty()) return;

    std::vector<BoardItem> remainingItems;
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        if (m_items[i].selected) {
            if (i == m_editor.GetEditingIndex()) m_editor.StopEditing();
            continue;
        }

        if (m_items[i].type == BoardItemType::Link) {
            auto& link = m_items[i];
            
            bool fromDeleted = std::find(selectedIndices.begin(), selectedIndices.end(), link.fromItemIdx) != selectedIndices.end();
            bool toDeleted = std::find(selectedIndices.begin(), selectedIndices.end(), link.toItemIdx) != selectedIndices.end();
            if (fromDeleted || toDeleted) {
                continue;
            }

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
    BoardTextEditor::UpdateNodeSize(node);
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
    
    float t = ((p.x - a.x) * (b.x - a.x) + (p.y - a.y) * (b.y - a.y)) / l2;
    t = std::clamp(t, 0.0f, 1.0f);
    
    PointF projection = { a.x + t * (b.x - a.x), a.y + t * (b.y - a.y) };
    return std::sqrt((p.x - projection.x) * (p.x - projection.x) + (p.y - projection.y) * (p.y - projection.y));
}

std::filesystem::path BoardPage::GetActiveDirectory() const {
    if (!m_app->m_fileList.empty()) {
        return std::filesystem::path(m_app->m_fileList[0]).parent_path();
    }
    int folderIdx = m_app->m_libraryData.GetCurrentFolderIndex();
    const auto& folders = m_app->m_libraryData.GetFolders();
    if (folderIdx >= 0 && folderIdx < static_cast<int>(folders.size())) {
        return folders[folderIdx].folderPath;
    }
    return std::filesystem::current_path();
}

RectF BoardPage::GetSearchModalBounds() const {
    auto screenSize = m_app->m_renderer.GetTargetSize().ToFloat();
    float boxW = 600.0f;
    float boxH = 360.0f;
    return RectF::FromXYWH(
        (screenSize.width - boxW) / 2.0f,
        (screenSize.height - boxH) / 2.0f,
        boxW,
        boxH
    );
}

static std::wstring GetCurrentTypingTag(const std::wstring& text, size_t& outLastCommaPos) {
    size_t pos = text.find_last_of(L',');
    if (pos == std::wstring::npos) {
        outLastCommaPos = std::wstring::npos;
        std::wstring sub = text;
        size_t start = sub.find_first_not_of(L" \t");
        if (start != std::wstring::npos) sub = sub.substr(start);
        return sub;
    } else {
        outLastCommaPos = pos;
        std::wstring sub = text.substr(pos + 1);
        size_t start = sub.find_first_not_of(L" \t");
        if (start != std::wstring::npos) sub = sub.substr(start);
        return sub;
    }
}

void BoardPage::UpdateFilteredSuggestions() {
    m_filteredSuggestions.clear();
    size_t lastComma = std::wstring::npos;
    std::wstring typing = GetCurrentTypingTag(m_searchText, lastComma);
    
    bool isExcluding = false;
    if (!typing.empty() && typing[0] == L'-') {
        isExcluding = true;
        typing = typing.substr(1);
    }
    
    size_t last = typing.find_last_not_of(L" \t");
    if (last != std::wstring::npos) {
        typing = typing.substr(0, last + 1);
    }
    
    if (typing.empty()) {
        m_filteredSuggestions = m_suggestions;
    } else {
        std::wstring typingLower = typing;
        std::transform(typingLower.begin(), typingLower.end(), typingLower.begin(), ::towlower);
        for (const auto& tag : m_suggestions) {
            std::wstring tagLower = tag;
            std::transform(tagLower.begin(), tagLower.end(), tagLower.begin(), ::towlower);
            if (tagLower.find(typingLower) == 0) {
                m_filteredSuggestions.push_back(tag);
            }
        }
    }
}

void BoardPage::SelectSuggestion(const std::wstring& suggestion) {
    size_t lastComma = std::wstring::npos;
    std::wstring typing = GetCurrentTypingTag(m_searchText, lastComma);
    bool isExcluding = (!typing.empty() && typing[0] == L'-');
    
    std::wstring prefix = L"";
    if (lastComma != std::wstring::npos) {
        prefix = m_searchText.substr(0, lastComma + 1) + L" ";
    }
    
    if (isExcluding) {
        m_searchText = prefix + L"-" + suggestion + L", ";
    } else {
        m_searchText = prefix + suggestion + L", ";
    }
    
    UpdateFilteredSuggestions();
    m_hoveredSuggestionIdx = -1;
}

void BoardPage::DrawSearchModal(Renderer& renderer) {
    if (!m_searchVisible) return;
    
    auto screenSize = renderer.GetTargetSize().ToFloat();
    RectF bounds = GetSearchModalBounds();
    
    renderer.FillRect(RectF::FromXYWH(0, 0, screenSize.width, screenSize.height), Color::Black(0.65f));
    
    Color panelBg = Color::FromRGBA(15, 15, 20, 245);
    Color panelBorder = Color::FromRGBA(0, 240, 255, 60);
    renderer.FillRoundedRect(bounds, 8.0f, 8.0f, panelBg);
    renderer.DrawRoundedRect(bounds, 8.0f, 8.0f, panelBorder, 1.5f);
    
    RectF titleRect = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 20.0f, bounds.Width() - 60.0f, 30.0f);
    renderer.DrawText(
        L"Buscar por etiquetas",
        titleRect,
        Color::White(),
        22.0f,
        TextAlign::Left,
        TextVAlign::Center
    );
    
    RectF searchRect = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 60.0f, bounds.Width() - 60.0f, 36.0f);
    Color searchBorder = Color::FromRGBA(0, 240, 255);
    renderer.FillRoundedRect(searchRect, 4.0f, 4.0f, Color::FromRGBA(20, 20, 25, 200));
    renderer.DrawRoundedRect(searchRect, 4.0f, 4.0f, searchBorder, 1.0f);
    
    if (m_searchText.empty()) {
        renderer.DrawText(
            L"Etiquetas separadas por comas (ej. tag1, -tag2)...",
            searchRect.Inset(8.0f),
            Color::FromRGBA(120, 120, 120),
            15.0f,
            TextAlign::Left,
            TextVAlign::Center
        );
    } else {
        renderer.DrawText(
            m_searchText,
            searchRect.Inset(8.0f),
            Color::White(),
            15.0f,
            TextAlign::Left,
            TextVAlign::Center
        );
    }
    
    static auto lastCaretSearch = std::chrono::steady_clock::now();
    static bool caretSearchVisible = true;
    if (std::chrono::steady_clock::now() - lastCaretSearch > std::chrono::milliseconds(500)) {
        caretSearchVisible = !caretSearchVisible;
        lastCaretSearch = std::chrono::steady_clock::now();
    }
    if (caretSearchVisible) {
        float textW = renderer.MeasureText(m_searchText, 15.0f).x;
        float caretX = searchRect.left + 8.0f + textW;
        renderer.DrawLine(
            { caretX, searchRect.top + 6.0f },
            { caretX, searchRect.bottom - 6.0f },
            Color::FromRGBA(0, 240, 255),
            1.5f
        );
    }
    
    RectF suggestionsArea = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 110.0f, bounds.Width() - 60.0f, 180.0f);
    renderer.FillRoundedRect(suggestionsArea, 4.0f, 4.0f, Color::FromRGBA(20, 20, 25, 150));
    renderer.DrawRoundedRect(suggestionsArea, 4.0f, 4.0f, Color::FromRGBA(255, 255, 255, 15), 1.0f);
    
    renderer.PushClip(suggestionsArea);
    
    float sX = suggestionsArea.left + 8.0f;
    float sY = suggestionsArea.top + 8.0f;
    float pillH = 28.0f;
    float spacingX = 6.0f;
    float spacingY = 6.0f;
    
    for (size_t i = 0; i < m_filteredSuggestions.size(); ++i) {
        std::wstring tagStr = m_filteredSuggestions[i];
        float textW = renderer.MeasureText(tagStr, 14.0f).x;
        float pillW = textW + 22.0f;
        
        if (sX + pillW > suggestionsArea.right - 8.0f) {
            sX = suggestionsArea.left + 8.0f;
            sY += pillH + spacingY;
        }
        
        RectF pillRect = RectF::FromXYWH(sX, sY, pillW, pillH);
        
        if (pillRect.bottom > suggestionsArea.top && pillRect.top < suggestionsArea.bottom) {
            bool hovered = (m_hoveredSuggestionIdx == static_cast<int>(i));
            Color pillBg = hovered ? Color::FromRGBA(255, 255, 255, 15) : Color::FromRGBA(255, 255, 255, 10);
            Color pillBorder = hovered ? Color::FromRGBA(255, 255, 255, 35) : Color::FromRGBA(255, 255, 255, 20);
            
            renderer.FillRoundedRect(pillRect, 4.0f, 4.0f, pillBg);
            renderer.DrawRoundedRect(pillRect, 4.0f, 4.0f, pillBorder, 1.0f);
            
            RectF textRect = RectF::FromXYWH(pillRect.left + 11.0f, pillRect.top, textW, pillRect.Height());
            renderer.DrawText(tagStr, textRect, Color::White(0.9f), 14.0f, TextAlign::Left, TextVAlign::Center);
        }
        
        sX += pillW + spacingX;
    }
    
    renderer.PopClip();
    
    RectF closeRect = RectF::FromXYWH(bounds.right - 150.0f, bounds.bottom - 52.0f, 120.0f, 32.0f);
    RectF searchBtnRect = RectF::FromXYWH(bounds.right - 285.0f, bounds.bottom - 52.0f, 120.0f, 32.0f);
    
    Color closeBg = (m_hoveredSearchButton == 2) ? Color::FromRGBA(60, 60, 65) : Color::FromRGBA(35, 35, 40);
    renderer.FillRoundedRect(closeRect, 4.0f, 4.0f, closeBg);
    renderer.DrawRoundedRect(closeRect, 4.0f, 4.0f, Color::White(0.15f), 1.0f);
    renderer.DrawText(L"Cerrar", closeRect, Color::White(0.9f), 15.0f, TextAlign::Center, TextVAlign::Center);
    
    Color searchBg = (m_hoveredSearchButton == 1) ? Color::FromRGBA(255, 55, 95) : Color::FromRGBA(220, 40, 75);
    renderer.FillRoundedRect(searchBtnRect, 4.0f, 4.0f, searchBg);
    renderer.DrawRoundedRect(searchBtnRect, 4.0f, 4.0f, Color::White(0.2f), 1.0f);
    renderer.DrawText(L"Buscar", searchBtnRect, Color::White(), 15.0f, TextAlign::Center, TextVAlign::Center);
}

void BoardPage::DrawScanProgressBar(Renderer& renderer) {
    if (!m_scanningTags) return;
    
    auto screenSize = renderer.GetTargetSize().ToFloat();
    
    float barW = 400.0f;
    float barH = 30.0f;
    float barX = (screenSize.width - barW) * 0.5f;
    float barY = 20.0f;
    
    RectF barRect = RectF::FromXYWH(barX, barY, barW, barH);
    
    renderer.FillRoundedRect(barRect, 6.0f, 6.0f, Color::FromRGBA(15, 15, 20, 240));
    renderer.DrawRoundedRect(barRect, 6.0f, 6.0f, Color::FromRGBA(0, 240, 255, 60), 1.5f);
    
    size_t total = m_scanTotal;
    size_t current = m_scanCurrent.load();
    float pct = (total > 0) ? (static_cast<float>(current) / static_cast<float>(total)) : 0.0f;
    pct = std::clamp(pct, 0.0f, 1.0f);
    
    RectF fillRect = RectF::FromXYWH(barX + 3.0f, barY + 3.0f, (barW - 6.0f) * pct, barH - 6.0f);
    if (fillRect.Width() > 0.0f) {
        renderer.FillRoundedRect(fillRect, 4.0f, 4.0f, Color::FromRGBA(0, 240, 255, 120));
    }
    
    std::wstring text = std::format(L"Escaneando directorio: {} / {} ({}%)", current, total, static_cast<int>(pct * 100.0f));
    renderer.DrawText(text, barRect, Color::White(), 12.0f, TextAlign::Center, TextVAlign::Center);
}

void BoardPage::DrawToastNotification(Renderer& renderer) {
    if (m_toastTimeRemaining <= 0.0 || m_toastMessage.empty()) return;
    
    auto screenSize = renderer.GetTargetSize().ToFloat();
    
    float textW = renderer.MeasureText(m_toastMessage, 13.0f).x;
    float toastW = textW + 30.0f;
    float toastH = 40.0f;
    float toastX = screenSize.width - toastW - 20.0f;
    float toastY = 20.0f;
    
    RectF toastRect = RectF::FromXYWH(toastX, toastY, toastW, toastH);
    
    float alpha = 1.0f;
    if (m_toastTimeRemaining < 0.5) {
        alpha = static_cast<float>(m_toastTimeRemaining / 0.5);
    }
    
    Color borderCol = Color::FromRGBA(255, 152, 0, static_cast<uint8_t>(alpha * 255.0f * 0.4f));
    Color bgCol = Color::FromRGBA(20, 20, 25, static_cast<uint8_t>(alpha * 255.0f * 0.95f));
    Color textCol = Color::FromRGBA(255, 255, 255, static_cast<uint8_t>(alpha * 255.0f * 0.9f));
    
    renderer.FillRoundedRect(toastRect, 6.0f, 6.0f, bgCol);
    renderer.DrawRoundedRect(toastRect, 6.0f, 6.0f, borderCol, 1.5f);
    renderer.DrawText(m_toastMessage, toastRect, textCol, 13.0f, TextAlign::Center, TextVAlign::Center);
}

void BoardPage::LoadSuggestionsFromDirectory(const std::filesystem::path& activeDir) {
    m_suggestions.clear();
    m_filteredSuggestions.clear();
    
    std::filesystem::path jsonPath = activeDir / "xmpTags.json";
    if (!std::filesystem::exists(jsonPath)) {
        return;
    }
    
    std::ifstream file(jsonPath, std::ios::binary);
    if (!file) return;
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    std::wstring wcontent;
    int len = MultiByteToWideChar(CP_UTF8, 0, content.c_str(), static_cast<int>(content.size()), nullptr, 0);
    if (len > 0) {
        wcontent.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, content.c_str(), static_cast<int>(content.size()), &wcontent[0], len);
    } else {
        return;
    }
    
    int arrayDepth = 0;
    size_t i = 0;
    while (i < wcontent.size()) {
        wchar_t ch = wcontent[i];
        if (ch == L'[') {
            arrayDepth++;
            i++;
        } else if (ch == L']') {
            if (arrayDepth > 0) arrayDepth--;
            i++;
        } else if (ch == L'"') {
            std::wstring str = L"";
            i++;
            while (i < wcontent.size() && wcontent[i] != L'"') {
                if (wcontent[i] == L'\\' && i + 1 < wcontent.size()) {
                    str += wcontent[i + 1];
                    i += 2;
                } else {
                    str += wcontent[i];
                    i++;
                }
            }
            if (i < wcontent.size()) i++;
            
            if (arrayDepth > 0 && !str.empty()) {
                if (std::find(m_suggestions.begin(), m_suggestions.end(), str) == m_suggestions.end()) {
                    m_suggestions.push_back(str);
                }
            }
        } else {
            i++;
        }
    }
    
    std::sort(m_suggestions.begin(), m_suggestions.end());
    m_filteredSuggestions = m_suggestions;
}

void BoardPage::StartDirectoryTagScan(const std::filesystem::path& activeDir) {
    m_scanningTags = true;
    m_scanCurrent = 0;
    m_scanTotal = 0;
    m_scanDir = activeDir.wstring();
    
    if (m_scanThread.joinable()) {
        m_scanThread.join();
    }
    
    m_scanThread = std::thread([this, activeDir]() {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        bool comInitialized = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
        
        std::vector<std::filesystem::path> files;
        try {
            for (const auto& entry : std::filesystem::directory_iterator(activeDir)) {
                if (entry.is_regular_file() && ImageLoader::IsSupported(entry.path().wstring())) {
                    files.push_back(entry.path());
                }
            }
        } catch (...) {}
        
        m_scanTotal = files.size();
        m_scanCurrent = 0;
        
        std::set<std::wstring> uniqueTags;
        
        for (const auto& filePath : files) {
            std::vector<std::wstring> fileTags;
            if (m_app->m_tagManager.ReadTags(filePath.wstring(), fileTags)) {
                for (const auto& tag : fileTags) {
                    if (!tag.empty()) {
                        uniqueTags.insert(tag);
                    }
                }
            }
            m_scanCurrent++;
            m_app->m_window.RequestRedraw();
        }
        
        if (!files.empty()) {
            std::filesystem::path jsonPath = activeDir / "xmpTags.json";
            
            std::wstring jsonW = L"[\n";
            bool first = true;
            for (const auto& tag : uniqueTags) {
                if (!first) {
                    jsonW += L",\n";
                }
                first = false;
                
                std::wstring escaped = L"";
                for (wchar_t ch : tag) {
                    if (ch == L'\\' || ch == L'"') {
                        escaped += L'\\';
                    }
                    escaped += ch;
                }
                jsonW += L"  \"" + escaped + L"\"";
            }
            jsonW += L"\n]\n";
            
            std::string jsonUtf8;
            int lenLen = WideCharToMultiByte(CP_UTF8, 0, jsonW.c_str(), static_cast<int>(jsonW.size()), nullptr, 0, nullptr, nullptr);
            if (lenLen > 0) {
                jsonUtf8.resize(lenLen);
                WideCharToMultiByte(CP_UTF8, 0, jsonW.c_str(), static_cast<int>(jsonW.size()), &jsonUtf8[0], lenLen, nullptr, nullptr);
            }
            
            std::ofstream file(jsonPath, std::ios::binary);
            if (file) {
                file.write(jsonUtf8.data(), jsonUtf8.size());
                file.close();
            }
        }
        
        m_app->m_tagManager.SaveCache();
        m_scanningTags = false;
        m_app->m_window.RequestRedraw();
        
        if (comInitialized) {
            CoUninitialize();
        }
    });
}

void BoardPage::PerformTagQueryAndInsert() {
    std::vector<std::wstring> posTags;
    std::vector<std::wstring> negTags;
    
    std::wstring term = L"";
    for (wchar_t ch : m_searchText) {
        if (ch == L',') {
            size_t start = term.find_first_not_of(L" \t");
            if (start != std::wstring::npos) {
                term = term.substr(start);
                size_t end = term.find_last_not_of(L" \t");
                if (end != std::wstring::npos) {
                    term = term.substr(0, end + 1);
                }
                if (!term.empty()) {
                    if (term[0] == L'-') {
                        std::wstring sub = term.substr(1);
                        if (!sub.empty()) negTags.push_back(sub);
                    } else {
                        posTags.push_back(term);
                    }
                }
            }
            term = L"";
        } else {
            term += ch;
        }
    }
    
    size_t start = term.find_first_not_of(L" \t");
    if (start != std::wstring::npos) {
        term = term.substr(start);
        size_t end = term.find_last_not_of(L" \t");
        if (end != std::wstring::npos) {
            term = term.substr(0, end + 1);
        }
        if (!term.empty()) {
            if (term[0] == L'-') {
                std::wstring sub = term.substr(1);
                if (!sub.empty()) negTags.push_back(sub);
            } else {
                posTags.push_back(term);
            }
        }
    }
    
    if (posTags.empty() && negTags.empty()) {
        return;
    }
    
    std::filesystem::path activeDir = GetActiveDirectory();
    std::vector<std::filesystem::path> matchingFiles;
    
    try {
        if (std::filesystem::exists(activeDir) && std::filesystem::is_directory(activeDir)) {
            for (const auto& entry : std::filesystem::directory_iterator(activeDir)) {
                if (entry.is_regular_file() && ImageLoader::IsSupported(entry.path().wstring())) {
                    std::wstring filePath = entry.path().wstring();
                    std::vector<std::wstring> fileTags;
                    if (m_app->m_tagManager.ReadTags(filePath, fileTags)) {
                        bool matchesPos = true;
                        for (const auto& posTag : posTags) {
                            bool tagFound = false;
                            for (const auto& ft : fileTags) {
                                if (_wcsicmp(posTag.c_str(), ft.c_str()) == 0) {
                                    tagFound = true;
                                    break;
                                }
                            }
                            if (!tagFound) {
                                matchesPos = false;
                                break;
                            }
                        }
                        
                        if (!matchesPos) continue;
                        
                        bool matchesNeg = true;
                        for (const auto& negTag : negTags) {
                            for (const auto& ft : fileTags) {
                                if (_wcsicmp(negTag.c_str(), ft.c_str()) == 0) {
                                    matchesNeg = false;
                                    break;
                                }
                            }
                            if (!matchesNeg) break;
                        }
                        
                        if (matchesPos && matchesNeg) {
                            matchingFiles.push_back(entry.path());
                        }
                    }
                }
            }
        }
    } catch (...) {}
    
    if (matchingFiles.empty()) {
        return;
    }
    
    std::vector<std::filesystem::path> newFiles;
    for (const auto& path : matchingFiles) {
        bool alreadyExists = false;
        std::wstring pathStr = path.wstring();
        for (const auto& item : m_items) {
            if (item.type == BoardItemType::Image) {
                if (_wcsicmp(item.filePath.c_str(), pathStr.c_str()) == 0) {
                    alreadyExists = true;
                    break;
                }
            }
        }
        if (!alreadyExists) {
            newFiles.push_back(path);
        }
    }
    
    if (newFiles.empty()) {
        return;
    }
    
    auto targetSize = m_app->m_renderer.GetTargetSize().ToFloat();
    PointF screenCenter = { targetSize.width * 0.5f, targetSize.height * 0.5f };
    PointF worldCenter = ScreenToWorld(screenCenter);
    
    int columns = 3;
    float spacingX = 40.0f;
    float spacingY = 40.0f;
    float itemW = 400.0f;
    float itemH = 300.0f;
    
    int count = static_cast<int>(newFiles.size());
    int gridCols = std::min(count, columns);
    int gridRows = (count + columns - 1) / columns;
    
    float gridWidth = gridCols * itemW + (gridCols - 1) * spacingX;
    float gridHeight = gridRows * itemH + (gridRows - 1) * spacingY;
    
    float startX = worldCenter.x - gridWidth * 0.5f;
    float startY = worldCenter.y - gridHeight * 0.5f;
    
    for (int i = 0; i < count; ++i) {
        int row = i / columns;
        int col = i % columns;
        
        float cellLeft = startX + col * (itemW + spacingX);
        float cellTop = startY + row * (itemH + spacingY);
        float centerX = cellLeft + itemW * 0.5f;
        float centerY = cellTop + itemH * 0.5f;
        
        BoardItem imgItem;
        imgItem.type = BoardItemType::Image;
        imgItem.position = { centerX, centerY };
        imgItem.filePath = newFiles[i].wstring();
        imgItem.label = newFiles[i].filename().wstring();
        imgItem.width = itemW;
        imgItem.height = itemH;
        
        m_items.push_back(imgItem);
        LoadItemTexture(m_items.back());
    }
    
    m_app->m_window.RequestRedraw();
}

} // namespace kv
