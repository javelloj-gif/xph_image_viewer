// ============================================================================
// Class: PngChunkInjector
// Description: PngChunkInjector injects raw XMP iTXt metadata chunks directly into PNG streams for [IMAGE_VIEWER_APP_NAME].
// Responsibility: Direct binary manipulation of PNG files to write/replace XMP XML payload packets and compute CRC32 checksums.
// ============================================================================
#include "common/pch.h"
#include "png_chunk_injector.h"
#include "common/types.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstring>

namespace kv {

uint32_t PngChunkInjector::s_crc32Table[256];
bool PngChunkInjector::s_crc32TableInitialized = false;

void PngChunkInjector::InitCrc32Table() {
    if (s_crc32TableInitialized) return;
    for (uint32_t n = 0; n < 256; ++n) {
        uint32_t c = n;
        for (int k = 0; k < 8; ++k) {
            if (c & 1) {
                c = 0xEDB88320UL ^ (c >> 1);
            } else {
                c >>= 1;
            }
        }
        s_crc32Table[n] = c;
    }
    s_crc32TableInitialized = true;
}

uint32_t PngChunkInjector::ComputeCrc32(const uint8_t* data, size_t length) {
    InitCrc32Table();
    uint32_t crc = 0xFFFFFFFFUL;
    for (size_t i = 0; i < length; ++i) {
        crc = s_crc32Table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFUL;
}

std::string PngChunkInjector::BuildXmpXmlUtf8(const std::vector<std::wstring>& tags) {
    std::wstring xml =
        L"<?xpacket begin='\uFEFF' id='W5M0MpCehiHzreSzNTczkc9d'?>\n"
        L"<x:xmpmeta xmlns:x='adobe:ns:meta/' x:xmptk='XMP Core 5.1.2'>\n"
        L" <rdf:RDF xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'>\n"
        L"  <rdf:Description rdf:about='' xmlns:dc='http://purl.org/dc/elements/1.1/'>\n"
        L"   <dc:subject>\n"
        L"    <rdf:Bag>\n";
    for (const auto& tag : tags) {
        xml += L"     <rdf:li>" + tag + L"</rdf:li>\n";
    }
    xml +=
        L"    </rdf:Bag>\n"
        L"   </dc:subject>\n"
        L"  </rdf:Description>\n"
        L" </rdf:RDF>\n"
        L"</x:xmpmeta>\n"
        L"<?xpacket end='w'?>";

    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, xml.c_str(), static_cast<int>(xml.size()), nullptr, 0, nullptr, nullptr);
    if (sizeNeeded <= 0) return {};
    std::string result(sizeNeeded, '\0');
    WideCharToMultiByte(CP_UTF8, 0, xml.c_str(), static_cast<int>(xml.size()), result.data(), sizeNeeded, nullptr, nullptr);
    return result;
}

void PngChunkInjector::WriteBE32(std::vector<uint8_t>& buf, size_t pos, uint32_t value) {
    buf[pos + 0] = static_cast<uint8_t>((value >> 24) & 0xFF);
    buf[pos + 1] = static_cast<uint8_t>((value >> 16) & 0xFF);
    buf[pos + 2] = static_cast<uint8_t>((value >>  8) & 0xFF);
    buf[pos + 3] = static_cast<uint8_t>((value >>  0) & 0xFF);
}

uint32_t PngChunkInjector::ReadBE32(const uint8_t* data) {
    return (static_cast<uint32_t>(data[0]) << 24)
         | (static_cast<uint32_t>(data[1]) << 16)
         | (static_cast<uint32_t>(data[2]) <<  8)
         | (static_cast<uint32_t>(data[3]) <<  0);
}

bool PngChunkInjector::InjectPngITxtChunk(const std::wstring& filePath, const std::vector<std::wstring>& tags) {
    std::ifstream inFile(filePath, std::ios::binary);
    if (!inFile) {
        LogError(std::format(L"InjectPngITxtChunk: Cannot open file for reading: {}", filePath));
        return false;
    }
    std::vector<uint8_t> pngData(
        (std::istreambuf_iterator<char>(inFile)),
        std::istreambuf_iterator<char>()
    );
    inFile.close();

    static const uint8_t PNG_SIG[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    if (pngData.size() < 8 || memcmp(pngData.data(), PNG_SIG, 8) != 0) {
        LogError(std::format(L"InjectPngITxtChunk: Invalid PNG signature in file: {}", filePath));
        return false;
    }

    std::vector<uint8_t> outputChunks;
    std::vector<uint8_t> afterIDAT;
    bool idatFound = false;
    size_t pos = 8;

    while (pos + 12 <= pngData.size()) {
        uint32_t chunkLength = ReadBE32(pngData.data() + pos);
        if (pos + 12 + chunkLength > pngData.size()) {
            LogError(std::format(L"InjectPngITxtChunk: Truncated chunk at offset {} in: {}", pos, filePath));
            return false;
        }

        const uint8_t* chunkType = pngData.data() + pos + 4;
        const uint8_t* chunkData = pngData.data() + pos + 8;
        size_t totalChunkSize = 12 + chunkLength;

        bool isIDAT = (chunkType[0] == 'I' && chunkType[1] == 'D' && chunkType[2] == 'A' && chunkType[3] == 'T');
        bool isIEND = (chunkType[0] == 'I' && chunkType[1] == 'E' && chunkType[2] == 'N' && chunkType[3] == 'D');
        bool isiTXt = (chunkType[0] == 'i' && chunkType[1] == 'T' && chunkType[2] == 'X' && chunkType[3] == 't');

        bool isXmpITXt = false;
        if (isiTXt && chunkLength >= 19) {
            static const char XMP_KEYWORD[] = "XML:com.adobe.xmp";
            isXmpITXt = (memcmp(chunkData, XMP_KEYWORD, 18) == 0 && chunkData[18] == 0);
        }

        if (!idatFound && !isIDAT && !isIEND) {
            if (!isXmpITXt) {
                outputChunks.insert(outputChunks.end(),
                    pngData.data() + pos,
                    pngData.data() + pos + totalChunkSize);
            }
        } else {
            if (isIDAT && !idatFound) {
                idatFound = true;
            }
            afterIDAT.insert(afterIDAT.end(),
                pngData.data() + pos,
                pngData.data() + pos + totalChunkSize);
        }

        pos += totalChunkSize;
    }

    if (!idatFound) {
        LogError(std::format(L"InjectPngITxtChunk: No IDAT chunk found in PNG: {}", filePath));
        return false;
    }

    std::vector<uint8_t> newITXtChunk;
    if (!tags.empty()) {
        std::string xmpXml = BuildXmpXmlUtf8(tags);
        if (xmpXml.empty()) {
            LogError(L"InjectPngITxtChunk: Failed to build XMP XML UTF-8 string");
            return false;
        }

        std::vector<uint8_t> chunkData;
        const char* keyword = "XML:com.adobe.xmp";
        size_t kwLen = strlen(keyword);
        chunkData.insert(chunkData.end(), reinterpret_cast<const uint8_t*>(keyword), reinterpret_cast<const uint8_t*>(keyword) + kwLen);
        chunkData.push_back(0x00);
        chunkData.push_back(0x00);
        chunkData.push_back(0x00);
        chunkData.push_back(0x00);
        chunkData.push_back(0x00);
        chunkData.insert(chunkData.end(),
            reinterpret_cast<const uint8_t*>(xmpXml.data()),
            reinterpret_cast<const uint8_t*>(xmpXml.data()) + xmpXml.size());

        static const uint8_t TYPE_iTXt[4] = { 'i', 'T', 'X', 't' };
        std::vector<uint8_t> crcInput;
        crcInput.insert(crcInput.end(), TYPE_iTXt, TYPE_iTXt + 4);
        crcInput.insert(crcInput.end(), chunkData.begin(), chunkData.end());
        uint32_t crc = ComputeCrc32(crcInput.data(), crcInput.size());

        uint32_t dataLen = static_cast<uint32_t>(chunkData.size());
        newITXtChunk.resize(4);
        WriteBE32(newITXtChunk, 0, dataLen);
        newITXtChunk.insert(newITXtChunk.end(), TYPE_iTXt, TYPE_iTXt + 4);
        newITXtChunk.insert(newITXtChunk.end(), chunkData.begin(), chunkData.end());
        newITXtChunk.resize(newITXtChunk.size() + 4);
        WriteBE32(newITXtChunk, newITXtChunk.size() - 4, crc);

        LogInfo(std::format(L"InjectPngITxtChunk: Built iTXt chunk: {} bytes data, {} bytes total",
            dataLen, newITXtChunk.size()));
    }

    std::wstring tempPath = filePath + L".tmp";
    std::ofstream outFile(tempPath, std::ios::binary | std::ios::trunc);
    if (!outFile) {
        LogError(std::format(L"InjectPngITxtChunk: Cannot open temp file for writing: {}", tempPath));
        return false;
    }

    outFile.write(reinterpret_cast<const char*>(PNG_SIG), 8);
    if (!outputChunks.empty())
        outFile.write(reinterpret_cast<const char*>(outputChunks.data()), outputChunks.size());
    if (!newITXtChunk.empty())
        outFile.write(reinterpret_cast<const char*>(newITXtChunk.data()), newITXtChunk.size());
    if (!afterIDAT.empty())
        outFile.write(reinterpret_cast<const char*>(afterIDAT.data()), afterIDAT.size());
    outFile.close();

    try {
        std::filesystem::remove(filePath);
        std::filesystem::rename(tempPath, filePath);
    } catch (const std::exception& e) {
        std::string whatStr = e.what();
        std::wstring whatWStr(whatStr.begin(), whatStr.end());
        LogError(std::format(L"InjectPngITxtChunk: Failed to replace original file: {}", whatWStr));
        std::filesystem::remove(tempPath);
        return false;
    }

    LogInfo(std::format(L"InjectPngITxtChunk: Successfully injected iTXt XMP chunk into: {}", filePath));
    return true;
}

} // namespace kv
