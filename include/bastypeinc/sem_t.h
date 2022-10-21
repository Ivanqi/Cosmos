/**********************************************************
	信号量头文件sem_t.h
***********************************************************/

#ifndef _SEM_T_H
#define _SEM_T_H

#define SEM_FLG_MUTEX 0
#define SEM_FLG_MULTI 1
#define SEM_MUTEX_ONE_LOCK 1
#define SEM_MULTI_LOCK 0

// 信号量同时解决了三个问题：等待、互斥、唤醒

/**
 * @brief 等待链数据结构
 * 	1. 用于挂载等待代码执行流（线程）的结构
 * 	2. 里面有用于挂载代码执行流的链表和计数器变量
 */
typedef struct s_KWLST {
    spinlock_t wl_lock;		// 自旋锁
    uint_t wl_tdnr;			// 等待进程的个数
    list_h_t wl_list;		// 挂载等待进程的链表头
} kwlst_t;

/**
 * @brief 信号量数据结构
 */
typedef struct s_SEM {
    spinlock_t sem_lock; // 维护sem_t自身数据的自旋锁
	uint_t sem_flg; 	 // 信号量相关的标志
	sint_t sem_count;	 // 信号量计数值
	kwlst_t sem_waitlst; // 用于挂载等待代码执行流（线程）结构
} sem_t;

typedef struct s_WAIT_L_HEAD {
	list_h_t wlh_llist;
	spinlock_t wlh_lock;
	atomic_t wlh_count;
	void* wlh_privte;
	bool_t  (*wlh_upfun)(u32_t func,struct s_WAIT_L_HEAD* wlhp);
} wait_l_head_t;

typedef struct s_WAIT_L {
	list_h_t wl_hlist;
	u32_t wl_flags;
	void* wl_thead;
} wait_l_t;

#endif