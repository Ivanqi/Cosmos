/**********************************************************
    平台相关的文件halplatform.c
***********************************************************/

#include "cosmostypes.h"
#include "cosmosmctrl.h"

// 虚拟地址转物理地址
adr_t viradr_to_phyadr(adr_t kviradr)
{
    if (kviradr < KRNL_MAP_VIRTADDRESS_START || kviradr > KRNL_MAP_VIRTADDRESS_END) {
        system_error("virtadr_to_phyadr err\n");
        return KRNL_ADDR_ERROR;
    }

    return kviradr - KRNL_MAP_VIRTADDRESS_START;
}

// 物理地址转虚拟地址
adr_t phyadr_to_viradr(adr_t kphyadr)
{
    if (kphyadr >= KRNL_MAP_PHYADDRESS_END) {
        system_error("phyadr_to_viradr err\n");
        return KRNL_ADDR_ERROR;
    }

    return kphyadr + KRNL_MAP_VIRTADDRESS_START;
}

// 初始化machbstart_t结构体
void machbstart_t_init(machbstart_t *initp)
{
    // 清零
    memset(initp, 0, sizeof(machbstart_t));
    return;
}

/**
 * @brief 把二级引导器建立的机器信息结构复制到hal层中的一个全局变量中
 *  1. 主要是把二级引导器建立的机器信息结构，复制到了hal层一份给内核使用，同时也为释放二级引导器占用的内存做好准备
 *  2. 其做法就是拷贝一份mbsp到kmbsp，其中用到了虚拟地址转换hyadr_to_viradr
 */
void init_machbstart()
{
    machbstart_t *kmbsp = &kmachbsp;
    machbstart_t *smbsp = MBSPADR;      // 物理地址1MB处

    machbstart_t_init(kmbsp);
    // 复制，要把地址转换成虚拟地址
    // 进入到分页的保护模式，需按生成的页表将目标物理地址转换成对应的虚拟地址
    memcopy((void *)phyadr_to_viradr((adr_t)smbsp), (void *)kmbsp, sizeof(machbstart_t));
    return;
}

/**
 * @brief 初始化平台
 *  1. 把二级引导器建立的机器信息结构复制到hal层中的一个全局变量中，方便内核中的其他代码使用里面的信息，之后二级引导器的数据所占用的内存都会被释放
 *  2. 初始化图形显示驱动，内核在在运行过程要在屏幕上输出信息
 */
void init_halplaltform()
{
    // 复制机器信息结构
    init_machbstart();
    // 初始化图形显示驱动
    init_bdvideo();
    return;
}

int strcmpl(const char *a, const char *b)
{
    while (*b && *a && (*b == *a)) {
        b++;
        a++;
    }

    return *b - *a;
}

fhdsc_t *get_fileinfo(char_t *fname, machbstart_t *mbsp)
{
    mlosrddsc_t *mrddadrs = (mlosrddsc_t *)phyadr_to_viradr((adr_t)(mbsp->mb_imgpadr + MLOSDSC_OFF));
    if (mrddadrs->mdc_endgic != MDC_ENDGIC || mrddadrs->mdc_rv != MDC_RVGIC || mrddadrs->mdc_fhdnr < 2 || mrddadrs->mdc_filnr < 2) {
        system_error("no mrddsc");
    }

    s64_t rethn = -1;
    fhdsc_t *fhdscstart = (fhdsc_t *)((uint_t)((mrddadrs->mdc_fhdbk_s) + (phyadr_to_viradr((adr_t)mbsp->mb_imgpadr))));

    for (u64_t i = 0; i < mrddadrs->mdc_fhdnr; i++) {
        if (strcmpl(fname, fhdscstart[i].fhd_name) == 0) {
            rethn = (s64_t)i;
            goto ok_l;
        }
    }
    rethn = -1;

ok_l:
    if (rethn < 0) {
        system_error("not find file");
    }

    return &fhdscstart[rethn];
}

void get_file_rvadrandsz(char_t *fname, machbstart_t *mbsp, u64_t *retadr, u64_t *retsz)
{
    u64_t padr = 0, fsz = 0;
    if (NULL == fname || NULL == mbsp) {
        *retadr = 0;
        return;
    }

    fhdsc_t *fhdsc = get_fileinfo(fname, mbsp);
    if (fhdsc == NULL) {
        *retadr = 0;
        return;
    }

    padr = fhdsc->fhd_intsfsoff + phyadr_to_viradr((adr_t)mbsp->mb_imgpadr);
    fsz = fhdsc->fhd_frealsz;

    *retadr = padr;
    *retsz = fsz;
    return;
}

e820map_t *get_maxmappadr_e820map(machbstart_t *mbsp, u64_t mappadr)
{
    if (NULL == mbsp) {
        return NULL;
    }

    u64_t enr = mbsp->mb_e820nr;
    e820map_t *emp = (e820map_t *)phyadr_to_viradr((adr_t)mbsp->mb_e820padr);
    e820map_t *retemp = NULL;
    u64_t maxadr = emp[0].saddr;

    for (u64_t i = 0; i < enr; i++) {
        if (emp[i].type == RAM_USABLE) {
            if (emp[i].saddr > maxadr && (mappadr > (emp[i].saddr + emp[i].lsize))) {
                maxadr = emp[i].saddr;
                retemp = &emp[i];
            }
        }
    }

    return retemp;
}

/**
 * @brief e820内存数组校验
 * 
 * @param mbsp 
 * @param mappadr 
 * @param cpsz 
 * @return e820map_t* 
 */
e820map_t *ret_kmaxmpadrcmpsz_e820map(machbstart_t *mbsp, u64_t mappadr, u64_t cpsz)
{
    if (NULL == mbsp) {
        return NULL;
    }

    u64_t enr = mbsp->mb_e820nr;                                                // e820 数组个数
    e820map_t *emp = (e820map_t *)phyadr_to_viradr((adr_t)mbsp->mb_e820padr);   // e820 地址

    u64_t maxadr = emp[0].saddr;

    e820map_t *retemp = NULL;

    // 遍历e820内存数组
    for (u64_t i = 0; i < enr; i++) {
        if (emp[i].type == RAM_USABLE) {
            if (emp[i].saddr >= maxadr                      // 内存区首地址大于已知最大区域起始地址（初始化位第一个区首地址
            && (mappadr > (emp[i].saddr + emp[i].lsize))    // 内存区尾地址小于内存映射最大地址
            && (emp[i].lsize >= cpsz))                      // 内存区大小大于镜像文件大小
            {
                maxadr = emp[i].saddr;                      // 已知最大区域起始地址
                retemp = &emp[i];                           // 更新最后满足条件内存区域
            }
        }
    }

    // 校验，但除非一个都不满足条件
    if ((mappadr > (retemp->saddr + retemp->lsize)) && (retemp->lsize >= cpsz)) {
        return retemp;
    }

    return NULL;
}

/**
 * @brief 将映像文件移动到最大地址
 * 
 * @param mbsp 二级引导器结构体
 */
void move_img2maxpadr(machbstart_t *mbsp)
{
    u64_t kmapadrend = mbsp->mb_kpmapphymemsz;  // 操作系统映射空间大小
    // 检查映射文件内存
    e820map_t *emp = ret_kmaxmpadrcmpsz_e820map(mbsp, kmapadrend, mbsp->mb_imgsz);
    if (NULL == emp) {
        system_error("move_img2maxpadr1 emp not ok");
    }

    // 新的操作系统映像地址[在操作系统映射空间中]
    u64_t imgtoadr = (emp->saddr + (emp->lsize - mbsp->mb_imgsz));
    imgtoadr &= ~(0xfffUL);
    if (initchkadr_is_ok(mbsp, imgtoadr, mbsp->mb_imgsz) != 0) {
        system_error("initchkadr_is_ok not ok\n");
    }

    void *sadr = (void *)phyadr_to_viradr((adr_t)mbsp->mb_imgpadr);
    void *dadr = (void *)phyadr_to_viradr((adr_t)imgtoadr);
    
    // 复制内容到新的操作系统映像地址
    if (m2mcopy(sadr, dadr, (sint_t)(mbsp->mb_imgsz)) != ((sint_t)(mbsp->mb_imgsz))) {
        system_error("move_img2maxpadr1 m2mcopy not ok");
    }

    mbsp->mb_imgpadr = imgtoadr;
    return;
}

/**
 * @brief 内存监测
 *  1. kadr 大于等于 sadr 且 kadr 小于 (sadr + slen)。意思是kadr 在 (sadr + slen) 之内返回-1
 *  2. kadr 小于等于 sadr 且 (kadr + klen) 大于等于 sadr. kadr 小于但 kadr + klen 大于 sadr 返回-2
 *  3. kadr 大于 sadr 或者 kadr 小于 sadr 返回 0。意味这两个内存区不交集
 * 
 * @param sadr 
 * @param slen 
 * @param kadr 
 * @param klen 
 * @return int 
 */
int adrzone_is_ok(u64_t sadr, u64_t slen, u64_t kadr, u64_t klen)
{
    if (kadr >= sadr && kadr <= (sadr + slen)) {
        return -1;
    }

    if (kadr <= sadr && ((kadr + klen) >= sadr)){
        return -2;
    }

    return 0;
}

// 两个内存区不交集
int initchkadr_is_ok(machbstart_t *mbsp, u64_t chkadr, u64_t cksz)
{
    //u64_t len=chkadr+cksz;
    if (adrzone_is_ok((mbsp->mb_krlinitstack - mbsp->mb_krlitstacksz), mbsp->mb_krlitstacksz, chkadr, cksz) != 0) {
        return -1;
    }

    if (adrzone_is_ok(mbsp->mb_imgpadr, mbsp->mb_imgsz, chkadr, cksz) != 0) {
        return -2;
    }

    if (adrzone_is_ok(mbsp->mb_krlimgpadr, mbsp->mb_krlsz, chkadr, cksz) != 0) {
        return -3;
    }

    if (adrzone_is_ok(mbsp->mb_bfontpadr, mbsp->mb_bfontsz, chkadr, cksz) != 0) {
        return -4;
    }

    if (adrzone_is_ok(mbsp->mb_e820padr, mbsp->mb_e820sz, chkadr, cksz) != 0) {
        return -5;
    }

    if (adrzone_is_ok(mbsp->mb_memznpadr, mbsp->mb_memznsz, chkadr, cksz) != 0) {
        return -6;
    }

    if (adrzone_is_ok(mbsp->mb_memmappadr, mbsp->mb_memmapsz, chkadr, cksz) != 0) {
        return -7;
    }

    if (adrzone_is_ok(mbsp->mb_e820expadr, mbsp->mb_e820exsz, chkadr, cksz) != 0) {
        return -8;
    }

    // chkadr地址大于 操作系统映射空间返回-9
    if ((chkadr + cksz) >= mbsp->mb_kpmapphymemsz) {
        return -9;
    }

    return 0;
}

#pragma GCC push_options
#pragma GCC optimize("O0")
void die(u32_t dt)
{

    u32_t dttt = dt, dtt = dt;
    if (dt == 0) {
        for (;;)
            ;
    }

    for (u32_t i = 0; i < dt; i++) {
        for (u32_t j = 0; j < dtt; j++) {
            for (u32_t k = 0; k < dttt; k++) {
                ;
            }
        }
    }

    return;
}

#pragma GCC pop_options