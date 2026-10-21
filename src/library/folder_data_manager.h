// ============================================================================
// Class: FolderDataManager
// Description: FolderDataManager maintains the persistence of recent paths, favorites, and session folders browsed in [IMAGE_VIEWER_APP_NAME].
// Responsibility: Read/write `session.txt` and `fav.txt` tracking browsed directories.
// ============================================================================
#pragma once

#include "common/types.h"
#include "loader/image_loader.h"
#include <string>
#include <vector>
#include <memory>
#include <filesystem>

namespace kv {

class App; // Forward declaration

struct PreviewItem {
    std::wstring filePath;
    std::shared_ptr<ImageData> texture = nullptr; // decoded 180px/200px preview image
    RectF rect = {};                              // layout rect in screen coordinates
    bool isLoading = false;
    bool loadFailed = false;
};

struct FolderInfo {
    std::wstring folderPath;
    std::wstring folderName;
    std::vector<std::wstring> filePaths;
    int currentIndex = 0;
    
    // Preview thumbnails loaded for this folder
    std::vector<PreviewItem> previews;
    bool previewsInitialized = false;
    float scrollOffset = 0.0f;
    float targetScrollOffset = 0.0f;
};

class FolderDataManager {
public:
    friend class App;
    FolderDataManager() = default;
    ~FolderDataManager() = default;

    // Non-copyable
    FolderDataManager(const FolderDataManager&) = delete;
    FolderDataManager& operator=(const FolderDataManager&) = delete;

    void Initialize(App* app);

    // ─── Folder Management ───────────────────────────────────────────────────
    void AddFolder(const std::wstring& folderPath, bool libraryLoading = false, bool loadImage = true);
    void RemoveFolder(int index);
    int GetCurrentFolderIndex() const { return m_currentFolderIdx; }
    void SetCurrentFolderIndex(int index, bool loadImage = true);
    
    std::vector<FolderInfo>& GetFolders() { return m_folders; }
    const std::vector<FolderInfo>& GetFolders() const { return m_folders; }

    // ─── Session Management ──────────────────────────────────────────────────
    bool LoadSession(bool loadImage = true);
    bool SaveSession();

    // ─── Favorites Management ────────────────────────────────────────────────
    bool LoadFavorites();
    bool SaveFavorites();
    void ToggleFavorite(const std::wstring& filePath);
    bool IsFavorite(const std::wstring& filePath) const;
    const std::vector<std::wstring>& GetFavoritesList() const { return m_favorites; }

    // ─── Viewing History ─────────────────────────────────────────────────────
    void AddToHistory(const std::wstring& filePath);
    const std::vector<std::wstring>& GetHistoryList() const { return m_history; }

    // ─── Sorting ─────────────────────────────────────────────────────────────
    enum class SortMode { NameAsc, NameDesc, DateAsc, DateDesc, SizeAsc, SizeDesc };
    void SortFolder(int folderIdx, SortMode mode);

private:
    std::wstring GetSessionFilePath() const;
    std::wstring GetFavoritesFilePath() const;
    
    App* m_app = nullptr;
    std::vector<FolderInfo> m_folders;
    int m_currentFolderIdx = -1;
    
    std::vector<std::wstring> m_favorites;
    std::vector<std::wstring> m_history;
    static constexpr size_t MAX_HISTORY_SIZE = 100;
};

} // namespace kv
