/**********************************************************
    内核内存空间对象分配释放文件kmsob.c
***********************************************************/

#include "cosmostypes.h"
#include "cosmosmctrl.h"

KLINE sint_t retn_mscidx(uint_t pages)
{
	sint_t pbits = search_64rlbits((uint_t)pages) - 1;
	if (pages & (pages - 1)) {
		pbits++;
	}
	return pbits;
}

void msclst_t_init(msclst_t *initp, uint_t pnr)
{
	initp->ml_msanr = 0;
	initp->ml_ompnr = pnr;
	list_init(&initp->ml_list);
	return;
}

void msomdc_t_init(msomdc_t *initp)
{
	for (uint_t i = 0; i < MSCLST_MAX; i++) {
		msclst_t_init(&initp->mc_lst[i], 1UL << i);
	}

	initp->mc_msanr = 0;
	list_init(&initp->mc_list);
	list_init(&initp->mc_kmobinlst);
	initp->mc_kmobinpnr = 0;
	return;
}

// 初始化内存对象数据结构
void freobjh_t_init(freobjh_t *initp, uint_t stus, void *stat)
{
	list_init(&initp->oh_list);
	initp->oh_stus = stus;
	initp->oh_stat = stat;
	return;
}

// 初始化内存对象容器数据结构
void kmsob_t_init(kmsob_t *initp)
{
	list_init(&initp->so_list);
	knl_spinlock_init(&initp->so_lock);
	initp->so_stus = 0;
	initp->so_flgs = 0;
	initp->so_vstat = NULL;
	initp->so_vend = NULL;
	initp->so_objsz = 0;
	initp->so_objrelsz = 0;
	initp->so_mobjnr = 0;
	initp->so_fobjnr = 0;
	list_init(&initp->so_frelst);
	list_init(&initp->so_alclst);
	list_init(&initp->so_mextlst);
	initp->so_mextnr = 0;
	msomdc_t_init(&initp->so_mc);
	initp->so_privp = NULL;
	initp->so_extdp = NULL;
	return;
}

// 初始化kmbnext_t结构
void kmbext_t_init(kmbext_t *initp, adr_t vstat, adr_t vend, kmsob_t *kmsp)
{
	list_init(&initp->mt_list);
	initp->mt_vstat = vstat;
	initp->mt_vend = vend;
	initp->mt_kmsb = kmsp;
	initp->mt_mobjnr = 0;
	return;
}

// 初始化koblst_t结构体
void koblst_t_init(koblst_t *initp, size_t koblsz)
{
	list_init(&initp->ol_emplst);
	initp->ol_cahe = NULL;
	initp->ol_emnr = 0;
	initp->ol_sz = koblsz;
	return;
}

// 初始化kmsobmgrhed_t结构体
void kmsobmgrhed_t_init(kmsobmgrhed_t *initp)
{
	size_t koblsz = 32;
	knl_spinlock_init(&initp->ks_lock);
	list_init(&initp->ks_tclst);
	initp->ks_tcnr = 0;
	initp->ks_msobnr = 0;
	initp->ks_msobche = NULL;

	// 这里并不是按照开始的图形分类的而是每次增加32字节，所以是32，64,96,128,160,192,224，256，.......
	for (uint_t i = 0; i < KOBLST_MAX; i++) {
		koblst_t_init(&initp->ks_msoblst[i], koblsz);
		koblsz += 32;
	}

	return;
}

void disp_kmsobmgr()
{
	kmsobmgrhed_t *kmmp = &memmgrob.mo_kmsobmgr;
	for (uint_t i = 0; i < KOBLST_MAX; i++) {
		kprint("koblst_t.ol_sz:%d\n", kmmp->ks_msoblst[i].ol_sz);
	}

	return;
}

void disp_kmsob(kmsob_t *kmsp)
{
	kprint("kmsob_t so_vstat:%x so_vend:%x so_objsz:%d so_fobjnr:%d so_mobjnr:%d\n", kmsp->so_vstat, kmsp->so_vend, kmsp->so_objsz,
		kmsp->so_fobjnr, kmsp->so_mobjnr);

	return;
}

// 初始化kmsob
void init_kmsob()
{
	kmsobmgrhed_t_init(&memmgrob.mo_kmsobmgr);
	return;
}

// 更新kmsobmgrhed_t结构的信息
void kmsob_updata_cache(kmsobmgrhed_t *kmobmgrp, koblst_t *koblp, kmsob_t *kmsp, uint_t flgs)
{
	if (KUC_NEWFLG == flgs) {
		kmobmgrp->ks_msobche = kmsp;
		koblp->ol_cahe = kmsp;
		return;
	}

	if (KUC_DELFLG == flgs) {
		kmobmgrp->ks_msobche = kmsp;
		koblp->ol_cahe = kmsp;
		return;
	}

	if (KUC_DSYFLG == flgs) {
		if (kmsp == kmobmgrp->ks_msobche) {
			kmobmgrp->ks_msobche = NULL;
		}

		if (kmsp == koblp->ol_cahe) {
			koblp->ol_cahe = NULL;
		}

		return;
	}

	return;
}

// 看看内存对象容器是不是合乎要求
kmsob_t *scan_newkmsob_isok(kmsob_t *kmsp, size_t msz)
{
	// 只要内存对象大小小于等于内存对象容器的对象大小就行
	if (NULL == kmsp || 1 > msz) {
		return NULL;
	}

	if (msz <= kmsp->so_objsz) {
		return kmsp;
	}

	return NULL;
}

// 检查释放的内存对象是不是在kmsob_t结构中
kmsob_t *scan_delkmsob_isok(kmsob_t *kmsp, void *fadrs, size_t fsz)
{
	// 检查释放内存对象的地址是否落在kmsob_t结构的地址区间
	if (NULL == kmsp || NULL == fadrs || 1 > fsz) {
		return NULL;
	}

	if ((adr_t)fadrs >= (kmsp->so_vstat + sizeof(kmsob_t)) && ((adr_t)fadrs + (adr_t)fsz) <= kmsp->so_vend) {
		if (fsz <= kmsp->so_objsz) {
			return kmsp;
		}
	}

	// 如果kmsob_t结构没有扩展空降，直接返回
	if (1 > kmsp->so_mextnr) {
		return NULL;
	}

	kmbext_t *bexp = NULL;
	list_h_t *tmplst = NULL;

	// 遍历kmsob_t结构中每个扩展空间
	list_for_each(tmplst, &kmsp->so_mextlst) {
		bexp = list_entry(tmplst, kmbext_t, mt_list);
	
		if (bexp->mt_kmsb != kmsp) {
			system_error("scan_delkmsob_isok err\n");
		}

		// 检查释放内存对象的地址是否落在扩展空间的地址区间
		if ((adr_t)fadrs >= (bexp->mt_vstat + sizeof(kmbext_t)) && ((adr_t)fadrs + (adr_t)fsz) <= bexp->mt_vend) {
			// 同样的要检查大小
			if (fsz <= kmsp->so_objsz) {
				return kmsp;
			}
		}
	}

	return NULL;
}

bool_t scan_nmszkmsob_isok(kmsob_t *kmsp, size_t msz)
{
	if (NULL == kmsp || 1 > msz) {
		return FALSE;
	}

	if (1 > kmsp->so_fobjnr || 1 > kmsp->so_mobjnr) {
		return FALSE;
	}

	if (msz > kmsp->so_objsz) {
		return FALSE;
	}
    
	if ((kmsp->so_vend - kmsp->so_vstat + 1) < PAGESIZE ||
		(kmsp->so_vend - kmsp->so_vstat + 1) < (adr_t)(sizeof(kmsob_t) + sizeof(freobjh_t))) {
		return FALSE;
	}

	if (list_is_empty_careful(&kmsp->so_frelst) == TRUE) {
		return FALSE;
	}

	return TRUE;
}

bool_t scan_fadrskmsob_isok(adr_t fostat, adr_t vend, void *fadrs, size_t objsz)
{

	if ((adr_t)fadrs < fostat) {
		return FALSE;
	}

	if ((adr_t)fadrs >= fostat && ((adr_t)fadrs + (adr_t)objsz) <= vend) {
		if (0 == (((adr_t)fadrs - fostat) % objsz)) {
			return TRUE;
		}

		return FALSE;
	}

	return FALSE;
}

bool_t scan_dfszkmsob_isok(kmsob_t *kmsp, void *fadrs, size_t fsz)
{
	list_h_t *tmplst = NULL;
	kmbext_t *bextp = NULL;
	if (NULL == kmsp || NULL == fadrs || 1 > fsz) {
		return FALSE;
	}

	if ((~0UL) <= kmsp->so_fobjnr) {
		return FALSE;
	}

	if ((adr_t)fadrs >= kmsp->so_vstat && ((adr_t)fadrs + (adr_t)fsz - 1) <= kmsp->so_vend) {
		if (FALSE == scan_fadrskmsob_isok((adr_t)(kmsp + 1), kmsp->so_vend, fadrs, kmsp->so_objsz)) {
			return FALSE;
		}

		return TRUE;
	}

	if (kmsp->so_mextnr > 0) {
		list_for_each(tmplst, &kmsp->so_mextlst) {
			bextp = list_entry(tmplst, kmbext_t, mt_list);
			if ((adr_t)fadrs >= bextp->mt_vstat && ((adr_t)fadrs + (adr_t)fsz - 1) <= bextp->mt_vend) {
				if (FALSE == scan_fadrskmsob_isok((adr_t)(bextp + 1), bextp->mt_vend, fadrs, kmsp->so_objsz)) {
					return FALSE;
				}
				return TRUE;
			}
		}
	}

	return FALSE;
}

// 如果内存对象容器中没有空闲的内存次对象了就需要扩展对象容器的内存了
uint_t scan_kmsob_objnr(kmsob_t *kmsp)
{
	if (NULL == kmsp) {
		system_error("scan_kmsob_objnr err1\n");
	}

	if (1 > kmsp->so_fobjnr && list_is_empty_careful(&kmsp->so_frelst) == FALSE) {
		system_error("scan_kmsob_objnr err2\n");
	}

	if (0 < kmsp->so_fobjnr) {
		return kmsp->so_fobjnr;
	}

	return 0;
}

// 返回合适的kmsob_t
kmsob_t *onkoblst_retn_newkmsob(koblst_t *koblp, size_t msz)
{
	kmsob_t *kmsp = NULL, *tkmsp = NULL;
	list_h_t *tmplst = NULL;

	if (NULL == koblp || 1 > msz) {
		return NULL;
	}

	// 先看看上次分配所用到的koblst_t是不是正好是这次需要的
	kmsp = scan_newkmsob_isok(koblp->ol_cahe, msz);
	if (NULL != kmsp) {
		return kmsp;
	}

	// 如果koblst_t中挂载的kmsob_t大于0
	if (0 < koblp->ol_emnr) {
		// 开始遍历koblst_t中挂载的kmsob_t
		list_for_each(tmplst, &koblp->ol_emplst) {
			tkmsp = list_entry(tmplst, kmsob_t, so_list);
			// 检查当前kmsob_t是否合乎要求
			kmsp = scan_newkmsob_isok(tkmsp, msz);

			if (NULL != kmsp) {
				return kmsp;
			}
		}
	}

	return NULL;
}

// 查找释放内存对象所属的kmsob_t结构
kmsob_t *onkoblst_retn_delkmsob(koblst_t *koblp, void *fadrs, size_t fsz)
{
	kmsob_t *kmsp = NULL, *tkmsp = NULL;
	list_h_t *tmplst = NULL;

	if (NULL == koblp || NULL == fadrs || 1 > fsz) {
		return NULL;
	}

	// 看看上次刚刚操作的kmsob_t结构
	kmsp = scan_delkmsob_isok(koblp->ol_cahe, fadrs, fsz);
	if (NULL != kmsp) {
		return kmsp;
	}

	if (0 < koblp->ol_emnr) {
		// 遍历挂载koblp->ol_emplst链表上的每个kmsob_t结构
		list_for_each(tmplst, &koblp->ol_emplst) {
			tkmsp = list_entry(tmplst, kmsob_t, so_list);
			// 检查释放的内存对象是不是属于这个kmsob_t结构
			kmsp = scan_delkmsob_isok(tkmsp, fadrs, fsz);

			if (NULL != kmsp) {
				return kmsp;
			}
		}
	}

	return NULL;
}

// koblst_t的内存对象大小大于要分配的数量，就返回这个koblst_t
koblst_t *onmsz_retn_koblst(kmsobmgrhed_t *kmmgrhlokp, size_t msz)
{
	// 遍历ks_msoblst数组
	if (NULL == kmmgrhlokp || 1 > msz) {
		return NULL;
	}

	for (uint_t kli = 0; kli < KOBLST_MAX; kli++) {
		// 只要大小合适就返回
		if (kmmgrhlokp->ks_msoblst[kli].ol_sz >= msz) {
			return &kmmgrhlokp->ks_msoblst[kli];
		}
	}

	return NULL;
}

// 把内存对象容器数据结构，挂载到对应的koblst_t结构中去
bool_t kmsob_add_koblst(koblst_t *koblp, kmsob_t *kmsp)
{
	if (NULL == koblp || NULL == kmsp) {
		return FALSE;
	}

	if (kmsp->so_objsz > koblp->ol_sz) {
		return FALSE;
	}

	list_add(&kmsp->so_list, &koblp->ol_emplst);
	koblp->ol_emnr++;
	return TRUE;
}

// 初始化内存对象容器
kmsob_t *_create_init_kmsob(kmsob_t *kmsp, size_t objsz, adr_t cvadrs, adr_t cvadre, msadsc_t *msa, uint_t relpnr)
{
	if (NULL == kmsp || 1 > objsz || NULL == cvadrs || NULL == cvadre || NULL == msa || 1 > relpnr) {
		return NULL;
	}

	if (objsz < sizeof(freobjh_t)) {
		return NULL;
	}

	if ((cvadre - cvadrs + 1) < PAGESIZE) {
		return NULL;
	}

	if ((cvadre - cvadrs + 1) <= (sizeof(kmsob_t) + sizeof(freobjh_t))) {
		return NULL;
	}

	// 初始化kmsob结构体
	kmsob_t_init(kmsp);

	// 设置内存对象容器的开始、结束地址，内存对象大小
	kmsp->so_vstat = cvadrs;
	kmsp->so_vend = cvadre;
	kmsp->so_objsz = objsz;

	// 把物理内存页面对应的msadsc_t结构加入到kmsob_t中的so_mc.mc_kmobinlst链表上
	list_add(&msa->md_list, &kmsp->so_mc.mc_kmobinlst);
	kmsp->so_mc.mc_kmobinpnr = (uint_t)relpnr;

	// 设置内存对象的开始地址为kmsob_t结构之后，结束地址为内存对象容器的结束地址
	// kmsp + 1 移动 kmsob_t 个字节
	freobjh_t *fohstat = (freobjh_t *)(kmsp + 1), *fohend = (freobjh_t *)cvadre;

	uint_t ap = (uint_t)((uint_t)fohstat);			// fohstat 地址
	freobjh_t *tmpfoh = (freobjh_t *)((uint_t)ap);	// fohstat 用于遍历的临时变量

	/**
	 * 在内存结束地址之前，建立多个freobjh_t结构体，然后放入kmsob_t结构体的链表中
	 * 
	 * 相当在kmsob_t结构体之后建立一个freobjh_t结构体数组
	 */
	for (; tmpfoh < fohend;) {
		if ((ap + (uint_t)kmsp->so_objsz) <= (uint_t)cvadre) {
			// 初始化每个freobjh_t结构体
			freobjh_t_init(tmpfoh, 0, (void *)tmpfoh);
			// 把每个freobjh_t结构体加入到kmsob_t结构体中的so_frelst中
			list_add(&tmpfoh->oh_list, &kmsp->so_frelst);
			kmsp->so_mobjnr++;
			kmsp->so_fobjnr++;
		}

		ap += (uint_t)kmsp->so_objsz;
		tmpfoh = (freobjh_t *)((uint_t)ap);
	}

	return kmsp;
}

/**
 * 建立一个内存对象容器
 * 	这个函数会找物理页面管理器申请一块连续内存页面
 * 	然后，在其中的开始部分建立kmsob_t结构的实例变量，又在kmsob_t结构的后面建立freobjh_t结构数组
 * 	并把每个freobjh_t结构挂载到kmsob_t结构体中的so_frelist中
 * 	最后再把kmsob_t结构，挂载到kmsobmgrhed_t结构对应的koblst_t结构中
 */
kmsob_t *_create_kmsob(kmsobmgrhed_t *kmmgrlokp, koblst_t *koblp, size_t objsz)
{
	if (NULL == kmmgrlokp || NULL == koblp || 1 > objsz) {
		return NULL;
	}

	kmsob_t *kmsp = NULL;
	msadsc_t *msa = NULL;
	uint_t relpnr = 0;
	uint_t pages = 1;

	if (128 < objsz) {
		pages = 2;	// 2048 字节
	}

	if (512 < objsz) {
		pages = 4;	// 4095 字节
	}

	// 为内存对象容器分配物理内存空间，这是我们之前实现的物理内存页面管理器
	msa = mm_division_pages(&memmgrob, pages, &relpnr, MA_TYPE_KRNL, DMF_RELDIV);
	if (NULL == msa) {
		return NULL;
	}

	if (NULL != msa && 0 == relpnr) {
		system_error("_create_kmsob mm_division_pages fail\n");
		return NULL;
	}

	u64_t phyadr = msa->md_phyadrs.paf_padrs << PSHRSIZE;
	u64_t phyade = phyadr + (relpnr << PSHRSIZE) - 1;
	/**
	 * 计算它们的虚拟地址
	 * 	vadrs: 开始地址
	 * 	vadre: 结束地址
	 */
	adr_t vadrs = phyadr_to_viradr((adr_t)phyadr);
	adr_t vadre = phyadr_to_viradr((adr_t)phyade);

	// 初始化kmsob_t并建立内存对象
	kmsp = _create_init_kmsob((kmsob_t *)vadrs, koblp->ol_sz, vadrs, vadre, msa, relpnr);
	if (NULL == kmsp) {
		// 把kmsob_t结构，挂载到对应的koblst_t结构中去.因为无法初始化kmsobt_t，所以要释放内存
		if (mm_merge_pages(&memmgrob, msa, relpnr) == FALSE) {
			system_error("_create_kmsob mm_merge_pages fail\n");
		}
		return NULL;
	}

	// 把内存对象容器数据结构，挂载到对应的koblst_t结构中去
	if (kmsob_add_koblst(koblp, kmsp) == FALSE) {
		system_error(" _create_kmsob kmsob_add_koblst FALSE\n");
	}

	// 增加计数
	kmmgrlokp->ks_msobnr++;
	return kmsp;
}

/**
 * 实际分配的内存对象
 * 	从空闲内存对象链表头中取出第一个内存对象，返回它的首地址
 */
void *kmsob_new_opkmsob(kmsob_t *kmsp, size_t msz)
{
	if (NULL == kmsp || 1 > msz) {
		return NULL;
	}

	if (scan_nmszkmsob_isok(kmsp, msz) == FALSE) {
		return NULL;
	}

	// 获取kmsob_t中的so_frelst链表透的第一个空闲内存对象
	freobjh_t *fobh = list_entry(kmsp->so_frelst.next, freobjh_t, oh_list);
	// 从链表中脱链
	list_del(&fobh->oh_list);
	// kmsob_t中的空闲对象计数减一
	kmsp->so_fobjnr--;
	// 返回内存对象首地址
	return (void *)(fobh);
}

/**
 * 扩展内存页面
 * 	如果不断重复分配同一大小的内存对象，那么内存对象容器中的内存对象，迟早要分配完的
 * 	一旦内存对象分配完，内存对象容器就没有空闲的内存空间产生内存对象，就要扩展内存空间
 * 
 *  所以，分配另一块连续的内存空间，作为空间的内存对象，并且把这块内存空间加内存对象容器中统一管理
 */
bool_t kmsob_extn_pages(kmsob_t *kmsp)
{
	if (NULL == kmsp) {
		return FALSE;
	}

	if ((~0UL) <= kmsp->so_mobjnr || (~0UL) <= kmsp->so_mextnr || (~0UL) <= kmsp->so_fobjnr) {
		return FALSE;
	}
    
	msadsc_t *msa = NULL;
	uint_t relpnr = 0;
	uint_t pages = 1;

	if (128 < kmsp->so_objsz) {
		pages = 2;
	}

	if (512 < kmsp->so_objsz) {
		pages = 4;
	}

	// 找物理内存页面管理器分配2或者4个连续的页面
	msa = mm_division_pages(&memmgrob, pages, &relpnr, MA_TYPE_KRNL, DMF_RELDIV);
	if (NULL == msa) {
		return FALSE;
	}

	if (NULL != msa && 0 == relpnr) {
		system_error("kmsob_extn_pages mm_division_pages fail\n");
		return FALSE;
	}

	u64_t phyadr = msa->md_phyadrs.paf_padrs << PSHRSIZE;
	u64_t phyade = phyadr + (relpnr << PSHRSIZE) - 1;
	/**
	 * vadrs: 内存开始地址
	 * vadre: 内存结束地址
	 */
	adr_t vadrs = phyadr_to_viradr((adr_t)phyadr);
	adr_t vadre = phyadr_to_viradr((adr_t)phyade);

	// 求出物理内存页面数对应在kmsob_t的so_mc.mc_lst数组中下标
	sint_t mscidx = retn_mscidx(relpnr);

	// 判断 mscidx 是否大于 MSCLST_MAX，是则释放分配的内存
	if (MSCLST_MAX <= mscidx || 0 > mscidx) {
		if (mm_merge_pages(&memmgrob, msa, relpnr) == FALSE) {
			system_error("kmsob_extn_pages mm_merge_pages fail\n");
		}
		return FALSE;
	}

	// 把物理内存页面对应的msadsc_t结构加入到kmsob_t的so_mc.mc_lst数组中
	list_add(&msa->md_list, &kmsp->so_mc.mc_lst[mscidx].ml_list);
	kmsp->so_mc.mc_lst[mscidx].ml_msanr++;

	kmbext_t *bextp = (kmbext_t *)vadrs;
	kmbext_t_init(bextp, vadrs, vadre, kmsp);

	freobjh_t *fohstat = (freobjh_t *)(bextp + 1), *fohend = (freobjh_t *)vadre;

	uint_t ap = (uint_t)((uint_t)fohstat);
	freobjh_t *tmpfoh = (freobjh_t *)((uint_t)ap);

	for (; tmpfoh < fohend;) {
		//ap+=(uint_t)kmsp->so_objsz;
		if ((ap + (uint_t)kmsp->so_objsz) <= (uint_t)vadre) {
			// 在扩展的内存空间中建立内存对象
			freobjh_t_init(tmpfoh, 0, (void *)tmpfoh);
			list_add(&tmpfoh->oh_list, &kmsp->so_frelst);
			kmsp->so_mobjnr++;
			kmsp->so_fobjnr++;
			bextp->mt_mobjnr++;
		}
		ap += (uint_t)kmsp->so_objsz;
		tmpfoh = (freobjh_t *)((uint_t)ap);
	}

	list_add(&bextp->mt_list, &kmsp->so_mextlst);
	kmsp->so_mextnr++;
	return TRUE;
}

// 从空闲内存对象链表头中取出第一个内存对象，返回它的首地址。
void *kmsob_new_onkmsob(kmsob_t *kmsp, size_t msz)
{
	if (NULL == kmsp || 1 > msz) {
		return NULL;
	}

	void *retptr = NULL;
	cpuflg_t cpuflg;
	knl_spinlock_cli(&kmsp->so_lock, &cpuflg);
	// 如果内存对象容器中没有空闲的内存次对象了就需要扩展对象容器的内存了
	if (scan_kmsob_objnr(kmsp) < 1) {
		// 扩展内存对象容器的内存
		if (kmsob_extn_pages(kmsp) == FALSE) {
			retptr = NULL;
			goto ret_step;
		}
	}

	// 实际分配内存对象
	retptr = kmsob_new_opkmsob(kmsp, msz);
ret_step:
	knl_spinunlock_sti(&kmsp->so_lock, &cpuflg);
	return retptr;
}

// 分配内存对象的核心函数
void *kmsob_new_core(size_t msz)
{
	// 获取kmsobmgrhed_t结构的地址
	kmsobmgrhed_t *kmobmgrp = &memmgrob.mo_kmsobmgr;
	void *retptr = NULL;
	koblst_t *koblp = NULL;
	kmsob_t *kmsp = NULL;
	cpuflg_t cpuflg;
	// 对kmsobmgrhed_t结构加锁
	knl_spinlock_cli(&kmobmgrp->ks_lock, &cpuflg);

	// koblst_t的内存对象大小大于要分配的数量，就返回这个koblst_t
	koblp = onmsz_retn_koblst(kmobmgrp, msz);
	if (NULL == koblp) {
		retptr = NULL;
		goto ret_step;
	}

	// 返回合适的kmsob_t
	kmsp = onkoblst_retn_newkmsob(koblp, msz);
	if (NULL == kmsp) {
		kmsp = _create_kmsob(kmobmgrp, koblp, koblp->ol_sz);
		if (NULL == kmsp) {
			retptr = NULL;
			goto ret_step;
		}
	}

	// 从空闲内存对象链表头中取出第一个内存对象，返回它的首地址
	retptr = kmsob_new_onkmsob(kmsp, msz);
	if (NULL == retptr) {
		retptr = NULL;
		goto ret_step;
	}

	// 更新kmsobmgrhed_t结构的信息
	kmsob_updata_cache(kmobmgrp, koblp, kmsp, KUC_NEWFLG);

ret_step:
	// 解锁kmsobmgrhed_t结构
	knl_spinunlock_sti(&kmobmgrp->ks_lock, &cpuflg);
	return retptr;
}

// 内存对象分配接口
void *kmsob_new(size_t msz)
{
	// 对于小于1 或者 大于2048字节的大小不支持 直接返回NULL表示失败
	if (1 > msz || 2048 < msz) {
		return NULL;
	}

	// 调用核心函数
	return kmsob_new_core(msz);
}

uint_t scan_freekmsob_isok(kmsob_t *kmsp)
{
	if (NULL == kmsp) {
		return 0;
	}

	if (kmsp->so_mobjnr < kmsp->so_fobjnr) {
		return 0;
	}

	// 当内存对象容器的总对象个数等于空闲对象个数时，说明这内存对象容器空闲
	if (kmsp->so_mobjnr == kmsp->so_fobjnr) {
		return 2;
	}

	return 1;
}

/**
 * 实际销毁内存对象容器
 * 	频繁请求的是不同大小的内存对象，那么空的内存对象容器会越来越多
 * 	这会占用大量内存，所以我们必须要把空的内存对象容器销毁
 * 
 * 	首先要释放内存对象容器的扩展空间所占用的物理内存页面，最后才可以释放内存对象容器自身占用物理内存页面
 */
bool_t _destroy_kmsob_core(kmsobmgrhed_t *kmobmgrp, koblst_t *koblp, kmsob_t *kmsp)
{
	if (NULL == kmobmgrp || NULL == koblp || NULL == kmsp) {
		return FALSE;
	}

	if (1 > kmsp->so_mc.mc_kmobinpnr || list_is_empty_careful(&kmsp->so_mc.mc_kmobinlst) == TRUE) {
		return FALSE;
	}

	// 将kmsob_t 脱链
	list_h_t *tmplst = NULL;
	msadsc_t *msa = NULL;
	msclst_t *mscp = kmsp->so_mc.mc_lst;
	list_del(&kmsp->so_list);
	koblp->ol_emnr--;
	kmobmgrp->ks_msobnr--;

	// 更新kmsobmgrhed_t结构的信息
	kmsob_updata_cache(kmobmgrp, koblp, kmsp, KUC_DSYFLG);

	/**
	 * 释放内存对象容器扩展空间的物理内存页面
	 * 遍历kmsob_t结构中的so_mc.mc_lst数组
	 * 
	 * 遍历释放kmsob_t中全部扩展结构占用的内存页面【先脱链、再释放】
	 */
	for (uint_t j = 0; j < MSCLST_MAX; j++) {
		if (0 < mscp[j].ml_msanr) {
			// 遍历每个so_mc.mc_lst数组中的msadsc_t结构
			list_for_each_head_dell(tmplst, &mscp[j].ml_list) {
				msa = list_entry(tmplst, msadsc_t, md_list);
				list_del(&msa->md_list);

				/**
				 * msadsc_t脱链
				 * 释放msadsc_t对应的物理内存页面
				 */
				if (mm_merge_pages(&memmgrob, msa, (uint_t)mscp[j].ml_ompnr) == FALSE) {
					system_error("_destroy_kmsob_core mm_merge_pages FALSE2\n");
				}
			}
		}
	}
	
	/**
	 * 释放内存对象容器本身占用的物理内存页面
	 * 遍历每个so_mc.mc_kmobinlst中的msadsc_t结构。它只会遍历一次
	 * 
	 * 释放kmsob_t自身占用的全部页面【先脱链，再释放】
	 */
	list_for_each_head_dell(tmplst, &kmsp->so_mc.mc_kmobinlst) {
		msa = list_entry(tmplst, msadsc_t, md_list);
		list_del(&msa->md_list);
		/**
		 * msadsc_t脱链
		 * 释放msadsc_t对应的物理内存页面
		 */
		if (mm_merge_pages(&memmgrob, msa, (uint_t)kmsp->so_mc.mc_kmobinpnr) == FALSE) {
			system_error("_destroy_kmsob_core mm_merge_pages FALSE2\n");
		}
	}

	return TRUE;
}

// 销毁内存对象容器
bool_t _destroy_kmsob(kmsobmgrhed_t *kmobmgrp, koblst_t *koblp, kmsob_t *kmsp)
{
	if (NULL == kmobmgrp || NULL == koblp || NULL == kmsp) {
		return FALSE;
	}

	if (1 > kmobmgrp->ks_msobnr || 1 > koblp->ol_emnr) {
		return FALSE;
	}

	// 看看能不能销毁
	uint_t screts = scan_freekmsob_isok(kmsp);
	if (0 == screts) {
		system_error("_destroy_kmsob scan_freekmsob_isok rets 0\n");
	}

	if (1 == screts) {
		kmsob_updata_cache(kmobmgrp, koblp, kmsp, KUC_DELFLG);
		return TRUE;
	}

	if (2 == screts) {
		// 调用销毁内存对象容器的核心函数
		return _destroy_kmsob_core(kmobmgrp, koblp, kmsp);
	}
    
	return FALSE;
}

/**
 * 释放内存对象
 * 	把要释放内存对象的空间，重新初始化，变成一个freobjh_t结构的实例变量
 * 	最后把这个freobjh_t结构加入到kmsobj_t结构中空闲链表中，这就实现了内存对象的释放
 */
bool_t kmsob_del_opkmsob(kmsob_t *kmsp, void *fadrs, size_t fsz)
{
	if (NULL == kmsp || NULL == fadrs || 1 > fsz) {
		return FALSE;
	}

	if ((kmsp->so_fobjnr + 1) > kmsp->so_mobjnr) {
		return FALSE;
	}

	if (scan_dfszkmsob_isok(kmsp, fadrs, fsz) == FALSE) {
		return FALSE;
	}

	// 让freobjh_t结构重新指向要释放的内存空间
	freobjh_t *obhp = (freobjh_t *)fadrs;
	// 重新初始化块内存空间
	freobjh_t_init(obhp, 0, obhp);
	// 加入kmsob_t结构的空闲链表
	list_add(&obhp->oh_list, &kmsp->so_frelst);
	// kmsob_t结构的空闲对象计数加一
	kmsp->so_fobjnr++;
	return TRUE;
}

// 释放内存对象
bool_t kmsob_delete_onkmsob(kmsob_t *kmsp, void *fadrs, size_t fsz)
{
	if (NULL == kmsp || NULL == fadrs || 1 > fsz) {
		return FALSE;
	}

	bool_t rets = FALSE;
	cpuflg_t cpuflg;
	// 对kmsob_t结构加锁
	knl_spinlock_cli(&kmsp->so_lock, &cpuflg);
	// 实际完成内存对象释放
	if (kmsob_del_opkmsob(kmsp, fadrs, fsz) == FALSE) {
		rets = FALSE;
		goto ret_step;
	}

	rets = TRUE;

ret_step:
	// 对kmsob_t结构解锁
	knl_spinunlock_sti(&kmsp->so_lock, &cpuflg);
	return rets;
}

// 释放内存接口
bool_t kmsob_delete_core(void *fadrs, size_t fsz)
{
	kmsobmgrhed_t *kmobmgrp = &memmgrob.mo_kmsobmgr;
	bool_t rets = FALSE;
	koblst_t *koblp = NULL;
	kmsob_t *kmsp = NULL;
	cpuflg_t cpuflg;
	knl_spinlock_cli(&kmobmgrp->ks_lock, &cpuflg);
	
	// 根据释放内存对象的大小在kmsobmgrhed_t中查找并返回koblst_t，在其中挂载对应的kmsob_t，这个在前面已经写好了
	koblp = onmsz_retn_koblst(kmobmgrp, fsz);
	if (NULL == koblp) {
		rets = FALSE;
		goto ret_step;
	}

	// 查找释放内存对象所属的kmsob_t结构
	kmsp = onkoblst_retn_delkmsob(koblp, fadrs, fsz);
	if (NULL == kmsp) {
		rets = FALSE;
		goto ret_step;
	}

	// 释放内存对象
	rets = kmsob_delete_onkmsob(kmsp, fadrs, fsz);
	if (FALSE == rets) {
		rets = FALSE;
		goto ret_step;
	}

	// 销毁内存对象容器
	if (_destroy_kmsob(kmobmgrp, koblp, kmsp) == FALSE) {
		rets = FALSE;
		goto ret_step;
	}

	rets = TRUE;

ret_step:
	knl_spinunlock_sti(&kmobmgrp->ks_lock, &cpuflg);
	return rets;
}

// 释放内存对象接口
bool_t kmsob_delete(void *fadrs, size_t fsz)
{
	if (NULL == fadrs || 1 > fsz || 2048 < fsz) {
		return FALSE;
	}

	// 调用释放内存对象的核心函数
	return kmsob_delete_core(fadrs, fsz);
}

bool_t chek_kmbext_findmsa(kmsob_t *kmsp, kmbext_t *cpbexp)
{
	uint_t pnr = (cpbexp->mt_vend - cpbexp->mt_vstat + 1) >> PSHRSIZE;
	sint_t msci = retn_mscidx(pnr);
	if (0 > msci || MSCLST_MAX <= msci) {
		return FALSE;
	}

	if (kmsp->so_mc.mc_lst[msci].ml_ompnr != pnr) {
		return FALSE;
	}

	uint_t phyadr = (uint_t)viradr_to_phyadr(cpbexp->mt_vstat);
	list_h_t *tmplst = NULL;
	msadsc_t *msa = NULL;
	list_for_each(tmplst, &kmsp->so_mc.mc_lst[msci].ml_list) {
		msa = list_entry(tmplst, msadsc_t, md_list);
		if ((msa->md_phyadrs.paf_padrs << PSHRSIZE) == phyadr) {
			return TRUE;
		}
	}

	return FALSE;
}

bool_t chek_one_kmbext(kmsob_t *kmsp, kmbext_t *cpbexp)
{
	list_h_t *tmplst = NULL;
	kmbext_t *bexp = NULL;
	list_for_each(tmplst, &kmsp->so_mextlst) {
		bexp = list_entry(tmplst, kmbext_t, mt_list);
		if (bexp != cpbexp) {
			if ((bexp->mt_vstat >= cpbexp->mt_vstat && bexp->mt_vstat <= cpbexp->mt_vend)) {
				return FALSE;
			}

			if (chek_kmbext_findmsa(kmsp, cpbexp) == FALSE) {
				return FALSE;
			}
		}

		if (chek_kmbext_findmsa(kmsp, bexp) == FALSE) {
			return FALSE;
		}
	}

	return TRUE;
}

bool_t chek_onekmsob_mbext(kmsob_t *kmsp)
{
	size_t mobsz = kmsp->so_mobjnr * kmsp->so_objsz;
	size_t kbsz = kmsp->so_vend - (adr_t)(kmsp + 1) + 1;
	if (mobsz > kbsz && kmsp->so_mextnr < 1) {
		system_error("chek_onekmsob_mbext err1\n");
	}

	if (kmsp->so_mextnr == 0) {
		return TRUE;
	}

	list_h_t *tmplst = NULL;
	kmbext_t *bexp = NULL;
	list_for_each(tmplst, &kmsp->so_mextlst) {
		bexp = list_entry(tmplst, kmbext_t, mt_list);
		if (chek_one_kmbext(kmsp, bexp) == FALSE) {
			return FALSE;
		}
	}

	return TRUE;
}

void chek_one_kmsob(kmsob_t *kmsp, size_t objsz)
{
	if (NULL == kmsp) {
		system_error("chek_one_kmsob err1\n");
	}

	if (kmsp->so_objsz != objsz) {
		system_error("chek_one_kmsob err2\n");
	}

	if (list_is_empty_careful(&kmsp->so_mc.mc_kmobinlst) == TRUE) {
		system_error("chek_one_kmsob err3\n");
	}

	msadsc_t *msa = list_entry(kmsp->so_mc.mc_kmobinlst.next, msadsc_t, md_list);
	if (kmsp->so_vstat != phyadr_to_viradr((adr_t)(msa->md_phyadrs.paf_padrs << PSHRSIZE))) {
		system_error("chek_one_kmsob err4\n");
	}

	if (((kmsp->so_vend - kmsp->so_vstat + 1) >> PSHRSIZE) != kmsp->so_mc.mc_kmobinpnr) {
		system_error("chek_one_kmsob err5\n");
	}

	if (chek_onekmsob_mbext(kmsp) == FALSE) {
		system_error("chek_one_kmsob err6\n");
	}

	return;
}

void chek_all_kmsobstruc()
{
	kmsob_t *kmsp = NULL;
	list_h_t *tmplst = NULL;
	uint_t nr = 0;
	for (uint_t i = 0; i < KOBLST_MAX; i++) {
		list_for_each(tmplst, &memmgrob.mo_kmsobmgr.ks_msoblst[i].ol_emplst) {
			kmsp = list_entry(tmplst, kmsob_t, so_list);
			chek_one_kmsob(kmsp, memmgrob.mo_kmsobmgr.ks_msoblst[i].ol_sz);
			kprint("chek_one_kmsob:%d\n", nr);
			nr++;
		}
	}

	return;
}

void kobcks_init(kobcks_t *initp, void *vadr, size_t sz)
{
	list_init(&initp->kk_list);
	initp->kk_vadr = vadr;
	initp->kk_sz = sz;
	return;
}

void write_kobcks(kmsobmgrhed_t *kmmgrp, void *ptr, size_t sz)
{
	if (NULL == kmmgrp || NULL == ptr || 1 > sz) {
		system_error("write_kobcks err\n");
	}

	kobcks_t *kkp = (kobcks_t *)ptr;
	kobcks_init(kkp, ptr, sz);
	list_add(&kkp->kk_list, &kmmgrp->ks_tclst);
	kmmgrp->ks_tcnr++;
	return;
}

void chek_one_kobcks(kobcks_t *kkp)
{
	list_h_t *tmplst = NULL;
	kobcks_t *tkkp = NULL;
	list_for_each(tmplst, &memmgrob.mo_kmsobmgr.ks_tclst) {
		tkkp = list_entry(tmplst, kobcks_t, kk_list);
		if (tkkp != kkp) {
			if (tkkp->kk_vadr == kkp->kk_vadr) {
				system_error("chek_one_kobcks fail\n");
			}
		}
	}

	return;
}

void chek_all_kobcks()
{
	chek_all_kmsobstruc();
	die(0x600);
	list_h_t *tmplst = NULL;
	kobcks_t *kkp = NULL;
	uint_t i = 0;
	list_for_each(tmplst, &memmgrob.mo_kmsobmgr.ks_tclst) {
		kkp = list_entry(tmplst, kobcks_t, kk_list);
		chek_one_kobcks(kkp);
		kprint("chek_one_kobcks isOK:%d\n", i);
		i++;
	}

	return;
}

void free_one_kobcks(kobcks_t *kkp)
{

	if (1 > memmgrob.mo_kmsobmgr.ks_tcnr) {
		system_error("free_one_kobcks err1\n");
	}

	list_del(&kkp->kk_list);

	if (kmsob_delete(kkp->kk_vadr, kkp->kk_sz) == FALSE) {
		kprint("kkp->kk_vadr:%x sz:%x\n", kkp->kk_vadr, kkp->kk_sz);
		system_error("free_one_kobcks fail\n");
	}

	memmgrob.mo_kmsobmgr.ks_tcnr--;
	return;
}

void free_all_kobcks()
{
	list_h_t *tmplst = NULL;
	kobcks_t *kkp = NULL;
	uint_t i = 0;

	list_for_each_head_dell(tmplst, &memmgrob.mo_kmsobmgr.ks_tclst) {
		kkp = list_entry(tmplst, kobcks_t, kk_list);
		free_one_kobcks(kkp);
		kprint("free_one_kobcks isOK:%d\n", i);
		i++;
	}

	kprint("free_all_kobcks OK\n");
	return;
}

void test_kmsob()
{
	void *ptr = NULL;
	u64_t stsc = 0, etsc = 0;
	size_t sz = 1;
	kprint("我笃信优秀的操作系统,起始于精湛的内存管理......\n");
	for (uint_t n = 0; n < 2048; n++, sz++) {
		for (uint_t i = 0; i < 5; i++) {
			stsc = x86_rdtsc();
			ptr = kmsob_new(sz);
			etsc = x86_rdtsc();
			if (NULL == ptr) {
				kprint("FREEPAGES:%x ALLOCPAGES:%x MAXPAGES:%x KMSOBNR:%x OBJNR:%x\n", (uint_t)memmgrob.mo_freepages, (uint_t)memmgrob.mo_alocpages,
					   (uint_t)memmgrob.mo_maxpages, (uint_t)memmgrob.mo_kmsobmgr.ks_msobnr,
					   (uint_t)memmgrob.mo_kmsobmgr.ks_tcnr);
				system_error("test_kmsob kmsob_new ret NULL\n");
			}
            
			kprint("找不到对象就NEW一个: _new 对象大小:%x retnadrs:%x CPU时钟周期:%x\n", sz, (uint_t)ptr, etsc - stsc);
			write_kobcks(&memmgrob.mo_kmsobmgr, ptr, sz);
			//kprint("kmsob_delete is ok\n");
		}
	}
	return;
}
