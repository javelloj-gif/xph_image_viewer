// ============================================================================
// Class: ShellIntegrator
// Description: ShellIntegrator coordinates Windows shell operations, COM bindings, system tray menus, and Recycle Bin deletes for [IMAGE_VIEWER_APP_NAME].
// Responsibility: Query explorer processes for paths, handle recycle-bin moves, and system tray menu events.
// ============================================================================
#pragma once

#include <vector>
#include <string>
#include <windows.h>

namespace kv {

class ShellIntegrator {
public:
    static HRESULT GetFileListFromActiveExplorerWindow(std::vector<std::wstring>& outFiles, bool fullpaths = true);
    static bool SendFileToRecycleBin(const std::wstring& filePath);

    // Tray Icon management
    static constexpr UINT WM_TRAY_ICON = WM_USER + 4502;
    static constexpr UINT TRAY_ICON_ID = 1001;

    static bool RegisterTrayIcon(HWND hwnd, HICON hIcon, const std::wstring& tooltip);
    static bool UnregisterTrayIcon(HWND hwnd);
    static void ShowTrayContextMenu(HWND hwnd, POINT pt);
    static bool SetRunOnStartup(bool enable);
};

} // namespace kv
