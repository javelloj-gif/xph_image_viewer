#include "common/pch.h"
#include "loader/image_loader.h"
#include "core/single_instance_ipc.h"
#include <iostream>
#include <stdexcept>

namespace kv {

// Owns a test-only Win32 handle; destroyed after all users have joined.
class ShutdownTestHandle {
public:
    // Adopts events, processes and pipe clients; null/invalid handles need no release.
    explicit ShutdownTestHandle(HANDLE handle) : value(handle) {}
    ~ShutdownTestHandle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
    // Unique owner avoids double close during failure unwinding.
    ShutdownTestHandle(const ShutdownTestHandle&) = delete;
    ShutdownTestHandle& operator=(const ShutdownTestHandle&) = delete;
    HANDLE value;
};

// Runs deterministic queue boundaries and private IPC endpoints; parent bounds child process lifetime.
class ShutdownTests {
public:
    // Release-active checks; failing children report their case without touching another process.
    static void Check(bool value, const char* label) {
        if (!value) throw std::runtime_error(label);
    }

    // Holds one active task while closing 1000 queued tasks, results and late submissions.
    static void Loader() {
        ShutdownTestHandle entered(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        ShutdownTestHandle release(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        Check(entered.value && release.value, "loader test events");
        ImageLoader loader;
        loader.EnqueueTask([] {});
        Check(loader.m_taskQueue.empty(), "uninitialized loader rejects work");
        Check(loader.Initialize(1), "loader initialize");
        Check(!loader.Initialize(0), "active loader cannot be initialized twice");
        std::atomic<int> active{0}, queued{0}, callbacks{0};
        auto owner = std::make_shared<int>(7);
        std::weak_ptr<int> weak = owner;
        loader.EnqueueTask([&, owner] {
            SetEvent(entered.value);
            WaitForSingleObject(release.value, 3000);
            ++active;
            std::lock_guard lock(loader.m_pendingMutex);
            loader.m_pendingTextures.push_back({nullptr, [&, owner](std::shared_ptr<ImageData>) { ++callbacks; }, {}});
            loader.m_pendingAnimatedTextures.push_back({nullptr, [&, owner](std::shared_ptr<AnimatedImageData>) { ++callbacks; }});
        });
        Check(WaitForSingleObject(entered.value, 3000) == WAIT_OBJECT_0, "active task entered");
        for (int i = 0; i < 1000; ++i) loader.EnqueueTask([&, owner] { ++queued; });
        loader.LoadAsync(L"missing-shutdown.png", [&, owner](std::shared_ptr<ImageData>) { ++callbacks; });
        loader.LoadAnimatedAsync(L"missing-shutdown.gif", [&, owner](std::shared_ptr<AnimatedImageData>) { ++callbacks; });
        loader.m_pendingTextures.push_back({nullptr, [&, owner](std::shared_ptr<ImageData>) { ++callbacks; }, {}});
        loader.m_pendingAnimatedTextures.push_back({nullptr, [&, owner](std::shared_ptr<AnimatedImageData>) { ++callbacks; }});
        owner.reset();
        std::jthread releaser([&] {
            const auto end = GetTickCount64() + 2000;
            while (!loader.m_shutdown.load() && GetTickCount64() < end) Sleep(1);
            // Races with join after the stop flag, exercising the actual queue admission lock.
            loader.EnqueueTask([&] { ++queued; });
            SetEvent(release.value);
        });
        const auto start = GetTickCount64();
        loader.Shutdown();
        releaser.join();
        std::cout << "LOADER close_ms=" << GetTickCount64() - start << " active=" << active
            << " queued_executed=" << queued << " callbacks=" << callbacks << std::endl;
        Check(active == 1, "active task joined");
        Check(queued == 0, "shutdown must discard unstarted tasks and reject late work");
        Check(callbacks == 0 && weak.expired(), "shutdown releases captured owners without callbacks");
        Check(loader.m_taskQueue.empty() && loader.m_pendingTextures.empty() && loader.m_pendingAnimatedTextures.empty(), "all queues empty");
        loader.LoadAsync(L"late.png", [&](std::shared_ptr<ImageData>) { ++callbacks; });
        Check(loader.m_taskQueue.empty(), "post-shutdown async rejected");
        Renderer unused;
        loader.ProcessPendingTextures(unused);
        Check(callbacks == 0, "no delivery after shutdown");
        loader.Shutdown();
        Check(loader.Initialize(0), "restart after completed shutdown");
        loader.EnqueueTask([&] { ++queued; });
        Check(loader.m_taskQueue.size() == 1, "restart accepts work");
        loader.Shutdown();
        Check(loader.m_taskQueue.empty(), "zero-worker shutdown also clears queue");
    }

    // Waits without consuming a connection; process watchdog bounds any regression in shutdown.
    static void Ready(SingleInstanceIpc& server) {
        const auto end = GetTickCount64() + 2000;
        while (!server.GetDiagnostics().listening && GetTickCount64() < end) Sleep(1);
        Check(server.GetDiagnostics().listening, "server listening");
    }

    // Tests idle/immediate/silent/disconnected/posted/repeated/legacy-sentinel and WM_QUIT-preserving shutdown.
    static void Ipc(int mode) {
        const auto pipe = L"\\\\.\\pipe\\lenteuv-shutdown-" + std::to_wstring(GetCurrentProcessId());
        HWND hwnd = CreateWindowExW(0, L"STATIC", L"shutdown-tests", 0, 0, 0, 0, 0,
            HWND_MESSAGE, nullptr, GetModuleHandleW(nullptr), nullptr);
        Check(hwnd != nullptr, "shutdown HWND");
        SingleInstanceIpc server;
        server.m_pipeName = pipe;
        for (int round = 0; round < (mode == 5 ? 12 : 1); ++round) {
            Check(server.Initialize(hwnd, L"") == IpcInitResult::Primary, "private primary");
            HANDLE client = INVALID_HANDLE_VALUE;
            if (mode != 1) Ready(server);
            if (mode == 2 || mode == 3 || mode == 5) {
                client = CreateFileW(pipe.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
                Check(client != INVALID_HANDLE_VALUE, "silent client connected");
                const auto end = GetTickCount64() + 2000;
                while (server.GetDiagnostics().listening && GetTickCount64() < end) Sleep(1);
                Check(!server.GetDiagnostics().listening, "server reached read stage");
                if (mode == 3) { CloseHandle(client); client = INVALID_HANDLE_VALUE; }
            }
            ShutdownTestHandle clientOwner(client);
            if (mode == 4 || mode == 6 || mode == 7) {
                Check(SingleInstanceIpc::SendMessage(pipe, mode == 6 ? "\x01SHUTDOWN\x01" : "missing-pending.png"), "test write");
                if (mode != 6) {
                    const auto end = GetTickCount64() + 2000;
                    while (server.GetDiagnostics().posted == 0 && GetTickCount64() < end) Sleep(1);
                    Check(server.GetDiagnostics().posted == 1, "payload queued without dispatch");
                }
            }
            if (mode == 7) PostQuitMessage(42);
            const auto start = GetTickCount64();
            server.Shutdown();
            server.Shutdown();
            Check(!server.m_serverThread.joinable(), "server joined");
            MSG message = {};
            if (mode == 7) {
                Check(PeekMessageW(&message, nullptr, WM_QUIT, WM_QUIT, PM_REMOVE) &&
                    message.message == WM_QUIT && message.wParam == 42, "shutdown preserves quit code");
            }
            const bool pending = PeekMessageW(&message, hwnd, SingleInstanceIpc::WM_IPC_OPEN_PATH,
                SingleInstanceIpc::WM_IPC_OPEN_PATH, PM_REMOVE) != FALSE;
            if (pending) delete reinterpret_cast<std::wstring*>(message.wParam);
            Check(!pending, "shutdown releases queued payload without dispatch");
            std::cout << "IPC mode=" << mode << " round=" << round << " close_ms=" << GetTickCount64() - start << std::endl;
        }
        DestroyWindow(hwnd);
    }

    // Starts only this harness, kills only its own timed-out child, and checks actual process exit.
    static int Parent() {
        wchar_t exe[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exe, MAX_PATH);
        int failed = 0;
        for (int scenario = 0; scenario < 9; ++scenario) {
            std::wstring command = L"\"" + std::wstring(exe) + L"\" --shutdown-child " + std::to_wstring(scenario);
            STARTUPINFOW startup = {};
            startup.cb = sizeof(startup);
            PROCESS_INFORMATION process = {};
            Check(CreateProcessW(exe, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                nullptr, nullptr, &startup, &process) != FALSE, "shutdown child launch");
            ShutdownTestHandle processOwner(process.hProcess), threadOwner(process.hThread);
            const auto start = GetTickCount64();
            const DWORD wait = WaitForSingleObject(process.hProcess, 5000);
            if (wait != WAIT_OBJECT_0) {
                TerminateProcess(process.hProcess, 99);
                WaitForSingleObject(process.hProcess, 1000);
            }
            DWORD code = 99;
            GetExitCodeProcess(process.hProcess, &code);
            std::cout << "SHUTDOWN scenario=" << scenario << " exit=" << code << " timeout="
                << (wait != WAIT_OBJECT_0) << " process_ms=" << GetTickCount64() - start << std::endl;
            if (wait != WAIT_OBJECT_0 || code != 0) ++failed;
        }
        return failed == 0 ? 0 : 1;
    }
};
} // namespace kv

// Dispatches one isolated child scenario or the watchdog parent; no user files or public endpoint.
int RunShutdownTests(int argc, char* argv[]) {
    try {
        kv::ScopedComInit com;
        if (argc == 2) return kv::ShutdownTests::Parent();
        kv::ShutdownTests::Check(argc == 3, "shutdown child arguments");
        const int scenario = std::stoi(argv[2]);
        if (scenario == 0) kv::ShutdownTests::Loader();
        else kv::ShutdownTests::Ipc(scenario - 1);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Shutdown failure: " << e.what() << std::endl;
        return 1;
    }
}
