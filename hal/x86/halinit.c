/****************************************************************
    Cosmos HAL全局初始化文件halinit.c
*****************************************************************/

#include "cosmostypes.h"
#include "cosmosmctrl.h"

void init_hal()
{
    // 初始化平台
    init_halplaltform();
    move_img2maxpadr(&kmachbsp);
    // 初始化内存
    init_halmm();
    // 初始化中断
    init_halintupt();
    return;
}