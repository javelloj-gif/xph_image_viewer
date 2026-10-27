// ============================================================================
// Class: AsyncDirectoryScanner
// Description: AsyncDirectoryScanner manages thread-pool directory scans to locate tagged images asynchronously in Fotassa.
// Responsibility: Scan files recursively on a background manager thread and parse cache-missed files in parallel using CPU-bounded threads.
// ============================================================================
#pragma once

#include "common/types.h"
#include "metadata_cache.h"
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <wincodec.h>

namespace kv {

using SearchMatchCallback = std::function<void(const std::wstring& filePath, const std::vector<std::wstring>& tags)>;
using SearchFinishedCallback = std::function<void(size_t totalScanned, size_t totalMatches)>;

class AsyncDirectoryScanner {
public:
    AsyncDirectoryScanner() = default;
    ~AsyncDirectoryScanner();

    // Owner thread: joins previous work, then scans with borrowed factory/cache alive through CancelSearch.
    // Match/finished callbacks run on scanner threads; finished is notified once even on cancellation.
    void SearchTagsAsync(
        IWICImagingFactory2* wicFactory,
        MetadataCache& cache,
        const std::wstring& rootDirectory,
        const std::vector<std::wstring>& queryTags,
        SearchMatchCallback onMatchFound,
        SearchFinishedCallback onSearchFinished
    );

    // Requests cooperative stop and joins on owner thread; an already admitted callback/WIC read may finish.
    // Callback-thread calls only request stop; they must not destroy/restart the scanner from that callback.
    void CancelSearch();
    // Observes scanner status atomically; join still establishes final callback/owner lifetime completion.
    bool IsSearchRunning() const;

private:
    static bool MatchTags(const std::vector<std::wstring>& fileTags, const std::vector<std::wstring>& queryTags);

    std::thread m_searchThread;
    mutable std::mutex m_searchMutex;
    std::mutex m_callbackMutex;
    std::atomic<bool> m_searchCancelled{false};
    std::atomic<bool> m_searchRunning{false};
};

} // namespace kv
