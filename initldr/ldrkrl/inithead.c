#include "cmctl.h"

/**
 * 具体功能
 *  1. 初始化光标
 *  2. 关闭光标
 *  3. 清屏
 *  4. 调用 write_realintsvefile() 函数 读取 initldrsve.bin 文件到特定的内存地址空间中
 *  5. 调用 write_ldrkrlfile(）函数 读取 initldrkrl.bin文件到特定的内存地址空间中
 * 
 * grub会把映像文件eki加载到内存0x4000000（64MB）的位置上，映像文件头描述符的位置在0x4000000+0x1000 (64MB + 4KB）
 * 从头信息解析到initldrsve.bin和initldrkrl.bin这两个文件在内存中的位置
 * 然后再分别拷贝到0x1000（4KB，调用BIOS中断的代码）和0x200000（2MB，二级引导器的代码）的位置上
 */
void inithead_entry()
{
    init_curs();
    close_curs();
    clear_screen(VGADP_DFVL);

    write_realintsvefile();
    write_ldrkrlfile();

    return;
}

// 写initldrsve.bin文件到特定的内存中(0x1000, 4KB，调用BIOS中断的代码)
void write_realintsvefile()
{

    fhdsc_t *fhdscstart = find_file("initldrsve.bin");
    if (fhdscstart == NULL) {
        error("not file initldrsve.bin");
    }

    // 负责把映像文件复制到具体空间
    m2mcopy((void *)((u32_t)(fhdscstart->fhd_intsfsoff) + LDRFILEADR), (void *)REALDRV_PHYADR, (sint_t)fhdscstart->fhd_frealsz);
    return;
}

/**
 * 在映像文件中查找对应的文件
 * find_file 函数负责扫描映像文件中的文件头描述符，对比其中的文件名
 * 然后返回对应的文件头描述符的地址，这样就可以得到文件在映像文件中的位置和大小了
 * 
 * eki加载到了0x4001000的地方，这个文件前4kb的空间是引导程序 所以eki文件的管理头的数据就在0x4001000的地址上开始 
 */
fhdsc_t *find_file(char_t *fname)
{
    mlosrddsc_t *mrddadrs = MRDDSC_ADR; // 0x4001000
    if (mrddadrs->mdc_endgic != MDC_ENDGIC || mrddadrs->mdc_rv != MDC_RVGIC ||
        mrddadrs->mdc_fhdnr < 2 || mrddadrs->mdc_filnr < 2) {
        error("no mrddsc");
    }

    s64_t rethn = -1;
    fhdsc_t *fhdscstart = (fhdsc_t *)((u32_t)(mrddadrs->mdc_fhdbk_s) + LDRFILEADR);

    for (u64_t i = 0; i < mrddadrs->mdc_fhdnr; i++) {
        if (strcmpl(fname, fhdscstart[i].fhd_name) == 0) {
            rethn = (s64_t)i;
            goto ok_l;
        }
    }

    rethn = -1;

ok_l:
    if (rethn < 0) {
        error("not find file");
    }
    return &fhdscstart[rethn];
}

// 写initldrkrl.bin文件到特定的内存中(0x200000, 2MB，二级引导器的代码)
void write_ldrkrlfile()
{
    fhdsc_t *fhdscstart = find_file("initldrkrl.bin");

    if (fhdscstart == NULL) {
        error("not file initldrkrl.bin");
    }

    // 负责把映像文件复制到具体空间
    m2mcopy((void *)((u32_t)(fhdscstart->fhd_intsfsoff) + LDRFILEADR), (void *)ILDRKRL_PHYADR, (sint_t)fhdscstart->fhd_frealsz);
    
    return ;
}

void error(char_t *estr)
{
    kprint("INITLDR DIE ERROR:%s\n", estr);
    for (;;)
        ;
    
    return;
}

// 指针比较
int strcmpl(const char *a, const char *b)
{
    while (*b && *a && (*b == *a)) {
        b++;
        a++;
    }

    return *b - *a;
}
