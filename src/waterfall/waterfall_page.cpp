// ============================================================================
// Class: WaterfallPage
// Description: WaterfallPage implements the virtual scrolling masonry grid layout for browsing folder contents in [IMAGE_VIEWER_APP_NAME].
// Responsibility: Virtual scroll column coordinate maps and modal image viewers.
// ============================================================================
#include "common/pch.h"
#include "waterfall_page.h"
#include "core/app.h"
#include "common/performance_trace.h"
#include <algorithm>
#include <filesystem>

namespace kv {

void WaterfallPage::OpenViewer() {
    if (m_app->m_currentFolderIdx < 0 || m_app->m_currentFolderIdx >= static_cast<int>(m_app->m_folders.size())) return;
    auto& folder = m_app->m_folders[m_app->m_currentFolderIdx];
    const int index = m_hoveredPreviewIdx;
    if (index < 0 || index >= static_cast<int>(folder.filePaths.size())) return;
    m_app->CancelHoverLoads();
    m_app->m_currentIndex = folder.currentIndex = index;
    m_viewerModal = true;
    m_app->m_thumbnailStrip.SetFileList(m_app->m_fileList);
    m_app->m_thumbnailStrip.SyncSelection(index);
    m_app->LoadImage(folder.filePaths[index]);
    m_app->m_viewerPage.ResetView();
    m_app->m_window.RequestRedraw();
}

void WaterfallPage::CloseViewer() {
    m_app->m_viewerPage.CancelPointerGesture();
    m_app->m_thumbnailStrip.CancelPointerGesture();
    m_app->m_thumbnailStrip.CancelPendingLoads();
    m_viewerModal = false;
    m_waterfallBlockActiveItem = true;
    m_unblockTimer = 1.0;
    m_app->m_window.RequestRedraw();
}

void WaterfallPage::Initialize(App* app) {
    m_app = app;
    m_layoutValid = false;
    m_scrollOffset = 0.0f;
    m_targetScrollOffset = 0.0f;
    m_columnsNumber = 0; // Auto columns
    m_gridSpacing = 15.0f;
    m_cornerRadius = 20.0f;
    m_hoveredPreviewIdx = -1;
    m_waterfallBlockActiveItem = false;
    m_unblockTimer = 0.0;
    m_viewerModal = false;
    m_scrollbarCaptured = false;
}

void WaterfallPage::Update(double dt) {
    if (m_viewerModal) {
        m_app->m_viewerPage.Update(dt);
        return;
    }

    // 1. Smooth scroll interpolation
    if (std::abs(m_targetScrollOffset - m_scrollOffset) > 0.1f) {
        m_scrollOffset += static_cast<float>((m_targetScrollOffset - m_scrollOffset) * 15.0 * dt);
        m_app->m_window.RequestRedraw();
    } else {
        m_scrollOffset = m_targetScrollOffset;
    }

    // 2. Unblock hovered item timer
    if (m_waterfallBlockActiveItem) {
        m_unblockTimer -= dt;
        if (m_unblockTimer <= 0.0) {
            m_waterfallBlockActiveItem = false;
        }
    }

    // 3. Update hovered animation sequencer
    if (m_hoveredOriginalAnim && !m_hoveredAnimSequencer.IsPaused()) {
        if (m_hoveredAnimSequencer.Update(dt)) {
            m_app->m_window.RequestRedraw();
        }
    }
}

void WaterfallPage::Draw(Renderer& renderer) {
    auto size = renderer.GetTargetSize().ToFloat();

    if (m_viewerModal) {
        // Draw backplate: draw the waterfall grid dimmed down
        
        // Draw grid
        DrawPreviewsGrid(renderer, RectF::FromXYWH(0, 0, size.width, size.height));
        
        // Draw dark translucent overlay (the blackplate)
        renderer.FillRect(RectF::FromXYWH(0, 0, size.width, size.height), Color::Black(0.66f));
        
        // Draw the modal viewer content on top
        m_app->DrawViewerComposition(renderer);
        return;
    }

    // Normal mode: Draw Previews and Scrollbars
    RectF rect = RectF::FromXYWH(0, 0, size.width, size.height);
    DrawPreviewsGrid(renderer, rect);
    DrawScrollbars(renderer, rect);
}

void WaterfallPage::CalculateLayout() {
    if (m_app->m_currentFolderIdx < 0 || m_app->m_currentFolderIdx >= static_cast<int>(m_app->m_folders.size())) {
        m_previewRects.clear();
        m_layoutValid = false;
        return;
    }

    auto& folder = m_app->m_folders[m_app->m_currentFolderIdx];
    auto size = m_app->m_renderer.GetTargetSize().ToFloat();
    const auto revision = m_app->m_folderData.GetPreviewLayoutRevision();
    if (m_layoutValid && m_layoutRevision == revision && m_layoutFolder == m_app->m_currentFolderIdx &&
        m_layoutWidth == size.width && m_layoutCount == folder.previews.size() &&
        m_layoutColumns == m_columnsNumber && m_layoutSpacing == m_gridSpacing) return;

    // 1. Calculate columns count
    float scrollbarWidth = 10.0f;
    float waterfallWidth = size.width - scrollbarWidth * 10.0f;
    if (waterfallWidth < 100.0f) waterfallWidth = size.width;

    int calcColumns = static_cast<int>((waterfallWidth + m_gridSpacing) / (200.0f + m_gridSpacing));
    if (calcColumns < 1) calcColumns = 1;

    int colsCount = calcColumns;
    if (m_columnsNumber > 0) {
        colsCount = std::min(m_columnsNumber, calcColumns);
    }

    // 2. Initialize column heights (offset by top padding 90px)
    std::vector<float> colHeights(colsCount, 90.0f);
    m_previewRects.resize(folder.previews.size());

    float layoutWidth = colsCount * 200.0f + (colsCount - 1) * m_gridSpacing;
    float startX = (size.width - layoutWidth) / 2.0f;

    // 3. Position previews using a greedy algorithm
    for (size_t i = 0; i < folder.previews.size(); ++i) {
        auto& item = folder.previews[i];

        // Find the column with the minimum height
        int minCol = 0;
        float minHeight = colHeights[0];
        for (int c = 1; c < colsCount; ++c) {
            if (colHeights[c] < minHeight) {
                minHeight = colHeights[c];
                minCol = c;
            }
        }

        float itemW = 200.0f;
        float itemH = 200.0f; // Default square fallback

        if (item.texture && item.texture->width > 0) {
            itemH = (200.0f / item.texture->width) * item.texture->height;
        }

        float x = startX + minCol * (200.0f + m_gridSpacing);
        float y = colHeights[minCol];

        m_previewRects[i] = RectF::FromXYWH(x, y, itemW, itemH);
        colHeights[minCol] += itemH + m_gridSpacing;
    }
    m_layoutRevision = revision;
    m_layoutFolder = m_app->m_currentFolderIdx;
    m_layoutWidth = size.width;
    m_layoutCount = folder.previews.size();
    m_layoutColumns = m_columnsNumber;
    m_layoutSpacing = m_gridSpacing;
    m_layoutValid = true;
}

RectF WaterfallPage::EnlargeActiveItemRect(const RectF& rect) const {
    float newW = rect.Width() + (200.0f / 3.0f);
    float newH = (newW / rect.Width()) * rect.Height();
    return RectF::Centered(rect.Center().x, rect.Center().y, newW, newH);
}

void WaterfallPage::DrawPreviewsGrid(Renderer& renderer, const RectF& rect) {
    if (m_app->m_currentFolderIdx < 0 || m_app->m_currentFolderIdx >= static_cast<int>(m_app->m_folders.size())) {
        renderer.DrawText(L"NO FOLDER SELECCIONADA", rect, Color::White(0.4f), 18.0f, TextAlign::Center, TextVAlign::Center);
        return;
    }

    CalculateLayout();

    auto size = renderer.GetTargetSize().ToFloat();
    auto& folder = m_app->m_folders[m_app->m_currentFolderIdx];

    // Push layout clipping (excluding scrollbar zones)
    renderer.PushClip(rect);

    int activeIdx = m_hoveredPreviewIdx;

    ComPtr<ID2D1Factory> factory;
    renderer.GetD2DContext()->GetFactory(&factory);

    // Cancel queued/decode work outside the current viewport without releasing reusable textures.
    for (size_t i=0;i<folder.previews.size();++i) {
        const auto& r=m_previewRects[i];
        if (r.bottom+m_scrollOffset<0 || r.top+m_scrollOffset>size.height) {
            auto& item=folder.previews[i];
            if (item.isLoading) { item.loadLifetime.reset(); item.pendingRequest.reset(); item.isLoading=false; }
        }
    }
    bool allVisibleReady=true;
    size_t visibleCount=0;
    // Draw normal previews (using virtual scrolling)
    for (size_t i = 0; i < folder.previews.size(); ++i) {
        if (static_cast<int>(i) == activeIdx) {
            // Hovered item drawn last
            continue;
        }

        auto& item = folder.previews[i];
        RectF rawRect = m_previewRects[i];
        RectF drawRect = rawRect;
        drawRect.top += m_scrollOffset;
        drawRect.bottom += m_scrollOffset;

        // Virtual scrolling: skip offscreen elements
        if (drawRect.bottom < 0 || drawRect.top > size.height) {
            continue;
        }

        ++visibleCount;
        allVisibleReady &= item.texture && item.texture->HasGpuBitmap();
        if (item.texture && item.texture->HasGpuBitmap()) {
            bool rounded = (m_cornerRadius > 0.0f);
            ComPtr<ID2D1Layer> layer;

            if (rounded) {
                ComPtr<ID2D1RoundedRectangleGeometry> roundedRectGeo;
                factory->CreateRoundedRectangleGeometry(
                    D2D1::RoundedRect(drawRect.ToD2D(), m_cornerRadius, m_cornerRadius),
                    &roundedRectGeo
                );

                renderer.GetD2DContext()->CreateLayer(&layer);
                renderer.GetD2DContext()->PushLayer(
                    D2D1::LayerParameters(D2D1::InfiniteRect(), roundedRectGeo.Get(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE),
                    layer.Get()
                );
            }

            if (item.texture->hasAlpha) {
                DrawCheckerboard(renderer, drawRect);
            }

            renderer.DrawBitmap(item.texture->gpuBitmap.Get(), drawRect, 0.85f, BitmapInterpolation::HighQuality);

            if (rounded) {
                renderer.GetD2DContext()->PopLayer();
            }
        } else {
            // Placeholder
            renderer.FillRoundedRect(drawRect, m_cornerRadius, m_cornerRadius, Color::FromRGBA(25, 25, 25, 120));
            renderer.DrawRoundedRect(drawRect, m_cornerRadius, m_cornerRadius, Color::White(0.08f), 1.0f);
            renderer.DrawText(L"CARGANDO...", drawRect, Color::White(0.3f), 11.0f, TextAlign::Center, TextVAlign::Center);
            LoadPreviewTexture(m_app->m_currentFolderIdx, static_cast<int>(i));
        }
    }

    if (activeIdx>=0 && activeIdx<static_cast<int>(folder.previews.size())) {
        const auto& active=folder.previews[activeIdx];
        allVisibleReady &= active.texture && active.texture->HasGpuBitmap();
        if (!active.texture && !active.isLoading && !active.loadFailed) LoadPreviewTexture(m_app->m_currentFolderIdx,activeIdx);
    }
    const bool ready=allVisibleReady && visibleCount>0 && std::abs(m_scrollOffset-m_targetScrollOffset)<0.1f;
    m_app->m_waterfallReadyDrawnThisFrame=ready && m_readyLastFrame && m_readyRevision==m_layoutRevision;
    m_readyLastFrame=ready; m_readyRevision=m_layoutRevision;
    if (m_app->m_waterfallReadyDrawnThisFrame) PerformanceTrace::Event("waterfall.viewport.ready",m_app->m_imageGeneration,visibleCount);
    // Draw enlarged hovered preview on top of other cells (with shadow)
    if (activeIdx >= 0 && activeIdx < static_cast<int>(folder.previews.size()) && !m_viewerModal) {
        auto& item = folder.previews[activeIdx];
        RectF rawRect = m_previewRects[activeIdx];
        RectF drawRect = EnlargeActiveItemRect(rawRect);
        drawRect.top += m_scrollOffset;
        drawRect.bottom += m_scrollOffset;

        if (item.texture && item.texture->HasGpuBitmap()) {
            // Draw background drop shadow
            RectF shadowRect = drawRect;
            shadowRect.left -= 6.0f;
            shadowRect.top -= 6.0f;
            shadowRect.right += 6.0f;
            shadowRect.bottom += 6.0f;
            renderer.FillRoundedRect(shadowRect, m_cornerRadius + 4.0f, m_cornerRadius + 4.0f, Color::Black(0.5f));

            bool rounded = (m_cornerRadius > 0.0f);
            ComPtr<ID2D1Layer> layer;

            if (rounded) {
                ComPtr<ID2D1RoundedRectangleGeometry> roundedRectGeo;
                factory->CreateRoundedRectangleGeometry(
                    D2D1::RoundedRect(drawRect.ToD2D(), m_cornerRadius, m_cornerRadius),
                    &roundedRectGeo
                );

                renderer.GetD2DContext()->CreateLayer(&layer);
                renderer.GetD2DContext()->PushLayer(
                    D2D1::LayerParameters(D2D1::InfiniteRect(), roundedRectGeo.Get(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE),
                    layer.Get()
                );
            }

            if (item.texture->hasAlpha) {
                DrawCheckerboard(renderer, drawRect);
            }

            // Draw bitmap
            ID2D1Bitmap1* animBitmap = nullptr;
            if (m_hoveredOriginalAnim && m_hoveredOriginalAnim->IsValid() && !m_hoveredAnimSequencer.IsPaused()) {
                int frameIdx = m_hoveredAnimSequencer.GetCurrentFrame();
                if (frameIdx >= 0 && frameIdx < static_cast<int>(m_hoveredOriginalAnim->frames.size())) {
                    animBitmap = m_hoveredOriginalAnim->frames[frameIdx].gpuBitmap.Get();
                }
            }

            ID2D1Bitmap1* drawBmp = animBitmap ? animBitmap : item.texture->gpuBitmap.Get();
            renderer.DrawBitmap(drawBmp, drawRect, 1.0f, BitmapInterpolation::HighQuality);

            if (rounded) {
                renderer.GetD2DContext()->PopLayer();
            }

            renderer.DrawRoundedRect(drawRect, m_cornerRadius, m_cornerRadius, Color::White(0.9f), 1.5f);
        }
    }

    renderer.PopClip();
}

void WaterfallPage::DrawScrollbars(Renderer& renderer, const RectF& rect) {
    if (m_app->m_currentFolderIdx < 0 || m_app->m_currentFolderIdx >= static_cast<int>(m_app->m_folders.size())) {
        return;
    }

    auto size = renderer.GetTargetSize().ToFloat();

    // Compute layout width and coordinates
    float scrollbarWidth = 10.0f;
    float waterfallWidth = size.width - scrollbarWidth * 10.0f;
    if (waterfallWidth < 100.0f) waterfallWidth = size.width;

    int calcColumns = static_cast<int>((waterfallWidth + m_gridSpacing) / (200.0f + m_gridSpacing));
    if (calcColumns < 1) calcColumns = 1;
    int colsCount = calcColumns;
    if (m_columnsNumber > 0) colsCount = std::min(m_columnsNumber, calcColumns);

    float layoutWidth = colsCount * 200.0f + (colsCount - 1) * m_gridSpacing;
    float startX = (size.width - layoutWidth) / 2.0f;

    RectF contentRect = RectF::FromXYWH(startX, 0.0f, layoutWidth, size.height);

    // Compute heights
    float contentHeight = 90.0f;
    for (const auto& item : m_previewRects) {
        if (item.bottom > contentHeight) {
            contentHeight = item.bottom;
        }
    }
    contentHeight += 70.0f; // bottom margin

    if (contentHeight <= size.height) {
        return; // No scrollbars needed
    }

    float opacity = m_scrollbarCaptured ? 0.9f : 0.4f;
    float verticalOffset = 50.0f;
    float scrollbarHeight = size.height - verticalOffset * 1.1f;
    float offsetTimes = 4.0f;

    // Draw Left Scrollbar
    RectF leftTrack = RectF::FromXYWH(
        contentRect.left - scrollbarWidth * offsetTimes,
        verticalOffset,
        scrollbarWidth,
        scrollbarHeight
    );
    DrawScrollbar(renderer, leftTrack, contentHeight, size.height, m_scrollOffset, opacity);

    // Draw Right Scrollbar
    RectF rightTrack = RectF::FromXYWH(
        contentRect.right + scrollbarWidth * (offsetTimes - 1.0f),
        verticalOffset,
        scrollbarWidth,
        scrollbarHeight
    );
    DrawScrollbar(renderer, rightTrack, contentHeight, size.height, m_scrollOffset, opacity);
}

void WaterfallPage::DrawScrollbar(Renderer& renderer, const RectF& trackRect, float contentHeight, float viewHeight, float scrollOffset, float opacity) {
    // Track background
    renderer.FillRoundedRect(trackRect, 4.0f, 4.0f, Color::White(0.05f * opacity));

    // Thumb height
    float viewRatio = viewHeight / contentHeight;
    float thumbHeight = std::max(40.0f, trackRect.Height() * viewRatio);

    // Thumb Y position
    float maxScroll = contentHeight - viewHeight;
    float scrollRatio = std::clamp(std::abs(scrollOffset) / maxScroll, 0.0f, 1.0f);
    float thumbY = trackRect.top + scrollRatio * (trackRect.Height() - thumbHeight);

    RectF thumbRect = RectF::FromXYWH(trackRect.left, thumbY, trackRect.Width(), thumbHeight);

    // Thumb foreground
    renderer.FillRoundedRect(thumbRect, 4.0f, 4.0f, Color::White(0.5f * opacity));
}

void WaterfallPage::LoadPreviewTexture(int folderIdx, int previewIdx) {
    if (folderIdx < 0 || folderIdx >= static_cast<int>(m_app->m_folders.size())) return;
    auto& folder = m_app->m_folders[folderIdx];
    if (previewIdx < 0 || previewIdx >= static_cast<int>(folder.previews.size())) return;
    auto& item = folder.previews[previewIdx];

    if (item.texture || item.isLoading || item.loadFailed) return;

    const auto request = std::make_shared<const std::wstring>(item.filePath);
    PerformanceTrace::Event("waterfall.request",m_app->m_imageGeneration,previewIdx);
    item.pendingRequest = request;
    item.loadLifetime = std::make_shared<const bool>(true);
    item.isLoading = true;
    m_app->m_imageLoader.LoadThumbnailAsync(item.filePath, [this, request](std::shared_ptr<ImageData> img) {
        const bool hasImage = img != nullptr;
        if (m_app->m_folderData.CompletePreview(request, std::move(img))) {
            // CompletePreview advances geometry revision; Draw rebuilds once after the delivery batch.
            if (hasImage) PerformanceTrace::Event("waterfall.ready",m_app->m_imageGeneration);
            else PerformanceTrace::Event("waterfall.failed",m_app->m_imageGeneration);
            m_app->m_window.RequestRedraw();
        }
    }, 200, [this, request]() { return m_app->m_folderData.IsPreviewRequestCurrent(request); }, item.loadLifetime); // 200px width previews
}

void WaterfallPage::DrawCheckerboard(Renderer& renderer, const RectF& rect) {
    if (!m_checkerboardBitmap) {
        constexpr int size = 15;
        constexpr int total = size * 2;
        std::vector<uint8_t> pixels(total * total * 4);
        const uint8_t lightGray = 40;
        const uint8_t darkGray  = 20;

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
    float tileW = 30.0f, tileH = 30.0f;
    for (float y = rect.top; y < rect.bottom; y += tileH) {
        for (float x = rect.left; x < rect.right; x += tileW) {
            RectF tile = RectF::FromXYWH(x, y, tileW, tileH);
            renderer.DrawBitmap(m_checkerboardBitmap.Get(), tile, 0.5f, BitmapInterpolation::NearestNeighbor);
        }
    }
    renderer.PopClip();
}

// ─── Input Events ────────────────────────────────────────────────────────────

bool WaterfallPage::OnMouseMove(float x, float y, KeyModifiers mods) {
    if (m_viewerModal) {
        m_app->m_viewerPage.OnMouseMove(x, y, mods);
        return true;
    }

    auto size = m_app->m_renderer.GetTargetSize().ToFloat();

    // 1. Scrollbar Dragging
    if (m_scrollbarCaptured && m_app->m_currentFolderIdx >= 0) {
        float contentHeight = 90.0f;
        for (const auto& rect : m_previewRects) {
            if (rect.bottom > contentHeight) {
                contentHeight = rect.bottom;
            }
        }
        contentHeight += 70.0f;

        float deltaY = y - m_dragStartY;
        float trackHeight = size.height - 100.0f;
        float scrollRatio = deltaY / trackHeight;

        m_targetScrollOffset = m_dragStartScrollOffset - scrollRatio * contentHeight;
        float maxScroll = contentHeight - size.height;
        m_targetScrollOffset = std::clamp(m_targetScrollOffset, -maxScroll, 0.0f);

        m_app->m_window.RequestRedraw();
        return true;
    }

    // 2. Hover detection
    int newHoveredPreviewIdx = -1;
    if (m_app->m_currentFolderIdx >= 0) {
        auto& folder = m_app->m_folders[m_app->m_currentFolderIdx];
        float relativeY = y - m_scrollOffset;

        // Check active item enlarged rect first (prevents micro-jitter when cursor leaves the normal bounds but stays in enlarged bounds)
        int activeIdx = m_hoveredPreviewIdx;
        if (activeIdx >= 0 && activeIdx < static_cast<int>(folder.previews.size())) {
            RectF enlarged = EnlargeActiveItemRect(m_previewRects[activeIdx]);
            if (enlarged.Contains(x, relativeY)) {
                newHoveredPreviewIdx = activeIdx;
            }
        }

        // If not hovered inside enlarged rect, check normal list
        if (newHoveredPreviewIdx == -1) {
            for (int i = 0; i < static_cast<int>(folder.previews.size()); ++i) {
                if (m_previewRects[i].Contains(x, relativeY)) {
                    newHoveredPreviewIdx = i;
                    break;
                }
            }
        }
    }

    if (newHoveredPreviewIdx != m_hoveredPreviewIdx && !m_waterfallBlockActiveItem) {
        m_hoverLoadLifetime.reset();
        m_hoveredPreviewIdx = newHoveredPreviewIdx;
        m_app->m_window.RequestRedraw();

        // Handle animation loading on hover
        if (m_hoveredPreviewIdx != -1) {
            auto& folder = m_app->m_folders[m_app->m_currentFolderIdx];
            std::wstring path = folder.previews[m_hoveredPreviewIdx].filePath;
            m_hoveredOriginalPath = path;
            m_hoverLoadLifetime = std::make_shared<const bool>(true);
            if (ImageLoader::IsAnimatedFormat(path)) {
                m_hoveredOriginalAnim = nullptr;
                m_hoveredOriginalLoading = true;
                
                m_app->m_imageLoader.LoadAnimatedAsync(path, [this, path](std::shared_ptr<AnimatedImageData> anim) {
                    if (m_hoveredOriginalPath == path) {
                        m_hoveredOriginalAnim = anim;
                        m_hoveredOriginalLoading = false;
                        if (anim && anim->IsValid()) {
                            std::vector<double> delays;
                            delays.reserve(anim->frames.size());
                            for (const auto& f : anim->frames) delays.push_back(f.delayMs);
                            m_hoveredAnimSequencer.SetFrames(delays, anim->loopCount);
                        }
                        m_app->m_window.RequestRedraw();
                    }
                }, m_hoverLoadLifetime);
            } else {
                m_hoveredOriginalAnim = nullptr;
                m_hoveredOriginalLoading = false;
            }
        } else {
            m_hoveredOriginalPath = L"";
            m_hoveredOriginalAnim = nullptr;
            m_hoveredOriginalLoading = false;
        }
    }

    return m_hoveredPreviewIdx != -1;
}

bool WaterfallPage::OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (m_viewerModal) {
        if (btn == MouseButton::Left && !m_app->m_viewerPage.IsInfoBarHit(x, y) && !m_app->m_viewerPage.HasPointerGesture() && !m_app->m_viewerPage.IsPointOverImage(x, y)) {
            CloseViewer();
            return true;
        }
        m_app->m_viewerPage.OnMouseDown(btn, x, y, mods);
        return true;
    }

    if (btn == MouseButton::Right) {
        POINT pt;
        if (GetCursorPos(&pt)) {
            HMENU hMenu = CreatePopupMenu();
            if (m_hoveredPreviewIdx != -1) {
                AppendMenuW(hMenu, MF_STRING, 10, L"Open in Modal Viewer (Enter)");
                AppendMenuW(hMenu, MF_STRING, 11, L"Tagging Modal... (T)");
                AppendMenuW(hMenu, MF_STRING, 12, L"Delete Image (Del)");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
            }

            AppendMenuW(hMenu, MF_STRING, 13, L"Viewer (NUM1)");
            HMENU hSortSub = CreatePopupMenu();
            int currentMode = m_app->m_settings.sort_mode;

            AppendMenuW(hSortSub, MF_STRING | (currentMode == 0 ? MF_CHECKED : 0), 30, L"Name (Ascending)");
            AppendMenuW(hSortSub, MF_STRING | (currentMode == 1 ? MF_CHECKED : 0), 31, L"Name (Descending)");
            AppendMenuW(hSortSub, MF_STRING | (currentMode == 2 ? MF_CHECKED : 0), 32, L"Date (Ascending)");
            AppendMenuW(hSortSub, MF_STRING | (currentMode == 3 ? MF_CHECKED : 0), 33, L"Date (Descending)");
            AppendMenuW(hSortSub, MF_STRING | (currentMode == 4 ? MF_CHECKED : 0), 34, L"Size (Ascending)");
            AppendMenuW(hSortSub, MF_STRING | (currentMode == 5 ? MF_CHECKED : 0), 35, L"Size (Descending)");

            AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hSortSub), L"Sort By");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

            AppendMenuW(hMenu, MF_STRING, 1, L"Settings... (O)");
            AppendMenuW(hMenu, MF_STRING, 2, L"Toggle Fullscreen (F11)");
            AppendMenuW(hMenu, MF_STRING, 3, L"Exit (Esc)");

            int selected = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                                          pt.x, pt.y, 0, m_app->m_window.GetHwnd(), nullptr);
            DestroyMenu(hMenu);

            if (selected == 13) {
                m_app->SetPage(Page::Viewer);
            } else if (selected == 1) {
                m_app->OnKeyDown('O', {});
            } else if (selected == 2) {
                m_app->OnKeyDown(VK_F11, {});
            } else if (selected == 3) {
                m_app->OnKeyDown(VK_ESCAPE, {});
            } else if (selected >= 30 && selected <= 35) {
                m_app->ApplySortMode(selected - 30);
            } else if (selected == 10) {
                OpenViewer();
            } else if (selected == 11) {
                m_app->OnKeyDown('T', {});
            } else if (selected == 12) {
                m_app->OnKeyDown(VK_DELETE, {});
            }
        }
        return true;
    }

    if (btn != MouseButton::Left) return false;

    auto size = m_app->m_renderer.GetTargetSize().ToFloat();

    // 1. Check scrollbar tracks
    if (m_app->m_currentFolderIdx >= 0) {
        float contentHeight = 90.0f;
        for (const auto& rect : m_previewRects) {
            if (rect.bottom > contentHeight) {
                contentHeight = rect.bottom;
            }
        }
        contentHeight += 70.0f;

        if (contentHeight > size.height) {
            // Scrollbars positions
            float scrollbarWidth = 10.0f;
            float waterfallWidth = size.width - scrollbarWidth * 10.0f;
            if (waterfallWidth < 100.0f) waterfallWidth = size.width;
            int calcColumns = static_cast<int>((waterfallWidth + m_gridSpacing) / (200.0f + m_gridSpacing));
            if (calcColumns < 1) calcColumns = 1;
            int colsCount = calcColumns;
            if (m_columnsNumber > 0) colsCount = std::min(m_columnsNumber, calcColumns);

            float layoutWidth = colsCount * 200.0f + (colsCount - 1) * m_gridSpacing;
            float startX = (size.width - layoutWidth) / 2.0f;

            float verticalOffset = 50.0f;
            float scrollbarHeight = size.height - verticalOffset * 1.1f;
            float offsetTimes = 4.0f;

            RectF leftTrack = RectF::FromXYWH(startX - scrollbarWidth * offsetTimes, verticalOffset, scrollbarWidth, scrollbarHeight);
            RectF rightTrack = RectF::FromXYWH(startX + layoutWidth + scrollbarWidth * (offsetTimes - 1.0f), verticalOffset, scrollbarWidth, scrollbarHeight);

            if (leftTrack.Contains(x, y) || rightTrack.Contains(x, y)) {
                m_scrollbarCaptured = true;
                m_dragStartY = y;
                m_dragStartScrollOffset = m_scrollOffset;
                m_app->m_window.RequestRedraw();
                return true;
            }
        }
    }

    // 2. Click previews
    if (m_app->m_currentFolderIdx >= 0) {
        auto& folder = m_app->m_folders[m_app->m_currentFolderIdx];
        float relativeY = y - m_scrollOffset;

        int clickedIdx = -1;
        
        // Check enlarged rect first
        int activeIdx = m_hoveredPreviewIdx;
        if (activeIdx >= 0 && activeIdx < static_cast<int>(folder.previews.size())) {
            RectF enlarged = EnlargeActiveItemRect(m_previewRects[activeIdx]);
            if (enlarged.Contains(x, relativeY)) {
                clickedIdx = activeIdx;
            }
        }

        // Check normal list
        if (clickedIdx == -1) {
            for (int i = 0; i < static_cast<int>(folder.previews.size()); ++i) {
                if (m_previewRects[i].Contains(x, relativeY)) {
                    clickedIdx = i;
                    break;
                }
            }
        }

        if (clickedIdx != -1) {
            if (m_waterfallBlockActiveItem) {
                m_waterfallBlockActiveItem = false;
                m_hoveredPreviewIdx = clickedIdx;
            }

            m_hoveredPreviewIdx = clickedIdx;
            OpenViewer();
            return true;
        }
    }

    return false;
}

bool WaterfallPage::OnMouseUp(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (m_viewerModal) {
        m_app->m_viewerPage.OnMouseUp(btn, x, y, mods);
        return true;
    }

    if (btn == MouseButton::Left && m_scrollbarCaptured) {
        m_scrollbarCaptured = false;
        m_app->m_window.RequestRedraw();
        return true;
    }
    return false;
}

bool WaterfallPage::OnMouseWheel(float delta, float x, float y, KeyModifiers mods) {
    if (m_viewerModal) {
        m_app->m_viewerPage.OnMouseWheel(delta, x, y, mods);
        return true;
    }

    auto size = m_app->m_renderer.GetTargetSize().ToFloat();

    // 1. Column changes (Ctrl + Wheel)
    if (mods.ctrl && !mods.shift && !mods.alt) {
        int dir = (delta > 0) ? 1 : -1;
        
        // Calculate max cols limit dynamically
        float scrollbarWidth = 10.0f;
        float waterfallWidth = size.width - scrollbarWidth * 10.0f;
        int maxCols = static_cast<int>((waterfallWidth + m_gridSpacing) / (200.0f + m_gridSpacing));
        if (maxCols < 1) maxCols = 1;

        int desired = m_columnsNumber;
        if (desired == 0) {
            // Find current active layout columns
            desired = maxCols;
        }
        desired += dir;
        desired = std::clamp(desired, 0, maxCols);
        
        m_columnsNumber = desired;
        CalculateLayout();
        m_app->m_window.RequestRedraw();
        return true;
    }

    // 2. Grid gap spacing changes (Ctrl + Shift + Wheel)
    if (mods.ctrl && mods.shift && !mods.alt) {
        float dir = (delta > 0) ? 5.0f : -5.0f;
        m_gridSpacing = std::clamp(m_gridSpacing + dir, 0.0f, 100.0f);
        CalculateLayout();
        m_app->m_window.RequestRedraw();
        return true;
    }

    // 3. Corner radius changes (Alt + Wheel)
    if (mods.alt && !mods.shift && !mods.ctrl) {
        float dir = (delta > 0) ? 2.0f : -2.0f;
        m_cornerRadius = std::clamp(m_cornerRadius + dir, 0.0f, 50.0f);
        m_app->m_window.RequestRedraw();
        return true;
    }

    // 4. Scroll (Regular wheel)
    if (m_app->m_currentFolderIdx >= 0) {
        float scrollAmount = 120.0f * (delta > 0 ? 1 : -1);

        float contentHeight = 90.0f;
        for (const auto& rect : m_previewRects) {
            if (rect.bottom > contentHeight) {
                contentHeight = rect.bottom;
            }
        }
        contentHeight += 70.0f;

        m_targetScrollOffset += scrollAmount;
        float maxScroll = contentHeight - size.height;
        m_targetScrollOffset = std::clamp(m_targetScrollOffset, -maxScroll, 0.0f);
        if (contentHeight <= size.height) m_targetScrollOffset = 0.0f;

        m_app->m_window.RequestRedraw();
        return true;
    }

    return false;
}

bool WaterfallPage::OnMouseDoubleClick(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (m_viewerModal) {
        m_app->m_viewerPage.OnMouseDoubleClick(btn, x, y, mods);
        return true;
    }
    return false;
}

bool WaterfallPage::OnKeyDown(uint32_t vk, KeyModifiers mods) {
    if (m_viewerModal) {
        if (vk == VK_NUMPAD1) {
            m_viewerModal = false;
            m_app->SetPage(Page::Viewer);
            m_app->m_window.RequestRedraw();
            return true;
        }
        if (vk == VK_ESCAPE || vk == VK_RETURN || vk == VK_SPACE || vk == 'F') {
            // Escape/Enter leaves modal viewer
            if (vk == VK_ESCAPE || vk == VK_RETURN) {
                CloseViewer();
                return true;
            }
        }
        return m_app->m_viewerPage.OnKeyDown(vk, mods);
    }

    // Normal mode keys
    switch (vk) {
        case VK_RETURN:
        case 'F':
            if (m_hoveredPreviewIdx != -1) {
                OpenViewer();
                return true;
            }
            break;

        case VK_HOME:
            m_targetScrollOffset = 0.0f;
            m_app->m_window.RequestRedraw();
            return true;

        case VK_END: {
            auto size = m_app->m_renderer.GetTargetSize().ToFloat();
            float contentHeight = 90.0f;
            for (const auto& rect : m_previewRects) {
                if (rect.bottom > contentHeight) {
                    contentHeight = rect.bottom;
                }
            }
            contentHeight += 70.0f;
            float maxScroll = contentHeight - size.height;
            m_targetScrollOffset = -maxScroll;
            if (contentHeight <= size.height) m_targetScrollOffset = 0.0f;
            m_app->m_window.RequestRedraw();
            return true;
        }

        case VK_PRIOR: { // Page Up
            auto size = m_app->m_renderer.GetTargetSize().ToFloat();
            m_targetScrollOffset += size.height * 0.9f;
            m_targetScrollOffset = std::min(0.0f, m_targetScrollOffset);
            m_app->m_window.RequestRedraw();
            return true;
        }

        case VK_NEXT: { // Page Down
            auto size = m_app->m_renderer.GetTargetSize().ToFloat();
            float contentHeight = 90.0f;
            for (const auto& rect : m_previewRects) {
                if (rect.bottom > contentHeight) {
                    contentHeight = rect.bottom;
                }
            }
            contentHeight += 70.0f;
            float maxScroll = contentHeight - size.height;
            m_targetScrollOffset -= size.height * 0.9f;
            m_targetScrollOffset = std::max(-maxScroll, m_targetScrollOffset);
            if (contentHeight <= size.height) m_targetScrollOffset = 0.0f;
            m_app->m_window.RequestRedraw();
            return true;
        }
    }

    return false;
}

} // namespace kv
