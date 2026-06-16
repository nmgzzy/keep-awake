#include "duration.h"
#include <cstdio>
#include <string>

static int g_fail = 0;

static void expectParse(const std::string& in, bool okWant, int secWant = 0) {
    int sec = -123;
    bool ok = parseDuration(in, sec);
    bool pass = (ok == okWant) && (!okWant || sec == secWant);
    if (!pass) {
        g_fail++;
        std::printf("FAIL parseDuration(\"%s\"): got ok=%d sec=%d, want ok=%d sec=%d\n",
                    in.c_str(), ok, sec, okWant, secWant);
    }
}

static void expectFormat(int sec, const std::string& want) {
    std::string got = formatRemaining(sec);
    if (got != want) {
        g_fail++;
        std::printf("FAIL formatRemaining(%d): got \"%s\", want \"%s\"\n",
                    sec, got.c_str(), want.c_str());
    }
}

int main() {
    // 纯数字按分钟
    expectParse("45", true, 45 * 60);
    expectParse(" 30 ", true, 30 * 60);
    // 带单位
    expectParse("90m", true, 90 * 60);
    expectParse("1h30m", true, 90 * 60);
    expectParse("2h", true, 2 * 3600);
    expectParse("30s", true, 30);
    expectParse("1H30M", true, 90 * 60);  // 大小写不敏感
    // 非法
    expectParse("", false);
    expectParse("abc", false);
    expectParse("0", false);
    expectParse("0m", false);
    expectParse("10x", false);
    expectParse("h30", false);   // 数字前缺失
    expectParse("99999999999", false);  // 超上限

    // 格式化
    expectFormat(45, "00:45");
    expectFormat(1845, "30:45");
    expectFormat(3930, "1:05:30");
    expectFormat(0, "00:00");

    if (g_fail == 0) {
        std::printf("All duration tests passed.\n");
        return 0;
    }
    std::printf("%d test(s) failed.\n", g_fail);
    return 1;
}
