/**********************************************************
    基本数据类型文件bastype_t.h
***********************************************************/

#ifndef _BASTYPE_T_H
#define _BASTYPE_T_H

// 无符号
typedef unsigned char u8_t;
typedef unsigned short u16_t;
typedef unsigned int u32_t;
typedef unsigned long u64_t;

// 有符号
typedef char s8_t;
typedef short s16_t;
typedef int s32_t;
typedef long s64_t;
typedef long sint_t;

typedef unsigned long uint_t;
typedef u64_t cpuflg_t;
typedef unsigned long adr_t;

typedef sint_t bool_t;
typedef u32_t dev_t;
typedef const char* str_t;
typedef char char_t;
typedef unsigned long drv_t;
typedef unsigned long mrv_t;
typedef sint_t drvstus_t;
typedef sint_t sysstus_t;
typedef sint_t hand_t;
typedef void* buf_t;
typedef unsigned long size_t;
typedef u32_t reg_t;

// 匿名函数
typedef void (*inthandler_t)();
typedef drv_t (*i_handle_t)(uint_t int_nr);
typedef drv_t (*f_handle_t)(uint_t int_nr,void *sframe);
// 中断回调函数类型
typedef drvstus_t(*intflthandle_t)(uint_t ift_nr, void *device, void *sframe);
typedef u64_t mmstus_t;

#define KLINE static inline
#define PUBLIC
#define private	static
#define EXTERN extern
#define KEXTERN extern

// 布尔值
#define NULL	0
#define TRUE    1
#define	FALSE	0

#define DFCERRSTUS (-1) // 失败
#define DFCOKSTUS (0)   // 成功
#define NO_HAND (-1)
// 对齐
#define ALIGN(x, a)     (((x) + (a) - 1) & ~((a) - 1))

#define LKHEAD_T __attribute__((section(".head.text")))
#define LKHEAD_D __attribute__((section(".head.data")))

#define LKINIT

#define EOK 		 0
#define	EPERM		 1	/* Operation not permitted,  不允许操作*/
#define	ENOENT		 2	/* No such file or directory, 没有这样的文件或目录 */
#define	ESRCH		 3	/* No such process, 没有这样的进程 */
#define	EINTR		 4	/* Interrupted system call, 中断系统调用*/
#define	EIO		     5	/* I/O error, IO 错误 */
#define	ENXIO		 6	/* No such device or address, 没有这样的设备或地址 */
#define	E2BIG		 7	/* Argument list too long, 参数列表太长*/
#define	ENOEXEC		 8	/* Exec format error, Exec格式错误 */
#define	EBADF		 9	/* Bad file number, 错误的文件号 */
#define	ECHILD		10	/* No child processes, 无子进程 */
#define	EAGAIN		11	/* Try again, 重试 */
#define	ENOMEM		12	/* Out of memory, 内存不足 */
#define	EACCES		13	/* Permission denied,  权限拒绝*/
#define	EFAULT		14	/* Bad address, 地址不正确 */
#define	ENOTBLK		15	/* Block device required, 需要块设备 */
#define	EBUSY		16	/* Device or resource busy, 设备或资源忙*/
#define	EEXIST		17	/* File exists, 文件存在 */
#define	EXDEV		18	/* Cross-device link, 跨设备链路 */
#define	ENODEV		19	/* No such device, 没有这样的装置 */
#define	ENOTDIR		20	/* Not a directory, 不是目录 */
#define	EISDIR		21	/* Is a directory, 是一个目录 */
#define	EINVAL		22	/* Invalid argument, 无效参数 */
#define	ENFILE		23	/* File table overflow, 文件表溢出 */
#define	EMFILE		24	/* Too many open files, 打开的文件太多 */
#define	ENOTTY		25	/* Not a typewriter, 不是打字机 */
#define	ETXTBSY		26	/* Text file busy, 文本文件忙 */
#define	EFBIG		27	/* File too large, 文件太大 */
#define	ENOSPC		28	/* No space left on device, 设备上没有剩余空间 */
#define	ESPIPE		29	/* Illegal seek, 非法寻找 */
#define	EROFS		30	/* Read-only file system, 只读文件系统 */
#define	EMLINK		31	/* Too many links, 链接太多 */
#define	EPIPE		32	/* Broken pipe, 断管 */
#define	EDOM		33	/* Math argument out of domain of func, func域外的数学参数 */
#define	ERANGE		34	/* Math result not representable, 数学结果不具有代表性 */
#define EALLOC		35 
#define ENOOBJ 		36
#define EGOON		37  /* go on, 继续 */
#define ECPLT		38  /* Complete, 完成 */
#define EPARAM		39  // 参数错误
#endif