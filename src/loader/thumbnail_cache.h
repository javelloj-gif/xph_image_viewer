#pragma once
#include <filesystem>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <thread>
#include <string>
#include <atomic>
#include <cstdint>
#include <vector>
#include <algorithm>

struct sqlite3;
namespace kv {
struct ImageData;
// Disposable exact-pixel thumbnail store. Workers read/touch recency; one bounded writer commits pixels.
// Contains no GPU/UI objects. Cache failure never prevents loading the source image.
class ThumbnailCache {
    friend class StartupCacheTests; // Exercises production async generations/cache with isolated data.
public:
    // Does not open files; database initialization is lazy on the first worker request.
    ThumbnailCache() = default;
    // Stops the writer before releasing database state; queued writes may be discarded.
    ~ThumbnailCache();
    // UI owner: resolves an already chosen private cache directory and starts one writer.
    void Initialize(const std::filesystem::path& root, uint64_t maximumBytes = 1024ull*1024*1024);
    // UI owner: discards queued writes and joins an in-progress small transaction/compression.
    void Shutdown();
    // Worker: returns an attribute/file-identity signature, or empty when source validation fails.
    static std::wstring Signature(const std::wstring& path);
    // Worker: database contention waits at most 10 ms; caller also verifies source signature after reading.
    std::shared_ptr<ImageData> Read(const std::wstring& path, const std::wstring& signature, uint32_t size);
    // Worker: best-effort bounded CPU-only submission; never retains GPU state or blocks on writes.
    void Store(const std::wstring& signature, uint32_t size, const ImageData& image);
    // Read-only counters for tests/diagnostics, independent of image validity and UI state.
    uint64_t Hits() const { return m_hits.load(); }
    uint64_t Misses() const { return m_misses.load(); }
    // Pure quota policy; metadata/journal reserve is deducted before accepting a cache write.
    static uint64_t Budget(uint64_t availableBytes) { return std::min<uint64_t>(1024ull*1024*1024,availableBytes/100); }
private:
    // One pending thumbnail owns only pixels, dimensions and its immutable source signature.
    struct Write { std::wstring key; uint32_t size, width, height; bool alpha; std::vector<uint8_t> pixels; };
    // Called under database lock on a worker; rollback journal and auto-vacuum keep accounting bounded.
    bool Open();
    // Writer-only: compresses exact bytes, reclaims LRU entries, commits or abandons on any error.
    void Commit(const Write& write);
    // Dedicated writer consumes at most 8 MiB of queued bytes, with cooperative terminal stop.
    void WriterLoop();
    std::filesystem::path m_root;
    uint64_t m_maximumBytes = 1024ull*1024*1024;
    sqlite3* m_db = nullptr;
    std::timed_mutex m_dbMutex;
    std::mutex m_writeMutex;
    std::condition_variable m_cv;
    std::deque<Write> m_writes;
    size_t m_queuedBytes = 0;
    std::atomic<bool> m_stop = true;
    std::thread m_writer;
    std::atomic<uint64_t> m_hits{0}, m_misses{0};
};
} // namespace kv
