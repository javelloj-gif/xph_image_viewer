// ============================================================================
// Class: AsyncDirectoryScanner
// Description: AsyncDirectoryScanner manages thread-pool directory scans to locate tagged images asynchronously in Fotassa.
// Responsibility: Scan files recursively on a background manager thread and parse cache-missed files in parallel using CPU-bounded threads.
// ============================================================================
#include "common/pch.h"
#include "async_directory_scanner.h"
#include "wic_metadata_reader_writer.h"
#include "loader/image_loader.h"
#include "common/types.h"
#include <filesystem>
#include <format>
#include <algorithm>

namespace kv {
namespace {
thread_local const AsyncDirectoryScanner* callbackScanner = nullptr;
// Marks callbacks on both manager and nested workers so cancellation never joins a thread waiting on itself.
class ScannerCallbackScope {
public:
    // Restores an outer callback marker when scopes unwind, including exceptional exit.
    explicit ScannerCallbackScope(const AsyncDirectoryScanner* scanner) : previous(callbackScanner) { callbackScanner = scanner; }
    ~ScannerCallbackScope() { callbackScanner = previous; }
private:
    const AsyncDirectoryScanner* previous;
};
}

AsyncDirectoryScanner::~AsyncDirectoryScanner() {
    CancelSearch();
}

bool AsyncDirectoryScanner::MatchTags(const std::vector<std::wstring>& fileTags, const std::vector<std::wstring>& queryTags) {
    if (queryTags.empty()) return true;
    for (const auto& q : queryTags) {
        bool found = false;
        for (const auto& ft : fileTags) {
            if (_wcsicmp(q.c_str(), ft.c_str()) == 0) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}

void AsyncDirectoryScanner::SearchTagsAsync(
    IWICImagingFactory2* wicFactory,
    MetadataCache& cache,
    const std::wstring& rootDirectory,
    const std::vector<std::wstring>& queryTags,
    SearchMatchCallback onMatchFound,
    SearchFinishedCallback onSearchFinished
) {
    if (callbackScanner == this) return; // Lifecycle/restart is owner-only, not reentrant from callbacks.
    std::wstring rootDir = MetadataCache::NormalizePath(rootDirectory);
    std::lock_guard<std::mutex> lock(m_searchMutex);
    
    CancelSearch();
    
    m_searchCancelled.store(false);
    m_searchRunning.store(true);
    
    m_searchThread = std::thread([this, wicFactory, &cache, rootDir, queryTags, onMatchFound, onSearchFinished]() {
        std::atomic<size_t> totalScanned{0};
        std::atomic<size_t> totalMatches{0};
        
        std::vector<std::wstring> filesToScan;
        
        // 1. Directory Discovery
        try {
            if (std::filesystem::exists(rootDir) && std::filesystem::is_directory(rootDir)) {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(rootDir, std::filesystem::directory_options::skip_permission_denied)) {
                    if (m_searchCancelled.load()) break;
                    
                    if (entry.is_regular_file()) {
                        std::wstring path = MetadataCache::NormalizePath(entry.path().wstring());
                        if (ImageLoader::IsSupported(path)) {
                            filesToScan.push_back(path);
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            std::string errStr = e.what();
            std::wstring errWStr(errStr.begin(), errStr.end());
            LogError(std::format(L"AsyncDirectoryScanner: Error discovering files: {}", errWStr));
        }
        
        if (m_searchCancelled.load()) {
            m_searchRunning.store(false);
            if (onSearchFinished) {
                ScannerCallbackScope scope(this);
                onSearchFinished(0, 0);
            }
            return;
        }
        
        std::vector<std::wstring> cacheMisses;
        
        // 2. Cache Checking
        for (const auto& filePath : filesToScan) {
            if (m_searchCancelled.load()) break;
            
            int64_t currentWriteTime = cache.GetLastWriteTime(filePath);
            if (m_searchCancelled.load()) break;
            bool cacheHit = false;
            std::vector<std::wstring> tags;
            
            {
                std::lock_guard<std::mutex> cacheLock(cache.m_cacheMutex);
                auto it = cache.m_cache.find(filePath);
                if (it != cache.m_cache.end() && it->second.lastWriteTime == currentWriteTime) {
                    tags = it->second.tags;
                    cacheHit = true;
                }
            }
            
            if (cacheHit) {
                totalScanned.fetch_add(1);
                if (MatchTags(tags, queryTags)) {
                    totalMatches.fetch_add(1);
                    if (onMatchFound) {
                        std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                        if (!m_searchCancelled.load()) {
                            ScannerCallbackScope scope(this);
                            onMatchFound(filePath, tags);
                        }
                    }
                }
            } else {
                cacheMisses.push_back(filePath);
            }
        }
        
        // 3. Parallel Worker Thread Pool (for Cache Misses)
        if (!cacheMisses.empty() && !m_searchCancelled.load()) {
            size_t numThreads = std::thread::hardware_concurrency();
            if (numThreads == 0) numThreads = 2;
            if (numThreads > cacheMisses.size()) numThreads = cacheMisses.size();
            
            std::atomic<size_t> nextIndex{0};
            std::vector<std::thread> workers;
            workers.reserve(numThreads);
            
            for (size_t t = 0; t < numThreads; ++t) {
                workers.push_back(std::thread([this, wicFactory, &cache, &cacheMisses, &nextIndex, &queryTags, &totalScanned, &totalMatches, onMatchFound]() {
                    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
                    bool comInitialized = SUCCEEDED(hr);
                    
                    while (!m_searchCancelled.load()) {
                        size_t index = nextIndex.fetch_add(1);
                        if (index >= cacheMisses.size()) break;
                        
                        const auto& filePath = cacheMisses[index];
                        std::vector<std::wstring> tags;
                        
                        if (WicMetadataReaderWriter::ReadTags(wicFactory, filePath, tags)) {
                            if (m_searchCancelled.load()) break;
                            int64_t writeTime = cache.GetLastWriteTime(filePath);
                            
                            {
                                std::lock_guard<std::mutex> cacheLock(cache.m_cacheMutex);
                                if (m_searchCancelled.load()) break;
                                cache.m_cache[filePath] = MetadataCache::CacheEntry{ writeTime, tags };
                                cache.m_cacheDirty = true;
                            }
                            
                            totalScanned.fetch_add(1);
                            if (MatchTags(tags, queryTags)) {
                                totalMatches.fetch_add(1);
                                if (onMatchFound) {
                                    std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                                    if (!m_searchCancelled.load()) {
                                        ScannerCallbackScope scope(this);
                                        onMatchFound(filePath, tags);
                                    }
                                }
                            }
                        }
                    }
                    
                    if (comInitialized) {
                        CoUninitialize();
                    }
                }));
            }
            
            for (auto& worker : workers) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
        }
        
        bool save = false;
        {
            std::lock_guard lock(cache.m_cacheMutex);
            save = cache.m_cacheDirty && !m_searchCancelled.load();
        }
        if (save && !m_searchCancelled.load()) {
            cache.SaveCache();
        }
        
        m_searchRunning.store(false);
        if (onSearchFinished) {
            ScannerCallbackScope scope(this);
            onSearchFinished(totalScanned.load(), totalMatches.load());
        }
    });
}

void AsyncDirectoryScanner::CancelSearch() {
    m_searchCancelled.store(true);
    if (callbackScanner == this) return;
    if (m_searchThread.joinable() && std::this_thread::get_id() != m_searchThread.get_id()) {
        m_searchThread.join();
    }
    // A callback-thread request returns without joining itself; the manager finishes its own epilogue.
    if (!m_searchThread.joinable() || std::this_thread::get_id() != m_searchThread.get_id()) {
        m_searchRunning.store(false);
    }
}

bool AsyncDirectoryScanner::IsSearchRunning() const {
    return m_searchRunning.load();
}

} // namespace kv
