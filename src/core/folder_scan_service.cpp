#include "common/pch.h"
#include "folder_scan_service.h"
#include "loader/image_loader.h"
#include "common/performance_trace.h"

namespace kv {
FolderScanService::FolderScanService() : m_thread(&FolderScanService::Run, this) { }
FolderScanService::~FolderScanService() { Shutdown(); }
void FolderScanService::Shutdown() {
    { std::lock_guard lock(m_mutex); m_stopped = true; m_stop.request_stop(); m_jobs.clear(); }
    m_cv.notify_all();
    if (m_thread.joinable()) m_thread.join();
    m_pending.clear();
}
void FolderScanService::Request(std::wstring path, int sortMode, bool foreground, ImageCancellation cancel, std::function<void(FolderSnapshot)> callback) {
    std::lock_guard lock(m_mutex);
    if (m_stopped) return;
    cancel.shutdown = m_stop.get_token();
    Job job{std::move(path), sortMode, foreground, cancel, std::move(callback)};
    auto where = foreground ? std::find_if(m_jobs.begin(), m_jobs.end(), [](const Job& item) { return !item.foreground; }) : m_jobs.end();
    m_jobs.insert(where, std::move(job)); m_cv.notify_one();
}
void FolderScanService::ProcessPending() {
    std::vector<Completion> results;
    { std::lock_guard lock(m_mutex); results.swap(m_pending); }
    for (auto& result : results)
        if (!result.job.cancel.IsCancelled() && result.job.callback) result.job.callback(std::move(result.snapshot));
}
void FolderScanService::Run() {
    for (;;) {
        Job job;
        { std::unique_lock lock(m_mutex); m_cv.wait(lock, [this] { return m_stopped || !m_jobs.empty(); });
            if (m_stopped) return;
            job = std::move(m_jobs.front()); m_jobs.pop_front(); }
        FolderSnapshot snapshot;
        try { snapshot = Scan(job.path, job.sortMode, job.cancel); } catch (...) { snapshot.path = job.path; }
        if (job.cancel.IsCancelled()) continue;
        std::lock_guard lock(m_mutex);
        if (!m_stopped) m_pending.push_back({std::move(snapshot), std::move(job)});
    }
}
FolderSnapshot FolderScanService::Scan(const std::wstring& path, int sortMode, ImageCancellation cancel) {
    FolderSnapshot result{path, {}, false, sortMode};
    PerformanceSpan enumeration("folder.enumerate.begin", "folder.enumerate.end");
    // Initial lexical order and the existing sort comparator preserve previous tie behavior.
    try {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (cancel.IsCancelled()) return result;
            if (entry.is_regular_file() && ImageLoader::IsSupported(entry.path().wstring())) result.files.push_back(entry.path().wstring());
        }
        std::sort(result.files.begin(), result.files.end());
        PerformanceSpan sorting("folder.order.begin", "folder.order.end");
        // Each file's expensive sort attributes are read once off the UI thread.
        struct Key { std::wstring path, name; std::filesystem::file_time_type date{}; uint64_t size=0; };
        std::vector<Key> keys; keys.reserve(result.files.size());
        for (const auto& file : result.files) {
            if (cancel.IsCancelled()) return result;
            Key key; key.path=file; key.name=std::filesystem::path(file).filename().wstring();
            std::transform(key.name.begin(), key.name.end(), key.name.begin(), ::towlower);
            try { if (sortMode==2 || sortMode==3) key.date=std::filesystem::last_write_time(file); } catch (...) { }
            try { if (sortMode==4 || sortMode==5) key.size=std::filesystem::file_size(file); } catch (...) { }
            keys.push_back(std::move(key));
        }
        std::sort(keys.begin(), keys.end(), [sortMode](const Key& a, const Key& b) {
            switch (sortMode) {
                case 1: return a.name>b.name;
                case 2: return a.date<b.date;
                case 3: return a.date>b.date;
                case 4: return a.size<b.size;
                case 5: return a.size>b.size;
                default: return a.name<b.name;
            }
        });
        result.files.clear();
        for (auto& key : keys) result.files.push_back(std::move(key.path));
        result.complete = !cancel.IsCancelled();
        PerformanceTrace::Event("folder.enumerated", 0, result.files.size());
    } catch (...) { result.files.clear(); }
    return result;
}
} // namespace kv
