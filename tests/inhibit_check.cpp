// 手动/CI 自测：启动防休眠，保持若干秒后释放。
// 运行期间用 `powercfg /requests`（需管理员）应能看到本进程的 SYSTEM 请求。
#include "inhibitor.h"
#include <cstdio>
#include <thread>
#include <chrono>

int main(int argc, char** argv) {
    int secs = (argc > 1) ? atoi(argv[1]) : 8;
    Inhibitor in;
    bool ok = in.start(false);
    std::printf("start=%d active=%d hold=%ds\n", ok ? 1 : 0, in.active() ? 1 : 0, secs);
    std::fflush(stdout);
    std::this_thread::sleep_for(std::chrono::seconds(secs));
    in.stop();
    std::printf("stopped active=%d\n", in.active() ? 1 : 0);
    return ok ? 0 : 1;
}
