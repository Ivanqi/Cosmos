#ifndef _LDRTYPE_H
#define _LDRTYPE_H

#define BFH_RW_R 1
#define BFH_RW_W 2

#define BFH_BUF_SZ 0x1000   // 4096
#define BFH_ONERW_SZ 0x1000 // 4096
#define BFH_RWONE_OK 1
#define BFH_RWONE_ER 2
#define BFH_RWALL_OK 3

#define FHDSC_NMAX 192      // 文件名长度
#define FHDSC_SZMAX 256

#define MDC_ENDGIC 0xaaffaaffaaffaaff // 12321755120571230975UL
#define MDC_RVGIC 0xffaaffaaffaaffaa  // 18422818335611355050UL

#define REALDRV_PHYADR 0x1000         // initldrkrl映射文件具体拷贝内存地址
#define ILDRKRL_PHYADR 0x200000       // initldrkrl映射文件具体拷贝内存地址
#define IMGSHEL_PHYADR 0x30000        // 196608

#define IKSTACK_PHYADR (0x90000-0x10) // 栈顶地址
#define IKSTACK_SIZE 0x1000           // 栈大小是4KB

#define IMGFILE_PHYADR 0x4000000      // 映射文件物理地址
#define IMGKRNL_PHYADR 0x2000000      // 内核文件物理地址，Cosmos.eki的位置

#define KINITPAGE_PHYADR 0x1000000    // 顶级页目录
#define KINITFRVM_PHYADR 0x800000     // 8388608
#define KINITFRVM_SZ 0x400000

#define LDRFILEADR IMGFILE_PHYADR

#define MLOSDSC_OFF (0x1000)          // 4096
#define MRDDSC_ADR (mlosrddsc_t*)(LDRFILEADR + 0x1000)  // 0x4001000

#define KRNL_VIRTUAL_ADDRESS_START 0xffff800000000000   // 18446603336221196288UL

#define KPML4_P (1 << 0)    // 1
#define KPML4_RW (1 << 1)   // 2
#define KPML4_US (1 << 2)   // 4
#define KPML4_PWT (1 << 3)  // 8
#define KPML4_PCD (1 << 4)  // 16
#define KPML4_A (1 << 5)    // 32

#define KPDPTE_P (1 << 0)   // 1
#define KPDPTE_RW (1 << 1)  // 2
#define KPDPTE_US (1 << 2)  // 4
#define KPDPTE_PWT (1 << 3) // 8
#define KPDPTE_PCD (1 << 4) // 16
#define KPDPTE_A (1 << 5)   // 32

#define KPDE_P (1 << 0)     // 1
#define KPDE_RW (1 << 1)    // 2
#define KPDE_US (1 << 2)    // 4
#define KPDE_PWT (1 << 3)   // 8
#define KPDE_PCD (1 << 4)   // 16
#define KPDE_A (1 << 5)     // 32
#define KPDE_D (1 << 6)     // 64
#define KPDE_PS (1 << 7)    // 128
#define KPDE_G (1 << 8)     // 256
#define KPDE_PAT (1 << 12)  // 4096

#define KPML4_SHIFT 39
#define KPDPTTE_SHIFT 30
#define KPDP_SHIFT 21
#define PGENTY_SIZE 512

// 文件头描述符
typedef struct s_fhdsc
{
    u64_t fhd_type;             // 文件类型
    u64_t fhd_subtype;          // 文件子类型
    u64_t fhd_stuts;            // 文件状态
    u64_t fhd_id;               // 文件id
    u64_t fhd_intsfsoff;        // 文件在映像文件位置开始偏移
    u64_t fhd_intsfend;         // 文件在映像文件的结束偏移
    u64_t fhd_frealsz;          // 文件实际大小
    u64_t fhd_fsum;             // 文件校验和
    char fhd_name[FHDSC_NMAX];  // 文件名
} fhdsc_t;

// 映像文件头描述符
typedef struct s_mlosrddsc
{
    u64_t mdc_mgic;             // 映射文件标识
    u64_t mdc_sfsum;            // 未使用
    u64_t mdc_sfsoff;           // 未使用
    u64_t mdc_sfeoff;           // 未使用
    u64_t mdc_sfrlsz;           // 未使用
    u64_t mdc_ldrbk_s;          // 映像文件中二级引导器的开始偏移
    u64_t mdc_ldrbk_e;          // 映像文件中的二级引导器的结束偏移
    u64_t mdc_ldrbk_rsz;        // 映像文件中的二级引导器的实际大小
    u64_t mdc_ldrbk_sum;        // 映像文件中的二级引导器的校验和
    u64_t mdc_fhdbk_s;          // 映像文件中文件头描述的开始偏移
    u64_t mdc_fhdbk_e;          // 映像文件中文件头描述的结束偏移
    u64_t mdc_fhdbk_rsz;        // 映像文件中文件头描述的实际大小
    u64_t mdc_fhdbk_sum;        // 映像文件中文件头描述符的校验和
    u64_t mdc_filbk_s;          // 映像文件中文件数据的开始偏移
    u64_t mdc_filbk_e;          // 映像文件中文件数据的结束偏移
    u64_t mdc_filbk_rsz;        // 映像文件中文件数据的实际大小
    u64_t mdc_filbk_sum;        // 映像文件中文件数据的校验和
    u64_t mdc_ldrcodenr;        // 映像文件中二级引导器的文件头描述符的索引号
    u64_t mdc_fhdnr;            // 映像文件中文件头描述符有多少个
    u64_t mdc_filnr;            // 映像文件中文件有多少个
    u64_t mdc_endgic;           // 映像文件结束标识
    u64_t mdc_rv;               // 映像文件版本
} mlosrddsc_t;

#define RLINTNR(x) (x * 2)

typedef struct s_DPT
{
    u8_t dp_bmgic;
    u8_t dp_pshead;
    u8_t dp_pssector;
    u8_t dp_pscyder;
    u8_t dp_ptype;
    u8_t dp_pehead;
    u8_t dp_pesector;
    u8_t dp_pecyder;
    u32_t dp_pslba;
    u32_t dp_plbasz;
} __attribute__((packed)) dpt_t;

// 主引导分区
typedef struct s_MBR
{
    char_t mb_byte[446];
    dpt_t mb_part[4];
    u16_t mb_endmgic;
} __attribute__((packed)) mbr_t;

typedef struct s_EBR
{
    char_t eb_byte[446];
    dpt_t eb_part[4];
    u16_t eb_endmgic;
} __attribute__((packed)) ebr_t;

typedef struct s_IDTR
{
    u16_t idtlen;
    u32_t idtbas;
} idtr_t;

typedef struct s_RWHDPACK
{
    //hd_addr_packet:		db	0x10		; [ 0] Packet size in bytes.
      //          db	0		; [ 1] Reserved, must be 0.
    //hd_sect_nr:		db	0		; [ 2] Nr of blocks to transfer.
   //             db	0		; [ 3] Reserved, must be 0.
   //o_ffset:		dw	0		; [ 4] Addr of transfer - Offset
    //s_eg:			dw	0		; [ 6] buffer.          - Seg
    //lba_l:			dd	0		; [ 8] LBA. Low  32-bits.
    //lba_h:			dd	0		; [12] LBA. High 32-bits.
    u8_t rwhpk_sz;
    u8_t rwhpk_rv;
    u8_t rwhpk_sn;
    u8_t rwhpk_rv1;
    u16_t rwhpk_of;
    u16_t rwhpk_se;
    u32_t rwhpk_ll;
    u32_t rwhpk_lh; 
} __attribute__((packed)) rwhdpach_t;

// 内存类型
#define RAM_USABLE 1        // 可用内存
#define RAM_RESERV 2        // 保留内存不可使用
#define RAM_ACPIREC 3       // ACPI表相关
#define RAM_ACPINVS 4       // ACPI NVS空间
#define RAM_AREACON 5       // 包含坏内存

typedef struct s_e820 {
    u64_t saddr;            /* start of memory segment8，内存开始地址 */
    u64_t lsize;            /* size of memory segment8，内存大小 */
    u32_t type;             /* type of memory segment 4， 内存类型*/
} __attribute__((packed)) e820map_t;

typedef struct s_VBEINFO
{
    char vbesignature[4];
    u16_t vbeversion;
    u32_t oemstringptr;
    u32_t capabilities;
    u32_t videomodeptr;
    u16_t totalmemory;
    u16_t oemsoftwarerev;
    u32_t oemvendornameptr;
    u32_t oemproductnameptr;
    u32_t oemproductrevptr;
    u8_t reserved[222];
    u8_t oemdata[256];
} __attribute__((packed)) vbeinfo_t;

typedef struct s_VBEOMINFO
{
    u16_t ModeAttributes;
    u8_t  WinAAttributes;
    u8_t  WinBAttributes;
    u16_t WinGranularity;
    u16_t WinSize;
    u16_t WinASegment;
    u16_t WinBSegment;
    u32_t WinFuncPtr;
    u16_t BytesPerScanLine;
    u16_t XResolution;
    u16_t YResolution;
    u8_t  XCharSize;
    u8_t  YCharSize;
    u8_t  NumberOfPlanes;
    u8_t  BitsPerPixel;
    u8_t  NumberOfBanks;
    u8_t  MemoryModel;
    u8_t  BankSize;
    u8_t  NumberOfImagePages;
    u8_t  Reserved;
    u8_t  RedMaskSize;
    u8_t  RedFieldPosition;
    u8_t  GreenMaskSize;
    u8_t  GreenFieldPosition;
    u8_t  BlueMaskSize;
    u8_t  BlueFieldPosition;
    u8_t  RsvdMaskSize;
    u8_t  RsvdFieldPosition;
    u8_t  DirectColorModeInfo;
    u32_t PhysBasePtr;
    u32_t Reserved1;
    u16_t Reserved2;
    u16_t LinBytesPerScanLine;
    u8_t  BnkNumberOfImagePages;
    u8_t  LinNumberOfImagePages;
    u8_t  LinRedMaskSize;
    u8_t  LinRedFieldPosition;
    u8_t  LinGreenMaskSize;
    u8_t  LinGreenFieldPosition;
    u8_t  LinBlueMaskSize;
    u8_t  LinBlueFieldPosition;
    u8_t  LinRsvdMaskSize;
    u8_t  LinRsvdFieldPosition;
    u32_t MaxPixelClock;
    u8_t  Reserved3[189];
} __attribute__((packed)) vbeominfo_t; 

typedef struct s_PIXCL
{
    u8_t cl_b;  // 蓝
    u8_t cl_g;  // 绿
    u8_t cl_r;  // 红
    u8_t cl_a;  // 透明
} __attribute__((packed)) pixcl_t;

#define BGRA(r, g, b) ((0 | (r << 16) | (g << 8) | b))
// 通常情况下用pilx_t 和 BGRA宏
typedef u32_t pixl_t;

#define VBEMODE 1   // VBE模式
#define GPUMODE 2
#define BGAMODE 3

/**
 * 图形模式结构体
 */
typedef struct s_GRAPH
{
    u32_t gh_mode;              // 图形模式
    u32_t gh_x;                 // 水平像素点
    u32_t gh_y;                 // 垂直像素点
    u32_t gh_framphyadr;        // 显存物理地址
    u32_t gh_onepixbits;        // 一个像素字占用的数据位数
    u32_t gh_vbemodenr;         // vbe模式号
    u32_t gh_vifphyadr;         // vbe信息地址
    u32_t gh_vmifphyadr;        // 具体vbe信息地址
    u32_t gh_bank;              // 显存的bank数
    u32_t gh_curdipbnk;         // 当前bank
    u32_t gh_nextbnk;           // 下一个bank
    u32_t gh_banksz;            // bank大小
    u32_t gh_logophyadrs;
    u32_t gh_logophyadre;
    u32_t gh_fontadr;           // 字库地址
    u32_t gh_ftsectadr;
    u32_t gh_ftsectnr;
    u32_t gh_fnthight;
    u32_t gh_nxtcharsx;         // 下一字符显示的x坐标
    u32_t gh_nxtcharsy;         // 下一字符显示的y坐标
    u32_t gh_linesz;            // 字符行高
    vbeinfo_t gh_vbeinfo;
    vbeominfo_t gh_vminfo;
}__attribute__((packed)) graph_t;

typedef struct s_BMFHEAD
{
    u16_t bf_tag;   // 0x4d42
    u32_t bf_size;
    u16_t bf_resd1;
    u16_t bf_resd2;
    u32_t bf_off;
}__attribute__((packed)) bmfhead_t; // 14


typedef struct s_BITMINFO
{
    u32_t bi_size;
    s32_t bi_w;
    s32_t bi_h;
    u16_t bi_planes;
    u16_t bi_bcount;
    u32_t bi_comp;
    u32_t bi_szimg;
    s32_t bi_xpelsper;
    s32_t bi_ypelsper;
    u32_t bi_clruserd;
    u32_t bi_clrimport;
}__attribute__((packed)) bitminfo_t; // 40

typedef struct s_BMDBGR
{
    u8_t bmd_b;
    u8_t bmd_g;
    u8_t bmd_r;
}__attribute__((packed)) bmdbgr_t;

typedef struct s_UTF8
{
    u8_t utf_b1;
    u8_t utf_b2;
    u8_t utf_b3;
    u8_t utf_b4;
    u8_t utf_b5;
    u8_t utf_b6;
}__attribute__((packed)) utf8_t;

typedef struct s_FONTFHDER
{
    u8_t  fh_mgic[4];
    u32_t fh_fsize;
    u8_t  fh_sectnr;
    u8_t  fh_fhght;
    u16_t fh_wcpflg;
    u16_t fh_nchars;
    u8_t  fh_rev[2];
}__attribute__((packed)) fontfhder_t;

typedef struct s_FTSECTIF
{
    u16_t fsf_fistuc;
    u16_t fsf_lastuc;
    u32_t fsf_off;
} __attribute__((packed)) ftsectif_t;

typedef struct s_UFTINDX
{
    u32_t ui_choff:26;
    u32_t ui_chwith:6;
} __attribute__((packed)) uftindx_t;

#define MAX_CHARBITBY 32

typedef struct s_FNTDATA
{
    u8_t fntwxbyte;
    u8_t fnthx;
    u8_t fntwx;
    u8_t fntchmap[MAX_CHARBITBY];
} __attribute__((packed)) fntdata_t;

typedef struct s_KLFOCPYMBLK
{
    u64_t sphyadr;
    u64_t ocymsz;
} __attribute__((packed)) klfocpymblk_t;

#define MBS_MIGC (u64_t)((((u64_t)'L') << 56) | (((u64_t)'M') << 48) | (((u64_t)'O') << 40) | (((u64_t)'S') << 32) | (((u64_t)'M') << 24) | (((u64_t)'B') << 16) | (((u64_t)'S') << 8) | ((u64_t)'P'))

typedef struct s_MRSDP
{
    u64_t rp_sign;
    u8_t rp_chksum;
    u8_t rp_oemid[6];
    u8_t rp_revn;
    u32_t rp_rsdtphyadr;
    u32_t rp_len;
    u64_t rp_xsdtphyadr;
    u8_t rp_echksum;
    u8_t rp_resv[3];
} __attribute__((packed)) mrsdp_t;

// 二级引导器收集的信息
typedef struct s_MACHBSTART
{
    u64_t   mb_migc;            // LMOSMBSP//0
    u64_t   mb_chksum;          // 8
    u64_t   mb_krlinitstack;    // 16, 内核栈地址
    u64_t   mb_krlitstacksz;    // 24, 内核栈大小
    u64_t   mb_imgpadr;         // 操作系统映像
    u64_t   mb_imgsz;           // 操作系统映像大小
    u64_t   mb_krlimgpadr;      // 内核文件地址。内核区地址
    u64_t   mb_krlsz;
    u64_t   mb_krlvec;
    u64_t   mb_krlrunmode;
    u64_t   mb_kalldendpadr;    // e820内存长度
    u64_t   mb_ksepadrs;
    u64_t   mb_ksepadre;
    u64_t   mb_kservadrs;
    u64_t   mb_kservadre;
    u64_t   mb_nextwtpadr;      // 下一段空闲内存的首地址
    u64_t   mb_bfontpadr;       // 操作系统字体地址
    u64_t   mb_bfontsz;         // 操作系统字体大小
    u64_t   mb_fvrmphyadr;      // 机器显存地址
    u64_t   mb_fvrmsz;          // 机器显存大小
    u64_t   mb_cpumode;         // 机器CPU工作模式
    u64_t   mb_memsz;           // 机器内存大小
    u64_t   mb_e820padr;        // 机器e820数组地址
    u64_t   mb_e820nr;          // 机器e820数组元素个数
    u64_t   mb_e820sz;          // 机器e820数组大小
    u64_t   mb_e820expadr;      // e820map_t结构数组的首地址
    u64_t   mb_e820exnr;        // e820map_t数据数组元素个数
    u64_t   mb_e820exsz;        // e820map_t结构数据大小
    u64_t   mb_memznpadr;
    u64_t   mb_memznnr;
    u64_t   mb_memznsz;
    u64_t   mb_memznchksum;
    u64_t   mb_memmappadr;
    u64_t   mb_memmapnr;
    u64_t   mb_memmapsz;
    u64_t   mb_memmapchksum;
    u64_t   mb_pml4padr;        // 机器页表数据地址。页表地址
    u64_t   mb_subpageslen;     // 机器页表个数
    u64_t   mb_kpmapphymemsz;   // 操作系统映射空间大小
    u64_t   mb_ebdaphyadr;      // ACPI地址
    mrsdp_t mb_mrsdp;
    graph_t mb_ghparm;          // 图形信息
}__attribute__((packed)) machbstart_t;

#define MBSPADR ((machbstart_t*)(0x100000)) // 1048576，1M

#define VBE_DISPI_IOPORT_INDEX (0x01CE) // 462
#define VBE_DISPI_IOPORT_DATA (0x01CF)  // 462
#define VBE_DISPI_INDEX_ID (0)
#define VBE_DISPI_INDEX_XRES (1)
#define VBE_DISPI_INDEX_YRES (2)
#define VBE_DISPI_INDEX_BPP (3)
#define VBE_DISPI_INDEX_ENABLE (4)
#define VBE_DISPI_INDEX_BANK (5)
#define VBE_DISPI_INDEX_VIRT_WIDTH (6)
#define VBE_DISPI_INDEX_VIRT_HEIGHT (7)
#define VBE_DISPI_INDEX_X_OFFSET (8)
#define VBE_DISPI_INDEX_Y_OFFSET (9)

#define BGA_DEV_ID0 (0xB0C0) // 设置X和Y分辨率和位深度（仅8 BPP），倾斜模式
#define BGA_DEV_ID1 (0xB0C1) // 虚拟宽度和高度，X和Y偏移0
#define BGA_DEV_ID2 (0xB0C2) // 15, 16, 24 和32 BPP模式，支持线性帧缓冲，支持在模式切换时保留内存内容
#define BGA_DEV_ID3 (0xB0C3) // 支持获取功能，支持使用8位DAC
#define BGA_DEV_ID4 (0xB0C4) // VRAM增加到8MB
#define BGA_DEV_ID5 (0xB0C5) // VRAM增加到16MB[TODO:验证并检查其他更改]

#define VBE_DISPI_BPP_4 (0x04)  // 4
#define VBE_DISPI_BPP_8 (0x08)  // 8
#define VBE_DISPI_BPP_15 (0x0F) // 15
#define VBE_DISPI_BPP_16 (0x10) // 16
#define VBE_DISPI_BPP_24 (0x18) // 24
#define VBE_DISPI_BPP_32 (0x20) // 32
#define VBE_DISPI_DISABLED (0x00)   // 0
#define VBE_DISPI_ENABLED (0x01)    // 1
#define VBE_DISPI_LFB_ENABLED (0x40)    // 64

void REGCALL realadr_call_entry(u16_t callint,u16_t val1,u16_t val2);

#endif