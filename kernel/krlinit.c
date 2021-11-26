/****************************************************************
    Cosmos kernel全局初始化文件krlinit.c
*****************************************************************/
#include "cosmostypes.h"
#include "cosmosmctrl.h"
void init_krl()
{
    // 初始化内核功能层的内存管理
    init_krlmm();
	// init_krldevice();
    // init_krldriver();
    // 初始化进程调度器
	init_krlsched();
    // init_ktime();
    // init_thread();
    // init_task();
    // 初始化空转进程
    init_krlcpuidle();

    // hal_enable_irqfiq();
    // 防止init_krl函数返回
    die(0);
    return;
}
