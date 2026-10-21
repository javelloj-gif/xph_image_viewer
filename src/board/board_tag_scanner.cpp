#include "common/pch.h"
#include "board_tag_scanner.h"
#include "board_page.h"
#include "core/app.h"
#include "loader/image_loader.h"
#include <fstream>
#include <set>
#include <algorithm>

namespace kv {

BoardTagScanner::BoardTagScanner(BoardPage* page) : m_page(page) {}

BoardTagScanner::~BoardTagScanner() {
    CancelScan();
}

void BoardTagScanner::CancelScan() {
    if (m_scanThread.joinable()) {
        m_scanThread.join();
    }
}

void BoardTagScanner::StartScan(const std::filesystem::path& activeDir) {
    m_scanningTags = true;
    m_scanCurrent = 0;
    m_scanTotal = 0;
    m_scanDir = activeDir.wstring();
    
    if (m_scanThread.joinable()) {
        m_scanThread.join();
    }
    
    m_scanThread = std::thread([this, activeDir]() {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        bool comInitialized = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
        
        std::vector<std::filesystem::path> files;
        try {
            for (const auto& entry : std::filesystem::directory_iterator(activeDir)) {
                if (entry.is_regular_file() && ImageLoader::IsSupported(entry.path().wstring())) {
                    files.push_back(entry.path());
                }
            }
        } catch (...) {}
        
        m_scanTotal = files.size();
        m_scanCurrent = 0;
        
        std::set<std::wstring> uniqueTags;
        
        for (const auto& filePath : files) {
            std::vector<std::wstring> fileTags;
            if (m_page->m_app->m_tagManager.ReadTags(filePath.wstring(), fileTags)) {
                for (const auto& tag : fileTags) {
                    if (!tag.empty()) {
                        uniqueTags.insert(tag);
                    }
                }
            }
            m_scanCurrent++;
            m_page->m_app->m_window.RequestRedraw();
        }
        
        if (!files.empty()) {
            std::filesystem::path jsonPath = activeDir / "xmpTags.json";
            
            std::wstring jsonW = L"[\n";
            bool first = true;
            for (const auto& tag : uniqueTags) {
                if (!first) {
                    jsonW += L",\n";
                }
                first = false;
                
                std::wstring escaped = L"";
                for (wchar_t ch : tag) {
                    if (ch == L'\\' || ch == L'"') {
                        escaped += L'\\';
                    }
                    escaped += ch;
                }
                jsonW += L"  \"" + escaped + L"\"";
            }
            jsonW += L"\n]\n";
            
            std::string jsonUtf8;
            int lenLen = WideCharToMultiByte(CP_UTF8, 0, jsonW.c_str(), static_cast<int>(jsonW.size()), nullptr, 0, nullptr, nullptr);
            if (lenLen > 0) {
                jsonUtf8.resize(lenLen);
                WideCharToMultiByte(CP_UTF8, 0, jsonW.c_str(), static_cast<int>(jsonW.size()), &jsonUtf8[0], lenLen, nullptr, nullptr);
            }
            
            std::ofstream file(jsonPath, std::ios::binary);
            if (file) {
                file.write(jsonUtf8.data(), jsonUtf8.size());
                file.close();
            }
        }
        
        m_page->m_app->m_tagManager.SaveCache();
        m_scanningTags = false;
        m_page->m_app->m_window.RequestRedraw();
        
        if (comInitialized) {
            CoUninitialize();
        }
    });
}

void BoardTagScanner::DrawProgressBar(Renderer& renderer) {
    if (!m_scanningTags) return;
    
    auto screenSize = renderer.GetTargetSize().ToFloat();
    
    float barW = 400.0f;
    float barH = 30.0f;
    float barX = (screenSize.width - barW) * 0.5f;
    float barY = 20.0f;
    
    RectF barRect = RectF::FromXYWH(barX, barY, barW, barH);
    
    renderer.FillRoundedRect(barRect, 6.0f, 6.0f, Color::FromRGBA(15, 15, 20, 240));
    renderer.DrawRoundedRect(barRect, 6.0f, 6.0f, Color::FromRGBA(0, 240, 255, 60), 1.5f);
    
    size_t total = m_scanTotal;
    size_t current = m_scanCurrent.load();
    float pct = (total > 0) ? (static_cast<float>(current) / static_cast<float>(total)) : 0.0f;
    pct = std::clamp(pct, 0.0f, 1.0f);
    
    RectF fillRect = RectF::FromXYWH(barX + 3.0f, barY + 3.0f, (barW - 6.0f) * pct, barH - 6.0f);
    if (fillRect.Width() > 0.0f) {
        renderer.FillRoundedRect(fillRect, 4.0f, 4.0f, Color::FromRGBA(0, 240, 255, 120));
    }
    
    std::wstring text = std::format(L"Escaneando directorio: {} / {} ({}%)", current, total, static_cast<int>(pct * 100.0f));
    renderer.DrawText(text, barRect, Color::White(), 12.0f, TextAlign::Center, TextVAlign::Center);
}

} // namespace kv
