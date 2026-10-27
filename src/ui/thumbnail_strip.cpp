// ============================================================================
// Class: ThumbnailStrip
// Description: ThumbnailStrip manages the reorderable row of image previews displayed at the bottom of the screen in [IMAGE_VIEWER_APP_NAME].
// Responsibility: Draw selection markers, re-ordering layout guides, and handle thumbnail click events.
// ============================================================================
#include "common/pch.h"
#include "thumbnail_strip.h"
#include "core/app.h"
#include "common/performance_trace.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <unordered_map>

namespace kv {

void ThumbnailStrip::CancelPointerGesture() {
    m_draggingThumbnails = m_groupSelecting = m_hasDragged = false;
    m_insertIndex = m_hoveredIndex = -1;
    m_mouseMoveTimer = 0;
    m_opacity = m_targetOpacity = 0;
}

void ThumbnailStrip::Initialize(App* app) {
    m_app = app;
    m_layoutWidth = GetThumbnailWidth();
    m_scrollX = 0.0f;
    m_targetScrollX = 0.0f;
    m_opacity = 0.0f;
    m_targetOpacity = 0.0f;
    m_selected.clear();
    m_activeIndex = -1;
    m_mouseMoveTimer = 0.0;
}

void ThumbnailStrip::SetFileList(const std::vector<std::wstring>& fileList) {
    constexpr size_t none = std::numeric_limits<size_t>::max();
    // Local per-path chain consumes old occurrences in order and remembers the first new entry.
    struct ReuseState {
        size_t nextOld = std::numeric_limits<size_t>::max();
        size_t firstNew = std::numeric_limits<size_t>::max();
    };
    auto previous = std::move(m_thumbnails);
    std::unordered_map<std::wstring, ReuseState> byPath;
    byPath.reserve(previous.size());
    std::vector<size_t> following(previous.size(), none);
    for (size_t i = previous.size(); i > 0;) {
        --i;
        auto& state = byPath[previous[i].path];
        following[i] = state.nextOld;
        state.nextOld = i;
    }
    std::vector<ThumbnailInfo> newThumbnails;
    newThumbnails.reserve(fileList.size());

    for (const auto& path : fileList) {
        ThumbnailInfo info;
        info.path = path;

        auto& state = byPath[path];
        if (state.nextOld != none) {
            const size_t index = state.nextOld;
            state.nextOld = following[index];
            info = std::move(previous[index]);
        } else if (state.firstNew != none) {
            // Preserve the first occurrence's cache, but never its pending request.
            const auto& cached = newThumbnails[state.firstNew];
            info.texture = cached.texture;
            info.loadFailed = cached.loadFailed;
        }
        if (state.firstNew == none) state.firstNew = newThumbnails.size();
        newThumbnails.push_back(std::move(info));
    }

    m_thumbnails = std::move(newThumbnails);
    m_selected.assign(m_thumbnails.size(), false);
    m_activeIndex = -1;
    RefreshActiveIndex(m_app ? m_app->m_currentIndex : -1);
}

void ThumbnailStrip::CancelPendingLoads() {
    for (auto& item : m_thumbnails) {
        item.loadLifetime.reset();
        item.pendingRequest.reset();
        item.isLoading = false;
    }
}

void ThumbnailStrip::SyncSelection(int currentIndex) {
    RefreshActiveIndex(currentIndex);
    if (m_draggingThumbnails || m_groupSelecting) return;
    for (size_t i = 0; i < m_selected.size(); ++i) {
        m_selected[i] = (static_cast<int>(i) == currentIndex);
    }
    if (currentIndex >= 0 && currentIndex < static_cast<int>(m_thumbnails.size())) {
        m_targetScrollX = GetCenteredScroll(currentIndex);
    }
}

bool ThumbnailStrip::HasActiveAnimations() const {
    return std::abs(m_targetScrollX - m_scrollX) > 0.1f ||
           std::abs(m_targetOpacity - m_opacity) > 0.01f ||
           m_mouseMoveTimer > 0.0;
}

void ThumbnailStrip::Update(double dt) {
    if (m_thumbnails.empty()) return;
    const int previousActive = m_activeIndex;
    RefreshActiveIndex(m_app->m_currentIndex);
    if ((m_activeIndex != previousActive || m_layoutWidth != GetThumbnailWidth()) && m_activeIndex >= 0 && !m_draggingThumbnails && !m_groupSelecting) {
        m_targetScrollX = GetCenteredScroll(m_activeIndex);
    }
    m_layoutWidth = GetThumbnailWidth();
    const auto [minScroll, maxScroll] = GetScrollLimits();
    m_targetScrollX = std::clamp(m_targetScrollX, minScroll, maxScroll);

    // Update mouse move timer
    if (m_mouseMoveTimer > 0.0) {
        m_mouseMoveTimer -= dt;
        if (m_mouseMoveTimer < 0.0) m_mouseMoveTimer = 0.0;
    }

    float lerpFactor = 1.0f - std::exp(-15.0f * static_cast<float>(dt));
    const float previousScroll=m_scrollX;
    const float previousOpacity=m_opacity;
    m_scrollX = std::lerp(m_scrollX, m_targetScrollX, lerpFactor);
    m_opacity = std::lerp(m_opacity, m_targetOpacity, lerpFactor);

    if (std::abs(m_targetScrollX - m_scrollX) < 0.1f) m_scrollX = m_targetScrollX;
    if (std::abs(m_targetOpacity - m_opacity) < 0.01f) m_opacity = m_targetOpacity;
    if (m_scrollX != previousScroll || m_opacity != previousOpacity) m_app->m_window.RequestRedraw();
    m_scrollStableFrames=m_scrollX==previousScroll ? std::min(2u,m_scrollStableFrames+1) : 0;

    // Check mouse autohide trigger
    POINT pt;
    if (GetCursorPos(&pt) && ScreenToClient(m_app->m_window.GetHwnd(), &pt)) {
        auto targetSize = m_app->m_renderer.GetTargetSize().ToFloat();
        if (pt.x >= 0 && pt.x <= targetSize.width && pt.y >= 0 && pt.y <= targetSize.height) {
            if (pt.y >= GetBounds().top || m_app->m_viewerPage.IsInfoBarHit(static_cast<float>(pt.x), static_cast<float>(pt.y))) {
                m_targetOpacity = 1.0f;
                m_mouseMoveTimer = 2.0; // Keep active while hovering
            } else if (m_mouseMoveTimer > 0.0) {
                m_targetOpacity = 1.0f;
            } else if (!m_draggingThumbnails && !m_groupSelecting) {
                m_targetOpacity = 0.0f;
            }
        } else if (m_mouseMoveTimer > 0.0) {
            m_targetOpacity = 1.0f;
        } else if (!m_draggingThumbnails && !m_groupSelecting) {
            m_targetOpacity = 0.0f;
        }
    } else {
        if (m_mouseMoveTimer > 0.0) {
            m_targetOpacity = 1.0f;
        } else if (!m_draggingThumbnails && !m_groupSelecting) {
            m_targetOpacity = 0.0f;
        }
    }

    // Prepare destination after the first original frame; keep intermediate animation, not its obsolete queue.
    if (m_opacity > 0.0f || (m_app->m_asyncLoading && !m_app->m_originalFramePending && m_app->m_currentImage)) {
        const auto size=m_app->m_renderer.GetTargetSize().ToFloat();
        const auto [first,end]=GetVisibleRange(size.width);
        const auto [targetFirst,targetEnd]=GetVisibleRange(size.width,m_targetScrollX);
        for (size_t i=0;i<m_thumbnails.size();++i) {
            if ((i<first || i>=end) && (i<targetFirst || i>=targetEnd)) {
                auto& item=m_thumbnails[i];
                if (item.isLoading) { item.loadLifetime.reset(); item.pendingRequest.reset(); item.isLoading=false; }
            }
        }
        if (m_app->m_currentIndex>=0) LoadThumbnail(m_app->m_currentIndex,true);
        for (size_t i=targetFirst;i<targetEnd;++i) LoadThumbnail(static_cast<int>(i),true);
        if (m_opacity>0.0f) for (size_t i=first;i<end;++i) {
            const auto rect=GetThumbnailRect(static_cast<int>(i),size.width,size.height);
            if (rect.right>=0 && rect.left<=size.width) LoadThumbnail(static_cast<int>(i));
        }
        const bool ready=targetFirst<targetEnd && std::all_of(m_thumbnails.begin()+targetFirst,m_thumbnails.begin()+targetEnd,
            [](const ThumbnailInfo& item) { return item.texture!=nullptr; });
        if (ready) {
            PerformanceTrace::Event("strip.target.prepared",m_app->m_imageGeneration,targetEnd-targetFirst);
            if (m_opacity>0 && (std::abs(m_scrollX-m_targetScrollX)<0.1f || m_scrollStableFrames>=2))
                PerformanceTrace::Event("strip.viewport.ready",m_app->m_imageGeneration,targetEnd-targetFirst);
        }
    }
}

void ThumbnailStrip::LoadThumbnail(int index, bool destination) {
    if (index < 0 || index >= static_cast<int>(m_thumbnails.size())) return;
    auto& info = m_thumbnails[index];

    if (!info.texture && !info.isLoading && !info.loadFailed) {
        const auto request = std::make_shared<const std::wstring>(info.path);
        PerformanceTrace::Event("strip.request",m_app->m_imageGeneration,index);
        info.pendingRequest = request;
        info.loadLifetime = std::make_shared<const bool>(true);
        info.isLoading = true;
        
        m_app->m_imageLoader.LoadThumbnailAsync(info.path, [this, request](std::shared_ptr<ImageData> data) {
            CompleteThumbnail(request, std::move(data));
        }, 100, [this, request]() { return IsThumbnailRequestCurrent(request); }, info.loadLifetime, destination); // decimate/scale to 100x100
    }
}

bool ThumbnailStrip::IsThumbnailRequestCurrent(const ImageRequestToken& request) const {
    return request && std::any_of(m_thumbnails.begin(), m_thumbnails.end(),
        [&request](const ThumbnailInfo& item) {
            return item.pendingRequest == request && item.path == *request;
        });
}

void ThumbnailStrip::CompleteThumbnail(const ImageRequestToken& request, std::shared_ptr<ImageData> image) {
    if (!request) return;
    auto it = std::find_if(m_thumbnails.begin(), m_thumbnails.end(),
        [&request](const ThumbnailInfo& item) {
            return item.pendingRequest == request && item.path == *request;
        });
    if (it == m_thumbnails.end()) return;
    it->pendingRequest.reset();
    it->loadLifetime.reset();
    it->isLoading = false;
    if (image && image->gpuBitmap) {
        it->texture = image->gpuBitmap;
        PerformanceTrace::Event("strip.ready",m_app->m_imageGeneration);
    } else {
        it->loadFailed = true;
        PerformanceTrace::Event("strip.failed",m_app->m_imageGeneration);
    }
    m_app->m_window.RequestRedraw();
}

float ThumbnailStrip::GetUiScale() const {
    return m_app ? m_app->m_window.GetDpiScale() : 1.0f;
}

float ThumbnailStrip::GetThumbnailWidth() const {
    const float width = m_app ? m_app->m_settings.thumbnail_width : THUMBNAIL_WIDTH;
    return (std::isfinite(width) && width > 0 ? width : THUMBNAIL_WIDTH) * GetUiScale();
}

void ThumbnailStrip::RefreshActiveIndex(int preferredIndex) {
    const std::wstring* path = nullptr;
    if (m_app && m_app->m_animatedImage) path = &m_app->m_animatedImage->filePath;
    else if (m_app && m_app->m_currentImage) path = &m_app->m_currentImage->filePath;
    const auto matches = [&](int index) {
        return path && index >= 0 && index < static_cast<int>(m_thumbnails.size()) && m_thumbnails[index].path == *path;
    };
    if (matches(preferredIndex)) { m_activeIndex = preferredIndex; return; }
    if (matches(m_activeIndex)) return;
    m_activeIndex = -1;
    if (path) {
        auto found = std::find_if(m_thumbnails.begin(), m_thumbnails.end(), [&](const ThumbnailInfo& item) { return item.path == *path; });
        if (found != m_thumbnails.end()) m_activeIndex = static_cast<int>(found - m_thumbnails.begin());
    }
}

RectF ThumbnailStrip::GetThumbnailRect(int index, float windowWidth, float windowHeight) const {
    return GetThumbnailRect(index, windowWidth, windowHeight, m_scrollX);
}

RectF ThumbnailStrip::GetThumbnailRect(int index, float windowWidth, float windowHeight, float scrollX) const {
    const float width = GetThumbnailWidth();
    const float extra = (ACTIVE_SCALE - 1.0f) * width;
    const float offset = (width + THUMBNAIL_GAP * GetUiScale()) * index + ((m_activeIndex >= 0 && index > m_activeIndex) ? extra : 0);
    const float side = index == m_activeIndex ? ACTIVE_SCALE * width : width;
    const float bottom = windowHeight - 20.0f * GetUiScale();
    return RectF::FromXYWH((windowWidth - width) / 2 + scrollX + offset, bottom - side, side, side);
}

RectF ThumbnailStrip::GetBounds() const {
    if (!m_app) return {};
    const auto size = m_app->m_renderer.GetTargetSize().ToFloat();
    if (m_thumbnails.empty()) return RectF::FromXYWH(0, size.height, size.width, 0);
    const float side = GetThumbnailWidth() * (m_activeIndex >= 0 ? ACTIVE_SCALE : 1.0f);
    const float top = std::max(0.0f, size.height - side - 22.0f * GetUiScale());
    return RectF(0, top, size.width, size.height);
}

float ThumbnailStrip::GetActiveCenter(float windowWidth) const {
    return m_activeIndex >= 0 ? GetThumbnailRect(m_activeIndex, windowWidth, 0).Center().x : windowWidth / 2;
}

float ThumbnailStrip::GetCenteredScroll(int index) const {
    const auto rect = GetThumbnailRect(index, 0, 0, 0);
    return -rect.Center().x;
}

std::pair<float, float> ThumbnailStrip::GetScrollLimits() const {
    return m_thumbnails.empty() ? std::pair{0.0f, 0.0f} :
        std::pair{GetCenteredScroll(static_cast<int>(m_thumbnails.size()) - 1), GetCenteredScroll(0)};
}

int ThumbnailStrip::HitTest(float x, float y) const {
    if (!GetBounds().Contains(x, y)) return -1;
    const auto size = m_app->m_renderer.GetTargetSize().ToFloat();
    const auto [first, end] = GetVisibleRange(size.width);
    size_t lo = first, hi = end;
    while (lo < hi) {
        const size_t mid = lo + (hi - lo) / 2;
        if (GetThumbnailRect(static_cast<int>(mid), size.width, size.height).right < x) lo = mid + 1;
        else hi = mid;
    }
    return lo < end && GetThumbnailRect(static_cast<int>(lo), size.width, size.height).Contains(x, y) ? static_cast<int>(lo) : -1;
}

int ThumbnailStrip::GetInsertionIndex(float x, float windowWidth) const {
    size_t lo = 0, hi = m_thumbnails.size();
    while (lo < hi) {
        const size_t mid = lo + (hi - lo) / 2;
        if (GetThumbnailRect(static_cast<int>(mid), windowWidth, 0).Center().x < x) lo = mid + 1;
        else hi = mid;
    }
    return static_cast<int>(lo);
}

std::pair<size_t, size_t> ThumbnailStrip::GetVisibleRange(float windowWidth) const {
    return GetVisibleRange(windowWidth,m_scrollX);
}

std::pair<size_t, size_t> ThumbnailStrip::GetVisibleRange(float windowWidth, float scrollX) const {
    const size_t count = m_thumbnails.size();
    const float width = GetThumbnailWidth();
    if (!std::isfinite(windowWidth) || !std::isfinite(scrollX) ||
        !std::isfinite(width) || width <= 0.0f || windowWidth < 0.0f ||
        !std::isfinite((windowWidth - width) / 2.0f + scrollX) ||
        count > static_cast<size_t>(std::numeric_limits<int>::max())) return {0, count};

    // Use the actual float geometry at both inclusive edges, avoiding division/rounding drift.
    auto rectAt = [&](size_t index) {
        return GetThumbnailRect(static_cast<int>(index), windowWidth, 0, scrollX);
    };
    size_t first = 0;
    size_t end = count;
    while (first < end) {
        const size_t mid = first + (end - first) / 2;
        if (rectAt(mid).right < 0.0f) first = mid + 1;
        else end = mid;
    }
    const size_t begin = first;
    end = count;
    while (first < end) {
        const size_t mid = first + (end - first) / 2;
        if (rectAt(mid).left <= windowWidth) first = mid + 1;
        else end = mid;
    }
    return {begin, first};
}

void ThumbnailStrip::DrawMirror(Renderer& renderer, ID2D1Bitmap1* bitmap, const RectF& rect, float opacity) {
    D2D1_POINT_2F pivot = D2D1::Point2F(rect.Center().x, rect.bottom);
    renderer.PushTransform(D2D1::Matrix3x2F::Scale(1.0f, -1.0f, pivot));
    renderer.DrawBitmap(bitmap, rect, opacity * 0.25f);
    renderer.PopTransform();
}

void ThumbnailStrip::Draw(Renderer& renderer) {
    if (detail::g_enableDebugIpc) {
        std::string traceMsg = "[THUMBSTRIP] Draw: empty=" + std::to_string(m_thumbnails.empty()) + ", opacity=" + std::to_string(m_opacity);
        std::ofstream dbg("debug_ipc.txt", std::ios::app);
        dbg << traceMsg << std::endl;
        dbg.close();
    }
    if (m_thumbnails.empty() || m_opacity == 0.0f) return;

    auto targetSize = renderer.GetTargetSize().ToFloat();
    float windowWidth = targetSize.width;
    float windowHeight = targetSize.height;

    renderer.PushClip(GetBounds());

    const auto [first, end] = GetVisibleRange(windowWidth);
    m_app->m_stripReadyDrawnThisFrame=first<end && (std::abs(m_scrollX-m_targetScrollX)<0.1f || m_scrollStableFrames>=2) &&
        std::all_of(m_thumbnails.begin()+first,m_thumbnails.begin()+end,[](const ThumbnailInfo& item) { return item.texture!=nullptr; });
    for (size_t i = first; i < end; ++i) {
        RectF rect = GetThumbnailRect(static_cast<int>(i), windowWidth, windowHeight);
        if (rect.right < 0 || rect.left > windowWidth) continue;

        bool isSelected = m_selected[i];
        bool isHovered = (static_cast<int>(i) == m_hoveredIndex);

        const bool isActive = static_cast<int>(i) == m_activeIndex;
        RectF drawRect = rect;
        float thumbOpacity = (isActive || isHovered || isSelected) ? 1.0f : 0.5f;
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
            std::wstring indicator = m_app->m_settings.draw_default_thumbnail ? L"⧖" : L"...";
            renderer.DrawText(indicator, drawRect, Color::White(0.5f * thumbOpacity), 12.0f, TextAlign::Center, TextVAlign::Center);
        } else if (m_thumbnails[i].loadFailed) {
            // Draw error placeholder text
            renderer.DrawText(L"?!", drawRect, Color::FromRGBA(200, 50, 50, static_cast<uint8_t>(200 * thumbOpacity)), 14.0f, TextAlign::Center, TextVAlign::Center);
        }

        if (isActive) {
            renderer.DrawRect(drawRect, Color::FromRGBA(0, 240, 255, static_cast<uint8_t>(255 * m_opacity)), 2.0f * GetUiScale());
        }
        if (isSelected) {
            const float scale = GetUiScale();
            const auto badge = RectF::FromXYWH(rect.right - 18 * scale, rect.top + 3 * scale, 15 * scale, 15 * scale);
            renderer.FillRoundedRect(badge, 3 * scale, 3 * scale, Color::Black(0.8f * m_opacity));
            renderer.DrawLine({badge.left + 3 * scale, badge.top + 7 * scale}, {badge.left + 6 * scale, badge.top + 10 * scale}, Color::White(m_opacity), 2 * scale);
            renderer.DrawLine({badge.left + 6 * scale, badge.top + 10 * scale}, {badge.right - 3 * scale, badge.top + 4 * scale}, Color::White(m_opacity), 2 * scale);
        }
    }

    // 4. Draw red insertion cursor indicator
    if (m_draggingThumbnails && m_insertIndex >= 0 && m_insertIndex <= static_cast<int>(m_thumbnails.size())) {
        float tw = GetThumbnailWidth();
        const auto insertion = GetThumbnailRect(m_insertIndex, windowWidth, windowHeight);
        float ix = insertion.left;
        float iy = insertion.top;
        
        PointF pTop = { ix, iy };
        PointF pBottom = { ix, iy + tw };

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
    return !m_thumbnails.empty() && m_opacity > 0 && GetBounds().Contains(x, y);
}

bool ThumbnailStrip::HandleMouseMove(float x, float y, KeyModifiers mods) {
    if (m_thumbnails.empty()) return false;

    auto targetSize = m_app->m_renderer.GetTargetSize().ToFloat();
    if (GetBounds().Contains(x, y)) {
        m_mouseMoveTimer = 2.0;
        m_targetOpacity = 1.0f;
    }

    if (m_opacity == 0.0f) {
        m_app->m_window.RequestRedraw();
        return false;
    }

    float windowWidth = targetSize.width;
    float windowHeight = targetSize.height;

    m_hoveredIndex = HitTest(x, y);

    if (m_groupSelecting) {
        m_selectionRect.left = std::min(m_selectionStart.x, x);
        m_selectionRect.top = std::min(m_selectionStart.y, y);
        m_selectionRect.right = std::max(m_selectionStart.x, x);
        m_selectionRect.bottom = std::max(m_selectionStart.y, y);
 
        // Selection is limited to the same visible client region as pointer hits and drawing.
        const auto bounds = GetBounds();
        const auto visibleSelection = RectF(std::max(bounds.left, m_selectionRect.left), std::max(bounds.top, m_selectionRect.top),
            std::min(bounds.right, m_selectionRect.right), std::min(bounds.bottom, m_selectionRect.bottom));
        for (size_t i = 0; i < m_thumbnails.size(); ++i) {
            RectF r = GetThumbnailRect(static_cast<int>(i), windowWidth, windowHeight);
            m_selected[i] = visibleSelection.Width() > 0 && visibleSelection.Height() > 0 && visibleSelection.Intersects(r);
        }
        m_app->m_window.RequestRedraw();
        return true;
    }
 
    if (m_draggingThumbnails) {
        if (std::hypot(x - m_dragStartPos.x, y - m_dragStartPos.y) > 5.0f) {
            m_hasDragged = true;
        }
 
        if (m_hasDragged) {
            m_insertIndex = GetInsertionIndex(x, windowWidth);
            m_insertIndex = std::clamp(m_insertIndex, 0, static_cast<int>(m_thumbnails.size()));
            
            // Adjust scroll dynamically if dragging near edges
            if (x < 60.0f) {
                m_targetScrollX = std::min(GetScrollLimits().second, m_targetScrollX + 5.0f);
            } else if (x > windowWidth - 60.0f) {
                m_targetScrollX = std::max(GetScrollLimits().first, m_targetScrollX - 5.0f);
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

            int idx = HitTest(x, y);
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
                int idx = HitTest(x, y);
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
        float tw = GetThumbnailWidth();
        const auto [minScroll, maxScroll] = GetScrollLimits();
        m_targetScrollX += (delta > 0) ? tw * 2.0f : -tw * 2.0f;
        m_targetScrollX = std::clamp(m_targetScrollX, minScroll, maxScroll);
        m_app->m_window.RequestRedraw();
        return true;
    }

    return false;
}

bool ThumbnailStrip::HandleMouseDoubleClick(MouseButton btn, float x, float y, KeyModifiers mods) {
    return IsMouseOver(x, y);
}

void ThumbnailStrip::HandleMouseLeave() {
    m_hoveredIndex = -1;
    m_app->m_window.RequestRedraw();
}

void ThumbnailStrip::Rearrange(int insertIndex) {
    if (insertIndex < 0 || insertIndex > static_cast<int>(m_thumbnails.size())) return;
    std::vector<size_t> selected, remaining;
    for (size_t i = 0; i < m_thumbnails.size(); ++i) {
        if (m_selected[i]) {
            selected.push_back(i);
        } else remaining.push_back(i);
    }
    if (selected.empty()) return;
    const auto position = std::clamp(insertIndex, 0, static_cast<int>(remaining.size()));
    remaining.insert(remaining.begin() + position, selected.begin(), selected.end());
    std::vector<std::wstring> paths;
    paths.reserve(remaining.size());
    int active = -1;
    for (size_t i = 0; i < remaining.size(); ++i) {
        paths.push_back(m_thumbnails[remaining[i]].path);
        if (static_cast<int>(remaining[i]) == m_activeIndex) active = static_cast<int>(i);
    }
    m_app->m_fileList = paths;
    if (active >= 0) m_app->m_currentIndex = active;
    SetFileList(paths);
    m_draggingThumbnails = false;
    m_app->m_window.RequestRedraw();
}

void ThumbnailStrip::ShowContextMenu(float screenX, float screenY) {
    HMENU hMenu = CreatePopupMenu();
    HMENU hSortSub = CreatePopupMenu();
    int currentMode = m_app->m_settings.sort_mode;

    AppendMenuW(hSortSub, MF_STRING | (currentMode == 0 ? MF_CHECKED : 0), 10, L"Name (Ascending)");
    AppendMenuW(hSortSub, MF_STRING | (currentMode == 1 ? MF_CHECKED : 0), 11, L"Name (Descending)");
    AppendMenuW(hSortSub, MF_STRING | (currentMode == 2 ? MF_CHECKED : 0), 12, L"Date (Ascending)");
    AppendMenuW(hSortSub, MF_STRING | (currentMode == 3 ? MF_CHECKED : 0), 13, L"Date (Descending)");
    AppendMenuW(hSortSub, MF_STRING | (currentMode == 4 ? MF_CHECKED : 0), 14, L"Size (Ascending)");
    AppendMenuW(hSortSub, MF_STRING | (currentMode == 5 ? MF_CHECKED : 0), 15, L"Size (Descending)");

    AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hSortSub), L"Sort By");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, 6, L"Save order metadata to disk");
    AppendMenuW(hMenu, MF_STRING, 7, L"Toggle fullscreen mode");

    int selected = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                                  static_cast<int>(screenX), static_cast<int>(screenY),
                                  0, m_app->m_window.GetHwnd(), nullptr);
    DestroyMenu(hMenu);

    if (selected == 0) return;

    if (selected >= 10 && selected <= 15) {
        m_app->ApplySortMode(selected - 10);
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
}

bool ThumbnailStrip::HandleKeyDown(uint32_t vk, KeyModifiers mods) {
    if (m_thumbnails.empty() || m_opacity == 0.0f) return false;

    auto targetSize = m_app->m_renderer.GetTargetSize().ToFloat();
    float windowWidth = targetSize.width;
    float tw = GetThumbnailWidth();
    const auto [minScroll, maxScroll] = GetScrollLimits();

    if (vk == VK_PRIOR) { // Page Up
        m_targetScrollX = std::min(maxScroll, m_targetScrollX + std::max(tw, windowWidth - tw * 2.0f));
        m_app->m_window.RequestRedraw();
        return true;
    } else if (vk == VK_NEXT) { // Page Down
        m_targetScrollX = std::max(minScroll, m_targetScrollX - std::max(tw, windowWidth - tw * 2.0f));
        m_app->m_window.RequestRedraw();
        return true;
    }
    return false;
}

} // namespace kv
