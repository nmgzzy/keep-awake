#include "input_dialog.h"
#include <windows.h>
#include <string>

// 用纯 Win32 控件拼一个极简模态输入框，避免依赖 .rc 对话框模板。
namespace {
const wchar_t* kClass = L"KeepAwakeInputDlg";
HWND  g_edit = nullptr;
bool  g_done = false;
bool  g_ok   = false;

LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_COMMAND:
        if (LOWORD(w) == IDOK)     { g_ok = true;  g_done = true; return 0; }
        if (LOWORD(w) == IDCANCEL) { g_ok = false; g_done = true; return 0; }
        break;
    case WM_CLOSE:
        g_ok = false; g_done = true; return 0;
    }
    return DefWindowProcW(h, m, w, l);
}
}  // namespace

bool promptCustomDuration(std::string& out) {
    HINSTANCE hInst = GetModuleHandleW(nullptr);

    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = kClass;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClassW(&wc);  // 已注册则失败，无妨

    const int W = 360, H = 168;
    int sx = GetSystemMetrics(SM_CXSCREEN);
    int sy = GetSystemMetrics(SM_CYSCREEN);
    HWND h = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, kClass,
        L"自定义时长 - keep-awake",
        WS_POPUPWINDOW | WS_CAPTION,
        (sx - W) / 2, (sy - H) / 2, W, H,
        nullptr, nullptr, hInst, nullptr);
    if (!h) { UnregisterClassW(kClass, hInst); return false; }

    HWND lbl = CreateWindowExW(0, L"STATIC",
        L"输入时长，例如 90m、1h30m、45（纯数字按分钟）：",
        WS_CHILD | WS_VISIBLE, 14, 14, W - 36, 40, h, nullptr, hInst, nullptr);
    g_edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        14, 58, W - 36, 26, h, nullptr, hInst, nullptr);
    HWND ok = CreateWindowExW(0, L"BUTTON", L"确定",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        W - 196, 98, 84, 30, h, (HMENU)IDOK, hInst, nullptr);
    HWND cancel = CreateWindowExW(0, L"BUTTON", L"取消",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        W - 102, 98, 84, 30, h, (HMENU)IDCANCEL, hInst, nullptr);

    HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    for (HWND c : {lbl, g_edit, ok, cancel})
        SendMessageW(c, WM_SETFONT, (WPARAM)font, TRUE);

    ShowWindow(h, SW_SHOW);
    SetForegroundWindow(h);
    SetFocus(g_edit);

    g_done = false; g_ok = false;
    MSG msg;
    while (!g_done && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        // IsDialogMessage 处理 Tab/Enter(默认按钮)/Esc(取消)
        if (!IsDialogMessageW(h, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    bool result = false;
    if (g_ok) {
        wchar_t buf[256] = {0};
        GetWindowTextW(g_edit, buf, 256);
        std::wstring ws(buf);
        if (!ws.empty()) {
            int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1,
                                          nullptr, 0, nullptr, nullptr);
            std::string s(len, 0);
            WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &s[0], len,
                                nullptr, nullptr);
            if (!s.empty() && s.back() == '\0') s.pop_back();
            out = s;
            result = !out.empty();
        }
    }

    DestroyWindow(h);
    UnregisterClassW(kClass, hInst);
    g_edit = nullptr;
    return result;
}
