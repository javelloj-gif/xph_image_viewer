// ============================================================================
// Class: MetadataCache
// Description: MetadataCache maintains in-memory and on-disk indexed tags to speed up directory searches in [IMAGE_VIEWER_APP_NAME].
// Responsibility: Load, save, and maintain the binary database (`user_data/xmp_tag_cache.bin` using the `KTAG` v1 format) with path normalization.
// ============================================================================
#include "common/pch.h"
#include "metadata_cache.h"
#include "common/types.h"
#include <filesystem>
#include <fstream>
#include <chrono>

namespace kv {

std::wstring MetadataCache::NormalizePath(const std::wstring& path) {
    std::wstring result = path;
    for (auto& c : result) {
        if (c == L'/') c = L'\\';
    }
    if (result.length() >= 2 && result[1] == L':') {
        result[0] = static_cast<wchar_t>(::towupper(result[0]));
    }
    return result;
}

int64_t MetadataCache::GetLastWriteTime(const std::wstring& filePath) const {
    try {
        if (!std::filesystem::exists(filePath)) return 0;
        auto time = std::filesystem::last_write_time(filePath);
        return std::chrono::duration_cast<std::chrono::microseconds>(time.time_since_epoch()).count();
    } catch (...) {
        return 0;
    }
}

std::wstring MetadataCache::GetCacheFilePath() const {
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::filesystem::path p(exePath);
    for (int i = 0; i < 6; ++i) {
        p = p.parent_path();
        if (std::filesystem::exists(p / "user_data")) {
            return (p / "user_data" / "xmp_tag_cache.bin").wstring();
        }
    }
    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
    return (exeDir / "xmp_tag_cache.bin").wstring();
}

bool MetadataCache::LoadCache() {
    std::wstring path = GetCacheFilePath();
    if (!std::filesystem::exists(path)) {
        return false;
    }

    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        LogError(L"MetadataCache: Failed to open cache file for reading");
        return false;
    }

    char magic[4] = {};
    file.read(magic, 4);
    if (magic[0] != 'K' || magic[1] != 'T' || magic[2] != 'A' || magic[3] != 'G') {
        LogError(L"MetadataCache: Cache file invalid magic signature");
        return false;
    }

    uint32_t version = 0;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != 1) {
        LogError(L"MetadataCache: Cache file unsupported version");
        return false;
    }

    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));

    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_cache.clear();

    for (uint32_t i = 0; i < count; ++i) {
        int64_t lastWriteTime = 0;
        file.read(reinterpret_cast<char*>(&lastWriteTime), sizeof(lastWriteTime));

        uint32_t filePathLength = 0;
        file.read(reinterpret_cast<char*>(&filePathLength), sizeof(filePathLength));

        std::wstring filePath(filePathLength, L'\0');
        if (filePathLength > 0) {
            file.read(reinterpret_cast<char*>(&filePath[0]), filePathLength * sizeof(wchar_t));
        }

        uint32_t tagsCount = 0;
        file.read(reinterpret_cast<char*>(&tagsCount), sizeof(tagsCount));

        std::vector<std::wstring> tags;
        tags.reserve(tagsCount);

        for (uint32_t j = 0; j < tagsCount; ++j) {
            uint32_t tagLength = 0;
            file.read(reinterpret_cast<char*>(&tagLength), sizeof(tagLength));

            std::wstring tag(tagLength, L'\0');
            if (tagLength > 0) {
                file.read(reinterpret_cast<char*>(&tag[0]), tagLength * sizeof(wchar_t));
            }
            tags.push_back(tag);
        }

        m_cache[NormalizePath(filePath)] = CacheEntry{ lastWriteTime, tags };
    }

    m_cacheDirty = false;
    LogInfo(std::format(L"MetadataCache: Loaded {} entries from tags cache", count));
    return true;
}

bool MetadataCache::SaveCache() {
    std::wstring path = GetCacheFilePath();
    
    std::filesystem::path p(path);
    try {
        std::filesystem::create_directories(p.parent_path());
    } catch (...) {
        LogError(L"MetadataCache: Failed to create cache directory path");
        return false;
    }

    std::ofstream file(path, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!file.is_open()) {
        LogError(L"MetadataCache: Failed to open cache file for writing");
        return false;
    }

    std::lock_guard<std::mutex> lock(m_cacheMutex);

    char magic[4] = { 'K', 'T', 'A', 'G' };
    file.write(magic, 4);

    uint32_t version = 1;
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));

    uint32_t count = static_cast<uint32_t>(m_cache.size());
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const auto& [filePath, entry] : m_cache) {
        file.write(reinterpret_cast<const char*>(&entry.lastWriteTime), sizeof(entry.lastWriteTime));

        uint32_t filePathLength = static_cast<uint32_t>(filePath.length());
        file.write(reinterpret_cast<const char*>(&filePathLength), sizeof(filePathLength));
        if (filePathLength > 0) {
            file.write(reinterpret_cast<const char*>(&filePath[0]), filePathLength * sizeof(wchar_t));
        }

        uint32_t tagsCount = static_cast<uint32_t>(entry.tags.size());
        file.write(reinterpret_cast<const char*>(&tagsCount), sizeof(tagsCount));

        for (const auto& tag : entry.tags) {
            uint32_t tagLength = static_cast<uint32_t>(tag.length());
            file.write(reinterpret_cast<const char*>(&tagLength), sizeof(tagLength));
            if (tagLength > 0) {
                file.write(reinterpret_cast<const char*>(&tag[0]), tagLength * sizeof(wchar_t));
            }
        }
    }

    m_cacheDirty = false;
    LogInfo(std::format(L"MetadataCache: Saved {} entries to tags cache", count));
    return true;
}

} // namespace kv
