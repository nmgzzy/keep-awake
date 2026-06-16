set_project("keep-awake")
set_languages("c++20")  -- tray.h 用到 C++20 指定初始化器
add_rules("mode.debug", "mode.release")

-- 主程序：常驻系统托盘的防休眠工具
target("keep-awake")
    set_kind("binary")
    add_includedirs("third_party", "src")
    add_files("src/main.cpp", "src/duration.cpp")

    if is_plat("windows") then
        add_files("src/inhibitor_win.cpp", "src/input_dialog_win.cpp")
        add_files("src/app.rc")                       -- 内嵌图标
        add_cxflags("/utf-8")                         -- 源码与执行字符集均按 UTF-8
        add_syslinks("user32", "shell32", "kernel32", "gdi32", "advapi32")
        -- GUI 子系统：不弹控制台窗口，但保留 int main 入口
        add_ldflags("/subsystem:windows", "/entry:mainCRTStartup", {force = true})
    elseif is_plat("macosx") then
        add_files("src/inhibitor_mac.mm", "src/input_dialog_mac.mm")
        add_frameworks("Cocoa", "IOKit", "CoreFoundation")
    else
        add_files("src/inhibitor_linux.cpp", "src/input_dialog_linux.cpp")
        -- Linux 托盘依赖 GTK3 + appindicator（需系统已安装）
        add_cxflags("$(shell pkg-config --cflags gtk+-3.0 appindicator3-0.1)", {force = true})
        add_ldflags("$(shell pkg-config --libs gtk+-3.0 appindicator3-0.1)", {force = true})
    end

-- 单元测试：时长解析/格式化纯函数（控制台程序）
target("duration_test")
    set_kind("binary")
    set_default(false)
    set_group("tests")
    add_includedirs("src")
    add_files("src/duration.cpp", "tests/duration_test.cpp")
    if is_plat("windows") then
        add_cxflags("/utf-8")
    end

-- 防休眠核心的端到端自测（控制台程序，需手动配合 powercfg /requests 验证）
target("inhibit_check")
    set_kind("binary")
    set_default(false)
    set_group("tests")
    add_includedirs("src")
    add_files("tests/inhibit_check.cpp")
    if is_plat("windows") then
        add_files("src/inhibitor_win.cpp")
        add_cxflags("/utf-8")
        add_syslinks("kernel32")
    elseif is_plat("macosx") then
        add_files("src/inhibitor_mac.mm")
        add_frameworks("IOKit", "CoreFoundation")
    else
        add_files("src/inhibitor_linux.cpp")
    end
