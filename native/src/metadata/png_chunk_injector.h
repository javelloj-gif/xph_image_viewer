// ============================================================================
// Class: PngChunkInjector
// Description: PngChunkInjector injects raw XMP iTXt metadata chunks directly into PNG streams for [IMAGE_VIEWER_APP_NAME].
// Responsibility: Direct binary manipulation of PNG files to write/replace XMP XML payload packets and compute CRC32 checksums.
// ============================================================================
#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace kv {

class PngChunkInjector {
public:
    static bool InjectPngITxtChunk(
        const std::wstring& filePath, 
        const std::vector<std::wstring>& tags,
        const std::wstring& title = L"",
        const std::wstring& creator = L"",
        const std::wstring& description = L"",
        int rating = 0
    );

private:
    static void InitCrc32Table();
    static uint32_t ComputeCrc32(const uint8_t* data, size_t length);
    static std::string BuildXmpXmlUtf8(
        const std::vector<std::wstring>& tags,
        const std::wstring& title = L"",
        const std::wstring& creator = L"",
        const std::wstring& description = L"",
        int rating = 0
    );
    static void WriteBE32(std::vector<uint8_t>& buf, size_t pos, uint32_t value);
    static uint32_t ReadBE32(const uint8_t* data);

    static uint32_t s_crc32Table[256];
    static bool s_crc32TableInitialized;
};

} // namespace kv
