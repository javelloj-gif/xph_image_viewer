// ============================================================================
// Class: MetadataCache
// Description: MetadataCache maintains in-memory and on-disk indexed tags to speed up directory searches in [IMAGE_VIEWER_APP_NAME].
// Responsibility: Load, save, and maintain the binary database (`user_data/xmp_tag_cache.bin` using the `KTAG` v1 format) with path normalization.
// ============================================================================
#pragma once

#include "common/types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace kv {

class MetadataCache {
public:
    // Owns the timestamp and tags of one normalized path in KTAG v1.
    struct CacheEntry {
        int64_t lastWriteTime = 0;
        std::vector<std::wstring> tags;
    };

    MetadataCache() = default;
    ~MetadataCache() = default;

    // Serializes with cache writers; publishes a complete KTAG v1 parse only. False retains entries/dirty.
    // Counts are bounded by file bytes, not an arbitrary valid-cache size cap; active readers may block.
    bool LoadCache();
    bool SaveCache();

    static std::wstring NormalizePath(const std::wstring& path);
    int64_t GetLastWriteTime(const std::wstring& filePath) const;
    // Resolves xmp_tag_cache.bin on each call using the shared user_data search and EXE fallback.
    std::wstring GetCacheFilePath() const;

    std::unordered_map<std::wstring, CacheEntry> m_cache;
    mutable std::mutex m_cacheMutex;
    bool m_cacheDirty = false;
};

} // namespace kv
