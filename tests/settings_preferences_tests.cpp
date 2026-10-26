#include "common/pch.h"
#include "core/settings.h"
#include "common/user_data_paths.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

// Exercises the production serializer only after proving the wrapper owns the EXE-local data tree.
int RunSettingsPreferencesTests() {
    try {
        int checks = 0;
        const auto check = [&](bool ok, const char* label) {
            if (!ok) throw std::runtime_error(label);
            ++checks;
        };
        wchar_t executable[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, executable, MAX_PATH);
        const auto root = std::filesystem::path(executable).parent_path();
        const auto data = root / "user_data";
        check(std::filesystem::is_regular_file(root / "preferences-test.marker") &&
            std::filesystem::is_directory(data) && kv::GetExecutableUserDataRoot() == data,
            "wrapper-owned EXE-local data root required before any write");
        const auto path = data / "settings.json";
        const auto read = [](const std::filesystem::path& file) {
            std::ifstream stream(file, std::ios::binary);
            return std::string(std::istreambuf_iterator<char>(stream), {});
        };
        const auto write = [&](const std::string& json) {
            std::ofstream stream(path, std::ios::binary | std::ios::trunc);
            stream << json;
            check(stream.good(), "synthetic settings written");
        };
        kv::Settings missing;
        check(missing.animated_zoom && !std::filesystem::exists(path), "fresh zoom default");
        check(!missing.Load() && missing.animated_zoom, "absent file retains default and reports absence");
        check(std::filesystem::exists(path), "absent file saves defaults as before");
        write("{\"settings\": {\"effects\": [false]}}");
        kv::Settings partial;
        check(partial.Load() && partial.animated_zoom && !partial.effects, "missing zoom key retains true");
        for (const bool zoom : {false, true}) {
            write(std::string("{\"settings\": {\"animated_zoom\": [") + (zoom ? "true" : "false") +
                "],\"effects\": [false],\"thumbnail_width\": [73],\"window_opacity\": [0.625],"
                "\"settings_window_opacity\": [0.75],\"ui_lang\": [\"es\"],\"sort_mode\": [3],"
                "\"doubleclick_toggle\": [false],\"desaturated_corner_buttons_and_corner_menus\": [true],"
                "\"draw_control_panel_backplate\": [true],\"show_cyberpunk\": [true]}}");
            kv::Settings old;
            check(old.Load() && old.animated_zoom == zoom, "legacy file honors explicit zoom");
            check(old.Save(), "legacy round trip saved");
            const auto json = read(path);
            for (const auto* key : {"doubleclick_toggle", "desaturated_corner_buttons_and_corner_menus",
                 "draw_control_panel_backplate", "show_cyberpunk"}) {
                check(json.find(key) == std::string::npos, "obsolete keys omitted");
            }
            kv::Settings restored;
            check(restored.Load() && restored.animated_zoom == zoom && !restored.effects &&
                restored.thumbnail_width == 73 && restored.window_opacity == 0.625f &&
                restored.settings_window_opacity == 0.75f && restored.ui_lang == L"es" && restored.sort_mode == 3,
                "current preferences survive legacy round trip");
        }
        const auto packaged = read(root / "default-settings.json");
        check(!packaged.empty(), "packaging fixture available");
        write(packaged);
        kv::Settings defaults;
        check(defaults.Load() && defaults.animated_zoom && defaults.effects && defaults.thumbnail_width == 50,
            "packaged zoom default with existing effects and thumbnail size");
        check(defaults.Save() && read(root / "default-settings.json") == packaged, "packaging source copy untouched");
        for (const auto* key : {"doubleclick_toggle", "desaturated_corner_buttons_and_corner_menus",
             "draw_control_panel_backplate", "show_cyberpunk"}) {
            check(packaged.find(key) == std::string::npos, "packaging omits obsolete keys");
        }
        std::cout << "settings preference checks: " << checks << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
