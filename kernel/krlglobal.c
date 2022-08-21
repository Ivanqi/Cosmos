/****************************************************************
    kernel全局数据结构文件krlglobal.c
*****************************************************************/
#define KRLGOBAL_HEAD
#include "cosmostypes.h"
#include "cosmosmctrl.h"

#if((defined CFG_X86_PLATFORM) || (defined CFG_S3C2440A_PLATFORM))   

KRL_DEFGLOB_VARIABLE(kvirmemadrs_t, krlvirmemadrs);
// 虚拟内存
KRL_DEFGLOB_VARIABLE(mmadrsdsc_t, initmmadrsdsc);

KRL_DEFGLOB_VARIABLE(kmempool_t,oskmempool);

KRL_DEFGLOB_VARIABLE(schedclass_t,osschedcls);
KRL_DEFGLOB_VARIABLE(ktime_t,osktime);

KRL_DEFGLOB_VARIABLE(syscall_t,osscalltab)[INR_MAX] = {
    NULL,krlsvetabl_mallocblk,              // 内存分配服务接口
    krlsvetabl_mfreeblk,                    // 内存释放服务接口
    krlsvetabl_exel_thread,                 // 进程服务接口
    krlsvetabl_exit_thread,                 // 进程退出服务接口
    krlsvetabl_retn_threadhand,             // 获取进程id服务接口
    krlsvetabl_retn_threadstats,            // 获取进程状态服务接口
    krlsvetabl_set_threadstats,             // 设置进程状态服务接口
    krlsvetabl_open,krlsvetabl_close,       // 文件打开、关闭服务接口
    krlsvetabl_read,krlsvetabl_write,       // 文件读、写服务接口
    krlsvetabl_ioctrl,krlsvetabl_lseek,     // 文件随机读写和控制服务接口
    krlsvetabl_time                         // 获取时间服务接口
};

KRL_DEFGLOB_VARIABLE(devtable_t, osdevtable);
// KRL_DEFGLOB_VARIABLE(iocheblkdsc_t,osiocheblk);
// 驱动程序表，然后再init_krldriver中运行
KRL_DEFGLOB_VARIABLE(drventyexit_t, osdrvetytabl)[] = {
    systick_entry
};
#endif

