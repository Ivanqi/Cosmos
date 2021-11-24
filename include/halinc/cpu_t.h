/**********************************************************
     CPU相关的宏定义文件cpu_t.h
***********************************************************/
#ifndef _CPU_T_H
#define _CPU_T_H

#ifdef CFG_X86_PLATFORM

typedef struct s_INTSTKREGS {
	uint_t r_gs;
	uint_t r_fs;
	uint_t r_es;
	uint_t r_ds;        // 段寄存器
	uint_t r_r15;
	uint_t r_r14;
	uint_t r_r13;
	uint_t r_r12;
	uint_t r_r11;
	uint_t r_r10;
	uint_t r_r9;
	uint_t r_r8;
	uint_t r_rdi;
	uint_t r_rsi;
	uint_t r_rbp;
	uint_t r_rdx;       // 通用寄存器
	uint_t r_rcx;
	uint_t r_rbx;
	uint_t r_rax;
	uint_t r_rip_old;   // 程序的指针寄存器
	uint_t r_cs_old;    // 代码段寄存器
	uint_t r_rflgs;     // rflags 标志寄存器
	uint_t r_rsp_old;   // 栈指针寄存器
	uint_t r_ss_old;    // 栈段寄存器
} intstkregs_t;


#endif


#endif // CPU_T_H
