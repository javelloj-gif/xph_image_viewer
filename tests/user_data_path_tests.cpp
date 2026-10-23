#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "common/user_data_paths.h"

namespace fs = std::filesystem;

// Owns a unique temporary tree; cleanup never touches fixtures or application data.
class TemporaryTree {
public:
    // Creates an isolated tree using an OS-generated unique temporary filename.
    TemporaryTree() {
        wchar_t tempPath[MAX_PATH] = {};
        wchar_t uniquePath[MAX_PATH] = {};
        if (!GetTempPathW(MAX_PATH, tempPath) ||
            !GetTempFileNameW(tempPath, L"luv", 0, uniquePath)) {
            throw std::runtime_error("Cannot reserve temporary tree");
        }
        root = uniquePath;
        fs::remove(root);
        fs::create_directory(root);
    }
    // Removes only this instance's tree without hiding a test failure on unwind.
    ~TemporaryTree() {
        std::error_code error;
        fs::remove_all(root, error);
    }
    // Prevents multiple owners from deleting the same tree.
    TemporaryTree(const TemporaryTree&) = delete;
    TemporaryTree& operator=(const TemporaryTree&) = delete;
    fs::path root;
};

// Fails in every build configuration, including Release with NDEBUG defined.
void Require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

// Checks historical selection against independent expected paths in temporary trees.
int main() {
    try {
        TemporaryTree tree;
        fs::path exeDirectory = tree.root / L"rutas con espacios á";
        for (int i = 0; i < 8; ++i) exeDirectory /= L"child";
        fs::create_directories(exeDirectory);
        std::vector<fs::path> levels{exeDirectory};
        for (int i = 1; i <= 6; ++i) levels.push_back(levels.back().parent_path());

        const std::vector<fs::path (*)(const fs::path&)> resolvers{
            kv::ResolveUserDataRoot};
        int checks = 0;
        // Validates all selected resolvers without writing to the selected roots.
        const auto expect = [&](const fs::path& expected, const std::string& scenario) {
            for (const auto resolver : resolvers) {
                const auto selected = resolver(exeDirectory);
                Require(selected == expected, scenario + ": unexpected root");
                ++checks;
            }
        };

        expect(exeDirectory, "absence falls back beside EXE");
        Require(!fs::exists(exeDirectory / "user_data"), "resolver created user_data");
        fs::create_directory(levels[6] / "user_data");
        expect(exeDirectory, "seventh candidate excluded");
        for (int level = 5; level >= 0; --level) {
            fs::create_directory(levels[level] / "user_data");
            expect(levels[level] / "user_data", "nearest candidate wins at level " + std::to_string(level));
        }
        for (int level = 0; level <= 6; ++level) fs::remove(levels[level] / "user_data");
        expect(exeDirectory, "removal observed without cached root");

        for (int level = 0; level < 6; ++level) {
            fs::create_directory(levels[level] / "user_data");
            expect(levels[level] / "user_data", "single candidate " + std::to_string(level));
            fs::remove(levels[level] / "user_data");
        }
        fs::create_directory(levels[1] / "user_data");
        std::ofstream(exeDirectory / "settings.json") << "{}";
        std::ofstream(exeDirectory / "session.txt") << "";
        std::ofstream(exeDirectory / "fav.txt") << "";
        std::ofstream(exeDirectory / "xmp_tag_cache.bin") << "";
        expect(levels[1] / "user_data", "empty user_data wins over fallback files");
        std::ofstream(exeDirectory / "user_data") << "a file is an existing candidate";
        expect(exeDirectory / "user_data", "file candidate wins over ancestor directory");
        fs::remove(exeDirectory / "user_data");
        fs::remove(levels[1] / "user_data");

        // The historical loop keeps revisiting the volume root when parents run out.
        const auto volumeRoot = exeDirectory.root_path();
        const auto expectedAtRoot = fs::exists(volumeRoot / "user_data")
            ? volumeRoot / "user_data" : volumeRoot;
        for (const auto resolver : resolvers) {
            Require(resolver(volumeRoot) == expectedAtRoot, "volume root traversal");
            ++checks;
        }
        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        Require(kv::GetExecutableUserDataRoot() ==
            kv::ResolveUserDataRoot(fs::path(exePath).parent_path()), "Win32 adapter");
        ++checks;
        std::cout << "Passed " << checks << " path selection checks\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
