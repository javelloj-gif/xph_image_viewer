// ============================================================================
// Class: TaggingModal
// Description: TaggingModal displays a transversal overlay window to manage XMP keywords,
//              star ratings, and general metadata parameters (Title, Creator, Description).
// Responsibility: Render search inputs, lists of tags, star rating widgets, and text fields
//                 with cyberpunk aesthetics.
// ============================================================================
#include "common/pch.h"
#include "tagging_modal.h"
#include "core/app.h"
#include <shlwapi.h>
#include "i18n/i18n_engine.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace kv {

// Helper function to query file statistics
static void GetFileStats(const std::wstring& filePath, std::wstring& outDimensions, std::wstring& outSize, std::wstring& outModified, IWICImagingFactory2* wicFactory) {
    outDimensions = L"-";
    outSize = L"-";
    outModified = L"-";
    if (filePath.empty()) return;

    try {
        if (!std::filesystem::exists(filePath)) return;

        // Size
        auto bytes = std::filesystem::file_size(filePath);
        if (bytes > 1024 * 1024) {
            outSize = std::format(L"{:.2f} MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
        } else {
            outSize = std::format(L"{:.1f} KB", static_cast<double>(bytes) / 1024.0);
        }

        // Modified date
        auto ftime = std::filesystem::last_write_time(filePath);
        auto sct = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
        std::time_t tt = std::chrono::system_clock::to_time_t(sct);
        std::tm tm;
        if (localtime_s(&tm, &tt) == 0) {
            wchar_t buf[64];
            if (wcsftime(buf, 64, L"%Y-%m-%d %H:%M:%S", &tm) > 0) {
                outModified = buf;
            }
        }

        // Dimensions using WIC
        if (wicFactory) {
            ComPtr<IWICBitmapDecoder> pDecoder;
            HRESULT hr = E_FAIL;
            ComPtr<IStream> pStream;
            if (SUCCEEDED(SHCreateStreamOnFileEx(filePath.c_str(), STGM_READ | STGM_SHARE_DENY_NONE, FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, &pStream))) {
                hr = wicFactory->CreateDecoderFromStream(
                    pStream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, &pDecoder
                );
            }
            if (FAILED(hr)) {
                hr = wicFactory->CreateDecoderFromFilename(
                    filePath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &pDecoder
                );
            }
            if (SUCCEEDED(hr)) {
                ComPtr<IWICBitmapFrameDecode> pFrame;
                if (SUCCEEDED(pDecoder->GetFrame(0, &pFrame))) {
                    uint32_t w = 0, h = 0;
                    pFrame->GetSize(&w, &h);
                    outDimensions = std::format(L"{} x {}", w, h);
                }
            }
        }
    } catch (...) {}
}

void TaggingModal::Initialize(App* app) {
    m_app = app;
}

void TaggingModal::Show(const std::wstring& filePath) {
    m_filePath = filePath;
    m_visible = true;
    m_currentTab = 0;
    m_leftScroll = 0.0f;
    m_rightScroll = 0.0f;
    m_editingTagIdx = -1;
    m_activeInputType = 0;
    m_searchText.clear();
    m_manualTagText.clear();
    m_editingTagText.clear();
    m_multiImageMode = false;
    m_filePaths.clear();
    m_originalFileTags.clear();
    m_initialCommonTags.clear();

    // Read tags and extended metadata from the file
    if (m_app) {
        m_app->m_tagManager.ReadTags(filePath, m_imageTags);
        m_app->m_tagManager.ReadMetadataExtended(
            filePath, m_title, m_creator, m_description, m_rating, m_deviceMake, m_deviceModel
        );
    }

    // Load available tags from folder's xmpTags.json
    LoadAvailableTags();

    if (m_app) {
        m_app->m_window.RequestRedraw();
    }
}

void TaggingModal::Show(const std::vector<std::wstring>& filePaths) {
    if (filePaths.empty()) return;
    if (filePaths.size() == 1) {
        Show(filePaths[0]);
        return;
    }

    m_filePaths = filePaths;
    m_filePath = filePaths[0]; // For relative path to xmpTags.json
    m_visible = true;
    m_currentTab = 0;
    m_leftScroll = 0.0f;
    m_rightScroll = 0.0f;
    m_editingTagIdx = -1;
    m_activeInputType = 0;
    m_searchText.clear();
    m_manualTagText.clear();
    m_editingTagText.clear();
    m_multiImageMode = true;

    m_originalFileTags.clear();
    m_originalFileTags.resize(filePaths.size());

    // Read tags for each file
    if (m_app) {
        for (size_t i = 0; i < filePaths.size(); ++i) {
            m_app->m_tagManager.ReadTags(filePaths[i], m_originalFileTags[i]);
        }
    }

    // Compute intersection of all tag lists (common tags)
    m_imageTags.clear();
    if (!m_originalFileTags.empty()) {
        m_imageTags = m_originalFileTags[0];
        for (size_t i = 1; i < m_originalFileTags.size(); ++i) {
            std::vector<std::wstring> intersection;
            for (const auto& tag : m_imageTags) {
                if (std::find(m_originalFileTags[i].begin(), m_originalFileTags[i].end(), tag) != m_originalFileTags[i].end()) {
                    intersection.push_back(tag);
                }
            }
            m_imageTags = intersection;
        }
    }
    m_initialCommonTags = m_imageTags;

    // Load available tags from folder's xmpTags.json
    LoadAvailableTags();

    if (m_app) {
        m_app->m_window.RequestRedraw();
    }
}

void TaggingModal::Hide() {
    m_visible = false;
    if (m_app) {
        m_app->m_window.RequestRedraw();
    }
}

bool TaggingModal::LoadAvailableTags() {
    m_availableTags.clear();
    m_filteredTags.clear();
    m_selectedAvailableIndices.clear();
    m_categories.clear();
    m_tagToCategory.clear();

    if (m_filePath.empty()) return false;

    std::filesystem::path jsonPath = std::filesystem::path(m_filePath).parent_path() / "xmpTags.json";
    if (!std::filesystem::exists(jsonPath)) {
        return false;
    }

    std::ifstream file(jsonPath, std::ios::binary);
    if (!file) return false;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Convert UTF-8 to wstring
    std::wstring wcontent;
    int len = MultiByteToWideChar(CP_UTF8, 0, content.c_str(), static_cast<int>(content.size()), nullptr, 0);
    if (len > 0) {
        wcontent.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, content.c_str(), static_cast<int>(content.size()), &wcontent[0], len);
    } else {
        return false;
    }

    size_t firstCharPos = wcontent.find_first_not_of(L" \t\r\n");
    if (firstCharPos == std::wstring::npos) return false;
    wchar_t startChar = wcontent[firstCharPos];

    if (startChar == L'{') {
        // Categorized JSON: {"personajes": ["tag1", "tag2"], "lugares": ["tag3"]}
        size_t i = firstCharPos + 1;
        std::wstring currentKey = L"";
        bool inArray = false;

        while (i < wcontent.size()) {
            wchar_t ch = wcontent[i];
            if (ch == L'"') {
                std::wstring str = L"";
                i++; // skip open quote
                while (i < wcontent.size() && wcontent[i] != L'"') {
                    if (wcontent[i] == L'\\' && i + 1 < wcontent.size()) {
                        str += wcontent[i + 1];
                        i += 2;
                    } else {
                        str += wcontent[i];
                        i++;
                    }
                }
                if (i < wcontent.size()) i++; // skip close quote

                if (!inArray) {
                    currentKey = str;
                    if (!currentKey.empty() && std::find(m_categories.begin(), m_categories.end(), currentKey) == m_categories.end()) {
                        m_categories.push_back(currentKey);
                    }
                } else {
                    if (!str.empty()) {
                        if (std::find(m_availableTags.begin(), m_availableTags.end(), str) == m_availableTags.end()) {
                            m_availableTags.push_back(str);
                        }
                        m_tagToCategory[str] = currentKey;
                    }
                }
            } else if (ch == L'[') {
                inArray = true;
                i++;
            } else if (ch == L']') {
                inArray = false;
                i++;
            } else {
                i++;
            }
        }
    } else {
        // Flat JSON list: ["tag1", "tag2"]
        int arrayDepth = 0;
        size_t i = firstCharPos;
        while (i < wcontent.size()) {
            wchar_t ch = wcontent[i];
            if (ch == L'[') {
                arrayDepth++;
                i++;
            } else if (ch == L']') {
                if (arrayDepth > 0) arrayDepth--;
                i++;
            } else if (ch == L'"') {
                std::wstring str = L"";
                i++; // skip open quote
                while (i < wcontent.size() && wcontent[i] != L'"') {
                    if (wcontent[i] == L'\\' && i + 1 < wcontent.size()) {
                        str += wcontent[i + 1];
                        i += 2;
                    } else {
                        str += wcontent[i];
                        i++;
                    }
                }
                if (i < wcontent.size()) i++; // skip close quote

                if (arrayDepth > 0 && !str.empty()) {
                    if (std::find(m_availableTags.begin(), m_availableTags.end(), str) == m_availableTags.end()) {
                        m_availableTags.push_back(str);
                    }
                }
            } else {
                i++;
            }
        }
    }

    std::sort(m_availableTags.begin(), m_availableTags.end());
    m_filteredTags = m_availableTags;
    m_selectedAvailableIndices.assign(m_filteredTags.size(), false);

    return true;
}

void TaggingModal::SaveChanges() {
    if (m_app) {
        if (m_multiImageMode) {
            // Find added and removed tags relative to initial common tags
            std::vector<std::wstring> addedTags;
            std::vector<std::wstring> removedTags;

            for (const auto& tag : m_imageTags) {
                if (std::find(m_initialCommonTags.begin(), m_initialCommonTags.end(), tag) == m_initialCommonTags.end()) {
                    addedTags.push_back(tag);
                }
            }

            for (const auto& tag : m_initialCommonTags) {
                if (std::find(m_imageTags.begin(), m_imageTags.end(), tag) == m_imageTags.end()) {
                    removedTags.push_back(tag);
                }
            }

            // Apply differences to each file
            for (size_t i = 0; i < m_filePaths.size(); ++i) {
                std::vector<std::wstring> currentTags = m_originalFileTags[i];

                // Remove tags
                for (const auto& rTag : removedTags) {
                    auto it = std::find_if(currentTags.begin(), currentTags.end(), [&](const std::wstring& s) {
                        return _wcsicmp(s.c_str(), rTag.c_str()) == 0;
                    });
                    if (it != currentTags.end()) {
                        currentTags.erase(it);
                    }
                }

                // Add tags (if not present)
                for (const auto& aTag : addedTags) {
                    auto it = std::find_if(currentTags.begin(), currentTags.end(), [&](const std::wstring& s) {
                        return _wcsicmp(s.c_str(), aTag.c_str()) == 0;
                    });
                    if (it == currentTags.end()) {
                        currentTags.push_back(aTag);
                    }
                }

                m_app->m_tagManager.WriteTags(m_filePaths[i], currentTags);
            }
        } else {
            m_app->m_tagManager.WriteMetadataExtended(
                m_filePath, m_imageTags, m_title, m_creator, m_description, m_rating
            );
        }
    }
    if (m_app) m_app->m_folderData.NotifyAttributesChanged();
    Hide();
}

void TaggingModal::GetScrollLimits(float& leftTotalH, float& leftMaxScroll, float& rightTotalH, float& rightMaxScroll) {
    leftTotalH = 0.0f;
    leftMaxScroll = 0.0f;
    rightTotalH = 0.0f;
    rightMaxScroll = 0.0f;

    if (!m_app) return;

    RectF bounds = GetBounds();
    bool hasAvailable = !m_availableTags.empty();

    RectF leftPane = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 105.0f, 456.0f, bounds.Height() - 195.0f);
    RectF rightPane;
    if (hasAvailable) {
        rightPane = RectF::FromXYWH(bounds.left + 594.0f, bounds.top + 105.0f, 528.0f, bounds.Height() - 195.0f);
    } else {
        rightPane = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 105.0f, bounds.Width() - 60.0f, bounds.Height() - 195.0f);
    }

    RectF listRect = RectF::FromXYWH(leftPane.left, leftPane.top + 46.0f, leftPane.Width(), leftPane.Height() - 46.0f);
    RectF gridRect = RectF::FromXYWH(rightPane.left, rightPane.top + 46.0f, rightPane.Width(), rightPane.Height() - 46.0f);

    // Left scroll limits
    if (hasAvailable) {
        float availX = listRect.left + 8.0f;
        float availY = listRect.top + 8.0f;
        float availH = 28.0f;
        float spacingX = 6.0f;
        float spacingY = 6.0f;

        if (!m_categories.empty()) {
            std::unordered_map<std::wstring, std::vector<size_t>> categoryToTagIndices;
            std::vector<size_t> uncategorizedIndices;

            for (size_t idx = 0; idx < m_filteredTags.size(); ++idx) {
                const auto& tag = m_filteredTags[idx];
                auto catIt = m_tagToCategory.find(tag);
                if (catIt != m_tagToCategory.end() && !catIt->second.empty()) {
                    categoryToTagIndices[catIt->second].push_back(idx);
                } else {
                    uncategorizedIndices.push_back(idx);
                }
            }

            for (const auto& catName : m_categories) {
                auto tagIt = categoryToTagIndices.find(catName);
                if (tagIt == categoryToTagIndices.end() || tagIt->second.empty()) {
                    continue;
                }

                if (availX > listRect.left + 8.0f) {
                    availX = listRect.left + 8.0f;
                    availY += availH + spacingY;
                }

                availY += 24.0f + spacingY;
                availX = listRect.left + 8.0f;

                for (size_t idx : tagIt->second) {
                    std::wstring tagStr = m_filteredTags[idx];
                    if (tagStr.length() > 30) {
                        tagStr = tagStr.substr(0, 27) + L"...";
                    }
                    float textW = m_app->m_renderer.MeasureText(tagStr, 14.0f).x;
                    float pillW = textW + 22.0f;

                    if (availX + pillW > listRect.right - 8.0f) {
                        availX = listRect.left + 8.0f;
                        availY += availH + spacingY;
                    }
                    availX += pillW + spacingX;
                }

                if (availX > listRect.left + 8.0f) {
                    availX = listRect.left + 8.0f;
                    availY += availH + spacingY;
                }
            }

            if (!uncategorizedIndices.empty()) {
                if (availX > listRect.left + 8.0f) {
                    availX = listRect.left + 8.0f;
                    availY += availH + spacingY;
                }

                availY += 24.0f + spacingY;
                availX = listRect.left + 8.0f;

                for (size_t idx : uncategorizedIndices) {
                    std::wstring tagStr = m_filteredTags[idx];
                    if (tagStr.length() > 30) {
                        tagStr = tagStr.substr(0, 27) + L"...";
                    }
                    float textW = m_app->m_renderer.MeasureText(tagStr, 14.0f).x;
                    float pillW = textW + 22.0f;

                    if (availX + pillW > listRect.right - 8.0f) {
                        availX = listRect.left + 8.0f;
                        availY += availH + spacingY;
                    }
                    availX += pillW + spacingX;
                }

                if (availX > listRect.left + 8.0f) {
                    availX = listRect.left + 8.0f;
                    availY += availH + spacingY;
                }
            }
        } else {
            for (int i = 0; i < static_cast<int>(m_filteredTags.size()); ++i) {
                std::wstring tagStr = m_filteredTags[i];
                if (tagStr.length() > 30) {
                    tagStr = tagStr.substr(0, 27) + L"...";
                }
                float textW = m_app->m_renderer.MeasureText(tagStr, 14.0f).x;
                float pillW = textW + 22.0f;

                if (availX + pillW > listRect.right - 8.0f) {
                    availX = listRect.left + 8.0f;
                    availY += availH + spacingY;
                }
                availX += pillW + spacingX;
            }
            if (!m_filteredTags.empty()) {
                availY += availH + spacingY;
            }
        }

        leftTotalH = availY - listRect.top;
        leftMaxScroll = std::max(0.0f, leftTotalH - listRect.Height());
    }

    // Right scroll limits
    float tagX = gridRect.left + 8.0f;
    float tagY = gridRect.top + 8.0f;
    float tagH = 28.0f;
    float spacingX = 6.0f;
    float spacingY = 6.0f;

    for (int i = 0; i < static_cast<int>(m_imageTags.size()); ++i) {
        std::wstring tagStr = m_imageTags[i];
        if (tagStr.length() > 30) {
            tagStr = tagStr.substr(0, 27) + L"...";
        }
        float textW = m_app->m_renderer.MeasureText(tagStr, 14.0f).x;
        float pillW = textW + 36.0f;

        if (tagX + pillW > gridRect.right - 8.0f) {
            tagX = gridRect.left + 8.0f;
            tagY += tagH + spacingY;
        }
        tagX += pillW + spacingX;
    }

    if (m_imageTags.empty()) {
        rightTotalH = 0.0f;
        rightMaxScroll = 0.0f;
    } else {
        rightTotalH = (tagY + tagH) - gridRect.top;
        rightMaxScroll = std::max(0.0f, rightTotalH - gridRect.Height());
    }
}

float TaggingModal::GetLayoutScale() const {
    const auto size=m_app->m_renderer.GetTargetSize().ToFloat();
    return std::max(0.1f,std::min({1.0f,(size.width-16)/1152,(size.height-16)/840}));
}
RectF TaggingModal::GetBounds() const {
    if (!m_app) return {};
    auto screenSize = m_app->m_renderer.GetTargetSize().ToFloat();
    const float scale=GetLayoutScale(); screenSize.width/=scale; screenSize.height/=scale;
    float boxW = 1152.0f;
    float boxH = 840.0f;
    return RectF::FromXYWH(
        (screenSize.width - boxW) / 2.0f,
        (screenSize.height - boxH) / 2.0f,
        boxW,
        boxH
    );
}

void TaggingModal::Draw(Renderer& renderer) {
    if (!m_visible) return;

    auto screenSize = renderer.GetTargetSize().ToFloat();
    const float scale=GetLayoutScale(); screenSize.width/=scale; screenSize.height/=scale;
    auto* context=renderer.GetD2DContext();
    D2D1_MATRIX_3X2_F previous; context->GetTransform(&previous);
    context->SetTransform(D2D1::Matrix3x2F::Scale(scale,scale)*previous);
    RectF bounds = GetBounds();

    // 1. Dark overlay background
    renderer.FillRect(RectF::FromXYWH(0, 0, screenSize.width, screenSize.height), Color::Black(0.65f));

    // 2. Main Dialog Box
    Color panelBg = Color::FromRGBA(15, 15, 20, 245);
    Color panelBorder = Color::FromRGBA(0, 240, 255, 60); // Cyberpunk Cyan accent
    renderer.FillRoundedRect(bounds, 8.0f, 8.0f, panelBg);
    renderer.DrawRoundedRect(bounds, 8.0f, 8.0f, panelBorder, 1.5f);

    // 3. Draw Title (Filename or Multi-Image Title)
    std::wstring dialogTitle;
    if (m_multiImageMode) {
        dialogTitle = _(L"Tags for {} images");
        size_t placeholder = dialogTitle.find(L"{}");
        if (placeholder != std::wstring::npos) {
            dialogTitle.replace(placeholder, 2, std::to_wstring(m_filePaths.size()));
        }
    } else {
        dialogTitle = std::filesystem::path(m_filePath).filename().wstring();
    }
    RectF titleRect = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 20.0f, bounds.Width() - 60.0f, 30.0f);
    renderer.DrawText(
        dialogTitle,
        titleRect,
        Color::White(),
        22.0f,
        TextAlign::Left,
        TextVAlign::Center
    );

    // 4. Draw Tabs
    float tabX = bounds.left + 30.0f;
    float tabY = bounds.top + 60.0f;
    float tabH = 30.0f;
    std::vector<std::wstring> tabLabels = { L"Tags", L"Metadata" };

    for (int i = 0; i < 2; ++i) {
        std::wstring label = _(tabLabels[i]);
        float labelW = 110.0f;
        RectF tabRect = RectF::FromXYWH(tabX, tabY, labelW, tabH);

        bool active = (m_currentTab == i);
        bool hovered = (m_hoveredTabIdx == i);
        bool disabled = (i == 1 && m_multiImageMode);

        Color textColor;
        if (disabled) {
            textColor = Color::FromRGBA(80, 80, 80);
        } else {
            textColor = active ? Color::White() : (hovered ? Color::FromRGBA(0, 240, 255) : Color::FromRGBA(160, 160, 160));
        }

        renderer.DrawText(label, tabRect, textColor, 17.0f, TextAlign::Center, TextVAlign::Center);
        if (active) {
            // Neon red active underline
            renderer.FillRect(
                RectF::FromXYWH(tabRect.left + 10.0f, tabRect.bottom - 2.0f, tabRect.Width() - 20.0f, 2.0f),
                Color::FromRGBA(255, 45, 85)
            );
        }
        tabX += labelW + 10.0f;
    }

    // Divider line
    renderer.DrawLine(
        { bounds.left + 25.0f, bounds.top + 92.0f },
        { bounds.right - 25.0f, bounds.top + 92.0f },
        Color::FromRGBA(255, 255, 255, 20),
        1.0f
    );

    // 5. Render Tab Content
    if (m_currentTab == 0) {
        bool hasAvailable = !m_availableTags.empty();
        RectF leftPane = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 105.0f, 456.0f, bounds.Height() - 195.0f);
        RectF rightPane;
        if (hasAvailable) {
            rightPane = RectF::FromXYWH(bounds.left + 594.0f, bounds.top + 105.0f, 528.0f, bounds.Height() - 195.0f);
        } else {
            rightPane = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 105.0f, bounds.Width() - 60.0f, bounds.Height() - 195.0f);
        }
        DrawTagsTab(renderer, leftPane, rightPane);
    } else {
        RectF contentBounds = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 105.0f, bounds.Width() - 60.0f, bounds.Height() - 195.0f);
        DrawMetadataTab(renderer, contentBounds);
    }

    // 6. Draw Bottom Actions (Save / Cancel)
    RectF cancelRect = RectF::FromXYWH(bounds.right - 150.0f, bounds.bottom - 52.0f, 120.0f, 32.0f);
    RectF saveRect = RectF::FromXYWH(bounds.right - 285.0f, bounds.bottom - 52.0f, 120.0f, 32.0f);

    // Cancel Button
    Color cancelBg = (m_hoveredButton == 2) ? Color::FromRGBA(60, 60, 65) : Color::FromRGBA(35, 35, 40);
    renderer.FillRoundedRect(cancelRect, 4.0f, 4.0f, cancelBg);
    renderer.DrawRoundedRect(cancelRect, 4.0f, 4.0f, Color::White(0.15f), 1.0f);
    renderer.DrawText(_(L"Cancel"), cancelRect, Color::White(0.9f), 15.0f, TextAlign::Center, TextVAlign::Center);

    // Save Button
    Color saveBg = (m_hoveredButton == 1) ? Color::FromRGBA(255, 55, 95) : Color::FromRGBA(220, 40, 75);
    renderer.FillRoundedRect(saveRect, 4.0f, 4.0f, saveBg);
    renderer.DrawRoundedRect(saveRect, 4.0f, 4.0f, Color::White(0.2f), 1.0f);
    renderer.DrawText(_(L"Save"), saveRect, Color::White(), 15.0f, TextAlign::Center, TextVAlign::Center);
    context->SetTransform(previous);
}

void TaggingModal::DrawTagsTab(Renderer& renderer, const RectF& leftPane, const RectF& rightPane) {
    bool hasAvailable = !m_availableTags.empty();
    float totalH = 0.0f, maxScroll = 0.0f;
    float gridTotalH = 0.0f, maxRightScroll = 0.0f;
    GetScrollLimits(totalH, maxScroll, gridTotalH, maxRightScroll);

    m_leftScroll = std::clamp(m_leftScroll, 0.0f, maxScroll);
    m_rightScroll = std::clamp(m_rightScroll, 0.0f, maxRightScroll);

    if (hasAvailable) {
        // --- LEFT PANE: AVAILABLE TAGS ---
        // 1. Search Box
        RectF searchRect = RectF::FromXYWH(leftPane.left, leftPane.top, leftPane.Width(), 36.0f);
        bool searchFocused = (m_activeInputType == 1);
        Color searchBorder = searchFocused ? Color::FromRGBA(0, 240, 255) : Color::FromRGBA(255, 255, 255, 30);
        renderer.FillRoundedRect(searchRect, 4.0f, 4.0f, Color::FromRGBA(20, 20, 25, 200));
        renderer.DrawRoundedRect(searchRect, 4.0f, 4.0f, searchBorder, 1.0f);

        if (m_searchText.empty()) {
            renderer.DrawText(_(L"Search tags..."), searchRect.Inset(8.0f), Color::FromRGBA(120, 120, 120), 15.0f, TextAlign::Left, TextVAlign::Center);
        } else {
            renderer.DrawText(m_searchText, searchRect.Inset(8.0f), Color::White(), 15.0f, TextAlign::Left, TextVAlign::Center);
        }

        if (searchFocused) {
            // Blinking caret
            static auto lastCaret = std::chrono::steady_clock::now();
            static bool caretVisible = true;
            if (std::chrono::steady_clock::now() - lastCaret > std::chrono::milliseconds(500)) {
                caretVisible = !caretVisible;
                lastCaret = std::chrono::steady_clock::now();
            }
            if (caretVisible) {
                float textW = renderer.MeasureText(m_searchText, 15.0f).x;
                float caretX = searchRect.left + 8.0f + textW;
                renderer.DrawLine({ caretX, searchRect.top + 6.0f }, { caretX, searchRect.bottom - 6.0f }, Color::FromRGBA(0, 240, 255), 1.5f);
            }
        }

        // 2. Scrollable List of Available Tags (Pills wrapped, categorized)
        RectF listRect = RectF::FromXYWH(leftPane.left, leftPane.top + 46.0f, leftPane.Width(), leftPane.Height() - 46.0f);
        renderer.FillRoundedRect(listRect, 4.0f, 4.0f, Color::FromRGBA(20, 20, 25, 150));
        renderer.DrawRoundedRect(listRect, 4.0f, 4.0f, Color::FromRGBA(255, 255, 255, 15), 1.0f);

        renderer.PushClip(listRect);

        float availX = listRect.left + 8.0f;
        float availY = listRect.top + 8.0f - m_leftScroll;
        float availH = 28.0f;
        float availSpacingX = 6.0f;
        float availSpacingY = 6.0f;

        if (!m_categories.empty()) {
            std::unordered_map<std::wstring, std::vector<size_t>> categoryToTagIndices;
            std::vector<size_t> uncategorizedIndices;

            for (size_t idx = 0; idx < m_filteredTags.size(); ++idx) {
                const auto& tag = m_filteredTags[idx];
                auto catIt = m_tagToCategory.find(tag);
                if (catIt != m_tagToCategory.end() && !catIt->second.empty()) {
                    categoryToTagIndices[catIt->second].push_back(idx);
                } else {
                    uncategorizedIndices.push_back(idx);
                }
            }

            for (const auto& catName : m_categories) {
                auto tagIt = categoryToTagIndices.find(catName);
                if (tagIt == categoryToTagIndices.end() || tagIt->second.empty()) {
                    continue;
                }

                if (availX > listRect.left + 8.0f) {
                    availX = listRect.left + 8.0f;
                    availY += availH + availSpacingY;
                }

                // Render Header
                RectF headerRect = RectF::FromXYWH(listRect.left + 8.0f, availY + 4.0f, listRect.Width() - 16.0f, 20.0f);
                if (headerRect.bottom > listRect.top && headerRect.top < listRect.bottom) {
                    renderer.DrawText(
                        catName,
                        headerRect,
                        Color::FromRGBA(0, 240, 255, 180),
                        14.0f,
                        TextAlign::Left,
                        TextVAlign::Center
                    );
                }
                availY += 24.0f + availSpacingY;
                availX = listRect.left + 8.0f;

                for (size_t idx : tagIt->second) {
                    std::wstring tagStr = m_filteredTags[idx];
                    if (tagStr.length() > 30) {
                        tagStr = tagStr.substr(0, 27) + L"...";
                    }

                    float textW = renderer.MeasureText(tagStr, 14.0f).x;
                    float pillW = textW + 22.0f;

                    if (availX + pillW > listRect.right - 8.0f) {
                        availX = listRect.left + 8.0f;
                        availY += availH + availSpacingY;
                    }

                    RectF pillRect = RectF::FromXYWH(availX, availY, pillW, availH);

                    if (pillRect.bottom > listRect.top && pillRect.top < listRect.bottom) {
                        bool selected = m_selectedAvailableIndices[idx];
                        bool hovered = (m_hoveredAvailableIdx == idx);

                        Color pillBg = selected ? Color::FromRGBA(0, 240, 255, 50) : (hovered ? Color::FromRGBA(255, 255, 255, 15) : Color::FromRGBA(255, 255, 255, 10));
                        Color pillBorder = selected ? Color::FromRGBA(0, 240, 255, 150) : (hovered ? Color::FromRGBA(255, 255, 255, 35) : Color::FromRGBA(255, 255, 255, 20));

                        renderer.FillRoundedRect(pillRect, 4.0f, 4.0f, pillBg);
                        renderer.DrawRoundedRect(pillRect, 4.0f, 4.0f, pillBorder, 1.0f);

                        Color textColor = selected ? Color::FromRGBA(0, 240, 255) : Color::White(0.9f);
                        RectF textRect = RectF::FromXYWH(pillRect.left + 11.0f, pillRect.top, textW, pillRect.Height());
                        renderer.DrawText(tagStr, textRect, textColor, 14.0f, TextAlign::Left, TextVAlign::Center);
                    }

                    availX += pillW + availSpacingX;
                }

                if (availX > listRect.left + 8.0f) {
                    availX = listRect.left + 8.0f;
                    availY += availH + availSpacingY;
                }
            }

            if (!uncategorizedIndices.empty()) {
                if (availX > listRect.left + 8.0f) {
                    availX = listRect.left + 8.0f;
                    availY += availH + availSpacingY;
                }

                // Render Header
                RectF headerRect = RectF::FromXYWH(listRect.left + 8.0f, availY + 4.0f, listRect.Width() - 16.0f, 20.0f);
                if (headerRect.bottom > listRect.top && headerRect.top < listRect.bottom) {
                    renderer.DrawText(
                        _(L"Others"),
                        headerRect,
                        Color::FromRGBA(0, 240, 255, 180),
                        14.0f,
                        TextAlign::Left,
                        TextVAlign::Center
                    );
                }
                availY += 24.0f + availSpacingY;
                availX = listRect.left + 8.0f;

                for (size_t idx : uncategorizedIndices) {
                    std::wstring tagStr = m_filteredTags[idx];
                    if (tagStr.length() > 30) {
                        tagStr = tagStr.substr(0, 27) + L"...";
                    }

                    float textW = renderer.MeasureText(tagStr, 14.0f).x;
                    float pillW = textW + 22.0f;

                    if (availX + pillW > listRect.right - 8.0f) {
                        availX = listRect.left + 8.0f;
                        availY += availH + availSpacingY;
                    }

                    RectF pillRect = RectF::FromXYWH(availX, availY, pillW, availH);

                    if (pillRect.bottom > listRect.top && pillRect.top < listRect.bottom) {
                        bool selected = m_selectedAvailableIndices[idx];
                        bool hovered = (m_hoveredAvailableIdx == idx);

                        Color pillBg = selected ? Color::FromRGBA(0, 240, 255, 50) : (hovered ? Color::FromRGBA(255, 255, 255, 15) : Color::FromRGBA(255, 255, 255, 10));
                        Color pillBorder = selected ? Color::FromRGBA(0, 240, 255, 150) : (hovered ? Color::FromRGBA(255, 255, 255, 35) : Color::FromRGBA(255, 255, 255, 20));

                        renderer.FillRoundedRect(pillRect, 4.0f, 4.0f, pillBg);
                        renderer.DrawRoundedRect(pillRect, 4.0f, 4.0f, pillBorder, 1.0f);

                        Color textColor = selected ? Color::FromRGBA(0, 240, 255) : Color::White(0.9f);
                        RectF textRect = RectF::FromXYWH(pillRect.left + 11.0f, pillRect.top, textW, pillRect.Height());
                        renderer.DrawText(tagStr, textRect, textColor, 14.0f, TextAlign::Left, TextVAlign::Center);
                    }

                    availX += pillW + availSpacingX;
                }

                if (availX > listRect.left + 8.0f) {
                    availX = listRect.left + 8.0f;
                    availY += availH + availSpacingY;
                }
            }
        } else {
            // Flat list
            for (int i = 0; i < static_cast<int>(m_filteredTags.size()); ++i) {
                std::wstring tagStr = m_filteredTags[i];
                if (tagStr.length() > 30) {
                    tagStr = tagStr.substr(0, 27) + L"...";
                }

                float textW = renderer.MeasureText(tagStr, 14.0f).x;
                float pillW = textW + 22.0f;

                if (availX + pillW > listRect.right - 8.0f) {
                    availX = listRect.left + 8.0f;
                    availY += availH + availSpacingY;
                }

                RectF pillRect = RectF::FromXYWH(availX, availY, pillW, availH);

                if (pillRect.bottom > listRect.top && pillRect.top < listRect.bottom) {
                    bool selected = m_selectedAvailableIndices[i];
                    bool hovered = (m_hoveredAvailableIdx == i);

                    Color pillBg = selected ? Color::FromRGBA(0, 240, 255, 50) : (hovered ? Color::FromRGBA(255, 255, 255, 15) : Color::FromRGBA(255, 255, 255, 10));
                    Color pillBorder = selected ? Color::FromRGBA(0, 240, 255, 150) : (hovered ? Color::FromRGBA(255, 255, 255, 35) : Color::FromRGBA(255, 255, 255, 20));

                    renderer.FillRoundedRect(pillRect, 4.0f, 4.0f, pillBg);
                    renderer.DrawRoundedRect(pillRect, 4.0f, 4.0f, pillBorder, 1.0f);

                    Color textColor = selected ? Color::FromRGBA(0, 240, 255) : Color::White(0.9f);
                    RectF textRect = RectF::FromXYWH(pillRect.left + 11.0f, pillRect.top, textW, pillRect.Height());
                    renderer.DrawText(tagStr, textRect, textColor, 14.0f, TextAlign::Left, TextVAlign::Center);
                }

                availX += pillW + availSpacingX;
            }
        }

        renderer.PopClip();

        // Draw left scrollbar track if scrollable
        if (maxScroll > 0.0f) {
            float scrollPct = m_leftScroll / maxScroll;
            float trackH = listRect.Height() - 20.0f;
            float thumbH = std::max(20.0f, (listRect.Height() / totalH) * trackH);
            float thumbY = listRect.top + 10.0f + (trackH - thumbH) * scrollPct;
            RectF thumbRect = RectF::FromXYWH(listRect.right - 6.0f, thumbY, 4.0f, thumbH);
            renderer.FillRoundedRect(thumbRect, 2.0f, 2.0f, Color::FromRGBA(255, 255, 255, 60));
        }

        // --- MIDDLE COPY BUTTON [ > ] ---
        RectF midBtnRect = RectF::FromXYWH(leftPane.right + 36.0f, leftPane.top + (leftPane.Height() - 50.0f) / 2.0f, 36.0f, 50.0f);
        Color midBtnBg = (m_hoveredButton == 3) ? Color::FromRGBA(0, 240, 255, 180) : Color::FromRGBA(30, 30, 35);
        Color arrowColor = (m_hoveredButton == 3) ? Color::Black() : Color::FromRGBA(180, 180, 180);
        renderer.FillRoundedRect(midBtnRect, 4.0f, 4.0f, midBtnBg);
        renderer.DrawRoundedRect(midBtnRect, 4.0f, 4.0f, Color::White(0.15f), 1.0f);
        renderer.DrawText(L">", midBtnRect, arrowColor, 19.0f, TextAlign::Center, TextVAlign::Center);
    }

    // --- RIGHT PANE: IMAGE TAGS ---
    // 1. Manual tag creation box
    RectF addRect = RectF::FromXYWH(rightPane.left, rightPane.top, rightPane.Width() - 40.0f, 36.0f);
    bool addFocused = (m_activeInputType == 6);
    Color addBorder = addFocused ? Color::FromRGBA(0, 240, 255) : Color::FromRGBA(255, 255, 255, 30);
    renderer.FillRoundedRect(addRect, 4.0f, 4.0f, Color::FromRGBA(20, 20, 25, 200));
    renderer.DrawRoundedRect(addRect, 4.0f, 4.0f, addBorder, 1.0f);

    if (m_manualTagText.empty()) {
        renderer.DrawText(_(L"New tag..."), addRect.Inset(8.0f), Color::FromRGBA(120, 120, 120), 15.0f, TextAlign::Left, TextVAlign::Center);
    } else {
        renderer.DrawText(m_manualTagText, addRect.Inset(8.0f), Color::White(), 15.0f, TextAlign::Left, TextVAlign::Center);
    }

    if (addFocused) {
        static auto lastCaretAdd = std::chrono::steady_clock::now();
        static bool caretAddVisible = true;
        if (std::chrono::steady_clock::now() - lastCaretAdd > std::chrono::milliseconds(500)) {
            caretAddVisible = !caretAddVisible;
            lastCaretAdd = std::chrono::steady_clock::now();
        }
        if (caretAddVisible) {
            float textW = renderer.MeasureText(m_manualTagText, 15.0f).x;
            float caretX = addRect.left + 8.0f + textW;
            renderer.DrawLine({ caretX, addRect.top + 6.0f }, { caretX, addRect.bottom - 6.0f }, Color::FromRGBA(0, 240, 255), 1.5f);
        }
    }

    // Plus (+) button
    RectF plusRect = RectF::FromXYWH(rightPane.right - 36.0f, rightPane.top, 36.0f, 36.0f);
    Color plusBg = (m_hoveredButton == 4) ? Color::FromRGBA(0, 240, 255, 100) : Color::FromRGBA(30, 30, 35);
    renderer.FillRoundedRect(plusRect, 4.0f, 4.0f, plusBg);
    renderer.DrawRoundedRect(plusRect, 4.0f, 4.0f, Color::White(0.15f), 1.0f);
    renderer.DrawText(L"+", plusRect, Color::White(), 17.0f, TextAlign::Center, TextVAlign::Center);

    // 2. Tags Grid View
    RectF gridRect = RectF::FromXYWH(rightPane.left, rightPane.top + 46.0f, rightPane.Width(), rightPane.Height() - 46.0f);
    renderer.FillRoundedRect(gridRect, 4.0f, 4.0f, Color::FromRGBA(20, 20, 25, 150));
    renderer.DrawRoundedRect(gridRect, 4.0f, 4.0f, Color::FromRGBA(255, 255, 255, 15), 1.0f);

    renderer.PushClip(gridRect);

    float tagX = gridRect.left + 8.0f;
    float tagY = gridRect.top + 8.0f - m_rightScroll;
    float tagH = 28.0f;
    float spacingX = 6.0f;
    float spacingY = 6.0f;

    for (int i = 0; i < static_cast<int>(m_imageTags.size()); ++i) {
        std::wstring tagStr = m_imageTags[i];
        if (tagStr.length() > 30) {
            tagStr = tagStr.substr(0, 27) + L"...";
        }

        // Measure size of pill
        float textW = renderer.MeasureText(tagStr, 14.0f).x;
        float pillW = textW + 36.0f; // extra space for padding and the close 'x'

        // Check wrap
        if (tagX + pillW > gridRect.right - 8.0f) {
            tagX = gridRect.left + 8.0f;
            tagY += tagH + spacingY;
        }

        RectF pillRect = RectF::FromXYWH(tagX, tagY, pillW, tagH);

        if (pillRect.bottom > gridRect.top && pillRect.top < gridRect.bottom) {
            bool isEditing = (m_editingTagIdx == i);
            bool isHovered = (m_hoveredImageTagIdx == i);

            if (isEditing) {
                // Render text box inside the pill
                renderer.FillRoundedRect(pillRect, 4.0f, 4.0f, Color::FromRGBA(30, 30, 45, 255));
                renderer.DrawRoundedRect(pillRect, 4.0f, 4.0f, Color::FromRGBA(0, 240, 255), 1.0f);
                renderer.DrawText(m_editingTagText, pillRect.Inset(4.0f), Color::White(), 14.0f, TextAlign::Left, TextVAlign::Center);

                // Blinking caret inside tag editor
                static auto lastTagCaret = std::chrono::steady_clock::now();
                static bool tagCaretVisible = true;
                if (std::chrono::steady_clock::now() - lastTagCaret > std::chrono::milliseconds(500)) {
                    tagCaretVisible = !tagCaretVisible;
                    lastTagCaret = std::chrono::steady_clock::now();
                }
                if (tagCaretVisible) {
                    float editW = renderer.MeasureText(m_editingTagText, 14.0f).x;
                    float editCaretX = pillRect.left + 4.0f + editW;
                    renderer.DrawLine({ editCaretX, pillRect.top + 4.0f }, { editCaretX, pillRect.bottom - 4.0f }, Color::FromRGBA(0, 240, 255), 1.5f);
                }
            } else {
                Color pillBg = isHovered ? Color::FromRGBA(255, 45, 85, 30) : Color::FromRGBA(255, 255, 255, 10);
                Color pillBorder = isHovered ? Color::FromRGBA(255, 45, 85, 120) : Color::FromRGBA(255, 255, 255, 20);
                renderer.FillRoundedRect(pillRect, 4.0f, 4.0f, pillBg);
                renderer.DrawRoundedRect(pillRect, 4.0f, 4.0f, pillBorder, 1.0f);

                // Text
                RectF textRect = RectF::FromXYWH(pillRect.left + 6.0f, pillRect.top, textW, pillRect.Height());
                renderer.DrawText(tagStr, textRect, Color::White(0.9f), 14.0f, TextAlign::Left, TextVAlign::Center);

                // Close 'x' icon
                RectF xRect = RectF::FromXYWH(pillRect.right - 22.0f, pillRect.top, 18.0f, pillRect.Height());
                Color xColor = isHovered ? Color::FromRGBA(255, 45, 85) : Color::White(0.5f);
                renderer.DrawText(L"×", xRect, xColor, 15.0f, TextAlign::Center, TextVAlign::Center);
            }
        }

        tagX += pillW + spacingX;
    }

    renderer.PopClip();

    // Draw right scrollbar track if scrollable
    if (maxRightScroll > 0.0f) {
        float scrollPct = m_rightScroll / maxRightScroll;
        float trackH = gridRect.Height() - 20.0f;
        float thumbH = std::max(20.0f, (gridRect.Height() / gridTotalH) * trackH);
        float thumbY = gridRect.top + 10.0f + (trackH - thumbH) * scrollPct;
        RectF thumbRect = RectF::FromXYWH(gridRect.right - 6.0f, thumbY, 4.0f, thumbH);
        renderer.FillRoundedRect(thumbRect, 2.0f, 2.0f, Color::FromRGBA(255, 255, 255, 60));
    }

    // --- BOTTOM PANE: STAR RATING ---
    RectF bounds = GetBounds();
    if (m_multiImageMode) {
        float startX = hasAvailable ? leftPane.left : rightPane.left;
        RectF starsTextRect = RectF::FromXYWH(startX, rightPane.bottom + 15.0f, bounds.Width() - 60.0f, 30.0f);
        renderer.DrawText(_(L"Rating unavailable for multiple images"), starsTextRect, Color::FromRGBA(120, 120, 120), 15.0f, TextAlign::Left, TextVAlign::Center);
    } else {
        float startX = hasAvailable ? leftPane.left : rightPane.left;
        RectF starsTextRect = RectF::FromXYWH(startX, rightPane.bottom + 15.0f, 120.0f, 30.0f);
        renderer.DrawText(_(L"Rating") + L":", starsTextRect, Color::White(0.85f), 15.0f, TextAlign::Left, TextVAlign::Center);

        float starX = starsTextRect.right + 12.0f;
        float starY = starsTextRect.top;
        float starW = 28.0f;

        for (int i = 1; i <= 5; ++i) {
            RectF starRect = RectF::FromXYWH(starX, starY, starW, 30.0f);
            bool filled = (m_hoveredStars > 0) ? (i <= m_hoveredStars) : (i <= m_rating);

            Color starColor = filled ? Color::FromRGBA(255, 185, 0) : Color::FromRGBA(80, 80, 80);
            std::wstring starChar = filled ? L"★" : L"☆";
            renderer.DrawText(starChar, starRect, starColor, 22.0f, TextAlign::Center, TextVAlign::Center);

            starX += starW + 2.0f;
        }
    }
}

void TaggingModal::DrawMetadataTab(Renderer& renderer, const RectF& bounds) {
    // 1. File Statistics (Read-Only)
    std::wstring dimStr, sizeStr, modStr;
    GetFileStats(m_filePath, dimStr, sizeStr, modStr, m_app ? m_app->m_tagManager.GetWicFactory() : nullptr);

    float labelW = 100.0f;
    float valW = 200.0f;
    float itemH = 26.0f;

    float infoX = bounds.left + 20.0f;
    float infoY = bounds.top + 10.0f;

    std::vector<std::pair<std::wstring, std::wstring>> stats = {
        { _(L"Dimensions"), dimStr },
        { _(L"Size"), sizeStr },
        { _(L"Modified"), modStr }
    };

    if (!m_deviceMake.empty() || !m_deviceModel.empty()) {
        std::wstring camera = m_deviceMake;
        if (!m_deviceModel.empty()) {
            if (!camera.empty()) camera += L" ";
            camera += m_deviceModel;
        }
        stats.push_back({ _(L"Camera"), camera });
    }

    for (const auto& [lbl, val] : stats) {
        RectF lblRect = RectF::FromXYWH(infoX, infoY, labelW, itemH);
        RectF valRect = RectF::FromXYWH(infoX + labelW + 10.0f, infoY, valW + 150.0f, itemH);

        renderer.DrawText(lbl + L":", lblRect, Color::White(0.5f), 15.0f, TextAlign::Left, TextVAlign::Center);
        renderer.DrawText(val, valRect, Color::White(0.9f), 15.0f, TextAlign::Left, TextVAlign::Center);

        infoY += itemH + 2.0f;
    }

    // Divider line
    renderer.DrawLine(
        { bounds.left + 20.0f, infoY + 12.0f },
        { bounds.right - 20.0f, infoY + 12.0f },
        Color::FromRGBA(255, 255, 255, 15),
        1.0f
    );

    // 2. Editable Fields: Title, Creator, Description
    float fieldsY = infoY + 28.0f;

    // Helper lambda to draw text box
    auto drawTextBox = [&](const std::wstring& label, const std::wstring& text, RectF rect, int inputType, bool multiline = false) {
        // Label
        RectF lblRect = RectF::FromXYWH(rect.left, rect.top - 20.0f, rect.Width(), 18.0f);
        renderer.DrawText(label, lblRect, Color::White(0.85f), 15.0f, TextAlign::Left, TextVAlign::Top);

        // Box
        bool focused = (m_activeInputType == inputType);
        Color border = focused ? Color::FromRGBA(0, 240, 255) : Color::FromRGBA(255, 255, 255, 30);
        renderer.FillRoundedRect(rect, 4.0f, 4.0f, Color::FromRGBA(20, 20, 25, 200));
        renderer.DrawRoundedRect(rect, 4.0f, 4.0f, border, 1.0f);

        // Content
        RectF textRect = rect.Inset(8.0f);
        renderer.DrawText(text, textRect, Color::White(), 15.0f, TextAlign::Left, multiline ? TextVAlign::Top : TextVAlign::Center);

        // Blinking caret
        if (focused) {
            static auto lastCaretMeta = std::chrono::steady_clock::now();
            static bool caretMetaVisible = true;
            if (std::chrono::steady_clock::now() - lastCaretMeta > std::chrono::milliseconds(500)) {
                caretMetaVisible = !caretMetaVisible;
                lastCaretMeta = std::chrono::steady_clock::now();
            }
            if (caretMetaVisible) {
                float textW = 0.0f;
                float caretYStart = rect.top + 6.0f;
                float caretYEnd = rect.bottom - 6.0f;

                if (multiline) {
                    // Approximate text wrapping width for caret (or keep at end of last char)
                    textW = renderer.MeasureText(text, 15.0f).x;
                    // Clamp textW to width of text rect
                    if (textW > textRect.Width()) textW = textRect.Width();
                    caretYStart = rect.top + 8.0f;
                    caretYEnd = caretYStart + 16.0f;
                } else {
                    textW = renderer.MeasureText(text, 15.0f).x;
                }

                float caretX = textRect.left + textW;
                renderer.DrawLine({ caretX, caretYStart }, { caretX, caretYEnd }, Color::FromRGBA(0, 240, 255), 1.5f);
            }
        }
    };

    // Title
    RectF titleRect = RectF::FromXYWH(bounds.left + 20.0f, fieldsY + 20.0f, bounds.Width() - 40.0f, 40.0f);
    drawTextBox(_(L"Title"), m_title, titleRect, 3);

    // Creator / Author
    RectF creatorRect = RectF::FromXYWH(bounds.left + 20.0f, fieldsY + 85.0f, bounds.Width() - 40.0f, 40.0f);
    drawTextBox(_(L"Creator"), m_creator, creatorRect, 4);

    // Description
    RectF descRect = RectF::FromXYWH(bounds.left + 20.0f, fieldsY + 150.0f, bounds.Width() - 40.0f, 90.0f);
    drawTextBox(_(L"Description / Comment"), m_description, descRect, 5, true);
}

bool TaggingModal::OnMouseMove(float x, float y, KeyModifiers mods) {
    if (!m_visible) return false;
    const float scale=GetLayoutScale(); x/=scale; y/=scale;

    RectF bounds = GetBounds();

    if (m_leftScrollCaptured) {
        RectF leftPane = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 105.0f, 456.0f, bounds.Height() - 195.0f);
        RectF listRect = RectF::FromXYWH(leftPane.left, leftPane.top + 46.0f, leftPane.Width(), leftPane.Height() - 46.0f);
        float leftTotalH = 0.0f, maxScroll = 0.0f;
        float gridTotalH = 0.0f, maxRightScroll = 0.0f;
        GetScrollLimits(leftTotalH, maxScroll, gridTotalH, maxRightScroll);

        float trackH = listRect.Height() - 20.0f;
        if (leftTotalH > 0.0f) {
            float thumbH = std::max(20.0f, (listRect.Height() / leftTotalH) * trackH);
            float travelRange = trackH - thumbH;
            if (travelRange > 0.0f) {
                float deltaY = y - m_dragStartY;
                m_leftScroll = m_dragStartScrollOffset + deltaY * (maxScroll / travelRange);
                m_leftScroll = std::clamp(m_leftScroll, 0.0f, maxScroll);
            }
        }
        if (m_app) m_app->m_window.RequestRedraw();
        return true;
    }

    if (m_rightScrollCaptured) {
        RectF rightPane = RectF::FromXYWH(bounds.left + 594.0f, bounds.top + 105.0f, 528.0f, bounds.Height() - 195.0f);
        RectF gridRect = RectF::FromXYWH(rightPane.left, rightPane.top + 46.0f, rightPane.Width(), rightPane.Height() - 46.0f);
        float leftTotalH = 0.0f, maxScroll = 0.0f;
        float gridTotalH = 0.0f, maxRightScroll = 0.0f;
        GetScrollLimits(leftTotalH, maxScroll, gridTotalH, maxRightScroll);

        float trackH = gridRect.Height() - 20.0f;
        if (gridTotalH > 0.0f) {
            float thumbH = std::max(20.0f, (gridRect.Height() / gridTotalH) * trackH);
            float travelRange = trackH - thumbH;
            if (travelRange > 0.0f) {
                float deltaY = y - m_dragStartY;
                m_rightScroll = m_dragStartScrollOffset + deltaY * (maxRightScroll / travelRange);
                m_rightScroll = std::clamp(m_rightScroll, 0.0f, maxRightScroll);
            }
        }
        if (m_app) m_app->m_window.RequestRedraw();
        return true;
    }

    m_hoveredTabIdx = -1;
    m_hoveredButton = 0;
    m_hoveredAvailableIdx = -1;
    m_hoveredImageTagIdx = -1;
    m_hoveredStars = 0;

    // Check Tab click bounds
    float tabX = bounds.left + 30.0f;
    float tabY = bounds.top + 60.0f;
    float tabH = 30.0f;
    for (int i = 0; i < 2; ++i) {
        RectF tabRect = RectF::FromXYWH(tabX, tabY, 100.0f, tabH);
        if (tabRect.Contains(x, y)) {
            if (i == 1 && m_multiImageMode) {
                break;
            }
            m_hoveredTabIdx = i;
            if (m_app) m_app->m_window.RequestRedraw();
            return true;
        }
        tabX += 110.0f;
    }

    // Save & Cancel buttons
    RectF cancelRect = RectF::FromXYWH(bounds.right - 150.0f, bounds.bottom - 52.0f, 120.0f, 32.0f);
    RectF saveRect = RectF::FromXYWH(bounds.right - 285.0f, bounds.bottom - 52.0f, 120.0f, 32.0f);

    if (saveRect.Contains(x, y)) {
        m_hoveredButton = 1;
        if (m_app) m_app->m_window.RequestRedraw();
        return true;
    }
    if (cancelRect.Contains(x, y)) {
        m_hoveredButton = 2;
        if (m_app) m_app->m_window.RequestRedraw();
        return true;
    }

    bool hasAvailable = !m_availableTags.empty();

    if (m_currentTab == 0) {
        // --- TAGS TAB MOUSE HOVER ---
        RectF leftPane = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 105.0f, 456.0f, bounds.Height() - 195.0f);
        RectF rightPane;
        if (hasAvailable) {
            rightPane = RectF::FromXYWH(bounds.left + 594.0f, bounds.top + 105.0f, 528.0f, bounds.Height() - 195.0f);
        } else {
            rightPane = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 105.0f, bounds.Width() - 60.0f, bounds.Height() - 195.0f);
        }

        // Middle arrow button [ > ] - only if hasAvailable
        if (hasAvailable) {
            RectF midBtnRect = RectF::FromXYWH(leftPane.right + 36.0f, leftPane.top + (leftPane.Height() - 50.0f) / 2.0f, 36.0f, 50.0f);
            if (midBtnRect.Contains(x, y)) {
                m_hoveredButton = 3;
                if (m_app) m_app->m_window.RequestRedraw();
                return true;
            }
        }

        // Plus (+) button
        RectF plusRect = RectF::FromXYWH(rightPane.right - 36.0f, rightPane.top, 36.0f, 36.0f);
        if (plusRect.Contains(x, y)) {
            m_hoveredButton = 4;
            if (m_app) m_app->m_window.RequestRedraw();
            return true;
        }

        // Left Pane Scrollable list items - only if hasAvailable
        if (hasAvailable) {
            RectF listRect = RectF::FromXYWH(leftPane.left, leftPane.top + 46.0f, leftPane.Width(), leftPane.Height() - 46.0f);
            if (listRect.Contains(x, y)) {
                float availX = listRect.left + 8.0f;
                float availY = listRect.top + 8.0f - m_leftScroll;
                float availH = 28.0f;
                float spacingX = 6.0f;
                float spacingY = 6.0f;

                if (!m_categories.empty()) {
                    std::unordered_map<std::wstring, std::vector<size_t>> categoryToTagIndices;
                    std::vector<size_t> uncategorizedIndices;

                    for (size_t idx = 0; idx < m_filteredTags.size(); ++idx) {
                        const auto& tag = m_filteredTags[idx];
                        auto catIt = m_tagToCategory.find(tag);
                        if (catIt != m_tagToCategory.end() && !catIt->second.empty()) {
                            categoryToTagIndices[catIt->second].push_back(idx);
                        } else {
                            uncategorizedIndices.push_back(idx);
                        }
                    }

                    auto processIdx = [&](size_t idx) -> bool {
                        std::wstring tagStr = m_filteredTags[idx];
                        if (tagStr.length() > 30) {
                            tagStr = tagStr.substr(0, 27) + L"...";
                        }
                        float textW = m_app->m_renderer.MeasureText(tagStr, 14.0f).x;
                        float pillW = textW + 22.0f;

                        if (availX + pillW > listRect.right - 8.0f) {
                            availX = listRect.left + 8.0f;
                            availY += availH + spacingY;
                        }

                        RectF pillRect = RectF::FromXYWH(availX, availY, pillW, availH);
                        if (pillRect.Contains(x, y)) {
                            m_hoveredAvailableIdx = static_cast<int>(idx);
                            return true;
                        }

                        availX += pillW + spacingX;
                        return false;
                    };

                    for (const auto& catName : m_categories) {
                        auto tagIt = categoryToTagIndices.find(catName);
                        if (tagIt == categoryToTagIndices.end() || tagIt->second.empty()) {
                            continue;
                        }

                        if (availX > listRect.left + 8.0f) {
                            availX = listRect.left + 8.0f;
                            availY += availH + spacingY;
                        }

                        availY += 24.0f + spacingY;
                        availX = listRect.left + 8.0f;

                        for (size_t idx : tagIt->second) {
                            if (processIdx(idx)) {
                                if (m_app) m_app->m_window.RequestRedraw();
                                return true;
                            }
                        }

                        if (availX > listRect.left + 8.0f) {
                            availX = listRect.left + 8.0f;
                            availY += availH + spacingY;
                        }
                    }

                    if (!uncategorizedIndices.empty()) {
                        if (availX > listRect.left + 8.0f) {
                            availX = listRect.left + 8.0f;
                            availY += availH + spacingY;
                        }

                        availY += 24.0f + spacingY;
                        availX = listRect.left + 8.0f;

                        for (size_t idx : uncategorizedIndices) {
                            if (processIdx(idx)) {
                                if (m_app) m_app->m_window.RequestRedraw();
                                return true;
                            }
                        }

                        if (availX > listRect.left + 8.0f) {
                            availX = listRect.left + 8.0f;
                            availY += availH + spacingY;
                        }
                    }
                } else {
                    for (int i = 0; i < static_cast<int>(m_filteredTags.size()); ++i) {
                        std::wstring tagStr = m_filteredTags[i];
                        if (tagStr.length() > 30) {
                            tagStr = tagStr.substr(0, 27) + L"...";
                        }
                        float textW = m_app->m_renderer.MeasureText(tagStr, 14.0f).x;
                        float pillW = textW + 22.0f;

                        if (availX + pillW > listRect.right - 8.0f) {
                            availX = listRect.left + 8.0f;
                            availY += availH + spacingY;
                        }

                        RectF pillRect = RectF::FromXYWH(availX, availY, pillW, availH);
                        if (pillRect.Contains(x, y)) {
                            m_hoveredAvailableIdx = i;
                            if (m_app) m_app->m_window.RequestRedraw();
                            return true;
                        }

                        availX += pillW + spacingX;
                    }
                }
            }
        }

        // Right Pane Tags Grid
        RectF gridRect = RectF::FromXYWH(rightPane.left, rightPane.top + 46.0f, rightPane.Width(), rightPane.Height() - 46.0f);
        if (gridRect.Contains(x, y)) {
            float tagX = gridRect.left + 8.0f;
            float tagY = gridRect.top + 8.0f - m_rightScroll;
            float tagH = 28.0f;
            float spacingX = 6.0f;
            float spacingY = 6.0f;

            for (int i = 0; i < static_cast<int>(m_imageTags.size()); ++i) {
                std::wstring tagStr = m_imageTags[i];
                if (tagStr.length() > 30) {
                    tagStr = tagStr.substr(0, 27) + L"...";
                }
                float textW = m_app->m_renderer.MeasureText(tagStr, 14.0f).x;
                float pillW = textW + 36.0f;

                if (tagX + pillW > gridRect.right - 8.0f) {
                    tagX = gridRect.left + 8.0f;
                    tagY += tagH + spacingY;
                }

                RectF pillRect = RectF::FromXYWH(tagX, tagY, pillW, tagH);
                if (pillRect.Contains(x, y)) {
                    m_hoveredImageTagIdx = i;
                    if (m_app) m_app->m_window.RequestRedraw();
                    return true;
                }

                tagX += pillW + spacingX;
            }
        }

        // Star ratings
        if (!m_multiImageMode) {
            float startX = hasAvailable ? leftPane.left : rightPane.left;
            RectF starsTextRect = RectF::FromXYWH(startX, rightPane.bottom + 15.0f, 120.0f, 30.0f);
            float starX = starsTextRect.right + 12.0f;
            float starY = starsTextRect.top;
            float starW = 28.0f;

            for (int i = 1; i <= 5; ++i) {
                RectF starRect = RectF::FromXYWH(starX, starY, starW, 30.0f);
                if (starRect.Contains(x, y)) {
                    m_hoveredStars = i;
                    if (m_app) m_app->m_window.RequestRedraw();
                    return true;
                }
                starX += starW + 2.0f;
            }
        }
    }

    if (m_app) m_app->m_window.RequestRedraw();
    return true;
}

bool TaggingModal::OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (!m_visible) return false;
    const float scale=GetLayoutScale(); x/=scale; y/=scale;
    if (btn != MouseButton::Left) return true;

    RectF bounds = GetBounds();
    if (!bounds.Contains(x, y)) {
        Hide();
        return true;
    }

    bool hasAvailable = !m_availableTags.empty();

    if (m_currentTab == 0) {
        RectF leftPane = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 105.0f, 456.0f, bounds.Height() - 195.0f);
        RectF rightPane;
        if (hasAvailable) {
            rightPane = RectF::FromXYWH(bounds.left + 594.0f, bounds.top + 105.0f, 528.0f, bounds.Height() - 195.0f);
        } else {
            rightPane = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 105.0f, bounds.Width() - 60.0f, bounds.Height() - 195.0f);
        }
        RectF listRect = RectF::FromXYWH(leftPane.left, leftPane.top + 46.0f, leftPane.Width(), leftPane.Height() - 46.0f);
        RectF gridRect = RectF::FromXYWH(rightPane.left, rightPane.top + 46.0f, rightPane.Width(), rightPane.Height() - 46.0f);

        float leftTotalH = 0.0f, maxScroll = 0.0f;
        float gridTotalH = 0.0f, maxRightScroll = 0.0f;
        GetScrollLimits(leftTotalH, maxScroll, gridTotalH, maxRightScroll);

        if (hasAvailable) {
            RectF leftTrackRect = RectF::FromXYWH(listRect.right - 12.0f, listRect.top, 12.0f, listRect.Height());
            if (maxScroll > 0.0f && leftTrackRect.Contains(x, y)) {
                m_leftScrollCaptured = true;
                m_dragStartY = y;
                m_dragStartScrollOffset = m_leftScroll;
                if (m_app) m_app->m_window.RequestRedraw();
                return true;
            }
        }

        RectF rightTrackRect = RectF::FromXYWH(gridRect.right - 12.0f, gridRect.top, 12.0f, gridRect.Height());
        if (maxRightScroll > 0.0f && rightTrackRect.Contains(x, y)) {
            m_rightScrollCaptured = true;
            m_dragStartY = y;
            m_dragStartScrollOffset = m_rightScroll;
            if (m_app) m_app->m_window.RequestRedraw();
            return true;
        }
    }

    // Dismiss active text editor if clicking elsewhere
    m_activeInputType = 0;
    if (m_editingTagIdx != -1) {
        // commit changes of renamed tag
        if (!m_editingTagText.empty()) {
            m_imageTags[m_editingTagIdx] = m_editingTagText;
        }
        m_editingTagIdx = -1;
    }

    // Tabs
    float tabX = bounds.left + 30.0f;
    float tabY = bounds.top + 60.0f;
    float tabH = 30.0f;
    for (int i = 0; i < 2; ++i) {
        RectF tabRect = RectF::FromXYWH(tabX, tabY, 110.0f, tabH);
        if (tabRect.Contains(x, y)) {
            if (i == 1 && m_multiImageMode) {
                return true;
            }
            m_currentTab = i;
            if (m_app) m_app->m_window.RequestRedraw();
            return true;
        }
        tabX += 120.0f;
    }

    // Save & Cancel buttons
    RectF cancelRect = RectF::FromXYWH(bounds.right - 150.0f, bounds.bottom - 52.0f, 120.0f, 32.0f);
    RectF saveRect = RectF::FromXYWH(bounds.right - 285.0f, bounds.bottom - 52.0f, 120.0f, 32.0f);

    if (saveRect.Contains(x, y)) {
        SaveChanges();
        return true;
    }
    if (cancelRect.Contains(x, y)) {
        Hide();
        return true;
    }

    if (m_currentTab == 0) {
        // --- TAGS TAB CLICKS ---
        RectF leftPane = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 105.0f, 456.0f, bounds.Height() - 195.0f);
        RectF rightPane;
        if (hasAvailable) {
            rightPane = RectF::FromXYWH(bounds.left + 594.0f, bounds.top + 105.0f, 528.0f, bounds.Height() - 195.0f);
        } else {
            rightPane = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 105.0f, bounds.Width() - 60.0f, bounds.Height() - 195.0f);
        }

        // Search Input Focus - only if hasAvailable
        if (hasAvailable) {
            RectF searchRect = RectF::FromXYWH(leftPane.left, leftPane.top, leftPane.Width(), 36.0f);
            if (searchRect.Contains(x, y)) {
                m_activeInputType = 1;
                if (m_app) m_app->m_window.RequestRedraw();
                return true;
            }
        }

        // Available Tags list clicks - only if hasAvailable
        if (hasAvailable) {
            RectF listRect = RectF::FromXYWH(leftPane.left, leftPane.top + 46.0f, leftPane.Width(), leftPane.Height() - 46.0f);
            if (listRect.Contains(x, y)) {
                float availX = listRect.left + 8.0f;
                float availY = listRect.top + 8.0f - m_leftScroll;
                float availH = 28.0f;
                float spacingX = 6.0f;
                float spacingY = 6.0f;

                if (!m_categories.empty()) {
                    std::unordered_map<std::wstring, std::vector<size_t>> categoryToTagIndices;
                    std::vector<size_t> uncategorizedIndices;

                    for (size_t idx = 0; idx < m_filteredTags.size(); ++idx) {
                        const auto& tag = m_filteredTags[idx];
                        auto catIt = m_tagToCategory.find(tag);
                        if (catIt != m_tagToCategory.end() && !catIt->second.empty()) {
                            categoryToTagIndices[catIt->second].push_back(idx);
                        } else {
                            uncategorizedIndices.push_back(idx);
                        }
                    }

                    auto processClickIdx = [&](size_t idx) -> bool {
                        std::wstring tagStr = m_filteredTags[idx];
                        if (tagStr.length() > 30) {
                            tagStr = tagStr.substr(0, 27) + L"...";
                        }
                        float textW = m_app->m_renderer.MeasureText(tagStr, 14.0f).x;
                        float pillW = textW + 22.0f;

                        if (availX + pillW > listRect.right - 8.0f) {
                            availX = listRect.left + 8.0f;
                            availY += availH + spacingY;
                        }

                        RectF pillRect = RectF::FromXYWH(availX, availY, pillW, availH);
                        if (pillRect.Contains(x, y)) {
                            if (mods.ctrl) {
                                m_selectedAvailableIndices[idx] = !m_selectedAvailableIndices[idx];
                            } else {
                                // Add directly
                                std::wstring tagToCopy = m_filteredTags[idx];
                                if (std::find(m_imageTags.begin(), m_imageTags.end(), tagToCopy) == m_imageTags.end()) {
                                    m_imageTags.push_back(tagToCopy);
                                }
                                m_searchText.clear();
                                m_filteredTags = m_availableTags;
                                m_selectedAvailableIndices.assign(m_filteredTags.size(), false);
                                m_leftScroll = 0.0f;
                                m_activeInputType = 1;
                            }
                            return true;
                        }

                        availX += pillW + spacingX;
                        return false;
                    };

                    for (const auto& catName : m_categories) {
                        auto tagIt = categoryToTagIndices.find(catName);
                        if (tagIt == categoryToTagIndices.end() || tagIt->second.empty()) {
                            continue;
                        }

                        if (availX > listRect.left + 8.0f) {
                            availX = listRect.left + 8.0f;
                            availY += availH + spacingY;
                        }

                        availY += 24.0f + spacingY;
                        availX = listRect.left + 8.0f;

                        for (size_t idx : tagIt->second) {
                            if (processClickIdx(idx)) {
                                if (m_app) m_app->m_window.RequestRedraw();
                                return true;
                            }
                        }

                        if (availX > listRect.left + 8.0f) {
                            availX = listRect.left + 8.0f;
                            availY += availH + spacingY;
                        }
                    }

                    if (!uncategorizedIndices.empty()) {
                        if (availX > listRect.left + 8.0f) {
                            availX = listRect.left + 8.0f;
                            availY += availH + spacingY;
                        }

                        availY += 24.0f + spacingY;
                        availX = listRect.left + 8.0f;

                        for (size_t idx : uncategorizedIndices) {
                            if (processClickIdx(idx)) {
                                if (m_app) m_app->m_window.RequestRedraw();
                                return true;
                            }
                        }

                        if (availX > listRect.left + 8.0f) {
                            availX = listRect.left + 8.0f;
                            availY += availH + spacingY;
                        }
                    }
                } else {
                    for (int i = 0; i < static_cast<int>(m_filteredTags.size()); ++i) {
                        std::wstring tagStr = m_filteredTags[i];
                        if (tagStr.length() > 30) {
                            tagStr = tagStr.substr(0, 27) + L"...";
                        }
                        float textW = m_app->m_renderer.MeasureText(tagStr, 14.0f).x;
                        float pillW = textW + 22.0f;

                        if (availX + pillW > listRect.right - 8.0f) {
                            availX = listRect.left + 8.0f;
                            availY += availH + spacingY;
                        }

                        RectF pillRect = RectF::FromXYWH(availX, availY, pillW, availH);
                        if (pillRect.Contains(x, y)) {
                            if (mods.ctrl) {
                                m_selectedAvailableIndices[i] = !m_selectedAvailableIndices[i];
                            } else {
                                std::wstring tagToCopy = m_filteredTags[i];
                                if (std::find(m_imageTags.begin(), m_imageTags.end(), tagToCopy) == m_imageTags.end()) {
                                    m_imageTags.push_back(tagToCopy);
                                }
                                m_searchText.clear();
                                m_filteredTags = m_availableTags;
                                m_selectedAvailableIndices.assign(m_filteredTags.size(), false);
                                m_leftScroll = 0.0f;
                                m_activeInputType = 1;
                            }
                            if (m_app) m_app->m_window.RequestRedraw();
                            return true;
                        }

                        availX += pillW + spacingX;
                    }
                }
            }
        }

        // Copy Arrow [ > ] Button - only if hasAvailable
        if (hasAvailable) {
            RectF midBtnRect = RectF::FromXYWH(leftPane.right + 36.0f, leftPane.top + (leftPane.Height() - 50.0f) / 2.0f, 36.0f, 50.0f);
            if (midBtnRect.Contains(x, y)) {
                for (size_t i = 0; i < m_filteredTags.size(); ++i) {
                    if (m_selectedAvailableIndices[i]) {
                        std::wstring tagToCopy = m_filteredTags[i];
                        if (std::find(m_imageTags.begin(), m_imageTags.end(), tagToCopy) == m_imageTags.end()) {
                            m_imageTags.push_back(tagToCopy);
                        }
                    }
                }
                m_searchText.clear();
                m_filteredTags = m_availableTags;
                m_selectedAvailableIndices.assign(m_filteredTags.size(), false);
                m_leftScroll = 0.0f;
                m_activeInputType = 1;
                if (m_app) m_app->m_window.RequestRedraw();
                return true;
            }
        }

        // Manual tag text focus
        RectF addRect = RectF::FromXYWH(rightPane.left, rightPane.top, rightPane.Width() - 42.0f, 36.0f);
        if (addRect.Contains(x, y)) {
            m_activeInputType = 6;
            if (m_app) m_app->m_window.RequestRedraw();
            return true;
        }

        // Add Tag (+) click
        RectF plusRect = RectF::FromXYWH(rightPane.right - 36.0f, rightPane.top, 36.0f, 36.0f);
        if (plusRect.Contains(x, y)) {
            if (!m_manualTagText.empty()) {
                if (std::find(m_imageTags.begin(), m_imageTags.end(), m_manualTagText) == m_imageTags.end()) {
                    m_imageTags.push_back(m_manualTagText);
                }
                m_manualTagText.clear();
            }
            m_activeInputType = 6; // Keep focus on manual tag input!
            if (m_app) m_app->m_window.RequestRedraw();
            return true;
        }

        // Tags Grid clicks
        RectF gridRect = RectF::FromXYWH(rightPane.left, rightPane.top + 46.0f, rightPane.Width(), rightPane.Height() - 46.0f);
        if (gridRect.Contains(x, y)) {
            float tagX = gridRect.left + 8.0f;
            float tagY = gridRect.top + 8.0f - m_rightScroll;
            float tagH = 28.0f;
            float spacingX = 6.0f;
            float spacingY = 6.0f;

            for (int i = 0; i < static_cast<int>(m_imageTags.size()); ++i) {
                std::wstring tagStr = m_imageTags[i];
                if (tagStr.length() > 30) {
                    tagStr = tagStr.substr(0, 27) + L"...";
                }
                float textW = m_app->m_renderer.MeasureText(tagStr, 14.0f).x;
                float pillW = textW + 36.0f;

                if (tagX + pillW > gridRect.right - 8.0f) {
                    tagX = gridRect.left + 8.0f;
                    tagY += tagH + spacingY;
                }

                RectF pillRect = RectF::FromXYWH(tagX, tagY, pillW, tagH);
                if (pillRect.Contains(x, y)) {
                    RectF xRect = RectF::FromXYWH(pillRect.right - 22.0f, pillRect.top, 18.0f, pillRect.Height());
                    if (xRect.Contains(x, y)) {
                        // Delete tag
                        m_imageTags.erase(m_imageTags.begin() + i);
                    } else {
                        // Focus edit this tag
                        m_editingTagIdx = i;
                        m_editingTagText = m_imageTags[i];
                        m_activeInputType = 2;
                    }
                    if (m_app) m_app->m_window.RequestRedraw();
                    return true;
                }

                tagX += pillW + spacingX;
            }
        }

        // Star rating clicks
        if (!m_multiImageMode) {
            float startX = hasAvailable ? leftPane.left : rightPane.left;
            RectF starsTextRect = RectF::FromXYWH(startX, rightPane.bottom + 15.0f, 120.0f, 30.0f);
            float starX = starsTextRect.right + 12.0f;
            float starY = starsTextRect.top;
            float starW = 28.0f;

            for (int i = 1; i <= 5; ++i) {
                RectF starRect = RectF::FromXYWH(starX, starY, starW, 30.0f);
                if (starRect.Contains(x, y)) {
                    m_rating = i;
                    if (m_app) m_app->m_window.RequestRedraw();
                    return true;
                }
                starX += starW + 2.0f;
            }
        }
    } else {
        // --- METADATA TAB CLICKS ---
        std::wstring dimStr, sizeStr, modStr;
        GetFileStats(m_filePath, dimStr, sizeStr, modStr, m_app ? m_app->m_tagManager.GetWicFactory() : nullptr);
        float infoY = bounds.top + 10.0f + 3.0f * 28.0f;
        if (!m_deviceMake.empty() || !m_deviceModel.empty()) infoY += 28.0f;
        float fieldsY = infoY + 28.0f;

        // Title box focus
        RectF titleRect = RectF::FromXYWH(bounds.left + 20.0f, fieldsY + 20.0f, bounds.Width() - 40.0f, 40.0f);
        if (titleRect.Contains(x, y)) {
            m_activeInputType = 3;
            if (m_app) m_app->m_window.RequestRedraw();
            return true;
        }

        // Creator focus
        RectF creatorRect = RectF::FromXYWH(bounds.left + 20.0f, fieldsY + 85.0f, bounds.Width() - 40.0f, 40.0f);
        if (creatorRect.Contains(x, y)) {
            m_activeInputType = 4;
            if (m_app) m_app->m_window.RequestRedraw();
            return true;
        }

        // Description focus
        RectF descRect = RectF::FromXYWH(bounds.left + 20.0f, fieldsY + 150.0f, bounds.Width() - 40.0f, 90.0f);
        if (descRect.Contains(x, y)) {
            m_activeInputType = 5;
            if (m_app) m_app->m_window.RequestRedraw();
            return true;
        }
    }

    if (m_app) m_app->m_window.RequestRedraw();
    return true;
}

bool TaggingModal::OnMouseUp(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (!m_visible) return false;
    if (m_leftScrollCaptured || m_rightScrollCaptured) {
        m_leftScrollCaptured = false;
        m_rightScrollCaptured = false;
        if (m_app) m_app->m_window.RequestRedraw();
        return true;
    }
    return true;
}

bool TaggingModal::OnMouseWheel(float delta, float x, float y, KeyModifiers mods) {
    if (!m_visible) return false;
    const float scale=GetLayoutScale(); x/=scale; y/=scale;

    RectF bounds = GetBounds();
    bool hasAvailable = !m_availableTags.empty();
    RectF leftPane = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 105.0f, 456.0f, bounds.Height() - 195.0f);
    RectF rightPane;
    if (hasAvailable) {
        rightPane = RectF::FromXYWH(bounds.left + 594.0f, bounds.top + 105.0f, 528.0f, bounds.Height() - 195.0f);
    } else {
        rightPane = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 105.0f, bounds.Width() - 60.0f, bounds.Height() - 195.0f);
    }

    if (m_currentTab == 0) {
        // Scroll left panel
        if (hasAvailable) {
            RectF listRect = RectF::FromXYWH(leftPane.left, leftPane.top + 46.0f, leftPane.Width(), leftPane.Height() - 46.0f);
            if (listRect.Contains(x, y)) {
                m_leftScroll -= delta * 15.0f; // Scroll speed multiplier
                if (m_app) m_app->m_window.RequestRedraw();
                return true;
            }
        }

        // Scroll right panel (Tags grid)
        RectF gridRect = RectF::FromXYWH(rightPane.left, rightPane.top + 46.0f, rightPane.Width(), rightPane.Height() - 46.0f);
        if (gridRect.Contains(x, y)) {
            m_rightScroll -= delta * 15.0f;
            if (m_app) m_app->m_window.RequestRedraw();
            return true;
        }
    }

    return true;
}

bool TaggingModal::OnKeyDown(uint32_t vk, KeyModifiers mods) {
    if (!m_visible) return false;

    if (vk == VK_ESCAPE) {
        if (m_activeInputType != 0) {
            // Dismiss focused text entry
            m_activeInputType = 0;
            m_editingTagIdx = -1;
            if (m_app) m_app->m_window.RequestRedraw();
        } else {
            Hide();
        }
        return true;
    }

    if (vk == VK_RETURN) {
        if (m_activeInputType == 1) {
            // Search field: do nothing or dismiss
            m_activeInputType = 0;
        } else if (m_activeInputType == 2 && m_editingTagIdx != -1) {
            // Renaming tag: commit
            if (!m_editingTagText.empty()) {
                m_imageTags[m_editingTagIdx] = m_editingTagText;
            }
            m_editingTagIdx = -1;
            m_activeInputType = 0;
        } else if (m_activeInputType == 6) {
            // Manual tag addition: commit
            if (!m_manualTagText.empty()) {
                if (std::find(m_imageTags.begin(), m_imageTags.end(), m_manualTagText) == m_imageTags.end()) {
                    m_imageTags.push_back(m_manualTagText);
                }
                m_manualTagText.clear();
            }
            m_activeInputType = 6; // Keep focus on manual tag input!
        } else if (m_activeInputType == 3 || m_activeInputType == 4 || m_activeInputType == 5) {
            // Metadata fields: dismiss focus
            m_activeInputType = 0;
        } else {
            // No focused input: save changes and close!
            SaveChanges();
        }
        if (m_app) m_app->m_window.RequestRedraw();
        return true;
    }

    return true;
}

void TaggingModal::OnChar(wchar_t ch) {
    if (!m_visible) return;
    if (m_activeInputType == 0) return;

    std::wstring* targetStr = nullptr;
    switch (m_activeInputType) {
        case 1: targetStr = &m_searchText; break;
        case 2: targetStr = &m_editingTagText; break;
        case 3: targetStr = &m_title; break;
        case 4: targetStr = &m_creator; break;
        case 5: targetStr = &m_description; break;
        case 6: targetStr = &m_manualTagText; break;
        default: return;
    }

    if (ch == L'\b') {
        // Backspace
        if (!targetStr->empty()) {
            targetStr->pop_back();
        }
    } else if (ch >= 32) {
        // Append printable characters (limit lengths)
        size_t limit = 100;
        if (m_activeInputType == 2 || m_activeInputType == 6) limit = 30; // tags limit
        if (m_activeInputType == 5) limit = 500; // description limit

        if (targetStr->length() < limit) {
            targetStr->push_back(ch);
        }
    }

    // If search text changed, filter available tags list
    if (m_activeInputType == 1) {
        m_filteredTags.clear();
        std::wstring lowerQuery = m_searchText;
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::towlower);

        for (const auto& tag : m_availableTags) {
            std::wstring lowerTag = tag;
            std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::towlower);
            if (lowerTag.find(lowerQuery) != std::wstring::npos) {
                m_filteredTags.push_back(tag);
            }
        }
        m_selectedAvailableIndices.assign(m_filteredTags.size(), false);
        m_leftScroll = 0.0f;
    }

    if (m_app) m_app->m_window.RequestRedraw();
}

} // namespace kv
