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

namespace kv {

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
}

bool BoardPage::OnMouseMove(float x, float y, KeyModifiers mods) {
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
    m_editor.HandleChar(ch, m_items, [this]() { m_app->m_window.RequestRedraw(); });
}

void BoardPage::OnFileDrop(const std::wstring& path, float screenX, float screenY) {
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

} // namespace kv
