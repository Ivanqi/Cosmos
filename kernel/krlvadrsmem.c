/**********************************************************
    内核虚拟地址空间文件kvadrsmem.c
***********************************************************/
#include "cosmostypes.h"
#include "cosmosmctrl.h"

void teststc_t_init(teststc_t *initp)
{
	list_init(&initp->tst_list);
	initp->tst_vadr = 0;
	initp->tst_vsiz = 0;
	initp->tst_type = 0;
	initp->tst_lime = 0;
	return;
}

teststc_t *new_teststc()
{
	teststc_t *t = (teststc_t *)kmsob_new(sizeof(teststc_t));
	if (NULL == t) {
		return NULL;
	}

	teststc_t_init(t);
	return t;
}

void del_teststc(teststc_t *delstc)
{
	if ((NULL != delstc)) {
		teststc_t_init(delstc);
		if (TRUE == kmsob_delete((void *)delstc, sizeof(teststc_t))) {
			return;
		}
	}

	system_error("del_teststc err\n");
	return;
}

void add_new_teststc(adr_t vadr, size_t vsiz)
{
	if (NULL == vadr || 1 > vsiz) {
		system_error("add_new_teststc parm err\n");
	}

	teststc_t *t = NULL;
	t = new_teststc();
	if (NULL == t) {
		system_error("add_new_teststc new_teststc NULL\n");
	}

	t->tst_vadr = vadr;
	t->tst_vsiz = vsiz; 
	list_add(&t->tst_list, &krlvirmemadrs.kvs_testhead);
	krlvirmemadrs.kvs_tstcnr++;
	return;
}

void vaslknode_t_init(vaslknode_t *initp)
{
	if (NULL == initp) {
		system_error("vaslknode_t_init pram err\n");
	}

	initp->vln_color = 0;
	initp->vln_flags = 0;
	initp->vln_left = NULL;
	initp->vln_right = NULL;
	initp->vln_prev = NULL;
	initp->vln_next = NULL;
	return;
}

void pgtabpage_t_init(pgtabpage_t *initp)
{
	krlspinlock_init(&initp->ptp_lock);
	list_init(&initp->ptp_msalist);
	initp->ptp_msanr = 0;
	return;
}

void virmemadrs_t_init(virmemadrs_t *initp)
{
	if (NULL == initp) {
		return;
	}

	krlspinlock_init(&initp->vs_lock);
	initp->vs_resalin = 0;
	list_init(&initp->vs_list);
	initp->vs_flgs = 0;
	initp->vs_kmvdscnr = 0;
	initp->vs_mm = NULL;
	initp->vs_startkmvdsc = 0;		// 开始的虚拟地址区间
	initp->vs_endkmvdsc = NULL;		// 结束的虚拟地址区间
	initp->vs_currkmvdsc = NULL;	// 当前的虚拟地址区间
	initp->vs_krlmapdsc = NULL;
	initp->vs_krlhwmdsc = NULL;
	initp->vs_krlolddsc = NULL;
	initp->vs_heapkmvdsc = NULL;
	initp->vs_stackkmvdsc = NULL;

	initp->vs_isalcstart = 0;		// 能分配的开始虚拟地址
	initp->vs_isalcend = 0;			// 能分配的结束虚拟地址

	initp->vs_privte = 0;			// 私有数据指针
	initp->vs_ext = 0;				// 扩展数据指针
	return;
}

void kmvarsdsc_t_init(kmvarsdsc_t *initp)
{
	if (NULL == initp) {
		system_error("kmvarsdsc_t_init pram err\n");
	}

	krlspinlock_init(&initp->kva_lock);
	initp->kva_maptype = 0;
	list_init(&initp->kva_list);
	initp->kva_flgs = 0;
	initp->kva_limits = 0;
	vaslknode_t_init(&initp->kva_lknode);
	initp->kva_mcstruct = NULL;
	initp->kva_start = 0;
	initp->kva_end = 0;
	initp->kva_kvmbox = NULL;
	initp->kva_kvmcobj = NULL;
	return;
}

void kvirmemadrs_t_init(kvirmemadrs_t *initp)
{
	if (NULL == initp) {
		system_error("kvirmemadrs_t_init pram err\n");
	}

	krlspinlock_init(&initp->kvs_lock);
	initp->kvs_flgs = 0;
	initp->kvs_kmvdscnr = 0;
	initp->kvs_startkmvdsc = NULL;
	initp->kvs_endkmvdsc = NULL;
	initp->kvs_krlmapdsc = NULL;
	initp->kvs_krlhwmdsc = NULL;
	initp->kvs_krlolddsc = NULL;
	initp->kvs_isalcstart = 0;
	initp->kvs_isalcend = 0;
	initp->kvs_privte = NULL;
	initp->kvs_ext = NULL;

	list_init(&initp->kvs_testhead);
	initp->kvs_tstcnr = 0;
	initp->kvs_randnext = 1;

	pgtabpage_t_init(&initp->kvs_ptabpgcs);
	kvmcobjmgr_t_init(&initp->kvs_kvmcomgr);
	kvmemcboxmgr_t_init(&initp->kvs_kvmemcboxmgr);

	return;
}

/**
 * @brief 创建一个kmvarsdsc_t
 * 
 * @return kmvarsdsc_t* 
 */
kmvarsdsc_t* new_kmvarsdsc()
{
	kmvarsdsc_t *kmvdc = NULL;
	kmvdc = (kmvarsdsc_t *)kmsob_new(sizeof(kmvarsdsc_t));
	if (NULL == kmvdc) {
		return NULL;
	}

	kmvarsdsc_t_init(kmvdc);
	return kmvdc;
}

/**
 * @brief kmvarsdsc 删除
 *  1. delkmvd 对应的内存删除
 * 
 * @param delkmvd 
 * @return bool_t 
 */
bool_t del_kmvarsdsc(kmvarsdsc_t *delkmvd)
{
	if (NULL == delkmvd) {
		return FALSE;
	}

	return kmsob_delete((void *)delkmvd, sizeof(kmvarsdsc_t));
}

virmemadrs_t *new_virmemadrs()
{
	virmemadrs_t *vmdsc = NULL;
	vmdsc = (virmemadrs_t *)kmsob_new(sizeof(virmemadrs_t));
	if (NULL == vmdsc) {
		return NULL;
	}

	virmemadrs_t_init(vmdsc);
	return vmdsc;
}

bool_t del_virmemadrs(virmemadrs_t *vmdsc)
{
	if (NULL == vmdsc) {
		return FALSE;
	}

	return kmsob_delete((void *)vmdsc, sizeof(virmemadrs_t));
}

/**
 * @brief 初始化内核空间区和线性映射区
 * 	1. 设置内存空间区地址
 * 	2. 设置线性映射区
 * 	3. 把线性映射区挂载到内存空间区中
 * 
 * @param kvma 
 */
void kvma_seting_kvirmemadrs(kvirmemadrs_t *kvma)
{
	kmvarsdsc_t *kmvdc = NULL;
	if (NULL == kvma) {
		system_error("kvma_seting_kvirmemadrs parm err\n");
	}

	kmvdc = new_kmvarsdsc();
	if (NULL == kmvdc) {
		system_error("kvma_seting_kvirmemadrs nomem err\n");
	}

	// 内核空间区
	kvma->kvs_isalcstart = KRNL_VIRTUAL_ADDRESS_START + KRNL_MAP_VIRTADDRESS_SIZE;
	kvma->kvs_isalcend = KRNL_VIRTUAL_ADDRESS_END;
	// 线性映射区
	kmvdc->kva_start = KRNL_VIRTUAL_ADDRESS_START;
	kmvdc->kva_end = KRNL_VIRTUAL_ADDRESS_START + KRNL_MAP_VIRTADDRESS_SIZE;
	kmvdc->kva_mcstruct = kvma;

	kvma->kvs_startkmvdsc = kmvdc;
	kvma->kvs_endkmvdsc = kmvdc;
	kvma->kvs_krlmapdsc = kmvdc;

	kvma->kvs_kmvdscnr++;
	return;
}

/**
 * @brief 建立了一个虚拟地址区间和一个栈区，栈区位于虚拟地址空间的顶端
 * 	1. vma 的 0 ～ 0x00007fffffffffff 设置为可分配的虚拟地址
 * 	2. vma 设置两个区域。 虚拟地址区间：kmvdc(0x1000 ~ 0x5000), 栈区：stackkmvdc(0x00007FFFBFFFFFFF ～ 0x00007fffffffffff)
 * 
 * @param vma 
 * @return bool_t 
 */
bool_t kvma_inituserspace_virmemadrs(virmemadrs_t *vma)
{
	kmvarsdsc_t *kmvdc = NULL, *stackkmvdc = NULL, *heapkmvdc = NULL;
	// 分配一个kmvarsdsc_t
	if (NULL == vma) {
		return FALSE;
	}

	kmvdc = new_kmvarsdsc();
	if (NULL == kmvdc) {
		return FALSE;
	}

	heapkmvdc = new_kmvarsdsc();
	if (NULL == heapkmvdc) {
		del_kmvarsdsc(kmvdc);
		return FALSE;
	}

	// 分配一个栈区的kmvarsdsc_t
	stackkmvdc = new_kmvarsdsc();
	if (NULL == stackkmvdc) {
		del_kmvarsdsc(kmvdc);
		del_kmvarsdsc(heapkmvdc);
		return FALSE;
	}

	// 虚拟区间开始地址0x1000
	kmvdc->kva_start = USER_VIRTUAL_ADDRESS_START + 0x1000;
	// 虚拟区间结束地址0x5000
	kmvdc->kva_end = kmvdc->kva_start + 0x4000;
	kmvdc->kva_mcstruct = vma;	// kmvdc的上层结构为vma

	heapkmvdc->kva_start = THREAD_HEAPADR_START;
	heapkmvdc->kva_end = heapkmvdc->kva_start + 0x1000;
	heapkmvdc->kva_mcstruct = vma;
	heapkmvdc->kva_maptype = KMV_HEAP_TYPE;

	// 0x00007FFFBFFFFFFF ～ 0x00007fffffffffff
	// 栈虚拟区间开始地址0x1000USER_VIRTUAL_ADDRESS_END - 0x40000000
	stackkmvdc->kva_start = PAGE_ALIGN(USER_VIRTUAL_ADDRESS_END - 0x40000000);
	// 栈虚拟区间结束地址0x1000USER_VIRTUAL_ADDRESS_END
	stackkmvdc->kva_end = USER_VIRTUAL_ADDRESS_END;
	stackkmvdc->kva_mcstruct = vma;	// stackkmvdc 的 上层结构为vma
	stackkmvdc->kva_maptype = KMV_STACK_TYPE;

	krlspinlock_lock(&vma->vs_lock);

	// 0 ～ 0x00007fffffffffff
	vma->vs_isalcstart = USER_VIRTUAL_ADDRESS_START;
	vma->vs_isalcend = USER_VIRTUAL_ADDRESS_END;
	// 设置虚拟地址空间的开始区间为kmvdc
	vma->vs_startkmvdsc = kmvdc;
	// 设置虚拟地址空间的开始区间为栈区 
	vma->vs_endkmvdsc = stackkmvdc;
	vma->vs_heapkmvdsc = heapkmvdc;
	vma->vs_stackkmvdsc = stackkmvdc;

	// 加入链表
	list_add_tail(&kmvdc->kva_list, &vma->vs_list);
	list_add_tail(&heapkmvdc->kva_list, &vma->vs_list);
	list_add_tail(&stackkmvdc->kva_list, &vma->vs_list);
	// 计数加3
	vma->vs_kmvdscnr += 3;
	krlspinlock_unlock(&vma->vs_lock);
	return TRUE;
}

adr_t kvma_initdefault_virmemadrs(mmadrsdsc_t* mm, adr_t start, size_t size, u32_t type)
{
	if(0x1000 >= start || 0 >= size || NULL == mm) {
		return NULL;
	}
	return vma_new_vadrs(mm, start, size, 0, type);
}

/**
 * @brief 虚拟内存管理结构初始化
 * 
 * @param initp 虚拟内存管理结构体
 */
void mmadrsdsc_t_init(mmadrsdsc_t* initp)
{
	if (NULL == initp) {
		return;
	}

	krlspinlock_init(&initp->msd_lock);
	list_init(&initp->msd_list);
	initp->msd_flag = 0;
	initp->msd_stus = 0;
	initp->msd_scount = 0;
	initp->msd_thread = NULL;

	// 信号设置
	krlsem_t_init(&initp->msd_sem);
	krlsem_set_sem(&initp->msd_sem, SEM_FLG_MUTEX, SEM_MUTEX_ONE_LOCK);

	// mmu设置
	mmudsc_t_init(&initp->msd_mmu);

	// 虚拟内存设置
	virmemadrs_t_init(&initp->msd_virmemadrs);
	
	initp->msd_stext = 0;
	initp->msd_etext = 0;
	initp->msd_sdata = 0;	 // 应用的数据去的开始、结束地址
	initp->msd_edata = 0;
	initp->msd_sbss = 0;
	initp->msd_ebss = 0;
	initp->msd_sbrk = 0;
	initp->msd_ebrk = 0;
	return; 
}

/**
 * @brief 创建虚拟内存
 * 
 * @return mmadrsdsc_t* 
 */
mmadrsdsc_t* new_mmadrsdsc()
{
	mmadrsdsc_t* mm;
	mm = (mmadrsdsc_t*)kmsob_new(sizeof(mmadrsdsc_t));
	if (NULL == mm) {
		return NULL;
	}

	mmadrsdsc_t_init(mm);
	kvma_inituserspace_virmemadrs(&mm->msd_virmemadrs);
	hal_mmu_init(&mm->msd_mmu);
	return mm;
}

/**
 * @brief 删除虚拟地址
 * 
 */
bool_t del_mmadrsdsc(mmadrsdsc_t *mm) 
{
	if (NULL == mm) {
		return FALSE;
	}

	if (hal_mmu_clean(&mm->msd_mmu) == FALSE) {
		return FALSE;
	}

	return kmsob_delete((void*)mm, sizeof(mmadrsdsc_t));
}

// 测试函数
void test_vadr()
{
	// 分配一个0x1000大小的虚拟地址空间
	adr_t vadr = vma_new_vadrs(&initmmadrsdsc, NULL, 0x1000, 0, 0);
	// 返回NULL表示分配失败
	if (NULL == vadr) {
		kprint("分配虚拟地址空间失败\n");
	}

	// 在刷屏幕上打印分配虚拟地址空间的开始地址
	kprint("分配虚拟地址空间地址:%x\n", vadr);
	kprint("开始写入分配虚拟地址空间\n");
	// 访问虚拟地址空间，把这空间全部设置为0 
	hal_memset((void*)vadr, 0, 0x1000);
	kprint("结束写入分配虚拟地址空间\n");
	return;
}

void init_kvirmemadrs()
{
	// 初始化mmadrsdsc_t结构
	mmadrsdsc_t_init(&initmmadrsdsc);

	kvirmemadrs_t_init(&krlvirmemadrs);

	// 初始化内核空间区和线性映射区
	kvma_seting_kvirmemadrs(&krlvirmemadrs);
	// 初始化进程的用户空间.建立了一个虚拟地址区间和一个栈区，栈区位于虚拟地址空间的顶端
	kvma_inituserspace_virmemadrs(&initmmadrsdsc.msd_virmemadrs);

	hal_mmu_init(&initmmadrsdsc.msd_mmu);
	hal_mmu_load(&initmmadrsdsc.msd_mmu);

	// test_vadr();
	kprint("虚拟内存初始化成功\n");
    // die(0x400);
	return;
}

/**
 * @brief 检查kmvarsdsc_t结构
 * 	1. 通过判断当前kmvarsdsc结构体的待分配的内存是否大于要分配的内存
 * 	2. 如果待分配内存不够，就需要切换下一个kmvarsdsc指针
 * 
 * @param vmalocked 
 * @param curr 当前的虚拟地址区间
 * @param start 需要分配的内存开始地址
 * @param vassize 需要分配的内存长度
 * @return kmvarsdsc_t* 
 */
kmvarsdsc_t* vma_find_kmvarsdsc_is_ok(virmemadrs_t *vmalocked, kmvarsdsc_t *curr, adr_t start, size_t vassize)
{
	kmvarsdsc_t *nextkmvd = NULL;
	adr_t newend = start + (adr_t)vassize;

	// 如果curr不是最后一个,先检查当前kmvarsdsc_t结构
	if (list_is_last(&curr->kva_list, &vmalocked->vs_list) == FALSE) {
		// 就获取curr的下一个kmvarsdsc_t结构
		nextkmvd = list_next_entry(curr, kmvarsdsc_t, kva_list);

		// 由系统动态决定分配虚拟空间的开始地址
		if (NULL == start) {
			// 如果curr的结束地址加上分配的大小小于等于下一个kmvarsdsc_t结构的开始地址就返回curr
			if ((curr->kva_end + (adr_t)vassize) <= nextkmvd->kva_start) {
				return curr;
			}
		} else {
			// 否则比较应用指定分配的开始，结束地址是不是在curr和下一个kmvarsdsc_t结构之间
			if ((curr->kva_end <= start) && (newend <= nextkmvd->kva_start)) {
				return curr;
			}
		}
	} else {
		// 否则curr为最后一个kmvarsdsc_t结构
		if (NULL == start) {
			// curr的结束地址加上分配空间的大小是不是小于整个虚拟地址空间
			if ((curr->kva_end + (adr_t)vassize) < vmalocked->vs_isalcend) {
				return curr;
			}
		} else {
			// 否则比较应用指定分配的开始、结束地址是不是在curr的结束地址和整个虚拟地址空间的结束地址之间
			if ((curr->kva_end <= start) && (newend < vmalocked->vs_isalcend)) {
				return curr;
			}
		}
	}

	return NULL;
}

/**
 * @brief 根据分配的开始地址和大小，在 virmemadrs_t 结构中查找相应的 kmvarsdsc_t 结构
 * 	1. 比如 virmemadrs_t 结构中有两个 kmvarsdsc_t 结构，A_kmvarsdsc_t 结构表示 0x1000～0x4000 的虚拟地址空间
 * 		B_kmvarsdsc_t 结构表示 0x7000～0x9000 的虚拟地址空间
 * 	2. 然后分配 2KB 的虚拟地址空间，vma_find_kmvarsdsc 函数查找发现 A_kmvarsdsc_t 结构和 B_kmvarsdsc_t 结构之间正好有 0x4000～0x7000 的空间
 * 		刚好放得下 0x2000 大小的空间，于是这个函数就会返回 A_kmvarsdsc_t 结构，否则就会继续向后查找
 * 	3. 为了节约 kmvarsdsc_t 结构占用的内存空间，规定只要分配的虚拟地址空间上一个虚拟地址空间是连续且类型相同的就借用上一个 kmvarsdsc_t 结构
 * 		而不是重新分配一个 kmvarsdsc_t 结构表示新分配的虚拟地址空间. 目的：避免个应用每次分配一个页面的虚拟地址空间，不停地分配，导致物理内存耗尽
 * 
 * @param vmalocked virmemadrs 内存指针
 * @param start 需要分配的内存的开始地址
 * @param vassize 需要分配的内存的长度
 * @return kmvarsdsc_t* 
 */
kmvarsdsc_t* vma_find_kmvarsdsc(virmemadrs_t *vmalocked, adr_t start, size_t vassize, u64_t vaslimits, u32_t vastype)
{
	kmvarsdsc_t *kmvdcurrent = NULL, *curr = vmalocked->vs_currkmvdsc;
	adr_t newend = start + vassize;	// 分配的内存的结束地址
	list_h_t *listpos = NULL;

	// 分配的虚拟空间大小小于4KB不行
	if (0x1000 > vassize) {
		return NULL;
	}

	// 将要分配虚拟地址空间的结束地址大于整个虚拟地址空间
	if (newend > vmalocked->vs_isalcend) {
		return NULL;
	}

	if (NULL != curr) {
		// 先检查当前kmvarsdsc_t结构
		kmvdcurrent = vma_find_kmvarsdsc_is_ok(vmalocked, curr, start, vassize);
		if (NULL != kmvdcurrent) {
			if (vaslimits == kmvdcurrent->kva_limits && vastype == kmvdcurrent->kva_maptype) {
				return kmvdcurrent;
			}
		}
	}

	// 遍历virmemadrs_t中的所有的kmvarsdsc_t结构
	list_for_each(listpos, &vmalocked->vs_list) {
		curr = list_entry(listpos, kmvarsdsc_t, kva_list);
		if (vaslimits == curr->kva_limits && vastype == curr->kva_maptype) {
			// 检查每个kmvarsdsc_t结构
			kmvdcurrent = vma_find_kmvarsdsc_is_ok(vmalocked, curr, start, vassize);
			if (NULL != kmvdcurrent) {
				// 如果符合要求就返回
				return kmvdcurrent;
			}
		}
		
	}

	list_for_each(listpos, &vmalocked->vs_list) {
		curr = list_entry(listpos, kmvarsdsc_t, kva_list);
		// 检查每个kmvarsdsc_t结构
		kmvdcurrent = vma_find_kmvarsdsc_is_ok(vmalocked, curr, start, vassize);
		if (NULL != kmvdcurrent) {
			return kmvdcurrent;
		}
	}

	return NULL;
}

/**
 * @brief 虚拟内存分配
 * 
 * @param mm 虚拟内存管理结构体地址
 * @param start 需要分配的内存的开始地址
 * @param vassize 需要分配的内存地址
 * @param vaslimits 
 * @param vastype 
 * @return adr_t 
 */
adr_t vma_new_vadrs_core(mmadrsdsc_t *mm, adr_t start, size_t vassize, u64_t vaslimits, u32_t vastype)
{
	adr_t retadrs = NULL;
	kmvarsdsc_t *newkmvd = NULL, *currkmvd = NULL;
	virmemadrs_t *vma = &mm->msd_virmemadrs;
	cpuflg_t cpuflg;
	krlspinlock_cli(&vma->vs_lock, &cpuflg);

	// 查找虚拟地址区间
	currkmvd = vma_find_kmvarsdsc(vma, start, vassize, vaslimits, vastype);
	if (NULL == currkmvd) {
		retadrs = NULL;
		goto out;
	}

	// 进行虚拟地址区间进行检查查看是否复用这个数据结构
	if (((NULL == start) || (start == currkmvd->kva_end)) && (vaslimits == currkmvd->kva_limits) && (vastype == currkmvd->kva_maptype)) {
		// 能复用的，当前虚拟地址区间的结束地址返回
		retadrs = currkmvd->kva_end;
		// 扩展当前虚拟地址区间的结束地址为分配虚拟地址区间大小
		currkmvd->kva_end += vassize;
		vma->vs_currkmvdsc = currkmvd;
		goto out;
	}

	// 建立一个新的kmvarsdsc_t虚拟地址区间结构
	newkmvd = new_kmvarsdsc();
	if (NULL == newkmvd) {
		retadrs = NULL;
		goto out;
	}

	// 如果分配的开始地址为NULL就由系统动态决定
	if (NULL == start) {
		// 当然是接着当前虚拟地址区间之后开始
		newkmvd->kva_start = currkmvd->kva_end;
	} else {
		// 否则这个新的虚拟地址区间的开始就是请求分配的开始地址
		newkmvd->kva_start = start;
	}

	// 设置新的虚拟地址区间的结束地址
	newkmvd->kva_end = newkmvd->kva_start + vassize;
	newkmvd->kva_limits = vaslimits;
	newkmvd->kva_maptype = vastype;
	newkmvd->kva_mcstruct = vma;
	vma->vs_currkmvdsc = newkmvd;

	// 将新的虚拟地址区间加入到virmemadrs_t结构中
	list_add(&newkmvd->kva_list, &currkmvd->kva_list);
	// 看看新的虚拟地址区间是否是最后一个
	if (list_is_last(&newkmvd->kva_list, &vma->vs_list) == TRUE) {
		vma->vs_endkmvdsc = newkmvd;
	}
	// 返回新的虚拟地址区间的开始地址
	retadrs = newkmvd->kva_start;
out:
	krlspinunlock_sti(&vma->vs_lock, &cpuflg);
	return retadrs;
}

/**
 * @brief 分配虚拟地址空间的接口
 * 
 * @param mm 虚拟内存管理结构体地址
 * @param start 需要分配的内存的开始地址
 * @param vassize 需要分配的内存地址
 * @param vaslimits 
 * @param vastype 
 * @return adr_t 
 */
adr_t vma_new_vadrs(mmadrsdsc_t *mm, adr_t start, size_t vassize, u64_t vaslimits, u32_t vastype)
{
	if (NULL == mm || 1 > vassize) {
		return NULL;
	}

	// 进行参数检查，开始地址要和页面(1KB)对齐，结束地址不能超过整个虚拟地址空间
	if (NULL != start) {
		if (((start & 0xfff) != 0) || (0x1000 > start) || (USER_VIRTUAL_ADDRESS_END < (start + vassize))) {
			return NULL;
		}
	}
	// 调用虚拟地址空间分配的核心函数
	return vma_new_vadrs_core(mm, start, VADSZ_ALIGN(vassize), vaslimits, vastype);
}
/**
 * @brief 查找要释放虚拟地址空间的kmvarsdsc_t结构
 * 	1. 释放时，查找虚拟地址区间的函数非常简单，仅仅是检查释放的虚拟地址空间是否落在查找 kmvarsdsc_t 结构表示的虚拟地址区间中
 * 
 * @param vmalocked virmemadrs内存指针
 * @param start 要释放的内存
 * @param vassize 要释放的内存长度
 * @return kmvarsdsc_t* 
 */
kmvarsdsc_t *vma_del_find_kmvarsdsc(virmemadrs_t *vmalocked, adr_t start, size_t vassize)
{
	kmvarsdsc_t *curr = vmalocked->vs_currkmvdsc;
	adr_t newend = start + (adr_t)vassize;
	list_h_t *listpos = NULL;

	if (0x1000 > vassize) {
		return NULL;
	}

	if (NULL != curr) {
		// 释放的虚拟地址空间落在了当前kmvarsdsc_t结构表示的虚拟地址区间
		if ((curr->kva_start) <= start && (newend <= curr->kva_end)) {
			return curr;
		}
	}

	// 遍历所有的kmvarsdsc_t结构
	list_for_each(listpos, &vmalocked->vs_list) {
		curr = list_entry(listpos, kmvarsdsc_t, kva_list);
		// 释放的虚拟地址空间是否落在了其中的某个kmvarsdsc_t结构表示的虚拟地址空间
		if ((start >= curr->kva_start) && (newend <= curr->kva_end)) {
			return curr;
		}
	}

	return NULL;
}

/**
 * @brief 删除 kmvarsdsc_t
 * 
 * @param vmalocked virmemadrs内存指针
 * @param del 需要被删除的kmvarsdsc指针
 */
void vma_del_set_endcurrkmvd(virmemadrs_t *vmalocked, kmvarsdsc_t *del)
{
	kmvarsdsc_t *prevkmvd = NULL, *nextkmvd = NULL;

	// del 为 vmalocked 链表的最后一个
	if (list_is_last(&del->kva_list, &vmalocked->vs_list) == TRUE) {
		// del 不是 vmalocked 链表的第一个。从链表中删除
		if (list_is_first(&del->kva_list, &vmalocked->vs_list) == FALSE) {
			prevkmvd = list_prev_entry(del, kmvarsdsc_t, kva_list);
			vmalocked->vs_endkmvdsc = prevkmvd;
			vmalocked->vs_currkmvdsc = prevkmvd;
		} else {
			vmalocked->vs_endkmvdsc = NULL;
			vmalocked->vs_currkmvdsc = NULL;
		}
	} else {
		// 切换下一个要删除的kmvarsdsc_t
		nextkmvd = list_next_entry(del, kmvarsdsc_t, kva_list);
		vmalocked->vs_currkmvdsc = nextkmvd;
	}

	return;
}

/**
 * @brief 删除映射的物理内存
 * 
 * @param mm mmadrsdsc内存指针
 * @param kmvd kmvarsdsc内存指针
 * @param start 要删除的内存开始地址
 * @param end 要删除的内存结束地址
 * @return bool_t 
 */
bool_t vma_del_unmapping_phyadrs(mmadrsdsc_t *mm, kmvarsdsc_t *kmvd, adr_t start, adr_t end)
{
	adr_t phyadrs;	// 物理地址
	bool_t rets = TRUE;
	mmudsc_t *mmu = &mm->msd_mmu;
	kvmemcbox_t *kmbox = kmvd->kva_kvmbox;

	// 遍历所有虚拟内存。4KB内存移动
	for (adr_t vadrs = start; vadrs < end; vadrs += VMAP_MIN_SIZE) {
		// 清除对应地址的顶级页目录项、页目录指针项、页目录项
		phyadrs = hal_mmu_untransform(mmu, vadrs);
		if (NULL != phyadrs && NULL != kmbox) {
			// 清除物理地址
			if (vma_del_usermsa(mm, kmbox, NULL, phyadrs) == FALSE) {
				rets = FALSE;
			}
		}
	}

	return rets;
}

/**
 * @brief 删除映射的物理内存
 * 
 * @param mm mmadrsdsc内存指针
 * @param kmvd kmvarsdsc内存指针
 * @param start 要删除的内存开始地址
 * @param vassize 要删除的内存的长度
 * @return bool_t 
 */
bool_t vma_del_unmapping(mmadrsdsc_t *mm, kmvarsdsc_t *kmvd, adr_t start, size_t vassize)
{
	adr_t end;

	if (NULL == mm || NULL == kmvd) {
		return FALSE;
	}

	end = start + (adr_t)vassize;

	return vma_del_unmapping_phyadrs(mm, kmvd, start, end);
}

/**
 * @brief 释放虚拟地址空间的核心函数
 * 	1. 首位都相等，砍掉kmvarsdsc_t结构
 * 	2. 开始位相等，砍掉kmvarsdsc_t开始位
 * 	3. 结尾位相等，砍掉kmvarsdsc_t结尾位
 * 	4. 首尾都不相等，砍掉中间部分，两边拆分为两个kmvarsdsc_t结构
 * 
 * @param mm mmadrsdsc 地址指针
 * @param start 要释放的地址
 * @param vassize 要释放的地址长度
 * @return bool_t 
 */
bool_t vma_del_vadrs_core(mmadrsdsc_t *mm, adr_t start, size_t vassize)
{
	bool_t rets = FALSE;
	kmvarsdsc_t *newkmvd = NULL, *delkmvd = NULL;
	virmemadrs_t *vma = &mm->msd_virmemadrs;
	cpuflg_t cpuflg;
	krlspinlock_cli(&vma->vs_lock, &cpuflg);

	// 查找要释放虚拟地址空间的kmvarsdsc_t结构
	delkmvd = vma_del_find_kmvarsdsc(vma, start, vassize);
	if (NULL == delkmvd) {
		rets = FALSE;
		goto out;
	}

	// 第一种情况要释放的虚拟地址空间正好等于查找的kmvarsdsc_t结构
	if ((delkmvd->kva_start == start) && (delkmvd->kva_end == (start + (adr_t)vassize))) {
		// 删除页表
		vma_del_unmapping(mm, delkmvd, start, vassize);
		// 删除 kmvarsdsc_t
		vma_del_set_endcurrkmvd(vma, delkmvd);
		// 删除 kvmemcbox_t
		knl_put_kvmemcbox(delkmvd->kva_kvmbox);

		// 脱链
		list_del(&delkmvd->kva_list);
		// 删除kmvarsdsc_t结构
		del_kmvarsdsc(delkmvd);
		vma->vs_kmvdscnr--;
		rets = TRUE;
		goto out;
	}

	// 第二种情况要释放的虚拟地址空间是在查找的kmvarsdsc_t结构的上半部分
	if ((delkmvd->kva_start == start) && (delkmvd->kva_end > (start + (adr_t)vassize))) {
		delkmvd->kva_start = start + (adr_t)vassize;
		vma_del_unmapping(mm, delkmvd, start, vassize);
		rets = TRUE;
		goto out;
	}

	// 第三种情况要释放的虚拟地址空间是在查找的kmvarsdsc_t结构的下半部分
	if ((delkmvd->kva_start < start) && (delkmvd->kva_end == (start + (adr_t)vassize))) {
		// 所以直接把查找的kmvarsdsc_t结构的结束地址设置为释放虚拟地址空间的开始地址
		delkmvd->kva_end = start;
		vma_del_unmapping(mm, delkmvd, start, vassize);
		rets = TRUE;
		goto out;
	}

	// 第四种情况要释放的虚拟地址空间是在查找的kmvarsdsc_t结构的中间
	if ((delkmvd->kva_start < start) && (delkmvd->kva_end > (start + (adr_t)vassize))) {
		// 所以要再新建一个kmvarsdsc_t结构来处理释放虚拟地址空间的下半虚拟部分地址空间
		newkmvd = new_kmvarsdsc();
		if (NULL == newkmvd) {
			rets = FALSE;
			goto out;
		}

		// 让新的kmvarsdsc_t结构指向查找的kmvarsdsc_t结构的后半部分虚拟地址空间
		newkmvd->kva_end = delkmvd->kva_end;
		newkmvd->kva_start = start + (adr_t)vassize;
		// 和查找到的kmvarsdsc_t结构保持一致
		newkmvd->kva_limits = delkmvd->kva_limits;
		newkmvd->kva_maptype = delkmvd->kva_maptype;
		newkmvd->kva_mcstruct = vma;
		delkmvd->kva_end = start;

		knl_count_kvmemcbox(delkmvd->kva_kvmbox);
		newkmvd->kva_kvmbox = delkmvd->kva_kvmbox;

		vma_del_unmapping(mm, delkmvd, start, vassize);
		
		// 加入链表
		list_add(&newkmvd->kva_list, &delkmvd->kva_list);
		vma->vs_kmvdscnr++;

        // 是否为最后一个kmvarsdsc_t结构
		if (list_is_last(&newkmvd->kva_list, &vma->vs_list) == TRUE) {
			vma->vs_endkmvdsc = newkmvd;
			vma->vs_currkmvdsc = newkmvd;
		} else {
			vma->vs_currkmvdsc = newkmvd;
		}

		rets = TRUE;
		goto out;
	}

	rets = FALSE;

out:
	krlspinunlock_sti(&vma->vs_lock, &cpuflg);
	return rets;
}

// 释放虚拟地址空间的接口
bool_t vma_del_vadrs(mmadrsdsc_t *mm, adr_t start, size_t vassize)
{
	// 对参数进行检查
	if (NULL == mm || 1 > vassize || NULL == start) {
		return FALSE;
	}
	// 调用核心处理函数
	return vma_del_vadrs_core(mm, start, VADSZ_ALIGN(vassize));
}


int imrand()
{
	krlvirmemadrs.kvs_randnext = krlvirmemadrs.kvs_randnext * 1103515245 + 12345;
	return ((unsigned)(krlvirmemadrs.kvs_randnext / 65536) % 32768);
}


int kvma_rand(int s, int e)
{
	return s + imrand() % e;
}


teststc_t *find_tstc_on_rnr(uint_t randnr)
{
	list_h_t *tmplst;

	teststc_t *tc = NULL;
	//bool_t rets=FALSE;
	//adr_t tmpendadr1=NULL,tmpendadr2=NULL;
	uint_t i = 0;
	if (randnr > krlvirmemadrs.kvs_tstcnr) {
		return NULL;
	}

	list_for_each(tmplst, &krlvirmemadrs.kvs_testhead) {
		if (i == randnr) {
			tc = list_entry(tmplst, teststc_t, tst_list);
			return tc;
		}

		i++;
	}

	return NULL;
}

void rand_write_tstc()
{
	teststc_t *t;
	int j = (int)(krlvirmemadrs.kvs_tstcnr / 2);
	//adr_t tmpadr;
	uint_t rd = 0;
	for (int i = 0; i < j; ++i) {
		rd = (uint_t)kvma_rand(0, (int)krlvirmemadrs.kvs_tstcnr);
		t = find_tstc_on_rnr(rd);
		if (NULL == t) {
			system_error("rand_del_tstc t null\n");
		}

		memset((void *)t->tst_vadr, 0xaa, t->tst_vsiz);
		kprint("成功随机写入第%x个虚拟地址空间:%x\n", rd, t->tst_vadr);
		//krlvirmemadrs.kvs_tstcnr--;
	}

	return;
}


void check_one_teststc(teststc_t *chktc)
{
	list_h_t *tmplst;

	teststc_t *tc = NULL;
	//bool_t rets=FALSE;
	adr_t tmpendadr1 = NULL, tmpendadr2 = NULL;
	list_for_each(tmplst, &krlvirmemadrs.kvs_testhead) {
		tc = list_entry(tmplst, teststc_t, tst_list);
		if (tc != chktc) {
			tmpendadr1 = chktc->tst_vadr + chktc->tst_vsiz;
			tmpendadr2 = tc->tst_vadr + tc->tst_vsiz;

			if ((chktc->tst_vadr > tc->tst_vadr && tmpendadr1 < tmpendadr2) ||
				(tc->tst_vadr > chktc->tst_vadr && tmpendadr2 < tmpendadr1) ||
				(chktc->tst_vadr > tc->tst_vadr && chktc->tst_vadr < tmpendadr2) ||
				(tc->tst_vadr > chktc->tst_vadr && tc->tst_vadr < tmpendadr1)) {
				system_error("check_one_teststc err\n");
			}
		}
	}

	return;
}

void check_teststc()
{
	list_h_t *tmplst;

	teststc_t *tc = NULL;
	//bool_t rets=FALSE;
	uint_t i = 0;
	list_for_each(tmplst, &krlvirmemadrs.kvs_testhead) {
		tc = list_entry(tmplst, teststc_t, tst_list);
		check_one_teststc(tc);
		i++;
		kprint("TC检查完第%x个虚拟地址空间:%x %x %x\n", i, tc->tst_vadr, tc->tst_vsiz, krlvirmemadrs.kvs_kmvdscnr);
	}

	return;
}


void disp_kvirmemadrs()
{
	kprint("krlvirmemadrs.kvs_kmvdscnr:%x\n", krlvirmemadrs.kvs_kmvdscnr);
	kprint("krlvirmemadrs.kom_kvmcobjnr:%x :%x\n", krlvirmemadrs.kvs_kvmcomgr.kom_kvmcobjnr, krlvirmemadrs.kvs_kvmcomgr.kom_kvmcodelnr);
	kprint("krlvirmemadrs.kvs_tstcnr:%x\n", krlvirmemadrs.kvs_tstcnr);
	kprint("krlvirmemadrs.ptp_msanr:%x\n", krlvirmemadrs.kvs_ptabpgcs.ptp_msanr);
	die(0x200);
	return;
}


void vma_del_all_kvma(mmadrsdsc_t *mm)
{
	list_h_t *tmplst;

	teststc_t *tc = NULL;
	bool_t rets = FALSE;
	list_for_each_head_dell(tmplst, &krlvirmemadrs.kvs_testhead) {
		tc = list_entry(tmplst, teststc_t, tst_list);
		rets = vma_del_vadrs(mm, tc->tst_vadr, tc->tst_vsiz);
		if (FALSE == rets) {
			system_error("vma_del_all_kvma err\n");
		}

		list_del(&tc->tst_list);
		del_teststc(tc);
		krlvirmemadrs.kvs_tstcnr--;
		kprint("成功删除第%x个虚拟地址空间:%x\n", krlvirmemadrs.kvs_tstcnr, tc);
	}

	return;
}

void vma_chk_one_kmva(virmemadrs_t *vma, kmvarsdsc_t *chkkmva)
{
	kmvarsdsc_t *head;
	if (NULL == chkkmva || NULL == vma) {
		system_error("vma_chk_one_kmva chkkmva NULL\n");
	}

	list_h_t *pos;
	list_for_each(pos, &vma->vs_list) {
		head = list_entry(pos, kmvarsdsc_t, kva_list);

		if (head != chkkmva) {
			if ((chkkmva->kva_start > head->kva_start && chkkmva->kva_end < head->kva_end) ||
				(chkkmva->kva_start < head->kva_start && chkkmva->kva_end > head->kva_end) ||
				(chkkmva->kva_start > head->kva_start && chkkmva->kva_start < head->kva_end) ||
				(chkkmva->kva_start < head->kva_start && chkkmva->kva_end > head->kva_start)) {
				kprint("chkkmva->kva_start:%x head->kva_start:%x chkkmva->kva_end:%x head->kva_end:%x\n",
						chkkmva->kva_start, head->kva_start, chkkmva->kva_end, head->kva_end);
				system_error("vma_chk_one_kmva err\n");
			}
		}
	}

	return;
}

void vma_check_kmva(mmadrsdsc_t *mm)
{
	virmemadrs_t *vma = &mm->msd_virmemadrs;
	kmvarsdsc_t *head;
	uint_t i = 0;

	list_h_t *pos;
	list_for_each(pos, &vma->vs_list) {
		head = list_entry(pos, kmvarsdsc_t, kva_list);
		vma_chk_one_kmva(vma, head);
		kprint("检查完第%x个虚拟地址空间:%x %x %x\n", i, head->kva_start, head->kva_end, krlvirmemadrs.kvs_kmvdscnr);
		i++;
	}

	return;
}

void vma_rand_del_tstc(mmadrsdsc_t *mm)
{
	teststc_t *t;
	int j = (int)(krlvirmemadrs.kvs_tstcnr / 2);
	adr_t tmpadr;
	uint_t rd = 0;
	for (int i = 0; i < j; ++i) {
		rd = (uint_t)kvma_rand(0, (int)krlvirmemadrs.kvs_tstcnr);
		t = find_tstc_on_rnr(rd);
		if (NULL == t) {
			system_error("vma_rand_del_tstc t null\n");
		}

		if (FALSE == vma_del_vadrs(mm, t->tst_vadr, t->tst_vsiz)) {
			system_error("vma_rand_del_tstc terr\n");
		}

		tmpadr = t->tst_vadr;
		list_del(&t->tst_list);
		del_teststc(t);
		kprint("成功随机删除第%x个虚拟地址空间:%x\n", rd, tmpadr);
		krlvirmemadrs.kvs_tstcnr--;
	}

	return;
}

void vma_rand_write_tstc()
{
	teststc_t *t;
	int j = (int)(krlvirmemadrs.kvs_tstcnr / 2);
	//adr_t tmpadr;
	uint_t rd = 0;

	for (int i = 0; i < j; ++i) {
		rd = (uint_t)kvma_rand(0, (int)krlvirmemadrs.kvs_tstcnr);
		t = find_tstc_on_rnr(rd);
		if (NULL == t) {
			system_error("vma_rand_del_tstc t null\n");
		}

		hal_memset((void *)t->tst_vadr, 0xaa, t->tst_vsiz);
		kprint("成功随机写入第%x个虚拟地址空间:%x\n", rd, t->tst_vadr);
		//krlvirmemadrs.kvs_tstcnr--;
	}

	return;
}

void __test_vma(mmadrsdsc_t *mm)
{
	size_t vsz = 0x4000UL << 12;
	adr_t retadr;
	uint_t max = 0x2000;

	for (uint_t i = 0; i < max; i++) {
		vsz = (size_t)kvma_rand(0x1, 0x4UL << 12);
		retadr = vma_new_vadrs(mm, NULL, vsz, i, 0);

		if (NULL != retadr) {
			add_new_teststc(retadr, VADSZ_ALIGN(vsz));
			kprint("分配的虚拟:%x:大小:%x:个数:%x\n", retadr, vsz, i);
		} else {
			break;
		}
	}

	return;
}

// void test_vma()
// {
// 	mmadrsdsc_t *mm = knl_current_mmadrsdsc();
// 	__test_vma(mm);
// 	vma_rand_write_tstc(mm);
// 	vma_check_kmva(mm);
// 	vma_rand_del_tstc(mm);
// 	vma_rand_write_tstc(mm);
// 	vma_check_kmva(mm);
// 	vma_del_all_kvma(mm);
// 	dump_kvmemcboxmgr(&krlvirmemadrs.kvs_kvmemcboxmgr);
// 	dump_mmu(&mm->msd_mmu);
// 	return;
// }


cr3s_t knl_retn_currentcpu_cr3s()
{
	cr3s_t cr3;
	cr3.c3s_entry = (u64_t)read_cr3();
	return cr3;
}

void kmap_fulshcurrcpu_mmutable()
{
	//adr_t pml4badr=0UL;
	cr3s_t cr3;
	cr3.c3s_entry = (u64_t)read_cr3();
	write_cr3(cr3.c3s_entry);
	return;
}

/**
 * @brief 查找是否被分配的虚拟地址
 * 
 * @param vmalocked virmemadrs内存指针
 * @param vadrs 缺页异常地址
 * @return kmvarsdsc_t* 
 */
kmvarsdsc_t* vma_map_find_kmvarsdsc(virmemadrs_t *vmalocked, adr_t vadrs)
{
	list_h_t *pos = NULL;
	kmvarsdsc_t *curr = vmalocked->vs_currkmvdsc;
	
	// 看看上一次刚刚被操作的kmvarsdsc_t结构
	if (NULL != curr) {
		// 虚拟地址是否落在kmvarsdsc_t结构表示的虚拟地址区间
		if ((vadrs >= curr->kva_start) && (vadrs < curr->kva_end)) {
			return curr;
		}
	}

	// 遍历每个kmvarsdsc_t结构
	list_for_each(pos, &vmalocked->vs_list) {
		curr = list_entry(pos, kmvarsdsc_t, kva_list);
		// 虚拟地址是否落在kmvarsdsc_t结构表示的虚拟地址区间
		if ((vadrs >= curr->kva_start) && (vadrs < curr->kva_end)) {
			return curr;
		}
	}

	return NULL;
}

/**
 * @brief 通过kmvarsdsc结构查找页面盒子(kvmemcbox)
 * 
 * @param kmvd 
 * @return kvmemcbox_t* 
 */
kvmemcbox_t *vma_map_retn_kvmemcbox(kmvarsdsc_t *kmvd)
{
	kvmemcbox_t *kmbox = NULL;
	// 如果kmvarsdsc_t结构中已经存在了kvmemcbox_t结构，则直接返回
	if (NULL == kmvd) {
		return NULL;
	}

	// 新建一个kvmemcbox_t结构
	if (NULL != kmvd->kva_kvmbox) {
		return kmvd->kva_kvmbox;
	}

	kmbox = knl_get_kvmemcbox();
	if (NULL == kmbox) {
		return NULL;
	}

	// 指向这个新建的kvmemcbox_t结构
	kmvd->kva_kvmbox = kmbox;
	return kmvd->kva_kvmbox;
}

/**
 * @brief 
 * 
 * @param mm 
 * @param kmbox 
 * @param msa 
 * @param phyadr 
 * @return bool_t 
 */
bool_t vma_del_usermsa(mmadrsdsc_t *mm, kvmemcbox_t *kmbox, msadsc_t *msa, adr_t phyadr)
{
	bool_t rets = FALSE;
	msadsc_t *tmpmsa = NULL, *delmsa = NULL;
	list_h_t *pos;

	if (NULL == mm || NULL == kmbox || NULL == phyadr) {
		return FALSE;
	}

	krlspinlock_lock(&kmbox->kmb_lock);

	if (NULL != msa) {
		if (msadsc_ret_addr(msa) == phyadr) {
			delmsa = msa;
			list_del(&msa->md_list);
			kmbox->kmb_msanr--;
			rets = TRUE;
			goto out;
		}
	}

	list_for_each(pos, &kmbox->kmb_msalist) {
		tmpmsa = list_entry(pos, msadsc_t, md_list);
		if (msadsc_ret_addr(tmpmsa) == phyadr) {
			delmsa = tmpmsa;
			list_del(&tmpmsa->md_list);
			kmbox->kmb_msanr--;
			rets = TRUE;
			goto out;
		}
	}

	delmsa = NULL;
	rets = FALSE;

out:
	krlspinlock_unlock(&kmbox->kmb_lock);

	if (NULL != delmsa) {
		if (mm_merge_pages(&memmgrob, delmsa, onfrmsa_retn_fpagenr(delmsa)) == FALSE) {
			system_error("vma_del_usermsa err\n");
			return FALSE;
		}
	}

	return rets;
}

/**
 * @brief 新增对应虚拟地址的msadsc
 * 
 * @param mm 
 * @param kmbox 
 * @return msadsc_t* 
 */
msadsc_t *vma_new_usermsa(mmadrsdsc_t *mm, kvmemcbox_t *kmbox)
{
	u64_t pages = 1, retpnr = 0;
	msadsc_t *msa = NULL;

	if (NULL == mm || NULL == kmbox) {
		return NULL;
	}

	msa = mm_divpages_procmarea(&memmgrob, pages, &retpnr);
	if (NULL == msa) {
		return NULL;
	}

	krlspinlock_lock(&kmbox->kmb_lock);

	list_add(&msa->md_list, &kmbox->kmb_msalist);
	kmbox->kmb_msanr++;

	krlspinlock_unlock(&kmbox->kmb_lock);
	return msa;
}

/**
 * @brief 映射物理内存页面
 * 	1. vma_new_usermsa 函数，分配一个物理内存页面并把对应的 msadsc_t 结构挂载到 kvmemcbox_t 结构上
 * 	2. 接着获取 msadsc_t 结构对应内存页面的物理地址
 * 	3. 最后是调用 hal_mmu_transform 函数完成虚拟地址到物理地址的映射工作，它主要是建立 MMU 页表
 * 
 * @param mm mmadrsdsc 内存指针
 * @param kmbox kvmemcbox页面盒子指针
 * @param vadrs 缺页异常地址
 * @param flags 
 * @return adr_t 
 */
adr_t vma_map_msa_fault(mmadrsdsc_t *mm, kvmemcbox_t *kmbox, adr_t vadrs, u64_t flags)
{
	msadsc_t *usermsa;
	adr_t phyadrs = NULL;
	if (NULL == mm || NULL == kmbox || NULL == vadrs) {
		return NULL;
	}
	// 分配一个物理内存页面，挂载到kvmemcbox_t中，并返回对应的msadsc_t结构
	usermsa = vma_new_usermsa(mm, kmbox);
	if (NULL == usermsa) {
		// 没有物理内存页面返回NULL表示失败
		return NULL;
	}

	// 获取msadsc_t对应的内存页面的物理地址
	phyadrs = msadsc_ret_addr(usermsa);

	// 建立MMU页表完成虚拟地址到物理地址的映射
	if (hal_mmu_transform(&mm->msd_mmu, vadrs, phyadrs, flags) == TRUE) {
		// 映射成功则返回物理地址
		return phyadrs;
	}

	// 映射失败就要先释放分配的物理内存页面
	vma_del_usermsa(mm, kmbox, usermsa, phyadrs);
	return NULL;
}

// 接口函数
adr_t vma_map_phyadrs(mmadrsdsc_t *mm, kmvarsdsc_t *kmvd, adr_t vadrs, u64_t flags)
{
	kvmemcbox_t *kmbox = kmvd->kva_kvmbox;
	if (NULL == kmbox) {
		return NULL;
	}
	// 调用核心函数，flags表示页表条目中的相关权限、存在、类型等位段
	return vma_map_msa_fault(mm, kmbox, vadrs, flags);
}

// 这样的骚操作只是暂时为之
void vma_full_textbin(mmadrsdsc_t* mm, kmvarsdsc_t* kmvd, adr_t vadr)
{
	thread_t* td = mm->msd_thread;
	adr_t vstart = kmvd->kva_start;
	adr_t vnr = (vadr - vstart) >> PAGE_SZRBIT;
	u64_t filevadr = 0, filesize = 0;
	if (KMV_BIN_TYPE != kmvd->kva_maptype) {
		return; 
	}

	if (NULL == td) {
		system_error("vma_full_textbin td is null\n");
		return;
		
	}
	get_file_rvadrandsz(td->td_appfilenm, &kmachbsp, &filevadr, &filesize);
	if (0 == filevadr || 0 == filesize) {
		system_error("vma_full_textbin file is null\n");
		return;
	}
	
	krlmemcopy((void*)(filevadr + ((u64_t)(vnr << PAGE_SZRBIT))), (void*)(vadr & (~0xfffUL)), (1 << PAGE_SZRBIT));
	// kprint("vma_full_textbin fvadr:%x fsize:%x vadr:%x vstart:%x\n", filevadr, filesize, vadr, vstart);
	// dump_mem_range((vadr & (~0xfffUL)), 10, 1);
	return;	
}

/**
 * @brief 缺页异常处理核心函数
 * 	1. 查找缺页地址对应的 kmvarsdsc_t 结构
 * 	2. 查找 kmvarsdsc_t 结构下面的对应 kvmemcbox_t 结构，它是用来挂载物理内存页面的
 * 	3. 分配物理内存页面并建立 MMU 页表映射关系
 * 
 * @param mm mmadrsdsc内存指针
 * @param vadrs 缺页异常地址
 * @return sint_t 返回状态码
 */
sint_t vma_map_fairvadrs_core(mmadrsdsc_t *mm, adr_t vadrs)
{
	sint_t rets = FALSE;
	adr_t phyadrs = NULL;
	virmemadrs_t *vma = &mm->msd_virmemadrs;
	kmvarsdsc_t *kmvd = NULL;
	kvmemcbox_t *kmbox = NULL;
	cpuflg_t cpuflg;
	krlspinlock_cli(&vma->vs_lock, &cpuflg);

	// 查找对应的kmvarsdsc_t结构。 没找到说明没有分配该虚拟地址空间，那属于非法访问不予处理
	kmvd = vma_map_find_kmvarsdsc(vma, vadrs);
	if (NULL == kmvd) {
		rets = -EFAULT;
		goto out;
	}

	// 返回kmvarsdsc_t结构下对应kvmemcbox_t结构，它是用来挂载物理内存页面的
	kmbox = vma_map_retn_kvmemcbox(kmvd);
	if (NULL == kmbox) {
		rets = -ENOOBJ;
		goto out;
	}

	// 分配物理内存页面并建立MMU页表
	phyadrs = vma_map_phyadrs(mm, kmvd, vadrs, (0 | PML4E_US | PML4E_RW | PML4E_P));
	if (NULL == phyadrs) {
		kprint("vma_map_phyadrs null %x\n", vadrs);
		rets = -ENOMEM;
		goto out;
	}

	vma_full_textbin(mm, kmvd, vadrs); // 骚操作
	rets = EOK;

out:
	krlspinunlock_sti(&vma->vs_lock, &cpuflg);
	return rets;
}

// 缺页异常处理接口
sint_t vma_map_fairvadrs(mmadrsdsc_t *mm, adr_t vadrs)
{
	// 对参数进行检查
	if ((0x1000 > vadrs) || (USER_VIRTUAL_ADDRESS_END < vadrs) || (NULL == mm)) {
		return -EPARAM;
	}

	// 进行缺页异常的核心处理
	return vma_map_fairvadrs_core(mm, vadrs);
}

mmadrsdsc_t* krl_curr_mmadrsdsc()
{
	thread_t* td;
	td = krlsched_retn_currthread();
	return td->td_mmdsc;
}

/**
 * @brief 由异常分发器调用的接口
 * 
 * @param fairvadrs 异常地址
 * @return sint_t 
 */
sint_t krluserspace_accessfailed(adr_t fairvadrs)
{
	// 这里应该获取当前进程的mm，但是现在我们没有进程，才initmmadrsdsc代替
	mmadrsdsc_t* mm = krl_curr_mmadrsdsc();
	// 应用程序的虚拟地址不可能大于USER_VIRTUAL_ADDRESS_END
	if (USER_VIRTUAL_ADDRESS_END < fairvadrs) {
		return -EACCES;
	}
	return vma_map_fairvadrs(mm, fairvadrs);
}

void test_krl_pages_fault()
{
	/*uint_t vsz=0x4000UL<<12;
	adr_t viradr; 
	for(int i=0;i<(10000);i++)
	{
		vsz=kvma_rand(0x1,0x4000UL<<12);
		viradr=kvma_vadrspace_new(vsz,0,0);
		kprint("分配的虚拟:%x:大小:%x:%x\n",viradr,vsz,i);
		//memset((void *)(viradr),0xaa,vsz);
		kprint("开始删除虚拟地址空:%x大小:%x\n",viradr,vsz);
		if(FALSE==kvma_vadrspace_del(viradr,vsz))
		{
			system_error("test_krl_pages_fault err\n");
		}
	}*/
	//test_kmva();
	return;
}

void kvmcobjmgr_t_init(kvmcobjmgr_t* initp)
{
	if (NULL == initp) {
		system_error("kvmcobjmgr_t_init parm NULL\n");
	}

	krlspinlock_init(&initp->kom_lock);
	initp->kom_flgs = 0;
	initp->kom_kvmcobjnr = 0;

	list_init(&initp->kom_kvmcohead);
	initp->kom_kvmcocahenr = 0;

	list_init(&initp->kom_kvmcocahe);
	initp->kom_kvmcodelnr = 0;

	list_init(&initp->kom_kvmcodelhead);
	return;
}

void kvmemcobj_t_init(kvmemcobj_t* initp)
{
	if (NULL == initp) {
		system_error("kvmemcobj_t_init parm NULL\n");
	}

	list_init(&initp->kco_list);
	krlspinlock_init(&initp->kco_lock);
	initp->kco_cont = 0;
	initp->kco_flgs = 0;
	initp->kco_type = 0;
	initp->kco_msadnr = 0;

	list_init(&initp->kco_msadlst);
	initp->kco_filenode = NULL;
	initp->kco_pager = NULL;
	initp->kco_extp = NULL;

	return;
}

kvmemcobj_t* new_kvmemcobj()
{
	kvmemcobj_t* kvcop = (kvmemcobj_t*)kmsob_new(sizeof(kvmemcobj_t));

	if (NULL == kvcop) {
		return NULL;
	}

	kvmemcobj_t_init(kvcop);
	return kvcop;
}

bool_t del_kvmemcobj(kvmemcobj_t* delkvmcop)
{
	if (NULL == delkvmcop) {
		return FALSE;
	}

	if (kmsob_delete((void*)delkvmcop,sizeof(kvmcobjmgr_t)) == FALSE) {
		return FALSE;
	}

	return TRUE;
}

/**
 * @brief 页面盒子初始化
 * 
 * @param init 
 */
void kvmemcbox_t_init(kvmemcbox_t* init)
{
	if(NULL == init) {
		return;
	}

	list_init(&init->kmb_list);
	krlspinlock_init(&init->kmb_lock);

	refcount_init(&init->kmb_cont);
	init->kmb_flgs = 0;
	init->kmb_stus = 0;
	init->kmb_type = 0;
	init->kmb_msanr = 0;

	list_init(&init->kmb_msalist);
	init->kmb_mgr = NULL;
	init->kmb_filenode = NULL;
	init->kmb_pager = NULL;
	init->kmb_ext = NULL;

	return;
}

void kvmemcboxmgr_t_init(kvmemcboxmgr_t* init)
{
	if (NULL == init) {
		return;
	}

	list_init(&init->kbm_list);
	krlspinlock_init(&init->kbm_lock);
	init->kbm_flgs = 0;
	init->kbm_stus = 0;
	init->kbm_kmbnr = 0;

	list_init(&init->kbm_kmbhead);
	init->kbm_cachenr = 0;
	init->kbm_cachemax = KMBOX_CACHE_MAX;
	init->kbm_cachemin = KMBOX_CACHE_MIN;

	list_init(&init->kbm_cachehead);
	init->kbm_ext = NULL;
	return;
}

kvmemcbox_t* new_kvmemcbox()
{
	kvmemcbox_t* kmbox = NULL;
	
	kmbox = (kvmemcbox_t*)kmsob_new(sizeof(kvmemcbox_t));
	if(NULL == kmbox) {
		return NULL;
	}

	kvmemcbox_t_init(kmbox);
	return kmbox;
}

/**
 * @brief kvmemcbox 删除
 * 	1. 把 del指针对应的内存删除
 * 
 * @param del memcbox指针
 * @return bool_t 
 */
bool_t del_kvmemcbox(kvmemcbox_t* del)
{
	if(NULL == del) {
		return FALSE;
	}

	return kmsob_delete((void*)del, sizeof(kvmemcbox_t));
}

// 增加 kvmemcbox 数量
void knl_count_kvmemcbox(kvmemcbox_t* kmbox)
{
	if(NULL == kmbox) {
		return;
	}

	// 增加 kvmemcbox 数量
	refcount_inc(&kmbox->kmb_cont);
	return;
}

void knl_decount_kvmemcbox(kvmemcbox_t* kmbox)
{
	if(NULL == kmbox) {
		return;
	}

	refcount_dec(&kmbox->kmb_cont);
	return;
}

/**
 * @brief 从页面盒子的头查找页面盒子
 * 
 * @return kvmemcbox_t* 
 */
kvmemcbox_t* knl_get_kvmemcbox()
{
	kvmemcbox_t* kmb = NULL;
	kvmemcboxmgr_t* kmbmgr = &krlvirmemadrs.kvs_kvmemcboxmgr;
	krlspinlock_lock(&kmbmgr->kbm_lock);

	if (0 < kmbmgr->kbm_cachenr) {
		// 缓存kvmemcbox_t结构的链表 不为空
		if(list_is_empty_careful(&kmbmgr->kbm_cachehead) == FALSE) {
			kmb = list_first_oneobj(&kmbmgr->kbm_cachehead, kvmemcbox_t, kmb_list);
			// 从缓存链表中脱链
			list_del(&kmb->kmb_list);
			kmbmgr->kbm_cachenr--;
			
			kvmemcbox_t_init(kmb);
			// 加入到kbm_kmbhead链表中
			list_add(&kmb->kmb_list, &kmbmgr->kbm_kmbhead);
			kmbmgr->kbm_kmbnr++;
			refcount_inc(&kmb->kmb_cont);
			kmb->kmb_mgr = kmbmgr;
			// kmb = kmb;
			goto out; 
		}
	}

	kmb = new_kvmemcbox();
	if (NULL == kmb) {
		system_error("mem: knl_get_kvmemcbox() null\n");
		goto out;
	}

	list_add(&kmb->kmb_list, &kmbmgr->kbm_kmbhead);
	kmbmgr->kbm_kmbnr++;
	refcount_inc(&kmb->kmb_cont);
	kmb->kmb_mgr = kmbmgr;

out:
	krlspinlock_unlock(&kmbmgr->kbm_lock);	
	return kmb;
}

/**
 * @brief 删除 kvmemcbox_t
 * 
 * @param kmbox kvmemcbox内存指针
 * @return bool_t 
 */
bool_t knl_put_kvmemcbox(kvmemcbox_t* kmbox)
{
	kvmemcboxmgr_t* kmbmgr = &krlvirmemadrs.kvs_kvmemcboxmgr;
	bool_t rets = FALSE;
	if(NULL == kmbox) {
		return FALSE;
	}

	krlspinlock_lock(&kmbmgr->kbm_lock);
	
	// 减1
	refcount_dec(&kmbox->kmb_cont);
	if (refcount_read(&kmbox->kmb_cont) >= 1) {
		rets = TRUE;
		goto out;
	}
	
	// 空闲kvmemcbox_t结构的个数 大于等于 最大缓存个数
	if (kmbmgr->kbm_cachenr >= kmbmgr->kbm_cachemax) {
		list_del(&kmbox->kmb_list);	// 从链表中删除
		if (del_kvmemcbox(kmbox) == FALSE) {
			rets = FALSE;
			goto out;
		} else {
			kmbmgr->kbm_kmbnr--;
			rets = TRUE;
			goto out;
		}
	}

	list_move(&kmbox->kmb_list, &kmbmgr->kbm_cachehead);
	kmbmgr->kbm_cachenr++;
	kmbmgr->kbm_kmbnr--;
	
	rets = TRUE;
out:
	krlspinlock_unlock(&kmbmgr->kbm_lock);
	return rets;
} 

void dump_kvmemcboxmgr(kvmemcboxmgr_t* dump)
{
	if (NULL == dump) {
		return;
	}
    
	kprint("kvmemcboxmgr_t.kmb_kmbnr:%x\n", dump->kbm_kmbnr);
	kprint("kvmemcboxmgr_t.kmb_kmbhead:%x\n", list_is_empty_careful(&dump->kbm_kmbhead));
	kprint("kvmemcboxmgr_t.kmb_cachenr:%x\n", dump->kbm_cachenr);
	kprint("kvmemcboxmgr_t.kmb_cachemax:%x\n", dump->kbm_cachemax);
	kprint("kvmemcboxmgr_t.kmb_cachemin:%x\n", dump->kbm_cachemin);
	kprint("kvmemcboxmgr_t.kmb_cachehead:%x\n", list_is_empty_careful(&dump->kbm_cachehead));
	return;
}

void dump_mem_range(adr_t vadr, uint_t count, uint_t type)
{
	u8_t* form = (u8_t*)vadr;
	if (1 != type) {
		return;
	}

	for (uint_t i = 0; i < count; i++) {
		kprint("dump_mem address:%x value:%x\n", &form[i], form[i]);
	}
	return;
}