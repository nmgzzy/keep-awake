#pragma once

// 跨平台“阻止系统休眠”句柄。start/stop 均幂等，析构自动释放。
// 每个平台一份实现（inhibitor_win.cpp / inhibitor_mac.mm / inhibitor_linux.cpp）。
class Inhibitor {
public:
    Inhibitor() = default;
    ~Inhibitor() { stop(); }
    Inhibitor(const Inhibitor&) = delete;
    Inhibitor& operator=(const Inhibitor&) = delete;

    // keepDisplayOn 为 true 时同时阻止屏幕进入睡眠。成功返回 true。
    bool start(bool keepDisplayOn);
    void stop();
    bool active() const { return active_; }

private:
    bool active_ = false;
#if defined(__APPLE__)
    unsigned long assertion_ = 0;   // IOPMAssertionID
#elif !defined(_WIN32)
    int pid_ = -1;                  // systemd-inhibit 子进程
#endif
};
