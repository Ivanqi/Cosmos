/****************************************************************
    kernel全局数据结构头文件krlglobal.h
*****************************************************************/
#ifndef _KRLGLOBAL_H
#define _KRLGLOBAL_H

#ifdef	KRLGOBAL_HEAD
#undef	KEXTERN
#define KEXTERN
#endif
#if((defined CFG_X86_PLATFORM) || (defined CFG_S3C2440A_PLATFORM))   
KRL_DEFGLOB_VARIABLE(kvirmemadrs_t, krlvirmemadrs);
// mmadrsdsc_t 数据结构的实例变量
KRL_DEFGLOB_VARIABLE(mmadrsdsc_t, initmmadrsdsc);

KRL_DEFGLOB_VARIABLE(kmempool_t, oskmempool);

// 管理进程的初始化
KRL_DEFGLOB_VARIABLE(schedclass_t, osschedcls);
KRL_DEFGLOB_VARIABLE(ktime_t, osktime);
// 系统服务表
KRL_DEFGLOB_VARIABLE(syscall_t, osservicetab)[INR_MAX];
// 管理设备
KRL_DEFGLOB_VARIABLE(devtable_t, osdevtable);
// KRL_DEFGLOB_VARIABLE(iocheblkdsc_t,osiocheblk);
// 驱动程序表，然后再init_krldriver中运行
KRL_DEFGLOB_VARIABLE(drventyexit_t,osdrvetytabl)[];
#endif

#endif // KRLGLOBAL_H
