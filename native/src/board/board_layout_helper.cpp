#include "common/pch.h"
#include "board_layout_helper.h"
#include "board_page.h"
#include "core/app.h"
#include "loader/image_loader.h"
#include <random>
#include <algorithm>

namespace kv {

BoardLayoutHelper::BoardLayoutHelper(BoardPage* page) : m_page(page) {}

PointF BoardLayoutHelper::FindSpiralPosition(const PointF& center, float width, float height, float margin, 
                                             const std::vector<BoardItem>& extraItems, bool ignoreCanvasItems) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distAngle(0.0f, 2.0f * 3.14159265f);

    bool hasElements = false;
    if (!ignoreCanvasItems) {
        for (const auto& item : m_page->m_items) {
            if (item.type != BoardItemType::Link) {
                hasElements = true;
                break;
            }
        }
    }
    if (!hasElements) {
        for (const auto& item : extraItems) {
            if (item.type != BoardItemType::Link) {
                hasElements = true;
                break;
            }
        }
    }
    if (!hasElements) {
        return center;
    }

    float candidateW = width;
    float candidateH = height;

    float theta = distAngle(gen);
    float r = 0.0f;
    float pitch = 200.0f; 
    float thetaStep = 0.05f; 

    int iterations = 0;
    while (iterations < 10000) {
        float cx = center.x + r * std::cos(theta);
        float cy = center.y + r * std::sin(theta);

        RectF candRect = RectF::Centered(cx, cy, candidateW + margin, candidateH + margin);

        bool collision = false;
        if (!ignoreCanvasItems) {
            for (const auto& item : m_page->m_items) {
                if (item.type != BoardItemType::Link) {
                    RectF otherRect = item.GetWorldRect();
                    if (candRect.Intersects(otherRect)) {
                        collision = true;
                        break;
                    }
                }
            }
        }

        if (!collision) {
            for (const auto& item : extraItems) {
                if (item.type != BoardItemType::Link) {
                    RectF otherRect = item.GetWorldRect();
                    if (candRect.Intersects(otherRect)) {
                        collision = true;
                        break;
                    }
                }
            }
        }

        if (!collision) {
            return { cx, cy };
        }

        theta += thetaStep;
        r = (pitch / (2.0f * 3.14159265f)) * theta;
        iterations++;
    }

    std::uniform_real_distribution<float> distFall(-500.0f, 500.0f);
    return { center.x + distFall(gen), center.y + distFall(gen) };
}

float BoardLayoutHelper::DistanceToLineSegment(const PointF& p, const PointF& a, const PointF& b) const {
    float l2 = (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y);
    if (l2 == 0.0f) {
        return std::sqrt((p.x - a.x) * (p.x - a.x) + (p.y - a.y) * (p.y - a.y));
    }
    
    float t = ((p.x - a.x) * (b.x - a.x) + (p.y - a.y) * (b.y - a.y)) / l2;
    t = std::clamp(t, 0.0f, 1.0f);
    
    PointF projection = { a.x + t * (b.x - a.x), a.y + t * (b.y - a.y) };
    return std::sqrt((p.x - projection.x) * (p.x - projection.x) + (p.y - projection.y) * (p.y - projection.y));
}

void BoardLayoutHelper::ListCurrentDirectoryImages() {
    std::filesystem::path activeDir = m_page->GetActiveDirectory();
    std::vector<std::filesystem::path> files;
    try {
        if (std::filesystem::exists(activeDir) && std::filesystem::is_directory(activeDir)) {
            for (const auto& entry : std::filesystem::directory_iterator(activeDir)) {
                if (entry.is_regular_file() && ImageLoader::IsSupported(entry.path().wstring())) {
                    std::wstring pathStr = entry.path().wstring();
                    bool alreadyExists = false;
                    for (const auto& item : m_page->m_items) {
                        if (item.type == BoardItemType::Image && _wcsicmp(item.filePath.c_str(), pathStr.c_str()) == 0) {
                            alreadyExists = true;
                            break;
                        }
                    }
                    if (!alreadyExists) {
                        files.push_back(entry.path());
                    }
                }
            }
        }
    } catch (...) {}

    if (files.empty()) {
        m_page->m_toastMessage = L"No new images found in active folder";
        m_page->m_toastTimeRemaining = 2.0;
        m_page->m_app->m_window.RequestRedraw();
        return;
    }

    std::sort(files.begin(), files.end());

    auto targetSize = m_page->m_app->m_renderer.GetTargetSize().ToFloat();
    PointF screenCenter = { targetSize.width * 0.5f, targetSize.height * 0.5f };
    PointF worldCenter = m_page->ScreenToWorld(screenCenter);

    float itemW = 400.0f;
    float itemH = 300.0f;
    int count = static_cast<int>(files.size());

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distRotation(-2.0f, 2.0f);

    std::vector<BoardItem> addedItems;
    for (int i = 0; i < count; ++i) {
        BoardItem imgItem;
        imgItem.type = BoardItemType::Image;
        imgItem.width = itemW;
        imgItem.height = itemH;
        imgItem.rotation = distRotation(gen);
        imgItem.filePath = files[i].wstring();
        imgItem.label = files[i].filename().wstring();

        PointF pos = FindSpiralPosition(worldCenter, itemW, itemH, 40.0f, addedItems);
        imgItem.position = pos;

        addedItems.push_back(imgItem);
        m_page->m_items.push_back(imgItem);
    }

    m_page->EvictFarBoardTextures();
    m_page->m_app->m_window.RequestRedraw();
}

void BoardLayoutHelper::ShowContextMenu(float screenX, float screenY) {
    POINT pt;
    if (GetCursorPos(&pt)) {
        PointF worldPos = m_page->ScreenToWorld({ screenX, screenY });
        int clickedIdx = -1;
        for (int i = static_cast<int>(m_page->m_items.size()) - 1; i >= 0; --i) {
            if (m_page->m_items[i].type == BoardItemType::Link) continue;
            if (m_page->m_items[i].GetWorldRect().Contains(worldPos.x, worldPos.y)) {
                clickedIdx = i;
                break;
            }
        }

        if (clickedIdx == -1) {
            float minDistance = 8.0f / m_page->m_canvas.m_zoom;
            for (int i = static_cast<int>(m_page->m_items.size()) - 1; i >= 0; --i) {
                auto& item = m_page->m_items[i];
                if (item.type == BoardItemType::Link && item.fromItemIdx >= 0 && item.fromItemIdx < static_cast<int>(m_page->m_items.size()) &&
                    item.toItemIdx >= 0 && item.toItemIdx < static_cast<int>(m_page->m_items.size())) {
                    PointF p1 = m_page->m_items[item.fromItemIdx].position;
                    PointF p2 = m_page->m_items[item.toItemIdx].position;
                    if (DistanceToLineSegment(worldPos, p1, p2) < minDistance) {
                        clickedIdx = i;
                        break;
                    }
                }
            }
        }

        HMENU hMenu = CreatePopupMenu();
        if (clickedIdx != -1) {
            if (m_page->m_items[clickedIdx].type == BoardItemType::Link) {
                AppendMenuW(hMenu, MF_STRING, 5, L"Delete Link (Del)");
            } else {
                AppendMenuW(hMenu, MF_STRING, 1, L"Link this item (Alt+Click)");
                AppendMenuW(hMenu, MF_STRING, 2, L"Delete item (Del)");
                if (m_page->m_items[clickedIdx].type == BoardItemType::Image) {
                    AppendMenuW(hMenu, MF_STRING, 12, L"Tagging Modal... (T)");
                }
            }
            AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        }
        
        AppendMenuW(hMenu, MF_STRING, 3, L"Add Note");
        AppendMenuW(hMenu, MF_STRING, 4, L"Add Node");
        AppendMenuW(hMenu, MF_STRING, 8, L"New Board");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, 9, L"Buscar por tags... (Ctrl+F)");
        AppendMenuW(hMenu, MF_STRING, 10, L"Escanear tags del directorio (Ctrl+L)");
        AppendMenuW(hMenu, MF_STRING, 7, L"Listar imágenes de carpeta (Shift+Ctrl+L)");
        AppendMenuW(hMenu, MF_STRING, 11, L"Save Board (Ctrl+S)");
        AppendMenuW(hMenu, MF_STRING, 13, L"Deselect All (Esc)");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, 6, L"Settings... (O)");

        int selected = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                                      pt.x, pt.y, 0, m_page->m_app->m_window.GetHwnd(), nullptr);
        DestroyMenu(hMenu);

        if (selected == 1) {
            m_page->StartLinking(clickedIdx);
        } else if (selected == 2) {
            for (auto& item : m_page->m_items) item.selected = false;
            m_page->m_items[clickedIdx].selected = true;
            m_page->DeleteSelectedItems();
        } else if (selected == 3) {
            m_page->AddNoteAt(worldPos, L"Note");
            m_page->m_app->m_window.RequestRedraw();
        } else if (selected == 4) {
            m_page->AddNodeAt(worldPos, L"Node");
            m_page->m_app->m_window.RequestRedraw();
        } else if (selected == 5) {
            for (auto& item : m_page->m_items) item.selected = false;
            m_page->m_items[clickedIdx].selected = true;
            m_page->DeleteSelectedItems();
        } else if (selected == 6) {
            m_page->m_app->OnKeyDown('O', {});
        } else if (selected == 7) {
            ListCurrentDirectoryImages();
        } else if (selected == 8) {
            m_page->m_items.clear();
            m_page->ResetView();
            m_page->m_app->m_window.RequestRedraw();
        } else if (selected == 9) {
            m_page->OnKeyDown('F', { false, true, false });
        } else if (selected == 10) {
            m_page->OnKeyDown('L', { false, true, false });
        } else if (selected == 11) {
            m_page->OnKeyDown('S', { false, true, false });
        } else if (selected == 12) {
            m_page->m_app->OnKeyDown('T', {});
        } else if (selected == 13) {
            for (auto& item : m_page->m_items) item.selected = false;
            m_page->m_app->m_window.RequestRedraw();
        }
    }
}

} // namespace kv
