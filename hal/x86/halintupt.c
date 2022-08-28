/**********************************************************
    hal层中断处理头文件halintupt.c
***********************************************************/

#include "cosmostypes.h"
#include "cosmosmctrl.h"

void dump_stack(void* stk)
{
    faultstkregs_t* f = (faultstkregs_t*)stk;
    kprint("程序指令寄存器%x:\n",f->r_rip_old);
    kprint("程序标志寄存器%x:\n",f->r_rflgs);
    kprint("程序栈寄存器%x:\n",f->r_rsp_old);
    kprint("程序缺页地址%x:\n",read_cr2());
    kprint("程序段寄存器CS:%x SS:%x DS:%x ES:%x FS:%x GS:%x\n",
    f->r_cs_old, f->r_ss_old, f->r_ds, f->r_es, f->r_fs, f->r_gs);
    return;
}

void intfltdsc_t_init(intfltdsc_t *initp, u32_t flg, u32_t sts, uint_t prity, uint_t irq)
{
    hal_spinlock_init(&initp->i_lock);
    initp->i_flg = flg;
    initp->i_stus = sts;
    initp->i_prity = prity;
    initp->i_irqnr = irq;
    initp->i_deep = 0;
    initp->i_indx = 0;

    list_init(&initp->i_serlist);
    initp->i_sernr = 0;

    list_init(&initp->i_serthrdlst);
    initp->i_serthrdnr = 0;
    initp->i_onethread = NULL;
    initp->i_rbtreeroot = NULL;

    list_init(&initp->i_serfisrlst);
    initp->i_serfisrnr = 0;
    initp->i_msgmpool = NULL;
    initp->i_privp = NULL;
    initp->i_extp = NULL;

    return;
}

// 初始化中断异常表machintflt，拷贝了中断相关信息
void init_intfltdsc()
{
    for (uint_t i = 0; i < IDTMAX; i++) {
        intfltdsc_t_init(&machintflt[i], 0, 0, i, i);
    }

    return;
}

// 中断初始化
PUBLIC void init_halintupt()
{
    init_descriptor();
    init_idt_descriptor();
    
    init_intfltdsc();

    init_i8259();
    // i8259_enabled_line(0);
    kprint("中断初始化成功\n");
    return;
}

/**
 * @brief 按中断码返回中断描述符
 * 
 * @param irqnr 
 * @return PUBLIC* 
 */
PUBLIC intfltdsc_t *hal_retn_intfltdsc(uint_t irqnr)
{
    if (irqnr > IDTMAX) {
        return NULL;
    }
    return &machintflt[irqnr];
}

/**
 * @brief 初始化中断
 *  1. 初始化intserdsc_t结构体实例变量，并把设备指针和回调函数放入其中
 *  2. 如果内核或者设备驱动程序要安装一个中断处理函数，就要先申请一个 intserdsc_t 结构体
 *  3. 然后把中断函数的地址写入其中，最后把这个结构挂载到对应的 intfltdsc_t 结构中的 i_serlist 链表中
 * 
 * @param initp 
 * @param flg 
 * @param intfltp 
 * @param device 
 * @param handle 
 */
void intserdsc_t_init(intserdsc_t *initp, u32_t flg, intfltdsc_t *intfltp, void *device, intflthandle_t handle)
{

    list_init(&initp->s_list);
    list_init(&initp->s_indevlst);

    initp->s_flg = flg;
    initp->s_intfltp = intfltp;
    initp->s_indx = 0;
    initp->s_device = device;
    initp->s_handle = handle;

    return;
}

/**
 * @brief 把intserdsc_t结构体实例变量挂载到中断异常描述符结构中
 * 
 * @param intdscp 
 * @param serdscp 
 * @return bool_t 
 */
bool_t hal_add_ihandle(intfltdsc_t *intdscp, intserdsc_t *serdscp)
{
    if (intdscp == NULL || serdscp == NULL) {
        return FALSE;
    }

    cpuflg_t cpuflg;
    hal_spinlock_saveflg_cli(&intdscp->i_lock, &cpuflg);
    list_add(&serdscp->s_list, &intdscp->i_serlist);

    intdscp->i_sernr++;
    hal_spinunlock_restflg_sti(&intdscp->i_lock, &cpuflg);

    return TRUE;
}

/**
 * @brief 开启中断请求
 * 
 * @param ifdnr 
 * @return drvstus_t 
 */
drvstus_t hal_enable_intline(uint_t ifdnr)
{
    if (20 > ifdnr || 36 < ifdnr) {
        return DFCERRSTUS;
    }

    i8259_enabled_line((u32_t)(ifdnr - 20));
    return DFCOKSTUS;
}

drvstus_t hal_disable_intline(uint_t ifdnr)
{

    if (20 > ifdnr || 36 < ifdnr) {
        return DFCERRSTUS;
    }

    i8259_disable_line((u32_t)(ifdnr - 20));
    return DFCOKSTUS;
}

drvstus_t hal_intflt_default(uint_t ift_nr, void *sframe)
{
    if (ift_nr == 0xffffffff || sframe == NULL) {
        return DFCERRSTUS;
    }

    return DFCOKSTUS;
}

/**
 * @brief 负责调用中断处理的回调函数
 *  1. 先获取中断异常表machintflt
 *  2. 然后调用i_serlist 链表上所有挂载intserdsc_t 结构中的中断处理的回调函数，是否处理由函数自己判断
 * 
 * @param ifdnr 中断码
 * @param sframe 
 */
void hal_run_intflthandle(uint_t ifdnr, void *sframe)
{
    intserdsc_t *isdscp;
    list_h_t *lst;
    // 根据中断号获取中断异常描述符地址
    intfltdsc_t *ifdscp = hal_retn_intfltdsc(ifdnr);

    if (ifdscp == NULL) {
        hal_sysdie("hal_run_intfdsc err");
        return;
    }
    
    // 遍历i_serlist链表
    list_for_each(lst, &ifdscp->i_serlist) {
        // 获取i_serlist链表上对象即intserdsc_t结构
        isdscp = list_entry(lst, intserdsc_t, s_list);
        // 调用中断处理回调函数
        isdscp->s_handle(ifdnr, isdscp->s_device, sframe);
    }

    return;
}

void hal_hwint_eoi()
{
    i8259_send_eoi();
    return;
}

/**
 * @brief 中断处理函数
 *  1. 加锁
 *  2. 调用中断回调函数hal_run_intflthandle
 *  3. 释放锁
 * 
 * @param intnumb 中断码
 * @param krnlsframp 
 */
void hal_do_hwint(uint_t intnumb, void *krnlsframp)
{
    intfltdsc_t *ifdscp = NULL;
    cpuflg_t cpuflg;

    // 根据中断号获取中断异常描述符地址
    if (intnumb > IDTMAX || krnlsframp == NULL) {
        hal_sysdie("hal_do_hwint fail\n");
        return;
    }

    ifdscp = hal_retn_intfltdsc(intnumb);
    if (ifdscp == NULL) {
        hal_sysdie("hal_do_hwint ifdscp NULL\n");
        return;
    }

    // 对段异常描述符加锁并中断
    hal_spinlock_saveflg_cli(&ifdscp->i_lock, &cpuflg);
    ifdscp->i_indx++;
    ifdscp->i_deep++;

    // 运行中断处理的回调函数
    hal_run_intflthandle(intnumb, krnlsframp);
    ifdscp->i_deep--;

    // 解锁并恢复中断状态
    hal_spinunlock_restflg_sti(&ifdscp->i_lock, &cpuflg);

    return;
}

/**
 * @brief 异常分发器
 *  1. 缺页异常(异常号14)
 *      1. 缺页异常是从 kernel.asm 文件中的 exc_page_fault 标号处开始，但它只是保存了 CPU 的上下文
 *      2. 然后调用了内核的通用异常分发器函数，最后由异常分发器函数调用不同的异常处理函数
 * 
 * @param faultnumb 异常码
 * @param krnlsframp 
 */
void hal_fault_allocator(uint_t faultnumb, void *krnlsframp)
{
    adr_t fairvadrs;
    cpuflg_t cpuflg;
    hal_cpuflag_sti(&cpuflg);

    // 如果异常号等于14则是内存缺页异常
    if (faultnumb == 14) {
        // 获取缺页的地址。打印缺页地址，这地址保存在CPU的CR2寄存器中
        fairvadrs = (adr_t)read_cr2();
        sint_t ret = krluserspace_accessfailed(fairvadrs);
        if (ret != 0) {
            dump_stack(krnlsframp);
            // 处理缺页失败就死机
            kprint("处理缺页失败状态码:%q, 缺页异常地址:%x\n", ret, fairvadrs);
            system_error("缺页处理失败\n");
        }

        hal_cpuflag_cli(&cpuflg);
        // 成功就返回
        return;
    }

    kprint("当前进程:%s,犯了不该犯的错误:%d,所以要杀\n", krlsched_retn_currthread()->td_appfilenm, faultnumb);
    dump_stack(krnlsframp);
    krlsve_exit_thread();
    hal_cpuflag_cli(&cpuflg);
    return;
}

// 中断分发器
sysstus_t hal_syscl_allocator(uint_t inr, void* krnlsframp)
{
	cpuflg_t cpuflg;
    sysstus_t ret;
    hal_cpuflag_sti(&cpuflg);
	ret = krlservice(inr, krnlsframp);
    hal_cpuflag_cli(&cpuflg);
    return ret;
}


/**
 * 有硬件中断时，会先到达中断处理入口，然后调用到硬件中断分发器函数hal_hwint_allocator
 *  第一个参数为中断编号，在rdi
 *  第二个参数为中断发生时的栈指针，在rsi
 *  然后调用异常处理函数hal_do_hwint
 */
void hal_hwint_allocator(uint_t intnumb, void *krnlsframp)
{
    cpuflg_t cpuflg;
    hal_cpuflag_sti(&cpuflg);

    hal_hwint_eoi();
    hal_do_hwint(intnumb, krnlsframp);
    krlsched_chkneed_pmptsched();

    hal_cpuflag_cli(&cpuflg);
    //kprint("暂时无法向任何服务进程发送中断消息,直接丢弃......\n");
    return;
}
