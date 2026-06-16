#pragma once
#include <string>

// 解析时长文本，成功时把总秒数写入 outSeconds 并返回 true。
// 支持："45"（纯数字按分钟）、"90m"、"1h30m"、"2h"、"30s" 等组合；
// 大小写不敏感，允许首尾空白。结果须 > 0 且 <= 7 天，否则返回 false。
bool parseDuration(const std::string& text, int& outSeconds);

// 把剩余秒数格式化为 "mm:ss"（不足 1 小时）或 "h:mm:ss"。
std::string formatRemaining(int seconds);
