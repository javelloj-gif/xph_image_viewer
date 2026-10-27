// ============================================================================
// Class: XmpTagManager
// Description: XmpTagManager acts as the primary service interface to query, modify, and search image metadata tags in Fotassa.
// Responsibility: Main entry/facade coordinating metadata requests, mapping calls to the cache, reader/writer, and directory scanners.
// ============================================================================
#include "common/pch.h"
#include "xmp_tag_manager.h"
#include "png_chunk_injector.h"
#include "jpeg_segment_injector.h"
#include "wic_metadata_reader_writer.h"
#include "common/types.h"
#include <filesystem>
#include <format>
#include <propsys.h>
#include <propkey.h>
#include <propvarutil.h>
#pragma comment(lib, "propsys.lib")

namespace kv {

void XmpTagManager::Initialize(IWICImagingFactory2* wicFactory) {
    m_wicFactory = wicFactory;
    m_cache.LoadCache();
    LogInfo(L"XmpTagManager initialized successfully");
}

bool XmpTagManager::ReadTags(const std::wstring& filePath, std::vector<std::wstring>& outTags) {
    std::wstring normalized = MetadataCache::NormalizePath(filePath);
    {
        std::lock_guard<std::mutex> lock(m_cache.m_cacheMutex);
        auto it = m_cache.m_cache.find(normalized);
        if (it != m_cache.m_cache.end()) {
            int64_t currentWriteTime = m_cache.GetLastWriteTime(normalized);
            if (it->second.lastWriteTime == currentWriteTime) {
                outTags = it->second.tags;
                return true;
            }
        }
    }

    bool success = WicMetadataReaderWriter::ReadTags(m_wicFactory, normalized, outTags);
    if (!success || outTags.empty()) {
        outTags.clear();
        ComPtr<IPropertyStore> pStore;
        HRESULT hr = SHGetPropertyStoreFromParsingName(normalized.c_str(), nullptr, GPS_DEFAULT, IID_PPV_ARGS(&pStore));
        if (SUCCEEDED(hr)) {
            PROPVARIANT var;
            PropVariantInit(&var);
            hr = pStore->GetValue(PKEY_Keywords, &var);
            if (SUCCEEDED(hr)) {
                if (var.vt == (VT_VECTOR | VT_LPWSTR)) {
                    for (ULONG i = 0; i < var.calpwstr.cElems; ++i) {
                        if (var.calpwstr.pElems[i]) {
                            outTags.push_back(var.calpwstr.pElems[i]);
                        }
                    }
                    if (!outTags.empty()) {
                        success = true;
                    }
                } else {
                    LogInfo(std::format(L"ReadTags PropertyStore: vt type is {}", var.vt));
                }
                PropVariantClear(&var);
            } else {
                LogWarn(std::format(L"ReadTags PropertyStore: GetValue failed with hr=0x{:X}", static_cast<unsigned>(hr)));
            }
        } else {
            LogWarn(std::format(L"ReadTags PropertyStore: SHGetPropertyStore failed with hr=0x{:X}", static_cast<unsigned>(hr)));
        }
    }

    if (success) {
        std::lock_guard<std::mutex> lock(m_cache.m_cacheMutex);
        m_cache.m_cache[normalized] = MetadataCache::CacheEntry{ m_cache.GetLastWriteTime(normalized), outTags };
        m_cache.m_cacheDirty = true;
    }
    return success;
}

bool XmpTagManager::WriteTags(const std::wstring& filePath, const std::vector<std::wstring>& tags) {
    std::wstring normalized = MetadataCache::NormalizePath(filePath);
    if (!m_wicFactory) {
        LogError(L"XmpTagManager: Factory not initialized");
        return false;
    }

    std::wstring ext = std::filesystem::path(normalized).extension().wstring();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    GUID containerFormat = {};
    if (ext == L".png" || ext == L".apng") {
        containerFormat = GUID_ContainerFormatPng;
    } else if (ext == L".jpg" || ext == L".jpeg" || ext == L".jfif") {
        containerFormat = GUID_ContainerFormatJpeg;
    } else if (ext == L".tif" || ext == L".tiff") {
        containerFormat = GUID_ContainerFormatTiff;
    }

    if (containerFormat == GUID_ContainerFormatPng) {
        bool pngOk = PngChunkInjector::InjectPngITxtChunk(normalized, tags);
        if (pngOk) {
            {
                std::lock_guard<std::mutex> lock(m_cache.m_cacheMutex);
                m_cache.m_cache[normalized] = MetadataCache::CacheEntry{ m_cache.GetLastWriteTime(normalized), tags };
                m_cache.m_cacheDirty = true;
            }
            m_cache.SaveCache();
            SyncToWindowsKeywords(normalized, tags);
        }
        return pngOk;
    }

    if (containerFormat == GUID_ContainerFormatJpeg) {
        bool jpegOk = JpegSegmentInjector::InjectJpegXmpSegment(normalized, tags);
        if (jpegOk) {
            {
                std::lock_guard<std::mutex> lock(m_cache.m_cacheMutex);
                m_cache.m_cache[normalized] = MetadataCache::CacheEntry{ m_cache.GetLastWriteTime(normalized), tags };
                m_cache.m_cacheDirty = true;
            }
            m_cache.SaveCache();
            SyncToWindowsKeywords(normalized, tags);
        }
        return jpegOk;
    }

    bool fastOk = WicMetadataReaderWriter::WriteTagsFast(m_wicFactory, normalized, containerFormat, tags);
    if (fastOk) {
        LogInfo(std::format(L"XmpTagManager: Saved tags to {} using Fast Metadata Encoding", normalized));
        {
            std::lock_guard<std::mutex> lock(m_cache.m_cacheMutex);
            m_cache.m_cache[normalized] = MetadataCache::CacheEntry{ m_cache.GetLastWriteTime(normalized), tags };
            m_cache.m_cacheDirty = true;
        }
        m_cache.SaveCache();
        SyncToWindowsKeywords(normalized, tags);
        return true;
    }

    LogInfo(std::format(L"XmpTagManager: Fast encoding failed or unsupported for {}. Falling back to full transcoding.", normalized));
    bool transcodeOk = WicMetadataReaderWriter::WriteTagsTranscode(m_wicFactory, normalized, containerFormat, tags);
    if (transcodeOk) {
        LogInfo(std::format(L"XmpTagManager: Saved tags to {} using Full Transcoding", normalized));
        {
            std::lock_guard<std::mutex> lock(m_cache.m_cacheMutex);
            m_cache.m_cache[normalized] = MetadataCache::CacheEntry{ m_cache.GetLastWriteTime(normalized), tags };
            m_cache.m_cacheDirty = true;
        }
        m_cache.SaveCache();
        SyncToWindowsKeywords(normalized, tags);
        return true;
    }

    return false;
}

bool XmpTagManager::ReadMetadataExtended(
    const std::wstring& filePath, 
    std::wstring& outTitle, 
    std::wstring& outCreator, 
    std::wstring& outDescription, 
    int& outRating,
    std::wstring& outDeviceMake,
    std::wstring& outDeviceModel
) {
    std::wstring normalized = MetadataCache::NormalizePath(filePath);
    return WicMetadataReaderWriter::ReadMetadataExtended(
        m_wicFactory, 
        normalized, 
        outTitle, 
        outCreator, 
        outDescription, 
        outRating, 
        outDeviceMake, 
        outDeviceModel
    );
}

bool XmpTagManager::WriteMetadataExtended(
    const std::wstring& filePath, 
    const std::vector<std::wstring>& tags,
    const std::wstring& title, 
    const std::wstring& creator, 
    const std::wstring& description, 
    int rating
) {
    std::wstring normalized = MetadataCache::NormalizePath(filePath);
    if (!m_wicFactory) {
        LogError(L"XmpTagManager: Factory not initialized");
        return false;
    }

    std::wstring ext = std::filesystem::path(normalized).extension().wstring();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    GUID containerFormat = {};
    if (ext == L".png" || ext == L".apng") {
        containerFormat = GUID_ContainerFormatPng;
    } else if (ext == L".jpg" || ext == L".jpeg" || ext == L".jfif") {
        containerFormat = GUID_ContainerFormatJpeg;
    } else if (ext == L".tif" || ext == L".tiff") {
        containerFormat = GUID_ContainerFormatTiff;
    }

    bool success = false;
    if (containerFormat == GUID_ContainerFormatPng) {
        success = PngChunkInjector::InjectPngITxtChunk(normalized, tags, title, creator, description, rating);
    } else if (containerFormat == GUID_ContainerFormatJpeg) {
        success = JpegSegmentInjector::InjectJpegXmpSegment(normalized, tags, title, creator, description, rating);
    } else {
        success = WicMetadataReaderWriter::WriteMetadataExtendedFast(m_wicFactory, normalized, containerFormat, tags, title, creator, description, rating);
        if (!success) {
            LogInfo(std::format(L"XmpTagManager: Fast metadata encoding failed for {}. Falling back to transcoding.", normalized));
            success = WicMetadataReaderWriter::WriteMetadataExtendedTranscode(m_wicFactory, normalized, containerFormat, tags, title, creator, description, rating);
        }
    }

    if (success) {
        {
            std::lock_guard<std::mutex> lock(m_cache.m_cacheMutex);
            m_cache.m_cache[normalized] = MetadataCache::CacheEntry{ m_cache.GetLastWriteTime(normalized), tags };
            m_cache.m_cacheDirty = true;
        }
        m_cache.SaveCache();
        SyncToWindowsKeywords(normalized, tags);
    }

    return success;
}

bool XmpTagManager::LoadCache() {
    return m_cache.LoadCache();
}

bool XmpTagManager::SaveCache() {
    return m_cache.SaveCache();
}

void XmpTagManager::SearchTagsAsync(
    const std::wstring& rootDirectory,
    const std::vector<std::wstring>& queryTags,
    SearchMatchCallback onMatchFound,
    SearchFinishedCallback onSearchFinished
) {
    m_scanner.SearchTagsAsync(m_wicFactory, m_cache, rootDirectory, queryTags, onMatchFound, onSearchFinished);
}

void XmpTagManager::CancelSearch() {
    m_scanner.CancelSearch();
}

bool XmpTagManager::IsSearchRunning() const {
    return m_scanner.IsSearchRunning();
}

void XmpTagManager::SyncToWindowsKeywords(const std::wstring& filePath, const std::vector<std::wstring>& tags) {
    ComPtr<IPropertyStore> pStore;
    HRESULT hr = SHGetPropertyStoreFromParsingName(filePath.c_str(), nullptr, GPS_READWRITE, IID_PPV_ARGS(&pStore));
    if (SUCCEEDED(hr)) {
        std::vector<PCWSTR> ptrs;
        ptrs.reserve(tags.size());
        for (const auto& tag : tags) ptrs.push_back(tag.c_str());
        PROPVARIANT var;
        PropVariantInit(&var);
        if (SUCCEEDED(InitPropVariantFromStringVector(ptrs.data(), static_cast<ULONG>(ptrs.size()), &var))) {
            pStore->SetValue(PKEY_Keywords, var);
            pStore->Commit();
            PropVariantClear(&var);
            LogInfo(std::format(L"Synced {} keywords to Windows Property Store for {}", tags.size(), filePath));
        } else {
            LogError(L"SyncToWindowsKeywords: InitPropVariantFromStringVector failed");
        }
    } else {
        LogWarn(std::format(L"SyncToWindowsKeywords: SHGetPropertyStoreFromParsingName failed with hr=0x{:X}", static_cast<unsigned>(hr)));
    }
}

} // namespace kv
