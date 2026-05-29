// ============================================================================
// thumbnail_strip.cpp — Bottom Thumbnail Navigation Panel Implementation
// Krumassan Image Viewer — Native C++20
// ============================================================================
#include "pch.h"
#include "thumbnail_strip.h"
#include "app.h"
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace kv {

void ThumbnailStrip::Initialize(App* app) {
    m_app = app;
    m_scrollX = 0.0f;
    m_targetScrollX = 0.0f;
    m_opacity = 0.0f;
    m_targetOpacity = 0.0f;
    m_selected.clear();
}

void ThumbnailStrip::SetFileList(const std::vector<std::wstring>& fileList) {
    // Preserve loaded texture cache if paths match
    std::vector<ThumbnailInfo> newThumbnails;
    newThumbnails.reserve(fileList.size());

    for (const auto& path : fileList) {
        ThumbnailInfo info;
        info.path = path;

        // Search in existing cache
        auto it = std::find_if(m_thumbnails.begin(), m_thumbnails.end(),
            [&path](const ThumbnailInfo& t) { return t.path == path; });
        
        if (it != m_thumbnails.end()) {
            info.texture = it->texture;
            info.isLoading = it->isLoading;
            info.loadFailed = it->loadFailed;
        }
        newThumbnails.push_back(std::move(info));
    }

    m_thumbnails = std::move(newThumbnails);
    m_selected.assign(m_thumbnails.size(), false);
}

void ThumbnailStrip::SyncSelection(int currentIndex) {
    if (m_draggingThumbnails || m_groupSelecting) return;

    for (size_t i = 0; i < m_selected.size(); ++i) {
        m_selected[i] = (static_cast<int>(i) == currentIndex);
    }

    // Scroll to center the active item
    if (currentIndex >= 0 && currentIndex < static_cast<int>(m_thumbnails.size())) {
        m_targetScrollX = -THUMBNAIL_WIDTH * currentIndex;
    }
}

bool ThumbnailStrip::HasActiveAnimations() const {
    return std::abs(m_targetScrollX - m_scrollX) > 0.1f ||
           std::abs(m_targetOpacity - m_opacity) > 0.01f;
}

void ThumbnailStrip::Update(double dt) {
    if (m_thumbnails.empty()) return;

    float lerpFactor = 1.0f - std::exp(-15.0f * static_cast<float>(dt));
    m_scrollX = std::lerp(m_scrollX, m_targetScrollX, lerpFactor);
    m_opacity = std::lerp(m_opacity, m_targetOpacity, lerpFactor);

    if (std::abs(m_targetScrollX - m_scrollX) < 0.1f) m_scrollX = m_targetScrollX;
    if (std::abs(m_targetOpacity - m_opacity) < 0.01f) m_opacity = m_targetOpacity;

    // Check mouse autohide trigger
    POINT pt;
    if (GetCursorPos(&pt) && ScreenToClient(m_app->m_window.GetHwnd(), &pt)) {
        auto targetSize = m_app->m_renderer.GetTargetSize().ToFloat();
        if (pt.x >= 0 && pt.x <= targetSize.width && pt.y >= 0 && pt.y <= targetSize.height) {
            if (pt.y >= targetSize.height - HEIGHT) {
                m_targetOpacity = 1.0f;
            } else if (!m_draggingThumbnails && !m_groupSelecting) {
                m_targetOpacity = 0.0f;
            }
        } else if (!m_draggingThumbnails && !m_groupSelecting) {
            m_targetOpacity = 0.0f;
        }
    }

    // Trigger loads for visible thumbnails
    if (m_opacity > 0.0f) {
        auto targetSize = m_app->m_renderer.GetTargetSize().ToFloat();
        float windowWidth = targetSize.width;
        float windowHeight = targetSize.height;

        for (size_t i = 0; i < m_thumbnails.size(); ++i) {
            RectF r = GetThumbnailRect(static_cast<int>(i), windowWidth, windowHeight);
            if (r.right >= 0 && r.left <= windowWidth) {
                LoadThumbnail(static_cast<int>(i));
            }
        }
    }
}

void ThumbnailStrip::LoadThumbnail(int index) {
    if (index < 0 || index >= static_cast<int>(m_thumbnails.size())) return;
    auto& info = m_thumbnails[index];

    if (!info.texture && !info.isLoading && !info.loadFailed) {
        info.isLoading = true;
        
        m_app->m_imageLoader.LoadAsync(info.path, [this, index](std::shared_ptr<ImageData> data) {
            if (data && data->gpuBitmap) {
                m_thumbnails[index].texture = data->gpuBitmap;
            } else {
                m_thumbnails[index].loadFailed = true;
            }
            m_thumbnails[index].isLoading = false;
            m_app->m_window.RequestRedraw();
        }, 100, 100); // decimate/scale to 100x100
    }
}

RectF ThumbnailStrip::GetThumbnailRect(int index, float windowWidth, float windowHeight) const {
    float baseX = (windowWidth - THUMBNAIL_WIDTH) / 2.0f + m_scrollX;
    float x = baseX + THUMBNAIL_WIDTH * index;
    float y = windowHeight - HEIGHT + 30.0f;
    return RectF::FromXYWH(x, y, THUMBNAIL_WIDTH, THUMBNAIL_WIDTH);
}

void ThumbnailStrip::DrawMirror(Renderer& renderer, ID2D1Bitmap1* bitmap, const RectF& rect, float opacity) {
    D2D1_POINT_2F pivot = D2D1::Point2F(rect.Center().x, rect.bottom);
    renderer.PushTransform(D2D1::Matrix3x2F::Scale(1.0f, -1.0f, pivot));
    renderer.DrawBitmap(bitmap, rect, opacity * 0.25f);
    renderer.PopTransform();
}

void ThumbnailStrip::Draw(Renderer& renderer) {
    if (m_thumbnails.empty() || m_opacity == 0.0f) return;

    auto targetSize = renderer.GetTargetSize().ToFloat();
    float windowWidth = targetSize.width;
    float windowHeight = targetSize.height;

    // 1. Draw backplate panel
    RectF panelRect = RectF::FromXYWH(0, windowHeight - HEIGHT, windowWidth, HEIGHT);
    renderer.FillRect(panelRect, Color::FromRGBA(20, 20, 20, static_cast<uint8_t>(220 * m_opacity)));

    // 2. Draw filename/metadata text
    std::wstring labelText;
    if (m_hoveredIndex >= 0 && m_hoveredIndex < static_cast<int>(m_thumbnails.size())) {
        std::filesystem::path p(m_thumbnails[m_hoveredIndex].path);
        labelText = p.filename().wstring();
    } else if (m_app->m_currentIndex >= 0 && m_app->m_currentIndex < static_cast<int>(m_thumbnails.size())) {
        std::filesystem::path p(m_thumbnails[m_app->m_currentIndex].path);
        labelText = p.filename().wstring();
    }

    if (!labelText.empty()) {
        RectF labelRect = RectF::FromXYWH(15, windowHeight - HEIGHT + 5, windowWidth - 30, 25);
        renderer.DrawText(labelText, labelRect, Color::White(0.9f * m_opacity), 12.0f, TextAlign::Center, TextVAlign::Center);
    }

    // 3. Draw thumbnails with clipping (thumbnails must not spill out of the strip region)
    renderer.PushClip(RectF::FromXYWH(0, windowHeight - HEIGHT + 30, windowWidth, HEIGHT - 30));

    for (size_t i = 0; i < m_thumbnails.size(); ++i) {
        RectF rect = GetThumbnailRect(static_cast<int>(i), windowWidth, windowHeight);
        if (rect.right < 0 || rect.left > windowWidth) continue;

        bool isSelected = m_selected[i];
        bool isHovered = (static_cast<int>(i) == m_hoveredIndex);

        RectF drawRect = rect;
        if (isSelected) {
            drawRect.top -= AUGMENTED_THUBNAIL_INCREMENT;
        }

        float thumbOpacity = (isHovered || isSelected) ? 1.0f : 0.5f;
        thumbOpacity *= m_opacity;

        // Draw thumbnail bounding box placeholder
        renderer.FillRect(drawRect, Color::Black(0.5f * thumbOpacity));

        if (m_thumbnails[i].texture) {
            // Calculate source crop rect to match aspect ratio of destination rect
            auto texSize = m_thumbnails[i].texture->GetSize();
            float texW = texSize.width;
            float texH = texSize.height;
            float destW = drawRect.Width();
            float destH = drawRect.Height();

            RectF srcRect;
            if (destW / destH > texW / texH) {
                float cropH = texW * destH / destW;
                srcRect = RectF::FromXYWH(0.0f, (texH - cropH) / 2.0f, texW, cropH);
            } else {
                float cropW = texH * destW / destH;
                srcRect = RectF::FromXYWH((texW - cropW) / 2.0f, 0.0f, cropW, texH);
            }

            renderer.DrawBitmap(m_thumbnails[i].texture.Get(), drawRect, thumbOpacity, BitmapInterpolation::Linear, &srcRect);
            
            // Draw reflection mirror effect below thumbnails (unless currently dragged/shifted)
            if (!isSelected) {
                DrawMirror(renderer, m_thumbnails[i].texture.Get(), rect, thumbOpacity);
            }
        } else if (m_thumbnails[i].isLoading) {
            // Draw loading indicator text
            renderer.DrawText(L"...", drawRect, Color::White(0.5f * thumbOpacity), 12.0f, TextAlign::Center, TextVAlign::Center);
        } else if (m_thumbnails[i].loadFailed) {
            // Draw error placeholder text
            renderer.DrawText(L"?!", drawRect, Color::FromRGBA(200, 50, 50, static_cast<uint8_t>(200 * thumbOpacity)), 14.0f, TextAlign::Center, TextVAlign::Center);
        }
    }

    // 4. Draw red insertion cursor indicator
    if (m_draggingThumbnails && m_insertIndex >= 0 && m_insertIndex <= static_cast<int>(m_thumbnails.size())) {
        float baseX = (windowWidth - THUMBNAIL_WIDTH) / 2.0f + m_scrollX;
        float ix = baseX + THUMBNAIL_WIDTH * m_insertIndex;
        float iy = windowHeight - HEIGHT + 30.0f;
        
        PointF pTop = { ix, iy };
        PointF pBottom = { ix, iy + THUMBNAIL_WIDTH };

        renderer.DrawLine(pTop, pBottom, Color::FromRGBA(220, 30, 30), 4.0f);
        renderer.DrawLine(pTop, { pTop.x - 5.0f, pTop.y + 5.0f }, Color::FromRGBA(220, 30, 30), 3.0f);
        renderer.DrawLine(pTop, { pTop.x + 5.0f, pTop.y + 5.0f }, Color::FromRGBA(220, 30, 30), 3.0f);
        renderer.DrawLine(pBottom, { pBottom.x - 5.0f, pBottom.y - 5.0f }, Color::FromRGBA(220, 30, 30), 3.0f);
        renderer.DrawLine(pBottom, { pBottom.x + 5.0f, pBottom.y - 5.0f }, Color::FromRGBA(220, 30, 30), 3.0f);
    }

    // 5. Draw group selection box
    if (m_groupSelecting) {
        renderer.DrawRect(m_selectionRect, Color::FromRGBA(50, 220, 50), 1.5f);
        renderer.FillRect(m_selectionRect, Color::FromRGBA(50, 220, 50, 40));
    }

    renderer.PopClip();
}

bool ThumbnailStrip::IsMouseOver(float x, float y) const {
    if (m_thumbnails.empty() || m_opacity == 0.0f) return false;
    auto targetSize = m_app->m_renderer.GetTargetSize().ToFloat();
    return y >= targetSize.height - HEIGHT && y <= targetSize.height;
}

bool ThumbnailStrip::HandleMouseMove(float x, float y, KeyModifiers mods) {
    if (m_thumbnails.empty() || m_opacity == 0.0f) return false;

    auto targetSize = m_app->m_renderer.GetTargetSize().ToFloat();
    float windowWidth = targetSize.width;
    float windowHeight = targetSize.height;

    // Detect hovered thumbnail
    m_hoveredIndex = -1;
    float baseX = (windowWidth - THUMBNAIL_WIDTH) / 2.0f + m_scrollX;
    float relX = x - baseX;
    int clickedIndex = static_cast<int>(std::floor(relX / THUMBNAIL_WIDTH));
    if (clickedIndex >= 0 && clickedIndex < static_cast<int>(m_thumbnails.size())) {
        m_hoveredIndex = clickedIndex;
    }

    if (m_groupSelecting) {
        m_selectionRect.left = std::min(m_selectionStart.x, x);
        m_selectionRect.top = std::min(m_selectionStart.y, y);
        m_selectionRect.right = std::max(m_selectionStart.x, x);
        m_selectionRect.bottom = std::max(m_selectionStart.y, y);

        // Select all thumbnails intersecting selection box
        for (size_t i = 0; i < m_thumbnails.size(); ++i) {
            RectF r = GetThumbnailRect(static_cast<int>(i), windowWidth, windowHeight);
            m_selected[i] = m_selectionRect.Intersects(r);
        }
        m_app->m_window.RequestRedraw();
        return true;
    }

    if (m_draggingThumbnails) {
        if (std::hypot(x - m_dragStartPos.x, y - m_dragStartPos.y) > 5.0f) {
            m_hasDragged = true;
        }

        if (m_hasDragged) {
            m_insertIndex = static_cast<int>(std::round(relX / THUMBNAIL_WIDTH));
            m_insertIndex = std::clamp(m_insertIndex, 0, static_cast<int>(m_thumbnails.size()));
            
            // Adjust scroll dynamically if dragging near edges
            if (x < 60.0f) {
                m_targetScrollX = std::min(0.0f, m_targetScrollX + 5.0f);
            } else if (x > windowWidth - 60.0f) {
                float maxScroll = -THUMBNAIL_WIDTH * (m_thumbnails.size() - 1);
                m_targetScrollX = std::max(maxScroll, m_targetScrollX - 5.0f);
            }
        }
        m_app->m_window.RequestRedraw();
        return true;
    }

    m_app->m_window.RequestRedraw();
    return IsMouseOver(x, y);
}

bool ThumbnailStrip::HandleMouseDown(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (m_thumbnails.empty() || m_opacity == 0.0f) return false;

    if (!IsMouseOver(x, y)) {
        if (m_targetOpacity > 0.0f && !m_draggingThumbnails && !m_groupSelecting) {
            m_targetOpacity = 0.0f;
            m_app->m_window.RequestRedraw();
        }
        return false;
    }

    if (btn == MouseButton::Left) {
        if (mods.ctrl) {
            // Start multi-selection box
            m_groupSelecting = true;
            m_selectionStart = { x, y };
            m_selectionRect = RectF(x, y, x, y);
            std::fill(m_selected.begin(), m_selected.end(), false);
        } else {
            // Start drag
            m_draggingThumbnails = true;
            m_dragStartPos = { x, y };
            m_hasDragged = false;
            m_insertIndex = -1;

            auto targetSize = m_app->m_renderer.GetTargetSize().ToFloat();
            float baseX = (targetSize.width - THUMBNAIL_WIDTH) / 2.0f + m_scrollX;
            int idx = static_cast<int>(std::floor((x - baseX) / THUMBNAIL_WIDTH));
            if (idx >= 0 && idx < static_cast<int>(m_thumbnails.size())) {
                // If the clicked element is not selected yet, clear other selections
                if (!m_selected[idx]) {
                    std::fill(m_selected.begin(), m_selected.end(), false);
                    m_selected[idx] = true;
                }
            }
        }
        m_app->m_window.RequestRedraw();
        return true;
    } else if (btn == MouseButton::Right) {
        POINT pt;
        if (GetCursorPos(&pt)) {
            ShowContextMenu(static_cast<float>(pt.x), static_cast<float>(pt.y));
        }
        return true;
    }

    return false;
}

bool ThumbnailStrip::HandleMouseUp(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (m_thumbnails.empty() || m_opacity == 0.0f) return false;

    if (btn == MouseButton::Left) {
        if (m_groupSelecting) {
            m_groupSelecting = false;
            m_app->m_window.RequestRedraw();
            return true;
        }

        if (m_draggingThumbnails) {
            if (m_hasDragged && m_insertIndex >= 0) {
                Rearrange(m_insertIndex);
            } else {
                // Perform select/click navigation
                auto targetSize = m_app->m_renderer.GetTargetSize().ToFloat();
                float baseX = (targetSize.width - THUMBNAIL_WIDTH) / 2.0f + m_scrollX;
                int idx = static_cast<int>(std::floor((x - baseX) / THUMBNAIL_WIDTH));
                if (idx >= 0 && idx < static_cast<int>(m_thumbnails.size())) {
                    m_app->LoadImage(m_thumbnails[idx].path);
                }
            }
            m_draggingThumbnails = false;
            m_insertIndex = -1;
            m_app->m_window.RequestRedraw();
            return true;
        }
    }

    return IsMouseOver(x, y);
}

bool ThumbnailStrip::HandleMouseWheel(float delta, float x, float y, KeyModifiers mods) {
    if (m_thumbnails.empty() || m_opacity == 0.0f) return false;

    if (IsMouseOver(x, y)) {
        // Scroll the strip left or right
        float maxScroll = -THUMBNAIL_WIDTH * (m_thumbnails.size() - 1);
        m_targetScrollX += (delta > 0) ? THUMBNAIL_WIDTH * 2.0f : -THUMBNAIL_WIDTH * 2.0f;
        m_targetScrollX = std::clamp(m_targetScrollX, maxScroll, 0.0f);
        m_app->m_window.RequestRedraw();
        return true;
    }

    return false;
}

bool ThumbnailStrip::HandleMouseDoubleClick(MouseButton btn, float x, float y, KeyModifiers mods) {
    return IsMouseOver(x, y);
}

void ThumbnailStrip::Rearrange(int insertIndex) {
    if (insertIndex < 0 || insertIndex > static_cast<int>(m_thumbnails.size())) return;

    // Collect all selected items' paths
    std::vector<std::wstring> selectedPaths;
    for (size_t i = 0; i < m_selected.size(); ++i) {
        if (m_selected[i]) {
            selectedPaths.push_back(m_thumbnails[i].path);
        }
    }

    if (selectedPaths.empty()) return;

    // Separate selected paths from remaining paths
    std::vector<std::wstring> remainingPaths;
    remainingPaths.reserve(m_app->m_fileList.size() - selectedPaths.size());
    for (const auto& path : m_app->m_fileList) {
        if (std::find(selectedPaths.begin(), selectedPaths.end(), path) == selectedPaths.end()) {
            remainingPaths.push_back(path);
        }
    }

    // Build the new list by inserting selectedPaths at insertIndex
    int clampInsertIndex = std::clamp(insertIndex, 0, static_cast<int>(remainingPaths.size()));
    
    std::vector<std::wstring> newFileList;
    newFileList.reserve(m_app->m_fileList.size());
    newFileList.insert(newFileList.end(), remainingPaths.begin(), remainingPaths.begin() + clampInsertIndex);
    newFileList.insert(newFileList.end(), selectedPaths.begin(), selectedPaths.end());
    newFileList.insert(newFileList.end(), remainingPaths.begin() + clampInsertIndex, remainingPaths.end());

    m_app->m_fileList = newFileList;
    SetFileList(newFileList);

    // Sync active index
    if (m_app->m_currentImage) {
        auto it = std::find(m_app->m_fileList.begin(), m_app->m_fileList.end(), m_app->m_currentImage->filePath);
        if (it != m_app->m_fileList.end()) {
            m_app->m_currentIndex = static_cast<int>(std::distance(m_app->m_fileList.begin(), it));
        }
    }

    // Reset selection flags
    std::fill(m_selected.begin(), m_selected.end(), false);
    m_draggingThumbnails = false;
    m_app->m_window.RequestRedraw();
}

void ThumbnailStrip::ShowContextMenu(float screenX, float screenY) {
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, 1, L"Original order");
    AppendMenuW(hMenu, MF_STRING, 2, L"Sort by filename (ascending)");
    AppendMenuW(hMenu, MF_STRING, 3, L"Sort by filename (descending)");
    AppendMenuW(hMenu, MF_STRING, 4, L"Sort by date (ascending)");
    AppendMenuW(hMenu, MF_STRING, 5, L"Sort by date (descending)");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, 6, L"Save order metadata to disk");
    AppendMenuW(hMenu, MF_STRING, 7, L"Toggle fullscreen mode");

    int selected = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                                  static_cast<int>(screenX), static_cast<int>(screenY),
                                  0, m_app->m_window.GetHwnd(), nullptr);
    DestroyMenu(hMenu);

    if (selected == 0) return;

    if (selected == 1 || selected == 2) {
        std::sort(m_app->m_fileList.begin(), m_app->m_fileList.end());
    } else if (selected == 3) {
        std::sort(m_app->m_fileList.begin(), m_app->m_fileList.end(), std::greater<std::wstring>());
    } else if (selected == 4 || selected == 5) {
        namespace fs = std::filesystem;
        auto compareAsc = [](const std::wstring& a, const std::wstring& b) {
            try { return fs::last_write_time(a) < fs::last_write_time(b); } catch (...) { return a < b; }
        };
        auto compareDesc = [](const std::wstring& a, const std::wstring& b) {
            try { return fs::last_write_time(a) > fs::last_write_time(b); } catch (...) { return a > b; }
        };
        if (selected == 4) {
            std::sort(m_app->m_fileList.begin(), m_app->m_fileList.end(), compareAsc);
        } else {
            std::sort(m_app->m_fileList.begin(), m_app->m_fileList.end(), compareDesc);
        }
    } else if (selected == 6) {
        if (!m_app->m_fileList.empty()) {
            namespace fs = std::filesystem;
            fs::path p(m_app->m_fileList[0]);
            fs::path orderFile = p.parent_path() / L"viewer_order.ini";
            std::wofstream out(orderFile);
            if (out) {
                for (const auto& f : m_app->m_fileList) {
                    out << fs::path(f).filename().wstring() << L"\n";
                }
                out.close();
                SetFileAttributesW(orderFile.c_str(), FILE_ATTRIBUTE_HIDDEN);
                LogInfo(L"Order metadata saved to viewer_order.ini");
            }
        }
    } else if (selected == 7) {
        m_app->m_window.ToggleFullscreen();
    }

    SetFileList(m_app->m_fileList);
    if (m_app->m_currentImage) {
        auto it = std::find(m_app->m_fileList.begin(), m_app->m_fileList.end(), m_app->m_currentImage->filePath);
        if (it != m_app->m_fileList.end()) {
            m_app->m_currentIndex = static_cast<int>(std::distance(m_app->m_fileList.begin(), it));
        }
    }
    m_app->m_window.RequestRedraw();
}

} // namespace kv
