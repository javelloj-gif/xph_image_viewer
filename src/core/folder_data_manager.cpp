// ============================================================================
// Class: FolderDataManager
// Description: Shared folder navigation and persistence for Image Viewer's pages.
// Responsibility: Own folder/preview state, selection, session, favorites and history.
// ============================================================================
#include "common/pch.h"
#include "folder_data_manager.h"
#include "core/app.h"
#include "common/performance_trace.h"
#include "common/user_data_paths.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace kv {


FolderDataManager::~FolderDataManager() { StopScanning(); }
void FolderDataManager::EnableAsync() { if (!m_scanner) m_scanner=std::make_unique<FolderScanService>(); }
void FolderDataManager::ProcessPending() { if (m_scanner) m_scanner->ProcessPending(); }
void FolderDataManager::StopScanning() { if (m_scanner) { m_scanner->Shutdown(); m_scanner.reset(); } }
void FolderDataManager::NotifyImageRemoved(const std::wstring& path) {
    ++m_fileMutation;
    for (auto& folder : m_folders) {
        std::erase(folder.inventory,path); folder.hiddenPreviews.erase(path); folder.viewLifetime.reset();
        folder.appliedSort=-1;
    }
    if (!m_pendingFolders.empty()) m_removedDuringScan.emplace_back(path,m_fileMutation);
}
void FolderDataManager::NotifyAttributesChanged() {
    ++m_fileMutation;
    for (auto& folder : m_folders) { folder.appliedSort=-1; folder.viewLifetime.reset(); }
    ApplyView(m_currentFolderIdx);
}

void FolderDataManager::RequestFolder(const std::wstring& path, bool restoring, bool loadImage) {
    if (!restoring) { ++m_navigationGeneration; m_restoreSelect=false; }
    for (size_t i=0;i<m_folders.size();++i) {
        if (PathEquals(m_folders[i].folderPath,path)) {
            if (!restoring) { SetCurrentFolderIndex(static_cast<int>(i),loadImage); if (m_app) m_app->OnFolderReady(); }
            return;
        }
    }
    auto old=std::find_if(m_pendingFolders.begin(),m_pendingFolders.end(),[&](const PendingFolder& f) { return PathEquals(f.path,path); });
    if (old!=m_pendingFolders.end()) {
        if (restoring) return;
        old->lifetime.reset(); m_pendingFolders.erase(old);
    }
    if (!restoring && m_app) {
        CancelPreviewLoads(); m_app->CancelHoverLoads();
        m_currentFolderIdx=-1; m_app->m_fileList.clear(); m_app->m_currentIndex=-1;
        m_app->m_thumbnailStrip.SetFileList({});
    }
    const auto request=std::make_shared<const std::wstring>(path);
    auto lifetime=std::make_shared<const bool>(true);
    m_pendingFolders.push_back({path,restoring,loadImage,m_navigationGeneration,m_fileMutation,lifetime});
    if (std::find(m_sessionOrder.begin(),m_sessionOrder.end(),path)==m_sessionOrder.end()) m_sessionOrder.push_back(path);
    m_scanner->Request(path,m_app?m_app->m_settings.sort_mode:0,!restoring,ImageCancellation(lifetime),
        [this,request](FolderSnapshot snapshot) { CompleteFolder(request,std::move(snapshot)); },m_app?m_app->m_settings.image_filter_mask:511);
}
void FolderDataManager::CompleteFolder(const ImageRequestToken& request, FolderSnapshot snapshot) {
    auto found=std::find_if(m_pendingFolders.begin(),m_pendingFolders.end(),[&](const PendingFolder& f) { return f.path==*request; });
    if (found==m_pendingFolders.end()) return;
    const auto pending=*found;
    m_pendingFolders.erase(found);
    if (!snapshot.complete) return;
    if (m_app && (snapshot.sortMode!=m_app->m_settings.sort_mode || snapshot.filterMask!=m_app->m_settings.image_filter_mask)) {
        // Reproject the completed inventory with current preferences, retaining navigation intent.
        auto retry=pending; retry.lifetime=std::make_shared<const bool>(true);
        m_pendingFolders.push_back(retry);
        m_scanner->RequestView(retry.path,std::move(snapshot.inventory),m_app->m_settings.sort_mode,m_app->m_settings.image_filter_mask,
            m_fileMutation,ImageCancellation(retry.lifetime),[this,request](FolderSnapshot value) { CompleteFolder(request,std::move(value)); });
        return;
    }
    std::erase_if(snapshot.files,[&](const std::wstring& path) {
        return std::any_of(m_removedDuringScan.begin(),m_removedDuringScan.end(),[&](const auto& removed) {
            return removed.second>pending.fileMutation && PathEquals(path,removed.first);
        });
    });
    std::erase_if(snapshot.inventory,[&](const std::wstring& path) {
        return std::any_of(m_removedDuringScan.begin(),m_removedDuringScan.end(),[&](const auto& removed) { return removed.second>pending.fileMutation && PathEquals(path,removed.first); });
    });
    if (m_pendingFolders.empty()) m_removedDuringScan.clear();
    if (!snapshot.complete || snapshot.inventory.empty()) return;
    FolderInfo folder; folder.folderPath=snapshot.path; folder.folderName=std::filesystem::path(snapshot.path).filename().wstring();
    if (folder.folderName.empty()) folder.folderName=snapshot.path;
    folder.inventory=std::move(snapshot.inventory); folder.appliedSort=snapshot.sortMode; folder.appliedFilter=snapshot.filterMask;
    folder.filePaths=std::move(snapshot.files); folder.previews.resize(folder.filePaths.size());
    for (size_t i=0;i<folder.previews.size();++i) folder.previews[i].filePath=folder.filePaths[i];
    folder.previewsInitialized=true; m_folders.push_back(std::move(folder)); NotifyPreviewLayoutChanged();
    const bool select=(!pending.restoring && pending.generation==m_navigationGeneration) || (pending.restoring && m_restoreSelect && m_currentFolderIdx<0);
    if (select) {
        m_restoreSelect=false;
        SetCurrentFolderIndex(static_cast<int>(m_folders.size()-1),pending.loadImage);
        if (m_app) m_app->OnFolderReady();
        if (!pending.restoring) SaveSession();
    }
}

void FolderDataManager::Initialize(App* app) {
    StopScanning();
    m_pendingFolders.clear(); m_sessionOrder.clear(); m_restoreSelect = false;
    m_removedDuringScan.clear(); m_fileMutation=0;
    m_app = app;
    m_folders.clear();
    NotifyPreviewLayoutChanged();
    m_currentFolderIdx = -1;
    m_favorites.clear();
    m_history.clear();
}

std::wstring FolderDataManager::GetSessionFilePath() const {
    return (GetExecutableUserDataRoot() / "session.txt").wstring();
}

std::wstring FolderDataManager::GetFavoritesFilePath() const {
    return (GetExecutableUserDataRoot() / "fav.txt").wstring();
}

void FolderDataManager::AddFolder(const std::wstring& folderPath, bool restoringSession, bool loadImage) {
    namespace fs = std::filesystem;
    
    auto normPath = NormalizeFolderPath(folderPath);
    if (m_scanner) { RequestFolder(normPath, restoringSession, loadImage); return; }
    for (size_t i = 0; i < m_folders.size(); ++i) {
        if (PathEquals(m_folders[i].folderPath, normPath)) {
            if (!restoringSession) {
                SetCurrentFolderIndex(static_cast<int>(i), loadImage);
            }
            return;
        }
    }
    
    FolderInfo folder;
    folder.folderPath = normPath;
    folder.folderName = fs::path(normPath).filename().wstring();
    if (folder.folderName.empty()) {
        folder.folderName = normPath;
    }

    PerformanceSpan scanSpan("folder.scan.begin", "folder.scan.end");
    try {
        for (const auto& entry : fs::directory_iterator(normPath)) {
            if (entry.is_regular_file() && ImageLoader::IsSupported(entry.path().wstring())) {
                folder.filePaths.push_back(entry.path().wstring());
            }
        }
    } catch (...) {
        LogError(std::format(L"Failed to scan directory: {}", normPath));
    }

    if (folder.filePaths.empty()) {
        LogInfo(std::format(L"Directory contains no supported images: {}", normPath));
        return;
    }

    std::sort(folder.filePaths.begin(), folder.filePaths.end());

    folder.previews.resize(folder.filePaths.size());
    for (size_t i = 0; i < folder.filePaths.size(); ++i) {
        folder.previews[i].filePath = folder.filePaths[i];
    }
    folder.previewsInitialized = true;

    folder.inventory=folder.filePaths;
    m_folders.push_back(std::move(folder));
    NotifyPreviewLayoutChanged();

    int currentSortMode = m_app ? m_app->m_settings.sort_mode : 0;
    SortFolder(static_cast<int>(m_folders.size() - 1), static_cast<SortMode>(currentSortMode));

    if (!restoringSession) {
        SetCurrentFolderIndex(static_cast<int>(m_folders.size() - 1), loadImage);
        SaveSession();
    }
}

void FolderDataManager::RemoveFolder(int index) {
    if (index < 0 || index >= static_cast<int>(m_folders.size())) return;
    if (m_app) m_app->CancelHoverLoads();
    
    std::erase(m_sessionOrder, m_folders[index].folderPath);
    ++m_navigationGeneration;
    m_folders.erase(m_folders.begin() + index);
    NotifyPreviewLayoutChanged();

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

void FolderDataManager::SetCurrentFolderIndex(int index, bool loadImage) {
    if (index < -1 || index >= static_cast<int>(m_folders.size())) return;
    if (index != m_currentFolderIdx) {
        CancelPreviewLoads();
        if (m_app) m_app->CancelHoverLoads();
    }
    
    ++m_navigationGeneration;
    m_currentFolderIdx = index;
    if (m_app && m_currentFolderIdx >= 0) {
        auto& folder = m_folders[m_currentFolderIdx];
        m_app->m_fileList = folder.filePaths;
        
        if (folder.currentIndex < 0 || folder.currentIndex >= static_cast<int>(folder.filePaths.size())) {
            folder.currentIndex = 0;
        }
        m_app->m_currentIndex = folder.currentIndex;
        if (!loadImage) {
            const auto active=m_app->m_currentImage ? m_app->m_currentImage->filePath : m_app->m_animatedImage ? m_app->m_animatedImage->filePath : L"";
            const auto match=std::find(folder.filePaths.begin(),folder.filePaths.end(),active);
            m_app->m_currentIndex=match==folder.filePaths.end() ? -1 : static_cast<int>(std::distance(folder.filePaths.begin(),match));
            folder.currentIndex=m_app->m_currentIndex;
        }
        m_app->m_thumbnailStrip.SetFileList(folder.filePaths);
        m_app->m_thumbnailStrip.SyncSelection(m_app->m_currentIndex);
        
        if (loadImage && !folder.filePaths.empty()) {
            m_app->LoadImage(folder.filePaths[folder.currentIndex]);
        }
    }
    
    if (m_app) {
        ApplyView(index);
        m_app->m_window.RequestRedraw();
    }
}

bool FolderDataManager::LoadSession(bool loadImage, const std::wstring& initialFolder, bool initialLoadImage) {
    m_restoreSelect = loadImage;
    std::wstring path = GetSessionFilePath();
    std::ifstream file(path, std::ios::in | std::ios::binary);

    std::string line;
    std::vector<std::string> lines;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }

    auto toWString = [](const std::string& str) -> std::wstring {
        if (str.empty()) return L"";
        int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        if (len <= 0) return L"";
        std::vector<wchar_t> w(len);
        if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, w.data(), len) == 0) {
            return L"";
        }
        return std::wstring(w.data());
    };

    std::vector<std::wstring> restoredPaths;
    for (size_t i = 0; i + 3 < lines.size(); i += 4) {
        std::wstring fPath = toWString(lines[i]);
        if (fPath.empty()) continue;
        if (!m_scanner && !std::filesystem::exists(fPath)) continue;
        if (std::find(m_sessionOrder.begin(),m_sessionOrder.end(),fPath)==m_sessionOrder.end()) m_sessionOrder.push_back(fPath);
        restoredPaths.push_back(std::move(fPath));
    }
    // Register persistent order before foreground scheduling changes completion order.
    if (!initialFolder.empty()) AddFolder(initialFolder, false, initialLoadImage);
    bool loadedAny = false;
    for (const auto& fPath : restoredPaths) {
        size_t prevSize = m_folders.size();
        try {
            AddFolder(fPath, true);
            if (m_folders.size() > prevSize) {
                loadedAny = true;
            }
        } catch (...) {
            LogError(std::format(L"Failed to add folder from session: {}", fPath));
        }
    }

    if (initialFolder.empty() && loadedAny && !m_folders.empty()) {
        SetCurrentFolderIndex(0, loadImage);
    }
    return loadedAny || HasPendingFolders();
}

bool FolderDataManager::SaveSession() {
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

    std::vector<std::wstring> paths;
    auto append = [&paths](const std::wstring& value) { if (std::find(paths.begin(),paths.end(),value)==paths.end()) paths.push_back(value); };
    for (const auto& value : m_sessionOrder) {
        if (std::any_of(m_folders.begin(),m_folders.end(),[&](const FolderInfo& f) { return PathEquals(f.folderPath,value); }) ||
            std::any_of(m_pendingFolders.begin(),m_pendingFolders.end(),[&](const PendingFolder& f) { return PathEquals(f.path,value); })) append(value);
    }
    for (const auto& folder : m_folders) append(folder.folderPath);
    for (const auto& pending : m_pendingFolders) append(pending.path);
    for (const auto& folderPath : paths) {
        std::string fPath = toUtf8(folderPath);
        std::string currentImageMd5 = "";
        std::string modifiers = "";
        
        file << fPath << "\n";
        file << currentImageMd5 << "\n";
        file << modifiers << "\n\n";
    }

    return true;
}

bool FolderDataManager::LoadFavorites() {
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

    if (lines.size() < 4) return false;

    auto toWString = [](const std::string& str) -> std::wstring {
        if (str.empty()) return L"";
        int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        if (len <= 0) return L"";
        std::vector<wchar_t> w(len);
        if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, w.data(), len) == 0) {
            return L"";
        }
        return std::wstring(w.data());
    };

    m_favorites.clear();
    for (size_t i = 0; i + 3 < lines.size(); i += 4) {
        std::wstring fPath = toWString(lines[i]);
        if (!fPath.empty()) {
            m_favorites.push_back(fPath);
        }
    }
    return true;
}

bool FolderDataManager::SaveFavorites() {
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
        file << "\n";
        file << "0\n\n";
    }

    return true;
}

void FolderDataManager::ToggleFavorite(const std::wstring& filePath) {
    auto it = std::find(m_favorites.begin(), m_favorites.end(), filePath);
    if (it != m_favorites.end()) {
        m_favorites.erase(it);
    } else {
        m_favorites.push_back(filePath);
    }
    SaveFavorites();
}

bool FolderDataManager::IsFavorite(const std::wstring& filePath) const {
    return std::find(m_favorites.begin(), m_favorites.end(), filePath) != m_favorites.end();
}

void FolderDataManager::AddToHistory(const std::wstring& filePath) {
    auto it = std::find(m_history.begin(), m_history.end(), filePath);
    if (it != m_history.end()) {
        m_history.erase(it);
    }
    m_history.insert(m_history.begin(), filePath);
    if (m_history.size() > MAX_HISTORY_SIZE) {
        m_history.resize(MAX_HISTORY_SIZE);
    }
}

void FolderDataManager::CancelPreviewLoads() {
    for (auto& folder : m_folders) {
        for (auto& item : folder.previews) {
            item.loadLifetime.reset();
            item.pendingRequest.reset();
            item.isLoading = false;
        }
    }
}

bool FolderDataManager::IsPreviewRequestCurrent(const ImageRequestToken& request) const {
    if (!request) return false;
    for (const auto& folder : m_folders) {
        for (const auto& item : folder.previews) {
            if (item.pendingRequest == request && item.filePath == *request) return true;
        }
    }
    return false;
}

bool FolderDataManager::CompletePreview(const ImageRequestToken& request, std::shared_ptr<ImageData> image) {
    if (!request) return false;
    for (auto& folder : m_folders) {
        for (auto& item : folder.previews) {
            if (item.pendingRequest != request || item.filePath != *request) continue;
            item.pendingRequest.reset();
            item.loadLifetime.reset();
            item.isLoading = false;
            if (image) {
                item.texture = std::move(image);
                NotifyPreviewLayoutChanged();
            } else {
                item.loadFailed = true;
            }
            return true;
        }
    }
    return false;
}

void FolderDataManager::ApplyView(int folderIdx) {
    if (!m_app || folderIdx<0 || folderIdx>=static_cast<int>(m_folders.size())) return;
    auto& folder=m_folders[folderIdx];
    if (folder.appliedSort==m_app->m_settings.sort_mode && folder.appliedFilter==m_app->m_settings.image_filter_mask) return;
    if (folder.inventory.empty() && !folder.filePaths.empty()) folder.inventory=folder.filePaths;
    if (!m_scanner) EnableAsync();
    folder.viewLifetime=std::make_shared<const bool>(true);
    const auto mutation=m_fileMutation;
    const auto path=folder.folderPath;
    m_scanner->RequestView(path,folder.inventory,m_app->m_settings.sort_mode,m_app->m_settings.image_filter_mask,
        mutation,ImageCancellation(folder.viewLifetime),[this,path,mutation](FolderSnapshot value) { CompleteView(path,mutation,std::move(value)); });
}
void FolderDataManager::CompleteView(const std::wstring& path, uint64_t mutation, FolderSnapshot snapshot) {
    auto found=std::find_if(m_folders.begin(),m_folders.end(),[&](const FolderInfo& f) { return f.folderPath==path; });
    if (found==m_folders.end() || !snapshot.complete || mutation!=m_fileMutation || !m_app) return;
    auto& folder=*found;
    if (snapshot.sortMode!=m_app->m_settings.sort_mode || snapshot.filterMask!=m_app->m_settings.image_filter_mask) return;
    auto& previews=folder.hiddenPreviews;
    for (auto& preview : folder.previews) previews.insert_or_assign(preview.filePath,std::move(preview));
    folder.previews.clear();
    folder.filePaths=std::move(snapshot.files);
    for (const auto& file : folder.filePaths) {
        auto it=previews.find(file);
        if (it!=previews.end()) { folder.previews.push_back(std::move(it->second)); previews.erase(it); }
        else { PreviewItem item; item.filePath=file; folder.previews.push_back(std::move(item)); }
    }
    for (auto& [hidden,item] : previews) {
        item.loadLifetime.reset(); item.pendingRequest.reset(); item.isLoading=false;
    }
    folder.appliedSort=snapshot.sortMode; folder.appliedFilter=snapshot.filterMask; folder.viewLifetime.reset();
    folder.scrollOffset=folder.targetScrollOffset=0;
    NotifyPreviewLayoutChanged();
    if (std::distance(m_folders.begin(),found)==m_currentFolderIdx) {
        m_app->CancelHoverLoads(); m_app->m_fileList=folder.filePaths;
        const auto active=m_app->m_currentImage ? m_app->m_currentImage->filePath : m_app->m_animatedImage ? m_app->m_animatedImage->filePath : L"";
        const auto image=std::find(folder.filePaths.begin(),folder.filePaths.end(),active);
        m_app->m_currentIndex=image==folder.filePaths.end() ? -1 : static_cast<int>(std::distance(folder.filePaths.begin(),image));
        folder.currentIndex=m_app->m_currentIndex;
        m_app->m_thumbnailStrip.SetFileList(folder.filePaths); m_app->m_thumbnailStrip.SyncSelection(m_app->m_currentIndex);
        m_app->OnFileViewReady();
    }
    m_app->m_window.RequestRedraw();
}

void FolderDataManager::SortFolder(int folderIdx, SortMode mode) {
    if (folderIdx<0 || folderIdx>=static_cast<int>(m_folders.size())) return;
    if (m_app && folderIdx==m_currentFolderIdx) m_app->CancelHoverLoads();
    auto& folder=m_folders[folderIdx];
    auto snapshot=FolderScanService::View(folder.folderPath,folder.filePaths,static_cast<int>(mode));
    if (!snapshot.complete) return;
    std::unordered_map<std::wstring,PreviewItem> previews;
    for (auto& preview : folder.previews) previews.emplace(preview.filePath,std::move(preview));
    folder.filePaths=std::move(snapshot.files); folder.previews.clear();
    for (const auto& path : folder.filePaths) {
        const auto found=previews.find(path);
        if (found!=previews.end()) folder.previews.push_back(std::move(found->second));
        else { PreviewItem item; item.filePath=path; folder.previews.push_back(std::move(item)); }
    }
    NotifyPreviewLayoutChanged();
}

} // namespace kv
