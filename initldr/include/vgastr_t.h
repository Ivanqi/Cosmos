/**********************************************************
	转换显示字符串头文件vgastr.h
***********************************************************/

#ifndef _VGASTR_T_H
#define _VGASTR_T_H

#define VGASTR_RAM_BASE (0xb8000)   // 753664
#define VGASTR_RAM_END  (0xbffff)   // 786431

#define VGADP_DFVL 0x0700           // 1792
#define VGADP_HLVL 0x0f00           // 3840

#define VGACTRL_REG_ADR 0x3d4       // 980
#define VGACTRL_REG_DAT 0x3d5       // 981
#define VGACURS_REG_INX 0x0a        // 10

#define VGACURS_CLOSE 0x20          // 32

#define VGACHAR_LR_CFLG 10
#define VGACHAR_DF_CFLG 0

#define VGASADH_REG_INX 0x0c        // 12
#define VGASADL_REG_INX 0x0d        // 13

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
