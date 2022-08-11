/**********************************************************
    线程管理头文件krlthread_t.h
***********************************************************/
#ifndef _KRLTHREAD_T_H
#define _KRLTHREAD_T_H

#define TDSTUS_RUN 0        // 进程运行状态
#define TDSTUS_SLEEP 3      // 进程睡眠状态
#define TDSTUS_WAIT 4       // 进程等待状态
#define TDSTUS_NEW 5        // 进程新建状态
#define TDSTUS_ZOMB 6       // 进程僵死状态
#define TDSTUS_EXIT 12      // 进程退出状态

#define TDFLAG_FREE (1)
#define TDFLAG_BUSY (2)


#define TDRUN_TICK 20

#define PRITY_MAX 64   // 最大优先级
#define PRITY_MIN 0    // 最小优先级
#define PRILG_SYS 0    // 内核级
#define PRILG_USR 5    // 用户级

#define MICRSTK_MAX 4

#define THREAD_MAX (4)

#define THREAD_NAME_MAX (64)    // 名字最长长度

// 内核态
#define KERNTHREAD_FLG 0
// 用户态
#define USERTHREAD_FLG 3

#if((defined CFG_X86_PLATFORM)) 
#define DAFT_TDUSRSTKSZ 0x8000
#define DAFT_TDKRLSTKSZ 0x8000  // 3kb
#endif


#if((defined CFG_X86_PLATFORM)) 
#define TD_HAND_MAX 8
#define DAFT_SPSR 0x10
#define DAFT_CPSR 0xd3
#define DAFT_CIDLESPSR 0x13   
#endif

#define K_CS_IDX    0x08        // 选择子
#define K_DS_IDX    0x10        // 选择子
#define U_CS_IDX    0x1b
#define U_DS_IDX    0x23
#define K_TAR_IDX   0x28
// 标志寄存器
#define UMOD_EFLAGS 0x1202
#define KMOD_EFLAGS	0x202

typedef struct s_MICRSTK {
    uint_t msk_val[MICRSTK_MAX];
} micrstk_t;

// 进程的机器上下文
typedef struct s_CONTEXT
{  
    uint_t       ctx_nextrip;   // 保存下一次运行的地址
    uint_t       ctx_nextrsp;   // 保存下一次运行时内核栈的地址
    x64tss_t*    ctx_nexttss;   // 指向tss结构
}context_t;

// 进程结构体
typedef struct s_THREAD {
    spinlock_t  td_lock;                // 进程的自旋锁
    list_h_t    td_list;                // 进程链表
    uint_t      td_flgs;                // 进程的标志
    uint_t      td_stus;                // 进程的状态
    uint_t      td_cpuid;               // 进程所在的CPU的id
    uint_t      td_id;                  // 进程id
    uint_t      td_tick;                // 进程运行了多少tick
    uint_t      td_sumtick;
    uint_t      td_privilege;           // 进程的权限
    uint_t      td_priority;            // 进程的优先级，数值越小优先级越高
    uint_t      td_runmode;             // 进程的运行模式
    adr_t       td_krlstktop;           // 应用程序内核栈顶地址
    adr_t       td_krlstkstart;         // 应用程序内核栈开始地址
    adr_t       td_usrstktop;           // 应用程序栈顶地址
    adr_t       td_usrstkstart;         // 应用程序栈开始地址
    mmadrsdsc_t* td_mmdsc;              // 地址空间结构，指向mmadrsdsc_t 结构
    void*       td_resdsc;
    void*       td_privtep;
    void*       td_extdatap;
    char_t*     td_appfilenm;
    uint_t      td_appfilenmlen;
    context_t   td_context;                 // 机器上下文结构
    objnode_t*  td_handtbl[TD_HAND_MAX];    // 打开的对象数组，进程打开的资源的描述符
    char_t      td_name[THREAD_NAME_MAX];   // 进程名称
} thread_t;

#endif // KRLTHREAD_T_H
