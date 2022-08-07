/**********************************************************
    MMU头文件halmmu_t.h
***********************************************************/

#ifndef _HALMMU_T_H
#define _HALMMU_T_H

#define TDIRE_MAX (512)
#define SDIRE_MAX (512)
#define IDIRE_MAX (512)
#define MDIRE_MAX (512)

#define MDIRE_IV_RSHTBIT (12)
#define MDIRE_IV_BITMASK (0x1ffUL)
#define MSA_PADR_LSHTBIT (12)
#define MDIRE_PADR_LSHTBIT (12)
#define IDIRE_IV_RSHTBIT (21)
#define IDIRE_IV_BITMASK (0x1ffUL)
#define IDIRE_PADR_LSHTBIT (12)
#define SDIRE_IV_RSHTBIT (30)
#define SDIRE_IV_BITMASK (0x1ffUL)
#define SDIRE_PADR_LSHTBIT (12)
#define TDIRE_IV_RSHTBIT (39)
#define TDIRE_IV_BITMASK (0x1ffUL)

#define PML4E_P (1 << 0)
#define PML4E_RW (1 << 1)
#define PML4E_US (1 << 2)
#define PML4E_PWT (1 << 3)
#define PML4E_PCD (1 << 4)

#define PDPTE_P (1 << 0)
#define PDPTE_RW (1 << 1)
#define PDPTE_US (1 << 2)
#define PDPTE_PWT (1 << 3)
#define PDPTE_PCD (1 << 4)
#define PDPTE_PS (1 << 7)

#define PDTE_P (1 << 0)
#define PDTE_RW (1 << 1)
#define PDTE_US (1 << 2)
#define PDTE_PWT (1 << 3)
#define PDTE_PCD (1 << 4)
#define PDTE_PS (1 << 7)

#define PTE_P (1 << 0)
#define PTE_RW (1 << 1)
#define PTE_US (1 << 2)
#define PTE_PWT (1 << 3)
#define PTE_PCD (1 << 4)
#define PTE_PAT (1 << 7)

#define PDPTEPHYADR_IP_LSHTBIT (12)
#define PDTEPHYADR_IP_LSHTBIT (12)
#define PTEPHYADR_IP_LSHTBIT (12)
#define PFMPHYADR_IP_LSHTBIT (12)
#define PTE_HAVE_MASK (~0xfff)
#define PDE_HAVE_MASK (~0xfff)
#define PDPTE_HAVE_MASK (~0xfff)
#define PML4E_HAVE_MASK (~0xfff)

// 页目录项
typedef struct MDIREFLAGS {
    u64_t m_p:1;    // 0
    u64_t m_rw:1;   // 1
    u64_t m_us:1;   // 2
    u64_t m_pwt:1;  // 3
    u64_t m_pcd:1;  // 4
    u64_t m_a:1;    // 5
    u64_t m_d:1;    // 6
    u64_t m_pat:1;  // 7
    u64_t m_g:1;    // 8
    u64_t m_ig1:3;  // 9\10\11
    u64_t m_msa:40; // 12
    u64_t m_ig2:11; // 52
    u64_t m_xd:1;   // 63
} __attribute__((packed)) mdireflags_t;

// 页目录项
typedef struct MDIRE {
    union {
        mdireflags_t m_flags;
        u64_t m_entry;
    } __attribute__((packed));
} __attribute__((packed)) mdire_t;

// 页目录指针项
typedef struct IDIREFLAGS {
    u64_t i_p:1;     // 0, 意为存在位. 若为1表示该页存在物理内存中, 若为0表示该表不在物理内存中
    u64_t i_rw:1;    // 1, 意为读写位. 若为1表示可读可写，若为0表示可读不可写
    u64_t i_us:1;    // 2, 意为普通用户/超级用户位. 若为1时，表示处于User级, 若为0时，表示处于Supervisor级，特权级别为3的程序不允许访问该页
    u64_t i_pwt:1;   // 3, 意为页级通写位，也称页级写透位. 若为1表示此项采用通用方式，表达该页不仅是普通内存，还是高速缓存, 这里直接置0就可以了
    u64_t i_pcd:1;   // 4, 意为页级高速缓存禁止位. 若为1表示该页启用高速缓存, 为0表示禁止该页缓存。这里将其置0
    u64_t i_a:1;     // 5, 意为访问位. 若为1表示该页被CPU访问过了，所以该位是由CPU设置的
    u64_t i_ig1:1;   // 6, 意为脏页位. 当CPU对一个页面执行写操作时，就会设置对应页表项的D位为1
    u64_t i_ps:1;    // 74kb==0。7, 意为页属性表位. 将此位置0即可
    u64_t i_ig2:4;   // 8\9\10\11, G, Global，意为全局位. AVL，意为Available位,表示可用
    u64_t i_mdir:40; // 12, 从13开始是物理页地址
    u64_t i_ig3:11;  // 52, 忽略
    u64_t i_xd:1;    // 63  XD，执行屏蔽位
} __attribute__((packed)) idireflags_t;

// 页目录指针项
typedef struct IDIRE {
    union {
        idireflags_t i_flags;
        u64_t i_entry;
    } __attribute__((packed));
} __attribute__((packed)) idire_t;

// 顶级目录项
typedef struct SDIREFLAGS {
    u64_t s_p:1;     // 0, 意为存在位. 若为1表示该页存在物理内存中, 若为0表示该表不在物理内存中
    u64_t s_rw:1;    // 1, 意为读写位. 若为1表示可读可写，若为0表示可读不可写
    u64_t s_us:1;    // 2, 意为普通用户/超级用户位. 若为1时，表示处于User级, 若为0时，表示处于Supervisor级，特权级别为3的程序不允许访问该页
    u64_t s_pwt:1;   // 3, 意为页级通写位，也称页级写透位. 若为1表示此项采用通用方式，表达该页不仅是普通内存，还是高速缓存, 这里直接置0就可以了
    u64_t s_pcd:1;   // 4, 意为页级高速缓存禁止位. 若为1表示该页启用高速缓存, 为0表示禁止该页缓存。这里将其置0
    u64_t s_a:1;     // 5, 意为访问位. 若为1表示该页被CPU访问过了，所以该位是由CPU设置的
    u64_t s_ig1:1;   // 6, 意为脏页位. 当CPU对一个页面执行写操作时，就会设置对应页表项的D位为1
    u64_t s_ps:1;    // 74kb==0, 7, 意为页属性表位. 将此位置0即可
    u64_t s_ig2:4;   // 8\9\10\11, G, Global，意为全局位. AVL，意为Available位,表示可用
    u64_t s_idir:40; // 12, 从13开始是物理页地址
    u64_t s_ig3:11;  // 52, 忽略
    u64_t s_xd:1;    // 63  XD，执行屏蔽位
} __attribute__((packed)) sdireflags_t;

// 顶级目录项
typedef struct SDIRE {
    union {
        sdireflags_t s_flags;
        u64_t s_entry;
    } __attribute__((packed));
} __attribute__((packed)) sdire_t;

// 63位的顶级页目录项
typedef struct TDIREFLAGS {
    u64_t t_p:1;     // 0, 意为存在位. 若为1表示该页存在物理内存中, 若为0表示该表不在物理内存中
    u64_t t_rw:1;    // 1, 意为读写位. 若为1表示可读可写，若为0表示可读不可写
    u64_t t_us:1;    // 2, 意为普通用户/超级用户位. 若为1时，表示处于User级, 若为0时，表示处于Supervisor级，特权级别为3的程序不允许访问该页
    u64_t t_pwt:1;   // 3, 意为页级通写位，也称页级写透位. 若为1表示此项采用通用方式，表达该页不仅是普通内存，还是高速缓存, 这里直接置0就可以了
    u64_t t_pcd:1;   // 4, 意为页级高速缓存禁止位. 若为1表示该页启用高速缓存, 为0表示禁止该页缓存。这里将其置0
    u64_t t_a:1;     // 5, 意为访问位. 若为1表示该页被CPU访问过了，所以该位是由CPU设置的
    u64_t t_ig1:1;   // 6, 意为脏页位. 当CPU对一个页面执行写操作时，就会设置对应页表项的D位为1
    u64_t t_rv1:1;   // 7, 意为页属性表位. 将此位置0即可
    u64_t t_ig2:4;   // 8\9\10\11, G, Global，意为全局位. AVL，意为Available位,表示可用
    u64_t t_sdir:40; // 12, 从13开始是物理页地址
    u64_t t_ig3:11;  // 52, 忽略
    u64_t t_xd:1;    // 63  XD，执行屏蔽位
} __attribute__((packed)) tdireflags_t;

// 页表项管理结构体
typedef struct TDIRE {
    union {
        tdireflags_t t_flags;
        u64_t t_entry;
    } __attribute__((packed));
} __attribute__((packed)) tdire_t;

// 页目录项管理结构体
typedef struct MDIREARR {
    mdire_t mde_arr[MDIRE_MAX];
} __attribute__((packed)) mdirearr_t;

// 页目录指针项管理结构体
typedef struct IDIREARR {
    idire_t ide_arr[IDIRE_MAX];
} __attribute__((packed)) idirearr_t;

// 顶级页目录项管理结构体。 2MB页表 顶级页目录项。内含512个顶级页目录项
typedef struct SDIREARR {
    sdire_t sde_arr[SDIRE_MAX];
} __attribute__((packed)) sdirearr_t;

// 页表项管理结构体
typedef struct TDIREARR {
    tdire_t tde_arr[TDIRE_MAX];
} __attribute__((packed)) tdirearr_t;

/**
 * 64位的CR3，cr3存储的是页表物理地址
 *  用cr3寄存器中的页表物理地址加上偏移量便是该页表的物理地址
 *  从该页表项中得到映射的物理页地址，然后用线性地址的低12位与该物理页地址相加，所得的地址之和便是最终要访问的物理地址
 */
typedef struct CR3SFLGS {
    u64_t c3s_pcid:12;  // 0
    u64_t c3s_plm4a:40; // 12, 写入物理地址的高20位
    u64_t c3s_rv:11;    // 52
    u64_t c3s_tbc:1;    // 63
} __attribute__((packed)) cr3sflgs_t;

// CR3 寄存器
typedef struct CR3S {
    union {
        cr3sflgs_t c3s_c3sflgs;
        u64_t c3s_entry;
    } __attribute__((packed));
} __attribute__((packed)) cr3s_t;

// MMU 管理结构体
typedef struct MMUDSC {
    spinlock_t mud_lock;
    u64_t mud_stus;
    u64_t mud_flag;
    tdirearr_t *mud_tdirearr;   // cr3的虚拟地址，页表项
    cr3s_t mud_cr3;             // cr3寄存器    
    list_h_t mud_tdirhead;
    list_h_t mud_sdirhead;
    list_h_t mud_idirhead;
    list_h_t mud_mdirhead;
    uint_t mud_tdirmsanr;
    uint_t mud_sdirmsanr;
    uint_t mud_idirmsanr;
    uint_t mud_mdirmsanr;
} mmudsc_t;

#endif