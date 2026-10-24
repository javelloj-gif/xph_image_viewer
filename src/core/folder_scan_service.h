#pragma once
#include "common/image_request.h"
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace kv {
// CPU-only snapshot: no folder/model addresses or GPU resources cross the worker boundary.
struct FolderSnapshot {
    std::wstring path;
    std::vector<std::wstring> files;
    bool complete = false;
    int sortMode = 0;
};
// One dedicated enumeration worker; callbacks execute only when the UI drains completions.
class FolderScanService {
public:
    // Starts one worker; UI owner must stop it before destroying callback consumers.
    FolderScanService();
    // Cooperatively stops enumeration, drops pending callbacks and joins its active filesystem call.
    ~FolderScanService();
    // UI: foreground requests precede unstarted restoration jobs, preserving FIFO within each class.
    void Request(std::wstring path, int sortMode, bool foreground, ImageCancellation cancel, std::function<void(FolderSnapshot)> callback);
    // UI: applies only completions whose cancellation lease is still alive.
    void ProcessPending();
    // UI: terminal, idempotent stop; never invokes a callback while joining.
    void Shutdown();
    // CPU-only scan/sort shared by worker and isolated tests; no source files are written.
    static FolderSnapshot Scan(const std::wstring& path, int sortMode, ImageCancellation cancel = {});
private:
    // Immutable job input plus non-owning UI callback; workers do not invoke it.
    struct Job { std::wstring path; int sortMode; bool foreground; ImageCancellation cancel; std::function<void(FolderSnapshot)> callback; };
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
