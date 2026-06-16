#pragma once

// 尝试获取“单实例”锁。
// 返回 true：本进程是唯一实例（已持有锁，锁保持到进程退出，由 OS 自动释放）。
// 返回 false：已有实例在运行，调用方应直接退出。
bool acquireSingleInstance();
