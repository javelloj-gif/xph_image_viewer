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
    struct CacheEntry {
        int64_t lastWriteTime = 0;
        std::vector<std::wstring> tags;
    };

    MetadataCache() = default;
    ~MetadataCache() = default;

    bool LoadCache();
    bool SaveCache();

    static std::wstring NormalizePath(const std::wstring& path);
    int64_t GetLastWriteTime(const std::wstring& filePath) const;
    std::wstring GetCacheFilePath() const;

    std::unordered_map<std::wstring, CacheEntry> m_cache;
    mutable std::mutex m_cacheMutex;
    bool m_cacheDirty = false;
};

} // namespace kv
