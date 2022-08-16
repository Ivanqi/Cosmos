/**********************************************************
    内核等待链头文件krlwaitlist.c
***********************************************************/
#include "cosmostypes.h"
#include "cosmosmctrl.h"

void kwlst_t_init(kwlst_t* initp)
{
    krlspinlock_init(&initp->wl_lock);
    initp->wl_tdnr=0;
    list_init(&initp->wl_list);
    return;
}

void krlwlst_wait(kwlst_t* wlst)
{
    krlsched_wait(wlst);
    return;
}

void krlwlst_up(kwlst_t* wlst)
{
    if (list_is_empty_careful(&wlst->wl_list) == TRUE) {
        return;
    }
    krlsched_up(wlst);
    
    return;
}

void krlwlst_allup(kwlst_t* wlst)
{
    while (list_is_empty_careful(&wlst->wl_list) == FALSE) {
        krlsched_up(wlst);

    }

    return;
}

/**
 * @brief 进程加入到 kwlst_t 等待结构
 * 
 * @param wlst 
 * @param tdp 
 */
void krlwlst_add_thread(kwlst_t* wlst, thread_t* tdp)
{
    cpuflg_t cufg;
    krlspinlock_cli(&wlst->wl_lock, &cufg);
    
    list_add(&tdp->td_list, &wlst->wl_list);
    wlst->wl_tdnr++;   
    krlspinunlock_sti(&wlst->wl_lock, &cufg);
    return;
}

/**
 * @brief 取出等待数据结构第一个进程并从等待数据结构中删除
 * 
 * @param wlst 
 * @return thread_t* 
 */
thread_t* krlwlst_del_thread(kwlst_t *wlst)
{
    thread_t* tdp=NULL;
    cpuflg_t cufg;
    list_h_t* list;
    krlspinlock_cli(&wlst->wl_lock, &cufg);
    list_for_each(list,&wlst->wl_list) {
        tdp = list_entry(list,thread_t,td_list);

        if (tdp == NULL || wlst->wl_tdnr < 1) {
            tdp = NULL;
            goto return_step;
        }

        list_del(&tdp->td_list);
        wlst->wl_tdnr--;
        tdp = tdp;
        goto return_step;
        
    }

    tdp = NULL;
return_step:    
    krlspinunlock_sti(&wlst->wl_lock, &cufg);
    return tdp;
}
