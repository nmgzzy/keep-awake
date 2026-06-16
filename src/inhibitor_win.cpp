#include "inhibitor.h"
#include <windows.h>

bool Inhibitor::start(bool keepDisplayOn) {
    EXECUTION_STATE flags = ES_CONTINUOUS | ES_SYSTEM_REQUIRED;
    if (keepDisplayOn) flags |= ES_DISPLAY_REQUIRED;
    // 设置后状态会一直保持，直到本线程再次调用 SetThreadExecutionState。
    active_ = (SetThreadExecutionState(flags) != 0);
    return active_;
}

void Inhibitor::stop() {
    if (active_) {
        SetThreadExecutionState(ES_CONTINUOUS);  // 清除所有请求，恢复正常休眠
        active_ = false;
    }
}
