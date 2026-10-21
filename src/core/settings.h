// ============================================================================
// Class: Settings
// Description: Settings handles serialization, deserialization, and runtime updates of user preferences for [IMAGE_VIEWER_APP_NAME].
// Responsibility: Load and save configurations from `settings.json`.
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
    bool single_instance = true;
    bool desaturated_corner_buttons_and_corner_menus = false;
    bool show_console_output = true;
    bool effects = true;
    bool show_noise_cells = false;

    // ─── Advanced Settings ───────────────────────────────────────────────────
    float settings_window_opacity = 0.5f;
    float window_opacity = 0.85f;

    // ─── Viewer Page Settings ────────────────────────────────────────────────
    bool animated_zoom = false;
    bool draw_control_panel_backplate = false;
    float thumbnail_width = 50.0f;
    bool zoom_on_mousewheel = true;
    bool draw_default_thumbnail = true;
    bool show_thirds = false;
    bool show_cyberpunk = false;
    bool show_metadata = true;
    int sort_mode = 0;

    // ─── Diagnostics ──────────────────────────────────────────────────────────
    bool enable_file_log = false;  // Enable kv_log.txt file logging (default: off)
    bool enable_debug_ipc = false; // Enable debug_ipc.txt IPC logging (default: off)

    /// Read settings.json early to set debug IPC flag before App initialize
    static void PreInitDebugIpc();

    /// Load settings from user_data/settings.json. Returns false on failure (e.g. file missing).
    bool Load();

    /// Save settings to user_data/settings.json.
    bool Save();

private:
    std::wstring GetSettingsFilePath() const;
};

} // namespace kv
