#pragma once
#include "rendering/renderer.h"

namespace kv {
class App;
// UI-owned draft preferences; only Apply changes settings or requests a worker projection.
class SortFilterModal {
    friend class AsyncPreviewTests;
public:
    // Binds the non-owning coordinator, which outlives this overlay.
    void Initialize(App* app) { m_app=app; }
    // Copies committed preferences into a new draft and focuses the first control.
    void Show();
    // Cancels the draft and requests redraw, without changing committed preferences.
    void Hide();
    // UI-only visibility read used to give the overlay input precedence.
    bool IsVisible() const { return m_visible; }
    // Draws a responsive settings-style overlay; no preference or filesystem mutation.
    void Draw(Renderer& renderer);
    // UI: modal mouse/key handlers consume all input, including outside presses.
    bool OnMouseMove(float x, float y);
    bool OnMouseDown(MouseButton button, float x, float y);
    bool OnKeyDown(uint32_t key, KeyModifiers mods);
private:
    // UI layout helpers use the same rectangles for drawing and hit testing.
    RectF Bounds() const;
    RectF Control(int index) const;
    // UI: edits the draft or commits it once through App.
    void Activate(int index);
    App* m_app=nullptr;
    bool m_visible=false;
    int m_mode=0, m_mask=511, m_focus=0, m_hover=-1;
};
}
