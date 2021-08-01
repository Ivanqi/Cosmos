/**********************************************************
	转换显示字符串文件vgastr.c
***********************************************************/

#include "cmctl.h"
// __attribute__主要用于改变所声明或定义的函数或 数据的特性，它有很多子项，用于改变作用对象的特性

// 将 cursor_t curs 放到.data段中
__attribute__((section(".data"))) cursor_t curs;

// 初始化光标
void init_curs()
{
    // 内存范围
    curs.vmem_s = VGASTR_RAM_BASE;
    curs.vmem_e = VGASTR_RAM_END;

    curs.cvmemadr = 0;
    curs.x = 0;
    curs.y = 0;
}

// 清空屏幕
void clear_screen(u16_t srrv)
{
    curs.x = 0;
    curs.y = 0;

    u16_t *p = (u16_t *) VGASTR_RAM_BASE;

    for (uint_t i = 0; i < 2001; i++) {
        p[i] = srrv;
    }

    close_curs();
    return ;
}

// 关闭光标
void close_curs()
{
    out_u8(VGACTRL_REG_ADR, VGACURS_REG_INX);
    out_u8(VGACTRL_REG_DAT, VGACURS_CLOSE);
    return ;
}

