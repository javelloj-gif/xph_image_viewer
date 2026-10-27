#include "common/pch.h"
#include "board_search_manager.h"
#include "board_page.h"
#include "board_layout_helper.h"
#include "core/app.h"
#include "loader/image_loader.h"
#include <random>
#include <chrono>
#include <algorithm>
#include <fstream>

namespace kv {

static std::wstring GetCurrentTypingTag(const std::wstring& text, size_t& outLastCommaPos) {
    size_t pos = text.find_last_of(L',');
    if (pos == std::wstring::npos) {
        outLastCommaPos = std::wstring::npos;
        std::wstring sub = text;
        size_t start = sub.find_first_not_of(L" \t");
        if (start != std::wstring::npos) sub = sub.substr(start);
        return sub;
    } else {
        outLastCommaPos = pos;
        std::wstring sub = text.substr(pos + 1);
        size_t start = sub.find_first_not_of(L" \t");
        if (start != std::wstring::npos) sub = sub.substr(start);
        return sub;
    }
}

BoardSearchManager::BoardSearchManager(BoardPage* page) : m_page(page) {}

void BoardSearchManager::Initialize() {
    m_searchModeGroup = true;
    m_hoveredToggleBtn = false;
    m_searchVisible = false;
    m_searchText.clear();
}

void BoardSearchManager::Update(double dt) {
    // No hay animaciones temporales complejas de búsqueda en este momento
}

void BoardSearchManager::SetSearchVisible(bool visible) {
    m_searchVisible = visible;
}

RectF BoardSearchManager::GetSearchModalBounds() const {
    auto screenSize = m_page->m_app->m_renderer.GetTargetSize().ToFloat();
    float boxW = std::min(600.0f,screenSize.width-16);
    float boxH = std::min(360.0f,screenSize.height-16);
    return RectF::FromXYWH(
        (screenSize.width - boxW) / 2.0f,
        (screenSize.height - boxH) / 2.0f,
        boxW,
        boxH
    );
}

RectF BoardSearchManager::GetSearchToggleRect(const SizeF& screenSize) const {
    float btnW = 200.0f;
    float btnH = 35.0f;
    return RectF::FromXYWH(
        screenSize.width - btnW - 20.0f,
        screenSize.height - btnH - 20.0f,
        btnW,
        btnH
    );
}

void BoardSearchManager::UpdateFilteredSuggestions() {
    m_filteredSuggestions.clear();
    size_t lastComma = std::wstring::npos;
    std::wstring typing = GetCurrentTypingTag(m_searchText, lastComma);
    
    bool isExcluding = false;
    if (!typing.empty() && typing[0] == L'-') {
        isExcluding = true;
        typing = typing.substr(1);
    }
    
    size_t last = typing.find_last_not_of(L" \t");
    if (last != std::wstring::npos) {
        typing = typing.substr(0, last + 1);
    }
    
    if (typing.empty()) {
        m_filteredSuggestions = m_suggestions;
    } else {
        std::wstring typingLower = typing;
        std::transform(typingLower.begin(), typingLower.end(), typingLower.begin(), ::towlower);
        for (const auto& tag : m_suggestions) {
            std::wstring tagLower = tag;
            std::transform(tagLower.begin(), tagLower.end(), tagLower.begin(), ::towlower);
            if (tagLower.find(typingLower) == 0) {
                m_filteredSuggestions.push_back(tag);
            }
        }
    }
}

void BoardSearchManager::SelectSuggestion(const std::wstring& suggestion) {
    size_t lastComma = std::wstring::npos;
    std::wstring typing = GetCurrentTypingTag(m_searchText, lastComma);
    bool isExcluding = (!typing.empty() && typing[0] == L'-');
    
    std::wstring prefix = L"";
    if (lastComma != std::wstring::npos) {
        prefix = m_searchText.substr(0, lastComma + 1) + L" ";
    }
    
    if (isExcluding) {
        m_searchText = prefix + L"-" + suggestion + L", ";
    } else {
        m_searchText = prefix + suggestion + L", ";
    }
    
    UpdateFilteredSuggestions();
    m_hoveredSuggestionIdx = -1;
}

void BoardSearchManager::Draw(Renderer& renderer) {
    if (!m_searchVisible) return;
    
    auto screenSize = renderer.GetTargetSize().ToFloat();
    RectF bounds = GetSearchModalBounds();
    
    renderer.FillRect(RectF::FromXYWH(0, 0, screenSize.width, screenSize.height), Color::Black(0.65f));
    
    Color panelBg = Color::FromRGBA(15, 15, 20, 245);
    Color panelBorder = Color::FromRGBA(0, 240, 255, 60);
    renderer.FillRoundedRect(bounds, 8.0f, 8.0f, panelBg);
    renderer.DrawRoundedRect(bounds, 8.0f, 8.0f, panelBorder, 1.5f);
    
    RectF titleRect = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 20.0f, bounds.Width() - 60.0f, 30.0f);
    renderer.DrawText(
        _(L"Search by tags"),
        titleRect,
        Color::White(),
        22.0f,
        TextAlign::Left,
        TextVAlign::Center
    );
    
    RectF searchRect = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 60.0f, bounds.Width() - 60.0f, 36.0f);
    Color searchBorder = Color::FromRGBA(0, 240, 255);
    renderer.FillRoundedRect(searchRect, 4.0f, 4.0f, Color::FromRGBA(20, 20, 25, 200));
    renderer.DrawRoundedRect(searchRect, 4.0f, 4.0f, searchBorder, 1.0f);
    
    if (m_searchText.empty()) {
        renderer.DrawText(
            _(L"Comma-separated tags (e.g. tag1, -tag2)..."),
            searchRect.Inset(8.0f),
            Color::FromRGBA(120, 120, 120),
            15.0f,
            TextAlign::Left,
            TextVAlign::Center
        );
    } else {
        renderer.DrawText(
            m_searchText,
            searchRect.Inset(8.0f),
            Color::White(),
            15.0f,
            TextAlign::Left,
            TextVAlign::Center
        );
    }
    
    static auto lastCaretSearch = std::chrono::steady_clock::now();
    static bool caretSearchVisible = true;
    if (std::chrono::steady_clock::now() - lastCaretSearch > std::chrono::milliseconds(500)) {
        caretSearchVisible = !caretSearchVisible;
        lastCaretSearch = std::chrono::steady_clock::now();
    }
    if (caretSearchVisible) {
        float textW = renderer.MeasureText(m_searchText, 15.0f).x;
        float caretX = searchRect.left + 8.0f + textW;
        renderer.DrawLine(
            { caretX, searchRect.top + 6.0f },
            { caretX, searchRect.bottom - 6.0f },
            Color::FromRGBA(0, 240, 255),
            1.5f
        );
    }
    
    RectF suggestionsArea = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 110.0f, bounds.Width() - 60.0f, 180.0f);
    renderer.FillRoundedRect(suggestionsArea, 4.0f, 4.0f, Color::FromRGBA(20, 20, 25, 150));
    renderer.DrawRoundedRect(suggestionsArea, 4.0f, 4.0f, Color::FromRGBA(255, 255, 255, 15), 1.0f);
    
    renderer.PushClip(suggestionsArea);
    
    float sX = suggestionsArea.left + 8.0f;
    float sY = suggestionsArea.top + 8.0f;
    float pillH = 28.0f;
    float spacingX = 6.0f;
    float spacingY = 6.0f;
    
    for (size_t i = 0; i < m_filteredSuggestions.size(); ++i) {
        std::wstring tagStr = m_filteredSuggestions[i];
        float textW = renderer.MeasureText(tagStr, 14.0f).x;
        float pillW = textW + 22.0f;
        
        if (sX + pillW > suggestionsArea.right - 8.0f) {
            sX = suggestionsArea.left + 8.0f;
            sY += pillH + spacingY;
        }
        
        RectF pillRect = RectF::FromXYWH(sX, sY, pillW, pillH);
        
        if (pillRect.bottom > suggestionsArea.top && pillRect.top < suggestionsArea.bottom) {
            bool hovered = (m_hoveredSuggestionIdx == static_cast<int>(i));
            Color pillBg = hovered ? Color::FromRGBA(255, 255, 255, 15) : Color::FromRGBA(255, 255, 255, 10);
            Color pillBorder = hovered ? Color::FromRGBA(255, 255, 255, 35) : Color::FromRGBA(255, 255, 255, 20);
            
            renderer.FillRoundedRect(pillRect, 4.0f, 4.0f, pillBg);
            renderer.DrawRoundedRect(pillRect, 4.0f, 4.0f, pillBorder, 1.0f);
            
            RectF textRect = RectF::FromXYWH(pillRect.left + 11.0f, pillRect.top, textW, pillRect.Height());
            renderer.DrawText(tagStr, textRect, Color::White(0.9f), 14.0f, TextAlign::Left, TextVAlign::Center);
        }
        
        sX += pillW + spacingX;
    }
    
    renderer.PopClip();
    
    RectF closeRect = RectF::FromXYWH(bounds.right - 150.0f, bounds.bottom - 52.0f, 120.0f, 32.0f);
    RectF searchBtnRect = RectF::FromXYWH(bounds.right - 285.0f, bounds.bottom - 52.0f, 120.0f, 32.0f);
    
    Color closeBg = (m_hoveredSearchButton == 2) ? Color::FromRGBA(60, 60, 65) : Color::FromRGBA(35, 35, 40);
    renderer.FillRoundedRect(closeRect, 4.0f, 4.0f, closeBg);
    renderer.DrawRoundedRect(closeRect, 4.0f, 4.0f, Color::White(0.15f), 1.0f);
    renderer.DrawText(_(L"Close"), closeRect, Color::White(0.9f), 15.0f, TextAlign::Center, TextVAlign::Center);
    
    Color searchBg = (m_hoveredSearchButton == 1) ? Color::FromRGBA(255, 55, 95) : Color::FromRGBA(220, 40, 75);
    renderer.FillRoundedRect(searchBtnRect, 4.0f, 4.0f, searchBg);
    renderer.DrawRoundedRect(searchBtnRect, 4.0f, 4.0f, Color::White(0.2f), 1.0f);
    renderer.DrawText(_(L"Search"), searchBtnRect, Color::White(), 15.0f, TextAlign::Center, TextVAlign::Center);
}

bool BoardSearchManager::OnMouseMove(float x, float y, KeyModifiers mods) {
    if (m_searchVisible) {
        RectF bounds = GetSearchModalBounds();
        m_hoveredSuggestionIdx = -1;
        m_hoveredSearchButton = 0;
        
        if (bounds.Contains(x, y)) {
            RectF searchBtnRect = RectF::FromXYWH(bounds.right - 285.0f, bounds.bottom - 52.0f, 120.0f, 32.0f);
            if (searchBtnRect.Contains(x, y)) {
                m_hoveredSearchButton = 1;
            }
            
            RectF closeBtnRect = RectF::FromXYWH(bounds.right - 150.0f, bounds.bottom - 52.0f, 120.0f, 32.0f);
            if (closeBtnRect.Contains(x, y)) {
                m_hoveredSearchButton = 2;
            }
            
            RectF suggestionsArea = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 110.0f, bounds.Width() - 60.0f, 180.0f);
            if (suggestionsArea.Contains(x, y)) {
                float sX = suggestionsArea.left + 8.0f;
                float sY = suggestionsArea.top + 8.0f;
                float pillH = 28.0f;
                float spacingX = 6.0f;
                float spacingY = 6.0f;
                
                for (size_t i = 0; i < m_filteredSuggestions.size(); ++i) {
                    std::wstring tagStr = m_filteredSuggestions[i];
                    float textW = m_page->m_app->m_renderer.MeasureText(tagStr, 14.0f).x;
                    float pillW = textW + 22.0f;
                    
                    if (sX + pillW > suggestionsArea.right - 8.0f) {
                        sX = suggestionsArea.left + 8.0f;
                        sY += pillH + spacingY;
                    }
                    
                    RectF pillRect = RectF::FromXYWH(sX, sY, pillW, pillH);
                    if (pillRect.Contains(x, y)) {
                        m_hoveredSuggestionIdx = static_cast<int>(i);
                        break;
                    }
                    
                    sX += pillW + spacingX;
                }
            }
        }
        m_page->m_app->m_window.RequestRedraw();
        return true;
    }

    // Comprobar hover en toggle
    auto screenSize = m_page->m_app->m_renderer.GetTargetSize().ToFloat();
    RectF toggleRect = GetSearchToggleRect(screenSize);
    bool isOverToggle = toggleRect.Contains(x, y);
    if (isOverToggle != m_hoveredToggleBtn) {
        m_hoveredToggleBtn = isOverToggle;
        m_page->m_app->m_window.RequestRedraw();
    }

    return false;
}

bool BoardSearchManager::OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods) {
    if (m_searchVisible) {
        RectF bounds = GetSearchModalBounds();
        if (btn == MouseButton::Left) {
            if (!bounds.Contains(x, y)) {
                m_searchVisible = false;
                m_page->m_app->m_window.RequestRedraw();
                return true;
            }
            
            RectF searchBtnRect = RectF::FromXYWH(bounds.right - 285.0f, bounds.bottom - 52.0f, 120.0f, 32.0f);
            if (searchBtnRect.Contains(x, y)) {
                PerformTagQueryAndInsert();
                m_searchVisible = false;
                m_page->m_app->m_window.RequestRedraw();
                return true;
            }
            
            RectF closeBtnRect = RectF::FromXYWH(bounds.right - 150.0f, bounds.bottom - 52.0f, 120.0f, 32.0f);
            if (closeBtnRect.Contains(x, y)) {
                m_searchVisible = false;
                m_page->m_app->m_window.RequestRedraw();
                return true;
            }
            
            RectF suggestionsArea = RectF::FromXYWH(bounds.left + 30.0f, bounds.top + 110.0f, bounds.Width() - 60.0f, 180.0f);
            if (suggestionsArea.Contains(x, y)) {
                float sX = suggestionsArea.left + 8.0f;
                float sY = suggestionsArea.top + 8.0f;
                float pillH = 28.0f;
                float spacingX = 6.0f;
                float spacingY = 6.0f;
                
                for (size_t i = 0; i < m_filteredSuggestions.size(); ++i) {
                    std::wstring tagStr = m_filteredSuggestions[i];
                    float textW = m_page->m_app->m_renderer.MeasureText(tagStr, 14.0f).x;
                    float pillW = textW + 22.0f;
                    
                    if (sX + pillW > suggestionsArea.right - 8.0f) {
                        sX = suggestionsArea.left + 8.0f;
                        sY += pillH + spacingY;
                    }
                    
                    RectF pillRect = RectF::FromXYWH(sX, sY, pillW, pillH);
                    if (pillRect.Contains(x, y)) {
                        SelectSuggestion(tagStr);
                        m_page->m_app->m_window.RequestRedraw();
                        return true;
                    }
                    
                    sX += pillW + spacingX;
                }
            }
        }
        return true;
    }
    return false;
}

bool BoardSearchManager::OnKeyDown(uint32_t vk, KeyModifiers mods) {
    if (m_searchVisible) {
        if (vk == VK_ESCAPE) {
            m_searchVisible = false;
            m_page->m_app->m_window.RequestRedraw();
            return true;
        }
        if (vk == VK_RETURN) {
            PerformTagQueryAndInsert();
            m_searchVisible = false;
            m_page->m_app->m_window.RequestRedraw();
            return true;
        }
        return true;
    }
    return false;
}

bool BoardSearchManager::OnChar(wchar_t ch) {
    if (m_searchVisible) {
        if (ch == L'\r' || ch == L'\n') {
            return true;
        }
        if (ch == L'\b') {
            if (!m_searchText.empty()) {
                m_searchText.pop_back();
                UpdateFilteredSuggestions();
                m_page->m_app->m_window.RequestRedraw();
            }
            return true;
        }
        if (ch == 27) {
            return true;
        }
        if (ch >= 32) {
            m_searchText += ch;
            UpdateFilteredSuggestions();
            m_page->m_app->m_window.RequestRedraw();
        }
        return true;
    }
    return false;
}

void BoardSearchManager::LoadSuggestionsFromDirectory(const std::filesystem::path& activeDir) {
    m_suggestions.clear();
    m_filteredSuggestions.clear();
    
    std::filesystem::path jsonPath = activeDir / "xmpTags.json";
    if (!std::filesystem::exists(jsonPath)) {
        return;
    }
    
    std::ifstream file(jsonPath, std::ios::binary);
    if (!file) return;
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    std::wstring wcontent;
    int len = MultiByteToWideChar(CP_UTF8, 0, content.c_str(), static_cast<int>(content.size()), nullptr, 0);
    if (len > 0) {
        wcontent.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, content.c_str(), static_cast<int>(content.size()), &wcontent[0], len);
    } else {
        return;
    }
    
    int arrayDepth = 0;
    size_t i = 0;
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
            i++;
            while (i < wcontent.size() && wcontent[i] != L'"') {
                if (wcontent[i] == L'\\' && i + 1 < wcontent.size()) {
                    str += wcontent[i + 1];
                    i += 2;
                } else {
                    str += wcontent[i];
                    i++;
                }
            }
            if (i < wcontent.size()) i++;
            
            if (arrayDepth > 0 && !str.empty()) {
                if (std::find(m_suggestions.begin(), m_suggestions.end(), str) == m_suggestions.end()) {
                    m_suggestions.push_back(str);
                }
            }
        } else {
            i++;
        }
    }
    
    std::sort(m_suggestions.begin(), m_suggestions.end());
    m_filteredSuggestions = m_suggestions;
}

void BoardSearchManager::PerformTagQueryAndInsert() {
    LogInfo(L"PerformTagQueryAndInsert started.");
    LogInfo(std::format(L"Search Text: '{}'", m_searchText));

    std::vector<std::wstring> posTags;
    std::vector<std::wstring> negTags;
    
    std::wstring term = L"";
    for (wchar_t ch : m_searchText) {
        if (ch == L',') {
            size_t start = term.find_first_not_of(L" \t");
            if (start != std::wstring::npos) {
                term = term.substr(start);
                size_t end = term.find_last_not_of(L" \t");
                if (end != std::wstring::npos) {
                    term = term.substr(0, end + 1);
                }
                if (!term.empty()) {
                    if (term[0] == L'-') {
                        std::wstring sub = term.substr(1);
                        if (!sub.empty()) negTags.push_back(sub);
                    } else {
                        posTags.push_back(term);
                    }
                }
            }
            term = L"";
        } else {
            term += ch;
        }
    }
    
    size_t start = term.find_first_not_of(L" \t");
    if (start != std::wstring::npos) {
        term = term.substr(start);
        size_t end = term.find_last_not_of(L" \t");
        if (end != std::wstring::npos) {
            term = term.substr(0, end + 1);
        }
        if (!term.empty()) {
            if (term[0] == L'-') {
                std::wstring sub = term.substr(1);
                if (!sub.empty()) negTags.push_back(sub);
            } else {
                posTags.push_back(term);
            }
        }
    }
    
    LogInfo(std::format(L"Parsed posTags count: {}, negTags count: {}", posTags.size(), negTags.size()));

    if (posTags.empty() && negTags.empty()) {
        LogInfo(L"No tags parsed, exiting PerformTagQueryAndInsert.");
        return;
    }
    
    std::filesystem::path activeDir = m_page->GetActiveDirectory();
    LogInfo(std::format(L"Active Directory: '{}'", activeDir.wstring()));
    std::vector<std::filesystem::path> matchingFiles;
    
    try {
        if (std::filesystem::exists(activeDir) && std::filesystem::is_directory(activeDir)) {
            for (const auto& entry : std::filesystem::directory_iterator(activeDir)) {
                if (entry.is_regular_file() && ImageLoader::IsSupported(entry.path().wstring())) {
                    std::wstring filePath = entry.path().wstring();
                    std::vector<std::wstring> fileTags;
                    if (m_page->m_app->m_tagManager.ReadTags(filePath, fileTags)) {
                        bool matchesPos = true;
                        for (const auto& posTag : posTags) {
                            bool tagFound = false;
                            for (const auto& ft : fileTags) {
                                if (_wcsicmp(posTag.c_str(), ft.c_str()) == 0) {
                                    tagFound = true;
                                    break;
                                }
                            }
                            if (!tagFound) {
                                matchesPos = false;
                                break;
                            }
                        }
                        
                        if (!matchesPos) continue;
                        
                        bool matchesNeg = true;
                        for (const auto& negTag : negTags) {
                            for (const auto& ft : fileTags) {
                                if (_wcsicmp(negTag.c_str(), ft.c_str()) == 0) {
                                    matchesNeg = false;
                                    break;
                                }
                            }
                            if (!matchesNeg) break;
                        }
                        
                        if (matchesPos && matchesNeg) {
                            matchingFiles.push_back(entry.path());
                        }
                    }
                }
            }
        }
    } catch (...) {
        LogError(L"Exception caught during activeDir directory iteration.");
    }
    
    LogInfo(std::format(L"matchingFiles size: {}", matchingFiles.size()));

    if (matchingFiles.empty()) {
        LogInfo(L"No matching files found. Requesting redraw and exiting.");
        m_page->m_app->m_window.RequestRedraw();
        return;
    }

    LogInfo(std::format(L"Search Mode: {}", m_searchModeGroup ? L"GROUP" : L"FILTER"));

    if (m_searchModeGroup) {
        // --- COMPORTAMIENTO B (AGRUPAR Y ENFOCAR EN EL CENTROIDE DE LOS RESULTADOS) ---
        auto targetSize = m_page->m_app->m_renderer.GetTargetSize().ToFloat();
        PointF screenCenter = { targetSize.width * 0.5f, targetSize.height * 0.5f };
        PointF worldCenter = m_page->ScreenToWorld(screenCenter);
        LogInfo(std::format(L"screenCenter: ({}, {}), worldCenter: ({}, {})", screenCenter.x, screenCenter.y, worldCenter.x, worldCenter.y));

        // 1. Encontrar qué ítems ya cargados en el tablero coinciden
        std::vector<size_t> existingMatchingIndices;
        for (size_t i = 0; i < m_page->m_items.size(); ++i) {
            auto& item = m_page->m_items[i];
            if (item.type == BoardItemType::Image && !item.filePath.empty()) {
                bool matches = false;
                for (const auto& path : matchingFiles) {
                    if (_wcsicmp(item.filePath.c_str(), path.wstring().c_str()) == 0) {
                        matches = true;
                        break;
                    }
                }
                if (matches) {
                    existingMatchingIndices.push_back(i);
                }
            }
        }
        LogInfo(std::format(L"existingMatchingIndices size: {}", existingMatchingIndices.size()));

        // 2. Mover temporalmente estas imágenes existentes fuera de rango para que FindSpiralPosition no colisione con su posición previa
        for (size_t i = 0; i < existingMatchingIndices.size(); ++i) {
            size_t idx = existingMatchingIndices[i];
            m_page->m_items[idx].position = { -999999.0f, -999999.0f };
        }

        // 3. Crear el set de colisiones del lote actual y registrar posiciones finales
        std::vector<BoardItem> addedItems;
        std::vector<PointF> finalPositions;
        finalPositions.reserve(existingMatchingIndices.size() + matchingFiles.size());
        float itemW = 400.0f;
        float itemH = 300.0f;

        // 4. Reposicionar las imágenes coincidentes existentes
        for (size_t i = 0; i < existingMatchingIndices.size(); ++i) {
            size_t idx = existingMatchingIndices[i];
            auto& item = m_page->m_items[idx];
            PointF pos = m_page->m_layoutHelper->FindSpiralPosition(worldCenter, item.width, item.height, 40.0f, addedItems, true);
            item.position = pos;
            finalPositions.push_back(pos);
            addedItems.push_back(item);
            LogInfo(std::format(L"  Repositioned existing item {} ('{}') to ({}, {})", idx, item.filePath, pos.x, pos.y));
        }

        // 5. Encontrar qué archivos coincidentes son nuevos
        std::vector<std::filesystem::path> newFiles;
        for (const auto& path : matchingFiles) {
            bool alreadyExists = false;
            std::wstring pathStr = path.wstring();
            for (const auto& item : m_page->m_items) {
                if (item.type == BoardItemType::Image && _wcsicmp(item.filePath.c_str(), pathStr.c_str()) == 0) {
                    alreadyExists = true;
                    break;
                }
            }
            if (!alreadyExists) {
                newFiles.push_back(path);
            }
        }
        LogInfo(std::format(L"newFiles size: {}", newFiles.size()));

        // 6. Añadir y posicionar las imágenes nuevas
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> distRotation(-2.0f, 2.0f);

        for (size_t i = 0; i < newFiles.size(); ++i) {
            BoardItem imgItem;
            imgItem.type = BoardItemType::Image;
            imgItem.width = itemW;
            imgItem.height = itemH;
            imgItem.rotation = distRotation(gen);
            imgItem.filePath = newFiles[i].wstring();
            imgItem.label = newFiles[i].filename().wstring();

            PointF pos = m_page->m_layoutHelper->FindSpiralPosition(worldCenter, itemW, itemH, 40.0f, addedItems, true);
            imgItem.position = pos;

            addedItems.push_back(imgItem);
            m_page->m_items.push_back(imgItem);
            m_page->LoadItemTexture(m_page->m_items.back());
            
            finalPositions.push_back(pos);
            LogInfo(std::format(L"  Added new file '{}' at position ({}, {})", imgItem.filePath, pos.x, pos.y));
        }

        // 7. Empujar radialmente hacia afuera los elementos no coincidentes que colisionen
        auto isMatchingItem = [&](const BoardItem& checkItem) {
            if (checkItem.type != BoardItemType::Image) {
                return false;
            }
            for (const auto& path : matchingFiles) {
                if (_wcsicmp(checkItem.filePath.c_str(), path.wstring().c_str()) == 0) {
                    return true;
                }
            }
            return false;
        };

        for (auto& item : m_page->m_items) {
            if (item.type == BoardItemType::Link) {
                continue;
            }
            if (isMatchingItem(item)) {
                continue;
            }

            RectF itemRect = item.GetWorldRect();
            RectF checkRect = RectF::Centered(itemRect.Center().x, itemRect.Center().y, itemRect.Width() + 320.0f, itemRect.Height() + 320.0f);

            bool collides = false;
            for (const auto& matchItem : addedItems) {
                if (matchItem.type != BoardItemType::Link) {
                    if (checkRect.Intersects(matchItem.GetWorldRect())) {
                        collides = true;
                        break;
                    }
                }
            }

            if (collides) {
                PointF dir = { item.position.x - worldCenter.x, item.position.y - worldCenter.y };
                float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                if (len < 0.1f) {
                    dir = { 1.0f, 0.0f };
                } else {
                    dir.x /= len;
                    dir.y /= len;
                }

                float pushStep = 20.0f;
                int maxPushIter = 500;
                int pushIter = 0;
                while (pushIter < maxPushIter) {
                    item.position.x += dir.x * pushStep;
                    item.position.y += dir.y * pushStep;
                    
                    RectF currentRect = item.GetWorldRect();
                    RectF currentCheckRect = RectF::Centered(currentRect.Center().x, currentRect.Center().y, currentRect.Width() + 320.0f, currentRect.Height() + 320.0f);
                    
                    bool stillCollides = false;
                    for (const auto& matchItem : addedItems) {
                        if (matchItem.type != BoardItemType::Link) {
                            if (currentCheckRect.Intersects(matchItem.GetWorldRect())) {
                                stillCollides = true;
                                break;
                            }
                        }
                    }
                    if (!stillCollides) {
                        break;
                    }
                    pushIter++;
                }
                LogInfo(std::format(L"  Pushed non-matching item '{}' radially by {} iterations to ({}, {})", 
                    item.label.empty() ? L"Note/Node" : item.label, pushIter, item.position.x, item.position.y));
            }
        }

        // 8. Enfocar el centro de la pantalla hacia el centroide de los ítems agrupados
        if (!finalPositions.empty()) {
            PointF centroid = { 0.0f, 0.0f };
            for (const auto& pos : finalPositions) {
                centroid.x += pos.x;
                centroid.y += pos.y;
            }
            centroid.x /= finalPositions.size();
            centroid.y /= finalPositions.size();

            PointF oldPan = m_page->m_canvas.m_pan;
            m_page->m_canvas.m_pan.x = screenCenter.x - centroid.x * m_page->m_canvas.m_zoom;
            m_page->m_canvas.m_pan.y = screenCenter.y - centroid.y * m_page->m_canvas.m_zoom;
            LogInfo(std::format(L"Centroid calculated: ({}, {}). Old pan: ({}, {}), New pan: ({}, {})", 
                centroid.x, centroid.y, oldPan.x, oldPan.y, m_page->m_canvas.m_pan.x, m_page->m_canvas.m_pan.y));
        }

        m_page->EvictFarBoardTextures();
        m_page->m_app->m_window.RequestRedraw();
    } else {
        // --- COMPORTAMIENTO ORIGINAL A (FILTRAR / ELIMINAR) ---
        std::vector<int> indicesToDelete;
        for (int i = 0; i < static_cast<int>(m_page->m_items.size()); ++i) {
            auto& item = m_page->m_items[i];
            if (item.type == BoardItemType::Image && !item.filePath.empty()) {
                bool matches = false;
                for (const auto& path : matchingFiles) {
                    if (_wcsicmp(item.filePath.c_str(), path.wstring().c_str()) == 0) {
                        matches = true;
                        break;
                    }
                }
                if (!matches) {
                    indicesToDelete.push_back(i);
                }
            }
        }

        if (!indicesToDelete.empty()) {
            m_page->DeleteItemsByIndices(indicesToDelete);
        }

        if (matchingFiles.empty()) {
            m_page->m_app->m_window.RequestRedraw();
            return;
        }
        
        std::vector<std::filesystem::path> newFiles;
        for (const auto& path : matchingFiles) {
            bool alreadyExists = false;
            std::wstring pathStr = path.wstring();
            for (const auto& item : m_page->m_items) {
                if (item.type == BoardItemType::Image) {
                    if (_wcsicmp(item.filePath.c_str(), pathStr.c_str()) == 0) {
                        alreadyExists = true;
                        break;
                    }
                }
            }
            if (!alreadyExists) {
                newFiles.push_back(path);
            }
        }
        
        if (newFiles.empty()) {
            m_page->m_app->m_window.RequestRedraw();
            return;
        }
        
        auto targetSize = m_page->m_app->m_renderer.GetTargetSize().ToFloat();
        PointF screenCenter = { targetSize.width * 0.5f, targetSize.height * 0.5f };
        PointF worldCenter = m_page->ScreenToWorld(screenCenter);
        
        float itemW = 400.0f;
        float itemH = 300.0f;
        int count = static_cast<int>(newFiles.size());
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> distRotation(-2.0f, 2.0f);

        std::vector<BoardItem> addedItems;
        for (int i = 0; i < count; ++i) {
            BoardItem imgItem;
            imgItem.type = BoardItemType::Image;
            imgItem.width = itemW;
            imgItem.height = itemH;
            imgItem.rotation = distRotation(gen);
            imgItem.filePath = newFiles[i].wstring();
            imgItem.label = newFiles[i].filename().wstring();

            PointF pos = m_page->m_layoutHelper->FindSpiralPosition(worldCenter, itemW, itemH, 40.0f, addedItems);
            imgItem.position = pos;

            addedItems.push_back(imgItem);
            m_page->m_items.push_back(imgItem);
            m_page->LoadItemTexture(m_page->m_items.back());
        }
        
        m_page->EvictFarBoardTextures();
        m_page->m_app->m_window.RequestRedraw();
    }
}

} // namespace kv
