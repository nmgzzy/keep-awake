#include "single_instance.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <cstdlib>
#include <string>

// 文件锁做单实例标记。fd 故意保持打开，进程退出时锁自动释放。
static int g_lockFd = -1;

bool acquireSingleInstance() {
    const char* dir = std::getenv("XDG_RUNTIME_DIR");
    if (!dir || !*dir) dir = "/tmp";
    std::string path = std::string(dir) + "/keep-awake.lock";
    g_lockFd = open(path.c_str(), O_CREAT | O_RDWR, 0600);
    if (g_lockFd < 0) return true;  // 无法判断则放行
    if (flock(g_lockFd, LOCK_EX | LOCK_NB) != 0) {
        close(g_lockFd);
        g_lockFd = -1;
        return false;
    }
    return true;
}
