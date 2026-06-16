#pragma once
#include <string>

// 弹出原生输入框让用户输入自定义时长。
// 用户确认且输入非空时把文本（UTF-8）写入 out 并返回 true；
// 取消、留空或平台不支持时返回 false。
bool promptCustomDuration(std::string& out);
