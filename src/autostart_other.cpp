#include "autostart.h"

// macOS / Linux 暂不实现开机自启（菜单项将显示为禁用）。
bool autostartSupported() { return false; }
bool autostartEnabled()   { return false; }
bool autostartSet(bool)   { return false; }
