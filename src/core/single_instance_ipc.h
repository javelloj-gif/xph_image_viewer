// ============================================================================
// Class: SingleInstanceIpc
// Description: SingleInstanceIpc implements named pipe communication to ensure a single instance of [IMAGE_VIEWER_APP_NAME] runs and handles file opening requests.
// Responsibility: Handle named-pipe client/server IPC to prevent multiple instances and handle external path requests using a poison-pill shutdown handler.
// ============================================================================
#pragma once

#include <string>
#include <windows.h>
#include <thread>
#include <atomic>

namespace kv {

class SingleInstanceIpc {
public:
    // Posted to the main window HWND when a new file path is received from
    // another instance. wParam = new std::wstring*, caller must delete it.
    static constexpr UINT WM_IPC_OPEN_PATH = WM_USER + 4501;

    SingleInstanceIpc() = default;
    ~SingleInstanceIpc();

    // Non-copyable, non-movable
    SingleInstanceIpc(const SingleInstanceIpc&) = delete;
    SingleInstanceIpc& operator=(const SingleInstanceIpc&) = delete;

    // Checks if another instance is already running.
    //   - If another instance is found: sends pathArg to it, returns false.
    //   - If this is the first instance: starts the server thread, returns true.
    bool Initialize(HWND hwnd, const std::wstring& pathArg);

    // Cleanly shuts down the server thread using the poison-pill technique.
    // Blocks until the server thread has fully exited (guaranteed no leaks).
    void Shutdown();

private:
    void ServerThreadProc();

    // Sends a single message over the pipe. Used both for real file paths
    // (from a second instance) and for the internal shutdown sentinel.
    static bool SendMessage(const std::wstring& pipeName, const std::string& utf8Message,
                            DWORD timeoutMs = 2000);

    static std::string  WideToUtf8(const std::wstring& w);
    static std::wstring Utf8ToWide(const std::string& s);

    HWND             m_hwnd        = nullptr;
    std::thread      m_serverThread;
    std::atomic<bool> m_running   { false };

    // Sentinel message sent as the poison pill to unblock the server thread.
    // This string must not be a valid filesystem path.
    static constexpr const char* kShutdownSentinel = "\x01SHUTDOWN\x01";
};

} // namespace kv
