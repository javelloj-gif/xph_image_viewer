// ============================================================================
// library_page.cpp — Library Page Component Implementation
// Krumassan Image Viewer — Native C++20
// ============================================================================
#include "pch.h"
#include "library_page.h"
#include "app.h"
#include <algorithm>
#include <filesystem>

namespace kv {

void LibraryPage::Initialize(App* app) {
    m_app = app;
    m_foldersScrollY = 0.0f;
    m_targetFoldersScrollY = 0.0f;
    m_hoveredFolderIdx = -1;
    m_hoveredPreviewIdx = -1;
    m_scrollCapture = ScrollCapture::None;

    // Scan the initial folder if we have one
    if (!m_app->m_fileList.empty()) {
        namespace fs = std::filesystem;
        fs::path curPath(m_app->m_fileList[0]);
        auto parentPath = curPath.parent_path().wstring();
        AddFolder(parentPath);

        // Scan sibling folders in the parent directory to populate the library list
        try {
            fs::path pParent = curPath.parent_path().parent_path();
            if (fs::exists(pParent) && fs::is_directory(pParent)) {
                int count = 0;
                for (const auto& entry : fs::directory_iterator(pParent)) {
                    if (entry.is_directory()) {
                        // Check if it has any supported images
                        bool hasImages = false;
                        for (const auto& subEntry : fs::directory_iterator(entry.path())) {
                            if (subEntry.is_regular_file() && ImageLoader::IsSupported(subEntry.path().wstring())) {
                                hasImages = true;
                                break;
                            }
                        }
                        if (hasImages) {
                            AddFolder(entry.path().wstring());
                            if (++count > 10) break; // Limit to 10 folders for startup performance
                        }
                    }
                }
            }
        } catch (...) {}
    }

    SyncCurrentFolderWithApp();
}

void LibraryPage::Update(double dt) {
    // 1. Smooth scroll interpolation for folders list
    if (std::abs(m_targetFoldersScrollY - m_foldersScrollY) > 0.1f) {
        m_foldersScrollY += static_cast<float>((m_targetFoldersScrollY - m_foldersScrollY) * 15.0 * dt);
    } else {
        m_foldersScrollY = m_targetFoldersScrollY;
    }

    // 2. Smooth scroll interpolation for active folder previews
    if (m_app->m_currentFolderIdx >= 0 && m_app->m_currentFolderIdx < static_cast<int>(m_app->m_folders.size())) {
        auto& folder = m_app->m_folders[m_app->m_currentFolderIdx];
        if (std::abs(folder.targetScrollOffset - folder.scrollOffset) > 0.1f) {
            folder.scrollOffset += static_cast<float>((folder.targetScrollOffset - folder.scrollOffset) * 15.0 * dt);
            m_app->m_window.RequestRedraw();
        } else {
            folder.scrollOffset = folder.targetScrollOffset;
        }
    }

    // 3. Update hovered animation sequencer
    if (m_hoveredOriginalAnim && !m_hoveredAnimSequencer.IsPaused()) {
        if (m_hoveredAnimSequencer.Update(dt)) {
            m_app->m_window.RequestRedraw();
        }
    }
}

void LibraryPage::Draw(Renderer& renderer) {
    auto size = renderer.GetTargetSize().ToFloat();
    float cxp = size.width / 2.0f;

    // 1. Calculate panels rects
    RectF leftRect = RectF::FromXYWH(0, 0, cxp - CENTER_OFFSET, size.height);
    RectF rightRect = RectF::FromXYWH(cxp + CENTER_OFFSET, 0, size.width - (cxp + CENTER_OFFSET), size.height);

    // 2. Draw dual panels content
    DrawFoldersList(renderer, leftRect);
    DrawPreviewsGrid(renderer, rightRect);

    // 3. Draw middle dividing line
    renderer.DrawLine({ cxp, 0 }, { cxp, size.height }, Color::FromHex(0x1E1E1E), 1.0f);

    // 4. Draw scrollbars (translucent overlays)
    // Left panel scrollbar
    float leftContentHeight = m_app->m_folders.size() * FOLDER_ITEM_HEIGHT + 20.0f;
    if (leftContentHeight > size.height) {
        RectF trackRect = RectF::FromXYWH(cxp - CENTER_OFFSET - SCROLLBAR_WIDTH - 5.0f, 10.0f, SCROLLBAR_WIDTH, size.height - 20.0f);
        float scrollbarOpacity = (m_scrollCapture == ScrollCapture::Folders) ? 0.9f : 0.4f;
        DrawScrollbar(renderer, trackRect, leftContentHeight, size.height, m_foldersScrollY, scrollbarOpacity);
    }

    // Right panel scrollbar
    if (m_app->m_currentFolderIdx >= 0 && m_app->m_currentFolderIdx < static_cast<int>(m_app->m_folders.size())) {
        auto& folder = m_app->m_folders[m_app->m_currentFolderIdx];
        // Calculate max previews height
        float rightContentHeight = 0.0f;
        for (const auto& item : folder.previews) {
            if (item.rect.bottom > rightContentHeight) {
                rightContentHeight = item.rect.bottom;
            }
        }
        rightContentHeight += 20.0f; // extra padding

        if (rightContentHeight > size.height) {
            RectF trackRect = RectF::FromXYWH(cxp + CENTER_OFFSET + 5.0f, 10.0f, SCROLLBAR_WIDTH, size.height - 20.0f);
            float scrollbarOpacity = (m_scrollCapture == ScrollCapture::Previews) ? 0.9f : 0.4f;
            DrawScrollbar(renderer, trackRect, rightContentHeight, size.height, folder.scrollOffset, scrollbarOpacity);
        }
    }

    // 5. Draw hovered preview overlay on the left side (if loaded)
    if (m_hoveredPreviewIdx != -1 && m_app->m_currentFolderIdx >= 0) {
        auto& folder = m_app->m_folders[m_app->m_currentFolderIdx];
        if (m_hoveredPreviewIdx < static_cast<int>(folder.previews.size())) {
            auto& previewItem = folder.previews[m_hoveredPreviewIdx];
            
            // Draw fit image on the left half of the screen
            if (m_hoveredOriginalAnim && m_hoveredOriginalAnim->IsValid()) {
                RectF leftHalfRect = RectF::FromXYWH(0, 0, cxp - CENTER_OFFSET, size.height);
                renderer.FillRect(leftHalfRect, Color::Black(0.8f));

                int frameIdx = m_hoveredAnimSequencer.GetCurrentFrame();
                if (frameIdx >= 0 && frameIdx < static_cast<int>(m_hoveredOriginalAnim->frames.size())) {
                    auto& frame = m_hoveredOriginalAnim->frames[frameIdx];
                    if (frame.gpuBitmap) {
                        float imgW = static_cast<float>(frame.width);
                        float imgH = static_cast<float>(frame.height);
                        float scaleX = leftHalfRect.Width() / imgW;
                        float scaleY = leftHalfRect.Height() / imgH;
                        float scale = std::min(scaleX, scaleY);

                        float fitW = imgW * scale;
                        float fitH = imgH * scale;
                        float x = leftHalfRect.left + (leftHalfRect.Width() - fitW) / 2.0f;
                        float y = leftHalfRect.top + (leftHalfRect.Height() - fitH) / 2.0f;
                        RectF imageRect = RectF::FromXYWH(x, y, fitW, fitH);

                        renderer.DrawBitmap(frame.gpuBitmap.Get(), imageRect, 1.0f, BitmapInterpolation::HighQuality);
                        
                        std::wstring labelText = std::format(L"{} ({}×{})", 
                            std::filesystem::path(previewItem.filePath).filename().wstring(), 
                            frame.width, frame.height);
                        renderer.DrawText(labelText, RectF::FromXYWH(15, size.height - 40, leftHalfRect.Width() - 30, 30), Color::White(0.9f), 14.0f);
                    }
                }
            } else if (m_hoveredOriginalImage && m_hoveredOriginalImage->HasGpuBitmap()) {
                RectF leftHalfRect = RectF::FromXYWH(0, 0, cxp - CENTER_OFFSET, size.height);
                
                // Draw background dark overlay first
                renderer.FillRect(leftHalfRect, Color::Black(0.8f));

                float imgW = static_cast<float>(m_hoveredOriginalImage->width);
                float imgH = static_cast<float>(m_hoveredOriginalImage->height);
                float scaleX = leftHalfRect.Width() / imgW;
                float scaleY = leftHalfRect.Height() / imgH;
                float scale = std::min(scaleX, scaleY);

                float fitW = imgW * scale;
                float fitH = imgH * scale;
                float x = leftHalfRect.left + (leftHalfRect.Width() - fitW) / 2.0f;
                float y = leftHalfRect.top + (leftHalfRect.Height() - fitH) / 2.0f;
                RectF imageRect = RectF::FromXYWH(x, y, fitW, fitH);

                if (m_hoveredOriginalImage->hasAlpha) {
                    DrawCheckerboard(renderer, imageRect);
                }

                renderer.DrawBitmap(m_hoveredOriginalImage->gpuBitmap.Get(), imageRect, 1.0f, BitmapInterpolation::HighQuality);
                
                // Draw text with dimensions and path
                std::wstring labelText = std::format(L"{} ({}×{})", 
                    std::filesystem::path(previewItem.filePath).filename().wstring(), 
                    m_hoveredOriginalImage->width, m_hoveredOriginalImage->height);
                renderer.DrawText(labelText, RectF::FromXYWH(15, size.height - 40, leftHalfRect.Width() - 30, 30), Color::White(0.9f), 14.0f);
            } else if (m_hoveredOriginalLoading) {
                RectF leftHalfRect = RectF::FromXYWH(0, 0, cxp - CENTER_OFFSET, size.height);
                renderer.FillRect(leftHalfRect, Color::Black(0.6f));
                renderer.DrawText(L"LOADING FULL IMAGE...", leftHalfRect, Color::White(0.5f), 16.0f, TextAlign::Center, TextVAlign::Center);
            }
        }
    }
}

void LibraryPage::DrawFoldersList(Renderer& renderer, const RectF& rect) {
    renderer.PushClip(rect);

    auto size = renderer.GetTargetSize().ToFloat();
    float left = rect.left;
    float width = rect.Width();

    for (int i = 0; i < static_cast<int>(m_app->m_folders.size()); ++i) {
        auto& folder = m_app->m_folders[i];
        float top = 10.0f + i * FOLDER_ITEM_HEIGHT + m_foldersScrollY;
        RectF itemRect = RectF::FromXYWH(left + 15.0f, top, width - 30.0f, FOLDER_ITEM_HEIGHT - 10.0f);

        // Skip drawing if offscreen
        if (itemRect.bottom < 0 || itemRect.top > size.height) {
            continue;
        }

        // 1. Draw backgrounds
        bool isCurrent = (i == m_app->m_currentFolderIdx);
        bool isHovered = (i == m_hoveredFolderIdx);

        if (isCurrent) {
            renderer.FillRoundedRect(itemRect, 6.0f, 6.0f, Color::FromRGBA(255, 160, 0, 40)); // Translucent gold
            renderer.DrawRoundedRect(itemRect, 6.0f, 6.0f, Color::FromRGBA(255, 160, 0, 150), 1.5f);
        } else if (isHovered) {
            renderer.FillRoundedRect(itemRect, 6.0f, 6.0f, Color::FromRGBA(255, 255, 255, 15));
            renderer.DrawRoundedRect(itemRect, 6.0f, 6.0f, Color::FromRGBA(255, 255, 255, 40), 1.0f);
        } else {
            renderer.FillRoundedRect(itemRect, 6.0f, 6.0f, Color::FromRGBA(255, 255, 255, 5));
        }

        // 2. Draw folder thumbnail (on the left side of the folder card)
        RectF thumbRect = RectF::FromXYWH(itemRect.left + 10.0f, itemRect.top + 10.0f, 90.0f, 90.0f);
        bool thumbnailDrawn = false;

        if (!folder.filePaths.empty()) {
            // Trigger preview texture load for the first item (folder card thumbnail)
            if (folder.previews[0].texture && folder.previews[0].texture->HasGpuBitmap()) {
                renderer.DrawBitmap(folder.previews[0].texture->gpuBitmap.Get(), thumbRect, 1.0f, BitmapInterpolation::HighQuality);
                thumbnailDrawn = true;
            } else {
                LoadPreviewTexture(i, 0);
            }
        }

        if (!thumbnailDrawn) {
            // Folder placeholder icon
            renderer.FillRoundedRect(thumbRect, 4.0f, 4.0f, Color::FromRGBA(50, 50, 50, 180));
            renderer.DrawText(L"📁", thumbRect, Color::FromRGBA(255, 200, 50, 180), 28.0f, TextAlign::Center, TextVAlign::Center);
        }

        // 3. Draw text details
        float textLeft = thumbRect.right + 15.0f;
        float textWidth = itemRect.right - textLeft - 10.0f;

        // Folder name (bold)
        RectF nameRect = RectF::FromXYWH(textLeft, itemRect.top + 12.0f, textWidth, 24.0f);
        renderer.DrawText(folder.folderName, nameRect, Color::White(0.9f), 16.0f, TextAlign::Left, TextVAlign::Center);

        // Folder file count and short path
        std::wstring descText = std::format(L"{} images | {}", folder.filePaths.size(), folder.folderPath);
        RectF descRect = RectF::FromXYWH(textLeft, itemRect.top + 38.0f, textWidth, 20.0f);
        renderer.DrawText(descText, descRect, Color::White(0.45f), 11.0f, TextAlign::Left, TextVAlign::Center);

        // 4. Draw horizontal mini row of thumbnails (up to 5 images)
        float miniThumbY = itemRect.top + 64.0f;
        float miniThumbW = 36.0f;
        float miniThumbGap = 5.0f;

        for (int miniIdx = 1; miniIdx < std::min(6, static_cast<int>(folder.filePaths.size())); ++miniIdx) {
            float miniThumbX = textLeft + (miniIdx - 1) * (miniThumbW + miniThumbGap);
            RectF miniRect = RectF::FromXYWH(miniThumbX, miniThumbY, miniThumbW, miniThumbW);

            if (folder.previews[miniIdx].texture && folder.previews[miniIdx].texture->HasGpuBitmap()) {
                renderer.DrawBitmap(folder.previews[miniIdx].texture->gpuBitmap.Get(), miniRect, 0.85f, BitmapInterpolation::HighQuality);
            } else {
                LoadPreviewTexture(i, miniIdx);
                renderer.FillRect(miniRect, Color::FromRGBA(40, 40, 40, 100));
            }
        }
    }

    renderer.PopClip();
}

void LibraryPage::DrawPreviewsGrid(Renderer& renderer, const RectF& rect) {
    if (m_app->m_currentFolderIdx < 0 || m_app->m_currentFolderIdx >= static_cast<int>(m_app->m_folders.size())) {
        renderer.DrawText(L"NO FOLDER SELECTED", rect, Color::White(0.4f), 18.0f, TextAlign::Center, TextVAlign::Center);
        return;
    }

    // Ensure layout is current
    CalculateLayout();

    renderer.PushClip(rect);

    auto size = renderer.GetTargetSize().ToFloat();
    auto& folder = m_app->m_folders[m_app->m_currentFolderIdx];

    // Render previews
    for (int i = 0; i < static_cast<int>(folder.previews.size()); ++i) {
        auto& item = folder.previews[i];
        
        // Offset calculated rect by scrollY
        RectF drawRect = item.rect;
        drawRect.top += folder.scrollOffset;
        drawRect.bottom += folder.scrollOffset;

        // Culling
        if (drawRect.bottom < 0 || drawRect.top > size.height) {
            continue;
        }

        // Draw preview
        bool isHovered = (i == m_hoveredPreviewIdx);

        if (item.texture && item.texture->HasGpuBitmap()) {
            if (isHovered) {
                // Enlarge slightly on hover
                float cx = drawRect.Center().x;
                float cy = drawRect.Center().y;
                float w = drawRect.Width() * 1.05f;
                float h = drawRect.Height() * 1.05f;
                RectF hoveredRect = RectF::Centered(cx, cy, w, h);

                // Draw background shadow/backplate
                renderer.FillRect(hoveredRect, Color::Black(0.9f));
                renderer.DrawBitmap(item.texture->gpuBitmap.Get(), hoveredRect, 1.0f, BitmapInterpolation::HighQuality);
                renderer.DrawRect(hoveredRect, Color::White(0.9f), 1.5f);
            } else {
                renderer.DrawBitmap(item.texture->gpuBitmap.Get(), drawRect, 0.85f, BitmapInterpolation::HighQuality);
            }
        } else {
            // Draw placeholder card and trigger async load
            renderer.FillRect(drawRect, Color::FromRGBA(25, 25, 25, 120));
            renderer.DrawRect(drawRect, Color::White(0.08f), 1.0f);
            renderer.DrawText(L"LOADING...", drawRect, Color::White(0.3f), 11.0f, TextAlign::Center, TextVAlign::Center);
            
            LoadPreviewTexture(m_app->m_currentFolderIdx, i);
        }
    }

    renderer.PopClip();
}

void LibraryPage::DrawScrollbar(Renderer& renderer, const RectF& trackRect, float contentHeight, float viewHeight, float scrollOffset, float opacity) {
    // Background track
    renderer.FillRoundedRect(trackRect, 4.0f, 4.0f, Color::White(0.05f * opacity));

    // Thumb height calculation
    float viewRatio = viewHeight / contentHeight;
    float thumbHeight = std::max(SCROLLBAR_MIN_HEIGHT, trackRect.Height() * viewRatio);

    // Thumb Y position calculation
    float maxScroll = contentHeight - viewHeight;
    float scrollRatio = std::clamp(std::abs(scrollOffset) / maxScroll, 0.0f, 1.0f);
    float thumbY = trackRect.top + scrollRatio * (trackRect.Height() - thumbHeight);

    RectF thumbRect = RectF::FromXYWH(trackRect.left, thumbY, trackRect.Width(), thumbHeight);

    // Draw thumb
    renderer.FillRoundedRect(thumbRect, 4.0f, 4.0f, Color::White(0.5f * opacity));
}

void LibraryPage::LoadPreviewTexture(int folderIdx, int previewIdx) {
    if (folderIdx < 0 || folderIdx >= static_cast<int>(m_app->m_folders.size())) return;
    auto& folder = m_app->m_folders[folderIdx];
    if (previewIdx < 0 || previewIdx >= static_cast<int>(folder.previews.size())) return;
    auto& item = folder.previews[previewIdx];

    if (item.texture || item.isLoading || item.loadFailed) return;

    item.isLoading = true;
    m_app->m_imageLoader.LoadAsync(item.filePath, [this, folderIdx, previewIdx](std::shared_ptr<ImageData> img) {
        // Runs on main/render thread inside ProcessPendingTextures
        if (folderIdx < static_cast<int>(m_app->m_folders.size())) {
            auto& f = m_app->m_folders[folderIdx];
            if (previewIdx < static_cast<int>(f.previews.size())) {
                auto& item = f.previews[previewIdx];
                item.isLoading = false;
                if (img) {
                    item.texture = img;
                    CalculateLayout(); // Recompute layout with actual dimensions
                } else {
                    item.loadFailed = true;
                }
                m_app->m_window.RequestRedraw();
            }
        }
    }, 180, 180); // scale to 180px previews
}

void LibraryPage::AddFolder(const std::wstring& path) {
    namespace fs = std::filesystem;
    try {
        fs::path p(path);
        auto normalizedPath = fs::canonical(p).wstring();

        // Check duplicates
        for (const auto& f : m_app->m_folders) {
            if (f.folderPath == normalizedPath) return;
        }

        FolderInfo folder;
        folder.folderPath = normalizedPath;
        folder.folderName = p.filename().wstring();
        if (folder.folderName.empty()) {
            folder.folderName = normalizedPath; // root folder / drive
        }

        // Read files
        for (const auto& entry : fs::directory_iterator(normalizedPath)) {
            if (entry.is_regular_file() && ImageLoader::IsSupported(entry.path().wstring())) {
                folder.filePaths.push_back(entry.path().wstring());
            }
        }

        if (folder.filePaths.empty()) return; // skip folder if it contains no images

        // Sort naturally
        std::sort(folder.filePaths.begin(), folder.filePaths.end());

        // Prepare previews grid
        folder.previews.resize(folder.filePaths.size());
        for (size_t i = 0; i < folder.filePaths.size(); ++i) {
            folder.previews[i].filePath = folder.filePaths[i];
        }
        folder.previewsInitialized = false;

        m_app->m_folders.push_back(std::move(folder));
        CalculateLayout();
    } catch (...) {}
}

void LibraryPage::SetCurrentFolder(int index) {
    if (index < 0 || index >= static_cast<int>(m_app->m_folders.size())) return;
    
    m_app->m_currentFolderIdx = index;
    auto& folder = m_app->m_folders[index];

    // Build files list in App
    m_app->m_fileList = folder.filePaths;
    m_app->m_currentIndex = folder.currentIndex;

    // Load active image in app
    if (!m_app->m_fileList.empty()) {
        if (m_app->m_currentIndex < 0 || m_app->m_currentIndex >= static_cast<int>(m_app->m_fileList.size())) {
            m_app->m_currentIndex = 0;
            folder.currentIndex = 0;
        }
        m_app->LoadImage(m_app->m_fileList[m_app->m_currentIndex]);
    }

    CalculateLayout();
    m_app->m_window.RequestRedraw();
}

void LibraryPage::SyncCurrentFolderWithApp() {
    if (m_app->m_fileList.empty()) return;

    namespace fs = std::filesystem;
    try {
        fs::path activeFilePath(m_app->m_fileList[0]);
        auto activeFolderPath = fs::canonical(activeFilePath.parent_path()).wstring();

        // Check if this folder is in the list
        int foundIdx = -1;
        for (size_t i = 0; i < m_app->m_folders.size(); ++i) {
            if (m_app->m_folders[i].folderPath == activeFolderPath) {
                foundIdx = static_cast<int>(i);
                break;
            }
        }

        if (foundIdx == -1) {
            AddFolder(activeFolderPath);
            foundIdx = static_cast<int>(m_app->m_folders.size()) - 1;
        }

        m_app->m_currentFolderIdx = foundIdx;
        m_app->m_folders[foundIdx].currentIndex = m_app->m_currentIndex;
        CalculateLayout();
    } catch (...) {}
}

void LibraryPage::CalculateLayout() {
    if (m_app->m_currentFolderIdx < 0 || m_app->m_currentFolderIdx >= static_cast<int>(m_app->m_folders.size())) {
        return;
    }

    auto& folder = m_app->m_folders[m_app->m_currentFolderIdx];
    auto size = m_app->m_renderer.GetTargetSize().ToFloat();
    float cxp = size.width / 2.0f;
    float rightPanelLeft = cxp + CENTER_OFFSET;
    float rightPanelWidth = size.width - rightPanelLeft;

    // Calculate dynamic column count
    int colsCount = static_cast<int>((rightPanelWidth - PREVIEW_GAP) / (PREVIEW_WIDTH + PREVIEW_GAP));
    if (colsCount < 1) colsCount = 1;

    std::vector<float> colHeights(colsCount, 10.0f); // 10px top margin

    float layoutWidth = colsCount * PREVIEW_WIDTH + (colsCount - 1) * PREVIEW_GAP;
    float startX = rightPanelLeft + (rightPanelWidth - layoutWidth) / 2.0f;

    for (size_t i = 0; i < folder.previews.size(); ++i) {
        auto& item = folder.previews[i];

        // Find column with minimum height
        int minCol = 0;
        float minHeight = colHeights[0];
        for (int c = 1; c < colsCount; ++c) {
            if (colHeights[c] < minHeight) {
                minHeight = colHeights[c];
                minCol = c;
            }
        }

        // Determine height
        float itemW = PREVIEW_WIDTH;
        float itemH = PREVIEW_WIDTH; // default aspect ratio 1.0 (square placeholder)
        if (item.texture && item.texture->width > 0) {
            itemH = (PREVIEW_WIDTH / item.texture->width) * item.texture->height;
        }

        float x = startX + minCol * (PREVIEW_WIDTH + PREVIEW_GAP);
        float y = colHeights[minCol];

        item.rect = RectF::FromXYWH(x, y, itemW, itemH);

        colHeights[minCol] += itemH + PREVIEW_GAP;
    }
}

// ─── Input Routing ────────────────────────────────────────────────────────────

bool LibraryPage::OnMouseMove(float x, float y, KeyModifiers mods) {
    auto size = m_app->m_renderer.GetTargetSize().ToFloat();
    float cxp = size.height; // Wait, cxp is center x
    cxp = size.width / 2.0f;

    // 1. Scrollbar Dragging
    if (m_scrollCapture != ScrollCapture::None) {
        float deltaY = y - m_dragStartY;
        if (m_scrollCapture == ScrollCapture::Folders) {
            float contentHeight = m_app->m_folders.size() * FOLDER_ITEM_HEIGHT + 20.0f;
            float scrollRatio = deltaY / (size.height - 20.0f);
            m_targetFoldersScrollY = m_dragStartScrollY - scrollRatio * contentHeight;
            float maxScroll = contentHeight - size.height;
            m_targetFoldersScrollY = std::clamp(m_targetFoldersScrollY, -maxScroll, 0.0f);
        } else if (m_scrollCapture == ScrollCapture::Previews && m_app->m_currentFolderIdx >= 0) {
            auto& folder = m_app->m_folders[m_app->m_currentFolderIdx];
            float rightContentHeight = 0.0f;
            for (const auto& item : folder.previews) {
                if (item.rect.bottom > rightContentHeight) {
                    rightContentHeight = item.rect.bottom;
                }
            }
            rightContentHeight += 20.0f;
            float scrollRatio = deltaY / (size.height - 20.0f);
            folder.targetScrollOffset = m_dragStartScrollY - scrollRatio * rightContentHeight;
            float maxScroll = rightContentHeight - size.height;
            folder.targetScrollOffset = std::clamp(folder.targetScrollOffset, -maxScroll, 0.0f);
        }
        m_app->m_window.RequestRedraw();
        return true;
    }

    // 2. Hover Detection
    int newHoveredFolderIdx = -1;
    int newHoveredPreviewIdx = -1;

    // Left Panel Hover
    if (x < cxp - CENTER_OFFSET) {
        float relativeY = y - m_foldersScrollY;
        int folderIdx = static_cast<int>((relativeY - 10.0f) / FOLDER_ITEM_HEIGHT);
        if (folderIdx >= 0 && folderIdx < static_cast<int>(m_app->m_folders.size())) {
            newHoveredFolderIdx = folderIdx;
        }
    }
    // Right Panel Hover
    else if (x > cxp + CENTER_OFFSET && m_app->m_currentFolderIdx >= 0) {
        auto& folder = m_app->m_folders[m_app->m_currentFolderIdx];
        float relativeY = y - folder.scrollOffset;
        for (int i = 0; i < static_cast<int>(folder.previews.size()); ++i) {
            if (folder.previews[i].rect.Contains(x, relativeY)) {
                newHoveredPreviewIdx = i;
                break;
            }
        }
    }

    bool changed = false;

    if (newHoveredFolderIdx != m_hoveredFolderIdx) {
        m_hoveredFolderIdx = newHoveredFolderIdx;
        changed = true;
    }

    if (newHoveredPreviewIdx != m_hoveredPreviewIdx) {
        m_hoveredPreviewIdx = newHoveredPreviewIdx;
        changed = true;

        if (m_hoveredPreviewIdx != -1) {
            auto& folder = m_app->m_folders[m_app->m_currentFolderIdx];
            std::wstring path = folder.previews[m_hoveredPreviewIdx].filePath;
            m_hoveredOriginalPath = path;
            m_hoveredOriginalImage = nullptr;
            m_hoveredOriginalAnim = nullptr;
            m_hoveredOriginalLoading = true;

            if (ImageLoader::IsAnimatedFormat(path)) {
                m_app->m_imageLoader.LoadAnimatedAsync(path, [this, path](std::shared_ptr<AnimatedImageData> anim) {
                    if (m_hoveredOriginalPath == path) {
                        if (anim && anim->IsValid()) {
                            m_hoveredOriginalAnim = anim;
                            m_hoveredOriginalLoading = false;
                            std::vector<double> delays;
                            delays.reserve(anim->frames.size());
                            for (const auto& f : anim->frames) delays.push_back(f.delayMs);
                            m_hoveredAnimSequencer.SetFrames(delays, anim->loopCount);
                            m_app->m_window.RequestRedraw();
                        } else {
                            // Fallback to static loading since it was not actually animated (e.g. static png/webp)
                            m_app->m_imageLoader.LoadAsync(path, [this, path](std::shared_ptr<ImageData> img) {
                                if (m_hoveredOriginalPath == path) {
                                    m_hoveredOriginalImage = img;
                                    m_hoveredOriginalLoading = false;
                                    m_app->m_window.RequestRedraw();
                                }
                            });
                        }
                    }
                });
            } else {
                m_app->m_imageLoader.LoadAsync(path, [this, path](std::shared_ptr<ImageData> img) {
                    if (m_hoveredOriginalPath == path) {
                        m_hoveredOriginalImage = img;
                        m_hoveredOriginalLoading = false;
                        m_app->m_window.RequestRedraw();
                    }
                });
            }
        } else {
            m_hoveredOriginalPath = L"";
            m_hoveredOriginalImage = nullptr;
            m_hoveredOriginalAnim = nullptr;
            m_hoveredOriginalLoading = false;
        }
    }

    if (changed) {
        m_app->m_window.RequestRedraw();
    }

    if (m_hoveredPreviewIdx != -1) {
        return true;
    }

    return false;
}

bool LibraryPage::OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (btn != MouseButton::Left) return false;

    auto size = m_app->m_renderer.GetTargetSize().ToFloat();
    float cxp = size.width / 2.0f;

    // Check Scrollbars Capture
    // Left panel scrollbar
    float leftContentHeight = m_app->m_folders.size() * FOLDER_ITEM_HEIGHT + 20.0f;
    if (leftContentHeight > size.height) {
        RectF trackRect = RectF::FromXYWH(cxp - CENTER_OFFSET - SCROLLBAR_WIDTH - 5.0f, 10.0f, SCROLLBAR_WIDTH, size.height - 20.0f);
        if (trackRect.Contains(x, y)) {
            m_scrollCapture = ScrollCapture::Folders;
            m_dragStartY = y;
            m_dragStartScrollY = m_foldersScrollY;
            return true;
        }
    }

    // Right panel scrollbar
    if (m_app->m_currentFolderIdx >= 0 && m_app->m_currentFolderIdx < static_cast<int>(m_app->m_folders.size())) {
        auto& folder = m_app->m_folders[m_app->m_currentFolderIdx];
        float rightContentHeight = 0.0f;
        for (const auto& item : folder.previews) {
            if (item.rect.bottom > rightContentHeight) {
                rightContentHeight = item.rect.bottom;
            }
        }
        rightContentHeight += 20.0f;

        if (rightContentHeight > size.height) {
            RectF trackRect = RectF::FromXYWH(cxp + CENTER_OFFSET + 5.0f, 10.0f, SCROLLBAR_WIDTH, size.height - 20.0f);
            if (trackRect.Contains(x, y)) {
                m_scrollCapture = ScrollCapture::Previews;
                m_dragStartY = y;
                m_dragStartScrollY = folder.scrollOffset;
                return true;
            }
        }
    }

    // Click inside previews grid -> Open in Viewer Page (Page 2)
    if (x > cxp + CENTER_OFFSET && m_app->m_currentFolderIdx >= 0) {
        auto& folder = m_app->m_folders[m_app->m_currentFolderIdx];
        float relativeY = y - folder.scrollOffset;
        for (int i = 0; i < static_cast<int>(folder.previews.size()); ++i) {
            if (folder.previews[i].rect.Contains(x, relativeY)) {
                // Set active index in app
                m_app->m_currentIndex = i;
                folder.currentIndex = i;
                m_app->LoadImage(folder.filePaths[i]);

                // Switch to Viewer Page
                m_app->SetPage(Page::Viewer);
                return true;
            }
        }
    }

    // Click inside folders list
    if (x < cxp - CENTER_OFFSET && m_hoveredFolderIdx != -1) {
        SetCurrentFolder(m_hoveredFolderIdx);
        return true;
    }

    return false;
}

bool LibraryPage::OnMouseUp(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (btn == MouseButton::Left && m_scrollCapture != ScrollCapture::None) {
        m_scrollCapture = ScrollCapture::None;
        m_app->m_window.RequestRedraw();
        return true;
    }
    return false;
}

bool LibraryPage::OnMouseWheel(float delta, float x, float y, KeyModifiers mods) {
    auto size = m_app->m_renderer.GetTargetSize().ToFloat();
    float cxp = size.width / 2.0f;

    float scrollAmount = 120.0f * (delta > 0 ? 1 : -1);

    if (x < cxp - CENTER_OFFSET) {
        // Scroll Left Panel (folders)
        float contentHeight = m_app->m_folders.size() * FOLDER_ITEM_HEIGHT + 20.0f;
        m_targetFoldersScrollY += scrollAmount;
        float maxScroll = contentHeight - size.height;
        m_targetFoldersScrollY = std::clamp(m_targetFoldersScrollY, -maxScroll, 0.0f);
        if (contentHeight <= size.height) m_targetFoldersScrollY = 0.0f;
        m_app->m_window.RequestRedraw();
        return true;
    } else if (x > cxp + CENTER_OFFSET && m_app->m_currentFolderIdx >= 0) {
        // Scroll Right Panel (previews)
        auto& folder = m_app->m_folders[m_app->m_currentFolderIdx];
        float rightContentHeight = 0.0f;
        for (const auto& item : folder.previews) {
            if (item.rect.bottom > rightContentHeight) {
                rightContentHeight = item.rect.bottom;
            }
        }
        rightContentHeight += 20.0f;

        folder.targetScrollOffset += scrollAmount;
        float maxScroll = rightContentHeight - size.height;
        folder.targetScrollOffset = std::clamp(folder.targetScrollOffset, -maxScroll, 0.0f);
        if (rightContentHeight <= size.height) folder.targetScrollOffset = 0.0f;
        m_app->m_window.RequestRedraw();
        return true;
    }

    return false;
}

bool LibraryPage::OnMouseDoubleClick(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (btn != MouseButton::Left) return false;

    auto size = m_app->m_renderer.GetTargetSize().ToFloat();
    float cxp = size.width / 2.0f;

    // Double-click preview -> Open in Viewer Page
    if (x > cxp + CENTER_OFFSET && m_app->m_currentFolderIdx >= 0) {
        auto& folder = m_app->m_folders[m_app->m_currentFolderIdx];
        float relativeY = y - folder.scrollOffset;
        for (int i = 0; i < static_cast<int>(folder.previews.size()); ++i) {
            if (folder.previews[i].rect.Contains(x, relativeY)) {
                // Set active index in app
                m_app->m_currentIndex = i;
                folder.currentIndex = i;
                m_app->LoadImage(folder.filePaths[i]);

                // Switch to Viewer Page
                m_app->SetPage(Page::Viewer);
                return true;
            }
        }
    }
    return false;
}

void LibraryPage::OnKeyDown(uint32_t vk, KeyModifiers mods) {
    // Custom library page navigation (if any)
}

void LibraryPage::DrawCheckerboard(Renderer& renderer, const RectF& rect) {
    if (!m_checkerboardBitmap) {
        constexpr int size = 10;
        constexpr int total = size * 2;
        std::vector<uint8_t> pixels(total * total * 4);
        for (int y = 0; y < total; ++y) {
            for (int x = 0; x < total; ++x) {
                int idx = (y * total + x) * 4;
                bool isLight = ((x / size) + (y / size)) % 2 == 0;
                uint8_t c = isLight ? 40 : 20;
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
    float tileW = 20.0f, tileH = 20.0f;
    for (float y = rect.top; y < rect.bottom; y += tileH) {
        for (float x = rect.left; x < rect.right; x += tileW) {
            RectF tile = RectF::FromXYWH(x, y, tileW, tileH);
            renderer.DrawBitmap(m_checkerboardBitmap.Get(), tile, 0.4f, BitmapInterpolation::NearestNeighbor);
        }
    }
    renderer.PopClip();
}

} // namespace kv
