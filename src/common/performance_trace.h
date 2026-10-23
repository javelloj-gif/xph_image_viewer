#pragma once
#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace kv {
// Optional bounded process-local QPC recorder. Never performs file I/O in an event path.
// Enable before launch with LENTEUV_PERF_OUTPUT; exports on normal exit.
class PerformanceTrace {
public:
    // First access resolves the diagnostic environment once; no product preference is persisted.
    static PerformanceTrace& Get() { static PerformanceTrace instance; return instance; }
    // Worker/UI safe: records a phase, request generation and numeric payload; drops overflow.
    static void Event(const char* phase, uint64_t generation = 0, uint64_t value = 0) noexcept {
        auto& trace = Get();
        if (!trace.m_enabled) return;
        LARGE_INTEGER now; QueryPerformanceCounter(&now);
        try {
            std::lock_guard lock(trace.m_mutex);
            if (trace.m_events.size() < 200000)
                trace.m_events.push_back({now.QuadPart, GetCurrentThreadId(), phase, generation, value});
            else ++trace.m_dropped;
        } catch (...) { }
    }
    // Process owner only: writes diagnostic CSV after workers join; safe to call more than once.
    void Export() noexcept {
        if (!m_enabled) return;
        try {
            std::lock_guard lock(m_mutex);
            std::ofstream out(m_path, std::ios::trunc);
            out << "qpc,frequency,thread,phase,generation,value\n";
            for (const auto& e : m_events)
                out << e.qpc << ',' << m_frequency << ',' << e.thread << ',' << e.phase
                    << ',' << e.generation << ',' << e.value << '\n';
            out << "0," << m_frequency << ",0,trace.dropped,0," << m_dropped << '\n';
        } catch (...) { }
    }
    // Diagnostics-only environment value; used by isolated native benchmark sessions.
    static std::wstring Environment(const wchar_t* name) {
        wchar_t value[32768]{};
        const DWORD count = GetEnvironmentVariableW(name, value, 32768);
        return count && count < 32768 ? std::wstring(value, count) : std::wstring{};
    }
    // Read-only diagnostic state; normal product runs do not activate benchmark input/exit.
    bool Enabled() const { return m_enabled; }
private:
    // Immutable event owns no image/path data; phase strings must have static storage.
    struct EventData { LONGLONG qpc; DWORD thread; const char* phase; uint64_t generation, value; };
    // Resolves an absolute output only; malformed/relative paths leave tracing disabled.
    PerformanceTrace() {
        try {
            m_path = Environment(L"LENTEUV_PERF_OUTPUT");
            m_enabled = !m_path.empty() && m_path.is_absolute();
            LARGE_INTEGER f; QueryPerformanceFrequency(&f); m_frequency = f.QuadPart;
            if (m_enabled) m_events.reserve(8192);
        } catch (...) { m_enabled = false; }
    }
    bool m_enabled = false;
    std::filesystem::path m_path;
    LONGLONG m_frequency = 1;
    std::mutex m_mutex;
    std::vector<EventData> m_events;
    size_t m_dropped = 0;
};

// Lexical CPU/queue phase pair; event collection is a no-op when diagnostics are off.
class PerformanceSpan {
public:
    // Phase names have static lifetime; destruction records the matching end event.
    PerformanceSpan(const char* begin, const char* end, uint64_t id = 0) : m_end(end), m_id(id) { PerformanceTrace::Event(begin, id); }
    ~PerformanceSpan() { PerformanceTrace::Event(m_end, m_id); }
private:
    const char* m_end;
    uint64_t m_id;
};
} // namespace kv
