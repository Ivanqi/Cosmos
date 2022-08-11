/**********************************************************
    线程管理文件krlthread.c
***********************************************************/

#include "cosmostypes.h"
#include "cosmosmctrl.h"

void micrstk_t_init(micrstk_t *initp) 
{
    for (uint_t i = 0; i < MICRSTK_MAX; i++) {
        initp->msk_val[i] = 0;
    }
    return;
}

// 初始化context_t结构
void context_t_init(context_t *initp)
{
    initp->ctx_nextrip = 0;
    initp->ctx_nextrsp = 0;
    // 指向当前CPU的tss
    initp->ctx_nexttss = &x64tss[hal_retn_cpuid()];
    return;
}

/**
 * @brief 返回进程id其实就thread_t结构的地址
 * 
 * @param tdp thread_t内存指针
 * @return uint_t 返回进程id
 */
uint_t krlretn_thread_id(thread_t *tdp)
{
    return (uint_t)tdp;
}

// 初始化thread_t结构
void thread_t_init(thread_t *initp)
{
    krlspinlock_init(&initp->td_lock);          // 进程的自旋锁
    list_init(&initp->td_list);                 // 进程链表
    initp->td_flgs = TDFLAG_FREE;               // 进程的标志
    initp->td_stus = TDSTUS_NEW;                // 进程状态为新建
    initp->td_cpuid = hal_retn_cpuid();         // 进程所在的CPU的id
    initp->td_id = krlretn_thread_id(initp);    // 进程id
    initp->td_tick = 0;                         // 进程运行了多个tick
    initp->td_privilege = PRILG_USR;            // 普通进程权限
    initp->td_priority = PRITY_MIN;             // 最高优先级，数值越小优先级越高
    initp->td_runmode = 0;                      // 进程的运行模式
    initp->td_krlstktop = NULL;                 // 应用程序内核栈顶地址
    initp->td_krlstkstart = NULL;               // 应用程序内核栈开始地址
    initp->td_usrstktop = NULL;                 // 应用程序栈顶地址
    initp->td_usrstkstart = NULL;               // 应用程序栈开始地址
    initp->td_mmdsc = &initmmadrsdsc;           // 指向默认的地址空间结构
    initp->td_resdsc = NULL;
    initp->td_privtep = NULL;
    initp->td_extdatap = NULL;
    initp->td_appfilenm = NULL;
    initp->td_appfilenmlen = 0;

    context_t_init(&initp->td_context);

    // 初始化td_handtbl数组
    for (uint_t hand = 0; hand < TD_HAND_MAX; hand++) {
        initp->td_handtbl[hand] = NULL;
    }

    krlmemset((void*)initp->td_name, 0, THREAD_NAME_MAX);
    return;
}

// 创建thread_t结构
thread_t *krlnew_thread_dsc()
{
    // 分配thread_t结构大小的内存空间
    thread_t *rettdp = (thread_t *)(krlnew((size_t)(sizeof(thread_t))));
    if (rettdp == NULL) {
        return NULL;
    }
    // 初始化刚刚分配的thread_t结构
    thread_t_init(rettdp);
    return rettdp;
}

void krlthd_inc_tick(thread_t *thdp)
{
    cpuflg_t cpuflg;
    krlspinlock_cli(&thdp->td_lock, &cpuflg);
    thdp->td_tick++;

    if (thdp->td_tick > TDRUN_TICK) {
        thdp->td_tick = 0;
        krlsched_set_schedflgs();
    }

    krlspinunlock_sti(&thdp->td_lock, &cpuflg);
    return;
}

hand_t krlthd_retn_nullhand(thread_t *thdp)
{
    cpuflg_t cpuflg;
    hand_t rethd = NO_HAND;
    krlspinlock_cli(&thdp->td_lock, &cpuflg);

    for (uint_t hand = 0; hand < TD_HAND_MAX; hand++) {
        if (thdp->td_handtbl[hand] == NULL) {
            rethd = (hand_t)hand;
            goto retn_step;
        }
    }
    rethd = NO_HAND;
retn_step:
    krlspinunlock_sti(&thdp->td_lock, &cpuflg);
    return rethd;
}

hand_t krlthd_add_objnode(thread_t *thdp, objnode_t *ondp)
{
    cpuflg_t cpuflg;
    hand_t rethd = NO_HAND;
    krlspinlock_cli(&thdp->td_lock, &cpuflg);
    for (uint_t hand = 0; hand < TD_HAND_MAX; hand++) {
        if (thdp->td_handtbl[hand] == NULL) {
            rethd = (hand_t)hand;
            goto next_step;
        }
    }
    rethd = NO_HAND;
    goto retn_step;

next_step:
    thdp->td_handtbl[rethd] = ondp;
retn_step:
    krlspinunlock_sti(&thdp->td_lock, &cpuflg);
    return rethd;
}

hand_t krlthd_del_objnode(thread_t *thdp, hand_t hand)
{
    if ((hand >= TD_HAND_MAX) || (hand <= NO_HAND)) {
        return NO_HAND;
    }

    cpuflg_t cpuflg;
    hand_t rethd = NO_HAND;
    krlspinlock_cli(&thdp->td_lock, &cpuflg);

    if (thdp->td_handtbl[hand] == NULL) {
        rethd = NO_HAND;
        goto retn_step;
    }

    thdp->td_handtbl[hand] = NULL;
    rethd = hand;
retn_step:
    krlspinunlock_sti(&thdp->td_lock, &cpuflg);
    return rethd;
}

objnode_t *krlthd_retn_objnode(thread_t *thdp, hand_t hand)
{
    if ((hand >= TD_HAND_MAX) || (hand <= NO_HAND)) {
        return NULL;
    }

    cpuflg_t cpuflg;
    
    objnode_t *retondp = NULL;
    krlspinlock_cli(&thdp->td_lock, &cpuflg);

    if (thdp->td_handtbl[hand] == NULL) {
        retondp = NULL;
        goto retn_step;
    }

    retondp = thdp->td_handtbl[hand];
retn_step:
    krlspinunlock_sti(&thdp->td_lock, &cpuflg);
    return retondp;
}

// 初始化内核栈
void krlthread_kernstack_init(thread_t *thdp, void *runadr, uint_t cpuflags)
{
    // 处理栈顶16字节对齐
    thdp->td_krlstktop &= (~0xf);
    thdp->td_usrstktop &= (~0xf);

    // 内核栈顶减去intstkregs_t结构的大小。因为地址是从下往上，减去了正好在栈底
    intstkregs_t *arp = (intstkregs_t *)(thdp->td_krlstktop - sizeof(intstkregs_t));
    // 把intstkregs_t结构的空间初始化为0
    hal_memset((void*)arp, 0, sizeof(intstkregs_t));
    
    // rip寄存器的值设为程序运行首地址
    arp->r_rip_old = (uint_t)runadr;
    // cs寄存器的值设为内核代码段选择子
    arp->r_cs_old = K_CS_IDX;
    arp->r_rflgs = cpuflags;
    
    // 返回进程的内核栈
    arp->r_rsp_old = thdp->td_krlstktop;
    arp->r_ss_old = 0;

    // 其它段寄存器的值设为内核数据段选择子
    arp->r_ds = K_DS_IDX;
    arp->r_es = K_DS_IDX;
    arp->r_fs = K_DS_IDX;
    arp->r_gs = K_DS_IDX;
    
    // 设置进程下一次运行的地址为runadr
    thdp->td_context.ctx_nextrip = (uint_t)runadr;
    // 设置进程下一次运行的栈地址为arp
    thdp->td_context.ctx_nextrsp = (uint_t)arp;

    return;
}

// 初始化返回进程应用程序空间的内核栈
void krlthread_userstack_init(thread_t *thdp, void *runadr, uint_t cpuflags)
{
    // 处理栈顶16字节对齐
    thdp->td_krlstktop &= (~0xf);
    thdp->td_usrstktop &= (~0xf);
    // 内核栈顶减去intstkregs_t结构的大小
    intstkregs_t *arp = (intstkregs_t *)(thdp->td_krlstktop - sizeof(intstkregs_t));
    // 把intstkregs_t结构的空间初始化为0
    hal_memset((void*)arp, 0, sizeof(intstkregs_t));
    
    // rip寄存器的值设为程序运行首地址
    arp->r_rip_old = (uint_t)runadr;
    // cs寄存器的值设为应用程序代码段选择子
    arp->r_cs_old = U_CS_IDX;
    arp->r_rflgs = cpuflags;
    // 返回进程应用程序空间的栈
    arp->r_rsp_old = thdp->td_usrstktop;

    // 其它段寄存器的值设为应用程序数据段选择子
    arp->r_ss_old = U_DS_IDX;
    arp->r_ds = U_DS_IDX;
    arp->r_es = U_DS_IDX;
    arp->r_fs = U_DS_IDX;
    arp->r_gs = U_DS_IDX;
    
    // 设置进程下一次运行的地址为runadr
    thdp->td_context.ctx_nextrip = (uint_t)runadr;
    // 设置进程下一次运行的栈地址为arp
    thdp->td_context.ctx_nextrsp = (uint_t)arp;

    return;
}

// 建立普通进程
thread_t *krlnew_user_thread_core(void *filerun, uint_t flg, uint_t prilg, uint_t prity, size_t usrstksz, size_t krlstksz)
{
    thread_t *ret_td = NULL;
    bool_t acs = FALSE;
    adr_t usrstkadr = NULL, krlstkadr = NULL;
    // 分配应用程序栈空间
    usrstkadr = krlnew(usrstksz);
    if (usrstkadr == NULL) {
        return NULL;
    }

    // 分配内核栈空间
    krlstkadr = krlnew(krlstksz);
    if (krlstkadr == NULL) {
        if (krldelete(usrstkadr, usrstksz) == FALSE) {
            return NULL;
        }
        return NULL;
    }

    // 建立thread_t结构体的实例变量
    ret_td = krlnew_thread_dsc();
    // 创建失败必须要释放之前的栈空间
    if (ret_td == NULL) {
        acs = krldelete(usrstkadr, usrstksz);
        acs = krldelete(krlstkadr, krlstksz);
        if (acs == FALSE) {
            return NULL;
        }
        return NULL;
    }

    // 设置进程权限
    ret_td->td_privilege = prilg;
    // 设置进程优先级
    ret_td->td_priority = prity;

    // 设置进程的内核栈顶和内核栈开始地址
    ret_td->td_krlstktop = krlstkadr + (adr_t)(krlstksz - 1);
    ret_td->td_krlstkstart = krlstkadr;

    // 设置进程的应用程序栈顶和内核应用程序栈开始地址
    ret_td->td_usrstktop = usrstkadr + (adr_t)(usrstksz - 1);
    ret_td->td_usrstkstart = usrstkadr;

    // 初始化返回进程应用程序空间的内核栈
    krlthread_userstack_init(ret_td, filerun, UMOD_EFLAGS);
    // 加入调度器系统
    krlschdclass_add_thread(ret_td);
    return ret_td;
}

/**
 * 建立内核进程
 *  内核进程就是用进程的方式去运行一段内核代码
 *  那么这段代码就可以随时暂停或者继续运行，又或者和其它代码段并发运行，只是这种进程永远不会回到进程应用程序地址空间中去，只会在内核地址空间中运行
 * 
 *  首先分配一个内核栈的内存空间，接着创建thread_t结构的实例变量
 *  然后通过thread_t结构体的字段进行设置，最后，初始化进程内核栈把这个新进程加入到进程的调度系统之中
 */
thread_t *krlnew_kern_thread_core(void *filerun, uint_t flg, uint_t prilg, uint_t prity, size_t usrstksz, size_t krlstksz)
{
    thread_t *ret_td = NULL;
    bool_t acs = FALSE;
    adr_t krlstkadr = NULL;

    // 分配内核栈空间
    krlstkadr = krlnew(krlstksz);
    if (krlstkadr == NULL) {
        return NULL;
    }

    // 建立thread_t结构体的实例变量
    ret_td = krlnew_thread_dsc();
    // 创建失败必须要释放之前的栈空间
    if (ret_td == NULL) {
        acs = krldelete(krlstkadr, krlstksz);
        if (acs == FALSE) {
            return NULL;
        }
        return NULL;
    }

    // 设置进程权限
    ret_td->td_privilege = prilg;
    // 设置进程优先级
    ret_td->td_priority = prity;
    
    // 设置进程的内核栈顶和内核栈开始地址
    ret_td->td_krlstktop = krlstkadr + (adr_t)(krlstksz - 1);
    ret_td->td_krlstkstart = krlstkadr;

    // 初始化进程的内核栈
    krlthread_kernstack_init(ret_td, filerun, KMOD_EFLAGS);
    // 加入进程调度系统
    krlschdclass_add_thread(ret_td);
    // 返回进程指针
    return ret_td;
}

/**
 * 建立进程接口
 * @param filerun 应用程序启动运行的地址
 * @param flg 创建标志
 * @param prity 进程权限和进程优先级
 * @param usrstksz 进程的应用程序栈
 * @param krlstksz 内核栈大小
 */
thread_t *krlnew_thread(void *filerun, uint_t flg, uint_t prilg, uint_t prity, size_t usrstksz, size_t krlstksz)
{
    size_t tustksz = 0, tkstksz = 0;
    // 对参数进行检查，不合乎要求就返回NULL表示创建失败
    if (filerun == NULL || usrstksz > DAFT_TDUSRSTKSZ || krlstksz > DAFT_TDKRLSTKSZ) {
        return NULL;
    }

    if ((prilg != PRILG_USR && prilg != PRILG_SYS) || (prity > PRITY_MAX)) {
        return NULL;
    }

    // 进程内核栈大小检查，大小默认大小则使用默认大小
    if (usrstksz < DAFT_TDUSRSTKSZ) {
        tustksz = DAFT_TDUSRSTKSZ;
    }

    // 进程内核栈大小检查，大于默认大小则使用默认大小
    if (krlstksz < DAFT_TDKRLSTKSZ) {
        tkstksz = DAFT_TDKRLSTKSZ;
    }

    // 是否建立内核进程
    if (KERNTHREAD_FLG == flg) {
        return krlnew_kern_thread_core(filerun, flg, prilg, prity, tustksz, tkstksz);
    } else if (USERTHREAD_FLG == flg) { // 是否建立普通进程
        return krlnew_user_thread_core(filerun, flg, prilg, prity, tustksz, tkstksz);
    }

    return NULL;
}
