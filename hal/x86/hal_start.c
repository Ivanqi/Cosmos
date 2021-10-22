/**********************************************************
    开始入口文件hal_start.c
***********************************************************/

#include "cosmostypes.h"
#include "cosmosmctrl.h"

void hal_start() {
    // 初始化Cosmos的hal层
    init_hal();
    // 初始化Cosmos的内核层
    init_krl();
    return;
}