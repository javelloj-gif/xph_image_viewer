// ============================================================================
// Class: WicMetadataReaderWriter
// Description: WicMetadataReaderWriter leverages the WIC API to parse and write XMP Dublin Core tags in standard-compliant files for [IMAGE_VIEWER_APP_NAME].
// Responsibility: Read and write XMP bag elements using Direct WIC query reader interfaces, falling back to full block transcoding if needed.
// ============================================================================
#include "common/pch.h"
#include "wic_metadata_reader_writer.h"
#include "common/types.h"
#include <wincodecsdk.h>
#include <filesystem>
#include <format>
#include <algorithm>

namespace kv {

std::wstring WicMetadataReaderWriter::GetXmpQueryPathPrefix(const GUID& containerFormat) {
    if (containerFormat == GUID_ContainerFormatPng) {
        return L"/iTXt/XML:com.adobe.xmp";
    } else if (containerFormat == GUID_ContainerFormatJpeg) {
        return L"/app1/xmp";
    }
    return L"/xmp";
}

void WicMetadataReaderWriter::EnsureMetadataPath(
    IWICImagingFactory2* wicFactory,
    IWICMetadataQueryWriter* pQueryWriter,
    const std::wstring& path,
    bool forceCreate
) {
    if (!forceCreate) {
        PROPVARIANT varTest;
        PropVariantInit(&varTest);
        if (SUCCEEDED(pQueryWriter->GetMetadataByName(path.c_str(), &varTest))) {
            if (varTest.vt != VT_EMPTY) {
                PropVariantClear(&varTest);
                return;
            }
        }
        PropVariantClear(&varTest);
    }

    std::wstring subBlockPath = L"";
    if (path.find(L"/app1/xmp/") == 0) {
        subBlockPath = L"/app1/xmp";
    } else if (path.find(L"/iTXt/XML:com.adobe.xmp/") == 0) {
        subBlockPath = L"/iTXt/XML:com.adobe.xmp";
    } else if (path.find(L"/xmp/") == 0) {
        subBlockPath = L"/xmp";
    }

    if (!subBlockPath.empty()) {
        if (!forceCreate) {
            PROPVARIANT varParent;
            PropVariantInit(&varParent);
            HRESULT hrParent = pQueryWriter->GetMetadataByName(subBlockPath.c_str(), &varParent);
            LogInfo(std::format(L"WicMetadataReaderWriter: EnsureMetadataPath: GetMetadataByName({}) hr: 0x{:08X}, vt: {}", subBlockPath, static_cast<unsigned>(hrParent), varParent.vt));
            if (SUCCEEDED(hrParent)) {
                if (varParent.vt != VT_EMPTY) {
                    PropVariantClear(&varParent);
                    return;
                }
            }
            PropVariantClear(&varParent);
        }

        ComPtr<IWICMetadataQueryWriter> pXmpQueryWriter;
        if (SUCCEEDED(wicFactory->CreateQueryWriter(GUID_MetadataFormatXMP, nullptr, &pXmpQueryWriter))) {
            PROPVARIANT varBlock;
            PropVariantInit(&varBlock);
            varBlock.vt = VT_UNKNOWN;
            varBlock.punkVal = pXmpQueryWriter.Get();
            pXmpQueryWriter->AddRef();
            
            HRESULT hr = pQueryWriter->SetMetadataByName(subBlockPath.c_str(), &varBlock);
            PropVariantClear(&varBlock);
            
            LogInfo(std::format(L"WicMetadataReaderWriter: EnsureMetadataPath: Created metadata sub-block at {} hr: 0x{:08X}", subBlockPath, static_cast<unsigned>(hr)));
        }
    }
}

bool WicMetadataReaderWriter::ReadTags(
    IWICImagingFactory2* wicFactory,
    const std::wstring& filePath,
    std::vector<std::wstring>& outTags
) {
    outTags.clear();
    if (!wicFactory) return false;

    ComPtr<IWICBitmapDecoder> pDecoder;
    HRESULT hr = wicFactory->CreateDecoderFromFilename(
        filePath.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnDemand,
        &pDecoder
    );

    if (FAILED(hr)) return false;

    ComPtr<IWICBitmapFrameDecode> pFrame;
    hr = pDecoder->GetFrame(0, &pFrame);
    if (FAILED(hr)) return false;

    ComPtr<IWICMetadataQueryReader> pQueryReader;
    hr = pFrame->GetMetadataQueryReader(&pQueryReader);
    if (FAILED(hr)) return true;

    GUID containerFormat = {};
    pDecoder->GetContainerFormat(&containerFormat);

    if (containerFormat == GUID_ContainerFormatPng) {
        PROPVARIANT varRawXml;
        PropVariantInit(&varRawXml);
        HRESULT hrPng = pQueryReader->GetMetadataByName(L"/iTXt/{str=XML:com.adobe.xmp}", &varRawXml);
        if (SUCCEEDED(hrPng)) {
            if (varRawXml.vt == VT_LPWSTR && varRawXml.pwszVal) {
                std::wstring xmlStr = varRawXml.pwszVal;
                size_t pos = 0;
                while (true) {
                    pos = xmlStr.find(L"<rdf:li>", pos);
                    if (pos == std::wstring::npos) break;
                    pos += 8;
                    size_t endPos = xmlStr.find(L"</rdf:li>", pos);
                    if (endPos == std::wstring::npos) break;
                    std::wstring tag = xmlStr.substr(pos, endPos - pos);
                    outTags.push_back(tag);
                    pos = endPos + 9;
                }
                PropVariantClear(&varRawXml);
                return true;
            }
            PropVariantClear(&varRawXml);
        }
    }

    std::wstring xmpPrefix = GetXmpQueryPathPrefix(containerFormat);

    PROPVARIANT var;
    PropVariantInit(&var);
    std::wstring directQueryPath = xmpPrefix + L"/dc:subject";
    hr = pQueryReader->GetMetadataByName(directQueryPath.c_str(), &var);
    LogInfo(std::format(L"WicMetadataReaderWriter: ReadTags GetMetadataByName({}) hr: 0x{:08X}, vt: {}", directQueryPath, static_cast<unsigned>(hr), var.vt));

    if (FAILED(hr)) {
        PROPVARIANT varXmp;
        PropVariantInit(&varXmp);
        HRESULT hrXmp = pQueryReader->GetMetadataByName(xmpPrefix.c_str(), &varXmp);
        LogInfo(std::format(L"WicMetadataReaderWriter: ReadTags GetMetadataByName({}) hr: 0x{:08X}, vt: {}", xmpPrefix, static_cast<unsigned>(hrXmp), varXmp.vt));
        if (SUCCEEDED(hrXmp) && varXmp.vt == VT_UNKNOWN && varXmp.punkVal) {
            ComPtr<IWICMetadataQueryReader> pXmpReader;
            if (SUCCEEDED(varXmp.punkVal->QueryInterface(IID_PPV_ARGS(&pXmpReader)))) {
                hr = pXmpReader->GetMetadataByName(L"/dc:subject", &var);
                LogInfo(std::format(L"WicMetadataReaderWriter: ReadTags pXmpReader->GetMetadataByName(/dc:subject) hr: 0x{:08X}, vt: {}", static_cast<unsigned>(hr), var.vt));
            }
        }
        PropVariantClear(&varXmp);
    }

    if (FAILED(hr) && xmpPrefix != L"/xmp") {
        hr = pQueryReader->GetMetadataByName(L"/xmp/dc:subject", &var);
        LogInfo(std::format(L"WicMetadataReaderWriter: ReadTags GetMetadataByName(/xmp/dc:subject) fallback hr: 0x{:08X}, vt: {}", static_cast<unsigned>(hr), var.vt));
    }

    if (FAILED(hr) && xmpPrefix != L"/xmp") {
        PROPVARIANT varXmp;
        PropVariantInit(&varXmp);
        HRESULT hrXmp = pQueryReader->GetMetadataByName(L"/xmp", &varXmp);
        LogInfo(std::format(L"WicMetadataReaderWriter: ReadTags GetMetadataByName(/xmp) fallback hr: 0x{:08X}, vt: {}", static_cast<unsigned>(hrXmp), varXmp.vt));
        if (SUCCEEDED(hrXmp) && varXmp.vt == VT_UNKNOWN && varXmp.punkVal) {
            ComPtr<IWICMetadataQueryReader> pXmpReader;
            if (SUCCEEDED(varXmp.punkVal->QueryInterface(IID_PPV_ARGS(&pXmpReader)))) {
                hr = pXmpReader->GetMetadataByName(L"/dc:subject", &var);
                LogInfo(std::format(L"WicMetadataReaderWriter: ReadTags fallback pXmpReader->GetMetadataByName(/dc:subject) hr: 0x{:08X}, vt: {}", static_cast<unsigned>(hr), var.vt));
            }
        }
        PropVariantClear(&varXmp);
    }

    if (FAILED(hr)) {
        PropVariantClear(&var);
        return true;
    }

    if (var.vt == (VT_VECTOR | VT_LPWSTR)) {
        for (ULONG i = 0; i < var.calpwstr.cElems; ++i) {
            if (var.calpwstr.pElems[i]) {
                outTags.push_back(var.calpwstr.pElems[i]);
            }
        }
    } else if (var.vt == VT_UNKNOWN) {
        ComPtr<IWICMetadataQueryReader> pBagReader;
        if (SUCCEEDED(var.punkVal->QueryInterface(IID_PPV_ARGS(&pBagReader)))) {
            ComPtr<IEnumString> pEnumString;
            if (SUCCEEDED(pBagReader->GetEnumerator(&pEnumString))) {
                ULONG fetched = 0;
                LPOLESTR pwszName = nullptr;
                while (pEnumString->Next(1, &pwszName, &fetched) == S_OK) {
                    PROPVARIANT itemVar;
                    PropVariantInit(&itemVar);
                    if (SUCCEEDED(pBagReader->GetMetadataByName(pwszName, &itemVar))) {
                        if (itemVar.vt == VT_LPWSTR && itemVar.pwszVal) {
                            outTags.push_back(itemVar.pwszVal);
                        } else if (itemVar.vt == VT_BSTR && itemVar.bstrVal) {
                            outTags.push_back(itemVar.bstrVal);
                        }
                        PropVariantClear(&itemVar);
                    }
                    CoTaskMemFree(pwszName);
                }
            }
        }
    } else if (var.vt == VT_LPWSTR && var.pwszVal) {
        outTags.push_back(var.pwszVal);
    } else if (var.vt == VT_BSTR && var.bstrVal) {
        outTags.push_back(var.bstrVal);
    }

    PropVariantClear(&var);
    return true;
}

bool WicMetadataReaderWriter::WritePngXmp(IWICMetadataQueryWriter* pQueryWriter, const std::vector<std::wstring>& tags) {
    int targetIndex = -1;
    int nextFreeIndex = 0;
    for (int i = 0; ; ++i) {
        std::wstring queryPath = std::format(L"/[{}]iTXt/Keyword", i);
        PROPVARIANT varKeyword;
        PropVariantInit(&varKeyword);
        HRESULT hrQuery = pQueryWriter->GetMetadataByName(queryPath.c_str(), &varKeyword);
        if (FAILED(hrQuery)) {
            nextFreeIndex = i;
            break;
        }
        if (varKeyword.vt == VT_LPSTR && varKeyword.pszVal) {
            if (strcmp(varKeyword.pszVal, "XML:com.adobe.xmp") == 0) {
                targetIndex = i;
                PropVariantClear(&varKeyword);
                break;
            }
        }
        PropVariantClear(&varKeyword);
    }

    if (tags.empty()) {
        if (targetIndex >= 0) {
            std::wstring removePath = std::format(L"/[{}]iTXt", targetIndex);
            pQueryWriter->RemoveMetadataByName(removePath.c_str());
        }
        return true;
    }

    std::wstring xml = L"<x:xmpmeta xmlns:x=\"adobe:ns:meta/\" x:xmptk=\"XMP Core 5.1.2\">\n"
                       L" <rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\n"
                       L"  <rdf:Description rdf:about=\"\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\">\n"
                       L"   <dc:subject>\n"
                       L"    <rdf:Bag>\n";
    for (const auto& tag : tags) {
        xml += L"     <rdf:li>" + tag + L"</rdf:li>\n";
    }
    xml += L"    </rdf:Bag>\n"
           L"   </dc:subject>\n"
           L"  </rdf:Description>\n"
           L" </rdf:RDF>\n"
           L"</x:xmpmeta>";

    int writeIndex = (targetIndex >= 0) ? targetIndex : nextFreeIndex;

    PROPVARIANT pvKeyword;
    PropVariantInit(&pvKeyword);
    pvKeyword.vt = VT_LPSTR;
    char keyword[] = "XML:com.adobe.xmp";
    pvKeyword.pszVal = keyword;
    std::wstring keywordPath = std::format(L"/[{}]iTXt/Keyword", writeIndex);
    HRESULT hr = pQueryWriter->SetMetadataByName(keywordPath.c_str(), &pvKeyword);
    if (FAILED(hr)) return false;

    PROPVARIANT pvText;
    PropVariantInit(&pvText);
    pvText.vt = VT_LPWSTR;
    pvText.pwszVal = const_cast<LPWSTR>(xml.c_str());
    std::wstring textPath = std::format(L"/[{}]iTXt/TextEntry", writeIndex);
    hr = pQueryWriter->SetMetadataByName(textPath.c_str(), &pvText);
    if (FAILED(hr)) return false;

    PROPVARIANT pvComp;
    PropVariantInit(&pvComp);
    pvComp.vt = VT_UI1;
    pvComp.bVal = 0;
    std::wstring compPath = std::format(L"/[{}]iTXt/CompressionFlag", writeIndex);
    pQueryWriter->SetMetadataByName(compPath.c_str(), &pvComp);

    PROPVARIANT pvLang;
    PropVariantInit(&pvLang);
    pvLang.vt = VT_LPSTR;
    char lang[] = "";
    pvLang.pszVal = lang;
    std::wstring langPath = std::format(L"/[{}]iTXt/LanguageTag", writeIndex);
    pQueryWriter->SetMetadataByName(langPath.c_str(), &pvLang);

    PROPVARIANT pvTrans;
    PropVariantInit(&pvTrans);
    pvTrans.vt = VT_LPWSTR;
    wchar_t trans[] = L"";
    pvTrans.pwszVal = trans;
    std::wstring transPath = std::format(L"/[{}]iTXt/TranslatedKeyword", writeIndex);
    pQueryWriter->SetMetadataByName(transPath.c_str(), &pvTrans);

    return true;
}

bool WicMetadataReaderWriter::WriteTagsFast(
    IWICImagingFactory2* wicFactory,
    const std::wstring& filePath,
    GUID containerFormat,
    const std::vector<std::wstring>& tags
) {
    bool fastEncodingSucceeded = false;
    ComPtr<IWICBitmapDecoder> pDecoder;
    HRESULT hr = wicFactory->CreateDecoderFromFilename(
        filePath.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnDemand,
        &pDecoder
    );

    if (SUCCEEDED(hr)) {
        ComPtr<IWICBitmapFrameDecode> pFrame;
        hr = pDecoder->GetFrame(0, &pFrame);
        if (SUCCEEDED(hr)) {
            ComPtr<IWICFastMetadataEncoder> pFastEncoder;
            hr = wicFactory->CreateFastMetadataEncoderFromFrameDecode(pFrame.Get(), &pFastEncoder);
            if (SUCCEEDED(hr)) {
                ComPtr<IWICMetadataQueryWriter> pFastQueryWriter;
                hr = pFastEncoder->GetMetadataQueryWriter(&pFastQueryWriter);
                if (SUCCEEDED(hr)) {
                    bool fastOk = false;
                    std::wstring xmpPrefix = GetXmpQueryPathPrefix(containerFormat);
                    std::wstring subjectPath = xmpPrefix + L"/<xmpbag>dc:subject";
                    EnsureMetadataPath(wicFactory, pFastQueryWriter.Get(), subjectPath, false);

                    PROPVARIANT var;
                    PropVariantInit(&var);
                    var.vt = VT_VECTOR | VT_LPWSTR;
                    var.calpwstr.cElems = static_cast<ULONG>(tags.size());
                    var.calpwstr.pElems = nullptr;

                    bool prepOk = true;
                    if (var.calpwstr.cElems > 0) {
                        var.calpwstr.pElems = static_cast<LPOLESTR*>(CoTaskMemAlloc(sizeof(LPOLESTR) * var.calpwstr.cElems));
                        if (!var.calpwstr.pElems) prepOk = false;
                        else {
                            for (size_t i = 0; i < tags.size(); ++i) {
                                size_t len = tags[i].length() + 1;
                                var.calpwstr.pElems[i] = static_cast<LPOLESTR>(CoTaskMemAlloc(sizeof(wchar_t) * len));
                                if (var.calpwstr.pElems[i]) {
                                    wcscpy_s(var.calpwstr.pElems[i], len, tags[i].c_str());
                                } else {
                                    for (size_t j = 0; j < i; ++j) {
                                        CoTaskMemFree(var.calpwstr.pElems[j]);
                                    }
                                    CoTaskMemFree(var.calpwstr.pElems);
                                    prepOk = false;
                                    break;
                                }
                            }
                        }
                    }

                    if (prepOk) {
                        hr = pFastQueryWriter->SetMetadataByName(subjectPath.c_str(), &var);
                        fastOk = SUCCEEDED(hr);
                    }
                    PropVariantClear(&var);

                    if (fastOk) {
                        hr = pFastEncoder->Commit();
                        if (SUCCEEDED(hr)) {
                            fastEncodingSucceeded = true;
                        }
                    }
                }
            }
        }
    }
    return fastEncodingSucceeded;
}

bool WicMetadataReaderWriter::WriteTagsTranscode(
    IWICImagingFactory2* wicFactory,
    const std::wstring& filePath,
    GUID containerFormat,
    const std::vector<std::wstring>& tags
) {
    std::wstring tempPath = filePath + L".tmp";
    bool transcodingSucceeded = false;

    {
        ComPtr<IWICBitmapDecoder> pDecoder;
        HRESULT hr = wicFactory->CreateDecoderFromFilename(
            filePath.c_str(),
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnDemand,
            &pDecoder
        );

        if (SUCCEEDED(hr)) {
            ComPtr<IWICBitmapFrameDecode> pFrame;
            hr = pDecoder->GetFrame(0, &pFrame);
            if (SUCCEEDED(hr)) {
                ComPtr<IWICStream> pStream;
                hr = wicFactory->CreateStream(&pStream);
                if (SUCCEEDED(hr)) {
                    hr = pStream->InitializeFromFilename(tempPath.c_str(), GENERIC_WRITE);
                    if (SUCCEEDED(hr)) {
                        ComPtr<IWICBitmapEncoder> pEncoder;
                        hr = wicFactory->CreateEncoder(containerFormat, nullptr, &pEncoder);
                        if (SUCCEEDED(hr)) {
                            hr = pEncoder->Initialize(pStream.Get(), WICBitmapEncoderNoCache);
                            if (SUCCEEDED(hr)) {
                                ComPtr<IWICBitmapFrameEncode> pFrameEncode;
                                ComPtr<IPropertyBag2> pPropertyBag;
                                hr = pEncoder->CreateNewFrame(&pFrameEncode, &pPropertyBag);
                                if (SUCCEEDED(hr)) {
                                    if (containerFormat == GUID_ContainerFormatJpeg && pPropertyBag) {
                                        PROPBAG2 propBag = { 0 };
                                        propBag.dwType = PROPBAG2_TYPE_DATA;
                                        propBag.vt = VT_R4;
                                        propBag.pstrName = const_cast<LPOLESTR>(L"ImageQuality");
                                        VARIANT varVal;
                                        VariantInit(&varVal);
                                        varVal.vt = VT_R4;
                                        varVal.fltVal = 0.95f;
                                        pPropertyBag->Write(1, &propBag, &varVal);
                                    }

                                    hr = pFrameEncode->Initialize(pPropertyBag.Get());
                                    if (SUCCEEDED(hr)) {
                                        uint32_t width = 0, height = 0;
                                        pFrame->GetSize(&width, &height);
                                        pFrameEncode->SetSize(width, height);

                                        double dpiX = 0, dpiY = 0;
                                        pFrame->GetResolution(&dpiX, &dpiY);
                                        pFrameEncode->SetResolution(dpiX, dpiY);

                                        WICPixelFormatGUID pixelFormat = {};
                                        pFrame->GetPixelFormat(&pixelFormat);
                                        pFrameEncode->SetPixelFormat(&pixelFormat);

                                        hr = pFrameEncode->WriteSource(pFrame.Get(), nullptr);
                                        if (SUCCEEDED(hr)) {
                                            ComPtr<IWICMetadataBlockReader> pBlockReader;
                                            ComPtr<IWICMetadataBlockWriter> pBlockWriter;
                                            if (SUCCEEDED(pFrame->QueryInterface(IID_PPV_ARGS(&pBlockReader))) &&
                                                SUCCEEDED(pFrameEncode->QueryInterface(IID_PPV_ARGS(&pBlockWriter)))) {
                                                pBlockWriter->InitializeFromBlockReader(pBlockReader.Get());

                                                if (containerFormat == GUID_ContainerFormatJpeg) {
                                                    bool hasXmp = false;
                                                    UINT count = 0;
                                                    pBlockWriter->GetCount(&count);
                                                    for (UINT i = 0; i < count; ++i) {
                                                        ComPtr<IWICMetadataWriter> pWriter;
                                                        if (SUCCEEDED(pBlockWriter->GetWriterByIndex(i, &pWriter))) {
                                                            GUID format = {};
                                                            pWriter->GetMetadataFormat(&format);
                                                            if (format == GUID_MetadataFormatXMP) {
                                                                hasXmp = true;
                                                                break;
                                                            }
                                                        }
                                                    }
                                                    if (!hasXmp) {
                                                        ComPtr<IWICMetadataWriter> pXmpWriter;
                                                        ComPtr<IWICComponentFactory> pComponentFactory;
                                                        HRESULT hrCreate = E_FAIL;
                                                        if (SUCCEEDED(wicFactory->QueryInterface(IID_PPV_ARGS(&pComponentFactory)))) {
                                                            hrCreate = pComponentFactory->CreateMetadataWriter(GUID_MetadataFormatXMP, nullptr, 0, &pXmpWriter);
                                                        }
                                                        if (SUCCEEDED(hrCreate)) {
                                                            pBlockWriter->AddWriter(pXmpWriter.Get());
                                                        }
                                                    }

                                                    if (!tags.empty()) {
                                                        ComPtr<IWICMetadataWriter> pXmpBlockWriter;
                                                        UINT xmpCount = 0;
                                                        pBlockWriter->GetCount(&xmpCount);
                                                        for (UINT i = 0; i < xmpCount; ++i) {
                                                            ComPtr<IWICMetadataWriter> pWriter;
                                                            if (SUCCEEDED(pBlockWriter->GetWriterByIndex(i, &pWriter))) {
                                                                GUID format = {};
                                                                pWriter->GetMetadataFormat(&format);
                                                                if (format == GUID_MetadataFormatXMP) {
                                                                    pXmpBlockWriter = pWriter;
                                                                    break;
                                                                }
                                                            }
                                                        }

                                                        if (pXmpBlockWriter) {
                                                            PROPVARIANT varSchema;
                                                            PropVariantInit(&varSchema);
                                                            varSchema.vt = VT_LPWSTR;
                                                            varSchema.pwszVal = const_cast<LPOLESTR>(L"http://purl.org/dc/elements/1.1/");

                                                            PROPVARIANT varId;
                                                            PropVariantInit(&varId);
                                                            varId.vt = VT_LPWSTR;
                                                            varId.pwszVal = const_cast<LPOLESTR>(L"subject");

                                                            PROPVARIANT val;
                                                            PropVariantInit(&val);
                                                            val.vt = VT_VECTOR | VT_LPWSTR;
                                                            val.calpwstr.cElems = static_cast<ULONG>(tags.size());
                                                            val.calpwstr.pElems = static_cast<LPOLESTR*>(
                                                                CoTaskMemAlloc(sizeof(LPOLESTR) * tags.size()));

                                                            bool xmpPrepOk = (val.calpwstr.pElems != nullptr);
                                                            if (xmpPrepOk) {
                                                                for (size_t i = 0; i < tags.size(); ++i) {
                                                                    size_t len = tags[i].length() + 1;
                                                                    val.calpwstr.pElems[i] = static_cast<LPOLESTR>(
                                                                        CoTaskMemAlloc(sizeof(wchar_t) * len));
                                                                    if (val.calpwstr.pElems[i]) {
                                                                        wcscpy_s(val.calpwstr.pElems[i], len, tags[i].c_str());
                                                                    } else {
                                                                        for (size_t j = 0; j < i; ++j)
                                                                            CoTaskMemFree(val.calpwstr.pElems[j]);
                                                                        CoTaskMemFree(val.calpwstr.pElems);
                                                                        xmpPrepOk = false;
                                                                        break;
                                                                    }
                                                                }
                                                            }

                                                            if (xmpPrepOk) {
                                                                pXmpBlockWriter->SetValue(&varSchema, &varId, &val);
                                                            }
                                                            PropVariantClear(&val);
                                                        }
                                                    }
                                                }
                                            }

                                            ComPtr<IWICMetadataQueryWriter> pQueryWriter;
                                            hr = pFrameEncode->GetMetadataQueryWriter(&pQueryWriter);
                                            if (SUCCEEDED(hr)) {
                                                bool allOk = true;
                                                if (containerFormat == GUID_ContainerFormatPng) {
                                                    allOk = WritePngXmp(pQueryWriter.Get(), tags);
                                                } else {
                                                    if (tags.empty()) {
                                                        std::wstring xmpPrefix = GetXmpQueryPathPrefix(containerFormat);
                                                        std::wstring removePath = xmpPrefix + L"/dc:subject";
                                                        pQueryWriter->RemoveMetadataByName(removePath.c_str());
                                                    }
                                                }
                                                
                                                if (allOk) {
                                                    hr = pFrameEncode->Commit();
                                                    if (SUCCEEDED(hr)) {
                                                        hr = pEncoder->Commit();
                                                        if (SUCCEEDED(hr)) {
                                                            transcodingSucceeded = true;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (!transcodingSucceeded) {
        std::filesystem::remove(tempPath);
        return false;
    }

    try {
        std::filesystem::remove(filePath);
        std::filesystem::rename(tempPath, filePath);
        return true;
    } catch (...) {
        std::filesystem::remove(tempPath);
        return false;
    }
}

} // namespace kv
