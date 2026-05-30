// ============================================================================
// ipc.cpp — Single Instance and Named Pipe IPC Implementation
// Krumassan Image Viewer — Native C++20
// ============================================================================
#include "pch.h"
#include "ipc.h"
#include "types.h"
#include <vector>

namespace kv {

static const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\krumassanimageviewer";

static std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

static std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

Ipc::~Ipc() {
    Shutdown();
}

bool Ipc::Initialize(HWND hwnd, const std::wstring& pathArg) {
    m_hwnd = hwnd;

    // Check if another instance is already running by attempting to connect
    HANDLE hPipe = CreateFileW(
        PIPE_NAME,
        GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (hPipe != INVALID_HANDLE_VALUE) {
        // Another instance is running!
        LogInfo(L"Found existing KrumassanViewer instance, sending path argument.");

        std::string utf8Path = WideToUtf8(pathArg);
        DWORD written = 0;
        WriteFile(hPipe, utf8Path.c_str(), (DWORD)utf8Path.size(), &written, nullptr);
        CloseHandle(hPipe);

        // Signal that the caller should exit
        return false;
    }

    // Start named pipe server
    m_running = true;
    m_pipeEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    m_serverThread = std::thread(&Ipc::ServerThreadProc, this);

    return true;
}

void Ipc::Shutdown() {
    if (m_running) {
        m_running = false;
        
        // Cancel any pending pipe operations
        if (m_pipeEvent != INVALID_HANDLE_VALUE) {
            SetEvent(m_pipeEvent);
        }

        // Connect a dummy client to unblock ConnectNamedPipe if it is blocking
        HANDLE hPipe = CreateFileW(PIPE_NAME, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(hPipe);
        }

        if (m_serverThread.joinable()) {
            m_serverThread.join();
        }

        if (m_pipeEvent != INVALID_HANDLE_VALUE) {
            CloseHandle(m_pipeEvent);
            m_pipeEvent = INVALID_HANDLE_VALUE;
        }
    }
}

void Ipc::ServerThreadProc() {
    LogInfo(L"IPC Server thread started.");

    while (m_running) {
        HANDLE hPipe = CreateNamedPipeW(
            PIPE_NAME,
            PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            1,
            4096,
            4096,
            0,
            nullptr
        );

        if (hPipe == INVALID_HANDLE_VALUE) {
            LogError(L"Failed to create Named Pipe");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        OVERLAPPED overlapped = {};
        overlapped.hEvent = m_pipeEvent;
        ResetEvent(m_pipeEvent);

        BOOL connected = ConnectNamedPipe(hPipe, &overlapped);
        if (!connected) {
            DWORD err = GetLastError();
            if (err == ERROR_IO_PENDING) {
                // Wait for client connection or shutdown signal
                WaitForSingleObject(m_pipeEvent, INFINITE);
            } else if (err != ERROR_PIPE_CONNECTED) {
                CloseHandle(hPipe);
                continue;
            }
        }

        if (!m_running) {
            CloseHandle(hPipe);
            break;
        }

        // Client connected, read path
        std::vector<char> buffer(4096);
        DWORD bytesRead = 0;
        
        // Since we are connected, synchronous ReadFile is fine
        BOOL success = ReadFile(hPipe, buffer.data(), (DWORD)buffer.size() - 1, &bytesRead, nullptr);
        if (success && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::string utf8Path(buffer.data());
            std::wstring* pPath = new std::wstring(Utf8ToWide(utf8Path));

            // Post to main window thread
            PostMessageW(m_hwnd, WM_IPC_OPEN_PATH, reinterpret_cast<WPARAM>(pPath), 0);
        }

        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }

    LogInfo(L"IPC Server thread stopped.");
}

} // namespace kv
