// ============================================================================
// Class: XmpTagManager
// Description: XmpTagManager acts as the primary service interface to query, modify, and search image metadata tags in [IMAGE_VIEWER_APP_NAME].
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

namespace kv {

void XmpTagManager::Initialize(IWICImagingFactory2* wicFactory) {
    m_wicFactory = wicFactory;
    m_cache.LoadCache();
    LogInfo(L"XmpTagManager initialized successfully");
}

bool XmpTagManager::ReadTags(const std::wstring& filePath, std::vector<std::wstring>& outTags) {
    std::wstring normalized = MetadataCache::NormalizePath(filePath);
    return WicMetadataReaderWriter::ReadTags(m_wicFactory, normalized, outTags);
}

bool XmpTagManager::WriteTags(const std::wstring& filePath, const std::vector<std::wstring>& tags) {
    std::wstring normalized = MetadataCache::NormalizePath(filePath);
    if (!m_wicFactory) {
        LogError(L"XmpTagManager: Factory not initialized");
        return false;
    }

    GUID containerFormat = {};
    {
        ComPtr<IWICBitmapDecoder> pDecoder;
        HRESULT hr = m_wicFactory->CreateDecoderFromFilename(
            normalized.c_str(),
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnDemand,
            &pDecoder
        );
        if (FAILED(hr)) {
            LogError(std::format(L"XmpTagManager: Failed to open decoder for format check hr: 0x{:08X}", static_cast<unsigned>(hr)));
            return false;
        }
        pDecoder->GetContainerFormat(&containerFormat);
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
        return true;
    }

    return false;
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

} // namespace kv
