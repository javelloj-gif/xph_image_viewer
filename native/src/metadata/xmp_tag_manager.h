// ============================================================================
// Class: XmpTagManager
// Description: XmpTagManager acts as the primary service interface to query, modify, and search image metadata tags in [IMAGE_VIEWER_APP_NAME].
// Responsibility: Main entry/facade coordinating metadata requests, mapping calls to the cache, reader/writer, and directory scanners.
// ============================================================================
#pragma once

#include "common/types.h"
#include "metadata_cache.h"
#include "async_directory_scanner.h"
#include <wincodec.h>
#include <string>
#include <vector>

namespace kv {

class XmpTagManager {
public:
    XmpTagManager() = default;
    ~XmpTagManager() = default;

    XmpTagManager(const XmpTagManager&) = delete;
    XmpTagManager& operator=(const XmpTagManager&) = delete;

    void Initialize(IWICImagingFactory2* wicFactory);

    bool ReadTags(const std::wstring& filePath, std::vector<std::wstring>& outTags);
    bool WriteTags(const std::wstring& filePath, const std::vector<std::wstring>& tags);

    bool LoadCache();
    bool SaveCache();

    void SearchTagsAsync(
        const std::wstring& rootDirectory,
        const std::vector<std::wstring>& queryTags,
        SearchMatchCallback onMatchFound,
        SearchFinishedCallback onSearchFinished
    );

    void CancelSearch();
    bool IsSearchRunning() const;

private:
    IWICImagingFactory2* m_wicFactory = nullptr;
    MetadataCache m_cache;
    AsyncDirectoryScanner m_scanner;
};

} // namespace kv
