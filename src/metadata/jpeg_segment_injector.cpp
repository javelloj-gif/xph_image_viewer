// ============================================================================
// Class: JpegSegmentInjector
// Description: JpegSegmentInjector inserts APP1 metadata segments containing XMP packets into JPEG files for Fotassa.
// Responsibility: Split JPEG streams at segments and insert APP1/XMP markers containing custom XML tag payloads.
// ============================================================================
#include "common/pch.h"
#include "jpeg_segment_injector.h"
#include "common/types.h"
#include <fstream>
#include <filesystem>
#include <cstring>
#include <thread>
#include <chrono>

namespace kv {

std::string JpegSegmentInjector::BuildXmpXmlUtf8(
    const std::vector<std::wstring>& tags,
    const std::wstring& title,
    const std::wstring& creator,
    const std::wstring& description,
    int rating
) {
    std::wstring xml =
        L"<?xpacket begin='\uFEFF' id='W5M0MpCehiHzreSzNTczkc9d'?>\n"
        L"<x:xmpmeta xmlns:x='adobe:ns:meta/' x:xmptk='XMP Core 5.1.2'>\n"
        L" <rdf:RDF xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'>\n"
        L"  <rdf:Description rdf:about=''\n"
        L"    xmlns:dc='http://purl.org/dc/elements/1.1/'\n"
        L"    xmlns:xmp='http://ns.adobe.com/xap/1.0/'>\n";

    if (!tags.empty()) {
        xml += L"   <dc:subject>\n"
               L"    <rdf:Bag>\n";
        for (const auto& tag : tags) {
            xml += L"     <rdf:li>" + tag + L"</rdf:li>\n";
        }
        xml += L"    </rdf:Bag>\n"
               L"   </dc:subject>\n";
    }

    if (!title.empty()) {
        xml += L"   <dc:title>\n"
               L"    <rdf:Alt>\n"
               L"     <rdf:li xml:lang='x-default'>" + title + L"</rdf:li>\n"
               L"    </rdf:Alt>\n"
               L"   </dc:title>\n";
    }

    if (!creator.empty()) {
        xml += L"   <dc:creator>\n"
               L"    <rdf:Seq>\n"
               L"     <rdf:li>" + creator + L"</rdf:li>\n"
               L"    </rdf:Seq>\n"
               L"   </dc:creator>\n";
    }

    if (!description.empty()) {
        xml += L"   <dc:description>\n"
               L"    <rdf:Alt>\n"
               L"     <rdf:li xml:lang='x-default'>" + description + L"</rdf:li>\n"
               L"    </rdf:Alt>\n"
               L"   </dc:description>\n";
    }

    if (rating > 0) {
        xml += L"   <xmp:Rating>" + std::to_wstring(rating) + L"</xmp:Rating>\n";
    }

    xml += L"  </rdf:Description>\n"
           L" </rdf:RDF>\n"
           L"</x:xmpmeta>\n"
           L"<?xpacket end='w'?>";

    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, xml.c_str(), static_cast<int>(xml.size()), nullptr, 0, nullptr, nullptr);
    if (sizeNeeded <= 0) return {};
    std::string result(sizeNeeded, '\0');
    WideCharToMultiByte(CP_UTF8, 0, xml.c_str(), static_cast<int>(xml.size()), result.data(), sizeNeeded, nullptr, nullptr);
    return result;
}

bool JpegSegmentInjector::InjectJpegXmpSegment(
    const std::wstring& filePath, 
    const std::vector<std::wstring>& tags,
    const std::wstring& title,
    const std::wstring& creator,
    const std::wstring& description,
    int rating
) {
    std::ifstream inFile(filePath, std::ios::binary);
    if (!inFile) {
        LogError(std::format(L"InjectJpegXmpSegment: Cannot open file for reading: {}", filePath));
        return false;
    }
    std::vector<uint8_t> jpegData(
        (std::istreambuf_iterator<char>(inFile)),
        std::istreambuf_iterator<char>()
    );
    inFile.close();

    if (jpegData.size() < 4 || jpegData[0] != 0xFF || jpegData[1] != 0xD8) {
        LogError(std::format(L"InjectJpegXmpSegment: Invalid JPEG SOI in: {}", filePath));
        return false;
    }

    static const char XMP_NS_HDR[] = "http://ns.adobe.com/xap/1.0/";
    static const size_t XMP_NS_HDR_LEN = sizeof(XMP_NS_HDR);

    std::vector<uint8_t> outputBody;
    size_t pos = 2;

    while (pos < jpegData.size()) {
        if (jpegData[pos] != 0xFF) {
            outputBody.insert(outputBody.end(), jpegData.begin() + pos, jpegData.end());
            pos = jpegData.size();
            break;
        }

        uint8_t marker = (pos + 1 < jpegData.size()) ? jpegData[pos + 1] : 0;
        bool standalone = (marker == 0xD8 || marker == 0xD9 ||
                          (marker >= 0xD0 && marker <= 0xD7));

        if (standalone) {
            outputBody.push_back(jpegData[pos]);
            outputBody.push_back(jpegData[pos + 1]);
            pos += 2;
            if (marker == 0xD9) break;
            continue;
        }

        if (pos + 4 > jpegData.size()) break;
        uint16_t segLen = (static_cast<uint16_t>(jpegData[pos + 2]) << 8) | jpegData[pos + 3];
        size_t segDataStart = pos + 4;
        size_t segDataLen   = (segLen >= 2) ? segLen - 2 : 0;
        size_t totalSegSize = 2 + 2 + segDataLen;

        if (pos + totalSegSize > jpegData.size()) {
            outputBody.insert(outputBody.end(), jpegData.begin() + pos, jpegData.end());
            pos = jpegData.size();
            break;
        }

        bool isXmpApp1 = false;
        if (marker == 0xE1 && segDataLen >= XMP_NS_HDR_LEN) {
            isXmpApp1 = (memcmp(jpegData.data() + segDataStart, XMP_NS_HDR, XMP_NS_HDR_LEN) == 0);
        }

        if (!isXmpApp1) {
            outputBody.insert(outputBody.end(),
                jpegData.begin() + pos,
                jpegData.begin() + pos + totalSegSize);
        }

        pos += totalSegSize;
    }

    bool hasAnyMetadata = !tags.empty() || !title.empty() || !creator.empty() || !description.empty() || rating > 0;
    std::vector<uint8_t> newXmpSeg;
    if (hasAnyMetadata) {
        std::string xmpXml = BuildXmpXmlUtf8(tags, title, creator, description, rating);
        if (xmpXml.empty()) {
            LogError(L"InjectJpegXmpSegment: Failed to build XMP XML UTF-8 string");
            return false;
        }

        std::vector<uint8_t> segData;
        segData.insert(segData.end(),
            reinterpret_cast<const uint8_t*>(XMP_NS_HDR),
            reinterpret_cast<const uint8_t*>(XMP_NS_HDR) + XMP_NS_HDR_LEN);
        segData.insert(segData.end(),
            reinterpret_cast<const uint8_t*>(xmpXml.data()),
            reinterpret_cast<const uint8_t*>(xmpXml.data()) + xmpXml.size());

        uint16_t segLenField = static_cast<uint16_t>(segData.size() + 2);
        newXmpSeg.push_back(0xFF);
        newXmpSeg.push_back(0xE1);
        newXmpSeg.push_back(static_cast<uint8_t>((segLenField >> 8) & 0xFF));
        newXmpSeg.push_back(static_cast<uint8_t>((segLenField >> 0) & 0xFF));
        newXmpSeg.insert(newXmpSeg.end(), segData.begin(), segData.end());

        LogInfo(std::format(L"InjectJpegXmpSegment: Built APP1/XMP segment: {} bytes data, {} bytes total",
            segData.size(), newXmpSeg.size()));
    }

    std::wstring tempPath = filePath + L".tmp";
    std::ofstream outFile(tempPath, std::ios::binary | std::ios::trunc);
    if (!outFile) {
        LogError(std::format(L"InjectJpegXmpSegment: Cannot open temp file for writing: {}", tempPath));
        return false;
    }

    outFile.put(static_cast<char>(0xFF));
    outFile.put(static_cast<char>(0xD8));
    if (!newXmpSeg.empty())
        outFile.write(reinterpret_cast<const char*>(newXmpSeg.data()), newXmpSeg.size());
    if (!outputBody.empty())
        outFile.write(reinterpret_cast<const char*>(outputBody.data()), outputBody.size());
    outFile.close();

    bool replaced = false;
    for (int retry = 0; retry < 10; ++retry) {
        try {
            if (std::filesystem::exists(filePath)) {
                std::filesystem::remove(filePath);
            }
            std::filesystem::rename(tempPath, filePath);
            replaced = true;
            break;
        } catch (...) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    if (!replaced) {
        LogError(std::format(L"InjectJpegXmpSegment: Failed to replace original file after retries: {}", filePath));
        std::filesystem::remove(tempPath);
        return false;
    }

    LogInfo(std::format(L"InjectJpegXmpSegment: Successfully injected APP1/XMP into: {}", filePath));
    return true;
}

} // namespace kv
