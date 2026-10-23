// ============================================================================
// Class: ViewerPage
// Description: ViewerPage implements the single-image display screen with advanced zoom, pan, and visual grids for [IMAGE_VIEWER_APP_NAME].
// Responsibility: Handle fit-to-screen transforms, cyberpunk boundary graphics, thirds divisions, and Ctrl+drag spotlights.
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
            L"Drop an image here or pass a file path as argument\n\nWASD / Arrow Keys - Move & Paging\nCtrl+Scroll - Zoom / Browse\nCtrl+Drag - Region Magnifier\nC - Cyberpunk Corners | G - Rule of Thirds",
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

    if (m_app->m_settings.show_cyberpunk) {
        DrawCyberpunkCorners(renderer, GetImageTransformedRect());
    }
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

void ViewerPage::DrawCyberpunkCorners(Renderer& renderer, const RectF& rect) {
    auto targetSize = renderer.GetTargetSize().ToFloat();

    ComPtr<ID2D1Factory> factory;
    renderer.GetD2DContext()->GetFactory(&factory);

    ComPtr<ID2D1RectangleGeometry> windowGeo;
    factory->CreateRectangleGeometry(D2D1::RectF(0, 0, targetSize.width, targetSize.height), &windowGeo);

    ComPtr<ID2D1RectangleGeometry> imageGeo;
    factory->CreateRectangleGeometry(rect.ToD2D(), &imageGeo);

    ComPtr<ID2D1PathGeometry> clipGeo;
    factory->CreatePathGeometry(&clipGeo);

    ComPtr<ID2D1GeometrySink> sink;
    clipGeo->Open(&sink);
    HRESULT hr = windowGeo->CombineWithGeometry(imageGeo.Get(), D2D1_COMBINE_MODE_EXCLUDE, nullptr, sink.Get());
    if (SUCCEEDED(hr)) {
        sink->Close();

        ComPtr<ID2D1Layer> layer;
        renderer.GetD2DContext()->CreateLayer(&layer);
        renderer.GetD2DContext()->PushLayer(
            D2D1::LayerParameters(D2D1::InfiniteRect(), clipGeo.Get(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE),
            layer.Get()
        );

        float val = std::min(rect.Width(), rect.Height());
        float factor = std::clamp((val - 200.0f) / 500.0f, 0.0f, 1.0f);
        float alpha = 200.0f / 255.0f * factor;
        Color color = Color::White(alpha);

        float offset = 4.0f;

        auto drawCorner = [&](PointF c, PointF f1, PointF f2, PointF d1, PointF d2) {
            ComPtr<ID2D1PathGeometry> cornerGeo;
            factory->CreatePathGeometry(&cornerGeo);
            ComPtr<ID2D1GeometrySink> cSink;
            cornerGeo->Open(&cSink);
            cSink->BeginFigure(d1.ToD2D(), D2D1_FIGURE_BEGIN_FILLED);
            cSink->AddLine(f1.ToD2D());
            cSink->AddLine(c.ToD2D());
            cSink->AddLine(f2.ToD2D());
            cSink->AddLine(d2.ToD2D());
            cSink->EndFigure(D2D1_FIGURE_END_CLOSED);
            cSink->Close();
            renderer.GetD2DContext()->FillGeometry(cornerGeo.Get(), renderer.GetBrush(color));
        };

        drawCorner(
            { rect.left - offset, rect.top - offset },
            { rect.left - offset + 85.0f, rect.top - offset },
            { rect.left - offset, rect.top - offset + 85.0f },
            { rect.left - offset + 85.0f + 20.0f, rect.top - offset + 40.0f },
            { rect.left - offset + 40.0f, rect.top - offset + 85.0f + 20.0f }
        );

        drawCorner(
            { rect.right + offset, rect.top - offset },
            { rect.right + offset - 85.0f, rect.top - offset },
            { rect.right + offset, rect.top - offset + 85.0f },
            { rect.right + offset - 85.0f - 20.0f, rect.top - offset + 40.0f },
            { rect.right + offset - 40.0f, rect.top - offset + 85.0f + 20.0f }
        );

        drawCorner(
            { rect.right + offset, rect.bottom + offset },
            { rect.right + offset - 85.0f, rect.bottom + offset },
            { rect.right + offset, rect.bottom + offset - 85.0f },
            { rect.right + offset - 85.0f - 20.0f, rect.bottom + offset - 40.0f },
            { rect.right + offset - 40.0f, rect.bottom + offset - 85.0f - 20.0f }
        );

        drawCorner(
            { rect.left - offset, rect.bottom + offset },
            { rect.left - offset + 85.0f, rect.bottom + offset },
            { rect.left - offset, rect.bottom + offset - 85.0f },
            { rect.left - offset + 85.0f + 20.0f, rect.bottom + offset - 40.0f },
            { rect.left - offset + 40.0f, rect.bottom + offset - 85.0f - 20.0f }
        );

        renderer.GetD2DContext()->PopLayer();
    }
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

void ViewerPage::DrawOverlayInfo(Renderer& renderer) {
    auto size = renderer.GetTargetSize().ToFloat();

    bool isWaterfallModal = (m_app->m_currentPage == Page::Waterfall && m_app->m_waterfallPage.m_viewerModal);
    if ((m_app->m_settings.show_metadata || isWaterfallModal) && m_metadataOpacity > 0.0f) {
        auto currentImage = m_app->m_currentImage;
        auto animatedImage = m_app->m_animatedImage;

        std::wstring info;
        if (isWaterfallModal) {
            if (animatedImage) {
                info = std::filesystem::path(animatedImage->filePath).filename().wstring();
            } else if (currentImage) {
                info = std::filesystem::path(currentImage->filePath).filename().wstring();
            }
        } else {
            if (animatedImage) {
                info = std::format(L"{} | Frame {}/{} | Zoom: {:.0f}%",
                    std::filesystem::path(animatedImage->filePath).filename().wstring(),
                    m_app->m_frameSequencer.GetCurrentFrame() + 1,
                    m_app->m_frameSequencer.GetFrameCount(),
                    GetDisplayZoom() * 100.0f);
            } else if (currentImage) {
                info = std::format(L"{} | {}×{} | Zoom: {:.0f}%",
                    std::filesystem::path(currentImage->filePath).filename().wstring(),
                    currentImage->width, currentImage->height,
                    GetDisplayZoom() * 100.0f);
            }

            if (!m_app->m_fileList.empty()) {
                info += std::format(L" | [{}/{}]", m_app->m_currentIndex + 1, m_app->m_fileList.size());
            }
        }

        if (!info.empty()) {
            float textWidth = renderer.MeasureText(info, 13.0f).x;
            float paddingH = 15.0f;
            float paddingV = 5.0f;
            float barW = textWidth + paddingH * 2.0f;
            float barH = 30.0f;
            float barX = (size.width - barW) / 2.0f;

            if (m_metadataOpacity > 0.0f) {
                // Offset the island upward if the thumbnail strip is visible
                float stripOpacity = m_app->m_thumbnailStrip.GetOpacity();
                float bottomOffset = 15.0f + stripOpacity * (ThumbnailStrip::HEIGHT - 15.0f);
                float barY = size.height - barH - bottomOffset;

                RectF barRect = RectF::FromXYWH(barX, barY, barW, barH);
                renderer.FillRoundedRect(barRect, 6.0f, 6.0f, Color::Black(0.6f * m_metadataOpacity));
                renderer.DrawRoundedRect(barRect, 6.0f, 6.0f, Color::White(0.15f * m_metadataOpacity), 1.0f);

                RectF textRect = RectF::FromXYWH(barX + paddingH, barY + paddingV, textWidth, barH - paddingV * 2.0f);
                renderer.DrawText(info, textRect, Color::White(0.9f * m_metadataOpacity), 13.0f, TextAlign::Center, TextVAlign::Center);
            }
        }
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

void ViewerPage::OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods) {
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
    AppendMenuW(hMenu, MF_STRING, 6, L"Invert Colors (I)");
    AppendMenuW(hMenu, MF_STRING, 7, L"Pixelated Zoom (P)");
    AppendMenuW(hMenu, MF_STRING, 8, L"Toggle Cyberpunk Corners (C)");
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
    } else if (selected == 6) {
        m_invertImage = !m_invertImage;
        m_app->m_window.RequestRedraw();
    } else if (selected == 7) {
        m_pixelatedZoom = !m_pixelatedZoom;
        m_app->m_window.RequestRedraw();
    } else if (selected == 8) {
        m_app->m_settings.show_cyberpunk = !m_app->m_settings.show_cyberpunk;
        m_app->m_settings.Save();
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
    if (btn == MouseButton::Left && m_app->m_settings.doubleclick_toggle) {
        m_app->m_window.ToggleFullscreen();
    }
}

bool ViewerPage::OnKeyDown(uint32_t vk, KeyModifiers mods) {
    switch (vk) {
        case '1':
            Toggle1To1();
            return true;
        case 'C':
            m_app->m_settings.show_cyberpunk = !m_app->m_settings.show_cyberpunk;
            m_app->m_settings.Save();
            m_app->m_window.RequestRedraw();
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
        case 'I':
            m_invertImage = !m_invertImage;
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
