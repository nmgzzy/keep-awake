// 自测开机自启：启用→确认→关闭，结束后恢复原状（不残留注册表项）。
#include "autostart.h"
#include <cstdio>

int main() {
    std::printf("supported=%d\n", autostartSupported() ? 1 : 0);
    if (!autostartSupported()) return 0;

    bool before = autostartEnabled();
    std::printf("before=%d\n", before ? 1 : 0);

    bool s1 = autostartSet(true);
    std::printf("set(true)=%d enabled=%d\n", s1 ? 1 : 0, autostartEnabled() ? 1 : 0);

    bool s2 = autostartSet(false);
    std::printf("set(false)=%d enabled=%d\n", s2 ? 1 : 0, autostartEnabled() ? 1 : 0);

    bool ok = s1 && s2 && !autostartEnabled();
    std::printf("%s\n", ok ? "autostart check passed" : "autostart check FAILED");
    return ok ? 0 : 1;
}
