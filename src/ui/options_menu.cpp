// ============================================================================
// Class: OptionsMenu
// Description: OptionsMenu renders the in-app HUD interface overlay to toggle options and settings for Fotassa.
// Responsibility: Render settings categories, checkboxes, sliders, and keybinding panels.
// ============================================================================
#include "common/pch.h"
#include "options_menu.h"
#include "core/app.h"
#include "i18n/i18n_engine.h"
#include "common/types.h"

namespace kv {

// Names persisted mode IDs for option labels; retired/unknown IDs have no active mode.
static std::wstring GetPageName(int pageVal) {
    switch (pageVal) {
        case 1: return _(L"Start");
        case 2: return _(L"Viewer");
        case 3: return _(L"Board");
        case 5: return _(L"Waterfall");
        default: return _(L"Unknown");
    }
}

// Cycles existing persisted IDs without compacting them or offering the reserved value 4.
static int NextPageValue(int value) {
    switch (value) {
        case 1: return 2;
        case 2: return 3;
        case 3: return 5;
        default: return 1;
    }
}

void OptionsMenu::Initialize(App* app) {
    m_app = app;
}

void OptionsMenu::Show() {
    m_visible = true;
    m_currentTab = 0;
    BuildControls();
    m_app->m_window.RequestRedraw();
}

void OptionsMenu::Hide() {
    m_visible = false;
    m_app->m_window.RequestRedraw();
}

RectF OptionsMenu::GetBounds() const {
    auto screenSize = m_app->m_renderer.GetTargetSize().ToFloat();
    float boxW = std::min(720.0f,screenSize.width-16);
    float boxH = std::min(560.0f,screenSize.height-16);
    return RectF::FromXYWH(
        (screenSize.width - boxW) / 2.0f,
        (screenSize.height - boxH) / 2.0f,
        boxW,
        boxH
    );
}

void OptionsMenu::BuildControls() {
    m_controls.clear();
    m_hoveredIdx = -1;
    m_activeIdx = -1;
    m_draggingSlider = false;

    RectF bounds = GetBounds();
    float contentLeft = bounds.left + 30.0f;
    float contentTop = bounds.top + 100.0f;
    float rowHeight = 35.0f;

    auto& s = m_app->m_settings;

    if (m_currentTab == 0) {
        // --- GENERAL OPTIONS ---
        std::vector<std::pair<std::wstring, bool*>> generalBools = {
            { L"Run on Windows Startup", &s.run_on_windows_startup },
            { L"Supress start dialog and run lite mode", &s.do_not_show_start_dialog },
            { L"Full-screen mode on application start", &s.show_fullscreen },
            { L"Hide to tray on close", &s.hide_to_tray_on_close },
            { L"Hide to tray on app start", &s.hide_on_app_start },
            { L"Enforce single instance", &s.single_instance },
            { L"Show standard (console) output overlay", &s.show_console_output },
            { L"Animated effects", &s.effects },
            { L"Show animated cells overlay", &s.show_noise_cells },
        };

        float currentY = contentTop;
        for (const auto& [label, pVal] : generalBools) {
            ControlItem item;
            item.type = ControlItem::Checkbox;
            item.name = label;
            item.label = _(label);
            item.pBoolVal = pVal;
            item.rect = RectF::FromXYWH(contentLeft, currentY, bounds.Width() - 60.0f, 25.0f);
            m_controls.push_back(item);
            currentY += rowHeight;
        }
    } else if (m_currentTab == 1) {
        // --- VIEWER PAGE OPTIONS ---
        std::vector<std::pair<std::wstring, bool*>> viewerBools = {
            { L"Animated zoom", &s.animated_zoom },
            { L"Enable mouse wheel to zoom and Ctrl+mouse wheel to navigate through image list", &s.zoom_on_mousewheel },
            { L"Show dummy-default thumbnail while generated one is not ready", &s.draw_default_thumbnail },
            { L"Show thirds", &s.show_thirds },
            { L"Show metadata", &s.show_metadata }
        };

        float currentY = contentTop;
        for (const auto& [label, pVal] : viewerBools) {
            ControlItem item;
            item.type = ControlItem::Checkbox;
            item.name = label;
            item.label = _(label);
            item.pBoolVal = pVal;
            item.rect = RectF::FromXYWH(contentLeft, currentY, bounds.Width() - 60.0f, 25.0f);
            m_controls.push_back(item);
            currentY += rowHeight;
        }

        // Add Thumbnail width slider
        ControlItem sliderItem;
        sliderItem.type = ControlItem::Slider;
        sliderItem.name = L"thumbnail_width";
        sliderItem.label = _(L"Thumbnails size");
        sliderItem.pFloatVal = &s.thumbnail_width;
        sliderItem.minVal = 30.0f;
        sliderItem.maxVal = 100.0f;
        sliderItem.rect = RectF::FromXYWH(contentLeft, currentY, bounds.Width() - 60.0f, 30.0f);
        m_controls.push_back(sliderItem);
    } else if (m_currentTab == 2) {
        // --- ADVANCED ---
        float currentY = contentTop;

        ControlItem advTitle;
        advTitle.type = ControlItem::Label;
        advTitle.label = _(L"Advanced Settings") + L":";
        advTitle.rect = RectF::FromXYWH(contentLeft, currentY, bounds.Width() - 60.0f, 25.0f);
        m_controls.push_back(advTitle);
        currentY += rowHeight;

        // Settings Window Opacity Slider
        ControlItem opacitySlider;
        opacitySlider.type = ControlItem::Slider;
        opacitySlider.name = L"settings_opacity";
        opacitySlider.label = _(L"Settings window opacity");
        opacitySlider.pFloatVal = &s.settings_window_opacity;
        opacitySlider.minVal = 0.1f;
        opacitySlider.maxVal = 1.0f;
        opacitySlider.rect = RectF::FromXYWH(contentLeft, currentY, bounds.Width() - 60.0f, 30.0f);
        m_controls.push_back(opacitySlider);
        currentY += 45.0f;

        // Application Window Opacity Slider
        ControlItem appOpacitySlider;
        appOpacitySlider.type = ControlItem::Slider;
        appOpacitySlider.name = L"window_opacity";
        appOpacitySlider.label = _(L"Application window opacity");
        appOpacitySlider.pFloatVal = &s.window_opacity;
        appOpacitySlider.minVal = 0.0f;
        appOpacitySlider.maxVal = 1.0f;
        appOpacitySlider.rect = RectF::FromXYWH(contentLeft, currentY, bounds.Width() - 60.0f, 30.0f);
        m_controls.push_back(appOpacitySlider);
        currentY += 45.0f;

        // Routing Options
        ControlItem launchBtn;
        launchBtn.type = ControlItem::Button;
        launchBtn.name = L"launch_page";
        launchBtn.label = _(L"Launch Page") + L": " + GetPageName(s.page_on_app_launch_from_explorer);
        launchBtn.rect = RectF::FromXYWH(contentLeft, currentY, 350.0f, 30.0f);
        launchBtn.onClick = [this, &s]() {
            s.page_on_app_launch_from_explorer = NextPageValue(s.page_on_app_launch_from_explorer);
            s.Save();
            BuildControls();
        };
        m_controls.push_back(launchBtn);
        currentY += 45.0f;

        ControlItem dndBtn;
        dndBtn.type = ControlItem::Button;
        dndBtn.name = L"dnd_page";
        dndBtn.label = _(L"Drag & Drop Page") + L": " + GetPageName(s.page_to_open_on_drag_and_drop);
        dndBtn.rect = RectF::FromXYWH(contentLeft, currentY, 350.0f, 30.0f);
        dndBtn.onClick = [this, &s]() {
            s.page_to_open_on_drag_and_drop = NextPageValue(s.page_to_open_on_drag_and_drop);
            s.Save();
            BuildControls();
        };
        m_controls.push_back(dndBtn);
        currentY += 45.0f;

        ControlItem debugBtn;
        debugBtn.type = ControlItem::Button;
        debugBtn.name = L"debug_page";
        debugBtn.label = _(L"Default Debug Page") + L": " + GetPageName(s.debug_page);
        debugBtn.rect = RectF::FromXYWH(contentLeft, currentY, 350.0f, 30.0f);
        debugBtn.onClick = [this, &s]() {
            s.debug_page = NextPageValue(s.debug_page);
            s.Save();
            BuildControls();
        };
        m_controls.push_back(debugBtn);
        currentY += 45.0f;

    } else if (m_currentTab == 3) {
        // --- LANGUAGE & ABOUT ---
        float currentY = contentTop;

        // Info Label
        ControlItem appLabel;
        appLabel.type = ControlItem::Label;
        appLabel.label = _(L"Fotassa — Native C++20 Version");
        appLabel.rect = RectF::FromXYWH(contentLeft, currentY, bounds.Width() - 60.0f, 30.0f);
        m_controls.push_back(appLabel);
        currentY += rowHeight * 1.5f;

        // Languages title
        ControlItem langTitle;
        langTitle.type = ControlItem::Label;
        langTitle.label = _(L"UI language") + L":";
        langTitle.rect = RectF::FromXYWH(contentLeft, currentY, bounds.Width() - 60.0f, 25.0f);
        m_controls.push_back(langTitle);
        currentY += rowHeight;

        // Toggle buttons for English / Russian
        ControlItem enBtn;
        enBtn.type = ControlItem::Button;
        enBtn.name = L"lang_en";
        enBtn.label = L"English (EN)";
        enBtn.rect = RectF::FromXYWH(contentLeft, currentY, (bounds.Width()-76)/3, 35.0f);
        enBtn.onClick = [this, &s]() {
            s.ui_lang = L"en";
            I18nEngine::Initialize(L"en");
            s.Save();
            BuildControls();
        };
        m_controls.push_back(enBtn);

        ControlItem ruBtn;
        ruBtn.type = ControlItem::Button;
        ruBtn.name = L"lang_ru";
        ruBtn.label = L"\u0420\u0443\u0441\u0441\u043a\u0438\u0439 (RU)";
        ruBtn.rect = RectF::FromXYWH(contentLeft + (bounds.Width()-60)/3, currentY, (bounds.Width()-76)/3, 35.0f);
        ruBtn.onClick = [this, &s]() {
            s.ui_lang = L"ru";
            I18nEngine::Initialize(L"ru");
            s.Save();
            BuildControls();
        };
        m_controls.push_back(ruBtn);
        ControlItem esBtn;
        esBtn.type=ControlItem::Button; esBtn.name=L"lang_es"; esBtn.label=L"Español (ES)";
        esBtn.rect=RectF::FromXYWH(contentLeft+2*(bounds.Width()-60)/3,currentY,(bounds.Width()-76)/3,35);
        esBtn.onClick=[this,&s]() { s.ui_lang=L"es"; I18nEngine::Initialize(L"es"); s.Save(); BuildControls(); };
        m_controls.push_back(esBtn);
    }
}

void OptionsMenu::Draw(Renderer& renderer) {
    if (!m_visible) return;

    auto screenSize = renderer.GetTargetSize().ToFloat();
    RectF bounds = GetBounds();

    // 1. Darken background
    renderer.FillRect(RectF::FromXYWH(0, 0, screenSize.width, screenSize.height), Color::Black(0.6f));

    // 2. Settings container backplate
    renderer.FillRoundedRect(bounds, 8.0f, 8.0f, Color::Black(m_app->m_settings.settings_window_opacity));
    renderer.DrawRoundedRect(bounds, 8.0f, 8.0f, Color::White(0.15f), 1.5f);

    // 3. Draw Title
    RectF titleRect = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 20.0f, bounds.Width() - 60.0f, 40.0f);
    renderer.DrawText(
        _(L"Settings"),
        titleRect,
        Color::White(),
        24.0f,
        TextAlign::Left,
        TextVAlign::Top
    );

    // 4. Draw Tab buttons
    float tabX = bounds.left + 30.0f;
    float tabY = bounds.top + 65.0f;
    float tabH = 30.0f;
    std::vector<std::wstring> tabLabels = {
        L"General",
        L"Viewer Page",
        L"Advanced",
        L"Language / About"
    };

    for (int i = 0; i < static_cast<int>(tabLabels.size()); ++i) {
        std::wstring label = _(tabLabels[i]);
        float labelW = (bounds.Width()-60)/4;
        RectF tabRect = RectF::FromXYWH(tabX, tabY, labelW, tabH);

        bool active = (m_currentTab == i);
        Color textColor = active ? Color::White() : Color::FromRGBA(160, 160, 160);

        renderer.DrawText(label, tabRect, textColor, std::min(14.0f,14.0f*tabRect.Width()/std::max(1.0f,renderer.MeasureText(label,14).x)), TextAlign::Center, TextVAlign::Center);
        if (active) {
            // Draw red underline for active tab
            renderer.FillRect(
                RectF::FromXYWH(tabRect.left, tabRect.bottom - 2.0f, tabRect.Width(), 2.0f),
                Color::FromRGBA(220, 50, 50)
            );
        }

        tabX += labelW;
    }

    // 5. Draw active controls
    for (size_t i = 0; i < m_controls.size(); ++i) {
        const auto& item = m_controls[i];
        bool isHovered = (m_hoveredIdx == static_cast<int>(i));

        if (item.type == ControlItem::Checkbox) {
            float boxSize = 16.0f;
            RectF boxRect = RectF::FromXYWH(item.rect.left, item.rect.top + (item.rect.Height() - boxSize) / 2.0f, boxSize, boxSize);

            // Draw Checkbox border
            renderer.DrawRect(boxRect, isHovered ? Color::White() : Color::FromRGBA(120, 120, 120), 1.5f);

            // Draw Check mark
            if (item.pBoolVal && *item.pBoolVal) {
                renderer.FillRect(boxRect.Inset(3.0f), Color::FromRGBA(220, 50, 50));
            }

            // Draw label
            RectF textRect = item.rect;
            textRect.left += boxSize + 15.0f;
            renderer.DrawText(item.label, textRect, Color::White(0.9f), 13.0f, TextAlign::Left, TextVAlign::Center);

        } else if (item.type == ControlItem::Slider) {
            // Draw slider label
            RectF textRect = item.rect;
            textRect.bottom = textRect.top + 15.0f;
            renderer.DrawText(item.label, textRect, Color::White(0.9f), 12.0f, TextAlign::Left, TextVAlign::Top);

            // Draw slider track
            float barY = item.rect.top + 22.0f;
            float barW = item.rect.Width();
            RectF trackRect = RectF::FromXYWH(item.rect.left, barY, barW, 4.0f);
            renderer.FillRect(trackRect, Color::FromRGBA(60, 60, 60));

            // Draw current fill
            float progress = 0.0f;
            if (item.pFloatVal) {
                progress = (*item.pFloatVal - item.minVal) / (item.maxVal - item.minVal);
            }
            progress = std::clamp(progress, 0.0f, 1.0f);
            renderer.FillRect(
                RectF::FromXYWH(item.rect.left, barY, barW * progress, 4.0f),
                Color::FromRGBA(220, 50, 50)
            );

            // Draw Handle
            float handleX = item.rect.left + barW * progress;
            renderer.FillEllipse(
                { handleX, barY + 2.0f },
                6.0f, 6.0f,
                isHovered ? Color::White() : Color::FromRGBA(220, 50, 50)
            );

        } else if (item.type == ControlItem::Label) {
            renderer.DrawText(item.label, item.rect, Color::White(0.85f), 14.0f, TextAlign::Left, TextVAlign::Center);

        } else if (item.type == ControlItem::Button) {
            bool currentLangSelected = false;
            if (item.name == L"lang_en" && m_app->m_settings.ui_lang == L"en") currentLangSelected = true;
            if (item.name == L"lang_es" && m_app->m_settings.ui_lang == L"es") currentLangSelected = true;
            if (item.name == L"lang_ru" && m_app->m_settings.ui_lang == L"ru") currentLangSelected = true;

            Color btnBg = currentLangSelected ? Color::FromRGBA(220, 50, 50, 180) : Color::FromRGBA(40, 40, 40);
            if (isHovered && !currentLangSelected) btnBg = Color::FromRGBA(60, 60, 60);

            renderer.FillRoundedRect(item.rect, 4.0f, 4.0f, btnBg);
            renderer.DrawRoundedRect(item.rect, 4.0f, 4.0f, Color::White(0.15f), 1.0f);
            renderer.DrawText(item.label, item.rect, Color::White(), std::min(13.0f,13.0f*(item.rect.Width()-8)/std::max(1.0f,renderer.MeasureText(item.label,13).x)), TextAlign::Center, TextVAlign::Center);
        }
    }
}

bool OptionsMenu::OnMouseMove(float x, float y, KeyModifiers mods) {
    if (!m_visible) return false;

    m_hoveredIdx = -1;

    // Check slider dragging
    if (m_draggingSlider && m_activeIdx >= 0 && m_activeIdx < static_cast<int>(m_controls.size())) {
        auto& item = m_controls[m_activeIdx];
        if (item.type == ControlItem::Slider && item.pFloatVal) {
            float pct = (x - item.rect.left) / item.rect.Width();
            pct = std::clamp(pct, 0.0f, 1.0f);
            *item.pFloatVal = item.minVal + pct * (item.maxVal - item.minVal);
            m_app->m_window.RequestRedraw();
        }
        return true;
    }

    // Check standard hover
    for (size_t i = 0; i < m_controls.size(); ++i) {
        if (m_controls[i].rect.Contains(x, y)) {
            m_hoveredIdx = static_cast<int>(i);
            m_app->m_window.RequestRedraw();
            return true;
        }
    }

    return true;
}

bool OptionsMenu::OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (!m_visible) return false;

    if (btn != MouseButton::Left) return true;

    RectF bounds = GetBounds();

    // Check click outside to dismiss
    if (!bounds.Contains(x, y)) {
        Hide();
        return true;
    }

    // Check tab clicks
    float tabX = bounds.left + 30.0f;
    float tabY = bounds.top + 65.0f;
    float tabH = 30.0f;
    std::vector<std::wstring> tabLabels = {
        L"General",
        L"Viewer Page",
        L"Advanced",
        L"Language / About"
    };

    for (int i = 0; i < static_cast<int>(tabLabels.size()); ++i) {
        float labelW = (bounds.Width()-60)/4;
        RectF tabRect = RectF::FromXYWH(tabX, tabY, labelW, tabH);
        if (tabRect.Contains(x, y)) {
            m_currentTab = i;
            BuildControls();
            m_app->m_window.RequestRedraw();
            return true;
        }
        tabX += labelW;
    }

    // Check control item clicks
    for (size_t i = 0; i < m_controls.size(); ++i) {
        auto& item = m_controls[i];
        if (item.rect.Contains(x, y)) {
            if (item.type == ControlItem::Checkbox && item.pBoolVal) {
                *item.pBoolVal = !(*item.pBoolVal);
                m_app->m_settings.Save();
                m_app->m_window.RequestRedraw();
                return true;
            } else if (item.type == ControlItem::Slider) {
                m_activeIdx = static_cast<int>(i);
                m_draggingSlider = true;
                return true;
            } else if (item.type == ControlItem::Button && item.onClick) {
                item.onClick();
                m_app->m_window.RequestRedraw();
                return true;
            }
        }
    }

    return true;
}

bool OptionsMenu::OnMouseUp(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (!m_visible) return false;

    if (m_draggingSlider) {
        m_app->m_settings.Save();
    }
    m_draggingSlider = false;
    m_activeIdx = -1;
    return true;
}

bool OptionsMenu::OnKeyDown(uint32_t vk, KeyModifiers mods) {
    if (!m_visible) return false;

    if (vk == VK_ESCAPE) {
        Hide();
        return true;
    }

    return true;
}

} // namespace kv
