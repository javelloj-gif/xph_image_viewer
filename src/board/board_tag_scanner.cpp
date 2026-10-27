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
    {
        std::lock_guard lock(m_publishMutex);
        m_cancelled = true;
    }
    if (m_scanThread.joinable()) m_scanThread.join();
    m_scanningTags = false;
}

void BoardTagScanner::StartScan(const std::filesystem::path& activeDir) {
    CancelScan();
    m_cancelled = false;
    m_scanningTags = true;
    m_scanCurrent = 0;
    m_scanTotal = 0;
    m_scanDir = activeDir.wstring();
    try {
        m_scanThread = std::thread([this, activeDir]() {
            ScopedComInit com;
            try {
                if (com.IsValid()) Scan(activeDir);
            } catch (const std::exception&) {
                LogError(L"BoardTagScanner: scan failed; existing catalog retained");
            }
            m_scanningTags = false;
            m_page->m_app->m_window.RequestRedraw();
        });
    } catch (...) {
        m_scanningTags = false;
        throw;
    }
}

void BoardTagScanner::Scan(const std::filesystem::path& activeDir) {
    std::vector<std::filesystem::path> files;
    if (m_cancelled) return;
    for (const auto& entry : std::filesystem::directory_iterator(activeDir)) {
        if (m_cancelled) return;
        if (entry.is_regular_file() && ImageLoader::IsSupported(entry.path().wstring())) files.push_back(entry.path());
    }
    m_scanTotal = files.size();
    std::set<std::wstring> uniqueTags;
    for (const auto& filePath : files) {
        if (m_cancelled) return;
        std::vector<std::wstring> fileTags;
        const bool read = m_page->m_app->m_tagManager.ReadTags(filePath.wstring(), fileTags);
        if (m_cancelled) return;
        if (read) {
            for (const auto& tag : fileTags) {
                if (m_cancelled) return;
                if (!tag.empty()) uniqueTags.insert(tag);
            }
        }
        ++m_scanCurrent;
        m_page->m_app->m_window.RequestRedraw();
    }
    if (files.empty() || m_cancelled) return;

    std::wstring json = L"[\n";
    bool first = true;
    for (const auto& tag : uniqueTags) {
        if (m_cancelled) return;
        if (!first) json += L",\n";
        first = false;
        json += L"  \"";
        for (wchar_t ch : tag) {
            if (m_cancelled) return;
            if (ch == L'\\' || ch == L'"') json += L'\\';
            json += ch;
        }
        json += L'"';
    }
    json += L"\n]\n";
    if (m_cancelled) return;
    const int length = WideCharToMultiByte(CP_UTF8, 0, json.data(), static_cast<int>(json.size()), nullptr, 0, nullptr, nullptr);
    if (length <= 0 || m_cancelled) return;
    std::string utf8(static_cast<size_t>(length), '\0');
    if (!WideCharToMultiByte(CP_UTF8, 0, json.data(), static_cast<int>(json.size()), utf8.data(), length, nullptr, nullptr)) return;
    PublishCatalog(activeDir, utf8);
    if (!m_cancelled) m_page->m_app->m_tagManager.SaveCache();
}

void BoardTagScanner::PublishCatalog(const std::filesystem::path& activeDir, const std::string& json) {
    if (m_cancelled) return;
    static std::atomic<uint64_t> sequence{0};
    const auto path = activeDir / (L"xmpTags.json.tmp-" + std::to_wstring(GetCurrentProcessId()) +
        L"-" + std::to_wstring(GetTickCount64()) + L"-" + std::to_wstring(++sequence));
    // Owns only this CREATE_NEW temporary; cancellation never removes or truncates the existing catalog.
    struct CatalogTemporary {
        HANDLE file;
        std::filesystem::path path;
        // Closes the owned file and removes its private name after success, cancellation or exception.
        ~CatalogTemporary() {
            if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
            if (owned) {
                std::error_code error;
                std::filesystem::remove(path, error);
            }
        }
        bool owned = file != INVALID_HANDLE_VALUE;
    } temporary{CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr), path};
    if (temporary.file == INVALID_HANDLE_VALUE) return;
    size_t offset = 0;
    while (offset < json.size()) {
        if (m_cancelled) return;
        const DWORD chunk = static_cast<DWORD>(std::min<size_t>(65536, json.size() - offset));
        DWORD written = 0;
        if (!WriteFile(temporary.file, json.data() + offset, chunk, &written, nullptr) || written != chunk) return;
        offset += written;
    }
    CloseHandle(temporary.file);
    temporary.file = INVALID_HANDLE_VALUE;
    std::lock_guard lock(m_publishMutex);
    if (!m_cancelled && !MoveFileExW(path.c_str(), (activeDir / L"xmpTags.json").c_str(), MOVEFILE_REPLACE_EXISTING)) {
        LogError(L"BoardTagScanner: catalog replacement failed; existing catalog retained");
    }
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
    
    const int percent=static_cast<int>(pct*100);
    std::wstring text = std::vformat(_(L"Scanning directory: {} / {} ({}%)"),std::make_wformat_args(current,total,percent));
    renderer.DrawText(text, barRect, Color::White(), 12.0f, TextAlign::Center, TextVAlign::Center);
}

} // namespace kv
