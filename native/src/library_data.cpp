// ============================================================================
// library_data.cpp — Data Management Layer Implementation
// Krumassan Image Viewer — Native C++20
// ============================================================================
#include "pch.h"
#include "library_data.h"
#include "app.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace kv {

void LibraryData::Initialize(App* app) {
    m_app = app;
    m_folders.clear();
    m_currentFolderIdx = -1;
    m_favorites.clear();
    m_history.clear();
}

std::wstring LibraryData::GetSessionFilePath() const {
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::filesystem::path p(exePath);
    for (int i = 0; i < 6; ++i) {
        p = p.parent_path();
        if (std::filesystem::exists(p / "user_data")) {
            return (p / "user_data" / "session.txt").wstring();
        }
    }
    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
    return (exeDir / "session.txt").wstring();
}

std::wstring LibraryData::GetFavoritesFilePath() const {
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::filesystem::path p(exePath);
    for (int i = 0; i < 6; ++i) {
        p = p.parent_path();
        if (std::filesystem::exists(p / "user_data")) {
            return (p / "user_data" / "fav.txt").wstring();
        }
    }
    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
    return (exeDir / "fav.txt").wstring();
}

void LibraryData::AddFolder(const std::wstring& folderPath, bool libraryLoading) {
    namespace fs = std::filesystem;
    
    // Check if duplicate
    auto normPath = fs::path(folderPath).lexically_normal().wstring();
    for (size_t i = 0; i < m_folders.size(); ++i) {
        if (fs::path(m_folders[i].folderPath).lexically_normal().wstring() == normPath) {
            if (!libraryLoading) {
                SetCurrentFolderIndex(static_cast<int>(i));
            }
            return;
        }
    }

    FolderInfo folder;
    folder.folderPath = folderPath;
    folder.folderName = fs::path(folderPath).filename().wstring();
    if (folder.folderName.empty()) {
        folder.folderName = folderPath;
    }

    // Populate file list
    try {
        for (const auto& entry : fs::directory_iterator(folderPath)) {
            if (entry.is_regular_file() && ImageLoader::IsSupported(entry.path().wstring())) {
                folder.filePaths.push_back(entry.path().wstring());
            }
        }
    } catch (...) {
        LogError(std::format(L"Failed to scan library directory: {}", folderPath));
    }

    // Sort by name default
    std::sort(folder.filePaths.begin(), folder.filePaths.end());

    // Initialize previews container
    folder.previews.resize(folder.filePaths.size());
    for (size_t i = 0; i < folder.filePaths.size(); ++i) {
        folder.previews[i].filePath = folder.filePaths[i];
    }
    folder.previewsInitialized = true;

    m_folders.push_back(std::move(folder));

    if (!libraryLoading) {
        SetCurrentFolderIndex(static_cast<int>(m_folders.size() - 1));
        SaveSession();
    }
}

void LibraryData::RemoveFolder(int index) {
    if (index < 0 || index >= static_cast<int>(m_folders.size())) return;
    
    m_folders.erase(m_folders.begin() + index);

    if (m_folders.empty()) {
        m_currentFolderIdx = -1;
    } else {
        m_currentFolderIdx = std::clamp(m_currentFolderIdx, 0, static_cast<int>(m_folders.size() - 1));
    }

    SaveSession();
    
    if (m_app) {
        m_app->m_window.RequestRedraw();
    }
}

void LibraryData::SetCurrentFolderIndex(int index) {
    if (index < -1 || index >= static_cast<int>(m_folders.size())) return;
    
    m_currentFolderIdx = index;
    if (m_app && m_currentFolderIdx >= 0) {
        auto& folder = m_folders[m_currentFolderIdx];
        m_app->m_fileList = folder.filePaths;
        m_app->m_currentIndex = folder.currentIndex;
        if (!folder.filePaths.empty()) {
            m_app->LoadImage(folder.filePaths[folder.currentIndex]);
        }
    }
    
    if (m_app) {
        m_app->m_window.RequestRedraw();
    }
}

bool LibraryData::LoadSession() {
    std::wstring path = GetSessionFilePath();
    if (!std::filesystem::exists(path)) return false;

    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) return false;

    std::string line;
    std::vector<std::string> lines;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }

    auto toWString = [](const std::string& str) -> std::wstring {
        int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        if (len > 0) {
            std::vector<wchar_t> w(len);
            MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, w.data(), len);
            return std::wstring(w.data());
        }
        return L"";
    };

    // Every record is 4 lines: folder_path, md5, modifiers, separator
    bool loadedAny = false;
    for (size_t i = 0; i < lines.size(); i += 4) {
        if (i >= lines.size()) break;
        std::wstring fPath = toWString(lines[i]);
        if (std::filesystem::exists(fPath)) {
            AddFolder(fPath, true);
            loadedAny = true;
        }
    }

    if (loadedAny && !m_folders.empty()) {
        SetCurrentFolderIndex(0);
    }
    return loadedAny;
}

bool LibraryData::SaveSession() {
    std::wstring path = GetSessionFilePath();
    
    std::filesystem::path dir(path);
    try {
        std::filesystem::create_directories(dir.parent_path());
    } catch (...) {
        return false;
    }

    std::ofstream file(path, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!file.is_open()) return false;

    auto toUtf8 = [](const std::wstring& wstr) -> std::string {
        if (wstr.empty()) return "";
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    };

    for (const auto& folder : m_folders) {
        std::string fPath = toUtf8(folder.folderPath);
        std::string currentImageMd5 = ""; // In C++ phase 2/3 we don't calculate md5 yet
        std::string modifiers = "";       // modifiers default to empty
        
        file << fPath << "\n";
        file << currentImageMd5 << "\n";
        file << modifiers << "\n\n";
    }

    return true;
}

bool LibraryData::LoadFavorites() {
    std::wstring path = GetFavoritesFilePath();
    if (!std::filesystem::exists(path)) return false;

    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) return false;

    std::string line;
    std::vector<std::string> lines;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }

    auto toWString = [](const std::string& str) -> std::wstring {
        int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        if (len > 0) {
            std::vector<wchar_t> w(len);
            MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, w.data(), len);
            return std::wstring(w.data());
        }
        return L"";
    };

    m_favorites.clear();
    for (size_t i = 0; i < lines.size(); i += 4) {
        if (i >= lines.size()) break;
        std::wstring fPath = toWString(lines[i]);
        m_favorites.push_back(fPath);
    }
    return true;
}

bool LibraryData::SaveFavorites() {
    std::wstring path = GetFavoritesFilePath();
    
    std::filesystem::path dir(path);
    try {
        std::filesystem::create_directories(dir.parent_path());
    } catch (...) {
        return false;
    }

    std::ofstream file(path, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!file.is_open()) return false;

    auto toUtf8 = [](const std::wstring& wstr) -> std::string {
        if (wstr.empty()) return "";
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    };

    for (const auto& fav : m_favorites) {
        file << toUtf8(fav) << "\n";
        file << "\n"; // md5 placeholder
        file << "0\n\n"; // size placeholder & separator
    }

    return true;
}

void LibraryData::ToggleFavorite(const std::wstring& filePath) {
    auto it = std::find(m_favorites.begin(), m_favorites.end(), filePath);
    if (it != m_favorites.end()) {
        m_favorites.erase(it);
    } else {
        m_favorites.push_back(filePath);
    }
    SaveFavorites();
}

bool LibraryData::IsFavorite(const std::wstring& filePath) const {
    return std::find(m_favorites.begin(), m_favorites.end(), filePath) != m_favorites.end();
}

void LibraryData::AddToHistory(const std::wstring& filePath) {
    auto it = std::find(m_history.begin(), m_history.end(), filePath);
    if (it != m_history.end()) {
        m_history.erase(it);
    }
    m_history.insert(m_history.begin(), filePath);
    if (m_history.size() > MAX_HISTORY_SIZE) {
        m_history.resize(MAX_HISTORY_SIZE);
    }
}

void LibraryData::SortFolder(int folderIdx, SortMode mode) {
    if (folderIdx < 0 || folderIdx >= static_cast<int>(m_folders.size())) return;
    auto& folder = m_folders[folderIdx];
    if (folder.filePaths.empty()) return;

    struct FileEntry {
        std::wstring path;
        std::wstring name;
        std::filesystem::file_time_type date;
        uint64_t size = 0;
        PreviewItem preview;
    };

    std::vector<FileEntry> entries;
    entries.reserve(folder.filePaths.size());

    for (size_t i = 0; i < folder.filePaths.size(); ++i) {
        const auto& path = folder.filePaths[i];
        FileEntry entry;
        entry.path = path;
        entry.name = std::filesystem::path(path).filename().wstring();

        // Lowercase for case insensitive
        std::wstring lowerName = entry.name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);
        entry.name = lowerName;

        try {
            entry.date = std::filesystem::last_write_time(path);
            entry.size = std::filesystem::file_size(path);
        } catch (...) {
            entry.date = std::filesystem::file_time_type();
            entry.size = 0;
        }

        if (i < folder.previews.size()) {
            entry.preview = folder.previews[i];
        } else {
            entry.preview.filePath = path;
        }
        entries.push_back(std::move(entry));
    }

    auto comp = [mode](const FileEntry& a, const FileEntry& b) {
        switch (mode) {
            case SortMode::NameAsc:
                return a.name < b.name;
            case SortMode::NameDesc:
                return a.name > b.name;
            case SortMode::DateAsc:
                return a.date < b.date;
            case SortMode::DateDesc:
                return a.date > b.date;
            case SortMode::SizeAsc:
                return a.size < b.size;
            case SortMode::SizeDesc:
                return a.size > b.size;
        }
        return false;
    };

    std::sort(entries.begin(), entries.end(), comp);

    folder.filePaths.clear();
    folder.previews.clear();
    for (auto& entry : entries) {
        folder.filePaths.push_back(entry.path);
        folder.previews.push_back(entry.preview);
    }
}

} // namespace kv
