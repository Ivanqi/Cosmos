/**********************************************************
	转换显示字符串头文件vgastr.h
***********************************************************/

#ifndef _VGASTR_T_H
#define _VGASTR_T_H

// 0xb8000 ~  0xbffff 用于文本模式显示适配器
#define VGASTR_RAM_BASE (0xb8000)    
#define VGASTR_RAM_END  (0xbffff)

#define VGADP_DFVL 0x0700           // 1792
#define VGADP_HLVL 0x0f00           // 3840

/**
 * 显卡内的寄存器很多，为了不过多占用主机的I/O空间，很多寄存器只能通过索引寄存器访问 
 * 其端口号是0x3d4，可以向它写入一个值来指定内部的某个寄存器。当指定之后，可以通过0x3d5端口来进行读写操12
 */
#define VGACTRL_REG_ADR 0x3d4
#define VGACTRL_REG_DAT 0x3d5
#define VGACURS_REG_INX 0x0a        // 换行分配符

#define VGACURS_CLOSE 0x20          // 32

#define VGACHAR_LR_CFLG 10
#define VGACHAR_DF_CFLG 0

#define VGASADH_REG_INX 0x0c        // 换页
#define VGASADL_REG_INX 0x0d        // 回车分配符

// 光标
typedef struct s_CURSOR
{
    uint_t vmem_s;
    uint_t vmem_e;
    uint_t cvmemadr;
	
    uint_t x;
    uint_t y;
} cursor_t;

#endif
