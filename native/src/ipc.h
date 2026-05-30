// ============================================================================
// ipc.h — Single Instance and Named Pipe IPC
// Krumassan Image Viewer — Native C++20
// ============================================================================
#pragma once

#include <string>
#include <windows.h>
#include <thread>
#include <atomic>

namespace kv {

class Ipc {
public:
    static constexpr UINT WM_IPC_OPEN_PATH = WM_USER + 4501;

    Ipc() = default;
    ~Ipc();

    // Checks if another instance is running. If so, sends pathArg and returns false.
    // Otherwise, starts the server thread and returns true.
    bool Initialize(HWND hwnd, const std::wstring& pathArg);
    void Shutdown();

private:
    void ServerThreadProc();

    HWND m_hwnd = nullptr;
    std::thread m_serverThread;
    std::atomic<bool> m_running{false};
    HANDLE m_pipeEvent = INVALID_HANDLE_VALUE;
};

} // namespace kv
