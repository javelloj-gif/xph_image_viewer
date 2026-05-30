// ============================================================================
// settings.h — Settings Manager
// Krumassan Image Viewer — Native C++20
// ============================================================================
#pragma once

#include <string>
#include <filesystem>

namespace kv {

class Settings {
public:
    Settings() = default;
    ~Settings() = default;

    // ─── General Settings ────────────────────────────────────────────────────
    std::wstring ui_lang = L"en";
    int page_on_app_launch_from_explorer = 2; // Viewer = 2
    int page_to_open_on_drag_and_drop = 2;   // Viewer = 2
    int debug_page = 1;                     // Start = 1
    bool run_on_windows_startup = true;
    bool do_not_show_start_dialog = true;
    bool show_fullscreen = true;
    bool doubleclick_toggle = true;
    bool hide_to_tray_on_close = true;
    bool hide_on_app_start = false;
    bool desaturated_corner_buttons_and_corner_menus = false;
    bool show_console_output = true;
    bool effects = true;
    bool show_noise_cells = false;

    // ─── Gamepad Settings ────────────────────────────────────────────────────
    bool start_with_gamepad_enabled = false;
    bool gamepad_input_requires_focus = false;
    float gamepad_dead_zone_radius = 0.1f;
    float settings_window_opacity = 0.5f;
    bool show_gamepad_monitor = false;
    float gamepad_move_stick_ease_in_expo_param = 2.0f;
    float gamepad_move_stick_speed = 20.0f;

    // ─── Viewer Page Settings ────────────────────────────────────────────────
    bool animated_zoom = false;
    bool draw_control_panel_backplate = false;
    float thumbnail_width = 50.0f;
    bool zoom_on_mousewheel = true;
    bool draw_default_thumbnail = true;
    bool show_thirds = false;
    bool show_cyberpunk = false;

    /// Load settings from user_data/settings.json. Returns false on failure (e.g. file missing).
    bool Load();

    /// Save settings to user_data/settings.json.
    bool Save();

private:
    std::wstring GetSettingsFilePath() const;
};

} // namespace kv
