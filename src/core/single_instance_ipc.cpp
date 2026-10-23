// ============================================================================
// Class: SingleInstanceIpc
// Description: Named-pipe discovery and path forwarding for LenteUV.
// Responsibility: Own the server thread and expose delivery stages without acknowledging image application.
// ============================================================================
#include "common/pch.h"
#include "single_instance_ipc.h"
#include "common/types.h"
#include <fstream>
#include <vector>

namespace kv {

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

IpcInitResult SingleInstanceIpc::Initialize(HWND hwnd, const std::wstring& pathArg) {
    m_hwnd = hwnd;

    LogInfo(std::format(L"[IPC] Initialize called. pathArg.length={}", pathArg.length()));

    HANDLE hProbe = CreateFileW(
        m_pipeName.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    const DWORD probeError = hProbe == INVALID_HANDLE_VALUE ? GetLastError() : ERROR_SUCCESS;
    const bool anotherInstanceExists = hProbe != INVALID_HANDLE_VALUE || probeError == ERROR_PIPE_BUSY;

    if (!anotherInstanceExists && probeError != ERROR_FILE_NOT_FOUND) {
        m_sendError = probeError;
        return IpcInitResult::ForwardFailed;
    }

    if (hProbe != INVALID_HANDLE_VALUE) {
        CloseHandle(hProbe);
    }

    if (anotherInstanceExists) {
        m_detected = true;
        m_sendAttempted = true;
        LogInfo(L"[IPC] Another instance found. Sending file path.");
        std::string utf8Path = WideToUtf8(pathArg);
        DWORD writeError = ERROR_SUCCESS;
        m_writeSucceeded = SendMessage(m_pipeName, utf8Path, 2000, &writeError);
        m_sendError = writeError;
        return m_writeSucceeded.load() ? IpcInitResult::Forwarded : IpcInitResult::ForwardFailed;
    }

    m_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!m_stopEvent) {
        m_sendError = GetLastError();
        return IpcInitResult::ForwardFailed;
    }
    m_ioEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!m_ioEvent) {
        m_sendError = GetLastError();
        CloseHandle(m_stopEvent);
        m_stopEvent = nullptr;
        return IpcInitResult::ForwardFailed;
    }
    m_running = true;
    m_serverThread = std::thread(&SingleInstanceIpc::ServerThreadProc, this);

    LogInfo(L"[IPC] Server started. This is the primary instance.");
    return IpcInitResult::Primary;
}

void SingleInstanceIpc::Shutdown() {
    m_running = false;
    if (m_stopEvent) SetEvent(m_stopEvent);
    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }
    if (m_ioEvent) { CloseHandle(m_ioEvent); m_ioEvent = nullptr; }
    if (m_stopEvent) { CloseHandle(m_stopEvent); m_stopEvent = nullptr; }
    m_listening = false;
    const HWND hwnd = std::exchange(m_hwnd, nullptr);
    if (hwnd && IsWindow(hwnd)) {
        MSG message = {};
        bool sawQuit = false;
        int quitCode = 0;
        while (PeekMessageW(&message, hwnd, WM_IPC_OPEN_PATH, WM_IPC_OPEN_PATH, PM_REMOVE)) {
            // WM_QUIT bypasses PeekMessage filters; preserve it and never interpret its code as a pointer.
            if (message.message == WM_QUIT) {
                sawQuit = true;
                quitCode = static_cast<int>(message.wParam);
            } else {
                delete reinterpret_cast<std::wstring*>(message.wParam);
            }
        }
        if (sawQuit) PostQuitMessage(quitCode);
    }
}

/*static*/ bool SingleInstanceIpc::SendMessage(const std::wstring& pipeName,
                                               const std::string& utf8Message,
                                               DWORD timeoutMs, DWORD* writeError)
{
    if (writeError) *writeError = ERROR_SUCCESS;
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

        if ((err == ERROR_PIPE_BUSY || err == ERROR_FILE_NOT_FOUND) && elapsed < timeoutMs) {
            const DWORD slice = (std::min)(timeoutMs - elapsed, DWORD{10});
            // WaitNamedPipe returns immediately when no instance exists: avoid spinning in the recreation gap.
            if (err == ERROR_PIPE_BUSY) WaitNamedPipeW(pipeName.c_str(), slice);
            else Sleep(slice);
            continue;
        }

        if (writeError) *writeError = err;
        return false;
    }

    DWORD written = 0;
    BOOL ok = WriteFile(hPipe, utf8Message.c_str(),
                        static_cast<DWORD>(utf8Message.size()),
                        &written, nullptr);
    const DWORD error = ok ? ERROR_SUCCESS : GetLastError();
    FlushFileBuffers(hPipe);
    CloseHandle(hPipe);

    const bool success = ok && written == static_cast<DWORD>(utf8Message.size());
    if (writeError) *writeError = success ? ERROR_SUCCESS : (ok ? ERROR_WRITE_FAULT : error);
    return success;
}

IpcDiagnosticSnapshot SingleInstanceIpc::GetDiagnostics() const {
    return {m_detected.load(), m_sendAttempted.load(), m_writeSucceeded.load(),
        m_sendError.load(), m_received.load(), m_posted.load(), m_postFailures.load(), m_listening.load()};
}

bool SingleInstanceIpc::CompleteServerIo(HANDLE pipe, OVERLAPPED& operation, DWORD& transferred) {
    const HANDLE events[] = {m_stopEvent, operation.hEvent};
    const DWORD wait = WaitForMultipleObjects(2, events, FALSE, INFINITE);
    if (wait == WAIT_OBJECT_0 + 1 && m_running.load()) {
        return GetOverlappedResult(pipe, &operation, &transferred, FALSE) != FALSE;
    }
    // CancelIoEx may race a successful completion; either way, wait before releasing its storage.
    CancelIoEx(pipe, &operation);
    GetOverlappedResult(pipe, &operation, &transferred, TRUE);
    SetLastError(ERROR_OPERATION_ABORTED);
    return false;
}

void SingleInstanceIpc::ServerThreadProc() {
    LogInfo(L"[IPC] Server thread started.");

    while (m_running.load()) {
        HANDLE hPipe = CreateNamedPipeW(
            m_pipeName.c_str(),
            PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
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
                WaitForSingleObject(m_stopEvent, 200);
            }
            continue;
        }

        OVERLAPPED operation = {};
        operation.hEvent = m_ioEvent;
        ResetEvent(m_ioEvent);
        m_listening = true;
        const BOOL connected = ConnectNamedPipe(hPipe, &operation);
        DWORD connErr = connected ? ERROR_SUCCESS : GetLastError();
        bool clientArrived = connected || connErr == ERROR_PIPE_CONNECTED;
        if (!connected && connErr == ERROR_IO_PENDING) {
            DWORD ignored = 0;
            clientArrived = CompleteServerIo(hPipe, operation, ignored);
            connErr = clientArrived ? ERROR_SUCCESS : GetLastError();
        }
        m_listening = false;
        if (!clientArrived || !m_running.load()) {
            if (m_running.load()) LogError(std::format(L"[IPC] ConnectNamedPipe failed: {}", connErr));
            CloseHandle(hPipe);
            continue;
        }

        char buffer[4096] = {};
        DWORD bytesRead = 0;

        operation = {};
        operation.hEvent = m_ioEvent;
        ResetEvent(m_ioEvent);
        BOOL readOk = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, &operation);
        if (!readOk && GetLastError() == ERROR_IO_PENDING) {
            readOk = CompleteServerIo(hPipe, operation, bytesRead);
        }

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
        ++m_received;
        LogInfo(std::format(L"[IPC] Received open request for: {}", path));

        auto pPath = std::make_unique<std::wstring>(std::move(path));
        if (PostMessageW(m_hwnd, WM_IPC_OPEN_PATH, reinterpret_cast<WPARAM>(pPath.get()), 0)) {
            (void)pPath.release(); // Window now owns the posted payload.
            ++m_posted;
        } else {
            const DWORD error = GetLastError();
            ++m_postFailures;
            LogError(std::format(L"[IPC] PostMessageW failed: {}", error));
        }

    }

    LogInfo(L"[IPC] Server thread exited cleanly.");
}

} // namespace kv
