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
	// init_krlsched();
    // init_ktime();
    // init_thread();
    // init_task();
    // init_krlcpuidle();

    // hal_enable_irqfiq();
   
    die(0);
    return;
}
