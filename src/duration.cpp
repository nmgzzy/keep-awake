#include "duration.h"
#include <cctype>
#include <cstdio>

namespace {
constexpr long kMaxSeconds = 7L * 24 * 3600;  // 上限 7 天，防误输入
}

bool parseDuration(const std::string& text, int& outSeconds) {
    // 去首尾空白
    size_t b = 0, e = text.size();
    while (b < e && std::isspace((unsigned char)text[b])) b++;
    while (e > b && std::isspace((unsigned char)text[e - 1])) e--;
    if (b == e) return false;
    std::string s = text.substr(b, e - b);

    // 纯数字 -> 按分钟
    bool allDigit = true;
    for (char c : s) {
        if (!std::isdigit((unsigned char)c)) { allDigit = false; break; }
    }

    long total = 0;
    if (allDigit) {
        long v = 0;
        for (char c : s) {
            v = v * 10 + (c - '0');
            if (v > kMaxSeconds) return false;  // 提前止损
        }
        total = v * 60;
    } else {
        // <数字><单位> 的若干段组合，单位 h/m/s
        size_t k = 0;
        bool any = false;
        while (k < s.size()) {
            if (!std::isdigit((unsigned char)s[k])) return false;
            long num = 0;
            while (k < s.size() && std::isdigit((unsigned char)s[k])) {
                num = num * 10 + (s[k] - '0');
                if (num > kMaxSeconds) return false;
                k++;
            }
            if (k >= s.size()) return false;  // 数字后必须跟单位
            char u = (char)std::tolower((unsigned char)s[k]);
            k++;
            long mult;
            if (u == 'h') mult = 3600;
            else if (u == 'm') mult = 60;
            else if (u == 's') mult = 1;
            else return false;
            total += num * mult;
            any = true;
        }
        if (!any) return false;
    }

    if (total <= 0 || total > kMaxSeconds) return false;
    outSeconds = (int)total;
    return true;
}

std::string formatRemaining(int seconds) {
    if (seconds < 0) seconds = 0;
    int h = seconds / 3600;
    int m = (seconds % 3600) / 60;
    int s = seconds % 60;
    char buf[32];
    if (h > 0) std::snprintf(buf, sizeof(buf), "%d:%02d:%02d", h, m, s);
    else std::snprintf(buf, sizeof(buf), "%02d:%02d", m, s);
    return std::string(buf);
}
