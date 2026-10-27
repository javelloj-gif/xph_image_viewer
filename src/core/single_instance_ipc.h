// ============================================================================
// Class: SingleInstanceIpc
// Description: Named-pipe discovery and path forwarding for Fotassa.
// Responsibility: Own the server thread and expose delivery stages without acknowledging image application.
// ============================================================================
#pragma once

#include <string>
#include <cstdint>
#include <windows.h>
#include <thread>
#include <atomic>

namespace kv {

// Startup role or transport failure; Forwarded does not acknowledge UI application.
enum class IpcInitResult { Primary, Forwarded, ForwardFailed };

// Diagnostic stages only: a successful write/post is not an acknowledgement of image application.
struct IpcDiagnosticSnapshot {
    bool detected = false;
    bool sendAttempted = false;
    bool writeSucceeded = false;
    DWORD sendError = ERROR_SUCCESS;
    uint32_t received = 0;
    uint32_t posted = 0;
    uint32_t postFailures = 0;
    bool listening = false;
};

// Owns the pipe server thread; UI caller owns the HWND and must keep it alive through shutdown.
class SingleInstanceIpc {
    friend class ShutdownTests; // Exercises owned private endpoints and shutdown in bounded child processes.
    friend class IpcDiagnosticTests; // Selects an isolated endpoint before starting any threads.
public:
    // Posted to the main window HWND when a new file path is received from
    // another instance. wParam = new std::wstring*, caller must delete it.
    static constexpr UINT WM_IPC_OPEN_PATH = WM_USER + 4501;

    SingleInstanceIpc() = default;
    ~SingleInstanceIpc();

    // Non-copyable, non-movable
    SingleInstanceIpc(const SingleInstanceIpc&) = delete;
    SingleInstanceIpc& operator=(const SingleInstanceIpc&) = delete;

    // UI thread: probes and forwards with a total connection/write budget; failure is in GetDiagnostics.
    // Only an absent receiver starts the owned server; this does not atomically elect an owner.
    IpcInitResult Initialize(HWND hwnd, const std::wstring& pathArg);

    // Owner/HWND thread, non-reentrant: signals stop, joins canceled server I/O and frees queued path payloads.
    // Idempotent; HWND remains alive until return. Does not send a sentinel or wait for a client to write.
    void Shutdown();

    // Thread-safe, non-transactional observation of stages over this object's lifetime; no path data.
    IpcDiagnosticSnapshot GetDiagnostics() const;

private:
    // Owns each pipe handle and posts heap path payloads to the caller's HWND; runs on server thread.
    void ServerThreadProc();

    // Server-only: waits for one pending operation or stop, then observes completion before returning.
    // Borrows pipe/OVERLAPPED/buffer; cancellation never releases them while the kernel still uses them.
    bool CompleteServerIo(HANDLE pipe, OVERLAPPED& operation, DWORD& transferred);

    // Sends one UTF-8 path within timeoutMs for connection plus overlapped write, never retries a write.
    // Timeout cancels and observes completion before freeing storage; no blocking flush or application ACK.
    // writeError reports transport status (ERROR_TIMEOUT on exhausted budget), not UI receipt/application.
    static bool SendMessage(const std::wstring& pipeName, const std::string& utf8Message,
                            DWORD timeoutMs = 2000, DWORD* writeError = nullptr);

    // Convert wire paths between Windows UTF-16 and UTF-8; empty input remains empty.
    static std::string  WideToUtf8(const std::wstring& w);
    static std::wstring Utf8ToWide(const std::string& s);

    HWND             m_hwnd        = nullptr;
    std::thread      m_serverThread;
    HANDLE           m_stopEvent = nullptr; // UI-owned; closed only after the server joins.
    HANDLE           m_ioEvent = nullptr;   // One server operation at a time; same lifetime as stop event.
    std::atomic<bool> m_running   { false };
    std::wstring m_pipeName = L"\\\\.\\pipe\\lenteuv"; // Immutable while server is running.
    std::atomic<bool> m_detected{false}, m_sendAttempted{false}, m_writeSucceeded{false};
    std::atomic<DWORD> m_sendError{ERROR_SUCCESS};
    std::atomic<uint32_t> m_received{0}, m_posted{0}, m_postFailures{0};
    std::atomic<bool> m_listening{false};

    // Legacy wire sentinel remains accepted; local shutdown uses its private event instead.
    static constexpr const char* kShutdownSentinel = "\x01SHUTDOWN\x01";
};

} // namespace kv
