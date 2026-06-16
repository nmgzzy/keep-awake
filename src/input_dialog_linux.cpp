#include "input_dialog.h"
#include <cstdio>
#include <string>

// 用 zenity 弹输入框；未安装 zenity 或用户取消时返回 false（菜单将降级为仅预设项）。
bool promptCustomDuration(std::string& out) {
    const char* cmd =
        "zenity --entry --title=keep-awake "
        "--text='输入时长（如 90m、1h30m、45）：' 2>/dev/null";
    FILE* p = popen(cmd, "r");
    if (!p) return false;
    std::string r;
    char buf[256];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), p)) > 0) r.append(buf, n);
    int rc = pclose(p);
    if (rc != 0) return false;
    while (!r.empty() && (r.back() == '\n' || r.back() == '\r')) r.pop_back();
    if (r.empty()) return false;
    out = r;
    return true;
}
