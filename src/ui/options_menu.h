// ============================================================================
// Class: OptionsMenu
// Description: OptionsMenu renders the in-app HUD interface overlay to toggle options and settings for Fotassa.
// Responsibility: Render settings categories, checkboxes, sliders, and keybinding panels.
// ============================================================================
#pragma once

#include "rendering/renderer.h"
#include "core/settings.h"

namespace kv {

class App; // Forward declaration

class OptionsMenu {
    friend class App;
    friend class AsyncPreviewTests; // Exercises responsive translated controls with the native WARP renderer.
public:
    OptionsMenu() = default;
    ~OptionsMenu() = default;

    void Initialize(App* app);

    void Show();
    void Hide();
    bool IsVisible() const { return m_visible; }

    // UI: draws localized controls and responsive tabs/buttons without changing preferences.
    void Draw(Renderer& renderer);
    bool OnMouseMove(float x, float y, KeyModifiers mods);
    // UI: uses the same responsive tab/control geometry as Draw and commits selected preferences.
    bool OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods);
    bool OnMouseUp(MouseButton btn, float x, float y, KeyModifiers mods);
    bool OnKeyDown(uint32_t vk, KeyModifiers mods);

private:
    struct ControlItem {
        enum Type { Checkbox, Slider, Label, Button };
        Type type;
        std::wstring name;
        std::wstring label;
        RectF rect;
        // Pointers to the setting variables
        bool* pBoolVal = nullptr;
        float* pFloatVal = nullptr;
        float minVal = 0.0f;
        float maxVal = 1.0f;
        
        // Button actions
        std::function<void()> onClick;
    };

    // UI: rebuilds current preference controls (no retired features); mode buttons cycle persisted IDs while skipping reserved 4.
    void BuildControls();
    // UI: centers the settings container within the current client, including narrow windows.
    RectF GetBounds() const;

    App* m_app = nullptr;
    bool m_visible = false;
    int m_currentTab = 0; // 0 = General, 1 = Viewer Page, 2 = Advanced, 3 = Language / About

    std::vector<ControlItem> m_controls;
    int m_hoveredIdx = -1;
    int m_activeIdx = -1; // for slider drag
    bool m_draggingSlider = false;
};

} // namespace kv
