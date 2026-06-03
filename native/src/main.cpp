// ============================================================================
// Class: None
// Description: Main is the WinMain entry point of the project, which is based on Krumassan's Image Viewer.
// Responsibility: Parse parameters, initialize COM, verify single instances via IPC, and initialize `App`.
// ============================================================================
#include "common/pch.h"
#include "core/app.h"
#include <fstream>
#include <cstdio>

// ─── wWinMainImpl ────────────────────────────────────────────────────────────
// Actual implementation containing C++ objects, allowing stack unwinding.
int wWinMainImpl(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR    lpCmdLine,
    int       nCmdShow)
{
    // ── Enable Per-Monitor DPI Awareness v2 ───────────────────────────────
    // This is also declared in the manifest, but calling the API ensures
    // it takes effect even if the manifest fails to load.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    {
        std::ofstream dbg("debug_ipc.txt", std::ios::app);
        dbg << "[MAIN] DPI awareness set." << std::endl;
        dbg.close();
    }

    // ── Initialize COM ────────────────────────────────────────────────────
    // STA (Single-Threaded Apartment) is required for WIC, Shell dialogs,
    // and DirectComposition. RAII ensures CoUninitialize on exit.
    kv::ScopedComInit comInit;
    {
        std::ofstream dbg("debug_ipc.txt", std::ios::app);
        dbg << "[MAIN] ScopedComInit created, isValid=" << comInit.IsValid() << std::endl;
        dbg.close();
    }
    if (!comInit.IsValid()) {
        MessageBoxW(nullptr, L"Failed to initialize COM.", L"Fatal Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // ── Parse Command Line ────────────────────────────────────────────────
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    {
        std::ofstream dbg("debug_ipc.txt", std::ios::app);
        dbg << "[MAIN] CommandLineToArgvW called, argc=" << argc << std::endl;
        dbg.close();
    }

    kv::AppConfig config = kv::App::ParseCommandLine(argc, argv);
    {
        std::ofstream dbg("debug_ipc.txt", std::ios::app);
        dbg << "[MAIN] ParseCommandLine completed. initialFilePath length=" << config.initialFilePath.length() << std::endl;
        dbg.close();
    }
    LocalFree(argv);

    // ── Create and Initialize Application ─────────────────────────────────
    kv::App app;
    {
        std::ofstream dbg("debug_ipc.txt", std::ios::app);
        dbg << "[MAIN] kv::App instantiated." << std::endl;
        dbg.close();
    }

    kv::InitResult initResult = app.Initialize(config);
    {
        std::ofstream dbg("debug_ipc.txt", std::ios::app);
        dbg << "[MAIN] app.Initialize returned: " << (int)initResult << std::endl;
        dbg.close();
    }
    if (initResult == kv::InitResult::AlreadyRunning) {
        return 0; // Exit silently since existing instance handled the request
    }
    if (initResult == kv::InitResult::Failed) {
        MessageBoxW(nullptr,
            L"Failed to initialize the application.\n\n"
            L"Possible causes:\n"
            L"• GPU driver does not support Direct3D 11\n"
            L"• Windows version is too old (requires Windows 10 1903+)\n"
            L"• DirectComposition is not available",
            L"Krumassan Image Viewer — Initialization Error",
            MB_OK | MB_ICONERROR);
        return 1;
    }

    // ── Run Main Loop ─────────────────────────────────────────────────────
    int exitCode = app.Run();

    kv::LogInfo(L"Application shutdown complete");
    return exitCode;
}

// ─── WinMain ─────────────────────────────────────────────────────────────────
// Using wWinMain for Unicode support (wide character command line).
int WINAPI wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPWSTR    lpCmdLine,
    _In_     int       nCmdShow)
{
    FILE* f = nullptr;
    if (fopen_s(&f, "debug_ipc.txt", "a") == 0 && f) {
        fprintf(f, "[MAIN] wWinMain started.\n");
        fclose(f);
    }

    __try {
        return wWinMainImpl(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        FILE* fErr = nullptr;
        if (fopen_s(&fErr, "debug_ipc.txt", "a") == 0 && fErr) {
            fprintf(fErr, "[CRASH] Structured Exception (SEH) caught in wWinMain! ExceptionCode: 0x%X\n", GetExceptionCode());
            fclose(fErr);
        }
        return 1;
    }
}
