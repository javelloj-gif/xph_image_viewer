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
void FolderScanService::Request(std::wstring path, int sortMode, bool foreground, ImageCancellation cancel, std::function<void(FolderSnapshot)> callback, int filterMask) {
    std::lock_guard lock(m_mutex);
    if (m_stopped) return;
    cancel.shutdown = m_stop.get_token();
    Job job; job.path=std::move(path); job.sortMode=sortMode; job.foreground=foreground; job.cancel=cancel; job.callback=std::move(callback); job.filterMask=filterMask;
    auto where = foreground ? std::find_if(m_jobs.begin(), m_jobs.end(), [](const Job& item) { return !item.foreground; }) : m_jobs.end();
    m_jobs.insert(where, std::move(job)); m_cv.notify_one();
}
int FolderScanService::FormatBit(const std::wstring& path) {
    auto ext=std::filesystem::path(path).extension().wstring();
    std::transform(ext.begin(),ext.end(),ext.begin(),::towlower);
    if (ext==L".jpg" || ext==L".jpeg" || ext==L".jfif") return 1;
    if (ext==L".png" || ext==L".apng") return 2;
    if (ext==L".gif") return 4;
    if (ext==L".bmp") return 8;
    if (ext==L".tif" || ext==L".tiff") return 16;
    if (ext==L".webp") return 32;
    if (ext==L".avif" || ext==L".avf") return 64;
    if (ext==L".heif" || ext==L".heic") return 128;
    return ImageLoader::IsSupported(path) ? 256 : 0;
}
void FolderScanService::RequestView(std::wstring path, std::vector<std::wstring> inventory,
    int sortMode, int filterMask, uint64_t revision, ImageCancellation cancel, std::function<void(FolderSnapshot)> callback) {
    std::lock_guard lock(m_mutex);
    if (m_stopped) return;
    cancel.shutdown=m_stop.get_token();
    Job job; job.path=std::move(path); job.inventory=std::move(inventory); job.sortMode=sortMode;
    job.filterMask=filterMask; job.revision=revision; job.cancel=cancel; job.callback=std::move(callback);
    job.project=true; job.foreground=true;
    std::erase_if(m_jobs,[](const Job& queued) { return queued.project && queued.cancel.IsCancelled(); });
    m_jobs.push_front(std::move(job)); m_cv.notify_one();
}
FolderSnapshot FolderScanService::Project(const Job& job) { return BuildProjection(job,m_catalogs[job.path]); }
FolderSnapshot FolderScanService::BuildProjection(const Job& job, Catalog& catalog) {
    FolderSnapshot result; result.path=job.path; result.sortMode=job.sortMode; result.filterMask=job.filterMask;
    if (catalog.inventory!=job.inventory || catalog.revision!=job.revision) {
        Catalog next; next.inventory=job.inventory; next.revision=job.revision;
        for (const auto& path : job.inventory) {
            if (job.cancel.IsCancelled()) return result;
            FileKey key; key.path=path; key.name=std::filesystem::path(path).filename().wstring();
            std::transform(key.name.begin(),key.name.end(),key.name.begin(),::towlower);
            key.format=FormatBit(path); next.formats.emplace(path,key.format); next.keys.push_back(std::move(key));
        }
        catalog=std::move(next);
    }
    const int mode=std::clamp(job.sortMode,0,5);
    const bool dates=mode==2 || mode==3, sizes=mode==4 || mode==5;
    if ((dates && !catalog.dates) || (sizes && !catalog.sizes)) {
        for (auto& key : catalog.keys) {
            if (job.cancel.IsCancelled()) return result;
            std::error_code error;
            if (dates) { key.date=std::filesystem::last_write_time(key.path,error); if (error) key.date={}; }
            if (sizes) { key.size=std::filesystem::file_size(key.path,error); if (error) key.size=0; }
        }
        if (dates) catalog.dates=true;
        if (sizes) catalog.sizes=true;
    }
    auto& order=catalog.orders[mode];
    if (order.empty() && !catalog.keys.empty()) {
        PerformanceSpan span("folder.sort.begin","folder.sort.end");
        std::vector<size_t> indices(catalog.keys.size());
        for (size_t i=0;i<indices.size();++i) indices[i]=i;
        std::sort(indices.begin(),indices.end(),[&](size_t x,size_t y) {
            const auto& a=catalog.keys[x]; const auto& b=catalog.keys[y];
            if (dates && a.date!=b.date) return mode==2 ? a.date<b.date : a.date>b.date;
            if (sizes && a.size!=b.size) return mode==4 ? a.size<b.size : a.size>b.size;
            if (a.name!=b.name) return mode==1 ? a.name>b.name : a.name<b.name;
            return a.path<b.path;
        });
        if (job.cancel.IsCancelled()) return result;
        for (auto index : indices) order.push_back(catalog.keys[index].path);
    }
    result.inventory=catalog.inventory;
    for (const auto& path : order) {
        if (job.cancel.IsCancelled()) return result;
        if (catalog.formats.at(path)&job.filterMask) result.files.push_back(path);
    }
    result.complete=true;
    return result;
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
        try {
            if (!job.project) {
                PerformanceSpan enumeration("folder.enumerate.begin","folder.enumerate.end");
                for (const auto& entry : std::filesystem::directory_iterator(job.path)) {
                    if (job.cancel.IsCancelled()) break;
                    if (entry.is_regular_file() && ImageLoader::IsSupported(entry.path().wstring())) job.inventory.push_back(entry.path().wstring());
                }
                m_catalogs.erase(job.path);
                snapshot = Project(job);
                PerformanceTrace::Event("folder.enumerated",0,snapshot.inventory.size());
            } else snapshot = Project(job);
        } catch (...) { m_catalogs.erase(job.path); snapshot.path = job.path; }
        if (job.cancel.IsCancelled()) continue;
        std::lock_guard lock(m_mutex);
        if (!m_stopped) m_pending.push_back({std::move(snapshot), std::move(job)});
    }
}
FolderSnapshot FolderScanService::View(const std::wstring& path, std::vector<std::wstring> inventory, int sortMode, int filterMask, ImageCancellation cancel) {
    Job job; job.path=path; job.inventory=std::move(inventory); job.sortMode=sortMode; job.filterMask=filterMask; job.cancel=cancel;
    Catalog catalog; return BuildProjection(job,catalog);
}
FolderSnapshot FolderScanService::Scan(const std::wstring& path, int sortMode, ImageCancellation cancel) {
    PerformanceSpan enumeration("folder.enumerate.begin","folder.enumerate.end");
    std::vector<std::wstring> inventory;
    try {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (cancel.IsCancelled()) return {};
            if (entry.is_regular_file() && ImageLoader::IsSupported(entry.path().wstring())) inventory.push_back(entry.path().wstring());
        }
        return View(path,std::move(inventory),sortMode,511,cancel);
    } catch (...) { return {}; }
}
} // namespace kv
