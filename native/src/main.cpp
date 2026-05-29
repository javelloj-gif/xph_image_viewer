// ============================================================================
// main.cpp — Application Entry Point
// Krumassan Image Viewer — Native C++20
//
// Entry point for the Windows application (WinMain).
// Responsibilities:
//   1. Initialize COM (required for WIC, DirectComposition, Shell)
//   2. Parse command-line arguments
//   3. Create and initialize the App
//   4. Run the main message loop
//   5. Clean shutdown
// ============================================================================

#include "pch.h"
#include "app.h"

// ─── WinMain ─────────────────────────────────────────────────────────────────
// Using wWinMain for Unicode support (wide character command line).

int WINAPI wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPWSTR    lpCmdLine,
    _In_     int       nCmdShow)
{
    // ── Enable Per-Monitor DPI Awareness v2 ───────────────────────────────
    // This is also declared in the manifest, but calling the API ensures
    // it takes effect even if the manifest fails to load.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // ── Initialize COM ────────────────────────────────────────────────────
    // STA (Single-Threaded Apartment) is required for WIC, Shell dialogs,
    // and DirectComposition. RAII ensures CoUninitialize on exit.
    kv::ScopedComInit comInit;
    if (!comInit.IsValid()) {
        MessageBoxW(nullptr, L"Failed to initialize COM.", L"Fatal Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // ── Parse Command Line ────────────────────────────────────────────────
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    kv::AppConfig config = kv::App::ParseCommandLine(argc, argv);
    LocalFree(argv);

    // ── Create and Initialize Application ─────────────────────────────────
    kv::App app;

    if (!app.Initialize(config)) {
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
