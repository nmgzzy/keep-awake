# keep-awake

一个常驻系统托盘的轻量防休眠小工具：在你设定的一段时间内阻止系统进入睡眠，到点自动恢复。
C++ 编写，编译为单个原生可执行文件，无需安装运行时依赖。

## 功能

- 托盘图标菜单，提供快捷时长：15 分钟 / 30 分钟 / 1 小时 / 2 小时 / 4 小时 / 一直保持。
- 「自定义时长…」弹原生输入框，支持 `90m`、`1h30m`、`45`（纯数字按分钟）等格式。
- 状态行实时显示剩余时间，到点自动停止；可随时切换时长或手动「停止」。
- 托盘图标按状态变色：**防休眠中=橙色，空闲=蓝色**（两枚图标均内嵌于 exe）。
- 「开机自启」开关（Windows：写 `HKCU\...\Run`；mac/Linux 暂未实现，菜单项置灰）。
- 进程退出自动释放系统锁，不会残留。

Windows 版静态链接 CRT，**仅依赖系统 DLL**（USER32/SHELL32/KERNEL32/GDI32/ADVAPI32），
无需安装 VC++ 运行库；Release 体积约 170 KB。

## 平台支持

| 平台 | 防休眠实现 | 状态 |
|------|------------|------|
| Windows | `SetThreadExecutionState` | ✅ 已实现并验证（首要目标） |
| macOS | `IOPMAssertionCreateWithName`（IOKit） | ⚠️ 已实现，未实测 |
| Linux | `systemd-inhibit` 子进程 | ⚠️ 已实现，未实测 |

- 自定义输入框：Windows 用原生 Win32 对话框；macOS 用 `osascript`；Linux 用 `zenity`（未装则菜单仅保留预设项）。
- Linux 托盘依赖系统托盘（GTK3 + libappindicator）。

## 构建

需要 [xmake](https://xmake.io) 和 C++ 编译器（Windows 用 MSVC，C++20）。

```bash
# 生成内嵌图标（首次或修改图标后）
python tools/generate_icon.py

# 编译主程序（Release）
xmake f -m release -y
xmake build keep-awake

# 运行（产物在 build/<plat>/<arch>/release/ 下）
xmake run keep-awake
```

### 测试

```bash
xmake build duration_test   && xmake run duration_test     # 时长解析/格式化单元测试
xmake build inhibit_check    && xmake run inhibit_check     # 防休眠核心：保持 8 秒后释放
xmake build autostart_check  && xmake run autostart_check   # 开机自启：启用/关闭，结束后恢复原状
```

## 在 Windows 上验证防休眠确实生效

`inhibit_check` 运行期间，在**管理员**命令提示符执行：

```
powercfg /requests
```

`SYSTEM` 段应出现本进程的请求；停止/到点后请求消失。
（`powercfg /requests` 需要管理员权限。）

## 项目结构

```
src/
  main.cpp            # 托盘菜单、状态机、主循环
  duration.*          # 时长解析/格式化（纯函数，可单测）
  inhibitor.*         # 防休眠统一接口 + 各平台实现
  input_dialog.*      # 自定义时长输入框 + 各平台实现
  app.rc              # Windows 内嵌两态图标（icon_idle/icon_awake.ico）
third_party/tray.h    # zserge/tray 单头文件托盘库（含一处本地修正）
tools/generate_icon.py
tests/                # 单元测试与防休眠自测
```

## 备注

- 默认仅阻止系统睡眠，不强制屏幕常亮。
- `third_party/tray.h` 对非阻塞 `tray_loop` 做了一处修正（原版在无消息时会读取未初始化的消息结构）。
