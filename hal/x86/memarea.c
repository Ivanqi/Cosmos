/**********************************************************
    物理内存区间文件memarea.c
***********************************************************/

#include "cosmostypes.h"
#include "cosmosmctrl.h"

void arclst_t_init(arclst_t *initp)
{
	list_init(&initp->al_lru1);
	list_init(&initp->al_lru2);
	initp->al_lru1nr = 0;
	initp->al_lru2nr = 0;
	return;
}

mmstus_t mafo_deft_init(struct s_MEMAREA *memarea, void *valp, uint_t val)
{
	return MMSTUS_ERR;
}

mmstus_t mafo_deft_exit(struct s_MEMAREA *memarea)
{
	return MMSTUS_ERR;
}

mmstus_t mafo_deft_afry(struct s_MEMAREA *memarea, mmafrets_t *mafrspack, void *valp, uint_t val)
{
	return MMSTUS_ERR;
}

void mafuncobjs_t_init(mafuncobjs_t *initp)
{
	initp->mafo_init = mafo_deft_init;
	initp->mafo_exit = mafo_deft_exit;
	initp->mafo_aloc = mafo_deft_afry;
	initp->mafo_free = mafo_deft_afry;
	initp->mafo_recy = mafo_deft_afry;
	return;
}

/**
 * @brief 初始化bafhlst_t结构体的基本数据
 * 
 * @param initp 
 * @param stus 
 * @param oder 
 * @param oderpnr 
 */
void bafhlst_t_init(bafhlst_t *initp, u32_t stus, uint_t oder, uint_t oderpnr)
{
	knl_spinlock_init(&initp->af_lock);
	initp->af_stus = stus;
	initp->af_oder = oder;
	initp->af_oderpnr = oderpnr;
	initp->af_fobjnr = 0;
	//initp->af_aobjnr=0;
	initp->af_mobjnr = 0;
	initp->af_alcindx = 0;
	initp->af_freindx = 0;
	
	list_init(&initp->af_frelst);
	list_init(&initp->af_alclst);
	list_init(&initp->af_ovelst);
	return;
}

/**
 * @brief 初始化medivmer_t结构体的基本数据
 * 
 * @param initp 
 */
void memdivmer_t_init(memdivmer_t *initp)
{
	knl_spinlock_init(&initp->dm_lock);
	initp->dm_stus = 0;			// 状态
	initp->dm_dmmaxindx = 0;
	initp->dm_phydmindx = 0;
	initp->dm_predmindx = 0;
	initp->dm_divnr = 0;
	initp->dm_mernr = 0;

	/**
	 * 循环初始化memdivmer_t结构体中dm_mdmlielst数组中的每个bafhlst_t结构的基本数据
	 * bafhlst_t数组中的每个bafhlst_t，会根据其在数组中的序号n，存放全部2的n次方的连续页面，也就是说：
	 * 	1. 第0个bafhlst_t，存放全部长度为1的内存段
	 * 	2. 第1个bafhlst_t，存放全部长度为2的内存段
	 * 	3. 第2个bafhlst_t，存放全部长度为4的内存段
	 */
	for (uint_t li = 0; li < MDIVMER_ARR_LMAX; li++) {
		bafhlst_t_init(&initp->dm_mdmlielst[li], BAFH_STUS_DIVM, li, (1UL << li));
	}

	bafhlst_t_init(&initp->dm_onemsalst, BAFH_STUS_ONEM, 0, 1UL);
	return;
}

/**
 * @brief 初始化memarea_t结构体的基本数据
 * 
 * @param initp memarea虚拟地址
 */
void memarea_t_init(memarea_t *initp)
{
	list_init(&initp->ma_list);
	knl_spinlock_init(&initp->ma_lock);

	initp->ma_stus = 0;
	initp->ma_flgs = 0;
	initp->ma_type = MA_TYPE_INIT;
	initp->ma_maxpages = 0;
	initp->ma_allocpages = 0;
	initp->ma_freepages = 0;
	initp->ma_resvpages = 0;
	initp->ma_horizline = 0;
	initp->ma_logicstart = 0;
	initp->ma_logicend = 0;
	initp->ma_logicsz = 0;
	initp->ma_effectstart = 0;
	initp->ma_effectend = 0;
	initp->ma_effectsz = 0;

	list_init(&initp->ma_allmsadsclst);
	initp->ma_allmsadscnr = 0;

	arclst_t_init(&initp->ma_arcpglst);
	mafuncobjs_t_init(&initp->ma_funcobj);
	// 初始化memarea_t结构体中的memdivmer_t结构体
	memdivmer_t_init(&initp->ma_mdmdata);

	initp->ma_privp = NULL;
	return;
}

/**
 * @brief 初始化内存区(memarea)
 * 
 * @param mbsp 
 * @return bool_t 
 */
bool_t init_memarea_core(machbstart_t *mbsp)
{
	// 获取memarea_t结构开始地址
	u64_t phymarea = mbsp->mb_nextwtpadr;
	// 检查内存空间够不够放下MEMAREA_MAX个memarea_t结构实例变量
	if (initchkadr_is_ok(mbsp, phymarea, (sizeof(memarea_t) * MEMAREA_MAX)) != 0) {
		return FALSE;
	}

	memarea_t *virmarea = (memarea_t *)phyadr_to_viradr((adr_t)phymarea);
	// 对 MEMAREA_MAX 个 memarea_t 结构进行了基本的初始化
	for (uint_t mai = 0; mai < MEMAREA_MAX; mai++) {
		// 循环初始化每个memarea_t结构实例变量
		memarea_t_init(&virmarea[mai]);
	}

	// 设置硬件区的类型和空间大小(硬件区)
	virmarea[0].ma_type = MA_TYPE_HWAD;
	virmarea[0].ma_logicstart = MA_HWAD_LSTART;
	virmarea[0].ma_logicend = MA_HWAD_LEND;
	virmarea[0].ma_logicsz = MA_HWAD_LSZ;

	// 设置内核区的类型和空间大小(内核区)
	virmarea[1].ma_type = MA_TYPE_KRNL;
	virmarea[1].ma_logicstart = MA_KRNL_LSTART;
	virmarea[1].ma_logicend = MA_KRNL_LEND;
	virmarea[1].ma_logicsz = MA_KRNL_LSZ;

	// 设置应用区的类型和空间大小(应用区)
	virmarea[2].ma_type = MA_TYPE_PROC;
	virmarea[2].ma_logicstart = MA_PROC_LSTART;
	virmarea[2].ma_logicend = MA_PROC_LEND;
	virmarea[2].ma_logicsz = MA_PROC_LSZ;

	virmarea[3].ma_type = MA_TYPE_SHAR;

	// 将memarea_t结构的开始的物理地址写入kmachbsp结构中
	mbsp->mb_memznpadr = phymarea;
	// 将memarea_t结构的个数写入kmachbsp结构中
	mbsp->mb_memznnr = MEMAREA_MAX;
	// 将所有memarea_t结构的大小写入kmachbsp结构中
	mbsp->mb_memznsz = sizeof(memarea_t) * MEMAREA_MAX;
	// 计算下一个空闲内存的开始地址
	mbsp->mb_nextwtpadr = PAGE_ALIGN(phymarea + sizeof(memarea_t) * MEMAREA_MAX);
	//.......
	return TRUE;
}

/**
 * @brief 初始化内存区
 * 
 * @return LKINIT 
 */
LKINIT void init_memarea()
{
	// 真正初始化内存区
	if (init_memarea_core(&kmachbsp) == FALSE) {
		system_error("init_memarea_core fail");
	}
	return;
}

bool_t find_inmarea_msadscsegmant(memarea_t *mareap, msadsc_t *fmstat, uint_t fmsanr, msadsc_t **retmsastatp, msadsc_t **retmsaendp, uint_t *retfmnr)
{
	if (NULL == mareap || NULL == fmstat || 0 == fmsanr || NULL == retmsastatp || NULL == retmsaendp || NULL == retfmnr) {
		return FALSE;
	}

	return TRUE;
}

/**
 * @brief 检测前后两个msadsc_t结构体的msadflgs_t是否和cmpmdfp一致且空闲的
 * 
 * @param prevmsa 上一个msadsc地址
 * @param nextmsa 下一个msadsc地址
 * @param cmpmdfp 
 * @return uint_t 返回-1，返回失败 | 返回0，返回失败 | 返回1，前后两个msadsc_t结构体的msadflgs_t是否和cmpmdfp一致且空闲的 
 * 				| 返回2，在1的基础上，前后两个msadsc_t是连续的
 */
uint_t continumsadsc_is_ok(msadsc_t *prevmsa, msadsc_t *nextmsa, msadflgs_t *cmpmdfp)
{
	if (NULL == prevmsa || NULL == cmpmdfp) {
		return (~0UL);
	}

	if (NULL != prevmsa && NULL != nextmsa) {
		if (prevmsa->md_indxflgs.mf_marty == cmpmdfp->mf_marty && 0 == prevmsa->md_indxflgs.mf_uindx &&
			MF_MOCTY_FREE == prevmsa->md_indxflgs.mf_mocty && PAF_NO_ALLOC == prevmsa->md_phyadrs.paf_alloc) {

			if (nextmsa->md_indxflgs.mf_marty == cmpmdfp->mf_marty && 0 == nextmsa->md_indxflgs.mf_uindx &&
				MF_MOCTY_FREE == nextmsa->md_indxflgs.mf_mocty && PAF_NO_ALLOC == nextmsa->md_phyadrs.paf_alloc) {
				
				// 前后两个msadsc_t刚好为一页
				if ((nextmsa->md_phyadrs.paf_padrs << PSHRSIZE) - (prevmsa->md_phyadrs.paf_padrs << PSHRSIZE) == PAGESIZE) {
					return 2;
				}

				return 1;
			}
			return 1;
		}
		return 0;
	}

	return (~0UL);
}

/**
 * @brief 返回的是连续地址msadsc_t的结构体的个数
 * 	1. 返回从这个msadsc_t结构开始到下一个非空闲、地址非连续的msadsc_t结构对应的msadsc_t结构索引号到retfindmnr变量中
 * 
 * @param mstat 某个msadsc地址
 * @param cmpmdfp 内存空间地址描述符标志
 * @param mnr msadsc 总长度
 * @param retmnr 连续长度
 * @return bool_t 
 */
bool_t scan_len_msadsc(msadsc_t *mstat, msadflgs_t *cmpmdfp, uint_t mnr, uint_t *retmnr)
{
	uint_t retclok = 0;
	uint_t retnr = 0;
	if (NULL == mstat || NULL == cmpmdfp || 0 == mnr || NULL == retmnr) {
		return FALSE;
	}

	// 从msadsc 的地址继续进行遍历
	for (uint_t tmdx = 0; tmdx < mnr - 1; tmdx++) {
		// 检测前后两个msadsc_t结构体的msadflgs_t是否和cmpmdfp一致且空闲的，并且检测是否是连续的
		retclok = continumsadsc_is_ok(&mstat[tmdx], &mstat[tmdx + 1], cmpmdfp);
		if ((~0UL) == retclok) {
			*retmnr = 0;
			return FALSE;
		}

		if (0 == retclok) {
			*retmnr = 0;
			return FALSE;
		}

		if (1 == retclok) {
			*retmnr = retnr;
			return TRUE;
		}

		retnr++;
	}

	*retmnr = retnr;
	return TRUE;
}

/**
 * @brief 检查连续的msadsc_t 结构
 * 	1. 为了判断有多少个连续的msadsc_t 属于 memarea_t
 * 
 * @param mareap memarea 内存区
 * @param stat msadsc 开始地址
 * @param end msadsc 结束地址
 * @param fmnr 用于检验的msadsc连续长度
 * @return uint_t 
 */
uint_t check_continumsadsc(memarea_t *mareap, msadsc_t *stat, msadsc_t *end, uint_t fmnr)
{
	msadsc_t *ms = stat, *me = end;
	u32_t muindx = 0;
	msadflgs_t *mdfp = NULL;
	if (NULL == ms || NULL == me || 0 == fmnr || ms > me) {
		return 0;
	}

	// memarea_t 分类
	switch (mareap->ma_type) {
	    case MA_TYPE_HWAD:
	    {
            muindx = MF_MARTY_HWD << 5;			// 硬件区
            mdfp = (msadflgs_t *)(&muindx);
            break;
	    }

        case MA_TYPE_KRNL:
        {
            muindx = MF_MARTY_KRL << 5;			// 内核区
            mdfp = (msadflgs_t *)(&muindx);
            break;
        }

        case MA_TYPE_PROC:
        {
            muindx = MF_MARTY_PRC << 5;			// 应用区
            mdfp = (msadflgs_t *)(&muindx);
            break;
        }

        default:
        {
            muindx = 0;
            mdfp = NULL;
            break;
        }
	}

	if (0 == muindx || NULL == mdfp) {
		return 0;
	}

	uint_t ok = 0;
	// 单个 msadsc_t
	if (ms == me) {
		if (0 != ms->md_indxflgs.mf_uindx) {
			return 0;
		}

		if (ms->md_indxflgs.mf_marty != mdfp->mf_marty) {
			return 0;
		}

		if (MF_MOCTY_FREE != ms->md_indxflgs.mf_mocty) {
			return 0;
		}

		if (PAF_NO_ALLOC != ms->md_phyadrs.paf_alloc) {
			return 0;
		}

		if ((ok + 1) != fmnr) {
			return 0;
		}

		return ok + 1;
	}

	// 对 msadsc_t 和 msadsc_t + 1的类型检测，直到遍历完me
	for (; ms < me; ms++) {
		if (ms->md_indxflgs.mf_marty != mdfp->mf_marty || (ms + 1)->md_indxflgs.mf_marty != mdfp->mf_marty) {
			return 0;
		}

		if (MF_MOCTY_FREE != ms->md_indxflgs.mf_mocty || MF_MOCTY_FREE != (ms + 1)->md_indxflgs.mf_mocty) {
			return 0;
		}

		if (ms->md_indxflgs.mf_uindx != 0 || (ms + 1)->md_indxflgs.mf_uindx != 0) {
			return 0;
		}

		if (PAF_NO_ALLOC != ms->md_phyadrs.paf_alloc || PAF_NO_ALLOC != (ms + 1)->md_phyadrs.paf_alloc) {
			return 0;
		}

		if (PAGESIZE != (((ms + 1)->md_phyadrs.paf_padrs << PSHRSIZE) - (ms->md_phyadrs.paf_padrs << PSHRSIZE))) {
			return 0;
		}

		ok++;
	}

	if (0 == ok) {
		return 0;
	}

	if ((ok + 1) != fmnr) {
		return 0;
	}

	return ok;
}

/**
 * @brief 获取地址连续的msadsc_t结构体的开始、结束地址、一共多少个msadsc_t结构体，下一次循环的fntmnr
 * 
 * @param mareap memarea地址
 * @param fmstat msadsc_t结构体的首地址
 * @param fntmsanr 已经遍历的msadsc_t的次数(包含连续地址)
 * @param fmsanr msadsc_t 结构体总长度
 * @param retmsastatp msadsc_t结构体的开始地址
 * @param retmsaendp msadsc_t结构体的结束地址
 * @param retfmnr 已经遍历到的msadsc_t的长度
 * @return bool_t 
 */
bool_t merlove_scan_continumsadsc(memarea_t *mareap, msadsc_t *fmstat, uint_t *fntmsanr, uint_t fmsanr, 
        msadsc_t **retmsastatp, msadsc_t **retmsaendp, uint_t *retfmnr)
{
	if (NULL == mareap || NULL == fmstat || NULL == fntmsanr || 0 == fmsanr || NULL == retmsastatp || NULL == retmsaendp || NULL == retfmnr) {
		return FALSE;
	}

	if (*fntmsanr >= fmsanr) {
		return FALSE;
	}

	u32_t muindx = 0;
	msadflgs_t *mdfp = NULL;
	switch (mareap->ma_type) {
        case MA_TYPE_HWAD:
        {
            muindx = MF_MARTY_HWD << 5;			// 硬件区
            mdfp = (msadflgs_t *)(&muindx);
            break;
        }

        case MA_TYPE_KRNL:
        {
            muindx = MF_MARTY_KRL << 5;			// 内核区
            mdfp = (msadflgs_t *)(&muindx);
            break;
        }

        case MA_TYPE_PROC:
        {
            muindx = MF_MARTY_PRC << 5;			// 应用区
            mdfp = (msadflgs_t *)(&muindx);
            break;
        }

        default:
        {
            muindx = 0;
            mdfp = NULL;
            break;
        }
	}

	if (0 == muindx || NULL == mdfp) {
		return FALSE;
	}

	msadsc_t *msastat = fmstat;
	uint_t retfindmnr = 0;
	bool_t rets = FALSE;
	uint_t tmidx = *fntmsanr;

	// 从外层函数的fntmnr变量开始遍历所有msadsc_t结构
	for (; tmidx < fmsanr; tmidx++) {

		// 一个msadsc_t结构是否属于这个内存区，是否空闲且没有分配的
		if (msastat[tmidx].md_indxflgs.mf_marty == mdfp->mf_marty && 0 == msastat[tmidx].md_indxflgs.mf_uindx &&
			MF_MOCTY_FREE == msastat[tmidx].md_indxflgs.mf_mocty && PAF_NO_ALLOC == msastat[tmidx].md_phyadrs.paf_alloc) {
			// 返回从这个msadsc_t结构开始到下一个非空闲、地址非连续的msadsc_t结构对应的msadsc_t结构索引号到retfindmnr变量中
			rets = scan_len_msadsc(&msastat[tmidx], mdfp, fmsanr, &retfindmnr);

			if (FALSE == rets) {
				system_error("scan_len_msadsc err\n");
			}
			// 下一轮开始的msadsc_t结构索引
			*fntmsanr = tmidx + retfindmnr + 1;
			// 当前地址连续msadsc_t结构的开始地址
			*retmsastatp = &msastat[tmidx];
			// 当前地址连续msadsc_t结构的结束地址
			*retmsaendp = &msastat[tmidx + retfindmnr];
			// 当前有多少个地址连续msadsc_t结构
			*retfmnr = retfindmnr + 1;
			return TRUE;
		}
	}

	if (tmidx >= fmsanr) {
		*fntmsanr = fmsanr;
		*retmsastatp = NULL;
		*retmsaendp = NULL;
		*retfmnr = 0;
		return TRUE;
	}

	return FALSE;
}

/**
 * @brief 给msadsc_t结构打上内存区标签
 * 
 * @param mareap 某个 memarea 结构体
 * @param mstat msadsc 的开始地址
 * @param msanr msadsc 的内存长度
 * @return uint_t 返回某个 memare下msadsc 的数量
 */
uint_t merlove_setallmarflgs_onmemarea(memarea_t *mareap, msadsc_t *mstat, uint_t msanr)
{
	if (NULL == mareap || NULL == mstat || 0 == msanr) {
		return ~0UL;
	}

	u32_t muindx = 0;
	msadflgs_t *mdfp = NULL;

	// 获取内存区类型
	switch (mareap->ma_type) {
        case MA_TYPE_HWAD:
        {
            muindx = MF_MARTY_HWD << 5;		// 硬件区标签
            mdfp = (msadflgs_t *)(&muindx);
            break;
        }

        case MA_TYPE_KRNL:
        {
            muindx = MF_MARTY_KRL << 5;		// 内核区标签
            mdfp = (msadflgs_t *)(&muindx);
            break;
        }

        case MA_TYPE_PROC:
        {
            muindx = MF_MARTY_PRC << 5;		// 应用区标签
            mdfp = (msadflgs_t *)(&muindx);
            break;
        }

        case MA_TYPE_SHAR:
        {
            return 0;
        }

        default:
        {
            muindx = 0;
            mdfp = NULL;
            break;
        }
	}

	if (0 == muindx || NULL == mdfp) {
		return ~0UL;
	}

	u64_t phyadr = 0;
	uint_t retnr = 0;

	// 扫描所有的msadsc_t结构
	for (uint_t mix = 0; mix < msanr; mix++) {
		if (MF_MARTY_INIT == mstat[mix].md_indxflgs.mf_marty) {
			// 获取msadsc_t结构对应的地址
			phyadr = mstat[mix].md_phyadrs.paf_padrs << PSHRSIZE;
			// 和内存区的地址区间比较。看msadsc_t落在哪里区域
			if (phyadr >= mareap->ma_logicstart && ((phyadr + PAGESIZE) - 1) <= mareap->ma_logicend) {
				// 设置msadsc_t结构的标签
				mstat[mix].md_indxflgs.mf_marty = mdfp->mf_marty;
				retnr++;
			}
		}
	}
	return retnr;
}

uint_t test_setflgs(memarea_t *mareap, msadsc_t *mstat, uint_t msanr)
{
	u32_t muindx = 0;
	msadflgs_t *mdfp = NULL;
	if (NULL == mareap || NULL == mstat || 0 == msanr) {
		return ~0UL;
	}

	switch (mareap->ma_type) {
        case MA_TYPE_HWAD:
        {
            muindx = MF_MARTY_HWD << 5;		// 硬件区标签
            mdfp = (msadflgs_t *)(&muindx);
            break;
        }
        case MA_TYPE_KRNL:
        {
            muindx = MF_MARTY_KRL << 5;		// 内核区标签
            mdfp = (msadflgs_t *)(&muindx);
            break;
        }
        case MA_TYPE_PROC:
        {
            muindx = MF_MARTY_PRC << 5;		// 应用区标签
            mdfp = (msadflgs_t *)(&muindx);
            break;
        }
        case MA_TYPE_SHAR:
        {
            return 0;
        }
        default:
        {
            muindx = 0;
            mdfp = NULL;
            break;
        }
	}

	if (0 == muindx || NULL == mdfp) {
		return ~0UL;
	}

	u64_t phyadr = 0;
	uint_t retnr = 0;

	for (uint_t mix = 0; mix < msanr; mix++) {
		phyadr = mstat[mix].md_phyadrs.paf_padrs << PSHRSIZE;
		if (phyadr >= mareap->ma_logicstart && ((phyadr + PAGESIZE) - 1) <= mareap->ma_logicend) {
			if (mstat[mix].md_indxflgs.mf_marty == mdfp->mf_marty) {
				retnr++;
			}		
		}
	}
	return retnr;
}

/**
 * @brief 根据地址连续的msadsc_t结构的数量查找合适bafhlst_t结构
 * 	1. 如果是用户区，页就被单独一个一个的被加入到 dm_onemsalst 中，而不是按组进行分割
 * 	2. 用户区的页面一般是 lazy allocated，用到一个页面分配一个页面
 * 
 * @param mareap memarea 内存区
 * @param fmnr 连续长度
 * @return bafhlst_t* 返回bafhlst首地址
 */
bafhlst_t* find_continumsa_inbafhlst(memarea_t *mareap, uint_t fmnr)
{
	bafhlst_t *retbafhp = NULL;
	uint_t in = 0;
	if (NULL == mareap || 0 == fmnr) {
		return NULL;
	}

	// 如果是用户区， 直接返回第一个
	if (MA_TYPE_PROC == mareap->ma_type) {
		return &mareap->ma_mdmdata.dm_onemsalst;
	}

	if (MA_TYPE_SHAR == mareap->ma_type) {
		return NULL;
	}

	in = 0;
	retbafhp = NULL;
	for (uint_t li = 0; li < MDIVMER_ARR_LMAX; li++) {
		if ((mareap->ma_mdmdata.dm_mdmlielst[li].af_oderpnr) <= fmnr) {
			retbafhp = &mareap->ma_mdmdata.dm_mdmlielst[li];
			in++;
		}
	}

	if (MDIVMER_ARR_LMAX <= in || NULL == retbafhp) {
		return NULL;
	}

	return retbafhp;
}

/**
 * @brief 应用区bafhlst挂载msadsc
 * 
 * @param mareap 内存区地址
 * @param bafhp bafhlst地址
 * @param fstat msadsc开始地址
 * @param fend msadsc结束地址
 * @param fmnr msads连续长度
 * @return bool_t 
 */
bool_t continumsadsc_add_procmareabafh(memarea_t *mareap, bafhlst_t *bafhp, msadsc_t *fstat, msadsc_t *fend, uint_t fmnr)
{
	if (NULL == mareap || NULL == bafhp || NULL == fstat || NULL == fend || 0 == fmnr) {
		return FALSE;
	}

	if (BAFH_STUS_ONEM != bafhp->af_stus || MA_TYPE_PROC != mareap->ma_type) {
		return FALSE;
	}

	if (bafhp->af_oderpnr != 1) {
		return FALSE;
	}

	if ((&fstat[fmnr - 1]) != fend) {
		return FALSE;
	}

	for (uint_t tmpnr = 0; tmpnr < fmnr; tmpnr++) {
		fstat[tmpnr].md_indxflgs.mf_olkty = MF_OLKTY_BAFH;
		fstat[tmpnr].md_odlink = bafhp;
		list_add(&fstat[tmpnr].md_list, &bafhp->af_frelst);
		bafhp->af_fobjnr++;
		bafhp->af_mobjnr++;
		mareap->ma_maxpages++;
		mareap->ma_freepages++;
		mareap->ma_allmsadscnr++;
	}

	return TRUE;
}

/**
 * @brief 根据地址连续的msadsc_t结构挂载到bafhlst_t结构中
 * 
 * @param mareap memarea 内存区地址
 * @param bafhp bafhlst 内存地址
 * @param fstat msadsc 开始地址
 * @param fend msadsc 结束地址
 * @param fmnr bafhlst 最长长度
 * @return bool_t 
 */
bool_t continumsadsc_add_bafhlst(memarea_t *mareap, bafhlst_t *bafhp, msadsc_t *fstat, msadsc_t *fend, uint_t fmnr)
{
	if (NULL == mareap || NULL == bafhp || NULL == fstat || NULL == fend || 0 == fmnr) {
		return FALSE;
	}

	if (bafhp->af_oderpnr != fmnr) {
		return FALSE;
	}

	if ((&fstat[fmnr - 1]) != fend) {
		return FALSE;
	}

	fstat->md_indxflgs.mf_olkty = MF_OLKTY_ODER;
	// 开始的msadsc_t结构指向最后的msadsc_t结构
	fstat->md_odlink = fend;
	// fstat == fend
	fend->md_indxflgs.mf_olkty = MF_OLKTY_BAFH;
	// 最后的msadsc_t结构指向它属于的bafhlst_t结构
	fend->md_odlink = bafhp;
	// 把多个地址连续的msadsc_t结构的的开始的那个msadsc_t结构挂载到bafhlst_t结构的af_frelst中
	list_add(&fstat->md_list, &bafhp->af_frelst);

	// 更新bafhlst_t的统计数据
	bafhp->af_fobjnr++;
	bafhp->af_mobjnr++;

	// 更新内存区的统计数据
	mareap->ma_maxpages += fmnr;
	mareap->ma_freepages += fmnr;
	mareap->ma_allmsadscnr += fmnr;
	return TRUE;
}

/**
 * @brief 为一段地址连续的msadsc_t结构寻找合适m_mdmlielst数组中的bafhlst_t结构
 * 
 * @param mareap 内存区
 * @param rfstat msadsc开始地址
 * @param rfend msadsc结束地址
 * @param rfmnr msads连续长度(还剩多少个地址连续长度)
 * @return bool_t 
 */
bool_t continumsadsc_mareabafh_core(memarea_t *mareap, msadsc_t **rfstat, msadsc_t **rfend, uint_t *rfmnr)
{

	if (NULL == mareap || NULL == rfstat || NULL == rfend || NULL == rfmnr) {
		return FALSE;
	}
    
	uint_t retval = *rfmnr, tmpmnr = 0;
	msadsc_t *mstat = *rfstat, *mend = *rfend;

	if (1 > (retval)) {
		return FALSE;
	}

	// 根据地址连续的msadsc_t结构的数量查找合适bafhlst_t结构
	bafhlst_t *bafhp = find_continumsa_inbafhlst(mareap, retval);
	if (NULL == bafhp) {
		return FALSE;
	}

	if (retval < bafhp->af_oderpnr) {
		return FALSE;
	}

	// 判断bafhlst_t结构状态和类型对不对
	if ((BAFH_STUS_DIVP == bafhp->af_stus || BAFH_STUS_DIVM == bafhp->af_stus) && MA_TYPE_PROC != mareap->ma_type) {
		// 看地址连续的msadsc_t结构的数量是不是正好是bafhp->af_oderpnr
		tmpmnr = retval - bafhp->af_oderpnr;
		// 根据地址连续的msadsc_t结构挂载到bafhlst_t结构中
		if (continumsadsc_add_bafhlst(mareap, bafhp, mstat, &mstat[bafhp->af_oderpnr - 1], bafhp->af_oderpnr) == FALSE) {
			return FALSE;
		}

		// 如果地址连续的msadsc_t结构的数量正好是bafhp->af_oderpnr则完成，否则返回再次进入此函数
		if (tmpmnr == 0) {
			*rfmnr = tmpmnr;
			*rfend = NULL;
			return TRUE;
		}

		// 挂载bafhp->af_oderpnr地址连续的msadsc_t结构到bafhlst_t中
		*rfstat = &mstat[bafhp->af_oderpnr];
		// 还剩多少个地址连续的msadsc_t结构
		*rfmnr = tmpmnr;

		return TRUE;
	}

	// 应用区判断
	if (BAFH_STUS_ONEM == bafhp->af_stus && MA_TYPE_PROC == mareap->ma_type) {
		if (continumsadsc_add_procmareabafh(mareap, bafhp, mstat, mend, *rfmnr) == FALSE) {
			return FALSE;
		}
		*rfmnr = 0;
		*rfend = NULL;
		return TRUE;
	}

	return FALSE;
}

/**
 * @brief 把msadsc_t 结构体 挂载到合适的 m_mdmlielst 数组中的 bafhlst_t 结构中
 * 
 * @param mareap 内存区地址
 * @param mstat msadsc 开始地址
 * @param mend msadsc 结束地址
 * @param mnr 连续 msadsc 的长度
 * @return bool_t 
 */
bool_t merlove_continumsadsc_mareabafh(memarea_t *mareap, msadsc_t *mstat, msadsc_t *mend, uint_t mnr)
{
	if (NULL == mareap || NULL == mstat || NULL == mend || 0 == mnr) {
		return FALSE;
	}

	uint_t mnridx = mnr;
	msadsc_t *fstat = mstat, *fend = mend;

	// 如果mnridx > 0并且NULL != fend就循环调用continumsadsc_mareabafh_core函数，而mnridx和fend由这个函数控制
	for (; (mnridx > 0 && NULL != fend);) {
		// 为一段地址连续的msadsc_t结构寻找合适m_mdmlielst数组中的bafhlst_t结构
		if (continumsadsc_mareabafh_core(mareap, &fstat, &fend, &mnridx) == FALSE) {
			system_error("continumsadsc_mareabafh_core fail\n");
		}
	}

	return TRUE;
}

/**
 * @brief  对每一个内存区进行 msadsc_t 结构的合并操作
 * 
 * @param mareap 某个memarea结构体
 * @param mstat msadsc 开始地址
 * @param msanr msadsc 的长度
 * @return bool_t 
 */
bool_t merlove_mem_onmemarea(memarea_t *mareap, msadsc_t *mstat, uint_t msanr)
{
	if (NULL == mareap || NULL == mstat || 0 == msanr) {
		return FALSE;
	}

	if (MA_TYPE_SHAR == mareap->ma_type) {
		return TRUE;
	}

	if (MA_TYPE_INIT == mareap->ma_type) {
		return FALSE;
	}

	msadsc_t *retstatmsap = NULL, *retendmsap = NULL, *fntmsap = mstat;
	uint_t retfindmnr = 0;
	uint_t fntmnr = 0;
	bool_t retscan = FALSE;

	/**
	 * 遍历 memarea_t 数组，然后对应单个memarea_t类型中对应的连续的msadsc_t有多少个
	 */
	for (; fntmnr < msanr;) {
		// 获取最多且地址连续的msadsc_t结构体的开始、结束地址、一共多少个msadsc_t结构体，下一次循环的fntmnr
		retscan = merlove_scan_continumsadsc(mareap, fntmsap, &fntmnr, msanr, &retstatmsap, &retendmsap, &retfindmnr);
		if (FALSE == retscan) {
			system_error("merlove_scan_continumsadsc fail\n");
		}

		if (NULL != retstatmsap && NULL != retendmsap) {
			if (check_continumsadsc(mareap, retstatmsap, retendmsap, retfindmnr) == 0) {
				system_error("check_continumsadsc fail\n");
			}

			// 把一组连续的msadsc_t结构体挂载到合适的m_mdmlielst数组中的bafhlst_t结构中
			if (merlove_continumsadsc_mareabafh(mareap, retstatmsap, retendmsap, retfindmnr) == FALSE) {
				system_error("merlove_continumsadsc_mareabafh fail\n");
			}
		}
	}

	return TRUE;
}

/**
 * @brief merlove_mem_core 两次遍历内存区
 * 	1. 第一次遍历是为了完成上述第一步：确定内存页属于哪个区
 * 	2. 当确定内存页属于哪个区之后，就来到了第二次遍历 memarea_t 结构，合并其中的 msadsc_t 结构，并把它们挂载到其中的 memdivmer_t 结构下的 dm_mdmlielst 数组中
 * 		1. 它要保证其中所有的 msadsc_t 结构挂载到 dm_mdmlielst 数组中合适的 bafhlst_t 结构中
 * 		2. 它要保证多个 msadsc_t 结构有最大的连续性
 * 		
 * @param mbsp 
 * @return bool_t 
 */
bool_t merlove_mem_core(machbstart_t *mbsp)
{
	// 获取msadsc_t结构的首地址
	msadsc_t *mstatp = (msadsc_t *)phyadr_to_viradr((adr_t)mbsp->mb_memmappadr);
	// 获取msadsc_t结构的个数
	uint_t msanr = (uint_t)mbsp->mb_memmapnr, maxp = 0;
	// 获取memarea_t结构的首地址
	memarea_t *marea = (memarea_t *)phyadr_to_viradr((adr_t)mbsp->mb_memznpadr);
	uint_t sretf = ~0UL, tretf = ~0UL;

	// 遍历每个memarea_t结构
	for (uint_t mi = 0; mi < (uint_t)mbsp->mb_memznnr; mi++) {
		// 针对其中一个memarea_t结构给msadsc_t结构打上内存区标签
		sretf = merlove_setallmarflgs_onmemarea(&marea[mi], mstatp, msanr);
		if ((~0UL) == sretf) {
			return FALSE;
		}

		tretf = test_setflgs(&marea[mi], mstatp, msanr);
		if ((~0UL) == tretf) {
			return FALSE;
		}

		if (sretf != tretf) {
			return FALSE;
		}
	}

	// 遍历每个memarea_t结构
	for (uint_t maidx = 0; maidx < (uint_t)mbsp->mb_memznnr; maidx++) {
		// 针对其中一个memarea_t结构对msadsc_t结构进行合并
		if (merlove_mem_onmemarea(&marea[maidx], mstatp, msanr) == FALSE) {
			return FALSE;
		}
		maxp += marea[maidx].ma_maxpages;
	}

	return TRUE;
}

uint_t check_multi_msadsc(msadsc_t *mstat, bafhlst_t *bafhp, memarea_t *mareap)
{
	if (NULL == mstat || NULL == bafhp || NULL == mareap) {
		return 0;
	}

	if (MF_OLKTY_ODER != mstat->md_indxflgs.mf_olkty && MF_OLKTY_BAFH != mstat->md_indxflgs.mf_olkty) {
		return 0;
	}

	if (NULL == mstat->md_odlink) {
		return 0;
	}

	msadsc_t *mend = NULL;
	if (MF_OLKTY_ODER == mstat->md_indxflgs.mf_olkty) {
		mend = (msadsc_t *)mstat->md_odlink;
	}

	if (MF_OLKTY_BAFH == mstat->md_indxflgs.mf_olkty) {
		mend = mstat;
	}

	if (NULL == mend) {
		return 0;
	}

	uint_t mnr = (mend - mstat) + 1;
	if (mnr != bafhp->af_oderpnr) {
		return 0;
	}

	if (MF_OLKTY_BAFH != mend->md_indxflgs.mf_olkty) {
		return 0;
	}

	if ((bafhlst_t *)(mend->md_odlink) != bafhp) {
		return 0;
	}

	u64_t phyadr = (~0UL);
	if (mnr == 1) {
		if (mstat->md_indxflgs.mf_marty != (u32_t)mareap->ma_type) {
			return 0;
		}

		if (PAF_NO_ALLOC != mstat->md_phyadrs.paf_alloc || 0 != mstat->md_indxflgs.mf_uindx) {
			return 0;
		}

		phyadr = mstat->md_phyadrs.paf_padrs << PSHRSIZE;
		if (phyadr < mareap->ma_logicstart || (phyadr + PAGESIZE - 1) > mareap->ma_logicend) {
			return 0;
		}
		return 1;
	}

	uint_t idx = 0;
	for (uint_t mi = 0; mi < mnr - 1; mi++) {
		if (mstat[mi].md_indxflgs.mf_marty != (u32_t)mareap->ma_type) {
			return 0;
		}

		if (PAF_NO_ALLOC != mstat[mi].md_phyadrs.paf_alloc || 0 != mstat[mi].md_indxflgs.mf_uindx) {
			return 0;
		}

		if (PAF_NO_ALLOC != mstat[mi + 1].md_phyadrs.paf_alloc || 0 != mstat[mi + 1].md_indxflgs.mf_uindx) {
			return 0;
		}

		if (((mstat[mi].md_phyadrs.paf_padrs << PSHRSIZE) + PAGESIZE) != (mstat[mi + 1].md_phyadrs.paf_padrs << PSHRSIZE)) {
			return 0;
		}

		if ((mstat[mi].md_phyadrs.paf_padrs << PSHRSIZE) < mareap->ma_logicstart ||
			(((mstat[mi + 1].md_phyadrs.paf_padrs << PSHRSIZE) + PAGESIZE) - 1) > mareap->ma_logicend) {
			return 0;
		}

		idx++;
	}
	return idx + 1;
}

bool_t check_one_bafhlst(bafhlst_t *bafhp, memarea_t *mareap)
{
	if (NULL == bafhp || NULL == mareap) {
		return FALSE;
	}

	if (1 > bafhp->af_mobjnr && 1 > bafhp->af_fobjnr) {
		return TRUE;
	}

	uint_t lindx = 0;
	list_h_t *tmplst = NULL;
	msadsc_t *msap = NULL;

	list_for_each(tmplst, &bafhp->af_frelst) {
		msap = list_entry(tmplst, msadsc_t, md_list);

		if (bafhp->af_oderpnr != check_multi_msadsc(msap, bafhp, mareap)) {
			return FALSE;
		}
		lindx++;
	}

	if (lindx != bafhp->af_fobjnr || lindx != bafhp->af_mobjnr) {
		return FALSE;
	}

	return TRUE;
}

bool_t check_one_memarea(memarea_t *mareap)
{
	if (NULL == mareap) {
		return FALSE;
	}

	if (1 > mareap->ma_maxpages) {
		return TRUE;
	}

	for (uint_t li = 0; li < MDIVMER_ARR_LMAX; li++) {
		if (check_one_bafhlst(&mareap->ma_mdmdata.dm_mdmlielst[li], mareap) == FALSE) {
			return FALSE;
		}
	}

	if (check_one_bafhlst(&mareap->ma_mdmdata.dm_onemsalst, mareap) == FALSE) {
		return FALSE;
	}

	return TRUE;
}

void mem_check_mareadata(machbstart_t *mbsp)
{
	memarea_t *marea = (memarea_t *)phyadr_to_viradr((adr_t)mbsp->mb_memznpadr);
	for (uint_t maidx = 0; maidx < mbsp->mb_memznnr; maidx++) {
		if (check_one_memarea(&marea[maidx]) == FALSE) {
			system_error("check_one_memarea fail\n");
		}
	}

	return;
}

// 初始化页面合并
void init_merlove_mem() {

	if (merlove_mem_core(&kmachbsp) == FALSE) {
		system_error("merlove_mem_core fail\n");
	}
	mem_check_mareadata(&kmachbsp);
	return;
}

void disp_bafhlst(bafhlst_t *bafhp)
{
	if (bafhp->af_mobjnr > 0) {
		kprint("bafhlst_t.af_stus:%x,af_indx:%x,af_onebnr:%x,af_fobjnr:%x\n",
			   bafhp->af_stus, bafhp->af_oder, bafhp->af_oderpnr, bafhp->af_fobjnr);
	}

	return;
}

void disp_memarea(machbstart_t *mbsp)
{
	memarea_t *marea = (memarea_t *)phyadr_to_viradr((adr_t)mbsp->mb_memznpadr);
	for (uint_t i = 0; i < mbsp->mb_memznnr; i++) {
		kprint("memarea.ma_type:%x,ma_maxpages:%x,ma_freepages:%x,ma_allmsadscnr:%x\n",
			   marea[i].ma_type, marea[i].ma_maxpages, marea[i].ma_freepages, marea[i].ma_allmsadscnr);

		for (uint_t li = 0; li < MDIVMER_ARR_LMAX; li++) {
			disp_bafhlst(&marea[i].ma_mdmdata.dm_mdmlielst[li]);
		}

		disp_bafhlst(&marea[i].ma_mdmdata.dm_onemsalst);
	}
	return;
}
