/****************************************************************
    Cosmos HAL全局数据结构头文件halglobal.h
*****************************************************************/

#ifndef _HALGLOBAL_H
#define _HALGLOBAL_H
#ifdef	HALGOBAL_HEAD
#undef	EXTERN
#define EXTERN
#endif

#ifdef CFG_X86_PLATFORM
// 除法错误异常 比如除0
void exc_divide_error();
// 单步执行异常
void exc_single_step_exception();
void exc_nmi();
// 调试断点异常
void exc_breakpoint_exception();
// 溢出异常
void exc_overflow();
// 段不存在异常
void exc_bounds_check();
void exc_inval_opcode();
void exc_copr_not_available();
void exc_double_fault();
void exc_copr_seg_overrun();
void exc_inval_tss();
void exc_segment_not_present();
// 栈异常
void exc_stack_exception();
// 通用异常
void exc_general_protection();
// 缺页异常
void exc_page_fault();
void exc_copr_error();
void exc_alignment_check();
void exc_machine_check();
void exc_simd_fault();
void hxi_exc_general_intpfault();
void hxi_hwint00();
void hxi_hwint01();
void hxi_hwint02();
void hxi_hwint03();
void hxi_hwint04();
void hxi_hwint05();
void hxi_hwint06();
void hxi_hwint07();
void hxi_hwint08();
void hxi_hwint09();
void hxi_hwint10();
void hxi_hwint11();
void hxi_hwint12();
void hxi_hwint13();
void hxi_hwint14();
void hxi_hwint15();
void hxi_hwint16();
void hxi_hwint17();
void hxi_hwint18();
void hxi_hwint19();
void hxi_hwint20();
void hxi_hwint21();
void hxi_hwint22();
void hxi_hwint23();
void hxi_apic_svr();
void hxi_apic_ipi_schedul();
void hxi_apic_timer();
void hxi_apic_thermal();
void hxi_apic_performonitor();
void hxi_apic_lint0();
void hxi_apic_lint1();
void hxi_apic_error();
void exi_sys_call();
void asm_ret_from_user_mode();

// 全局变量
HAL_DEFGLOB_VARIABLE(descriptor_t, x64_gdt)[CPUCORE_MAX][GDTMAX];

// 定义中断表，中断表其实是gate_t结构的数组，由CPU的IDTR寄存器指向,IDTMAX为256
// 在 x86 CPU 上，最多支持 256 个中断
HAL_DEFGLOB_VARIABLE(gate_t, x64_idt)[IDTMAX];

// 每个CPU核心一个tss
HAL_DEFGLOB_VARIABLE(x64tss_t, x64tss)[CPUCORE_MAX]; 

// 任务状态段
HAL_DEFGLOB_VARIABLE(x64tss_t, x64tss)[CPUCORE_MAX]; 

HAL_DEFGLOB_VARIABLE(igdtr_t, x64_igdt_reg)[CPUCORE_MAX];
HAL_DEFGLOB_VARIABLE(iidtr_t, x64_iidt_reg);

// 全局二级引导器结构体
HAL_DEFGLOB_VARIABLE(machbstart_t, kmachbsp);
// 全局图像结构体
HAL_DEFGLOB_VARIABLE(dftgraph_t, kdftgh);

// 小内存管理
HAL_DEFGLOB_VARIABLE(memmgrob_t, memmgrob);

// 定义intfltdsc_t结构数组大小为256
HAL_DEFGLOB_VARIABLE(intfltdsc_t, machintflt)[IDTMAX];

#endif
void die(u32_t dt);

#endif