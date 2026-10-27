// ============================================================================
// Class: XmpTagManager
// Description: XmpTagManager acts as the primary service interface to query, modify, and search image metadata tags in Fotassa.
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
    friend class AsyncPreviewTests; // Holds cache access to verify cooperative cancellation at a real read boundary.
public:
    XmpTagManager() = default;
    ~XmpTagManager() = default;

    XmpTagManager(const XmpTagManager&) = delete;
    XmpTagManager& operator=(const XmpTagManager&) = delete;

    void Initialize(IWICImagingFactory2* wicFactory);
    IWICImagingFactory2* GetWicFactory() const { return m_wicFactory; }

    bool ReadTags(const std::wstring& filePath, std::vector<std::wstring>& outTags);
    bool WriteTags(const std::wstring& filePath, const std::vector<std::wstring>& tags);

    bool ReadMetadataExtended(
        const std::wstring& filePath, 
        std::wstring& outTitle, 
        std::wstring& outCreator, 
        std::wstring& outDescription, 
        int& outRating,
        std::wstring& outDeviceMake,
        std::wstring& outDeviceModel
    );

    bool WriteMetadataExtended(
        const std::wstring& filePath, 
        const std::vector<std::wstring>& tags,
        const std::wstring& title, 
        const std::wstring& creator, 
        const std::wstring& description, 
        int rating
    );

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
    void SyncToWindowsKeywords(const std::wstring& filePath, const std::vector<std::wstring>& tags);

    IWICImagingFactory2* m_wicFactory = nullptr;
    MetadataCache m_cache;
    AsyncDirectoryScanner m_scanner;
};

} // namespace kv
