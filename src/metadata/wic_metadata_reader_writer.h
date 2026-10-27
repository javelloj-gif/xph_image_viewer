// ============================================================================
// Class: WicMetadataReaderWriter
// Description: WicMetadataReaderWriter leverages the WIC API to parse and write XMP Dublin Core tags in standard-compliant files for Fotassa.
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

    static bool ReadMetadataExtended(
        IWICImagingFactory2* wicFactory,
        const std::wstring& filePath,
        std::wstring& outTitle,
        std::wstring& outCreator,
        std::wstring& outDescription,
        int& outRating,
        std::wstring& outDeviceMake,
        std::wstring& outDeviceModel
    );

    static bool WriteMetadataExtendedFast(
        IWICImagingFactory2* wicFactory,
        const std::wstring& filePath,
        GUID containerFormat,
        const std::vector<std::wstring>& tags,
        const std::wstring& title,
        const std::wstring& creator,
        const std::wstring& description,
        int rating
    );

    static bool WriteMetadataExtendedTranscode(
        IWICImagingFactory2* wicFactory,
        const std::wstring& filePath,
        GUID containerFormat,
        const std::vector<std::wstring>& tags,
        const std::wstring& title,
        const std::wstring& creator,
        const std::wstring& description,
        int rating
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
    static bool ReadStringProperty(IWICMetadataQueryReader* pReader, const std::wstring& path, std::wstring& outVal);
    static std::wstring RemoveQualifier(std::wstring path);
    static bool WriteStringVectorProperty(
        IWICImagingFactory2* wicFactory,
        IWICMetadataQueryWriter* pWriter,
        const std::wstring& pathWithQualifier,
        const std::wstring& val
    );

    static Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> GetFrameDecoder(
        IWICImagingFactory2* wicFactory,
        const std::wstring& filePath,
        Microsoft::WRL::ComPtr<IWICBitmapDecoder>& outDecoder
    );

    static bool CreateStringVectorPropVariant(
        const std::vector<std::wstring>& strings,
        PROPVARIANT& outVar
    );

    static bool InitializeTranscoder(
        IWICImagingFactory2* wicFactory,
        const std::wstring& tempPath,
        GUID containerFormat,
        IWICBitmapFrameDecode* pFrame,
        Microsoft::WRL::ComPtr<IWICBitmapEncoder>& outEncoder,
        Microsoft::WRL::ComPtr<IWICBitmapFrameEncode>& outFrameEncode
    );

    static bool ReplaceFileWithTemp(
        const std::wstring& filePath,
        const std::wstring& tempPath
    );
};

} // namespace kv
