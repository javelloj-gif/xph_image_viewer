// ============================================================================
// explorer_integration.h — Windows Explorer & Shell Integration
// Krumassan Image Viewer — Native C++20
// ============================================================================
#pragma once

#include <vector>
#include <string>
#include <windows.h>

namespace kv {

class ExplorerIntegration {
public:
    static HRESULT GetFileListFromActiveExplorerWindow(std::vector<std::wstring>& outFiles, bool fullpaths = true);
    static bool SendFileToRecycleBin(const std::wstring& filePath);

    // Tray Icon management
    static constexpr UINT WM_TRAY_ICON = WM_USER + 4502;
    static constexpr UINT TRAY_ICON_ID = 1001;

    static bool RegisterTrayIcon(HWND hwnd, HICON hIcon, const std::wstring& tooltip);
    static bool UnregisterTrayIcon(HWND hwnd);
    static void ShowTrayContextMenu(HWND hwnd, POINT pt);
};

} // namespace kv
