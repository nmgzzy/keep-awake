#include "autostart.h"
#include <windows.h>
#include <string>

// 通过 HKCU\...\CurrentVersion\Run 下的注册表值实现当前用户开机自启。
namespace {
const wchar_t* kRunKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const wchar_t* kName   = L"keep-awake";

std::wstring quotedExePath() {
    wchar_t buf[512] = {0};
    GetModuleFileNameW(nullptr, buf, 512);
    return std::wstring(L"\"") + buf + L"\"";
}
}  // namespace

bool autostartSupported() { return true; }

bool autostartEnabled() {
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_READ, &key) != ERROR_SUCCESS)
        return false;
    DWORD type = 0, len = 0;
    LONG r = RegQueryValueExW(key, kName, nullptr, &type, nullptr, &len);
    RegCloseKey(key);
    return r == ERROR_SUCCESS && type == REG_SZ;
}

bool autostartSet(bool enable) {
    HKEY key;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRunKey, 0, nullptr, 0,
                        KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS)
        return false;
    LONG r;
    if (enable) {
        std::wstring p = quotedExePath();
        r = RegSetValueExW(key, kName, 0, REG_SZ, (const BYTE*)p.c_str(),
                           (DWORD)((p.size() + 1) * sizeof(wchar_t)));
    } else {
        r = RegDeleteValueW(key, kName);
        if (r == ERROR_FILE_NOT_FOUND) r = ERROR_SUCCESS;  // 本就没有，视为成功
    }
    RegCloseKey(key);
    return r == ERROR_SUCCESS;
}
