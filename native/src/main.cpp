// ============================================================================
// Class: None
// Description: Main is the WinMain entry point of the project. lenteuv imageviewer basado en Sergkruma's image viewer.
// Responsibility: Parse parameters, initialize COM, verify single instances via IPC, and initialize `App`.
// ============================================================================
#include "common/pch.h"
#include "core/app.h"
#include <fstream>
#include <cstdio>

static void TraceIpc(std::string_view msg) {
    if (kv::detail::g_enableDebugIpc) {
        std::ofstream dbg("debug_ipc.txt", std::ios::app);
        dbg << msg << std::endl;
        dbg.close();
    }
}

static void SafeTraceIpc(const char* msg) {
    if (kv::detail::g_enableDebugIpc) {
        FILE* f = nullptr;
        if (fopen_s(&f, "debug_ipc.txt", "a") == 0 && f) {
            fprintf(f, "%s\n", msg);
            fclose(f);
        }
    }
}

static void TraceIpcSEH(DWORD code) {
    if (kv::detail::g_enableDebugIpc) {
        FILE* fErr = nullptr;
        if (fopen_s(&fErr, "debug_ipc.txt", "a") == 0 && fErr) {
            fprintf(fErr, "[CRASH] Structured Exception (SEH) caught in wWinMain! ExceptionCode: 0x%X\n", code);
            fclose(fErr);
        }
    }
}

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
    TraceIpc("[MAIN] DPI awareness set.");

    // ── Initialize COM ────────────────────────────────────────────────────
    // STA (Single-Threaded Apartment) is required for WIC, Shell dialogs,
    // and DirectComposition. RAII ensures CoUninitialize on exit.
    kv::ScopedComInit comInit;
    TraceIpc(std::format("[MAIN] ScopedComInit created, isValid={}", comInit.IsValid()));
    if (!comInit.IsValid()) {
        MessageBoxW(nullptr, L"Failed to initialize COM.", L"Fatal Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // ── Parse Command Line ────────────────────────────────────────────────
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    TraceIpc(std::format("[MAIN] CommandLineToArgvW called, argc={}", argc));

    kv::AppConfig config = kv::App::ParseCommandLine(argc, argv);
    TraceIpc(std::format("[MAIN] ParseCommandLine completed. initialFilePath length={}", config.initialFilePath.length()));
    LocalFree(argv);

    // ── Create and Initialize Application ─────────────────────────────────
    kv::App app;
    TraceIpc("[MAIN] kv::App instantiated.");

    kv::InitResult initResult = app.Initialize(config);
    TraceIpc(std::format("[MAIN] app.Initialize returned: {}", (int)initResult));
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
            L"LenteUV — Error de inicialización",
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
    kv::Settings::PreInitDebugIpc();

    SafeTraceIpc("[MAIN] wWinMain started.");

    __try {
        return wWinMainImpl(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        TraceIpcSEH(GetExceptionCode());
        return 1;
    }
}
