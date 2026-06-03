// ============================================================================
// Class: WicMetadataReaderWriter
// Description: WicMetadataReaderWriter leverages the WIC API to parse and write XMP Dublin Core tags in standard-compliant files for [IMAGE_VIEWER_APP_NAME].
// Responsibility: Read and write XMP bag elements using Direct WIC query reader interfaces, falling back to full block transcoding if needed.
// ============================================================================
#pragma once

#include <wincodec.h>
#include <string>
#include <vector>

namespace kv {

class WicMetadataReaderWriter {
public:
    static bool ReadTags(
        IWICImagingFactory2* wicFactory,
        const std::wstring& filePath,
        std::vector<std::wstring>& outTags
    );

    static bool WriteTagsFast(
        IWICImagingFactory2* wicFactory,
        const std::wstring& filePath,
        GUID containerFormat,
        const std::vector<std::wstring>& tags
    );

    static bool WriteTagsTranscode(
        IWICImagingFactory2* wicFactory,
        const std::wstring& filePath,
        GUID containerFormat,
        const std::vector<std::wstring>& tags
    );

private:
    static std::wstring GetXmpQueryPathPrefix(const GUID& containerFormat);
    static void EnsureMetadataPath(
        IWICImagingFactory2* wicFactory,
        IWICMetadataQueryWriter* pQueryWriter,
        const std::wstring& path,
        bool forceCreate = false
    );
    static bool WritePngXmp(
        IWICMetadataQueryWriter* pQueryWriter,
        const std::vector<std::wstring>& tags
    );
};

} // namespace kv
