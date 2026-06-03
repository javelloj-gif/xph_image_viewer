// ============================================================================
// Class: JpegSegmentInjector
// Description: JpegSegmentInjector inserts APP1 metadata segments containing XMP packets into JPEG files for [IMAGE_VIEWER_APP_NAME].
// Responsibility: Split JPEG streams at segments and insert APP1/XMP markers containing custom XML tag payloads.
// ============================================================================
#pragma once

#include <string>
#include <vector>

namespace kv {

class JpegSegmentInjector {
public:
    static bool InjectJpegXmpSegment(const std::wstring& filePath, const std::vector<std::wstring>& tags);

private:
    static std::string BuildXmpXmlUtf8(const std::vector<std::wstring>& tags);
};

} // namespace kv
