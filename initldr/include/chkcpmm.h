/**********************************************************
	系统全局内存检查头文件chkcpmm.h
***********************************************************/

#ifndef _CHKCPMM_H
#define _CHKCPMM_H

void init_mem(machbstart_t* mbsp);

void init_krlinitstack(machbstart_t* mbsp);

void init_meme820(machbstart_t* mbsp);

void init_chkcpu(machbstart_t* mbsp);

void mmap(e820map_t** retemp,u32_t* retemnr);

int chk_cpuid();

int chk_cpu_longmode();

e820map_t* chk_memsize(e820map_t* e8p,u32_t enr,u64_t sadr,u64_t size);

u64_t get_memsize(e820map_t* e8p,u32_t enr);

void init_chkmm();

void out_char(char* c);

void init_bstartpages(machbstart_t *mbsp);

void ldr_createpage_and_open();

/**
 * HLT: 处理器暂停,  直到出现中断或复位信号才继续.
 * 	HLT 使 CPU 进入这么一个状态：既不取指令，也不读写数据，总线上“静悄悄”的
 * 	这条指令用的地方不多，一般用于等外部中断
 * 
 * CLI: CLI汇编指令全称为Clear Interupt，该指令的作用是禁止中断发生
 * 	在CLI起效之后，所有外部中断都被屏蔽，这样可以保证当前运行的代码不被打断，起到保护代码运行的作用
 * 
 * 如果hlt指令之前，做了cli，那可屏蔽中断不能唤醒cpu
 */
#define CLI_HALT() __asm__ __volatile__("cli; hlt": : :"memory")

#endif
