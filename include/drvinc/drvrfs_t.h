#ifndef DRVRFS_T_H
#define DRVRFS_T_H

#define DR_NM_MAX (128 - (sizeof(uint_t) * 3))
#define FBLKS_MAX 32
// 目录类型
#define RDR_NUL_TYPE 0  // 空类型
#define RDR_DIR_TYPE 1  // 目录类型
#define RDR_FIL_TYPE 2  // 文件类型
#define RDR_DEL_TYPE 5  // 已删除的类型

#define FMD_NUL_TYPE 0
#define FMD_DIR_TYPE 1
#define FMD_FIL_TYPE 2
#define FMD_DEL_TYPE 5  // 文件管理头也需要表明它管理的是目录文件还是普通文件
#define FSMM_BLK 0x400000
#define FSYS_ALCBLKSZ 0x1000    // 4096

typedef struct s_RFSDEVEXT {
    spinlock_t rde_lock;    // 自旋锁
    list_h_t rde_list;      // 链表
    uint_t rde_flg;         // 标志
    uint_t rde_stus;        // 状态
    void* rde_mstart;       // 用于模拟储存介质的内存块的开始地址
    size_t rde_msize;       // 内存块的大小
    void* rde_ext;          // 扩展所用
} rfsdevext_t;


typedef struct s_RFSDIR {
    uint_t rdr_stus;                // 目录状态
    uint_t rdr_type;                // 目录类型，可以是空类型、目录类型、文件类型、已删除的类型
    uint_t rdr_blknr;               // 指向文件数据管理头的块号，不像内存可以用指针，只能按块访问
    char_t rdr_name[DR_NM_MAX];     // 名称数组，大小为DR_NM_MAX
} rfsdir_t;

typedef struct s_FILBLKS {
    uint_t fb_blkstart;     // 开始的逻辑储存块号
    uint_t fb_blknr;        // 逻辑储存块的块数，从blkstart开始的连续块数
} filblks_t;

// 超级块
typedef struct s_RFSSUBLK {
    spinlock_t rsb_lock;        // 超级块在内存中使用的自旋锁
    uint_t rsb_mgic;            // 文件系统标识
    uint_t rsb_vec;             // 文件系统版本
    uint_t rsb_flg;             // 标志
    uint_t rsb_stus;            // 状态
    size_t rsb_sz;              // 该数据结构本身的大小
    size_t rsb_sblksz;          // 超级块大小
    size_t rsb_dblksz;          // 文件系统逻辑储存块大小，我们这里用的是4KB
    uint_t rsb_bmpbks;          // 位图的开始逻辑储存块
    uint_t rsb_bmpbknr;         // 位图占用多少个逻辑储存块
    uint_t rsb_fsysallblk;      // 文件系统有多少个逻辑储存块
    rfsdir_t rsb_rootdir;       // 根目录，后面会看到这个数据结构的
} rfssublk_t;

typedef struct s_fimgrhd {
    uint_t fmd_stus;                    // 文件状态
    uint_t fmd_type;                    // 文件类型：可以是目录文件、普通文件、空文件、已删除的文件
    uint_t fmd_flg;                     // 文件标志
    uint_t fmd_sfblk;                   // 文件管理头自身所在的逻辑储存块
    uint_t fmd_acss;                    // 文件访问权限
    uint_t fmd_newtime;                 // 文件的创建时间，换算成秒
    uint_t fmd_acstime;                 // 文件的访问时间，换算成秒
    uint_t fmd_fileallbk;               // 文件一共占用多少个逻辑储存块
    uint_t fmd_filesz;                  // 文件大小
    uint_t fmd_fileifstbkoff;           // 文件数据在第一块逻辑储存块中的偏移
    uint_t fmd_fileiendbkoff;           // 文件数据在最后一块逻辑储存块中的偏移
    uint_t fmd_curfwritebk;             // 文件数据当前将要写入的逻辑储存块
    uint_t fmd_curfinwbkoff;            // 文件数据当前将要写入的逻辑储存块中的偏移
    filblks_t fmd_fleblk[FBLKS_MAX];    // 文件占用逻辑储存块的数组，一共32个filblks_t结构
    uint_t fmd_linkpblk;                // 指向文件的上一个文件管理头的逻辑储存块
    uint_t fmd_linknblk;                // 指向文件的下一个文件管理头的逻辑储存块
} fimgrhd_t;

#endif
