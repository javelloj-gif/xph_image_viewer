// ============================================================================
// Class: ShellIntegrator
// Description: ShellIntegrator coordinates Windows shell operations, COM bindings, system tray menus, and Recycle Bin deletes for Fotassa.
// Responsibility: Query explorer processes for paths, handle recycle-bin moves, and system tray menu events.
// ============================================================================
#include "common/pch.h"
#include "i18n/i18n_engine.h"
#include "shell_integrator.h"
#include "common/types.h"
#include <shlwapi.h>
#include <shlobj.h>
#include <exdisp.h>
#include <shellapi.h>
#include <psapi.h>

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Psapi.lib")

namespace kv {

#define EXPLORER_PATH_CAPACITY 2048

HRESULT ShellIntegrator::GetFileListFromActiveExplorerWindow(std::vector<std::wstring>& outFiles, bool fullpaths) {
    WCHAR szItem[EXPLORER_PATH_CAPACITY] = {};
    int SHGDN_value = fullpaths ? SHGDN_FORPARSING : SHGDN_NORMAL;

    HWND hwndFind = GetForegroundWindow();
    if (!hwndFind) return E_FAIL;

    HWND hwndActiveTab = FindWindowExW(hwndFind, nullptr, L"ShellTabWindowClass", nullptr);
    if (!hwndActiveTab) {
        hwndActiveTab = FindWindowExW(hwndFind, nullptr, L"TabWindowClass", nullptr);
    }
    if (!hwndActiveTab) {
        return E_FAIL;
    }

    IShellWindows* psw = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_ALL, IID_IShellWindows, (void**)&psw);
    if (FAILED(hr)) {
        return hr;
    }

    VARIANT v;
    V_VT(&v) = VT_I4;
    IDispatch* pdisp = nullptr;

    for (V_I4(&v) = 0; psw->Item(v, &pdisp) == S_OK; V_I4(&v)++) {
        IWebBrowserApp* pwba = nullptr;
        hr = pdisp->QueryInterface(IID_IWebBrowserApp, (void**)&pwba);
        pdisp->Release();
        if (FAILED(hr)) continue;

        HWND hwndWBA = nullptr;
        hr = pwba->get_HWND((LONG_PTR*)&hwndWBA);
        if (FAILED(hr) || hwndWBA != hwndFind) {
            pwba->Release();
            continue;
        }

        IServiceProvider* psp = nullptr;
        hr = pwba->QueryInterface(IID_IServiceProvider, (void**)&psp);
        pwba->Release();
        if (FAILED(hr)) continue;

        IShellBrowser* psb = nullptr;
        hr = psp->QueryService(SID_STopLevelBrowser, IID_IShellBrowser, (void**)&psb);
        psp->Release();
        if (FAILED(hr)) continue;

        HWND hwndShellBrowser = nullptr;
        hr = psb->GetWindow(&hwndShellBrowser);
        if (FAILED(hr) || hwndShellBrowser != hwndActiveTab) {
            psb->Release();
            continue;
        }

        IShellView* psv = nullptr;
        hr = psb->QueryActiveShellView(&psv);
        psb->Release();
        if (FAILED(hr)) continue;

        IFolderView* pfv = nullptr;
        hr = psv->QueryInterface(IID_IFolderView, (void**)&pfv);
        psv->Release();
        if (FAILED(hr)) continue;

        IShellFolder* psf = nullptr;
        hr = pfv->GetFolder(IID_IShellFolder, (void**)&psf);
        if (FAILED(hr)) {
            pfv->Release();
            continue;
        }

        IEnumIDList* pEnum = nullptr;
        hr = pfv->Items(static_cast<UINT>(SVGIO_FLAG_VIEWORDER), IID_IEnumIDList, (LPVOID*)&pEnum);
        pfv->Release();
        if (FAILED(hr)) {
            psf->Release();
            continue;
        }

        LPITEMIDLIST pidl = nullptr;
        ULONG fetched = 0;
        STRRET str = {};

        while (pEnum->Next(1, &pidl, &fetched) == S_OK && fetched) {
            hr = psf->GetDisplayNameOf(pidl, SHGDN_value, &str);
            if (SUCCEEDED(hr)) {
                std::memset(szItem, 0, sizeof(szItem));
                StrRetToBufW(&str, pidl, szItem, EXPLORER_PATH_CAPACITY);
                outFiles.push_back(szItem);
            }
            CoTaskMemFree(pidl);
        }

        pEnum->Release();
        psf->Release();
        break; // Found the active folder
    }

    psw->Release();
    return S_OK;
}

bool ShellIntegrator::SendFileToRecycleBin(const std::wstring& filePath) {
    SHFILEOPSTRUCTW fileOp = {};
    fileOp.hwnd = nullptr;
    fileOp.wFunc = FO_DELETE;
    
    // Path must be double-null terminated
    std::vector<wchar_t> doubleNullPath(filePath.size() + 2, L'\0');
    std::copy(filePath.begin(), filePath.end(), doubleNullPath.begin());
    
    fileOp.pFrom = doubleNullPath.data();
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

    int result = SHFileOperationW(&fileOp);
    return result == 0 && !fileOp.fAnyOperationsAborted;
}

bool ShellIntegrator::RegisterTrayIcon(HWND hwnd, HICON hIcon, const std::wstring& tooltip) {
    NOTIFYICONDATAW nid = {};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = TRAY_ICON_ID;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAY_ICON;
    nid.hIcon = hIcon;

    // Set tooltip
    wcsncpy_s(nid.szTip, tooltip.c_str(), _countof(nid.szTip));

    // Remove first in case it already exists
    Shell_NotifyIconW(NIM_DELETE, &nid);
    return Shell_NotifyIconW(NIM_ADD, &nid) == TRUE;
}

bool ShellIntegrator::UnregisterTrayIcon(HWND hwnd) {
    NOTIFYICONDATAW nid = {};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = TRAY_ICON_ID;
    return Shell_NotifyIconW(NIM_DELETE, &nid) == TRUE;
}

void ShellIntegrator::ShowTrayContextMenu(HWND hwnd, POINT pt) {
    HMENU hMenu = CreatePopupMenu();
    if (hMenu) {
        // We can display memory details or a quit button
        // Get allocated memory size for process
        DWORD processID = GetCurrentProcessId();
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
        double mbSize = 0.0;
        if (hProcess) {
            PROCESS_MEMORY_COUNTERS pmc;
            if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                mbSize = pmc.WorkingSetSize / 1024.0 / 1024.0;
            }
            CloseHandle(hProcess);
        }

        std::wstring memStr = std::vformat(_(L"Memory Allocated: ~{:.2f} MB"),std::make_wformat_args(mbSize));
        AppendMenuW(hMenu, MF_STRING | MF_GRAYED, 0, memStr.c_str());
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, 1002, _(L"Quit").c_str());

        // Required by track popup menu
        SetForegroundWindow(hwnd);
        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, nullptr);
        DestroyMenu(hMenu);
    }
}

bool ShellIntegrator::SetRunOnStartup(bool enable) {
    HKEY hKey;
    LONG lRes = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey);
    if (lRes == ERROR_SUCCESS) {
        if (enable) {
            WCHAR szPath[MAX_PATH] = {};
            GetModuleFileNameW(nullptr, szPath, MAX_PATH);
            std::wstring val = L"\"" + std::wstring(szPath) + L"\"";
            RegSetValueExW(hKey, L"LenteUV", 0, REG_SZ, 
                           reinterpret_cast<const BYTE*>(val.c_str()), 
                           static_cast<DWORD>((val.size() + 1) * sizeof(wchar_t)));
        } else {
            RegDeleteValueW(hKey, L"LenteUV");
        }
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

} // namespace kv
