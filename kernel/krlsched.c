/**********************************************************
    线程调度文件krlsched.c
***********************************************************/
#include "cosmostypes.h"
#include "cosmosmctrl.h"

void thrdlst_t_init(thrdlst_t *initp)
{
    list_init(&initp->tdl_lsth);    // 初始化挂载进程的链表
    initp->tdl_curruntd = NULL;     // 该链表上正在运行的进程。开始没有运行进程
    initp->tdl_nr = 0;              // 该链表上进程个数。开始没有进程
    return;
}

void schdata_t_init(schdata_t *initp)
{
    krlspinlock_init(&initp->sda_lock);
    initp->sda_cpuid = hal_retn_cpuid();    // 获取CPU id
    initp->sda_schdflgs = NOTS_SCHED_FLGS;
    initp->sda_premptidx = 0;               // 进程抢占计数
    initp->sda_threadnr = 0;                // 进程数
    initp->sda_prityidx = 0;                // 当前优先级
    initp->sda_cpuidle = NULL;              // 当前CPU的空转进程。开始没有空转进程和运行进程
    initp->sda_currtd = NULL;               // 当前正在运行的进程

    // 初始化schdata_t结构中的每个thrdlst_t结构
    for (uint_t ti = 0; ti < PRITY_MAX; ti++) {
        thrdlst_t_init(&initp->sda_thdlst[ti]);
    }

    list_init(&initp->sda_exitlist);
    return;
}

void schedclass_t_init(schedclass_t *initp)
{
    krlspinlock_init(&initp->scls_lock);
    initp->scls_cpunr = CPUCORE_MAX;        // CPU最大个数
    initp->scls_threadnr = 0;               // 系统中所有的进程数，开始没有进程
    initp->scls_threadid_inc = 0;           // 分配进程id所用

    // 初始化osschedcls变量中的每个schdata_t。 每个CPU调度数据结构
    for (uint_t si = 0; si < CPUCORE_MAX; si++) {
        schdata_t_init(&initp->scls_schda[si]);
    }

    return;
}

/**
 * @brief init_krlsched 函数调用 schedclass_t_init 函数
 *  对 osschedcls 变量进行初始化工作
 * 
 */
void init_krlsched()
{
    // 初始化osschedcls变量
    schedclass_t_init(&osschedcls);
    kprint("进程调度器初始化成功\n");
    return;
}

/**
 * @brief 获取当前运行的进程
 *  1. 获取当前正在运行的进程，目的是为了保存当前进程的运行上下文，确保在下一次调度到当前运行的进程时能够恢复运行
 * 
 * @return thread_t* 返回当前运行的进程
 */
thread_t *krlsched_retn_currthread()
{
    uint_t cpuid = hal_retn_cpuid();
    // 通过cpuid获取当前cpu的调度数据结构
    schdata_t *schdap = &osschedcls.scls_schda[cpuid];
    // 若调度数据结构中当前运行进程的指针为空，就出错死机
    if (schdap->sda_currtd == NULL) {
        hal_sysdie("schdap->sda_currtd NULL");
    }

    // 返回当前运行的进程
    return schdap->sda_currtd;
}

uint_t krlsched_retn_schedflgs()
{
    uint_t cpuid = hal_retn_cpuid();
    schdata_t *schdap = &osschedcls.scls_schda[cpuid];
    return schdap->sda_schdflgs;
}

/**
 * @brief 进程等待
 *  1. 这个函数会设置进程状态为等待状态，让进程从调度系统数据结构中脱离，最后让进程加入到 kwlst_t 等待结构中
 * 
 * @param wlst 
 */
void krlsched_wait(kwlst_t *wlst)
{
    cpuflg_t cufg, tcufg;
    uint_t cpuid = hal_retn_cpuid();
    schdata_t *schdap = &osschedcls.scls_schda[cpuid];

    // 获取当前正在运行的进程
    thread_t *tdp = krlsched_retn_currthread();
    // 获取进程优先级
    uint_t pity = tdp->td_priority;

    if (pity >= PRITY_MAX || wlst == NULL) {
        goto err_step;
    }

    if (schdap->sda_thdlst[pity].tdl_nr < 1) {
        goto err_step;
    }

    krlspinlock_cli(&schdap->sda_lock, &cufg);

    krlspinlock_cli(&tdp->td_lock, &tcufg);
    tdp->td_stus = TDSTUS_WAIT; // 设置进程状态为等待状态
    list_del(&tdp->td_list);    // 脱链
    krlspinunlock_sti(&tdp->td_lock, &tcufg);

    if (schdap->sda_thdlst[pity].tdl_curruntd == tdp) {
        schdap->sda_thdlst[pity].tdl_curruntd = NULL;
    }

    schdap->sda_thdlst[pity].tdl_nr--;

    krlspinunlock_sti(&schdap->sda_lock, &cufg);
    krlwlst_add_thread(wlst, tdp);  // 将进程加入等待结构中

    return;

err_step:
    hal_sysdie("krlsched_wait err");
    return;
}

/**
 * @brief 进程唤醒
 *  1. 进程的唤醒则是进程等待的反向操作行为
 *  2. 即从等待数据结构中获取进程，然后设置进程的状态为运行状态，最后将这个进程加入到进程调度系统数据结构中
 * 
 * @param wlst 
 */
void krlsched_up(kwlst_t *wlst)
{
    cpuflg_t cufg, tcufg;
    uint_t cpuid = hal_retn_cpuid();
    schdata_t *schdap = &osschedcls.scls_schda[cpuid];
    thread_t *tdp;
    uint_t pity;

    if (wlst == NULL) {
        goto err_step;
    }

    // 取出等待数据结构第一个进程并从等待数据结构中删除
    tdp = krlwlst_del_thread(wlst);
    if (tdp == NULL) {
        goto err_step;
    }

    pity = tdp->td_priority;    // 获取进程的优先级
    if (pity >= PRITY_MAX) {
        goto err_step;
    }

    krlspinlock_cli(&schdap->sda_lock, &cufg);
    krlspinlock_cli(&tdp->td_lock, &tcufg);

    tdp->td_stus = TDSTUS_RUN;  // 设置进程的状态为运行状态

    krlspinunlock_sti(&tdp->td_lock, &tcufg);
    list_add_tail(&tdp->td_list, &(schdap->sda_thdlst[pity].tdl_lsth));
    schdap->sda_thdlst[pity].tdl_nr++;
    krlspinunlock_sti(&schdap->sda_lock, &cufg);

    return;
err_step:
    hal_sysdie("krlsched_up err");
    return;
}

/**
 * @brief 获取空转进程
 *  1. 在选择下一个进程的函数中，如果没有找到合适的进程，就返回默认的空转进程
 * 
 * @return thread_t* 
 */
thread_t* krlsched_retn_idlethread()
{
    uint_t cpuid = hal_retn_cpuid();
    // 通过cpuid获取当前cpu的调度数据结构
    schdata_t *schdap = &osschedcls.scls_schda[cpuid];

    // 若调度数据结构中空转进程的指针为空，就出错死机
    if (schdap->sda_cpuidle == NULL) {
        hal_sysdie("schdap->sda_cpuidle NULL");
    }

    // 返回空转进程
    return schdap->sda_cpuidle;
}

void krlsched_set_schedflgs()
{
    cpuflg_t cpuflg;
    uint_t cpuid = hal_retn_cpuid();
    schdata_t *schdap = &osschedcls.scls_schda[cpuid];

    krlspinlock_cli(&schdap->sda_lock, &cpuflg);
    schdap->sda_schdflgs = NEED_SCHED_FLGS;
    krlspinunlock_sti(&schdap->sda_lock, &cpuflg);
    return;
}

// 设置CPU进程管理状态
void krlsched_set_schedflgs_ex(uint_t flags)
{
    cpuflg_t cpuflg;
    uint_t cpuid = hal_retn_cpuid();
    schdata_t *schdap = &osschedcls.scls_schda[cpuid];

    krlspinlock_cli(&schdap->sda_lock, &cpuflg);
    schdap->sda_schdflgs = flags;
    krlspinunlock_sti(&schdap->sda_lock, &cpuflg);
    return;
}

void krlsched_chkneed_pmptsched()
{
    cpuflg_t cpuflg;
    uint_t schd = 0, cpuid = hal_retn_cpuid();
    schdata_t *schdap = &osschedcls.scls_schda[cpuid];

    krlspinlock_cli(&schdap->sda_lock, &cpuflg);

    if (schdap->sda_schdflgs == NEED_SCHED_FLGS && schdap->sda_premptidx == PMPT_FLGS) {
        schdap->sda_schdflgs = NOTS_SCHED_FLGS;
        schd = 1;
    }

    if (schdap->sda_schdflgs == NEED_START_CPUILDE_SCHED_FLGS) {
        schd = 1;
    }

    krlspinunlock_sti(&schdap->sda_lock, &cpuflg);
    if (schd == 1) {
        krlschedul();
    }

    return;
}

/**
 * @brief 选择下一个进程
 *  1. 从高到低扫描优先级进程链表，然后若当前优先级进程链表不为空，就取出该链表上的第一个进程
 *  2. 放入 thrdlst_t 结构中的 tdl_curruntd 字段中，并把之前 thrdlst_t 结构的 tdl_curruntd 字段中的进程挂入该链表的尾部，并返回
 *  3. 最后，当扫描到最低优先级时也没有找到进程，就返回默认的空转进程
 * 
 * @return thread_t* 
 */
thread_t *krlsched_select_thread()
{
    thread_t *retthd = NULL, *tdtmp = NULL, *cur = NULL;
    list_h_t *pos = NULL;
    cpuflg_t cufg;
    uint_t cpuid = hal_retn_cpuid();
    schdata_t *schdap = &osschedcls.scls_schda[cpuid];

    krlspinlock_cli(&schdap->sda_lock, &cufg);

    // 从最高优先级开始扫描
    for (uint_t pity = 0; pity < PRITY_MAX; pity++) {
        // 若当前优先级的进程链表不为空
        // 取出当前优先级进程链表下的第一个进程
        list_for_each(pos, &(schdap->sda_thdlst[pity].tdl_lsth)) {
            tdtmp = list_entry(pos, thread_t, td_list);
            // 进程状态
            if (tdtmp->td_stus == TDSTUS_RUN || tdtmp->td_stus == TDSTUS_NEW) {
                list_del(&tdtmp->td_list);  // 脱链
                // 将这sda_thdlst[pity].tdl_curruntd的进程挂入链表尾
                cur = schdap->sda_thdlst[pity].tdl_curruntd;
                if (cur != NULL) {
                    list_add_tail(&cur->td_list, &schdap->sda_thdlst[pity].tdl_lsth);
                }

                schdap->sda_thdlst[pity].tdl_curruntd = tdtmp;
                // 将选择的进程放入sda_thdlst[pity].tdl_curruntd中，并返回
                retthd = tdtmp;
                goto return_step;

            }
        }

        if (schdap->sda_thdlst[pity].tdl_curruntd != NULL) {
            retthd = schdap->sda_thdlst[pity].tdl_curruntd;
            goto return_step;
        }
    }

    // 如果最后也没有找到进程就返回默认的空转进程
    schdap->sda_prityidx = PRITY_MIN;
    retthd = krlsched_retn_idlethread();

return_step:
    // 解锁并返回进程
    krlspinunlock_sti(&schdap->sda_lock, &cufg);
    return retthd;
}

/**
 * @brief 进程调度器入口
 *  1. 确定当前正在运行的进程，然后选择下一个将要运行的进程
 *  2. 最后从当前运行的进程，切换到下一个将要运行的进程
 * 
 */
void krlschedul()
{
    // 运行空转进程
    if (krlsched_retn_schedflgs() == NEED_START_CPUILDE_SCHED_FLGS) {
        // 设置CPU进程管理状态
        krlsched_set_schedflgs_ex(NOTS_SCHED_FLGS);
        /**
         * krlsched_retn_idlethread: 获取空转进程
         */
        retnfrom_first_sched(krlsched_retn_idlethread());
        return;
    }

    thread_t *prev = krlsched_retn_currthread(),    // 返回当前运行进程
             *next = krlsched_select_thread();      // 选择下一个运行的进程

    kprint("krlschedul run currtd:%x,nexttd:%x\n", prev, next);

    // 从当前进程切换到下一个进程
    save_to_new_context(next, prev);
    return;
}

void krlschdclass_del_thread_addexit(thread_t *thdp)
{
    uint_t cpuid = hal_retn_cpuid();
    schdata_t *schdap = &osschedcls.scls_schda[cpuid];
    cpuflg_t cufg;

    krlspinlock_cli(&schdap->sda_lock, &cufg);

    schdap->sda_thdlst[thdp->td_priority].tdl_nr--;
    schdap->sda_threadnr--;

    if (schdap->sda_thdlst[thdp->td_priority].tdl_curruntd == thdp) {
        schdap->sda_thdlst[thdp->td_priority].tdl_curruntd = NULL;
    }

    list_del(&thdp->td_list);
    thdp->td_stus = TDSTUS_EXIT;
    list_add(&thdp->td_list, &schdap->sda_exitlist);

    krlspinunlock_sti(&schdap->sda_lock, &cufg);
    return;
}

/**
 * @brief 进程退出
 * 
 */
void krlsched_exit()
{
    //cpuflg_t cufg, tcufg;
    uint_t cpuid = hal_retn_cpuid();
    schdata_t *schdap = &osschedcls.scls_schda[cpuid];
    thread_t *tdp = krlsched_retn_currthread();

    if (tdp == krlsched_retn_idlethread())
    {
        system_error("krlsched_exit");
        return;
    }

    uint_t pity = tdp->td_priority;

    if (pity >= PRITY_MAX)
    {
        goto err_step;
    }
    if (schdap->sda_thdlst[pity].tdl_nr < 1)
    {
        goto err_step;
    }

    return krlschdclass_del_thread_addexit(tdp);
err_step:
    hal_sysdie("krlsched_wait err");
    return;
}

/**
 * @brief 把进程加入调度系统(原子操作)
 * 
 * @param thdp 进程实例
 */
void krlschdclass_add_thread(thread_t *thdp)
{
    uint_t cpuid = hal_retn_cpuid();
    schdata_t *schdap = &osschedcls.scls_schda[cpuid];
    cpuflg_t cufg;

    // 按进程的优先级进行下标存储
    krlspinlock_cli(&schdap->sda_lock, &cufg);
    list_add(&thdp->td_list, &schdap->sda_thdlst[thdp->td_priority].tdl_lsth);
    schdap->sda_thdlst[thdp->td_priority].tdl_nr++;
    schdap->sda_threadnr++;
    krlspinunlock_sti(&schdap->sda_lock, &cufg);

    // 增减运行中的进程的数量
    krlspinlock_cli(&osschedcls.scls_lock, &cufg);
    osschedcls.scls_threadnr++;
    krlspinunlock_sti(&osschedcls.scls_lock, &cufg);

    return;
}

/**
 * @brief 上下文切换
 *  1. 设置当前运行的进程，处理 CPU 发生中断时需要切换栈的问题
 *  2. tss切换
 *  3. 切换了一个进程的 MMU 页表（即使用新进程的地址空间）
 *  4. 最后如果是新建进程第一次运行，就调用 retnfrom_first_sched 函数进行处理
 * 
 * @param next 上一个进程
 * @param prev 下一个进程
 */
void __to_new_context(thread_t *next, thread_t *prev)
{
    uint_t cpuid = hal_retn_cpuid();
    schdata_t *schdap = &osschedcls.scls_schda[cpuid];

    // 设置当前运行进程为下一个运行的进程
    schdap->sda_currtd = next;

    // 设置下一个运行进程的tss为当前CPU的tss
    next->td_context.ctx_nexttss = &x64tss[cpuid];
    // 设置当前CPU的tss中的R0栈为下一个运行进程的内核栈
    next->td_context.ctx_nexttss->rsp0 = next->td_krlstktop;

    // 装载下一个运行进程的MMU页表
    hal_mmu_load(&next->td_mmdsc->msd_mmu);

    // 如果是新建进程第一次运行就要进行处理
    if (next->td_stus == TDSTUS_NEW) {
        next->td_stus = TDSTUS_RUN;
        retnfrom_first_sched(next);
    }

    return;
}

/**
 * @brief 进程切换的函数
 *  1. 首先把当前进程的通用寄存器保存到当前进程的内核栈中
 *  2. 然后，保存 CPU 的 RSP 寄存器到当前进程的机器上下文结构中，并且读取保存在下一个进程机器上下文结构中的 RSP 的值，把它存到 CPU 的 RSP 寄存器中
 *      接着，调用一个函数切换 MMU 页表后，从下一个进程的内核栈中恢复下一个进程的通用寄存器  
 * 
 * @param next 下一个进程
 * @param prev 上一个进程
 */
void save_to_new_context(thread_t *next, thread_t *prev)
{
#ifdef CFG_X86_PLATFORM
    __asm__ __volatile__(
        "pushfq \n\t"
        "cli \n\t"
        "pushq %%rax\n\t"
        "pushq %%rbx\n\t"
        "pushq %%rcx\n\t"
        "pushq %%rdx\n\t"
        "pushq %%rbp\n\t"
        "pushq %%rsi\n\t"
        "pushq %%rdi\n\t"
        "pushq %%r8\n\t"
        "pushq %%r9\n\t"
        "pushq %%r10\n\t"
        "pushq %%r11\n\t"
        "pushq %%r12\n\t"
        "pushq %%r13\n\t"
        "pushq %%r14\n\t"
        "pushq %%r15\n\t"

        "movq %%rsp, %[PREV_RSP] \n\t"   // 保存CPU的RSP寄存器到当前进程的机器上下文结构中
        // 把下一个进程的机器上下文结构中的RSP的值，写入CPU的RSP寄存器中
        // 事实上这里已经切换到下一个进程了，因为切换进程的内核栈
        "movq %[NEXT_RSP], %%rsp \n\t"
        // 调用__to_new_context函数切换MMU页表
        "callq __to_new_context\n\t"

        // 恢复下一个进程的通用寄存器
        "popq %%r15\n\t"
        "popq %%r14\n\t"
        "popq %%r13\n\t"
        "popq %%r12\n\t"
        "popq %%r11\n\t"
        "popq %%r10\n\t"
        "popq %%r9\n\t"
        "popq %%r8\n\t"
        "popq %%rdi\n\t"
        "popq %%rsi\n\t"
        "popq %%rbp\n\t"
        "popq %%rdx\n\t"
        "popq %%rcx\n\t"
        "popq %%rbx\n\t"
        "popq %%rax\n\t"
        "popfq \n\t"        // 恢复下一个进程的标志寄存器
        : [ PREV_RSP ] "=m"(prev->td_context.ctx_nextrsp)                       // 输出当前进程的内核栈地址
        // 读取下一个进程的内核栈地址
        : [ NEXT_RSP ] "m"(next->td_context.ctx_nextrsp), "D"(next), "S"(prev)
        : "memory");
#endif
    return;
}

/**
 * @brief 新进程第一次运行
 *  1. 把要运行的地址，复制到rsp栈寄存器中
 *  2. 恢复进程保存在内核栈中的段寄存器
 *  3. 通过iretd返回原来进程的上下文
 * 
 * @param thrdp 进程实例
 */
void retnfrom_first_sched(thread_t *thrdp)
{

#ifdef CFG_X86_PLATFORM
    __asm__ __volatile__(
        "movq %[NEXT_RSP],%%rsp\n\t"    // 设置CPU的RSP寄存器为该进程机器上下文结构中的RSP
        // 恢复进程保存在内核栈中的段寄存器
        "popq %%r14\n\t"
        "movw %%r14w, %%gs\n\t"
        "popq %%r14\n\t"
        "movw %%r14w, %%fs\n\t"
        "popq %%r14\n\t"
        "movw %%r14w, %%es\n\t"
        "popq %%r14\n\t"
        "movw %%r14w, %%ds\n\t"
        // 恢复进程保存在内核栈中的通用寄存器
        "popq %%r15\n\t"
        "popq %%r14\n\t"
        "popq %%r13\n\t"
        "popq %%r12\n\t"
        "popq %%r11\n\t"
        "popq %%r10\n\t"
        "popq %%r9\n\t"
        "popq %%r8\n\t"
        "popq %%rdi\n\t"
        "popq %%rsi\n\t"
        "popq %%rbp\n\t"
        "popq %%rdx\n\t"
        "popq %%rcx\n\t"
        "popq %%rbx\n\t"
        "popq %%rax\n\t"
        // 恢复进程保存在内核栈中的RIP、CS、RFLAGS，（有可能需要恢复进程应用程序的RSP、SS）寄存器
        "iretq\n\t"

        :
        : [ NEXT_RSP ] "m"(thrdp->td_context.ctx_nextrsp)
        : "memory");
#endif
}
