#include "common/pch.h"
#include "thumbnail_cache.h"
#include "image_loader.h"
#include "common/performance_trace.h"
#include <winsqlite/winsqlite3.h>
#include <compressapi.h>

namespace kv {
namespace {
// Scoped statement used only while the cache database mutex is held.
struct Statement {
    sqlite3_stmt* value = nullptr;
    Statement(sqlite3* db, const char* sql) { sqlite3_prepare_v2(db, sql, -1, &value, nullptr); }
    ~Statement() { sqlite3_finalize(value); }
};
// Stable byte checksum detects corrupt/unexpected payloads after lossless decompression.
uint64_t Checksum(const std::vector<uint8_t>& bytes) {
    uint64_t hash = 14695981039346656037ull;
    for (auto byte : bytes) { hash ^= byte; hash *= 1099511628211ull; }
    return hash;
}
// Reads a scalar pragma/query; negative means failure and is never used as a size.
sqlite3_int64 Scalar(sqlite3* db, const char* sql) {
    Statement query(db, sql);
    return query.value && sqlite3_step(query.value) == SQLITE_ROW ? sqlite3_column_int64(query.value, 0) : -1;
}
// Owns one Compression API handle; buffer mode retains exact BGRA bytes, including premultiplied alpha.
struct Compression {
    COMPRESSOR_HANDLE compressor = nullptr;
    DECOMPRESSOR_HANDLE decompressor = nullptr;
    ~Compression() { if (compressor) CloseCompressor(compressor); if (decompressor) CloseDecompressor(decompressor); }
};
}
ThumbnailCache::~ThumbnailCache() { Shutdown(); }
void ThumbnailCache::Initialize(const std::filesystem::path& root, uint64_t maximumBytes) {
    Shutdown();
    m_root = root;
    m_maximumBytes=std::min<uint64_t>(maximumBytes,1024ull*1024*1024);
    m_stop = false;
    try { m_writer = std::thread(&ThumbnailCache::WriterLoop, this); }
    catch (...) { m_stop=true; m_root.clear(); throw; }
}
void ThumbnailCache::Shutdown() {
    { std::lock_guard lock(m_writeMutex); m_stop = true; m_writes.clear(); m_touches.clear(); m_queuedBytes = 0; }
    m_cv.notify_all();
    if (m_writer.joinable()) m_writer.join();
    std::lock_guard lock(m_dbMutex);
    if (m_db) { sqlite3_close(m_db); m_db = nullptr; }
}
std::wstring ThumbnailCache::Signature(const std::wstring& path) {
    HANDLE file = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return {};
    BY_HANDLE_FILE_INFORMATION info{};
    FILE_BASIC_INFO basic{};
    const bool ok = GetFileInformationByHandle(file, &info) != FALSE;
    GetFileInformationByHandleEx(file, FileBasicInfo, &basic, sizeof(basic));
    CloseHandle(file);
    if (!ok) return {};
    return std::format(L"v1|{}|{}:{}|{}:{}|{}|{}:{}|{}", std::filesystem::absolute(path).lexically_normal().wstring(),
        info.nFileSizeHigh, info.nFileSizeLow, info.ftLastWriteTime.dwHighDateTime, info.ftLastWriteTime.dwLowDateTime,
        info.dwVolumeSerialNumber, info.nFileIndexHigh, info.nFileIndexLow,basic.ChangeTime.QuadPart);
}
bool ThumbnailCache::Open() {
    if (m_db) return true;
    if (m_root.empty()) return false;
    std::error_code error;
    std::filesystem::create_directories(m_root, error);
    if (error) return false;
    if (sqlite3_open16((m_root / L"thumbnails.sqlite").c_str(), &m_db) != SQLITE_OK) {
        if (m_db) sqlite3_close(m_db);
        m_db = nullptr; return false;
    }
    sqlite3_busy_timeout(m_db, 0);
    // DELETE journal avoids an unbounded WAL; FULL vacuum releases evicted pages without a separate VACUUM copy.
    const char* schema = "PRAGMA journal_mode=DELETE; PRAGMA auto_vacuum=FULL; PRAGMA synchronous=NORMAL;"
        "CREATE TABLE IF NOT EXISTS thumbs(key TEXT NOT NULL,size INTEGER NOT NULL,w INTEGER,h INTEGER,alpha INTEGER,"
        "compressed INTEGER,checksum INTEGER,pixels BLOB,used INTEGER,PRIMARY KEY(key,size));"
        "CREATE INDEX IF NOT EXISTS thumbs_used ON thumbs(used);";
    if (sqlite3_exec(m_db, schema, nullptr, nullptr, nullptr) != SQLITE_OK) {
        sqlite3_close(m_db); m_db = nullptr; return false;
    }
    return true;
}
std::shared_ptr<ImageData> ThumbnailCache::Read(const std::wstring& path, const std::wstring& signature, uint32_t size) {
    PerformanceSpan span("cache.read.begin", "cache.read.end");
    if (m_stop.load()) { ++m_misses; return nullptr; }
    std::unique_lock lock(m_dbMutex, std::defer_lock);
    const bool acquired=lock.try_lock_for(std::chrono::milliseconds(10));
    if ((size!=100 && size!=200) || signature.empty() || !acquired || !Open()) { ++m_misses; return nullptr; }
    Statement query(m_db, "SELECT w,h,alpha,compressed,checksum,pixels FROM thumbs WHERE key=? AND size=?");
    if (!query.value) { ++m_misses; return nullptr; }
    sqlite3_bind_text16(query.value, 1, signature.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(query.value, 2, static_cast<int>(size));
    if (sqlite3_step(query.value) != SQLITE_ROW) { ++m_misses; return nullptr; }
    const int w = sqlite3_column_int(query.value, 0), h = sqlite3_column_int(query.value, 1);
    const int length = sqlite3_column_bytes(query.value, 5);
    if (w < 1 || h < 1 || w > static_cast<int>(size) || h > static_cast<int>(size) || length < 1 || length > 200000) { ++m_misses; return nullptr; }
    auto image = std::make_shared<ImageData>();
    image->filePath = path; image->width = w; image->height = h;
    image->hasAlpha = sqlite3_column_int(query.value, 2) != 0;
    image->pixels.resize(static_cast<size_t>(w) * h * 4);
    const auto data = sqlite3_column_blob(query.value, 5);
    if (!data) { ++m_misses; return nullptr; }
    bool valid = false;
    if (sqlite3_column_int(query.value, 3)) {
        Compression handles;
        SIZE_T actual = 0;
        valid = CreateDecompressor(COMPRESS_ALGORITHM_XPRESS_HUFF, nullptr, &handles.decompressor)
            && Decompress(handles.decompressor, data, length, image->pixels.data(), image->pixels.size(), &actual)
            && actual == image->pixels.size();
    } else if (static_cast<size_t>(length) == image->pixels.size()) {
        memcpy(image->pixels.data(), data, image->pixels.size()); valid = true;
    }
    if (!valid || Checksum(image->pixels) != static_cast<uint64_t>(sqlite3_column_int64(query.value, 4))) { ++m_misses; return nullptr; }
    // Finalize before queuing recency; synchronous UPDATE/journal flushes here serialize warm hits.
    sqlite3_finalize(query.value); query.value = nullptr;
    try { QueueTouch(signature,size); } catch (...) { } // Recency admission is optional even for a valid hit.
    ++m_hits; PerformanceTrace::Event("cache.hit", 0, size);
    return image;
}
void ThumbnailCache::Store(const std::wstring& signature, uint32_t size, const ImageData& image) {
    if (signature.empty() || !image.IsValid() || (size != 100 && size != 200) || image.width>size || image.height>size ||
        image.pixels.size()!=static_cast<size_t>(image.width)*image.height*4) return;
    std::lock_guard lock(m_writeMutex);
    if (m_stop || m_writes.size()>=128 || m_queuedBytes + image.pixels.size() > 8 * 1024 * 1024) return;
    m_writes.push_back({signature, size, image.width, image.height, image.hasAlpha, image.pixels});
    m_queuedBytes += image.pixels.size();
    m_cv.notify_one();
}
void ThumbnailCache::WriterLoop() {
    for (;;) {
        Write write;
        bool hasWrite=false;
        std::deque<Touch> touches;
        { std::unique_lock lock(m_writeMutex); m_cv.wait(lock, [this] { return m_stop || !m_writes.empty() || !m_touches.empty(); });
            if (m_stop) return;
            if (m_writes.empty() && !m_touches.empty())
                m_cv.wait_until(lock,m_touchReady,[this] { return m_stop || !m_writes.empty(); });
            if (m_stop) return;
            touches.swap(m_touches);
            if (!m_writes.empty()) {
                write = std::move(m_writes.front()); m_writes.pop_front(); m_queuedBytes -= write.pixels.size(); hasWrite=true;
            }
        }
        try { if (!touches.empty()) CommitTouches(touches); if (hasWrite) Commit(write); }
        catch (...) { } // Disposable cache failures cannot terminate a worker/process.
    }
}
void ThumbnailCache::QueueTouch(const std::wstring& key, uint32_t size) {
    std::lock_guard lock(m_writeMutex);
    if (m_stop || m_touches.size()>=128) return;
    FILETIME now; GetSystemTimeAsFileTime(&now);
    ULARGE_INTEGER stamp; stamp.LowPart=now.dwLowDateTime; stamp.HighPart=now.dwHighDateTime;
    if (m_touches.empty()) m_touchReady=std::chrono::steady_clock::now()+std::chrono::milliseconds(250);
    m_touches.push_back({key,size,stamp.QuadPart}); m_cv.notify_one();
}
void ThumbnailCache::CommitTouches(const std::deque<Touch>& touches) {
    PerformanceSpan span("cache.touch.begin","cache.touch.end");
    std::lock_guard lock(m_dbMutex);
    ULARGE_INTEGER free{};
    if (m_stop || !GetDiskFreeSpaceExW(m_root.c_str(),&free,nullptr,nullptr) || free.QuadPart<1024ull*1024*1024 ||
        !Open() || sqlite3_exec(m_db,"BEGIN IMMEDIATE",nullptr,nullptr,nullptr)!=SQLITE_OK) return;
    bool success=true;
    {
        Statement update(m_db,"UPDATE thumbs SET used=? WHERE key=? AND size=?");
        success=update.value!=nullptr;
        for (const auto& touch : touches) {
            if (!success || m_stop) { success=false; break; }
            sqlite3_reset(update.value); sqlite3_clear_bindings(update.value);
            sqlite3_bind_int64(update.value,1,static_cast<sqlite3_int64>(touch.used));
            sqlite3_bind_text16(update.value,2,touch.key.c_str(),-1,SQLITE_TRANSIENT);
            sqlite3_bind_int(update.value,3,static_cast<int>(touch.size));
            success=sqlite3_step(update.value)==SQLITE_DONE;
        }
    }
    if (!success || sqlite3_exec(m_db,"COMMIT",nullptr,nullptr,nullptr)!=SQLITE_OK)
        sqlite3_exec(m_db,"ROLLBACK",nullptr,nullptr,nullptr);
}
void ThumbnailCache::Commit(const Write& write) {
    PerformanceSpan span("cache.write.begin", "cache.write.end");
    std::vector<uint8_t> encoded;
    Compression handles;
    SIZE_T bound = 0, actual = 0;
    bool compressed = false;
    if (CreateCompressor(COMPRESS_ALGORITHM_XPRESS_HUFF, nullptr, &handles.compressor)) {
        Compress(handles.compressor, write.pixels.data(), write.pixels.size(), nullptr, 0, &bound);
        if (bound && bound < 1024 * 1024) {
            encoded.resize(bound);
            compressed = Compress(handles.compressor, write.pixels.data(), write.pixels.size(), encoded.data(), bound, &actual)
                && actual < write.pixels.size();
            if (compressed) encoded.resize(actual);
        }
    }
    const auto& payload = compressed ? encoded : write.pixels;
    std::lock_guard lock(m_dbMutex);
    if (m_stop.load()) return;
    if (!Open()) return;
    ULARGE_INTEGER free{};
    if (!GetDiskFreeSpaceExW(m_root.c_str(), &free, nullptr, nullptr)) return;
    const uint64_t budget = free.QuadPart<1024ull*1024*1024 ? 0 : std::min(Budget(free.QuadPart),m_maximumBytes);
    bool success = false;
    // Each eviction is one autocommit, not a transaction proportional to the entire cache.
    // Bound both maintenance work and rollback journal growth; a later request continues a large trim.
    for (int attempt=0;attempt<32;++attempt) {
        const auto pages = Scalar(m_db, "PRAGMA page_count"), pageSize = Scalar(m_db, "PRAGMA page_size");
        if (pages < 0 || pageSize < 1) break;
        if (static_cast<uint64_t>(pages * pageSize) + payload.size() * 2 + 2*1024*1024 <= budget) {
            const auto limit=std::format("PRAGMA max_page_count={}",(budget-2*1024*1024)/pageSize);
            sqlite3_exec(m_db,limit.c_str(),nullptr,nullptr,nullptr);
            success=true; break;
        }
        if (sqlite3_exec(m_db, "DELETE FROM thumbs WHERE rowid IN (SELECT rowid FROM thumbs ORDER BY used LIMIT 1)", nullptr, nullptr, nullptr) != SQLITE_OK || sqlite3_changes(m_db) == 0) break;
    }
    if (!success || sqlite3_exec(m_db,"BEGIN IMMEDIATE",nullptr,nullptr,nullptr)!=SQLITE_OK) return;
    if (success) {
        Statement insert(m_db, "INSERT OR REPLACE INTO thumbs VALUES(?,?,?,?,?,?,?,?,?)");
        success = insert.value != nullptr;
        if (success) {
            sqlite3_bind_text16(insert.value, 1, write.key.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(insert.value, 2, static_cast<int>(write.size)); sqlite3_bind_int(insert.value, 3, static_cast<int>(write.width));
            sqlite3_bind_int(insert.value, 4, static_cast<int>(write.height)); sqlite3_bind_int(insert.value, 5, write.alpha);
            sqlite3_bind_int(insert.value, 6, compressed); sqlite3_bind_int64(insert.value, 7, static_cast<sqlite3_int64>(Checksum(write.pixels)));
            sqlite3_bind_blob(insert.value, 8, payload.data(), static_cast<int>(payload.size()), SQLITE_TRANSIENT);
            FILETIME now; GetSystemTimeAsFileTime(&now); ULARGE_INTEGER stamp; stamp.LowPart=now.dwLowDateTime; stamp.HighPart=now.dwHighDateTime;
            sqlite3_bind_int64(insert.value, 9, static_cast<sqlite3_int64>(stamp.QuadPart));
            success = sqlite3_step(insert.value) == SQLITE_DONE;
        }
    }
    // A reader in another process can reject COMMIT after INSERT succeeded. Do not
    // retain that transaction/lock or poison subsequent BEGIN attempts in this connection.
    if (!success || sqlite3_exec(m_db,"COMMIT",nullptr,nullptr,nullptr)!=SQLITE_OK)
        sqlite3_exec(m_db,"ROLLBACK",nullptr,nullptr,nullptr);
}
} // namespace kv
