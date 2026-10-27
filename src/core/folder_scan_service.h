#pragma once
#include "common/image_request.h"
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include <array>
#include <filesystem>
#include <unordered_map>

namespace kv {
// CPU-only snapshot: no folder/model addresses or GPU resources cross the worker boundary.
struct FolderSnapshot {
    std::wstring path;
    std::vector<std::wstring> files;
    bool complete = false;
    int sortMode = 0;
    int filterMask = 511;
    std::vector<std::wstring> inventory;
};
// One dedicated enumeration worker; callbacks execute only when the UI drains completions.
class FolderScanService {
public:
    // Starts one worker; UI owner must stop it before destroying callback consumers.
    FolderScanService();
    // Cooperatively stops enumeration, drops pending callbacks and joins its active filesystem call.
    ~FolderScanService();
    // UI: foreground requests precede unstarted restoration jobs, preserving FIFO within each class.
    void Request(std::wstring path, int sortMode, bool foreground, ImageCancellation cancel, std::function<void(FolderSnapshot)> callback, int filterMask = 511);
    // UI: queues a projection of an existing inventory; worker caches keys/orders until the mutation revision changes.
    void RequestView(std::wstring path, std::vector<std::wstring> inventory, int sortMode, int filterMask,
        uint64_t revision, ImageCancellation cancel, std::function<void(FolderSnapshot)> callback);
    // CPU-only extension classification; bit 8 contains all other supported image extensions.
    static int FormatBit(const std::wstring& path);
    // UI: applies only completions whose cancellation lease is still alive.
    void ProcessPending();
    // UI: terminal, idempotent stop; never invokes a callback while joining.
    void Shutdown();
    // CPU-only scan/sort shared by worker and isolated tests; no source files are written.
    static FolderSnapshot Scan(const std::wstring& path, int sortMode, ImageCancellation cancel = {});
    // CPU-only synchronous adapter for isolated consumers; shares the worker comparator and filtering contract.
    static FolderSnapshot View(const std::wstring& path, std::vector<std::wstring> inventory,
        int sortMode, int filterMask = 511, ImageCancellation cancel = {});
private:
    // Immutable job input plus non-owning UI callback; workers do not invoke it.
    struct Job { std::wstring path; int sortMode; bool foreground; ImageCancellation cancel; std::function<void(FolderSnapshot)> callback;
        bool project = false; int filterMask = 511; uint64_t revision = 0; std::vector<std::wstring> inventory; };
    // Worker-only cached attributes and six orders; never shares mutable state or graphics with UI.
    struct FileKey { std::wstring path, name; int format = 0; std::filesystem::file_time_type date{}; uint64_t size = 0; };
    struct Catalog { std::vector<FileKey> keys; std::vector<std::wstring> inventory;
        std::unordered_map<std::wstring,int> formats;
        std::array<std::vector<std::wstring>, 6> orders; bool dates = false, sizes = false; uint64_t revision = 0; };
    // Worker: builds a projection without filesystem calls inside the comparator; cancellation prevents publication.
    FolderSnapshot Project(const Job& job);
    // CPU-only common projection; caller owns the cache and never shares it with UI consumers.
    static FolderSnapshot BuildProjection(const Job& job, Catalog& catalog);
    std::unordered_map<std::wstring, Catalog> m_catalogs;
    // Completion retains the CPU snapshot until UI delivery or shutdown.
    struct Completion { FolderSnapshot snapshot; Job job; };
    // Dedicated CPU loop; lifetime cancellation is checked between filesystem/sort stages.
    void Run();
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<Job> m_jobs;
    std::vector<Completion> m_pending;
    std::stop_source m_stop;
    bool m_stopped = false;
    std::thread m_thread;
};
} // namespace kv
