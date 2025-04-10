// autologon.cpp : 定义应用程序的入口点。
//

#include <windows.h>
#include <tchar.h>
#include <winreg.h>
#include <shellapi.h>
#include <strsafe.h>
#include <windowsx.h>

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define ID_CHECKBOX 101
#define ID_USERNAME 102
#define ID_PASSWORD 103
#define ID_SAVE_BTN 104

const TCHAR szWindowClass[] = _T("AutoLogonApp");
const TCHAR szTitle[] = _T("Windows自动登录配置工具");

HINSTANCE hInst;
HWND hCheck, hUser, hPass, hSaveBtn;

struct RegistryKeys {
    LPCTSTR autoLogon = _T("AutoAdminLogon");
    LPCTSTR userName = _T("DefaultUserName");
    LPCTSTR password = _T("DefaultPassword");
} RegKeys;

const LPCTSTR regPath = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");

BOOL IsAdmin() {
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;
    BOOL bIsAdmin = FALSE;

    if (AllocateAndInitializeSid(&NtAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0, &AdministratorsGroup)) {
        CheckTokenMembership(NULL, AdministratorsGroup, &bIsAdmin);
        FreeSid(AdministratorsGroup);
    }
    return bIsAdmin;
}

void ShowError(LPCTSTR msg) {
    MessageBox(NULL, msg, _T("错误"), MB_ICONERROR | MB_OK);
}

LPTSTR ReadRegistryString(HKEY hKey, LPCTSTR valueName) {
    HKEY hRegKey;
    DWORD dwType, dwSize;
    LPTSTR data = NULL;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regPath, 0, KEY_READ, &hRegKey) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hRegKey, valueName, NULL, &dwType, NULL, &dwSize) == ERROR_SUCCESS) {
            if (dwType == REG_SZ) {
                data = (LPTSTR)malloc(dwSize);
                if (RegQueryValueEx(hRegKey, valueName, NULL, NULL, (LPBYTE)data, &dwSize) != ERROR_SUCCESS) {
                    free(data);
                    data = NULL;
                }
            }
        }
        RegCloseKey(hRegKey);
    }
    return data;
}

BOOL WriteRegistryString(HKEY hKey, LPCTSTR valueName, LPCTSTR valueData) {
    HKEY hRegKey;
    DWORD disposition;
    LONG result = RegCreateKeyEx(HKEY_LOCAL_MACHINE, regPath, 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hRegKey, &disposition);

    if (result == ERROR_SUCCESS) {
        result = RegSetValueEx(hRegKey, valueName, 0, REG_SZ,
            (const BYTE*)valueData, 
            static_cast<DWORD>((_tcslen(valueData) + 1) * sizeof(TCHAR)));  // 修复类型转换警告
        RegCloseKey(hRegKey);
    }
    return result == ERROR_SUCCESS;
}

void LoadRegistryValues() {
    LPTSTR autoLogon = ReadRegistryString(HKEY_LOCAL_MACHINE, RegKeys.autoLogon);
    if (autoLogon) {
        SendMessage(hCheck, BM_SETCHECK, 
            (_tcscmp(autoLogon, _T("1")) == 0) ? BST_CHECKED : BST_UNCHECKED, 0);
        free(autoLogon);
    }

    LPTSTR userName = ReadRegistryString(HKEY_LOCAL_MACHINE, RegKeys.userName);
    if (userName) {
        SetWindowText(hUser, userName);
        free(userName);
    }

    LPTSTR password = ReadRegistryString(HKEY_LOCAL_MACHINE, RegKeys.password);
    if (password) {
        SetWindowText(hPass, password);
        free(password);
    }
}

void SaveSettings() {
    if (!IsAdmin()) {
        ShowError(_T("请以管理员身份运行本程序"));
        return;
    }

    TCHAR autoLogon[2] = _T("0");
    if (SendMessage(hCheck, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        _tcscpy_s(autoLogon, _T("1"));
    }

    TCHAR userName[256];
    TCHAR password[256];
    GetWindowText(hUser, userName, ARRAYSIZE(userName));
    GetWindowText(hPass, password, ARRAYSIZE(password));

    BOOL success = TRUE;
    success &= WriteRegistryString(HKEY_LOCAL_MACHINE, RegKeys.autoLogon, autoLogon);
    success &= WriteRegistryString(HKEY_LOCAL_MACHINE, RegKeys.userName, userName);
    success &= WriteRegistryString(HKEY_LOCAL_MACHINE, RegKeys.password, password);

    if (success) {
        MessageBox(NULL, _T("配置已保存，重启后生效"), _T("成功"), MB_ICONINFORMATION | MB_OK);
    }
    else {
        ShowError(_T("注册表写入失败"));
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        CreateWindow(_T("STATIC"), _T("自动登录:"), WS_VISIBLE | WS_CHILD,
            20, 20, 80, 24, hWnd, NULL, hInst, NULL);

        hCheck = CreateWindow(_T("BUTTON"), _T("启用自动登录"), 
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            110, 20, 200, 24, hWnd, (HMENU)ID_CHECKBOX, hInst, NULL);

        CreateWindow(_T("STATIC"), _T("用户名:"), WS_VISIBLE | WS_CHILD,
            20, 60, 80, 24, hWnd, NULL, hInst, NULL);

        hUser = CreateWindow(_T("EDIT"), _T(""), WS_VISIBLE | WS_CHILD | WS_BORDER,
            110, 60, 260, 24, hWnd, (HMENU)ID_USERNAME, hInst, NULL);

        CreateWindow(_T("STATIC"), _T("密码:"), WS_VISIBLE | WS_CHILD,
            20, 100, 80, 24, hWnd, NULL, hInst, NULL);

        hPass = CreateWindow(_T("EDIT"), _T(""), WS_VISIBLE | WS_CHILD | WS_BORDER,
            110, 100, 260, 24, hWnd, (HMENU)ID_PASSWORD, hInst, NULL);

        hSaveBtn = CreateWindow(_T("BUTTON"), _T("保存配置"), 
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            280, 140, 90, 28, hWnd, (HMENU)ID_SAVE_BTN, hInst, NULL);

        LoadRegistryValues();
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_SAVE_BTN) {
            SaveSettings();
        }
        break;

    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = 400;
        mmi->ptMinTrackSize.y = 220;
        mmi->ptMaxTrackSize.x = 400;
        mmi->ptMaxTrackSize.y = 220;
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance;

    HWND hWnd = CreateWindow(szWindowClass, szTitle, 
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 220, NULL, NULL, hInstance, NULL);

    if (!hWnd) return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    return RegisterClassEx(&wcex);
}

int APIENTRY _tWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPTSTR    lpCmdLine,
    _In_ int       nCmdShow)  // 修复批注警告
{
    if (!IsAdmin()) {
        TCHAR path[MAX_PATH];
        GetModuleFileName(NULL, path, MAX_PATH);
        ShellExecute(NULL, _T("runas"), path, NULL, NULL, SW_SHOWNORMAL);
        return 0;
    }

    MyRegisterClass(hInstance);
    if (!InitInstance(hInstance, nCmdShow)) return FALSE;

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
