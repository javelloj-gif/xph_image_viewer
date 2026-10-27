// ============================================================================
// Class: Settings
// Description: Settings handles serialization, deserialization, and runtime updates of user preferences for Fotassa.
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
    bool hide_to_tray_on_close = true;
    bool hide_on_app_start = false;
    bool single_instance = true;
    bool show_console_output = true;
    bool effects = true;
    bool show_noise_cells = false;

    // ─── Advanced Settings ───────────────────────────────────────────────────
    float settings_window_opacity = 0.5f;
    float window_opacity = 0.85f;

    // ─── Viewer Page Settings ────────────────────────────────────────────────
    bool animated_zoom = true;
    float thumbnail_width = 44.0f;
    bool zoom_on_mousewheel = true;
    bool draw_default_thumbnail = true;
    bool show_thirds = false;
    bool show_metadata = true;
    int sort_mode = 0;
    int image_filter_mask = 511; // Nine supported extension families; all enabled by default.

    // ─── Diagnostics ──────────────────────────────────────────────────────────
    bool enable_file_log = false;  // Enable kv_log.txt file logging (default: off)
    bool enable_debug_ipc = false; // Enable debug_ipc.txt IPC logging (default: off)

    /// Reads the shared resolved settings.json before App initialization and sets the debug IPC flag.
    static void PreInitDebugIpc();

    /// Loads known preferences; missing keys retain defaults, obsolete keys are ignored.
    /// Returns false on failure; a missing file is saved with current defaults.
    bool Load();

    /// Saves current preferences to user_data/settings.json, omitting obsolete keys.
    bool Save();

private:
    // Resolves settings.json on each call using the shared user_data search and EXE fallback.
    std::wstring GetSettingsFilePath() const;
};

} // namespace kv
