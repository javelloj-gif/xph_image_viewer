// ============================================================================
// Class: SingleInstanceIpc
// Description: SingleInstanceIpc implements named pipe communication to ensure a single instance of [IMAGE_VIEWER_APP_NAME] runs and handles file opening requests.
// Responsibility: Handle named-pipe client/server IPC to prevent multiple instances and handle external path requests using a poison-pill shutdown handler.
// ============================================================================
#include "common/pch.h"
#include "single_instance_ipc.h"
#include "common/types.h"
#include <fstream>
#include <vector>

namespace kv {

static const wchar_t* kPipeName = L"\\\\.\\pipe\\lenteuv";

/*static*/ std::string SingleInstanceIpc::WideToUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(),
                                nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(),
                        s.data(), n, nullptr, nullptr);
    return s;
}

/*static*/ std::wstring SingleInstanceIpc::Utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
    return w;
}

SingleInstanceIpc::~SingleInstanceIpc() {
    Shutdown();
}

bool SingleInstanceIpc::Initialize(HWND hwnd, const std::wstring& pathArg) {
    m_hwnd = hwnd;

    LogInfo(std::format(L"[IPC] Initialize called. pathArg.length={}", pathArg.length()));

    HANDLE hProbe = CreateFileW(
        kPipeName,
        GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    bool anotherInstanceExists = false;

    if (hProbe != INVALID_HANDLE_VALUE) {
        CloseHandle(hProbe);
        anotherInstanceExists = true;
    } else {
        DWORD err = GetLastError();
        if (err == ERROR_PIPE_BUSY) {
            if (WaitNamedPipeW(kPipeName, 1000)) {
                hProbe = CreateFileW(kPipeName, GENERIC_WRITE, 0, nullptr,
                                     OPEN_EXISTING, 0, nullptr);
                if (hProbe != INVALID_HANDLE_VALUE) {
                    CloseHandle(hProbe);
                    anotherInstanceExists = true;
                }
            } else {
                hProbe = CreateFileW(kPipeName, GENERIC_WRITE, 0, nullptr,
                                     OPEN_EXISTING, 0, nullptr);
                if (hProbe != INVALID_HANDLE_VALUE) {
                    CloseHandle(hProbe);
                    anotherInstanceExists = true;
                }
            }
        }
    }

    if (anotherInstanceExists) {
        LogInfo(L"[IPC] Another instance found. Sending file path.");
        std::string utf8Path = WideToUtf8(pathArg);
        SendMessage(std::wstring(kPipeName), utf8Path, 2000);
        return false;
    }

    m_running = true;
    m_serverThread = std::thread(&SingleInstanceIpc::ServerThreadProc, this);

    LogInfo(L"[IPC] Server started. This is the primary instance.");
    return true;
}

void SingleInstanceIpc::Shutdown() {
    if (!m_running.exchange(false)) {
        return;
    }

    LogInfo(L"[IPC] Shutdown: sending poison pill.");

    constexpr int kMaxRetries = 10;
    constexpr int kRetryDelayMs = 50;

    bool sent = false;
    for (int i = 0; i < kMaxRetries; ++i) {
        if (SendMessage(std::wstring(kPipeName), kShutdownSentinel, 200)) {
            sent = true;
            break;
        }
        if (m_serverThread.joinable() &&
            WaitForSingleObject(m_serverThread.native_handle(), kRetryDelayMs) == WAIT_OBJECT_0) {
            break;
        }
    }

    if (!sent) {
        LogError(L"[IPC] Poison pill could not be delivered after retries. Forcing thread stop.");
    }

    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }

    LogInfo(L"[IPC] Server thread joined cleanly.");
}

/*static*/ bool SingleInstanceIpc::SendMessage(const std::wstring& pipeName,
                                               const std::string& utf8Message,
                                               DWORD timeoutMs)
{
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    DWORD startMs = GetTickCount();

    while (true) {
        hPipe = CreateFileW(
            pipeName.c_str(),
            GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if (hPipe != INVALID_HANDLE_VALUE) break;

        DWORD err = GetLastError();
        DWORD elapsed = GetTickCount() - startMs;

        if (err == ERROR_PIPE_BUSY && elapsed < timeoutMs) {
            DWORD remaining = timeoutMs - elapsed;
            WaitNamedPipeW(pipeName.c_str(), remaining);
            continue;
        }

        return false;
    }

    DWORD written = 0;
    BOOL ok = WriteFile(hPipe, utf8Message.c_str(),
                        static_cast<DWORD>(utf8Message.size()),
                        &written, nullptr);
    FlushFileBuffers(hPipe);
    CloseHandle(hPipe);

    return ok && written == static_cast<DWORD>(utf8Message.size());
}

void SingleInstanceIpc::ServerThreadProc() {
    LogInfo(L"[IPC] Server thread started.");

    while (m_running.load()) {
        HANDLE hPipe = CreateNamedPipeW(
            kPipeName,
            PIPE_ACCESS_INBOUND,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            1,
            0,
            4096,
            0,
            nullptr
        );

        if (hPipe == INVALID_HANDLE_VALUE) {
            DWORD err = GetLastError();
            LogError(std::format(L"[IPC] CreateNamedPipeW failed: {}", err));
            if (m_running.load()) {
                Sleep(200);
            }
            continue;
        }

        BOOL connected = ConnectNamedPipe(hPipe, nullptr);
        DWORD connErr = GetLastError();

        bool clientArrived = connected || (connErr == ERROR_PIPE_CONNECTED);

        if (!clientArrived) {
            LogError(std::format(L"[IPC] ConnectNamedPipe failed unexpectedly: {}", connErr));
            CloseHandle(hPipe);
            continue;
        }

        char buffer[4096] = {};
        DWORD bytesRead = 0;

        BOOL readOk = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr);

        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);

        if (!readOk || bytesRead == 0) {
            LogInfo(L"[IPC] Client connected but sent no data.");
            continue;
        }

        buffer[bytesRead] = '\0';
        std::string message(buffer, bytesRead);

        if (message == kShutdownSentinel) {
            LogInfo(L"[IPC] Received shutdown sentinel. Server thread exiting.");
            break;
        }

        if (!m_running.load()) {
            break;
        }

        std::wstring path = Utf8ToWide(message);
        LogInfo(std::format(L"[IPC] Received open request for: {}", path));

        std::wstring* pPath = new std::wstring(std::move(path));
        PostMessageW(m_hwnd, WM_IPC_OPEN_PATH, reinterpret_cast<WPARAM>(pPath), 0);

    }

    LogInfo(L"[IPC] Server thread exited cleanly.");
}

} // namespace kv
