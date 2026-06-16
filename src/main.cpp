#include <string>
#include <chrono>
#include <thread>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#endif

#include "duration.h"
#include "inhibitor.h"
#include "input_dialog.h"
#include "autostart.h"

// 选择 tray.h 的平台实现（须在 include 之前定义）
#ifdef _WIN32
#define TRAY_WINAPI
#elif defined(__APPLE__)
#define TRAY_APPKIT
#else
#define TRAY_APPINDICATOR
#endif
#include "tray.h"

namespace {

using clock_t_ = std::chrono::steady_clock;

// ---- 全局状态 ----
struct State {
    Inhibitor inhibitor;
    bool active = false;
    bool infinite = false;
    clock_t_::time_point endTime;
    long chosenSeconds = 0;  // 用于给对应预设打勾；-1 表示“一直保持”
};

State g;
struct tray g_tray;
bool g_running = true;
bool g_inited = false;
#ifdef _WIN32
// "exe路径,索引"：索引 0=空闲(蓝)，1=防休眠(橙)，与 app.rc 资源 id 对应。
std::string g_iconIdle;
std::string g_iconAwake;
#endif

// 菜单文本（已转成本地编码）的持久存储，菜单项的 text 指针指向这里。
std::string g_lbl[16];
struct tray_menu g_menu[16];

// 预设项
struct Preset { const char* label; long seconds; };
const Preset kPresets[] = {
    {"15 分钟", 15 * 60},
    {"30 分钟", 30 * 60},
    {"1 小时",  60 * 60},
    {"2 小时",  120 * 60},
    {"4 小时",  240 * 60},
};

// 把 UTF-8 文本转成托盘菜单（ANSI Win32 API）可用的本地编码；其他平台原样返回。
std::string toLocal(const std::string& utf8) {
#ifdef _WIN32
    if (utf8.empty()) return {};
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring w(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &w[0], wlen);
    int alen = WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string a(alen, 0);
    WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, &a[0], alen, nullptr, nullptr);
    if (!a.empty() && a.back() == '\0') a.pop_back();
    return a;
#else
    return utf8;
#endif
}

void notifyError(const std::string& msg) {
#ifdef _WIN32
    int wlen = MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), -1, nullptr, 0);
    std::wstring w(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), -1, &w[0], wlen);
    MessageBoxW(nullptr, w.c_str(), L"keep-awake", MB_OK | MB_ICONWARNING);
#else
    (void)msg;
#endif
}

// 主动归还暂时用不到的物理页，降低工作集占用（需要时系统会再换回）。
void trimWorkingSet() {
#ifdef _WIN32
    SetProcessWorkingSetSizeEx(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1, 0);
#endif
}

int remainingSeconds() {
    auto d = std::chrono::duration_cast<std::chrono::seconds>(g.endTime - clock_t_::now());
    return (int)d.count();
}

std::string statusTextUtf8() {
    if (!g.active) return "○ 未激活";
    if (g.infinite) return "● 防休眠中 · 一直保持";
    int rem = remainingSeconds();
    if (rem < 0) rem = 0;
    return "● 防休眠中 · 剩余 " + formatRemaining(rem);
}

// 前置声明回调
void cbPreset(struct tray_menu* m);
void cbForever(struct tray_menu* m);
void cbCustom(struct tray_menu* m);
void cbStop(struct tray_menu* m);
void cbAutostart(struct tray_menu* m);
void cbQuit(struct tray_menu* m);

void buildMenu() {
    int i = 0;
    auto put = [&](const std::string& utf8, int disabled, int checked,
                   void (*cb)(struct tray_menu*), void* ctx) {
        g_lbl[i] = toLocal(utf8);
        g_menu[i] = {const_cast<char*>(g_lbl[i].c_str()), disabled, checked, cb, ctx, nullptr};
        i++;
    };
    auto sep = [&]() {
        g_lbl[i] = "-";
        g_menu[i] = {const_cast<char*>(g_lbl[i].c_str()), 0, 0, nullptr, nullptr, nullptr};
        i++;
    };

    put(statusTextUtf8(), 1, 0, nullptr, nullptr);  // 状态行（不可点）
    sep();
    for (const auto& p : kPresets) {
        int checked = (g.active && !g.infinite && g.chosenSeconds == p.seconds) ? 1 : 0;
        put(p.label, 0, checked, cbPreset, (void*)(intptr_t)p.seconds);
    }
    put("一直保持（直到退出）", 0, (g.active && g.infinite) ? 1 : 0, cbForever, nullptr);
    put("自定义时长…", 0, 0, cbCustom, nullptr);
    sep();
    put("停止", g.active ? 0 : 1, 0, cbStop, nullptr);  // 仅激活时可用
    put("开机自启", autostartSupported() ? 0 : 1,
        autostartEnabled() ? 1 : 0, cbAutostart, nullptr);
    put("退出", 0, 0, cbQuit, nullptr);
    g_menu[i] = {nullptr, 0, 0, nullptr, nullptr, nullptr};  // 终止项

    g_tray.menu = g_menu;
}

void refresh() {
#ifdef _WIN32
    // 按状态切换图标：防休眠中=橙，空闲=蓝。
    g_tray.icon = const_cast<char*>((g.active ? g_iconAwake : g_iconIdle).c_str());
#endif
    buildMenu();
    if (g_inited) tray_update(&g_tray);
}

void startTimed(long sec) {
    g.inhibitor.start(false);
    g.active = true;
    g.infinite = false;
    g.chosenSeconds = sec;
    g.endTime = clock_t_::now() + std::chrono::seconds(sec);
    refresh();
}

void startInfinite() {
    g.inhibitor.start(false);
    g.active = true;
    g.infinite = true;
    g.chosenSeconds = -1;
    refresh();
}

void stopAll() {
    g.inhibitor.stop();
    g.active = false;
    g.infinite = false;
    g.chosenSeconds = 0;
    refresh();
    trimWorkingSet();  // 计时期间换回的页，回到空闲后再归还
}

void cbPreset(struct tray_menu* m) { startTimed((long)(intptr_t)m->context); }
void cbForever(struct tray_menu*)  { startInfinite(); }

void cbCustom(struct tray_menu*) {
    std::string in;
    if (!promptCustomDuration(in)) return;  // 取消/不支持
    int sec = 0;
    if (parseDuration(in, sec)) startTimed(sec);
    else notifyError("无法识别的时长：" + in + "\n请输入如 90m、1h30m、45（分钟）。");
    trimWorkingSet();  // 输入框/消息框会拉起额外的 UI 页，用完归还
}

void cbStop(struct tray_menu*) { stopAll(); }

void cbAutostart(struct tray_menu*) {
    bool now = autostartEnabled();
    if (!autostartSet(!now)) notifyError("设置开机自启失败。");
    refresh();
}

void cbQuit(struct tray_menu*) {
    stopAll();
    g_running = false;
    tray_exit();
}

// 倒计时 tick：每秒刷新一次状态行，到点自动停止。
void tick() {
    static int lastShown = -1;
    if (!g.active || g.infinite) { lastShown = -1; return; }
    int rem = remainingSeconds();
    if (rem <= 0) { stopAll(); lastShown = -1; return; }
    if (rem != lastShown) { lastShown = rem; refresh(); }
}

}  // namespace

int main() {
#ifdef _WIN32
    {
        char exe[512] = {0};
        GetModuleFileNameA(nullptr, exe, sizeof(exe));   // 图标内嵌在 exe 资源里，运行时从自身提取
        g_iconIdle  = std::string(exe) + ",0";           // 蓝
        g_iconAwake = std::string(exe) + ",1";           // 橙
        g_tray.icon = const_cast<char*>(g_iconIdle.c_str());
    }
#else
    g_tray.icon = (char*)"keep-awake";  // mac/linux：图标名（未实测）
#endif

    buildMenu();
    if (tray_init(&g_tray) != 0) return 1;
    g_inited = true;
    trimWorkingSet();  // 初始化加载了一堆 shell/COM 页，启动后归还一次

    while (g_running) {
        if (tray_loop(0) < 0) break;  // 收到 WM_QUIT
        tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    g.inhibitor.stop();
    return 0;
}
