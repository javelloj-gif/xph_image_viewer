// ============================================================================
// Class: FolderDataManager
// Description: Shared folder navigation and persistence for Image Viewer's pages.
// Responsibility: Own folder/preview state, selection, session, favorites and history.
// ============================================================================
#pragma once

#include "common/types.h"
#include "common/image_request.h"
#include "loader/image_loader.h"
#include "core/folder_scan_service.h"
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <unordered_map>

namespace kv {

class App; // Forward declaration

// Shared preview state; texture ownership is shared and all mutations occur on the UI thread.
struct PreviewItem {
    std::wstring filePath;
    std::shared_ptr<ImageData> texture = nullptr; // decoded shared preview image
    RectF rect = {};                              // layout rect in screen coordinates
    bool isLoading = false;
    bool loadFailed = false;
    ImageRequestToken pendingRequest; // Transient identity, moved with this preview; never persisted.
    ImageLoadLifetime loadLifetime; // Weakly borrowed by the loader; never persisted.
};

// Owns one folder's file list and preview/layout state, shared with pages through App.
struct FolderInfo {
    std::wstring folderPath;
    std::wstring folderName;
    std::vector<std::wstring> filePaths;
    int currentIndex = 0;
    std::vector<std::wstring> inventory; // Complete, unfiltered CPU paths.
    int appliedSort = -1, appliedFilter = -1;
    ImageLoadLifetime viewLifetime; // Cancels superseded worker projections.
    std::unordered_map<std::wstring,PreviewItem> hiddenPreviews; // Ready state retained only for hidden inventory paths; no pending leases.

    // Preview thumbnails loaded for this folder
    std::vector<PreviewItem> previews;
    bool previewsInitialized = false;
    float scrollOffset = 0.0f;
    float targetScrollOffset = 0.0f;
};

// App owns this service; all access is serialized on the UI thread (no internal locks).
// The optional App pointer is non-owning and must outlive calls that use it.
// Folder/preview mutators below automatically advance the transient layout revision when geometry inputs change.
class FolderDataManager {
    friend class AsyncPreviewTests;
    friend class StartupCacheTests; // Exercises production async generations/cache with isolated data.
public:
    friend class App;
    // Creates empty state; destruction releases owned state without saving it.
    FolderDataManager() = default;
    // Joins enumeration before model/callback storage is destroyed.
    ~FolderDataManager();
    // UI owner: enables dedicated-worker enumeration; isolated non-window consumers retain synchronous AddFolder.
    void EnableAsync();
    // UI: drains worker snapshots and applies only current navigation requests.
    void ProcessPending();
    // UI: cancels enumeration and callbacks before HWND/loader teardown, preserving pending session records.
    void StopScanning();
    // UI read: pending descriptors keep startup from falling back to an unrelated current directory.
    bool HasPendingFolders() const { return !m_pendingFolders.empty(); }
    // UI: prevents a snapshot started before a known deletion from reintroducing that file into a model.
    void NotifyImageRemoved(const std::wstring& path);
    // UI: invalidates attribute/order caches after known metadata writes; reprojects only the active folder.
    void NotifyAttributesChanged();

    // Keeps state ownership and App's aliases bound to a single service instance.
    FolderDataManager(const FolderDataManager&) = delete;
    FolderDataManager& operator=(const FolderDataManager&) = delete;

    // Binds an optional App and clears in-memory folders, selection, favorites and history; no disk I/O.
    void Initialize(App* app);

    // ─── Folder Management ───────────────────────────────────────────────────
    // UI: queues CPU enumeration/sort when async is enabled, otherwise scans synchronously; ignores empty folders.
    // Restoration selects only the first successful no-argument session folder. Explicit requests select
    // only their current navigation generation; new-folder saves also preserve unresolved session records.
    void AddFolder(const std::wstring& folderPath, bool restoringSession = false, bool loadImage = true);
    // Erases a valid folder (canceling its leases/hover), clamps selection, saves and redraws; does not reload App's file list.
    void RemoveFolder(int index);
    // Returns the shared folder index, or -1 when no folder is selected.
    int GetCurrentFolderIndex() const { return m_currentFolderIdx; }
    // Selects a valid index or -1; with App, copies files/selection, updates the strip and optionally loads.
    // Requests redraw without saving; selecting -1 leaves App's existing file list untouched.
    // A different selection cancels outstanding shared preview/hover requests before replacing the active list.
    void SetCurrentFolderIndex(int index, bool loadImage = true);

    // Borrows the owned folder vector on the UI thread; structural/order/dimension mutations must notify below.
    std::vector<FolderInfo>& GetFolders() { return m_folders; }
    const std::vector<FolderInfo>& GetFolders() const { return m_folders; }

    // UI-thread transient epoch for the preview layout; never persisted or reset during reinitialization.
    uint64_t GetPreviewLayoutRevision() const { return m_previewLayoutRevision; }
    // UI thread: call after changing folders, preview order/structure or effective texture dimensions (not scroll/hover).
    void NotifyPreviewLayoutChanged() { ++m_previewLayoutRevision; }

    // UI: cancels all in-flight previews, preserving textures/failure state and allowing fresh requests.
    void CancelPreviewLoads();

    // ─── Session Management ──────────────────────────────────────────────────
    // Appends missing folders from UTF-8 four-line records, skipping invalid entries without saving.
    // Optional startup folder is queued first, once, while retaining the original session record order.
    // Async callbacks remain deferred to ProcessPending; loadImage controls no-argument restoration.
    bool LoadSession(bool loadImage = true, const std::wstring& initialFolder = {}, bool initialLoadImage = true);
    // Writes folder paths and historical placeholders as four-line UTF-8 records; false on open/create failure.
    bool SaveSession();

    // ─── Favorites Management ────────────────────────────────────────────────
    // Reads four-line UTF-8 records into favorites; missing/short/unopenable input leaves state unchanged.
    bool LoadFavorites();
    // Writes current favorites as four-line UTF-8 records; false on open/create failure.
    bool SaveFavorites();
    // Adds/removes an exact path match in memory and attempts to save favorites immediately.
    void ToggleFavorite(const std::wstring& filePath);
    // Tests an exact path match in the current in-memory favorites.
    bool IsFavorite(const std::wstring& filePath) const;
    // Borrows the favorites vector; callers must keep access on the UI thread.
    const std::vector<std::wstring>& GetFavoritesList() const { return m_favorites; }

    // ─── Viewing History ─────────────────────────────────────────────────────
    // Moves an exact path match to the front, keeping at most 100 entries in memory; no disk I/O.
    void AddToHistory(const std::wstring& filePath);
    // Borrows the most-recent-first history; no persistence is implied.
    const std::vector<std::wstring>& GetHistoryList() const { return m_history; }

    // ─── Sorting ─────────────────────────────────────────────────────────────
    // Values are shared with persisted settings.sort_mode; preserve their order.
    enum class SortMode { NameAsc, NameDesc, DateAsc, DateDesc, SizeAsc, SizeDesc };
    // Synchronous isolated-consumer adapter; production preference changes use ApplyView.
    // Reorders files/previews with the shared CPU comparator, cancels hover, and neither saves nor redraws.
    void SortFolder(int folderIdx, SortMode mode);
    // UI: projects the selected folder on the worker, retaining the old list until atomic publication.
    void ApplyView(int folderIdx);
    // UI: reconciles previews by path and only updates navigation for the currently selected folder.
    void CompleteView(const std::wstring& path, uint64_t mutation, FolderSnapshot snapshot);

    // UI-only read: matches token and exact path across current folders; no loading/layout mutation or borrowed address.
    bool IsPreviewRequestCurrent(const ImageRequestToken& request) const;

    // UI thread: resolves token and exact path in current folders, applies success/failure once,
    // and ends loading; a texture invalidates the preview layout. False for obsolete requests; no redraw or I/O.
    bool CompletePreview(const ImageRequestToken& request, std::shared_ptr<ImageData> image);

private:
    // UI-only descriptor preserves a requested/restored folder until its worker completes or it is canceled.
    struct PendingFolder { std::wstring path; bool restoring, loadImage; uint64_t generation, fileMutation; ImageLoadLifetime lifetime; };
    // UI: enqueues/reprioritizes a folder without reading its contents or borrowing model addresses.
    void RequestFolder(const std::wstring& path, bool restoring, bool loadImage);
    // UI: publishes a complete snapshot, guarding navigation against superseded requests and sort changes.
    void CompleteFolder(const ImageRequestToken& request, FolderSnapshot snapshot);
    std::unique_ptr<FolderScanService> m_scanner;
    std::vector<PendingFolder> m_pendingFolders;
    std::vector<std::wstring> m_sessionOrder;
    uint64_t m_navigationGeneration = 0;
    bool m_restoreSelect = false;
    uint64_t m_fileMutation = 0;
    std::vector<std::pair<std::wstring,uint64_t>> m_removedDuringScan;
    // Resolves session.txt on each call using the shared user_data search and EXE fallback.
    std::wstring GetSessionFilePath() const;
    // Resolves fav.txt on each call using the shared user_data search and EXE fallback.
    std::wstring GetFavoritesFilePath() const;

    App* m_app = nullptr; // Non-owning; App owns this manager and its UI consumers.
    std::vector<FolderInfo> m_folders;
    uint64_t m_previewLayoutRevision = 0; // UI-only invalidation epoch, independent of persisted data.
    int m_currentFolderIdx = -1;

    std::vector<std::wstring> m_favorites;
    std::vector<std::wstring> m_history;
    static constexpr size_t MAX_HISTORY_SIZE = 100;
};

} // namespace kv
