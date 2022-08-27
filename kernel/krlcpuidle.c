/**********************************************************
    idle线程头文件krlcpuidle.c
***********************************************************/
#include "cosmostypes.h"
#include "cosmosmctrl.h"

// 进程A主函数
void thread_a_main()
{
    uint_t i = 0;
    // for (; i > 10; i++) {
        kprint("进程A运行:%x\n", i);
        // die(200); //延迟一会
        // krlschedul();
    // }
    return;
}

// 进程B主函数
void thread_b_main()
{
    uint_t i = 0;
    for (; i > 10; i++) {
        kprint("进程B运行:%x\n", i);
        die(150); //延迟一会
        krlschedul();
    }
    return;
}

void init_ab_thread()
{
    krlnew_thread("kernelthread-a", (void*)thread_a_main, KERNTHREAD_FLG, PRILG_SYS, PRITY_MIN, DAFT_TDUSRSTKSZ, DAFT_TDKRLSTKSZ);
    // krlnew_thread("kernelthread-b", (void*)thread_b_main, KERNTHREAD_FLG, PRILG_SYS, PRITY_MIN, DAFT_TDUSRSTKSZ, DAFT_TDKRLSTKSZ);
    return;
}

void init_user_thread()
{
    thread_t *t = NULL;
    t = krlnew_thread("oneuser.app", (void *)APPRUN_START_VITRUALADDR, USERTHREAD_FLG, PRILG_USR, PRITY_MIN, DAFT_TDUSRSTKSZ, DAFT_TDKRLSTKSZ);
    kprint("init_user_thread t1: %x\n", t);
    t = krlthread_execvl(t, "oneuser.app");
    kprint("init_user_thread t2: %x\n", t);
    if (NULL != t) {
        kprint("oneuser.app进程建立成功:%x\n", (uint_t)t);
    }
    return;
}

// 初始化空转进程
void init_krlcpuidle()
{
    new_cpuidle();      // 建立空转进程
    init_ab_thread();   // 初始化建立A、B进程
    // init_user_thread();
    krlcpuidle_start(); // 启动空转进程运行
    return;
}

/**
 * @brief 空转进程运行
 *  1. 首先就是取出空转进程
 *  2. 然后设置一下机器上下文结构和运行状态
 *  3. 最后调用 retnfrom_first_sched 函数，恢复进程内核栈中的内容，让进程启动运行
 */
void krlcpuidle_start()
{

    uint_t cpuid = hal_retn_cpuid();
    schdata_t *schdap = &osschedcls.scls_schda[cpuid];

    // 取得空转进程
    thread_t *tdp = schdap->sda_cpuidle;

    // 设置空转进程的tss和R0特权级的栈
    tdp->td_context.ctx_nexttss = &x64tss[cpuid];
    x64tss[cpuid].rsp0 = tdp->td_krlstktop;
    tdp->td_context.ctx_nexttss->rsp0 = tdp->td_krlstktop;
    // 设置空转进程的状态为运行状态
    tdp->td_stus = TDSTUS_RUN;
    // 启动进程运行
    retnfrom_first_sched(tdp);

    return;
}

/**
 * @brief 建立空转进程
 *  1. 并把这个空进程CPU下的schdata中
 * 
 * @return thread_t* 
 */
thread_t *new_cpuidle_thread()
{

    thread_t *ret_td = NULL;
    bool_t acs = FALSE;
    adr_t krlstkadr = NULL;
    uint_t cpuid = hal_retn_cpuid();
    schdata_t *schdap = &osschedcls.scls_schda[cpuid];

    // 分配进程的内核栈
    krlstkadr = krlnew(DAFT_TDKRLSTKSZ);
    if (krlstkadr == NULL) {
        return NULL;
    }

    // 分配thread_t结构体变量
    ret_td = krlnew_thread_dsc();
    if (ret_td == NULL) {
        acs = krldelete(krlstkadr, DAFT_TDKRLSTKSZ);
        if (acs == FALSE) {
            return NULL;
        }
        return NULL;
    }

    thread_name(ret_td, "cpuidle-thread");

    // 设置进程具有系统权限
    ret_td->td_privilege = PRILG_SYS;
    ret_td->td_priority = PRITY_MIN;

    // 设置进程的内核栈顶和内核栈开始地址
    ret_td->td_krlstktop = krlstkadr + (adr_t)(DAFT_TDKRLSTKSZ - 1);
    ret_td->td_krlstkstart = krlstkadr;

    // 初始化进程的内核栈
    krlthread_kernstack_init(ret_td, (void *)krlcpuidle_main, KMOD_EFLAGS);
   
    // 设置调度系统数据结构的空转进程和当前进程为ret_td
    schdap->sda_cpuidle = ret_td;
    schdap->sda_currtd = ret_td;

    return ret_td;
}

// 新建空转进程
void new_cpuidle()
{
    // 建立空转进程
    thread_t *thp = new_cpuidle_thread();
    // 失败则主动死机
    if (thp == NULL) {
        hal_sysdie("newcpuilde err");
    }
    return;
}

void krlcpuidle_main()
{
    uint_t i = 0;
    for (;; i++) {
        // kprint("cpuidle is run:%x\n", i);
        // die(0x400);
        krlschedul();   // 调度进程
    }
    return;
}