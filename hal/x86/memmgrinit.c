/**********************************************************
    物理内存管理器初始化文件memmgrinit.c
***********************************************************/

#include "cosmostypes.h"
#include "cosmosmctrl.h"

// Cosmos物理内存管理器初始化
void init_memmgr()
{
	// 初始化内存页结构msadsc_t
	init_msadsc();
	// 初始化内存区结构memarea_t
	init_memarea();
	init_copy_pagesfvm();
	
	// 处理内存占用
	init_search_krloccupymm(&kmachbsp);

	// 合并内存页到内存区中
	init_merlove_mem();
	// 物理地址转虚拟地址，便于以后使用
	init_memmgrob();

	// 初始化kmsob
	init_kmsob();

	return;
}

void disp_memmgrob()
{

	test_divsion_pages();
	return;
}

// 初始化内存管理核心数据结构的地址和数量，并计算了一些统计信息/物理地址转为虚拟地址，便于以后使用
// 内存memmgrob_t被划分为多个功能分区，每个功能分区用一个memarea_t描述
void init_memmgrob()
{
	machbstart_t *mbsp = &kmachbsp;
	memmgrob_t *mobp = &memmgrob;
	memmgrob_t_init(mobp);
	if (NULL == mbsp->mb_e820expadr || 0 == mbsp->mb_e820exnr) {
		system_error("mbsp->mb_e820expadr==NULL\n");
	}

	if (NULL == mbsp->mb_memmappadr || 0 == mbsp->mb_memmapnr) {
		system_error("mbsp->mb_memmappadr==NULL\n");
	}

	if (NULL == mbsp->mb_memznpadr || 0 == mbsp->mb_memznnr) {
		system_error("mbsp->mb_memznpadr==NULL\n");
	}

	mobp->mo_pmagestat = (phymmarge_t *)phyadr_to_viradr((adr_t)mbsp->mb_e820expadr);
	mobp->mo_pmagenr = mbsp->mb_e820exnr;
	mobp->mo_msadscstat = (msadsc_t *)phyadr_to_viradr((adr_t)mbsp->mb_memmappadr);
	mobp->mo_msanr = mbsp->mb_memmapnr;
	mobp->mo_mareastat = (memarea_t *)phyadr_to_viradr((adr_t)mbsp->mb_memznpadr);
	mobp->mo_mareanr = mbsp->mb_memznnr;
	mobp->mo_memsz = mbsp->mb_memmapnr << PSHRSIZE;
	mobp->mo_maxpages = mbsp->mb_memmapnr;
	uint_t aidx = 0;

	for (uint_t i = 0; i < mobp->mo_msanr; i++) {
		if (1 == mobp->mo_msadscstat[i].md_indxflgs.mf_uindx && MF_MOCTY_KRNL == mobp->mo_msadscstat[i].md_indxflgs.mf_mocty &&
			PAF_ALLOC == mobp->mo_msadscstat[i].md_phyadrs.paf_alloc) {
			aidx++;
		}
	}

	mobp->mo_alocpages = aidx;
	mobp->mo_freepages = mobp->mo_maxpages - mobp->mo_alocpages;
	return;
}

void memmgrob_t_init(memmgrob_t *initp)
{
	list_init(&initp->mo_list);
	knl_spinlock_init(&initp->mo_lock);
	initp->mo_stus = 0;
	initp->mo_flgs = 0;
	initp->mo_memsz = 0;
	initp->mo_maxpages = 0;
	initp->mo_freepages = 0;
	initp->mo_alocpages = 0;
	initp->mo_resvpages = 0;
	initp->mo_horizline = 0;
	initp->mo_pmagestat = NULL;
	initp->mo_pmagenr = 0;
	initp->mo_msadscstat = NULL;
	initp->mo_msanr = 0;
	initp->mo_mareastat = NULL;
	initp->mo_mareanr = 0;
	initp->mo_privp = NULL;
	initp->mo_extp = NULL;
	return;
}

/**
 * @brief 页表设置(虚拟内存)
 * 
 * @param mbsp 二级信息引导结构体
 * @return bool_t 
 */
bool_t copy_pages_data(machbstart_t *mbsp)
{

	uint_t topgadr = mbsp->mb_nextwtpadr;

	if (initchkadr_is_ok(mbsp, topgadr, mbsp->mb_subpageslen) != 0) {
		return FALSE;
	}

	// 顶级页目录
	uint_t *p = (uint_t *)phyadr_to_viradr((adr_t)topgadr);
	kprint("topgadr:%x, p:%x\n", topgadr, p);
	// 页目录指针
	uint_t *pdpte = (uint_t *)(((uint_t)p) + 0x1000);
	// 页目录
	uint_t *pde = (uint_t *)(((uint_t)p) + 0x2000);

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
	uint_t adr = 0;
	uint_t pdepd = 0;
	for (uint_t pdei = 0; pdei < 16; pdei++) {
		pdepd = (uint_t)viradr_to_phyadr((adr_t)pde);
		pdpte[pdei] = (uint_t)(pdepd | KPDPTE_RW | KPDPTE_P);

		for (uint_t pdeii = 0; pdeii < PGENTY_SIZE; pdeii++) {
			pde[pdeii] = 0 | adr | KPDE_PS | KPDE_RW | KPDE_P;
			adr += 0x200000;
		}

		pde = (uint_t *)((uint_t)pde + 0x1000);
	}

	/**
     * 让顶级页目录中第0项和第((KRNL_VIRTUAL_ADDRESS_START) >> KPML4_SHIFT) & 0x1ff项，指向同一个页目录指针页
     * 这样的话就能让虚拟地址：0xffff800000000000 ~ 0xffff800400000000 和虚拟地址0 ～ 0x400000000，访问都同一个物理地址空间 0 ～ 0x400000000
     * 这样做是由目的，内核在启动初期，虚拟地址和物理地址要保持相同
     */
	uint_t pdptepd = (uint_t)viradr_to_phyadr((adr_t)pdpte);
	p[((KRNL_VIRTUAL_ADDRESS_START) >> KPML4_SHIFT) & 0x1ff] = (uint_t)(pdptepd | KPML4_RW | KPML4_P);
	p[0] = (uint_t)(pdptepd | KPML4_RW | KPML4_P);

	// 把页表首地址保存在机器信息结构中
	mbsp->mb_pml4padr = topgadr;
	mbsp->mb_subpageslen = (uint_t)(0x1000 * 16 + 0x2000);
	mbsp->mb_kpmapphymemsz = (uint_t)(0x400000000);
	mbsp->mb_nextwtpadr = PAGE_ALIGN(mbsp->mb_pml4padr + mbsp->mb_subpageslen);
	return TRUE;
}

/**
 * @brief 把显存虚拟地址复制到mb_nextwtpadr中
 * 
 * @param mbsp 二级信息引导结构体
 * @param dgp  图像结构体
 * @return bool_t 
 */
bool_t copy_fvm_data(machbstart_t *mbsp, dftgraph_t *dgp)
{
	u64_t tofvadr = mbsp->mb_nextwtpadr;
	if (initchkadr_is_ok(mbsp, tofvadr, dgp->gh_fvrmsz) != 0) {
		return FALSE;
	}

	sint_t retcl = m2mcopy((void *)((uint_t)dgp->gh_fvrmphyadr), (void *)phyadr_to_viradr((adr_t)(tofvadr)), (sint_t)dgp->gh_fvrmsz);
	if (retcl != (sint_t)dgp->gh_fvrmsz) {
		return FALSE;
	}

	dgp->gh_fvrmphyadr = phyadr_to_viradr((adr_t)tofvadr);
	mbsp->mb_fvrmphyadr = tofvadr;
	mbsp->mb_nextwtpadr = PAGE_ALIGN(tofvadr + dgp->gh_fvrmsz);
	return TRUE;
}

void memi_set_mmutabl(uint_t tblpadr, void *edatap)
{
	set_cr3(tblpadr);
	return;
}

void init_copy_pagesfvm()
{
	if (copy_pages_data(&kmachbsp) == FALSE) {
		system_error("copy_pages_data fail");
	}

	if (copy_fvm_data(&kmachbsp, &kdftgh) == FALSE) {
		system_error("copy_fvm_data fail");
	}
    
	memi_set_mmutabl(kmachbsp.mb_pml4padr, NULL);
	return;
}

/*void disp_msa(msadsc_t* p)
{
	kprint("msadsc_t sz:%d mf_alcidx:%d mf_pgemax:%d mf_lstty:%d\n",sizeof(*p),p->md_flgs.mf_alcidx,
		p->md_flgs.mf_pgemax,p->md_flgs.mf_lstty);	
	return;
}*/