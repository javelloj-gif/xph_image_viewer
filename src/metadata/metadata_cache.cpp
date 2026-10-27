// ============================================================================
// Class: MetadataCache
// Description: MetadataCache maintains in-memory and on-disk indexed tags to speed up directory searches in Fotassa.
// Responsibility: Load, save, and maintain the binary database (`user_data/xmp_tag_cache.bin` using the `KTAG` v1 format) with path normalization.
// ============================================================================
#include "common/pch.h"
#include "metadata_cache.h"
#include "common/types.h"
#include "common/user_data_paths.h"
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
    return (GetExecutableUserDataRoot() / "xmp_tag_cache.bin").wstring();
}

bool MetadataCache::LoadCache() {
    try {
        std::ifstream file(GetCacheFilePath(), std::ios::binary | std::ios::ate);
        if (!file) return false;
        const auto end = file.tellg();
        if (end < std::streampos{0}) return false;
        auto remaining = static_cast<uint64_t>(static_cast<std::streamoff>(end));
        file.seekg(0);
        if (!file) return false;

        // Bounds every read by actual file bytes; never allocate from unchecked counts.
        const auto read = [&](void* data, size_t bytes) {
            if (bytes > remaining) return false;
            if (bytes && !file.read(static_cast<char*>(data), static_cast<std::streamsize>(bytes))) {
                return false;
            }
            remaining -= bytes;
            return true;
        };
        // Division before multiplication also prevents overflow for corrupt UTF-16 lengths.
        const auto readString = [&](std::wstring& value) {
            uint32_t length = 0;
            if (!read(&length, sizeof(length)) || length > remaining / sizeof(wchar_t)) return false;
            value.resize(length);
            return read(value.data(), static_cast<size_t>(length) * sizeof(wchar_t));
        };

        char magic[4] = {};
        uint32_t version = 0;
        uint32_t count = 0;
        if (!read(magic, sizeof(magic)) || std::string_view(magic, 4) != "KTAG" ||
            !read(&version, sizeof(version)) || version != 1 ||
            !read(&count, sizeof(count))) return false;
        constexpr size_t minimumEntryBytes = sizeof(int64_t) + 2 * sizeof(uint32_t);
        if (count > remaining / minimumEntryBytes) return false;

        // Preserve serialization with cache writers while publishing only a complete parse.
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        std::unordered_map<std::wstring, CacheEntry> loaded;
        for (uint32_t i = 0; i < count; ++i) {
            int64_t lastWriteTime = 0;
            std::wstring filePath;
            uint32_t tagsCount = 0;
            if (!read(&lastWriteTime, sizeof(lastWriteTime)) || !readString(filePath) ||
                !read(&tagsCount, sizeof(tagsCount)) || tagsCount > remaining / sizeof(uint32_t)) {
                return false;
            }
            std::vector<std::wstring> tags;
            for (uint32_t j = 0; j < tagsCount; ++j) {
                std::wstring tag;
                if (!readString(tag)) return false;
                tags.push_back(std::move(tag));
            }
            loaded[NormalizePath(filePath)] = CacheEntry{lastWriteTime, std::move(tags)};
        }
        m_cache.swap(loaded);
        m_cacheDirty = false;
        return true;
    } catch (const std::exception&) {
        LogError(L"MetadataCache: Failed to read complete cache; live entries retained");
        return false;
    }
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
