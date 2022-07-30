/**********************************************************
    设置全局／中断描述符文件halgdtidt.c
***********************************************************/

#include "cosmostypes.h"
#include "cosmosmctrl.h"

/**
 * @brief 中断描述分设置
 * 
 * @param vector 向量也是中断号
 * @param desc_type 中断门类型，中断门，陷阱门
 * @param handler 中断处理程序的入口地址
 * @param privilege 中断门的权限级别
 */
void set_idt_desc(u8_t vector, u8_t desc_type, inthandler_t handler, u8_t privilege)
{
    gate_t *p_gate = &x64_idt[vector];  // 从idt中取出中断描述符(中断门)
    u64_t base = (u64_t)handler;

    // 中断处理程序所在段的段选择子和段内偏移地址
    p_gate->offset_low = base & 0xFFFF;
    p_gate->selector = SELECTOR_KERNEL_CS;

    p_gate->dcount = 0;
    // 属性
    p_gate->attr = (u8_t)(desc_type | (privilege << 5));

    p_gate->offset_high = (u16_t)((base >> 16) & 0xFFFF);
    p_gate->offset_high_h = (u32_t)((base >> 32) & 0xffffffff);
    p_gate->offset_resv = 0;
    return;
}

void set_igdtr(descriptor_t *gdtptr)
{
    return;
}

void set_iidtr(gate_t *idtptr)
{

    x64_iidt_reg.idtbass = (u64_t)idtptr;
    x64_iidt_reg.idtLen = sizeof(x64_idt) - 1;
    return;
}

/**
 * @brief 设置段描述符
 * 
 * @param p_desc 段描述符
 * @param base 段基址参数
 * @param limit 段界限参数
 * @param attribute 属性
 */
void set_descriptor(descriptor_t *p_desc, u32_t base, u32_t limit, u16_t attribute)
{
    p_desc->limit_low = limit & 0x0FFFF;                                                   // 段界限 1(2 字节)
    p_desc->base_low = base & 0x0FFFF;                                                     // 段基址 1(2 字节)
    p_desc->base_mid = (base >> 16) & 0x0FF;                                               // 段基址 2(1 字节)
    p_desc->attr1 = (u8_t)(attribute & 0xFF);                                              // 属性 1
    p_desc->limit_high_attr2 = (u8_t)(((limit >> 16) & 0x0F) | ((attribute >> 8) & 0xF0)); // 段界限 2 + 属性 2
    p_desc->base_high = (u8_t)((base >> 24) & 0x0FF);                                      // 段基址 3\(1 字节)
    return;
}

/**
 * @brief 设置任务状态段
 * 
 * @param p_desc 任务状态段
 * @param base 段基址参数
 * @param limit 段界限参数
 * @param attribute 属性
 */
void set_x64tss_descriptor(descriptor_t *p_desc, u64_t base, u32_t limit, u16_t attribute)
{
    u32_t *x64tssb_h = (u32_t *)(p_desc + 1);

    p_desc->limit_low = limit & 0x0FFFF;                                                   // 段界限 1(2 字节)
    p_desc->base_low = base & 0x0FFFF;                                                     // 段基址 1(2 字节)
    p_desc->base_mid = (base >> 16) & 0x0FF;                                               // 段基址 2(1 字节)
    p_desc->attr1 = (u8_t)(attribute & 0xFF);                                              // 属性 1
    p_desc->limit_high_attr2 = (u8_t)(((limit >> 16) & 0x0F) | ((attribute >> 8) & 0xF0)); // 段界限 2 + 属性 2
    p_desc->base_high = (u8_t)((base >> 24) & 0x0FF);

    *x64tssb_h = (u32_t)((base >> 32) & 0xffffffff);

    *(x64tssb_h + 1) = 0;
}

// 载入gdt
PUBLIC LKINIT void load_x64_gdt(igdtr_t *igdtrp)
{

    __asm__ __volatile__(

        "cli \n\t"
        "pushq %%rax \n\t"
        "lgdt (%0) \n\t"
        "movabsq $1f,%%rax \n\t"
        "pushq   $8 \n\t"
        "pushq   %%rax    \n\t"
        "lretq \n\t"
        "1:\n\t"
        "movw $0x10,%%ax\n\t"
        "movw %%ax,%%ds\n\t"
        "movw %%ax,%%es\n\t"
        "movw %%ax,%%ss\n\t"
        "movw %%ax,%%fs\n\t"
        "movw %%ax,%%gs\n\t"
        "popq %%rax \n\t"
        :
        : "r"(igdtrp)
        : "rax", "memory"
    );

    return;
}

PUBLIC LKINIT void load_x64_idt(iidtr_t *idtptr)
{
    __asm__ __volatile__(
        "lidt (%0) \n\t"
        :
        : "r"(idtptr)
        : "memory"
    );
    return;
}

PUBLIC LKINIT void load_x64_tr(u16_t trindx)
{
    __asm__ __volatile__(
        "ltr %0 \n\t"
        :
        : "r"(trindx)
        : "memory"
    );
}

/**
 * @brief 初始化段描述符和TSS描述符
 * 
 * @return void 
 */
PUBLIC LKINIT void init_descriptor()
{

    /**
     * 通过全局描述符(gdt)0~4索引，寻找对应的段描述符。然后设置段描述符的信息
     * 
     * 因为TSS也是存储在gdt中，所以索引下标为6的描述符，作为TSS
     */
    for (u32_t gdtindx = 0; gdtindx < CPUCORE_MAX; gdtindx++) {

        // GDT中的第0个段描述符是不可用的
        set_descriptor(&x64_gdt[gdtindx][0], 0, 0, 0); 

        // 第一个描述符的数据段(向下扩展的数据段)只读可执行
        set_descriptor(&x64_gdt[gdtindx][1], 0, 0, DA_CR | DA_64 | 0);

        // 第二个描述符的数据段(向下扩展的数据段)只读
        set_descriptor(&x64_gdt[gdtindx][2], 0, 0, DA_DRW | DA_64 | 0); 

        // 第三个描述符的数据段(向下扩展的数据段)只读可执行, 权限级别为3
        set_descriptor(&x64_gdt[gdtindx][3], 0, 0, DA_CR | DA_64 | DA_DPL3 | 0); 

        // 第三个描述符的数据段(向下扩展的数据段)只读, 权限级别为3
        set_descriptor(&x64_gdt[gdtindx][4], 0, 0, DA_DRW | DA_64 | DA_DPL3 | 0);

        // 当任务被创建时，此时尚未上CPU执行，因此，此时的B位为0，TYPE的值为1001
        set_x64tss_descriptor(&x64_gdt[gdtindx][6], (u64_t)&x64tss[gdtindx], sizeof(x64tss[gdtindx]) - 1, DA_386TSS);

        // 用一层 x64_igdt_reg 把gdt包裹起来
        x64_igdt_reg[gdtindx].gdtbass = (u64_t)x64_gdt[gdtindx];
        x64_igdt_reg[gdtindx].gdtLen = sizeof(x64_gdt[gdtindx]) - 1;
    }

    // 载入gdt
    load_x64_gdt(&x64_igdt_reg[0]);
    // 载入TR寄存器，用于TSS
    load_x64_tr(0x30);

    return;
}

/**
 * @brief 初始化中断描述符
 *  1. 中断向量号是中断描述符的索引，当处理器收到一个外部中断向量后，它用此向量号在中断描述符表中查询对应的中断描述符，然后再去执行该中断中的中断处理程序
 *  2. 由于中断描述符是8个字节(64位)，所以处理器用中断向量号乘以8后，再与IDTR中的中断描述符表地址相加，所求的地址之和便是该中断向量号对应的中断描述符
 * 
 * @return void
 */
PUBLIC LKINIT void init_idt_descriptor()
{   
    // 一开始把所有中断的处理程序设置为保留的通用处理程序
    for (u16_t intindx = 0; intindx <= 255; intindx++) {
        set_idt_desc((u8_t)intindx, DA_386IGate, hxi_exc_general_intpfault, PRIVILEGE_KRNL);
    }

    // 除法错误异常 比如除0
    set_idt_desc(INT_VECTOR_DIVIDE, DA_386IGate, exc_divide_error, PRIVILEGE_KRNL);

    // 单步执行异常
    set_idt_desc(INT_VECTOR_DEBUG, DA_386IGate, exc_single_step_exception, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_NMI, DA_386IGate, exc_nmi, PRIVILEGE_KRNL);

    // 调试断点异常
    set_idt_desc(INT_VECTOR_BREAKPOINT, DA_386IGate, exc_breakpoint_exception, PRIVILEGE_USER);

    // 溢出异常
    set_idt_desc(INT_VECTOR_OVERFLOW, DA_386IGate, exc_overflow, PRIVILEGE_USER);

    set_idt_desc(INT_VECTOR_BOUNDS, DA_386IGate, exc_bounds_check, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_INVAL_OP, DA_386IGate, exc_inval_opcode, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_COPROC_NOT, DA_386IGate, exc_copr_not_available, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_DOUBLE_FAULT, DA_386IGate, exc_double_fault, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_COPROC_SEG, DA_386IGate, exc_copr_seg_overrun, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_INVAL_TSS, DA_386IGate, exc_inval_tss, PRIVILEGE_KRNL);

    // 段不存在异常
    set_idt_desc(INT_VECTOR_SEG_NOT, DA_386IGate, exc_segment_not_present, PRIVILEGE_KRNL);

    // 栈异常
    set_idt_desc(INT_VECTOR_STACK_FAULT, DA_386IGate, exc_stack_exception, PRIVILEGE_KRNL);

    // 通用异常
    set_idt_desc(INT_VECTOR_PROTECTION, DA_386IGate, exc_general_protection, PRIVILEGE_KRNL);

    // 缺页异常
    set_idt_desc(INT_VECTOR_PAGE_FAULT, DA_386IGate, exc_page_fault, PRIVILEGE_KRNL);

    //set_idt_desc(15,DA_386IGate,hxi_exc_general_intpfault,PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_COPROC_ERR, DA_386IGate, exc_copr_error, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_ALIGN_CHEK, DA_386IGate, exc_alignment_check, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_MACHI_CHEK, DA_386IGate, exc_machine_check, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_SIMD_FAULT, DA_386IGate, exc_simd_fault, PRIVILEGE_KRNL);

    // 中断向量表，绑定中断编号32和hxi_hwint00，特权级别设置为PRIVILEGE_KRNL
    set_idt_desc(INT_VECTOR_IRQ0 + 0, DA_386IGate, hxi_hwint00, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ0 + 1, DA_386IGate, hxi_hwint01, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ0 + 2, DA_386IGate, hxi_hwint02, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ0 + 3, DA_386IGate, hxi_hwint03, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ0 + 4, DA_386IGate, hxi_hwint04, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ0 + 5, DA_386IGate, hxi_hwint05, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ0 + 6, DA_386IGate, hxi_hwint06, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ0 + 7, DA_386IGate, hxi_hwint07, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ8 + 0, DA_386IGate, hxi_hwint08, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ8 + 1, DA_386IGate, hxi_hwint09, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ8 + 2, DA_386IGate, hxi_hwint10, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ8 + 3, DA_386IGate, hxi_hwint11, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ8 + 4, DA_386IGate, hxi_hwint12, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ8 + 5, DA_386IGate, hxi_hwint13, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ8 + 6, DA_386IGate, hxi_hwint14, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ8 + 7, DA_386IGate, hxi_hwint15, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ8 + 8, DA_386IGate, hxi_hwint16, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ8 + 9, DA_386IGate, hxi_hwint17, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ8 + 10, DA_386IGate, hxi_hwint18, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ8 + 11, DA_386IGate, hxi_hwint19, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ8 + 12, DA_386IGate, hxi_hwint20, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ8 + 13, DA_386IGate, hxi_hwint21, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ8 + 14, DA_386IGate, hxi_hwint22, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_IRQ8 + 15, DA_386IGate, hxi_hwint23, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_APIC_IPI_SCHEDUL, DA_386IGate, hxi_apic_ipi_schedul, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_APIC_SVR, DA_386IGate, hxi_apic_svr, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_APIC_TIMER, DA_386IGate, hxi_apic_timer, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_APIC_THERMAL, DA_386IGate, hxi_apic_thermal, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_APIC_PERFORM, DA_386IGate, hxi_apic_performonitor, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_APIC_LINTO, DA_386IGate, hxi_apic_lint0, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_APIC_LINTI, DA_386IGate, hxi_apic_lint1, PRIVILEGE_KRNL);

    set_idt_desc(INT_VECTOR_APIC_ERROR, DA_386IGate, hxi_apic_error, PRIVILEGE_KRNL);

    // 中断向量表，绑定中断编号255和exi_sys_call，特权级别设置为PRIVILEGE_USER
    set_idt_desc(INT_VECTOR_SYSCALL, DA_386IGate, exi_sys_call, PRIVILEGE_USER);

    set_iidtr(x64_idt);
    load_x64_idt(&x64_iidt_reg);
    
    return;
}