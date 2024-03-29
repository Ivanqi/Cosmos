/**********************************************************
    物理内存空间数组文件msadsc_t.h
***********************************************************/
#ifndef _MSADSC_T_H
#define _MSADSC_T_H

#define PAGPHYADR_SZLSHBIT (12)
#define MSAD_PAGE_MAX (8)
#define MSA_SIZE (1 << PAGPHYADR_SZLSHBIT)	// 2 的 12次方，页目标大小是4KB，也就是4096个字节

// 挂入链表的类型
#define MF_OLKTY_INIT (0)
#define MF_OLKTY_ODER (1)
#define MF_OLKTY_BAFH (2)	// 单个msadsc_t
#define MF_OLKTY_TOBJ (3)

// 是否挂入链表
#define MF_LSTTY_LIST (0)

// 分配类型(内核、应用、空闲)
#define MF_MOCTY_FREE (0)
#define MF_MOCTY_KRNL (1)
#define MF_MOCTY_USER (2)

#define MF_MRV1_VAL (0)

// 分配计数
#define MF_UINDX_INIT (0)
#define MF_UINDX_MAX (0xffffff)

// 属于哪个区
#define MF_MARTY_INIT (0)
#define MF_MARTY_HWD (1)	// 硬件区
#define MF_MARTY_KRL (2)	// 内核区
#define MF_MARTY_PRC (3)	// 应用区
#define MF_MARTY_SHD (4)	// 共享区

// 内存空间地址描述符标志
typedef struct s_MSADFLGS {
    u32_t mf_olkty:2;	// 挂入链表的类型
	u32_t mf_lstty:1;	// 是否挂入链表
	u32_t mf_mocty:2;	// 分配类型，被谁占用来，内核还是应用或者空闲
	u32_t mf_marty:3;	// 属于哪个区
	u32_t mf_uindx:24;	// 分配计数
} __attribute__((packed)) msadflgs_t;

// 分配位
#define  PAF_NO_ALLOC (0)	// 未分配 
#define  PAF_ALLOC (1)		// 已分配

// 共享位
#define  PAF_NO_SHARED (0)

// 交换位
#define  PAF_NO_SWAP (0)

// 缓存位
#define  PAF_NO_CACHE (0)

// 映射位
#define  PAF_NO_KMAP (0)

// 锁定位
#define  PAF_NO_LOCK (0)

// 脏位
#define  PAF_NO_DIRTY (0)

// 忙位
#define  PAF_NO_BUSY (0)

// 保留位
#define  PAF_RV2_VAL (0)

// 页物理地址位
#define  PAF_INIT_PADRS (0)

// 物理地址和标志
typedef struct s_PHYADRFLGS {
    u64_t paf_alloc:1;	// 分配位
	u64_t paf_shared:1;	// 共享位
	u64_t paf_swap:1;	// 交换位
	u64_t paf_cache:1;	// 缓存位
	u64_t paf_kmap:1;	// 映射位
	u64_t paf_lock:1;	// 锁定位
	u64_t paf_dirty:1;	// 脏位
	u64_t paf_busy:1;	// 忙位
	u64_t paf_rv2:4;	// 保留位
	u64_t paf_padrs:52;	// 页物理地址位
} __attribute__((packed)) phyadrflgs_t;

// 内存空间地址描述符
typedef struct s_MSADSC {
    list_h_t md_list;               // 16，链表
	spinlock_t md_lock;             // 4，保护自身的自旋锁
	msadflgs_t md_indxflgs;         // 4，内存空间地址描述符标志
	phyadrflgs_t md_phyadrs;        // 8，物理地址和标志
	void* md_odlink;                // 8，相邻且相同大小msdsc的指针，所以msadsc_t本身也是一个连续内存页链表
} __attribute__((packed)) msadsc_t; // 32+24;

#endif