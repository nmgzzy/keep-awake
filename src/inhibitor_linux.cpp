#include "inhibitor.h"
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

// 用 systemd-inhibit 持有一个 sleep 锁：启动一个子进程 `sleep infinity`，
// 子进程存活期间锁有效；stop() 杀掉子进程即释放。
bool Inhibitor::start(bool /*keepDisplayOn*/) {
    if (active_) return true;
    pid_t pid = fork();
    if (pid < 0) return false;
    if (pid == 0) {
        execlp("systemd-inhibit", "systemd-inhibit",
               "--what=sleep", "--who=keep-awake",
               "--why=user request", "--mode=block",
               "sleep", "infinity", (char*)nullptr);
        _exit(127);  // exec 失败
    }
    pid_ = pid;
    active_ = true;
    return true;
}

void Inhibitor::stop() {
    if (active_) {
        if (pid_ > 0) {
            kill(pid_, SIGTERM);
            waitpid(pid_, nullptr, 0);
        }
        pid_ = -1;
        active_ = false;
    }
}
