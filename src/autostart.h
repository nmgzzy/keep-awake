#pragma once

// 开机自启开关（平台相关实现）。
bool autostartSupported();          // 当前平台是否支持
bool autostartEnabled();            // 当前是否已启用
bool autostartSet(bool enable);     // 启用/关闭，成功返回 true
