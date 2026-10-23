#include "user_data_paths.h"
#include <Windows.h>

namespace kv {

std::filesystem::path ResolveUserDataRoot(const std::filesystem::path& exeDirectory) {
    auto directory = exeDirectory;
    for (int i = 0; i < 6; ++i) {
        if (std::filesystem::exists(directory / "user_data")) {
            return directory / "user_data";
        }
        directory = directory.parent_path();
    }
    return exeDirectory;
}

std::filesystem::path GetExecutableUserDataRoot() {
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    return ResolveUserDataRoot(std::filesystem::path(exePath).parent_path());
}

} // namespace kv
