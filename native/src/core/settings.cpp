// ============================================================================
// Class: Settings
// Description: Settings handles serialization, deserialization, and runtime updates of user preferences for [IMAGE_VIEWER_APP_NAME].
// Responsibility: Load and save configurations from `settings.json`.
// ============================================================================
#include "common/pch.h"
#include "settings.h"
#include "common/types.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>

namespace kv {

static std::string ReadFileToString(const std::wstring& filePath) {
    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file.is_open()) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static std::string FindKeyValueRaw(const std::string& json, const std::string& key) {
    size_t keyPos = json.find("\"" + key + "\"");
    if (keyPos == std::string::npos) return "";

    size_t colonPos = json.find(":", keyPos);
    if (colonPos == std::string::npos) return "";

    size_t startPos = json.find("[", colonPos);
    if (startPos == std::string::npos) return "";

    size_t commaPos = json.find(",", startPos);
    size_t endBracketPos = json.find("]", startPos);
    size_t stopPos = (commaPos < endBracketPos) ? commaPos : endBracketPos;

    if (stopPos == std::string::npos) return "";

    std::string valStr = json.substr(startPos + 1, stopPos - startPos - 1);
    valStr.erase(0, valStr.find_first_not_of(" \t\r\n"));
    valStr.erase(valStr.find_last_not_of(" \t\r\n") + 1);

    return valStr;
}

static bool ParseBool(const std::string& valStr, bool defaultVal) {
    if (valStr == "true") return true;
    if (valStr == "false") return false;
    return defaultVal;
}

static float ParseFloat(const std::string& valStr, float defaultVal) {
    try {
        return std::stof(valStr);
    } catch (...) {
        return defaultVal;
    }
}

static int ParseInt(const std::string& valStr, int defaultVal) {
    try {
        return std::stoi(valStr);
    } catch (...) {
        return defaultVal;
    }
}

static std::wstring ParseWString(const std::string& valStr, const std::wstring& defaultVal) {
    if (valStr.size() >= 2 && valStr.front() == '"' && valStr.back() == '"') {
        std::string s = valStr.substr(1, valStr.size() - 2);
        int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
        if (len > 0) {
            std::vector<wchar_t> w(len);
            MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), len);
            return std::wstring(w.data());
        }
    }
    return defaultVal;
}

std::wstring Settings::GetSettingsFilePath() const {
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::filesystem::path p(exePath);
    for (int i = 0; i < 6; ++i) {
        p = p.parent_path();
        if (std::filesystem::exists(p / "user_data")) {
            return (p / "user_data" / "settings.json").wstring();
        }
    }
    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
    return (exeDir / "settings.json").wstring();
}

bool Settings::Load() {
    auto trace = [](std::string_view msg) {
        std::ofstream dbg("debug_ipc.txt", std::ios::app);
        dbg << "[SETTINGS] Load step: " << msg << std::endl;
        dbg.close();
    };
    trace("Start");

    std::wstring path = GetSettingsFilePath();
    trace("Path retrieved");

    if (!std::filesystem::exists(path)) {
        trace("Path does not exist");
        LogWarn(L"Settings file does not exist, creating default settings.");
        Save();
        return false;
    }
    trace("Path exists");

    std::string json = ReadFileToString(path);
    trace("File read to string");
    
    if (json.empty()) {
        trace("JSON is empty");
        LogError(L"Failed to read settings file or file is empty.");
        return false;
    }
    trace("JSON is not empty, starting parsing");

    ui_lang = ParseWString(FindKeyValueRaw(json, "ui_lang"), ui_lang);
    page_on_app_launch_from_explorer = ParseInt(FindKeyValueRaw(json, "page_on_app_launch_from_explorer"), page_on_app_launch_from_explorer);
    page_to_open_on_drag_and_drop = ParseInt(FindKeyValueRaw(json, "page_to_open_on_drag_and_drop"), page_to_open_on_drag_and_drop);
    debug_page = ParseInt(FindKeyValueRaw(json, "debug_page"), debug_page);
    run_on_windows_startup = ParseBool(FindKeyValueRaw(json, "run_on_windows_startup"), run_on_windows_startup);
    do_not_show_start_dialog = ParseBool(FindKeyValueRaw(json, "do_not_show_start_dialog"), do_not_show_start_dialog);
    show_fullscreen = ParseBool(FindKeyValueRaw(json, "show_fullscreen"), show_fullscreen);
    doubleclick_toggle = ParseBool(FindKeyValueRaw(json, "doubleclick_toggle"), doubleclick_toggle);
    hide_to_tray_on_close = ParseBool(FindKeyValueRaw(json, "hide_to_tray_on_close"), hide_to_tray_on_close);
    hide_on_app_start = ParseBool(FindKeyValueRaw(json, "hide_on_app_start"), hide_on_app_start);
    single_instance = ParseBool(FindKeyValueRaw(json, "single_instance"), single_instance);
    desaturated_corner_buttons_and_corner_menus = ParseBool(FindKeyValueRaw(json, "desaturated_corner_buttons_and_corner_menus"), desaturated_corner_buttons_and_corner_menus);
    show_console_output = ParseBool(FindKeyValueRaw(json, "show_console_output"), show_console_output);
    effects = ParseBool(FindKeyValueRaw(json, "effects"), effects);
    show_noise_cells = ParseBool(FindKeyValueRaw(json, "show_noise_cells"), show_noise_cells);

    settings_window_opacity = ParseFloat(FindKeyValueRaw(json, "settings_window_opacity"), settings_window_opacity);
    window_opacity = ParseFloat(FindKeyValueRaw(json, "window_opacity"), window_opacity);

    animated_zoom = ParseBool(FindKeyValueRaw(json, "animated_zoom"), animated_zoom);
    draw_control_panel_backplate = ParseBool(FindKeyValueRaw(json, "draw_control_panel_backplate"), draw_control_panel_backplate);
    thumbnail_width = ParseFloat(FindKeyValueRaw(json, "thumbnail_width"), thumbnail_width);
    zoom_on_mousewheel = ParseBool(FindKeyValueRaw(json, "zoom_on_mousewheel"), zoom_on_mousewheel);
    draw_default_thumbnail = ParseBool(FindKeyValueRaw(json, "draw_default_thumbnail"), draw_default_thumbnail);
    show_thirds = ParseBool(FindKeyValueRaw(json, "show_thirds"), show_thirds);
    show_cyberpunk = ParseBool(FindKeyValueRaw(json, "show_cyberpunk"), show_cyberpunk);
    show_metadata = ParseBool(FindKeyValueRaw(json, "show_metadata"), show_metadata);
    sort_mode = ParseInt(FindKeyValueRaw(json, "sort_mode"), sort_mode);

    LogInfo(L"Settings loaded successfully");
    return true;
}

bool Settings::Save() {
    std::wstring path = GetSettingsFilePath();

    std::filesystem::path dir(path);
    try {
        std::filesystem::create_directories(dir.parent_path());
    } catch (...) {
        LogError(L"Failed to create settings directory path");
        return false;
    }

    std::ofstream file(path, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!file.is_open()) {
        LogError(L"Failed to open settings file for writing");
        return false;
    }

    auto toUtf8 = [](const std::wstring& wstr) -> std::string {
        if (wstr.empty()) return "";
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    };

    file << "{\n \"settings\": {\n";

    file << "  \"---general\": \"General\",\n";
    file << "  \"ui_lang\": [\n   \"" << toUtf8(ui_lang) << "\",\n   \"UI language\"\n  ],\n";
    file << "  \"page_on_app_launch_from_explorer\": [\n   " << page_on_app_launch_from_explorer << ",\n   [\n    [2, \"VIEWER\"],\n    [3, \"BOARD\"],\n    [4, \"LIBRARY\"],\n    [5, \"WATERFALL\"]\n   ],\n   \"Page on App Launch (when opened from Explorer)\"\n  ],\n";
    file << "  \"page_to_open_on_drag_and_drop\": [\n   " << page_to_open_on_drag_and_drop << ",\n   [\n    [2, \"VIEWER\"],\n    [3, \"BOARD\"],\n    [4, \"LIBRARY\"],\n    [5, \"WATERFALL\"]\n   ],\n   \"Page to open on Drag&Drop\"\n  ],\n";
    file << "  \"debug_page\": [\n   " << debug_page << ",\n   [\n    [1, \"START\"],\n    [2, \"VIEWER\"],\n    [3, \"BOARD\"],\n    [4, \"LIBRARY\"],\n    [5, \"WATERFALL\"]\n   ],\n   \"Page to open when run debug mode\"\n  ],\n";
    file << "  \"run_on_windows_startup\": [\n   " << (run_on_windows_startup ? "true" : "false") << ",\n   \"Run on Windows Startup\"\n  ],\n";
    file << "  \"do_not_show_start_dialog\": [\n   " << (do_not_show_start_dialog ? "true" : "false") << ",\n   \"Supress start dialog and run lite mode\"\n  ],\n";
    file << "  \"show_fullscreen\": [\n   " << (show_fullscreen ? "true" : "false") << ",\n   \"Full-screen mode on application start\"\n  ],\n";
    file << "  \"doubleclick_toggle\": [\n   " << (doubleclick_toggle ? "true" : "false") << ",\n   \"Toggle between full-screen and window mode via double click\"\n  ],\n";
    file << "  \"hide_to_tray_on_close\": [\n   " << (hide_to_tray_on_close ? "true" : "false") << ",\n   \"Hide to tray on close\"\n  ],\n";
    file << "  \"hide_on_app_start\": [\n   " << (hide_on_app_start ? "true" : "false") << ",\n   \"Hide to tray on app start\"\n  ],\n";
    file << "  \"single_instance\": [\n   " << (single_instance ? "true" : "false") << ",\n   \"Single instance enforcement\"\n  ],\n";
    file << "  \"desaturated_corner_buttons_and_corner_menus\": [\n   " << (desaturated_corner_buttons_and_corner_menus ? "true" : "false") << ",\n   \"Desaturated corner buttons and corner menus\"\n  ],\n";
    file << "  \"show_console_output\": [\n   " << (show_console_output ? "true" : "false") << ",\n   \"Show standard (console) output overlay\"\n  ],\n";
    file << "  \"effects\": [\n   " << (effects ? "true" : "false") << ",\n   \"Animated effects\"\n  ],\n";
    file << "  \"show_noise_cells\": [\n   " << (show_noise_cells ? "true" : "false") << ",\n   \"Show animated cells overlay\"\n  ],\n";

    file << "  \"settings_window_opacity\": [\n   " << settings_window_opacity << ",\n   [\n    0.1,\n    1.0\n   ],\n   \"Settings window opacity\"\n  ],\n";
    file << "  \"window_opacity\": [\n   " << window_opacity << ",\n   [\n    0.0,\n    1.0\n   ],\n   \"Application window opacity\"\n  ],\n";

    file << "  \"---viewerpage\": \"Viewer page\",\n";
    file << "  \"animated_zoom\": [\n   " << (animated_zoom ? "true" : "false") << ",\n   \"Animated zoom\"\n  ],\n";
    file << "  \"draw_control_panel_backplate\": [\n   " << (draw_control_panel_backplate ? "true" : "false") << ",\n   \"Draw backplate for control panel\"\n  ],\n";
    file << "  \"thumbnail_width\": [\n   " << thumbnail_width << ",\n   [\n    30.0,\n    100.0\n   ],\n   \"Thumbnails size\"\n  ],\n";
    file << "  \"zoom_on_mousewheel\": [\n   " << (zoom_on_mousewheel ? "true" : "false") << ",\n   \"Enable mouse wheel to zoom and Ctrl+mouse wheel to navigate through image list\"\n  ],\n";
    file << "  \"draw_default_thumbnail\": [\n   " << (draw_default_thumbnail ? "true" : "false") << ",\n   \"Show dummy-default thumbnail while generated one is not ready\"\n  ],\n";
    file << "  \"show_thirds\": [\n   " << (show_thirds ? "true" : "false") << ",\n   \"Show thirds\"\n  ],\n";
    file << "  \"show_cyberpunk\": [\n   " << (show_cyberpunk ? "true" : "false") << ",\n   \"Cyberpunk frame\"\n  ],\n";
    file << "  \"show_metadata\": [\n   " << (show_metadata ? "true" : "false") << ",\n   \"Show metadata\"\n  ],\n";
    file << "  \"sort_mode\": [\n   " << sort_mode << ",\n   \"Sort mode\"\n  ]\n";

    file << " }\n}\n";

    LogInfo(L"Settings saved successfully");
    return true;
}

} // namespace kv
