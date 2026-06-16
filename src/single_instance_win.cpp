#include "single_instance.h"
#include <windows.h>

// 命名互斥量做单实例标记。"Local\\" 命名空间 = 当前登录会话内唯一，
// 正好匹配“每个用户一个托盘实例”的需求。句柄故意不关闭，进程退出时由 OS 释放。
bool acquireSingleInstance() {
    HANDLE h = CreateMutexW(nullptr, TRUE, L"Local\\keep-awake-single-instance-mutex");
    if (h == nullptr) return true;  // 无法判断则放行，不阻止启动
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(h);
        return false;
    }
    return true;
}
