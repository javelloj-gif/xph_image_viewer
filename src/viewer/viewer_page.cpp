// ============================================================================
// Class: ViewerPage
// Description: ViewerPage implements the single-image display screen with advanced zoom, pan, and visual grids for [IMAGE_VIEWER_APP_NAME].
// Responsibility: Handle fit-to-screen transforms, thirds divisions, and Ctrl+drag spotlights.
// ============================================================================
#include "common/pch.h"
#include "viewer_page.h"
#include "core/app.h"

namespace kv {

static RectF FitRectIntoRect(const RectF& source, const RectF& target) {
    float srcW = source.Width();
    float srcH = source.Height();
    if (srcW <= 0.0f || srcH <= 0.0f) return target;

    float scaleX = target.Width() / srcW;
    float scaleY = target.Height() / srcH;
    float scale = std::min(scaleX, scaleY);

    float fitW = srcW * scale;
    float fitH = srcH * scale;

    float x = target.left + (target.Width() - fitW) / 2.0f;
    float y = target.top + (target.Height() - fitH) / 2.0f;

    return RectF::FromXYWH(x, y, fitW, fitH);
}

void ViewerPage::Initialize(App* app) {
    m_app = app;
    ResetView();
    m_lastZoom = m_zoom;
}

void ViewerPage::Update(double dt) {
    bool panXActive = m_app->m_animations.IsActive("pan_x");
    bool panYActive = m_app->m_animations.IsActive("pan_y");

    if (m_app->m_animations.IsActive("zoom")) {
        float oldZoom = m_zoom;
        m_zoom = m_app->m_animations.GetValue("zoom", m_zoom);

        if (oldZoom > 0.0f) {
            float actualFactor = m_zoom / oldZoom;
            PointF imgCenter = CalculateImageRect().Center();
            if (!panXActive) {
                m_pan.x = (m_zoomAnchor.x - imgCenter.x) - ((m_zoomAnchor.x - imgCenter.x) - m_pan.x) * actualFactor;
            }
            if (!panYActive) {
                m_pan.y = (m_zoomAnchor.y - imgCenter.y) - ((m_zoomAnchor.y - imgCenter.y) - m_pan.y) * actualFactor;
            }
        }
    }

    if (panXActive) {
        m_pan.x = m_app->m_animations.GetValue("pan_x", m_pan.x);
    }
    if (panYActive) {
        m_pan.y = m_app->m_animations.GetValue("pan_y", m_pan.y);
    }

    if (m_app->m_animations.IsActive("region_zoom_factor")) {
        m_regionStageFactor = m_app->m_animations.GetValue("region_zoom_factor", 0.0f);

        m_zoom = m_origZoom + (m_zoomLevelText - m_origZoom) * m_regionStageFactor;
        m_pan.x = m_origPan.x + (m_targetPan.x - m_origPan.x) * m_regionStageFactor;
        m_pan.y = m_origPan.y + (m_targetPan.y - m_origPan.y) * m_regionStageFactor;

        m_inputRectAnimated.left = m_inputRect.left + (m_projectedRect.left - m_inputRect.left) * m_regionStageFactor;
        m_inputRectAnimated.top = m_inputRect.top + (m_projectedRect.top - m_inputRect.top) * m_regionStageFactor;
        m_inputRectAnimated.right = m_inputRect.right + (m_projectedRect.right - m_inputRect.right) * m_regionStageFactor;
        m_inputRectAnimated.bottom = m_inputRect.bottom + (m_projectedRect.bottom - m_inputRect.bottom) * m_regionStageFactor;
    }

    if (std::abs(m_zoom - m_lastZoom) > 0.0001f) {
        m_zoomTextTimer = 1.5;
        m_lastZoom = m_zoom;
        m_app->m_window.RequestRedraw();
    }

    if (m_zoomTextTimer > 0.0) {
        m_zoomTextTimer -= dt;
        if (m_zoomTextTimer <= 0.0) {
            m_zoomTextTimer = 0.0;
            m_zoomTextOpacity = 0.0f;
        } else if (m_zoomTextTimer < 0.5) {
            m_zoomTextOpacity = static_cast<float>(m_zoomTextTimer / 0.5);
        } else {
            m_zoomTextOpacity = 1.0f;
        }
        m_app->m_window.RequestRedraw();
    }

    if (IsInfoBarHit(m_app->m_mouseX, m_app->m_mouseY) ||
        (m_app->IsViewerActive() && m_app->m_thumbnailStrip.IsMouseOver(m_app->m_mouseX, m_app->m_mouseY))) m_mouseInactivityTimer = 0;
    m_mouseInactivityTimer += dt;
    if (m_mouseInactivityTimer >= 2.0) {
        if (m_metadataOpacity > 0.0f) {
            m_metadataOpacity = std::max(0.0f, m_metadataOpacity - static_cast<float>(dt * 2.0));
            m_app->m_window.RequestRedraw();
        }
    } else {
        if (m_metadataOpacity < 1.0f) {
            m_metadataOpacity = std::min(1.0f, m_metadataOpacity + static_cast<float>(dt * 4.0));
            m_app->m_window.RequestRedraw();
        }
    }
}

void ViewerPage::Draw(Renderer& renderer) {
    ID2D1Bitmap1* bitmap = nullptr;

    auto animatedImage = m_app->m_animatedImage;
    auto currentImage = m_app->m_currentImage;

    if (animatedImage && !animatedImage->frames.empty()) {
        int frameIdx = m_app->m_frameSequencer.GetCurrentFrame();
        if (frameIdx >= 0 && frameIdx < static_cast<int>(animatedImage->frames.size())) {
            bitmap = animatedImage->frames[frameIdx].gpuBitmap.Get();
        }
    } else if (currentImage && currentImage->HasGpuBitmap()) {
        bitmap = currentImage->gpuBitmap.Get();
    }

    if (!bitmap) {
        auto size = renderer.GetTargetSize().ToFloat();
        RectF textRect = RectF::FromXYWH(0, 0, size.width, size.height);
        renderer.DrawText(
            L"Drop an image here or pass a file path as argument\n\nWASD / Arrow Keys - Move & Paging\nCtrl+Scroll - Zoom / Browse\nCtrl+Drag - Region Magnifier\nG - Rule of Thirds",
            textRect,
            Color::FromRGBA(180, 180, 180),
            20.0f,
            TextAlign::Center,
            TextVAlign::Center
        );
        return;
    }

    DrawImageFitted(renderer, bitmap);
    DrawRegionZoom(renderer);

    if (m_zoomTextOpacity > 0.0f) {
        auto size = renderer.GetTargetSize().ToFloat();
        float boxW = 140.0f;
        float boxH = 50.0f;
        RectF boxRect = RectF::FromXYWH(
            (size.width - boxW) / 2.0f,
            (size.height - boxH) / 2.0f,
            boxW,
            boxH
        );
        renderer.FillRoundedRect(boxRect, 8.0f, 8.0f, Color::Black(0.6f * m_zoomTextOpacity));
        renderer.DrawRoundedRect(boxRect, 8.0f, 8.0f, Color::White(0.15f * m_zoomTextOpacity), 1.0f);
        std::wstring zoomText = std::format(L"{:.0f}%", std::round(GetDisplayZoom() * 100.0f));
        renderer.DrawText(zoomText, boxRect, Color::White(0.95f * m_zoomTextOpacity), 20.0f, TextAlign::Center, TextVAlign::Center);
    }
}

void ViewerPage::DrawImageFitted(Renderer& renderer, ID2D1Bitmap1* bitmap) {
    if (!bitmap) return;

    auto imageRect = CalculateImageRect();

    auto center = imageRect.Center();
    renderer.PushTranslation(m_pan.x, m_pan.y);
    renderer.PushScale(m_zoom, m_zoom, center);
    if (m_rotation != 0.0f) {
        renderer.PushRotation(m_rotation, center);
    }

    auto interp = m_pixelatedZoom 
        ? BitmapInterpolation::NearestNeighbor 
        : BitmapInterpolation::HighQuality;

    renderer.DrawBitmap(bitmap, imageRect, 1.0f, interp);
    m_app->m_originalDrawnThisFrame=true;

    if (m_rotation != 0.0f) renderer.PopTransform();
    renderer.PopTransform();
    renderer.PopTransform();

    if (m_app->m_settings.show_thirds) {
        DrawThirds(renderer, GetImageTransformedRect());
    }

    DrawOverlayInfo(renderer);

    auto animatedImage = m_app->m_animatedImage;
    if (animatedImage && !m_app->m_frameSequencer.IsPaused()) {
        DrawProgressBar(renderer, GetImageTransformedRect());
    }
}

RectF ViewerPage::CalculateImageRect() const {
    auto currentImage = m_app->m_currentImage;
    auto animatedImage = m_app->m_animatedImage;
    if (!currentImage && !animatedImage) return {};

    float imgW, imgH;
    if (animatedImage && !animatedImage->frames.empty()) {
        imgW = static_cast<float>(animatedImage->frames[0].width);
        imgH = static_cast<float>(animatedImage->frames[0].height);
    } else if (currentImage) {
        imgW = static_cast<float>(currentImage->width);
        imgH = static_cast<float>(currentImage->height);
    } else {
        return {};
    }

    auto screenSize = m_app->m_renderer.GetTargetSize().ToFloat();
    float scale = 1.0f;
    if (imgW > screenSize.width || imgH > screenSize.height) {
        float scaleX = screenSize.width / imgW;
        float scaleY = screenSize.height / imgH;
        scale = std::min(scaleX, scaleY);
    }

    float fitW = imgW * scale;
    float fitH = imgH * scale;
    float x = (screenSize.width - fitW) / 2.0f;
    float y = (screenSize.height - fitH) / 2.0f;

    return RectF::FromXYWH(x, y, fitW, fitH);
}

RectF ViewerPage::GetImageTransformedRect() const {
    RectF rect = CalculateImageRect();
    PointF center = rect.Center();

    D2D1_MATRIX_3X2_F matrix = D2D1::Matrix3x2F::Rotation(m_rotation, center.ToD2D()) *
                               D2D1::Matrix3x2F::Scale(m_zoom, m_zoom, center.ToD2D()) *
                               D2D1::Matrix3x2F::Translation(m_pan.x, m_pan.y);

    D2D1_POINT_2F tl = D2D1::Point2F(rect.left * matrix._11 + rect.top * matrix._21 + matrix._31,
                                     rect.left * matrix._12 + rect.top * matrix._22 + matrix._32);
    D2D1_POINT_2F br = D2D1::Point2F(rect.right * matrix._11 + rect.bottom * matrix._21 + matrix._31,
                                     rect.right * matrix._12 + rect.bottom * matrix._22 + matrix._32);

    return RectF(tl.x, tl.y, br.x, br.y);
}

void ViewerPage::DrawCheckerboard(Renderer& renderer, const RectF& rect) {
    if (!m_checkerboardBitmap) {
        constexpr int size = 20;
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
    float tileW = 40.0f, tileH = 40.0f;
    for (float y = rect.top; y < rect.bottom; y += tileH) {
        for (float x = rect.left; x < rect.right; x += tileW) {
            RectF tile = RectF::FromXYWH(x, y, tileW, tileH);
            renderer.DrawBitmap(m_checkerboardBitmap.Get(), tile, 0.5f, BitmapInterpolation::NearestNeighbor);
        }
    }
    renderer.PopClip();
}

void ViewerPage::DrawThirds(Renderer& renderer, const RectF& rect) {
    Color color = Color::FromRGBA(255, 255, 255, 30);
    float w = rect.Width();
    float h = rect.Height();

    renderer.DrawLine({ rect.left + w / 3.0f, rect.top }, { rect.left + w / 3.0f, rect.bottom }, color, 1.5f);
    renderer.DrawLine({ rect.left + w / 3.0f * 2.0f, rect.top }, { rect.left + w / 3.0f * 2.0f, rect.bottom }, color, 1.5f);

    renderer.DrawLine({ rect.left, rect.top + h / 3.0f }, { rect.right, rect.top + h / 3.0f }, color, 1.5f);
    renderer.DrawLine({ rect.left, rect.top + h / 3.0f * 2.0f }, { rect.right, rect.top + h / 3.0f * 2.0f }, color, 1.5f);
}

RectF ViewerPage::GetInfoBarRect(float desiredWidth, float height) const {
    const auto size = m_app->m_renderer.GetTargetSize().ToFloat();
    const float scale = m_app->m_window.GetDpiScale();
    const float margin = std::min(8.0f * scale, size.width / 2);
    const float width = std::clamp(desiredWidth, 0.0f, std::max(0.0f, size.width - 2 * margin));
    const bool stripVisible = m_app->IsViewerActive() && m_app->m_thumbnailStrip.GetOpacity() > 0;
    const float anchor = stripVisible ? m_app->m_thumbnailStrip.GetActiveCenter(size.width) : size.width / 2;
    const float x = std::clamp(anchor - width / 2, margin, std::max(margin, size.width - margin - width));
    const float bottom = stripVisible ? m_app->m_thumbnailStrip.GetBounds().top - 12 * scale : size.height - 15 * scale;
    return RectF::FromXYWH(x, std::max(0.0f, bottom - height), width, std::min(height, std::max(0.0f, bottom)));
}

std::wstring ViewerPage::FitInfoName(Renderer& renderer, const std::wstring& name, float width, float fontSize) const {
    if (renderer.MeasureText(name, fontSize).x <= width) return name;
    const std::wstring ellipsis = L"\u2026";
    if (renderer.MeasureText(ellipsis, fontSize).x > width) return {};
    size_t lo = 0, hi = name.size();
    while (lo < hi) {
        const size_t mid = lo + (hi - lo + 1) / 2;
        if (renderer.MeasureText(name.substr(0, mid) + ellipsis, fontSize).x <= width) lo = mid;
        else hi = mid - 1;
    }
    if (lo > 0 && name[lo - 1] >= 0xd800 && name[lo - 1] <= 0xdbff) --lo;
    return name.substr(0, lo) + ellipsis;
}

bool ViewerPage::IsInfoBarHit(float x, float y) const {
    if (!m_app) return false;
    const bool modal = m_app->m_currentPage == Page::Waterfall && m_app->m_waterfallPage.m_viewerModal;
    return (m_app->m_settings.show_metadata || modal) && m_metadataOpacity > 0 &&
        (m_infoBarRect.Contains(x, y) || m_infoTooltipRect.Contains(x, y));
}

void ViewerPage::DrawOverlayInfo(Renderer& renderer) {
    const bool overPreviousTooltip = m_infoTooltipRect.Contains(m_app->m_mouseX, m_app->m_mouseY);
    m_infoBarRect = {};
    m_infoTooltipRect = {};
    const bool modal = m_app->m_currentPage == Page::Waterfall && m_app->m_waterfallPage.m_viewerModal;
    if ((!m_app->m_settings.show_metadata && !modal) || m_metadataOpacity <= 0) return;
    const auto image = m_app->m_currentImage;
    const auto animation = m_app->m_animatedImage;
    if (!image && !animation) return;
    const float scale = m_app->m_window.GetDpiScale();
    const float fontSize = 13.0f; // Original client-unit text size; do not inflate it again with window DPI.
    const auto size = renderer.GetTargetSize().ToFloat();
    const std::wstring name = std::filesystem::path(animation ? animation->filePath : image->filePath).filename().wstring();
    std::wstring suffix;
    if (!modal) {
        if (animation) {
            suffix = std::format(L" | Frame {}/{} | Zoom: {:.0f}%", m_app->m_frameSequencer.GetCurrentFrame() + 1,
                m_app->m_frameSequencer.GetFrameCount(), GetDisplayZoom() * 100);
        } else {
            suffix = std::format(L" | {}×{} | Zoom: {:.0f}%", image->width, image->height, GetDisplayZoom() * 100);
        }
        if (!m_app->m_fileList.empty()) suffix += std::format(L" | [{}/{}]", m_app->m_currentIndex + 1, m_app->m_fileList.size());
    }
    const float padding = 12 * scale;
    const float suffixWidth = renderer.MeasureText(suffix, fontSize).x;
    const float desiredWidth = renderer.MeasureText(name, fontSize).x + suffixWidth + 2 * padding;
    // A second line preserves suffixes on narrow clients rather than dropping dimensions/zoom.
    const bool twoLines = suffixWidth > std::max(0.0f, size.width - 2 * padding - 16 * scale) * 0.7f;
    m_infoBarRect = GetInfoBarRect(desiredWidth, (twoLines ? 50.0f : 30.0f) * scale);
    if (m_infoBarRect.Width() <= 0 || m_infoBarRect.Height() <= 0) return;
    const float textWidth = std::max(0.0f, m_infoBarRect.Width() - 2 * padding);
    const auto shownName = FitInfoName(renderer, name, std::max(0.0f, textWidth - (twoLines ? 0 : suffixWidth)), fontSize);
    renderer.FillRoundedRect(m_infoBarRect, 6 * scale, 6 * scale, Color::Black(0.6f * m_metadataOpacity));
    renderer.DrawRoundedRect(m_infoBarRect, 6 * scale, 6 * scale, Color::White(0.15f * m_metadataOpacity), scale);
    renderer.PushClip(m_infoBarRect);
    const auto textRect = RectF::FromXYWH(m_infoBarRect.left + padding, m_infoBarRect.top + 3 * scale, textWidth, (twoLines ? 22.0f : 24.0f) * scale);
    renderer.DrawText(shownName + (twoLines ? L"" : suffix), textRect, Color::White(0.9f * m_metadataOpacity), fontSize, TextAlign::Center, TextVAlign::Center);
    if (twoLines) {
        const float suffixFont = std::min(fontSize, fontSize * textWidth / std::max(1.0f, suffixWidth));
        renderer.DrawText(suffix, RectF::FromXYWH(textRect.left, m_infoBarRect.top + 25 * scale, textWidth, 22 * scale),
            Color::White(0.9f * m_metadataOpacity), suffixFont, TextAlign::Center, TextVAlign::Center);
    }
    renderer.PopClip();
    if (shownName != name && (m_infoBarRect.Contains(m_app->m_mouseX, m_app->m_mouseY) || overPreviousTooltip)) {
        auto context = renderer.GetD2DContext();
        if (!context) return;
        if (!m_infoTooltipLayout || m_infoTooltipName != name || m_infoTooltipWidth != textWidth || m_infoTooltipFont != fontSize) {
            auto format = renderer.CreateTextFormat(L"Segoe UI", fontSize);
            if (!format) return;
            format->SetWordWrapping(DWRITE_WORD_WRAPPING_EMERGENCY_BREAK);
            ComPtr<IDWriteFactory> factory;
            if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(factory.GetAddressOf())))) return;
            m_infoTooltipLayout.Reset();
            if (FAILED(factory->CreateTextLayout(name.c_str(), static_cast<UINT32>(name.size()), format.Get(),
                std::max(1.0f, textWidth), 100000.0f, &m_infoTooltipLayout))) return;
            m_infoTooltipName = name;
            m_infoTooltipWidth = textWidth;
            m_infoTooltipFont = fontSize;
            m_infoTooltipScroll = 0;
        }
        DWRITE_TEXT_METRICS metrics{};
        m_infoTooltipLayout->GetMetrics(&metrics);
        const float height = std::min(metrics.height + 8 * scale, m_infoBarRect.top);
        m_infoTooltipRect = RectF::FromXYWH(m_infoBarRect.left, m_infoBarRect.top - height,
            m_infoBarRect.Width(), height);
        renderer.FillRoundedRect(m_infoTooltipRect, 4 * scale, 4 * scale, Color::Black(0.95f));
        renderer.PushClip(m_infoTooltipRect);
        context->DrawTextLayout(D2D1::Point2F(m_infoTooltipRect.left + padding, m_infoTooltipRect.top + 4 * scale - m_infoTooltipScroll), m_infoTooltipLayout.Get(), renderer.GetBrush(Color::White()));
        renderer.PopClip();
    }
}

void ViewerPage::DrawProgressBar(Renderer& renderer, const RectF& rect) {
    double progress = m_app->m_frameSequencer.GetProgress();
    float w = rect.Width();
    float progressW = static_cast<float>(w * progress);

    RectF barRect = RectF::FromXYWH(rect.left, rect.bottom, progressW, 3.0f);
    renderer.FillRect(barRect, Color::FromRGBA(50, 220, 50, 200));
}

void ViewerPage::DrawRegionZoom(Renderer& renderer) {
    auto targetSize = renderer.GetTargetSize().ToFloat();

    if (m_regionZoomActive && !m_regionZoomDefined) {
        renderer.DrawRect(m_inputRect, Color::White(), 1.0f);
        renderer.DrawRect(m_projectedRect, Color::White(0.4f), 1.0f);

        renderer.DrawLine(m_inputRect.TopLeft(), m_projectedRect.TopLeft(), Color::White(0.4f), 1.0f);
        renderer.DrawLine(m_inputRect.TopRight(), m_projectedRect.TopRight(), Color::White(0.4f), 1.0f);
        renderer.DrawLine(m_inputRect.BottomLeft(), m_projectedRect.BottomLeft(), Color::White(0.4f), 1.0f);
        renderer.DrawLine(m_inputRect.BottomRight(), m_projectedRect.BottomRight(), Color::White(0.4f), 1.0f);

        float displayZoomTarget = GetDisplayZoomForValue(m_zoomLevelText);
        int zoomPercent = static_cast<int>(std::ceil(displayZoomTarget * 100.0f));
        std::wstring text = std::format(L"Zoom: {}%", zoomPercent);
        renderer.DrawText(text, RectF::FromXYWH(0, 0, targetSize.width, targetSize.height), Color::White(), 16.0f, TextAlign::Center, TextVAlign::Center);
    }

    if (m_regionStageFactor > 0.0f) {
        ComPtr<ID2D1Factory> factory;
        renderer.GetD2DContext()->GetFactory(&factory);

        ComPtr<ID2D1RectangleGeometry> windowGeo;
        factory->CreateRectangleGeometry(D2D1::RectF(0, 0, targetSize.width, targetSize.height), &windowGeo);

        ComPtr<ID2D1RectangleGeometry> animRectGeo;
        factory->CreateRectangleGeometry(m_inputRectAnimated.ToD2D(), &animRectGeo);

        ComPtr<ID2D1PathGeometry> clipGeo;
        factory->CreatePathGeometry(&clipGeo);

        ComPtr<ID2D1GeometrySink> sink;
        clipGeo->Open(&sink);
        windowGeo->CombineWithGeometry(animRectGeo.Get(), D2D1_COMBINE_MODE_EXCLUDE, nullptr, sink.Get());
        sink->Close();

        renderer.GetD2DContext()->PushLayer(
            D2D1::LayerParameters(D2D1::InfiniteRect(), clipGeo.Get(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE),
            nullptr
        );

        renderer.FillRect(
            RectF::FromXYWH(0, 0, targetSize.width, targetSize.height),
            Color::Black(0.75f * m_regionStageFactor)
        );

        renderer.GetD2DContext()->PopLayer();

        renderer.DrawRect(m_inputRectAnimated, Color::White(), 1.0f);
    }
}

void ViewerPage::CancelPointerGesture() {
    m_leftButtonDown = m_exteriorPress = false;
    if (m_regionZoomActive) CancelRegionZoom();
}

void ViewerPage::OnMouseMove(float x, float y, KeyModifiers mods) {
    m_mouseInactivityTimer = 0.0;
    if (m_metadataOpacity < 1.0f) {
        m_app->m_window.RequestRedraw();
    }
    if (m_leftButtonDown && !m_regionZoomActive) {
        const float dx = x - m_leftButtonDownX;
        const float dy = y - m_leftButtonDownY;
        const float dist2 = dx * dx + dy * dy;
        constexpr float kPanDragThresholdPx = 4.0f;
        if (dist2 >= kPanDragThresholdPx * kPanDragThresholdPx) {
            if (m_app->m_window.IsFullscreen()) {
                m_pan.x = m_panAtDragStart.x + dx;
                m_pan.y = m_panAtDragStart.y + dy;
                m_app->m_window.RequestRedraw();
            } else {
                m_leftButtonDown = false;
                ReleaseCapture();
                SendMessageW(m_app->m_window.GetHwnd(), WM_NCLBUTTONDOWN, HTCAPTION, 0);
            }
        }
        return;
    }
    if (m_regionZoomActive && !m_regionZoomDefined) {
        UpdateRegionZoom(x, y);
    }
}

bool ViewerPage::IsExteriorPress(float x, float y, KeyModifiers mods) const {
    return m_app->m_currentPage == Page::Viewer && m_app->m_window.IsTranslucent() && m_app->m_settings.window_opacity < 1.0f
        && (m_app->m_currentImage || m_app->m_animatedImage) && !mods.ctrl && !mods.shift && !mods.alt
        && !HasPointerGesture() && !IsInfoBarHit(x, y) && !IsPointOverImage(x, y);
}

void ViewerPage::OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (btn == MouseButton::Left && IsExteriorPress(x, y, mods)) {
        m_exteriorPress = true;
        if (m_app->m_window.IsFullscreen()) m_app->m_window.SetFullscreen(false);
        if (GetCapture() == m_app->m_window.GetHwnd()) ReleaseCapture();
        return;
    }
    if (IsInfoBarHit(x, y)) return;
    m_mouseInactivityTimer = 0.0;
    if (m_metadataOpacity < 1.0f) {
        m_app->m_window.RequestRedraw();
    }
    if (btn == MouseButton::Right) {
        ShowContextMenu();
        return;
    }

    if (btn == MouseButton::Left) {
        if (mods.ctrl) {
            StartRegionZoom(x, y);
        } else {
            m_app->m_animations.Cancel("zoom");
            m_app->m_animations.Cancel("pan_x");
            m_app->m_animations.Cancel("pan_y");
            m_leftButtonDown = true;
            m_leftButtonStart = { x, y };
            m_leftButtonDownX = x;
            m_leftButtonDownY = y;
            m_panAtDragStart = m_pan;
            m_leftButtonDownTime = std::chrono::duration<double>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
        }
    }
}

void ViewerPage::OnMouseUp(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (btn == MouseButton::Left && m_exteriorPress) { m_exteriorPress = false; return; }
    m_mouseInactivityTimer = 0.0;
    if (m_metadataOpacity < 1.0f) {
        m_app->m_window.RequestRedraw();
    }
    if (btn == MouseButton::Right) {
        return;
    }
    if (btn == MouseButton::Left) {
        m_leftButtonDown = false;
        if (m_regionZoomActive && !m_regionZoomDefined) {
            FinishRegionZoom(x, y);
        }
    }
}

void ViewerPage::ShowContextMenu() {
    POINT pt;
    if (!GetCursorPos(&pt)) return;
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;
    AppendMenuW(hMenu, MF_STRING, 4, L"Toggle 1:1 Zoom (1)");
    AppendMenuW(hMenu, MF_STRING, 5, L"Rotate 90° (R)");
    AppendMenuW(hMenu, MF_STRING, 7, L"Pixelated Zoom (P)");
    AppendMenuW(hMenu, MF_STRING, 9, L"Toggle Rule of Thirds (G)");
    AppendMenuW(hMenu, MF_STRING, 10, L"Toggle Metadata Overlay (M)");
    AppendMenuW(hMenu, MF_STRING, 11, L"Tagging Modal... (T)");
    AppendMenuW(hMenu, MF_STRING, 12, L"Reset Zoom/Pan (0)");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, 13, L"Next Image (Right Arrow)");
    AppendMenuW(hMenu, MF_STRING, 14, L"Previous Image (Left Arrow)");
    AppendMenuW(hMenu, MF_STRING, 15, L"Delete Image (Del)");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, 1, L"Settings... (O)");
    AppendMenuW(hMenu, MF_STRING, 2, L"Toggle Fullscreen (F11)");
    AppendMenuW(hMenu, MF_STRING, 3, L"Exit (Esc)");

    int selected = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                                   pt.x, pt.y, 0, m_app->m_window.GetHwnd(), nullptr);
    DestroyMenu(hMenu);

    if (selected == 1) {
        m_app->OnKeyDown('O', {});
    } else if (selected == 2) {
        m_app->OnKeyDown(VK_F11, {});
    } else if (selected == 3) {
        m_app->OnKeyDown(VK_ESCAPE, {});
    } else if (selected == 4) {
        Toggle1To1();
    } else if (selected == 5) {
        m_rotation += 90.0f;
        m_app->m_window.RequestRedraw();
    } else if (selected == 7) {
        m_pixelatedZoom = !m_pixelatedZoom;
        m_app->m_window.RequestRedraw();
    } else if (selected == 9) {
        m_app->m_settings.show_thirds = !m_app->m_settings.show_thirds;
        m_app->m_settings.Save();
        m_app->m_window.RequestRedraw();
    } else if (selected == 10) {
        m_app->m_settings.show_metadata = !m_app->m_settings.show_metadata;
        m_app->m_settings.Save();
        m_app->m_window.RequestRedraw();
    } else if (selected == 11) {
        m_app->OnKeyDown('T', {});
    } else if (selected == 12) {
        ResetView();
    } else if (selected == 13) {
        m_app->NavigateImage(1);
    } else if (selected == 14) {
        m_app->NavigateImage(-1);
    } else if (selected == 15) {
        m_app->OnKeyDown(VK_DELETE, {});
    }
}

void ViewerPage::OnMouseWheel(float delta, float x, float y, KeyModifiers mods) {
    if (IsInfoBarHit(x, y) && m_infoTooltipLayout) {
        DWRITE_TEXT_METRICS metrics{};
        m_infoTooltipLayout->GetMetrics(&metrics);
        m_infoTooltipScroll = std::clamp(m_infoTooltipScroll - delta * 30 * m_app->m_window.GetDpiScale(),
            0.0f, std::max(0.0f, metrics.height + 8 * m_app->m_window.GetDpiScale() - m_infoTooltipRect.Height()));
        m_app->m_window.RequestRedraw();
        return;
    }
    m_mouseInactivityTimer = 0.0;
    if (m_metadataOpacity < 1.0f) {
        m_app->m_window.RequestRedraw();
    }
    bool shouldZoom = m_app->m_settings.zoom_on_mousewheel ? !mods.ctrl : mods.ctrl;
    if (!shouldZoom) {
        m_app->NavigateImage(delta > 0 ? -1 : 1);
    } else {
        float factor = (delta > 0) ? 1.07f : (1.0f / 1.07f);
        
        auto targetSize = m_app->m_renderer.GetTargetSize().ToFloat();
        m_zoomAnchor = { targetSize.width / 2.0f, targetSize.height / 2.0f };

        m_app->m_animations.Cancel("pan_x");
        m_app->m_animations.Cancel("pan_y");

        if (!m_app->m_animations.IsActive("zoom")) {
            m_targetZoom = m_zoom;
        }

        m_targetZoom = m_targetZoom * factor;
        m_targetZoom = std::clamp(m_targetZoom, 0.01f, 100.0f);

        bool useZoomEffects = m_app->m_settings.effects && m_app->m_settings.animated_zoom;
        if (useZoomEffects) {
            m_app->m_animations.Start("zoom", m_zoom, m_targetZoom, 0.20, Easing::EaseOutCubic);
        } else {
            m_app->m_animations.Cancel("zoom");
            float oldZoom = m_zoom;
            m_zoom = m_targetZoom;
            if (oldZoom > 0.0f) {
                float actualFactor = m_zoom / oldZoom;
                PointF imgCenter = CalculateImageRect().Center();
                m_pan.x = (m_zoomAnchor.x - imgCenter.x) - ((m_zoomAnchor.x - imgCenter.x) - m_pan.x) * actualFactor;
                m_pan.y = (m_zoomAnchor.y - imgCenter.y) - ((m_zoomAnchor.y - imgCenter.y) - m_pan.y) * actualFactor;
            }
        }
        m_app->m_window.RequestRedraw();
    }
}

void ViewerPage::OnMouseDoubleClick(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (btn == MouseButton::Left && !IsInfoBarHit(x, y) && !IsExteriorPress(x, y, mods)) {
        CancelPointerGesture();
        m_app->m_window.ToggleFullscreen();
    }
}

bool ViewerPage::OnKeyDown(uint32_t vk, KeyModifiers mods) {
    switch (vk) {
        case '1':
            Toggle1To1();
            return true;
        case 'G':
            m_app->m_settings.show_thirds = !m_app->m_settings.show_thirds;
            m_app->m_settings.Save();
            m_app->m_window.RequestRedraw();
            return true;
        case 'M':
            m_app->m_settings.show_metadata = !m_app->m_settings.show_metadata;
            m_app->m_settings.Save();
            m_app->m_window.RequestRedraw();
            return true;
        case 'P':
            m_pixelatedZoom = !m_pixelatedZoom;
            m_app->m_window.RequestRedraw();
            return true;
        
        case 'W': {
            float move = mods.shift ? 100.0f : 15.0f;
            m_pan.y += move;
            m_app->m_window.RequestRedraw();
            return true;
        }
        case 'S': {
            float move = mods.shift ? 100.0f : 15.0f;
            m_pan.y -= move;
            m_app->m_window.RequestRedraw();
            return true;
        }
        case 'A': {
            float move = mods.shift ? 100.0f : 15.0f;
            m_pan.x += move;
            m_app->m_window.RequestRedraw();
            return true;
        }
        case 'D': {
            float move = mods.shift ? 100.0f : 15.0f;
            m_pan.x -= move;
            m_app->m_window.RequestRedraw();
            return true;
        }

        case VK_UP: {
            float factor = 1.07f;
            auto targetSize = m_app->m_renderer.GetTargetSize().ToFloat();
            m_zoomAnchor = { targetSize.width / 2.0f, targetSize.height / 2.0f };
            m_app->m_animations.Cancel("pan_x");
            m_app->m_animations.Cancel("pan_y");
            if (!m_app->m_animations.IsActive("zoom")) {
                m_targetZoom = m_zoom;
            }
            m_targetZoom = m_targetZoom * factor;
            m_targetZoom = std::clamp(m_targetZoom, 0.01f, 100.0f);
            m_app->m_animations.Start("zoom", m_zoom, m_targetZoom, 0.20, Easing::EaseOutCubic);
            m_app->m_window.RequestRedraw();
            return true;
        }
        case VK_DOWN: {
            float factor = 1.0f / 1.07f;
            auto targetSize = m_app->m_renderer.GetTargetSize().ToFloat();
            m_zoomAnchor = { targetSize.width / 2.0f, targetSize.height / 2.0f };
            m_app->m_animations.Cancel("pan_x");
            m_app->m_animations.Cancel("pan_y");
            if (!m_app->m_animations.IsActive("zoom")) {
                m_targetZoom = m_zoom;
            }
            m_targetZoom = m_targetZoom * factor;
            m_targetZoom = std::clamp(m_targetZoom, 0.01f, 100.0f);
            m_app->m_animations.Start("zoom", m_zoom, m_targetZoom, 0.20, Easing::EaseOutCubic);
            m_app->m_window.RequestRedraw();
            return true;
        }
    }
    return false;
}

void ViewerPage::ResetView() {
    m_app->m_animations.Cancel("zoom");
    m_app->m_animations.Cancel("pan_x");
    m_app->m_animations.Cancel("pan_y");
    m_app->m_animations.Cancel("region_zoom_factor");
    float defaultZoom = GetDefaultZoom();
    m_zoom = defaultZoom;
    m_targetZoom = defaultZoom;
    m_pan = {};
    m_rotation = 0.0f;
    m_regionZoomActive = false;
    m_regionZoomDefined = false;
    m_regionStageFactor = 0.0f;

    m_zoomTextOpacity = 0.0f;
    m_zoomTextTimer = 0.0;
    m_lastZoom = defaultZoom;

    m_metadataOpacity = 1.0f;
    m_mouseInactivityTimer = 0.0;

    m_app->m_window.RequestRedraw();
}

bool ViewerPage::HasActiveAnimations() const {
    return m_zoomTextTimer > 0.0 && m_zoomTextTimer < 0.5;
}

void ViewerPage::Toggle1To1() {
    if (m_regionZoomDefined || m_regionZoomActive) {
        CancelRegionZoom();
        return;
    }

    float imgW = 0.0f;
    auto currentImage = m_app->m_currentImage;
    auto animatedImage = m_app->m_animatedImage;
    if (animatedImage && !animatedImage->frames.empty()) {
        imgW = static_cast<float>(animatedImage->frames[0].width);
    } else if (currentImage) {
        imgW = static_cast<float>(currentImage->width);
    }

    float fitW = CalculateImageRect().Width();
    float zoom1to1 = (fitW > 0.0f && imgW > 0.0f) ? (imgW / fitW) : 1.0f;

    bool isAt1to1 = (std::abs(m_zoom - zoom1to1) < 0.005f);

    m_app->m_animations.Cancel("zoom");
    m_app->m_animations.Cancel("pan_x");
    m_app->m_animations.Cancel("pan_y");
    m_app->m_animations.Cancel("region_zoom_factor");

    bool useEffects = m_app->m_settings.effects;
    bool useZoomEffects = useEffects && m_app->m_settings.animated_zoom;

    if (isAt1to1) {
        m_targetZoom = 1.0f;
        if (useZoomEffects) {
            m_app->m_animations.Start("zoom", m_zoom, 1.0f, 0.25, Easing::EaseOutCubic);
        } else {
            m_zoom = 1.0f;
        }
        if (useEffects) {
            m_app->m_animations.Start("pan_x", m_pan.x, 0.0f, 0.25, Easing::EaseOutCubic);
            m_app->m_animations.Start("pan_y", m_pan.y, 0.0f, 0.25, Easing::EaseOutCubic);
        } else {
            m_pan.x = 0.0f;
            m_pan.y = 0.0f;
        }
    } else {
        auto targetSize = m_app->m_renderer.GetTargetSize().ToFloat();
        m_zoomAnchor = { targetSize.width / 2.0f, targetSize.height / 2.0f };

        m_targetZoom = zoom1to1;
        if (useZoomEffects) {
            m_app->m_animations.Start("zoom", m_zoom, zoom1to1, 0.25, Easing::EaseOutCubic);
        } else {
            m_zoom = zoom1to1;
        }
        if (useEffects) {
            m_app->m_animations.Start("pan_x", m_pan.x, 0.0f, 0.25, Easing::EaseOutCubic);
            m_app->m_animations.Start("pan_y", m_pan.y, 0.0f, 0.25, Easing::EaseOutCubic);
        } else {
            m_pan.x = 0.0f;
            m_pan.y = 0.0f;
        }
    }
    m_app->m_window.RequestRedraw();
}

void ViewerPage::ZoomToPoint(float factor, float screenX, float screenY) {
    m_app->m_animations.Cancel("pan_x");
    m_app->m_animations.Cancel("pan_y");

    if (!m_app->m_animations.IsActive("zoom")) {
        m_targetZoom = m_zoom;
    }

    m_zoomAnchor = { screenX, screenY };
    m_targetZoom = m_targetZoom * factor;
    m_targetZoom = std::clamp(m_targetZoom, 0.01f, 100.0f);

    bool useZoomEffects = m_app->m_settings.effects && m_app->m_settings.animated_zoom;
    if (useZoomEffects) {
        m_app->m_animations.Start("zoom", m_zoom, m_targetZoom, 0.20, Easing::EaseOutCubic);
    } else {
        m_app->m_animations.Cancel("zoom");
        float oldZoom = m_zoom;
        m_zoom = m_targetZoom;
        if (oldZoom > 0.0f) {
            float actualFactor = m_zoom / oldZoom;
            PointF imgCenter = CalculateImageRect().Center();
            m_pan.x = (m_zoomAnchor.x - imgCenter.x) - ((m_zoomAnchor.x - imgCenter.x) - m_pan.x) * actualFactor;
            m_pan.y = (m_zoomAnchor.y - imgCenter.y) - ((m_zoomAnchor.y - imgCenter.y) - m_pan.y) * actualFactor;
        }
    }
    m_app->m_window.RequestRedraw();
}

void ViewerPage::StartRegionZoom(float startX, float startY) {
    m_regionZoomActive = true;
    m_regionStartPoint = { startX, startY };
    m_regionEndPoint = { startX, startY };
    m_regionZoomDefined = false;
    m_regionStageFactor = 0.0f;
    m_origZoom = m_zoom;
    m_origPan = m_pan;

    m_inputRect = RectF(startX, startY, startX, startY);
    m_projectedRect = {};
    m_app->m_window.RequestRedraw();
}

void ViewerPage::UpdateRegionZoom(float curX, float curY) {
    m_regionEndPoint = { curX, curY };
    
    m_inputRect.left   = std::min(m_regionStartPoint.x, m_regionEndPoint.x);
    m_inputRect.top    = std::min(m_regionStartPoint.y, m_regionEndPoint.y);
    m_inputRect.right  = std::max(m_regionStartPoint.x, m_regionEndPoint.x);
    m_inputRect.bottom = std::max(m_regionStartPoint.y, m_regionEndPoint.y);

    auto targetSize = m_app->m_renderer.GetTargetSize().ToFloat();
    RectF viewportRect = RectF::FromXYWH(0, 0, targetSize.width, targetSize.height);
    m_projectedRect = FitRectIntoRect(m_inputRect, viewportRect);

    if (m_inputRect.Width() > 0.0f) {
        m_zoomLevelText = m_projectedRect.Width() / m_inputRect.Width();
    } else {
        m_zoomLevelText = 1.0f;
    }

    m_app->m_window.RequestRedraw();
}

void ViewerPage::FinishRegionZoom(float endX, float endY) {
    UpdateRegionZoom(endX, endY);

    if (m_inputRect.Width() < 10.0f || m_inputRect.Height() < 10.0f) {
        CancelRegionZoom();
        return;
    }

    m_regionZoomDefined = true;

    auto targetSize = m_app->m_renderer.GetTargetSize().ToFloat();
    PointF windowCenter = { targetSize.width / 2.0f, targetSize.height / 2.0f };
    PointF imageCenter  = CalculateImageRect().Center();
    PointF inputCenter  = m_inputRect.Center();

    float factor = m_projectedRect.Width() / m_inputRect.Width();
    m_zoomLevelText = m_zoom * factor;
    
    m_targetPan.x = windowCenter.x - imageCenter.x - (inputCenter.x - imageCenter.x - m_pan.x) * factor;
    m_targetPan.y = windowCenter.y - imageCenter.y - (inputCenter.y - imageCenter.y - m_pan.y) * factor;

    m_inputRectAnimated = m_inputRect;

    bool useEffects = m_app->m_settings.effects && m_app->m_settings.animated_zoom;
    if (useEffects) {
        m_app->m_animations.Start("region_zoom_factor", 0.0f, 1.0f, 0.40, Easing::EaseInOutCubic);
    } else {
        m_zoom = m_zoomLevelText;
        m_pan = m_targetPan;
        m_regionZoomActive = false;
        m_regionZoomDefined = false;
        m_regionStageFactor = 0.0f;
    }
    m_app->m_window.RequestRedraw();
}

void ViewerPage::CancelRegionZoom() {
    if (m_regionZoomDefined) {
        m_origZoom = m_zoom;
        m_origPan = m_pan;
        m_zoomLevelText = 1.0f;
        m_targetPan = { 0.0f, 0.0f };

        m_inputRect = m_inputRectAnimated;
        
        auto targetSize = m_app->m_renderer.GetTargetSize().ToFloat();
        RectF viewportRect = RectF::FromXYWH(0, 0, targetSize.width, targetSize.height);
        m_projectedRect = FitRectIntoRect(CalculateImageRect(), viewportRect);

        bool useEffects = m_app->m_settings.effects && m_app->m_settings.animated_zoom;
        if (useEffects) {
            m_app->m_animations.Start("region_zoom_factor", 0.0f, 1.0f, 0.30, Easing::EaseInOutCubic, [this]() {
                m_regionZoomActive = false;
                m_regionZoomDefined = false;
                m_regionStageFactor = 0.0f;
                ResetView();
            });
        } else {
            m_regionZoomActive = false;
            m_regionZoomDefined = false;
            m_regionStageFactor = 0.0f;
            ResetView();
        }
    } else {
        m_regionZoomActive = false;
        m_regionZoomDefined = false;
        m_regionStageFactor = 0.0f;
        m_app->m_window.RequestRedraw();
    }
}

bool ViewerPage::IsPointOverImage(float x, float y) const {
    RectF rect = CalculateImageRect();
    if (rect.Width() <= 0.0f || rect.Height() <= 0.0f) return false;

    PointF center = rect.Center();
    D2D1_MATRIX_3X2_F matrix = D2D1::Matrix3x2F::Rotation(m_rotation, center.ToD2D()) *
                               D2D1::Matrix3x2F::Scale(m_zoom, m_zoom, center.ToD2D()) *
                               D2D1::Matrix3x2F::Translation(m_pan.x, m_pan.y);

    D2D1_MATRIX_3X2_F invert;
    if (D2D1InvertMatrix(&matrix)) {
        invert = matrix;
    } else {
        invert = D2D1::Matrix3x2F::Identity();
    }

    D2D1_POINT_2F localPoint = D2D1::Point2F(x * invert._11 + y * invert._21 + invert._31,
                                             x * invert._12 + y * invert._22 + invert._32);
    return rect.Contains(localPoint.x, localPoint.y);
}

float ViewerPage::GetDefaultZoom() const {
    return 1.0f;
}

float ViewerPage::GetDisplayZoom() const {
    return GetDisplayZoomForValue(m_zoom);
}

float ViewerPage::GetDisplayZoomForValue(float zoomValue) const {
    auto currentImage = m_app->m_currentImage;
    auto animatedImage = m_app->m_animatedImage;
    if (!currentImage && !animatedImage) return 1.0f;

    float imgW = 0.0f;
    if (animatedImage && !animatedImage->frames.empty()) {
        imgW = static_cast<float>(animatedImage->frames[0].width);
    } else if (currentImage) {
        imgW = static_cast<float>(currentImage->width);
    }
    if (imgW <= 0.0f) return 1.0f;

    float fitW = CalculateImageRect().Width();
    if (fitW <= 0.0f) return 1.0f;

    return zoomValue * (fitW / imgW);
}

} // namespace kv
