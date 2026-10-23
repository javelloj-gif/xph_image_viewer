#pragma once

#include <filesystem>

namespace kv {

// Resolves the first existing user_data at exeDirectory or its five parents,
// accepting any existing entry. Falls back to exeDirectory; no writes or cache.
// Filesystem exceptions propagate. Callers supply the EXE directory, not its filename.
std::filesystem::path ResolveUserDataRoot(const std::filesystem::path& exeDirectory);

// Resolves on each call using the current EXE's historical MAX_PATH Win32 lookup.
// Owns no shared state, creates no data and propagates filesystem exceptions.
std::filesystem::path GetExecutableUserDataRoot();

} // namespace kv
