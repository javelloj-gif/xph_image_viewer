#pragma once
#include <Windows.h>
#include <cstdint>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace kv {
// Optional bounded process-local QPC recorder. Never performs file I/O in an event path.
// Enable before launch with IMAGE_VIEWER_PERF_OUTPUT; exports on normal exit.
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
    // Diagnostics-only: neutral names take precedence; absent values accept the historical diagnostic alias.
    static std::wstring Environment(const wchar_t* name) {
        wchar_t value[32768]{};
        SetLastError(ERROR_SUCCESS); // Empty-but-present values must not inherit an earlier missing-variable error.
        DWORD count = GetEnvironmentVariableW(name, value, 32768);
        const std::wstring requested(name);
        if (!count && GetLastError() == ERROR_ENVVAR_NOT_FOUND && requested.starts_with(L"IMAGE_VIEWER_")) {
            const auto legacy = L"LENTEUV_" + requested.substr(13);
            count = GetEnvironmentVariableW(legacy.c_str(), value, 32768);
        }
        return count && count < 32768 ? std::wstring(value, count) : std::wstring{};
    }
    // Read-only diagnostic state; normal product runs do not activate benchmark input/exit.
    bool Enabled() const { return m_enabled; }
    // UI benchmark only: exits after the requested content is submitted plus a settling interval, or safety timeout.
    bool ShouldExit(bool originalSubmitted, bool stripSubmitted, bool waterfallSubmitted) {
        if (!m_enabled) return false;
        const auto now=GetTickCount64();
        if (!m_firstFrame) m_firstFrame=now;
        m_originalSubmitted |= originalSubmitted;
        const bool ready=m_scenario==L"strip" ? stripSubmitted : m_scenario==L"waterfall" ? waterfallSubmitted : m_originalSubmitted;
        if (ready && !m_readyFrame) m_readyFrame=now;
        return (m_readyExitMs && m_readyFrame && now-m_readyFrame>=m_readyExitMs) ||
            (m_exitMs && now-m_firstFrame>=m_exitMs);
    }
private:
    // Immutable event owns no image/path data; phase strings must have static storage.
    struct EventData { LONGLONG qpc; DWORD thread; const char* phase; uint64_t generation, value; };
    // Resolves an absolute output only; malformed/relative paths leave tracing disabled.
    PerformanceTrace() {
        try {
            m_path = Environment(L"IMAGE_VIEWER_PERF_OUTPUT");
            m_enabled = !m_path.empty() && m_path.is_absolute();
            if (m_enabled) {
                m_scenario=Environment(L"IMAGE_VIEWER_PERF_SCENARIO");
                m_exitMs=wcstoull(Environment(L"IMAGE_VIEWER_PERF_EXIT_MS").c_str(),nullptr,10);
                m_readyExitMs=wcstoull(Environment(L"IMAGE_VIEWER_PERF_READY_EXIT_MS").c_str(),nullptr,10);
            }
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
    std::wstring m_scenario;
    ULONGLONG m_firstFrame=0,m_readyFrame=0,m_exitMs=0,m_readyExitMs=0;
    bool m_originalSubmitted=false;
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
