// ============================================================================
// Class: JpegSegmentInjector
// Description: JpegSegmentInjector inserts APP1 metadata segments containing XMP packets into JPEG files for Fotassa.
// Responsibility: Split JPEG streams at segments and insert APP1/XMP markers containing custom XML tag payloads.
// ============================================================================
#pragma once

#include <string>
#include <vector>

namespace kv {

class JpegSegmentInjector {
public:
    static bool InjectJpegXmpSegment(
        const std::wstring& filePath, 
        const std::vector<std::wstring>& tags,
        const std::wstring& title = L"",
        const std::wstring& creator = L"",
        const std::wstring& description = L"",
        int rating = 0
    );

private:
    static std::string BuildXmpXmlUtf8(
        const std::vector<std::wstring>& tags,
        const std::wstring& title = L"",
        const std::wstring& creator = L"",
        const std::wstring& description = L"",
        int rating = 0
    );
};

} // namespace kv
