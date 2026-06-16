# keep-awake 防休眠小工具 — 设计文档

- 日期：2026-06-16
- 状态：已确认，待编写实现计划

## 1. 目标

一个常驻系统托盘的小工具，让用户在自定义的一段时间内阻止系统进入睡眠/休眠。要求：

1. 可自定义时长，并提供若干快捷选项。
2. 轻量、跨平台，编译成单个原生可执行文件，无需安装额外运行时依赖。

**平台优先级**：Windows 为首要打磨与验证目标；macOS / Linux 代码同时实现，但不保证开发者本机能直接验证。

## 2. 技术选型

- **语言 / 构建**：C++ + xmake（开发者本机已具备编译器与 xmake，零环境成本）。
- **托盘库**：[`zserge/tray`](https://github.com/zserge/tray)，单头文件、极简、跨平台。
  - Windows：纯 Win32 `Shell_NotifyIcon`，零第三方依赖。
  - macOS：Cocoa（需 Objective-C++ 编译）。
  - Linux：libappindicator / GTK（Linux 系统托盘的固有依赖）。
- **防休眠**：各平台原生 API，无第三方依赖。
  - Windows：`SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED [| ES_DISPLAY_REQUIRED])`。
  - macOS：`IOPMAssertionCreateWithName`（IOKit），类型 `kIOPMAssertionTypeNoIdleSleep`。
  - Linux：`systemd-inhibit`（作为子进程持有 inhibitor lock，停止时杀掉该进程释放）。

### 是否阻止屏幕关闭

默认仅阻止系统睡眠（`ES_SYSTEM_REQUIRED`），不强制保持屏幕常亮。后续若需要可加一个菜单开关切换 `ES_DISPLAY_REQUIRED` / `kIOPMAssertionTypePreventUserIdleDisplaySleep`。本期不做（YAGNI）。

## 3. 代码结构

```
keep-awake/
  xmake.lua                 // 按平台条件编译、链接系统库
  third_party/
    tray.h                  // zserge/tray 单头文件
  src/
    main.cpp                // 托盘菜单、状态机、主循环、tick
    duration.h / duration.cpp   // parseDuration / formatRemaining 纯函数（可单测）
    inhibitor.h             // 统一接口
    inhibitor_win.cpp       // SetThreadExecutionState
    inhibitor_mac.mm        // IOPMAssertion
    inhibitor_linux.cpp     // systemd-inhibit 子进程
    input_dialog.h          // 自定义时长输入框统一接口
    input_dialog_win.cpp    // Win32 模态输入对话框
    input_dialog_mac.mm     // osascript
    input_dialog_linux.cpp  // zenity（缺失则返回失败，菜单降级）
  tests/
    duration_test.cpp       // parseDuration / formatRemaining 单元测试
```

### 接口

```cpp
// inhibitor.h
struct Inhibitor {
    bool start(bool keepDisplayOn);  // 开始阻止休眠，幂等
    void stop();                     // 释放，幂等
    bool active() const;
};
// 每平台一份实现（条件编译）。

// input_dialog.h
// 弹出原生输入框，返回用户输入文本；取消或不支持返回 false。
bool promptCustomDuration(std::string& out);

// duration.h
// 解析 "90m" "1h30m" "45"(默认分钟) "2h" 等；非法返回 false。
bool parseDuration(const std::string& text, int& seconds);
// 把剩余秒数格式化为 "29:45" 或 "1:05:30"。
std::string formatRemaining(int seconds);
```

## 4. 托盘菜单与交互

弹出菜单：
```
● 防休眠中 · 剩余 29:45     // 状态行，disabled，实时刷新（空闲时显示「○ 未激活」）
─────────────────
  15 分钟
  30 分钟
  1 小时
  2 小时
  4 小时
  一直保持（直到退出）
  自定义时长…
─────────────────
  停止                       // 仅激活时 enabled
  退出
```

- 当前生效的时长项前显示选中标记（`checked`）。
- 点任一预设时长：立即 `inhibitor.start()` 并设定倒计时；到点自动 `stop()` 回到空闲。
- 运行期间点另一时长 = 重新计时；点「一直保持」= 无倒计时持续到退出。
- 「停止」：立即 `stop()`，回到空闲。
- 「退出」：`stop()` 后退出进程（确保释放系统锁）。
- 状态行每秒刷新；图标 tooltip 同步显示剩余时间。

### 自定义时长输入

托盘菜单无法输入文本，点「自定义时长…」弹出原生输入框：
- Windows（首要）：Win32 模态输入对话框，接受 `90m` / `1h30m` / `45`（纯数字默认分钟）等格式。
- macOS：`osascript` 输入框；Linux：`zenity` 输入框。
- 解析交给 `parseDuration`；非法输入提示并重新弹出或取消。
- Linux 上 `zenity` 缺失时输入框返回失败，菜单仅提供预设项（降级，不崩溃）。

## 5. 主循环与并发模型

单线程，全部在消息循环线程内，规避跨线程刷新托盘的坑：

```
init tray + 菜单
loop:
    tray_loop(0)            // 非阻塞处理菜单/图标事件
    每 ~100ms 一次 tick：
        若激活且有倒计时：
            remaining = endTime - now
            若 remaining <= 0: stop(); 回到空闲
            否则若显示的剩余秒数变化: 更新状态行 + tooltip + tray_update()
    sleep(短) 避免空转占 CPU
退出时: inhibitor.stop()
```

- 倒计时基于单调时钟（`std::chrono::steady_clock`）计算 endTime，不受系统时间调整影响。
- `tray_update()` 仅在显示文本变化时调用，避免无谓刷新。

## 6. 错误处理

- `inhibitor.start()` 失败（API 返回错误）：状态行提示失败，不进入激活态。
- 自定义输入非法 / 取消：不改变当前状态。
- 进程异常退出兜底：Windows 的 `ES_CONTINUOUS` 锁随进程结束自动释放；Linux 的 `systemd-inhibit` 子进程随父进程退出而终止释放。

## 7. 测试策略

- **单元测试**（`tests/duration_test.cpp`，xmake 跑）：
  - `parseDuration`：`"45"`→2700s、`"90m"`→5400s、`"1h30m"`→5400s、`"2h"`→7200s、空串/乱码→false、`"0"`→false。
  - `formatRemaining`：`45`→`"00:45"`、`1845`→`"30:45"`、`3930`→`"1:05:30"`。
- **手动验证（Windows）**：运行后选时长，命令行 `powercfg /requests` 应看到本进程的 SYSTEM 请求；停止/到点后请求消失。

## 8. 不做（YAGNI）

- 强制屏幕常亮开关（保留扩展位，本期不做）。
- 开机自启、配置文件持久化、自定义图标主题、多语言。
- 暂停/续期等复杂时长管理。

## 9. 交付

- `xmake build` 在 Windows 产出单个 `keep-awake.exe`，双击常驻托盘。
- README 说明用法、各平台依赖（Linux 需系统托盘 + zenity）、构建命令。
