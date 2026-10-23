#include "common/pch.h"
#include "core/app.h"
#include "shell/shell_integrator.h"
#include <iostream>
#include <stdexcept>
#include <fstream>

namespace kv {

// Temporarily replaces one import in this test EXE only; restores it before fixture teardown.
class ScopedStartupImport {
public:
    // Locates a named import in the current executable; no product binary or DLL is modified.
    ScopedStartupImport(const char* name, void* replacement) {
        auto* base = reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
        auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        auto* descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base
            + nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
        for (; descriptor->Name; ++descriptor) {
            if (!descriptor->OriginalFirstThunk) continue;
            auto* names = reinterpret_cast<IMAGE_THUNK_DATA*>(base + descriptor->OriginalFirstThunk);
            auto* addresses = reinterpret_cast<IMAGE_THUNK_DATA*>(base + descriptor->FirstThunk);
            for (; names->u1.AddressOfData; ++names, ++addresses) {
                if (IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal)) continue;
                auto* entry = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + names->u1.AddressOfData);
                if (std::strcmp(reinterpret_cast<const char*>(entry->Name), name) != 0) continue;
                slot = reinterpret_cast<void**>(&addresses->u1.Function);
                original = *slot;
                Replace(replacement);
                return;
            }
        }
        throw std::runtime_error("startup test import not found");
    }
    // Restores the executable import while all called code is still loaded.
    ~ScopedStartupImport() { Replace(original); }
    // One scope owns each restoration; copying would restore the same slot twice.
    ScopedStartupImport(const ScopedStartupImport&) = delete;
    ScopedStartupImport& operator=(const ScopedStartupImport&) = delete;
    void* original = nullptr;
private:
    // Changes only the selected IAT slot and restores its page protection immediately.
    void Replace(void* value) {
        DWORD protection = 0;
        if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &protection)) std::terminate();
        InterlockedExchangePointer(reinterpret_cast<void* volatile*>(slot), value);
        DWORD ignored = 0;
        if (!VirtualProtect(slot, sizeof(void*), protection, &ignored)) std::terminate();
    }
    void** slot = nullptr;
};

// Isolates real IPC processes and a real Window/App receiver backed by offscreen software D2D.
class IpcDiagnosticTests {
public:
    // Builds only receiver prerequisites; never runs App::Initialize or touches the global endpoint.
    IpcDiagnosticTests() {
        wchar_t executable[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, executable, MAX_PATH);
        exe = executable;
        root = exe.parent_path();
        Check(std::filesystem::is_directory(root / "user_data"), "isolated user_data required");
        pipe = L"\\\\.\\pipe\\lenteuv-diagnostic-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64());
        app.m_ipc.m_pipeName = pipe;
        WNDCLASSW windowClass = {};
        windowClass.lpfnWndProc = Window::StaticWndProc;
        windowClass.hInstance = GetModuleHandleW(nullptr);
        windowClass.lpszClassName = L"LenteUV-IpcDiagnostic";
        Check(RegisterClassW(&windowClass) != 0, "message-only class");
        Check(CreateWindowExW(0, windowClass.lpszClassName, L"IPC diagnostic", 0,
            0, 0, 0, 0, HWND_MESSAGE, nullptr, windowClass.hInstance, &app.m_window) != nullptr, "message-only receiver");
        app.m_window.m_fullscreen = true; // Avoid size adjustment; message-only windows cannot be shown.
        app.m_window.events.onIpcOpenPath = [this](const std::wstring& path) {
            ++handled;
            app.OnIpcOpenPath(path);
            appliedPaths.push_back(CurrentPath());
        };
        app.m_folderData.Initialize(&app);
        app.m_thumbnailStrip.Initialize(&app);
        app.m_waterfallPage.Initialize(&app);
        app.m_viewerPage.Initialize(&app);
        Check(app.m_imageLoader.Initialize(0), "WIC receiver");
        ComPtr<ID3D11Device> d3d;
        Check(SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, &d3d, nullptr, nullptr)), "WARP receiver");
        ComPtr<IDXGIDevice> dxgi;
        Check(SUCCEEDED(d3d.As(&dxgi)), "DXGI receiver");
        ComPtr<ID2D1Factory1> factory;
        Check(SUCCEEDED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(&factory))), "D2D factory");
        ComPtr<ID2D1Device> device;
        Check(SUCCEEDED(factory->CreateDevice(dxgi.Get(), &device)), "D2D receiver device");
        ComPtr<ID2D1DeviceContext> context;
        Check(SUCCEEDED(device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &context)), "D2D receiver context");
        Check(SUCCEEDED(context.As(&app.m_renderer.m_d2dContext)), "D2D context2");
        app.m_renderer.m_width = 640;
        app.m_renderer.m_height = 480;
        Check(app.m_ipc.Initialize(app.m_window.GetHwnd(), L"") == IpcInitResult::Primary, "primary receiver");
        Ready();
    }

    // Reaps only owned children, then joins this fixture's server while its receiver HWND remains alive.
    ~IpcDiagnosticTests() {
        for (const auto& process : children) {
            if (WaitForSingleObject(process.hProcess, 0) != WAIT_OBJECT_0) {
                TerminateProcess(process.hProcess, 99);
                WaitForSingleObject(process.hProcess, 1000);
            }
            CloseHandle(process.hThread);
            CloseHandle(process.hProcess);
        }
        app.m_ipc.Shutdown();
    }

    // Fails harness invariants independently of Release assertions.
    static void Check(bool condition, const char* label) {
        if (!condition) throw std::runtime_error(label);
    }

    // Waits for the observed ConnectNamedPipe stage without consuming a pipe connection.
    void Ready() {
        const auto end = GetTickCount64() + 3000;
        DWORD lastError = ERROR_SUCCESS;
        while (GetTickCount64() < end) {
            if (app.m_ipc.GetDiagnostics().listening) return;
            WaitNamedPipeW(pipe.c_str(), 50);
            lastError = GetLastError();
            Sleep(2);
        }
        std::cerr << "endpoint error=" << lastError << " handled=" << handled << '\n';
        throw std::runtime_error("isolated endpoint not ready");
    }

    // Launches only a child owned by this fixture, with no shell and no visible console.
    PROCESS_INFORMATION Start(const std::wstring& mode, const std::wstring& path) {
        Ready();
        const auto childExe = mode == L"--ipc-app-reference" ? root / "reference-tests.exe" : exe;
        std::wstring command = L"\"" + childExe.wstring() + L"\" " + mode + L" \"" + pipe + L"\" \"" + path + L"\"";
        STARTUPINFOW startup = {};
        startup.cb = sizeof(startup);
        PROCESS_INFORMATION process = {};
        Check(CreateProcessW(childExe.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
            nullptr, root.c_str(), &startup, &process) != FALSE, "sender process");
        children.push_back(process);
        return process;
    }

    // Bounded cleanup affects only the specific child handles returned by Start.
    DWORD Finish(PROCESS_INFORMATION process) {
        const DWORD wait = WaitForSingleObject(process.hProcess, 5000);
        if (wait != WAIT_OBJECT_0) {
            TerminateProcess(process.hProcess, 99);
            WaitForSingleObject(process.hProcess, 1000);
        }
        DWORD code = 99;
        GetExitCodeProcess(process.hProcess, &code);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        std::erase_if(children, [&process](const PROCESS_INFORMATION& owned) {
            return owned.dwProcessId == process.dwProcessId;
        });
        Check(wait == WAIT_OBJECT_0, "sender timeout (owned child terminated)");
        return code;
    }

    // Pumps actual Window::HandleMessage and App::OnIpcOpenPath until the expected count or deadline.
    void Pump(uint32_t expected) {
        const auto end = GetTickCount64() + 1500;
        do {
            MSG message = {};
            while (PeekMessageW(&message, app.m_window.GetHwnd(), 0, 0, PM_REMOVE)) {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
            if (handled >= expected) return;
            Sleep(2);
        } while (GetTickCount64() < end);
    }

    // Reads the real applied image path and checks uploaded bitmap availability, not just sender exit.
    std::wstring CurrentPath() const {
        if (app.m_currentImage && app.m_currentImage->gpuBitmap) return app.m_currentImage->filePath;
        if (app.m_animatedImage && !app.m_animatedImage->frames.empty() && app.m_animatedImage->frames[0].gpuBitmap)
            return app.m_animatedImage->filePath;
        return {};
    }

    // Requires successful image application and exact delivery counts, including after an extra drain.
    int Run() {
        const auto jpeg = (root / "test/seed.jpg").lexically_normal().wstring();
        const auto png = (root / L"test/imagen Ω.png").lexically_normal().wstring();
        Check(std::filesystem::exists(jpeg) && std::filesystem::exists(png), "copied fixtures");
        Check(Finish(Start(L"--ipc-write", jpeg)) == 0, "direct control write");
        Pump(1);
        Check(handled == 1 && CurrentPath() == jpeg, "direct control applied JPEG bitmap");
        uint32_t writeOk = 0, applied = 0, failed = 0, unintendedPrimary = 0;
        for (uint32_t trial = 0; trial < 5; ++trial) {
            const auto& path = (trial % 2 == 0) ? png : jpeg;
            const uint32_t before = handled;
            const DWORD code = Finish(Start(L"--ipc-client", path));
            Pump(before + (code == 0 ? 1u : 0u));
            const bool changed = handled > before && CurrentPath() == path;
            if (code == 0) ++writeOk;
            else if (code == 20) ++unintendedPrimary;
            else ++failed;
            if (changed) ++applied;
            std::cout << "PROBE trial=" << trial << " exit=" << code << " handler=" << (handled - before)
                << " applied=" << changed << '\n';
            Check(code == 0 && handled == before + 1 && changed, "Initialize must apply exactly one valid bitmap");
        }
        auto before = handled;
        const auto oldPath = CurrentPath();
        Check(Finish(Start(L"--ipc-client", png)) == 0, "delayed UI write");
        Sleep(50); // Server may post while UI intentionally does not pump.
        Check(handled == before && CurrentPath() == oldPath, "write alone does not apply on UI");
        Pump(before + 1);
        Check(handled == before + 1 && CurrentPath() == png, "deferred UI applies Unicode PNG bitmap");
        std::cout << "DELAYED_UI write_ok=1 applied_before_pump=0 applied_after_pump=1\n";

        before = handled;
        Check(Finish(Start(L"--ipc-client", (root / "test/absent.png").wstring())) == 0, "invalid path transported");
        Pump(before + 1);
        Check(handled == before + 1 && CurrentPath() == png, "invalid path ignored by App");
        Check(Finish(Start(L"--ipc-client", L"")) == 0, "empty write transport status");
        Pump(handled);
        Check(CurrentPath() == png, "empty path leaves current image");
        std::cout << "INVALID_PATH transported=1 applied=0; EMPTY_PATH applied=0\n";

        before = handled;
        auto first = Start(L"--ipc-client", jpeg);
        auto second = Start(L"--ipc-client", png);
        const auto firstCode = Finish(first);
        const auto secondCode = Finish(second);
        Pump(handled + (firstCode == 0 ? 1u : 0u) + (secondCode == 0 ? 1u : 0u));
        Check(firstCode == 0 && secondCode == 0 && handled == before + 2, "concurrent clients each delivered once");
        Check(CurrentPath() == jpeg || CurrentPath() == png, "concurrent final bitmap valid");
        Check((appliedPaths[before] == jpeg && appliedPaths[before + 1] == png)
            || (appliedPaths[before] == png && appliedPaths[before + 1] == jpeg), "both concurrent bitmap paths applied");
        std::cout << "CONCURRENT existing_receiver=1 client1_exit=" << firstCode << " client2_exit=" << secondCode << '\n';
        before = handled;
        Check(Finish(Start(L"--ipc-multiple", jpeg)) == 0, "multiple/lite parsing");
        Pump(handled);
        Check(handled == before, "multiple/lite child did not forward (parser/branch probe only)");
        std::cout << "MULTIPLE_LITE parser_and_bypass=pass full_app_startup=not_tested\n";
        auto snapshot = app.m_ipc.GetDiagnostics();
        Sleep(100);
        Pump(handled + 1); // Wait through a full quiet interval to catch late duplicates.
        snapshot = app.m_ipc.GetDiagnostics();
        Check(handled == 10 && snapshot.received == 10 && snapshot.posted == 10 && snapshot.postFailures == 0,
            "exact final receive/post/handler counts without duplicates");
        SecondaryCases(jpeg, png);
        FailureCases(jpeg);
        std::cout << "SUMMARY probes=5 write_ok=" << writeOk << " applied=" << applied << " write_failed=" << failed
            << " unintended_primary=" << unintendedPrimary << " received=" << snapshot.received
            << " posted=" << snapshot.posted << " post_failures=" << snapshot.postFailures << '\n';
        return 0;
    }

    // Runs real App secondaries; optional local reference uses the same pipe, settings and fixtures.
    void SecondaryCases(const std::wstring& jpeg, const std::wstring& png) {
        std::ofstream(root / "user_data/fav.txt") << SingleInstanceIpc::WideToUtf8(png) << "\n\n0\n\n";
        const bool compare = std::filesystem::exists(root / "reference-tests.exe");
        for (int round = 0; round < 6; ++round) {
            for (int side = 0; side < (compare ? 2 : 1); ++side) {
                const bool reference = compare && (round + side) % 2 == 0;
                const auto before = handled;
                const auto start = std::chrono::steady_clock::now();
                Check(Finish(Start(reference ? L"--ipc-app-reference" : L"--ipc-app-client", png)) == 0,
                    "real App secondary exits successfully");
                const auto exited = std::chrono::steady_clock::now();
                Check(handled == before, "secondary cannot acknowledge UI application");
                Pump(before + 1);
                const auto applied = std::chrono::steady_clock::now();
                Check(handled == before + 1 && CurrentPath() == png, "App secondary applies exact bitmap");
                std::cout << "SECONDARY round=" << round << " reference=" << reference
                    << " process_us=" << std::chrono::duration<double, std::micro>(exited - start).count()
                    << " apply_after_exit_us=" << std::chrono::duration<double, std::micro>(applied - exited).count() << '\n';
            }
        }
        const auto before = handled;
        auto first = Start(L"--ipc-app-client", jpeg);
        auto second = Start(L"--ipc-app-client", png);
        Check(Finish(first) == 0 && Finish(second) == 0, "concurrent real App secondaries");
        Pump(before + 2);
        Check(handled == before + 2, "concurrent App requests delivered exactly once");
        Check((appliedPaths[before] == jpeg && appliedPaths[before + 1] == png)
            || (appliedPaths[before] == png && appliedPaths[before + 1] == jpeg), "both App bitmap paths");
    }

    // Exercises open recovery/errors, shutdown event failures and failed post ownership on private endpoints only.
    void FailureCases(const std::wstring& path) {
        for (int failAt : {1, 2}) {
            SingleInstanceIpc failed;
            failed.m_pipeName = pipe + L"-event-failure";
            eventCall = 0;
            failedEventCall = failAt;
            {
                ScopedStartupImport hook("CreateEventW", reinterpret_cast<void*>(&FailIpcEvent));
                originalCreateEvent = reinterpret_cast<decltype(originalCreateEvent)>(hook.original);
                Check(failed.Initialize(nullptr, L"") == IpcInitResult::ForwardFailed, "IPC event creation failure explicit");
            }
            Check(failed.GetDiagnostics().sendError == ERROR_NOT_ENOUGH_MEMORY &&
                !failed.m_serverThread.joinable() && !failed.m_stopEvent && !failed.m_ioEvent,
                "partial event setup releases handles and starts no thread");
            failed.Shutdown();
        }
        const auto delayedPipe = pipe + L"-delayed";
        bool received = false;
        std::thread delayed([&] {
            Sleep(100); // Force ERROR_FILE_NOT_FOUND before the first server instance exists.
            HANDLE server = CreateNamedPipeW(delayedPipe.c_str(), PIPE_ACCESS_INBOUND,
                PIPE_TYPE_BYTE | PIPE_WAIT, 1, 0, 4096, 0, nullptr);
            if (server == INVALID_HANDLE_VALUE) return;
            // Poll readiness with nonblocking connection so harness failure cannot leave a blocked join.
            DWORD mode = PIPE_NOWAIT;
            SetNamedPipeHandleState(server, &mode, nullptr, nullptr);
            const auto end = GetTickCount64() + 3000;
            while (GetTickCount64() < end) {
                ConnectNamedPipe(server, nullptr);
                char bytes[4096] = {};
                DWORD count = 0;
                if (ReadFile(server, bytes, sizeof(bytes), &count, nullptr) && count > 0) {
                    received = std::string(bytes, count) == SingleInstanceIpc::WideToUtf8(path);
                    break;
                }
                Sleep(2);
            }
            DisconnectNamedPipe(server);
            CloseHandle(server);
        });
        DWORD error = 0;
        const bool recovered = SingleInstanceIpc::SendMessage(delayedPipe, SingleInstanceIpc::WideToUtf8(path), 2000, &error);
        delayed.join();
        Check(recovered && error == ERROR_SUCCESS && received, "missing pipe recovers with one complete payload");
        const auto start = GetTickCount64();
        Check(!SingleInstanceIpc::SendMessage(delayedPipe, "absent", 80, &error)
            && error == ERROR_FILE_NOT_FOUND && GetTickCount64() - start < 1000, "missing pipe deadline explicit error");

        const auto busyPipe = pipe + L"-busy";
        HANDLE server = CreateNamedPipeW(busyPipe.c_str(), PIPE_ACCESS_INBOUND,
            PIPE_TYPE_BYTE | PIPE_WAIT, 1, 0, 4096, 0, nullptr);
        Check(server != INVALID_HANDLE_VALUE, "busy fixture server");
        HANDLE held = CreateFileW(busyPipe.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (held == INVALID_HANDLE_VALUE) {
            CloseHandle(server);
            Check(false, "busy fixture connection");
        }
        SingleInstanceIpc client;
        client.m_pipeName = busyPipe;
        const auto busyStart = GetTickCount64();
        const auto outcome = client.Initialize(nullptr, path);
        const auto busyElapsed = GetTickCount64() - busyStart;
        App failedApp;
        failedApp.m_ipc.m_pipeName = busyPipe;
        AppConfig config;
        config.initialFilePath = path;
        const auto appOutcome = failedApp.Initialize(config);
        CloseHandle(held);
        DisconnectNamedPipe(server);
        CloseHandle(server);
        const auto state = client.GetDiagnostics();
        Check(outcome == IpcInitResult::ForwardFailed && state.detected && state.sendAttempted
            && !state.writeSucceeded && state.sendError == ERROR_PIPE_BUSY && !client.m_running
            && !client.m_serverThread.joinable() && busyElapsed < 3500, "busy receiver fails explicitly without extra primary");
        Check(appOutcome == InitResult::ForwardFailed && failedApp.GetIpcSendError() == ERROR_PIPE_BUSY
            && !failedApp.m_ipc.m_running && !failedApp.m_ipc.m_serverThread.joinable(), "real App startup preserves forwarding failure");
        Check(failedApp.m_imageLoader.GetWicFactory() == nullptr && failedApp.m_tagManager.GetWicFactory() == nullptr
            && !failedApp.m_renderer.IsInitialized()
            && !IsWindowVisible(failedApp.m_window.GetHwnd()), "failed secondary skips heavy services and visible UI");

        HWND destroyed = CreateWindowExW(0, L"STATIC", L"post failure", 0,
            0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandleW(nullptr), nullptr);
        Check(destroyed != nullptr, "post failure fixture HWND");
        DestroyWindow(destroyed);
        SingleInstanceIpc noWindow;
        noWindow.m_pipeName = pipe + L"-post-failure";
        Check(noWindow.Initialize(destroyed, L"") == IpcInitResult::Primary, "post failure server");
        Check(SingleInstanceIpc::SendMessage(noWindow.m_pipeName, SingleInstanceIpc::WideToUtf8(path), 2000, &error),
            "write does not acknowledge failed post");
        const auto postEnd = GetTickCount64() + 2000;
        while (noWindow.GetDiagnostics().postFailures == 0 && GetTickCount64() < postEnd) Sleep(2);
        const auto post = noWindow.GetDiagnostics();
        noWindow.Shutdown();
        Check(post.received == 1 && post.posted == 0 && post.postFailures == 1, "failed post has no transferred payload");
        std::cout << "FAILURES missing_recovered=1 missing_timeout=explicit busy_forward_failed=1 failed_post=1\n";
    }

    // Launches only an isolated extracted product EXE, switches the remaining modes and checks graceful process exit.
    static int PackagedCases(const std::filesystem::path& executable, const std::filesystem::path& image) {
        const auto root = executable.parent_path();
        Check(executable.filename() == L"lenteuv.exe" && image.parent_path() == root / L"test" &&
            std::filesystem::is_directory(root / L"user_data"), "package smoke requires owned EXE/test/user_data staging");
        for (const auto* flag : {L"--multiple", L"--lite"}) {
            std::wstring command = L"\"" + executable.wstring() + L"\" " + flag + L" --windowed \"" + image.wstring() + L"\"";
            STARTUPINFOW startup = {};
            startup.cb = sizeof(startup);
            startup.dwFlags = STARTF_USESHOWWINDOW;
            startup.wShowWindow = SW_HIDE;
            PROCESS_INFORMATION child = {};
            Check(CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, FALSE, 0, nullptr,
                root.c_str(), &startup, &child) != FALSE, "packaged product launch");
            // Owns only this launched child; failure cleanup never targets another app instance.
            struct OwnedChild {
                PROCESS_INFORMATION process;
                // Reaps the private child on failed assertions; successful cases already exited normally.
                ~OwnedChild() {
                    if (WaitForSingleObject(process.hProcess, 0) == WAIT_TIMEOUT) {
                        TerminateProcess(process.hProcess, 99);
                        WaitForSingleObject(process.hProcess, 1000);
                    }
                    CloseHandle(process.hThread); CloseHandle(process.hProcess);
                }
            } owned{child};
            Check(WaitForInputIdle(child.hProcess, 5000) == 0, "packaged product reaches message loop");
            struct WindowSearch { DWORD pid; HWND hwnd; } search{child.dwProcessId, nullptr};
            // Finds the application class for this child PID, never an existing personal window.
            EnumWindows([](HWND window, LPARAM parameter) -> BOOL {
                auto& target = *reinterpret_cast<WindowSearch*>(parameter);
                DWORD pid = 0; GetWindowThreadProcessId(window, &pid);
                wchar_t name[64] = {};
                if (pid == target.pid && GetClassNameW(window, name, 64) && std::wstring_view(name) == L"LenteUVWindow") {
                    target.hwnd = window; return FALSE;
                }
                return TRUE;
            }, reinterpret_cast<LPARAM>(&search));
            Check(search.hwnd != nullptr, "packaged product main HWND");
            for (UINT key : {VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD4, VK_NUMPAD1}) {
                PostMessageW(search.hwnd, WM_KEYDOWN, key, 0);
                Sleep(100);
                Check(WaitForSingleObject(child.hProcess, 0) == WAIT_TIMEOUT, "packaged product survives mode transition");
            }
            Check(PostMessageW(search.hwnd, WM_CLOSE, 0, 0) != FALSE, "packaged product close");
            Check(WaitForSingleObject(child.hProcess, 5000) == WAIT_OBJECT_0, "packaged product graceful shutdown");
            DWORD code = 99; GetExitCodeProcess(child.hProcess, &code);
            Check(code == 0, "packaged product exit zero");
            std::wcout << L"PACKAGE " << flag << L" modes=3 reserved_num2=sent graceful_exit=0\n";
        }
        return 0;
    }

    // Child role uses the production transport/Initialize; failure exits preserve Win32 error numbers.
    static int Client(int argc, wchar_t** argv) {
        Check(argc == 4, "sender arguments");
        const std::wstring mode = argv[1];
        if (mode == L"--ipc-package") return PackagedCases(argv[2], argv[3]);
        if (mode == L"--ipc-startup") return StartupCases(argv[2], std::filesystem::path(argv[3]).lexically_normal().wstring());
        if (mode == L"--ipc-app-client" || mode == L"--ipc-app-reference") {
            App secondary;
            secondary.m_ipc.m_pipeName = argv[2];
            AppConfig config;
            config.initialFilePath = argv[3];
            const auto start = std::chrono::steady_clock::now();
            const auto result = secondary.Initialize(config);
            const auto micros = std::chrono::duration<double, std::micro>(
                std::chrono::steady_clock::now() - start).count();
            Check(result == InitResult::AlreadyRunning, "real secondary forwards");
            Check(!IsWindowVisible(secondary.m_window.GetHwnd())
                && !secondary.m_renderer.IsInitialized(), "secondary has no visible/rendered UI");
            if (mode == L"--ipc-app-client") {
                Check(secondary.m_imageLoader.GetWicFactory() == nullptr
                    && secondary.m_tagManager.GetWicFactory() == nullptr, "secondary skips WIC and tags");
                Check(!secondary.m_folderData.IsFavorite(config.initialFilePath), "secondary skips fixture favorites");
            }
            std::ofstream timing("secondary-timing.txt", std::ios::app);
            timing << micros << '\n';
            return 0;
        }
        if (mode == L"--ipc-multiple") {
            wchar_t name[] = L"lenteuv.exe", multiple[] = L"--multiple", lite[] = L"--lite";
            wchar_t* flags[] = {name, multiple};
            Check(App::ParseCommandLine(2, flags).multiple, "multiple flag");
            flags[1] = lite;
            Check(App::ParseCommandLine(2, flags).multiple, "lite flag");
            return 0;
        }
        if (mode == L"--ipc-write") {
            DWORD error = 0;
            const bool ok = SingleInstanceIpc::SendMessage(argv[2], SingleInstanceIpc::WideToUtf8(argv[3]), 2000, &error);
            return ok ? 0 : static_cast<int>(2000 + error);
        }
        Check(mode == L"--ipc-client", "known child role");
        SingleInstanceIpc client;
        client.m_pipeName = argv[2];
        const auto result = client.Initialize(nullptr, argv[3]);
        if (result == IpcInitResult::Primary) {
            // A competing primary must not send a shutdown sentinel to the real receiver on unwind.
            ExitProcess(20);
        }
        const auto snapshot = client.GetDiagnostics();
        Check(snapshot.detected && snapshot.sendAttempted, "client detected a receiver");
        Check((result == IpcInitResult::Forwarded) == snapshot.writeSucceeded, "explicit result matches transport");
        return result == IpcInitResult::Forwarded ? 0 : static_cast<int>(1000 + snapshot.sendError);
    }

private:
    // Fails one event allocation on the owner thread; delegates other calls and never patches a system DLL.
    static HANDLE WINAPI FailIpcEvent(LPSECURITY_ATTRIBUTES attributes, BOOL manual, BOOL initial, LPCWSTR name) {
        if (++eventCall == failedEventCall) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return nullptr;
        }
        return originalCreateEvent(attributes, manual, initial, name);
    }

    // Injects UI reentry or WIC failure on the initializing thread; other COM calls are forwarded.
    static HRESULT WINAPI StartupCoCreate(REFCLSID clsid, LPUNKNOWN outer, DWORD context, REFIID iid, LPVOID* result) {
        if (clsid == CLSID_WICImagingFactory2 && beforeWic) {
            beforeWic();
            if (failWic) { *result = nullptr; return E_FAIL; }
        }
        return originalCoCreate(clsid, outer, context, iid, result);
    }

    // Forces the normal Window::Create failure branch without changing production code or OS classes.
    static HWND WINAPI FailCreateWindow(DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int,
        HWND, HMENU, HINSTANCE, LPVOID) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return nullptr;
    }

    // Forces the renderer's existing device failure branch after the primary server has started.
    static HRESULT WINAPI FailCreateDevice(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT,
        const D3D_FEATURE_LEVEL*, UINT, UINT, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**) {
        return E_FAIL;
    }

    // Exercises actual startup/failures and WM_CLOSE versus hide-to-tray with isolated data/endpoint.
    static int StartupCases(const std::wstring& endpoint, const std::wstring& path) {
        wchar_t executable[MAX_PATH] = {};
        const auto length = GetModuleFileNameW(nullptr, executable, MAX_PATH);
        Check(length > 0 && length < MAX_PATH, "startup executable path");
        const auto isolatedRoot = std::filesystem::path(executable).parent_path();
        const auto fixtureDirectory = std::filesystem::path(path).parent_path();
        Check(fixtureDirectory == isolatedRoot / "test" && std::filesystem::is_directory(isolatedRoot / "user_data"),
            "startup requires executable-local copied fixtures and user_data");
        const auto settingsPath = fixtureDirectory.parent_path() / "user_data/settings.json";
        const auto sessionPath = settingsPath.parent_path() / "session.txt";
        const auto earlyPath = (fixtureDirectory / L"imagen Ω.png").wstring();
        ScopedStartupImport hook("CoCreateInstance", reinterpret_cast<void*>(&StartupCoCreate));
        originalCoCreate = reinterpret_cast<decltype(originalCoCreate)>(hook.original);
        for (int scenario = 0; scenario < 10; ++scenario) {
            std::ofstream(settingsPath) << "{\"settings\":{\"single_instance\":["
                << (scenario == 4 ? "false" : "true")
                << "],\"hide_on_app_start\":[true],\"show_fullscreen\":[false]}}";
            std::ofstream(sessionPath, std::ios::trunc);
            App primary;
            primary.m_ipc.m_pipeName = endpoint + L"-startup-" + std::to_wstring(scenario);
            AppConfig config;
            config.initialFilePath = path;
            if (scenario == 2 || scenario == 3) {
                wchar_t name[] = L"lenteuv.exe", multiple[] = L"--multiple", lite[] = L"--lite";
                wchar_t* flags[] = {name, scenario == 2 ? multiple : lite};
                config = App::ParseCommandLine(2, flags);
                config.initialFilePath = path;
            }
            if (scenario >= 5 && scenario <= 7) {
                config.initialFilePath.clear();
                if (scenario == 5) config.initialFolderPath = fixtureDirectory.wstring();
                if (scenario == 6) {
                    std::ofstream(sessionPath) << SingleInstanceIpc::WideToUtf8(fixtureDirectory.wstring()) << "\n\n\n\n";
                }
            }
            const bool bypass = scenario >= 2 && scenario <= 4;
            failWic = scenario == 0;
            bool injected = false;
            beforeWic = [&] {
                Check(primary.m_window.GetHwnd() != nullptr && !primary.m_renderer.IsInitialized(), "WIC prerequisites");
                if (scenario <= 1 || scenario == 9) {
                    DWORD error = 0;
                    uint32_t count = 0;
                    for (const auto& requested : {path, earlyPath}) {
                        Check(SingleInstanceIpc::SendMessage(primary.m_ipc.m_pipeName,
                            SingleInstanceIpc::WideToUtf8(requested), 2000, &error), "early startup transport");
                        const auto deadline = GetTickCount64() + 2000;
                        ++count;
                        while (primary.m_ipc.GetDiagnostics().posted < count && GetTickCount64() < deadline) Sleep(2);
                        Check(primary.m_ipc.GetDiagnostics().posted == count, "early path posted");
                        MSG message = {};
                        Check(PeekMessageW(&message, primary.m_window.GetHwnd(), WM_USER + 4501,
                            WM_USER + 4501, PM_REMOVE) != FALSE, "early path queued");
                        DispatchMessageW(&message); // Deliberate reentry while WIC/renderer/pages are not ready.
                        Check(!primary.m_currentImage && !primary.m_animatedImage, "early path retained without decode");
                    }
                    injected = true;
                }
            };
            std::unique_ptr<ScopedStartupImport> failure;
            if (scenario == 8) failure = std::make_unique<ScopedStartupImport>("CreateWindowExW", reinterpret_cast<void*>(&FailCreateWindow));
            if (scenario == 9) failure = std::make_unique<ScopedStartupImport>("D3D11CreateDevice", reinterpret_cast<void*>(&FailCreateDevice));
            const auto oldDirectory = std::filesystem::current_path();
            if (scenario == 7) std::filesystem::current_path(fixtureDirectory);
            const auto result = primary.Initialize(config);
            std::filesystem::current_path(oldDirectory);
            failure.reset();
            beforeWic = {};
            if (failWic || scenario >= 8) {
                Check(result == InitResult::Failed && !primary.m_currentImage && !primary.m_animatedImage,
                    "partial startup failure preserves error without applying early path");
                Check(scenario == 8 ? !primary.m_ipc.m_running : injected, "failure stage has expected server/path");
                if (scenario == 9) ShellIntegrator::UnregisterTrayIcon(primary.m_window.GetHwnd());
            } else {
                Check(result == InitResult::Success && primary.m_renderer.IsInitialized()
                    && primary.m_imageLoader.GetWicFactory() != nullptr, "full primary or bypass initialized");
                Check(primary.m_currentImage && primary.m_currentImage->gpuBitmap, "startup bitmap applied");
                if (scenario <= 4) Check(primary.m_currentImage->filePath == (injected ? earlyPath : path),
                    "retained path overrides initial file only after startup");
                Check(primary.m_ipc.m_running.load() == !bypass, "bypass or primary server");
                Check(!primary.m_folders.empty(), "folder/session/current-directory restored");
                for (const auto page : {Page::Viewer, Page::Waterfall, Page::Board}) {
                    primary.SetPage(page);
                    primary.RenderFrame(); // Native device/presentation smoke, not a visual assertion.
                }
                primary.m_window.Show(SW_HIDE);
                ShellIntegrator::UnregisterTrayIcon(primary.m_window.GetHwnd());
                if (scenario == 1) {
                    primary.m_settings.hide_to_tray_on_close = true;
                    Check(!primary.m_window.events.onCloseRequest() && primary.m_ipc.m_running,
                        "hiding to tray keeps IPC alive");
                    primary.m_settings.hide_to_tray_on_close = false;
                    const auto before = primary.m_ipc.GetDiagnostics().posted;
                    Check(SingleInstanceIpc::SendMessage(primary.m_ipc.m_pipeName,
                        SingleInstanceIpc::WideToUtf8(path)), "pending path before native close");
                    const auto end = GetTickCount64() + 2000;
                    while (primary.m_ipc.GetDiagnostics().posted == before && GetTickCount64() < end) Sleep(1);
                    Check(primary.m_ipc.GetDiagnostics().posted == before + 1, "native close has pending payload");
                    const auto current = primary.m_currentImage;
                    SendMessageW(primary.m_window.GetHwnd(), WM_CLOSE, 0, 0);
                    Check(!IsWindow(primary.m_window.GetHwnd()) && !primary.m_ipc.m_serverThread.joinable(),
                        "IPC stops before actual HWND destruction");
                    Check(primary.m_currentImage == current, "native close discards path without applying it");
                    MSG quit = {};
                    Check(PeekMessageW(&quit, nullptr, WM_QUIT, WM_QUIT, PM_REMOVE) && quit.message == WM_QUIT,
                        "native close still posts quit");
                }
            }
            if (result == InitResult::Success) {
                primary.m_ipc.Shutdown();
                primary.m_imageLoader.Shutdown();
            } // Failed startup deliberately relies on member RAII, as main does.
            std::cout << "STARTUP scenario=" << scenario << " result=" << static_cast<int>(result)
                << " reentrant_path=" << injected << '\n';
        }
        failWic = false;
        std::filesystem::remove(settingsPath);
        return 0;
    }

    inline static decltype(&CoCreateInstance) originalCoCreate = nullptr;
    inline static decltype(&CreateEventW) originalCreateEvent = nullptr;
    inline static thread_local int eventCall = 0, failedEventCall = 0;
    inline static thread_local std::function<void()> beforeWic;
    inline static thread_local bool failWic = false;
    App app;
    std::filesystem::path root, exe;
    std::wstring pipe;
    uint32_t handled = 0;
    std::vector<std::wstring> appliedPaths; // Bitmap-bearing path after each real UI handler invocation.
    std::vector<PROCESS_INFORMATION> children; // Only handles created by this fixture; reaped on every exit.
};
} // namespace kv

// Selects the parent or child role using Unicode command-line parsing and a local COM apartment.
int RunIpcDiagnosticTests() {
    try {
        kv::ScopedComInit com;
        int count = 0;
        auto args = CommandLineToArgvW(GetCommandLineW(), &count);
        if (!args) throw std::runtime_error("Unicode command-line arguments");
        const bool parent = count == 2 && std::wstring_view(args[1]) == L"--ipc-diagnostics";
        if (!parent) {
            const int code = kv::IpcDiagnosticTests::Client(count, args);
            LocalFree(args);
            return code;
        }
        LocalFree(args);
        kv::detail::g_enableFileLog = true; // Local copied diagnostic EXE only; never enabled in product defaults.
        kv::IpcDiagnosticTests tests;
        return tests.Run();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
