/**********************************************************
	系统全局内存检查文件头chkcpmm_t.h
***********************************************************/

#ifndef _CHKCPMM_T_H
#define _CHKCPMM_T_H

#define EMAP_PTR E80MAP_ADR
#define EMAP_NR_PTR E80MAP_NR

#define BASE_MEM_SZ 0x3f80000   // 66584576

#define PML4T_BADR 0x20000      // 131072
#define PDPTE_BADR 0x21000      // 135168

#define PDE_BADR 0x22000        // 139264
#define PTE_BADR 0x23000        // 143360

#define PG_SIZE 512

#define PDT_S_PNT	0x1         // 1
#define PDT_S_RW	0x2         // 2
#define PDT_S_US	0x4         // 4
#define PDT_S_PWT	0x8         // 8
#define PDT_S_PCD	0x10        // 16
#define PDT_S_ACC	0x20        // 32
#define PDT_S_DITYRE    0x40    // 64
#define PDT_S_SIZE	0x80        // 128
#define PDT_S_GLOP	0x100       // 256

#define PT_S_PNT	0x1         // 1
#define PT_S_RW		0x2         // 2
#define PT_S_US		0x4         // 4
#define PT_S_PWT	0x8         // 8
#define PT_S_PCD	0x10        // 10
#define PT_S_ACC	0x20        // 32
#define PT_S_DITY	0x40        // 64
#define PT_S_PTARE	0x80        // 128
#define PT_S_GLOP	0x100       // 256

// 无符号长整型
typedef unsigned long long pt64_t;
#endif