/**********************************************************
	系统全局内存检查文件chkcpmm.c
***********************************************************/

#include "cmctl.h"

// acpi内存
unsigned int acpi_get_bios_ebda()
{
    unsigned int address = *(unsigned short *)0x40E;
    address <<= 4; // 0x40E0
    return address;
}

int acpi_checksum(unsigned char *ap, s32_t len)
{
    int sum = 0;
    while (len--) {
        sum += *ap++;
    }

    return sum & 0xFF;
}

mrsdp_t *acpi_rsdp_isok(mrsdp_t *rdp)
{
    if (rdp->rp_len == 0 || rdp->rp_revn == 0) {
        return NULL;
    }

    if (0 == acpi_checksum((unsigned char *)rdp, (s32_t)rdp->rp_len)) {
        return rdp;
    }

    return NULL;
}

mrsdp_t *findacpi_rsdp_core(void *findstart, u32_t findlen)
{
    if (NULL == findstart || 1024 > findlen) {
        return NULL;
    }

    u8_t *tmpdp = (u8_t *)findstart;

    mrsdp_t *retdrp = NULL;

    for (u64_t i = 0; i <= findlen; i++) {
        if (('R' == tmpdp[i]) && ('S' == tmpdp[i + 1]) && ('D' == tmpdp[i + 2]) && (' ' == tmpdp[i + 3]) 
            && ('P' == tmpdp[i + 4]) && ('T' == tmpdp[i + 5]) && ('R' == tmpdp[i + 6]) && (' ' == tmpdp[i + 7])) 
        {
            retdrp = acpi_rsdp_isok((mrsdp_t *)(&tmpdp[i]));
            if (NULL != retdrp) {
                return retdrp;
            }
        }
    }

    return NULL;
}

PUBLIC mrsdp_t *find_acpi_rsdp()
{
    void *fndp = (void *)acpi_get_bios_ebda();
    mrsdp_t *rdp = findacpi_rsdp_core(fndp, 1024);
    
    if (NULL != rdp) {
        return rdp;
    }

    //0E0000h和0FFFFFH
    fndp = (void *)(0xe0000);
    rdp = findacpi_rsdp_core(fndp, (0xfffff - 0xe0000));

    if (NULL != rdp) {
        return rdp;
    }

    return NULL;
}

// 初始化 acpi
PUBLIC void init_acpi(machbstart_t *mbsp)
{
    mrsdp_t *rdp = NULL;
    rdp = find_acpi_rsdp();

    if (NULL == rdp) {
        kerror("Your computer is not support ACPI!");
    }

    m2mcopy(rdp, &mbsp->mb_mrsdp, (sint_t)((sizeof(mrsdp_t))));

    if (acpi_rsdp_isok(&mbsp->mb_mrsdp) == NULL) {
        kerror("Your computer is not support ACPI!");
    }

    return;
}

/**
 * 获取内存布局信息
 *  1. 获取e820map_t结构体
 *  2. 检查内存大小
 */
void init_mem(machbstart_t *mbsp)
{
    e820map_t *retemp;
    u32_t retemnr = 0;

    mbsp->mb_ebdaphyadr = acpi_get_bios_ebda();
    /**
     * retemnr e820map_t内存数组个数 
     * retemp 内存首地址
     */
    mmap(&retemp, &retemnr);

    if (retemnr == 0) {
        kerror("no e820map\n");
    }

    // 根据e820map_t结构数据检查内存大小
    if (chk_memsize(retemp, retemnr, 0x100000, 0x8000000) == NULL) {
        kerror("Your computer is low on memory, the memory cannot be less than 128MB!");
    }

    mbsp->mb_e820expadr = (u64_t)((u32_t)(retemp));     // 把e820map_t结构数组的首地址传给mbsp->mb_e820padr
    mbsp->mb_e820nr = (u64_t)retemnr;                   // 把e820map_t数据数组元素个数传给mbsp->mb_e820nr
    mbsp->mb_e820sz = retemnr * (sizeof(e820map_t));    // 把e820map_t结构数据大小传给mbsp->mb_e820sz
    mbsp->mb_memsz = get_memsize(retemp, retemnr);      // 根据e820map_t结构数据计算内存大小
    init_acpi(mbsp);

    return;
}

/**
 * 检查CPU
 *  1. 通过eflags寄存器观察是否支持CPUID
 *  2. 检查CPU是否支持长模式
 */
void init_chkcpu(machbstart_t *mbsp)
{
    if (!chk_cpuid()) {
        kerror("Your CPU is not support CPUID sys is die!");
        CLI_HALT();
    }

    if (!chk_cpu_longmode()) {
        kerror("Your CPU is not support 64bits mode sys is die!");
        CLI_HALT();
    }

    // 如果成功则设置机器信息结构CPU模式为64位
    mbsp->mb_cpumode = 0x40;
    return;
}

// 初始化内核栈
void init_krlinitstack(machbstart_t *mbsp)
{
    // 检测0x8f000～（0x8f000+0x1001）与其他内存是否有冲突
    if (1 > move_krlimg(mbsp, (u64_t)(0x8f000), 0x1001)) {
        kerror("iks_moveimg err");
    }

    mbsp->mb_krlinitstack = IKSTACK_PHYADR; // 栈顶地址
    mbsp->mb_krlitstacksz = IKSTACK_SIZE;   // 栈大小是4kb

    return;
}

/**
 * 内核虚拟空间从0xffff800000000000开始，所以这个虚拟地址映射从物理地址0开始，大小都是0x400000000即16GB
 * 也就说是要虚拟地址空间：0xffff800000000000 ～ 0xffff800400000000 映射到物理地址空间 0 ~ 0x400000000
 * 
 * 长模式下的2MB分页方式
 */
void init_bstartpages(machbstart_t *mbsp)
{
    // 顶级页目录
    u64_t *p = (u64_t *)(KINITPAGE_PHYADR);                 // 16MB地址处
    // 页目录指针
    u64_t *pdpte = (u64_t *)(KINITPAGE_PHYADR + 0x1000);    // 4KB大小，其中各有512个条目，每个条目8字节64位大小
    // 页目录
    u64_t *pde = (u64_t *)(KINITPAGE_PHYADR + 0x2000);
    // 物理地址从0开始
    u64_t adr = 0;

    if (1 > move_krlimg(mbsp, (u64_t)(KINITFRVM_PHYADR), (0x1000 * 16 + 0x2000))) {
        kerror("move_krlimg err");
    }

    // 将顶级页目录，页目录指针的空间清0
    for (uint_t mi = 0; mi < PGENTY_SIZE; mi++) {
        p[mi] = 0;
        pdpte[mi] = 0;
    }

    /**
     * 映射
     *  映射的核心逻辑由两重循环控制，外层循环控制页目录指针顶，只有16项，其中每一项都指向一个页目录，每个页目录中有512个物理页地址
     * 
     *  物理地址每次增加2MB，由内层循环控制，每次执行一次外层循环就要执行512次内层循环
     */
    for (uint_t pdei = 0; pdei < 16; pdei++) {
        pdpte[pdei] = (u64_t)((u32_t)pde | KPDPTE_RW | KPDPTE_P);
        for (uint_t pdeii = 0; pdeii < PGENTY_SIZE; pdeii++) {
            // 大页KPDE_PS 2MB， 可读写KPDE_RW， 存在KPDE_P
            pde[pdeii] = 0 | adr | KPDE_PS | KPDE_RW | KPDE_P;
            adr += 0x200000;
        }
        pde = (u64_t *)((u32_t)pde + 0x1000);
    }

    /**
     * 让顶级页目录中第0项和第((KRNL_VIRTUAL_ADDRESS_START) >> KPML4_SHIFT) & 0x1ff项，指向同一个页目录指针页
     * 这样的话就能让虚拟地址：0xffff800000000000 ~ 0xffff800400000000 和虚拟地址0 ～ 0x400000000，访问都同一个物理地址空间 0 ～ 0x400000000
     * 这样做是由目的，内核在启动初期，虚拟地址和物理地址要保持相同
     */
    p[((KRNL_VIRTUAL_ADDRESS_START) >> KPML4_SHIFT) & 0x1ff] = (u64_t)((u32_t)pdpte | KPML4_RW | KPML4_P);
    p[0] = (u64_t)((u32_t)pdpte | KPML4_RW | KPML4_P);

    // 把页表首地址保存在机器信息结构中
    mbsp->mb_pml4padr = (u64_t)(KINITPAGE_PHYADR);
    mbsp->mb_subpageslen = (u64_t)(0x1000 * 16 + 0x2000);
    mbsp->mb_kpmapphymemsz = (u64_t)(0x400000000);

    return;
}

void init_meme820(machbstart_t *mbsp)
{
    e820map_t *semp = (e820map_t *)((u32_t)(mbsp->mb_e820padr));
    u64_t senr = mbsp->mb_e820nr;
    e820map_t *demp = (e820map_t *)((u32_t)(mbsp->mb_nextwtpadr));

    if (1 > move_krlimg(mbsp, (u64_t)((u32_t)demp), (senr * (sizeof(e820map_t))))) {
        kerror("move_krlimg err");
    }

    m2mcopy(semp, demp, (sint_t)(senr * (sizeof(e820map_t))));
    mbsp->mb_e820padr = (u64_t)((u32_t)(demp));
    mbsp->mb_e820sz = senr * (sizeof(e820map_t));
    mbsp->mb_nextwtpadr = mbsp->mb_e820exnr + mbsp->mb_e820sz;
    return;
}

/**
 * 通过调用realadr_call_entry函数，来调用实模式下的_getmmap函数，并且在_getmmap函数中调用BIOS中断
 */
void mmap(e820map_t **retemp, u32_t *retemnr)
{
    realadr_call_entry(RLINTNR(0), 0, 0);
    *retemnr = *((u32_t *)(E80MAP_NR));
    *retemp = (e820map_t *)(*((u32_t *)(E80MAP_ADRADR)));
    return ;
}

// 检查可用内存大小
e820map_t *chk_memsize(e820map_t *e8p, u32_t enr, u64_t sadr, u64_t size)
{
    u64_t len = sadr + size;
    if (enr == 0 || e8p == NULL) {
        return NULL;
    }

    for (u32_t i = 0; i < enr; i++) {
        if (e8p[i].type == RAM_USABLE) {
            if ((sadr >= e8p[i].saddr) && (len < (e8p[i].saddr + e8p[i].lsize))) {
                return &e8p[i];
            }
        }
    }

    return NULL;
}

/**
 * 计算可用内存大小
 */
u64_t get_memsize(e820map_t *e8p, u32_t enr)
{
    u64_t len = 0;
    if (enr == 0 || e8p == NULL) {
        return 0;
    }

    for (u32_t i = 0; i < enr; i++) {
        if (e8p[i].type == RAM_USABLE) {
            len += e8p[i].lsize;
        }
    }

    return len;
}

// 通过改写Eflags寄存器的第21位，观察其位的变化判断是否支持CPUID
int chk_cpuid()
{
    int rets = 0;
    __asm__ __volatile__(
        "pushfl \n\t"                   // pushf 的功能是将标志寄存器的值压栈
        "popl %%eax \n\t"
        "movl %%eax,%%ebx \n\t"
        "xorl $0x0200000,%%eax \n\t"    // xorl: 按位异或, 将把a和b中一个为1另一个为0的位设为 1 . 0x0200000: 0010 0000 | 0000 0000 0000 0000
        "pushl %%eax \n\t"              // 暂存eax，同时存储eflags的信息

        "popfl \n\t"                    // popf 是从栈中探出标志寄存器
        "pushfl \n\t"

        "popl %%eax \n\t"
        "xorl %%ebx,%%eax \n\t"         // 两个值xor
        "jz 1f \n\t"                    // jz为0则跳转
        "movl $1,%0 \n\t"               // 成功返回1
        "jmp 2f \n\t"
        "1: movl $0,%0 \n\t"
        "2: \n\t"
        : "=c"(rets)
        :
        :
    );
    return rets;
}

// 检查CPU是否支持长模式
int chk_cpu_longmode()
{
    int rets = 0;
    __asm__ __volatile__(
        "movl $0x80000000,%%eax \n\t"
        "cpuid \n\t"                        // 把eax中放入0x80000000调用CPUID指令
        "cmpl $0x80000001,%%eax \n\t"       // 看eax中返回结果
        "setnb %%al \n\t"                   // 不为0x80000001，则不支持0x80000001号功能。setnb: ~CF
        "jb 1f \n\t"                        // JB表示无符号小于则跳转，CF=1 且ZF=0 即A<B转移
        "movl $0x80000001,%%eax \n\t"
        "cpuid \n\t"                        // 把eax中放入0x80000001调用CPUID指令，检查edx中的返回数据
        "bt $29,%%edx  \n\t"                // bt 表示 Bit Test，测试并用原值设置进位值.长模式 支持位 是否为1
        "setcb %%al \n\t"                   // CF=1
        "1: \n\t"
        "movzx %%al,%%eax \n\t"
        : "=a"(rets)
        :
        :
    );
    return rets;
}

void init_chkmm()
{
    e820map_t *map = (e820map_t *)EMAP_PTR;
    u16_t *map_nr = (u16_t *)EMAP_NR_PTR;
    u64_t mmsz = 0;

    for (int j = 0; j < (*map_nr); j++) {
        if (map->type == RAM_USABLE) {
            mmsz += map->lsize;
        }
        map++;
    }

    if (mmsz < BASE_MEM_SZ) {
        kprint("Your computer is low on memory, the memory cannot be less than 64MB!");
        CLI_HALT();
    }

    if (!chk_cpuid()) {
        kprint("Your CPU is not support CPUID sys is die!");
        CLI_HALT();
    }

    if (!chk_cpu_longmode()) {
        kprint("Your CPU is not support 64bits mode sys is die!");
        CLI_HALT();
    }

    ldr_createpage_and_open();
    return;
}

void out_char(char *c)
{
    char *str = c, *p = (char *)0xb8000;
    while (*str) {
        *p = *str;
        p += 2;
        str++;
    }

    return;
}

void init_bstartpagesold(machbstart_t *mbsp)
{
    if (1 > move_krlimg(mbsp, (u64_t)(PML4T_BADR), 0x3000)) {
        kerror("ip_moveing err");
    }

    pt64_t *pml4p = (pt64_t *)PML4T_BADR, *pdptp = (pt64_t *) PDPTE_BADR, *pdep = (pt64_t *)PDE_BADR;
    for (int pi = 0; pi < PG_SIZE; pi++) {
        pml4p[pi] = 0;
        pdptp[pi] = 0;

        pdep[pi] = 0;
    }

    pml4p[0] = 0 | PDPTE_BADR | PDT_S_RW | PDT_S_PNT;
    pdptp[0] = 0 | PDE_BADR | PDT_S_RW | PDT_S_PNT;
    pml4p[256] = 0 | PDPTE_BADR | PDT_S_RW | PDT_S_PNT;

    pt64_t tmpba = 0, tmpbd = 0 | PDT_S_SIZE | PDT_S_RW | PDT_S_PNT;

    for (int di = 0; di < PG_SIZE; di++) {
        pdep[di] = tmpbd;
        tmpba += 0x200000;
        tmpbd = tmpba | PDT_S_SIZE | PDT_S_RW | PDT_S_PNT;
    }

    mbsp->mb_pml4padr = (u64_t)((u32_t)pml4p);
    mbsp->mb_subpageslen = 0x3000;
    mbsp->mb_kpmapphymemsz = (0x200000 * 512);

    return;
}

void ldr_createpage_and_open()
{
    pt64_t *pml4p = (pt64_t *)PML4T_BADR, *pdptp = (pt64_t *)PDPTE_BADR, *pdep = (pt64_t *)PDE_BADR;
    
    for (int pi = 0; pi < PG_SIZE; pi++) {
        pml4p[pi] = 0;
        pdptp[pi] = 0;
        pdep[pi] = 0;
    }

    pml4p[0] = 0 | PDPTE_BADR | PDT_S_RW | PDT_S_PNT;
    pdptp[0] = 0 | PDE_BADR | PDT_S_RW | PDT_S_PNT;

    pml4p[256] = 0 | PDPTE_BADR | PDT_S_RW | PDT_S_PNT;

    pt64_t tmpba = 0, tmpbd = 0 | PDT_S_SIZE | PDT_S_RW | PDT_S_PNT;

    for (int di = 0; di < PG_SIZE; di++) {
        pdep[di] = tmpbd;
        tmpba += 0x200000;
        tmpbd = tmpba | PDT_S_SIZE | PDT_S_RW | PDT_S_PNT;
    }

    return;
}