/**********************************************************
    设备管理文件krldevice.c
***********************************************************/
#include "cosmostypes.h"
#include "cosmosmctrl.h"

void devtlst_t_init(devtlst_t *initp, uint_t dtype)
{
    initp->dtl_type = dtype;        // 设置设备类型 initp->dtl_nr = 0;
    initp->dtl_nr = 0;
    list_init(&initp->dtl_list);

    return;
}

void devtable_t_init(devtable_t *initp)
{
    list_init(&initp->devt_list);
    krlspinlock_init(&initp->devt_lock);
    list_init(&initp->devt_devlist);
    list_init(&initp->devt_drvlist);
    initp->devt_devnr = 0;
    initp->devt_drvnr = 0;

    // 初始化设备链表
    for (uint_t t = 0; t < DEVICE_MAX; t++) {
        devtlst_t_init(&initp->devt_devclsl[t], t);
    }
    return;
}

void devid_t_init(devid_t *initp, uint_t mty, uint_t sty, uint_t nr)
{
    initp->dev_mtype = mty;
    initp->dev_stype = sty;
    initp->dev_nr = nr;
    return;
}

/**
 * @brief 设备初始化
 * 
 * @param initp 
 */
void device_t_init(device_t *initp)
{
    list_init(&initp->dev_list);
    list_init(&initp->dev_indrvlst);
    list_init(&initp->dev_intbllst);

    krlspinlock_init(&initp->dev_lock);
    initp->dev_count = 0;

    krlsem_t_init(&initp->dev_sem);
    initp->dev_stus = 0;
    initp->dev_flgs = 0;

    devid_t_init(&initp->dev_id, 0, 0, 0);
    initp->dev_intlnenr = 0;

    list_init(&initp->dev_intserlst);
    list_init(&initp->dev_rqlist);
    initp->dev_rqlnr = 0;

    krlsem_t_init(&initp->dev_waitints);
    initp->dev_drv = NULL;
    initp->dev_attrb = NULL;
    initp->dev_privdata = NULL;
    initp->dev_userdata = NULL;
    initp->dev_extdata = NULL;
    initp->dev_name = NULL;

    return;
}

/**
 * @brief 设置驱动ID
 * 
 * @param dverp 驱动程序
 */
void krlretn_driverid(driver_t *dverp)
{
    dverp->drv_id = (uint_t)dverp;
    return;
}

/**
 * @brief 初始化驱动
 * 
 * @param initp 
 */
void driver_t_init(driver_t *initp)
{
    krlspinlock_init(&initp->drv_lock);
    list_init(&initp->drv_list);
    initp->drv_stuts = 0;
    initp->drv_flg = 0;
    krlretn_driverid(initp);
    initp->drv_count = 0;
    krlsem_t_init(&initp->drv_sem);
    initp->drv_safedsc = NULL;
    initp->drv_attrb = NULL;
    initp->drv_privdata = NULL;

    for (uint_t dsi = 0; dsi < IOIF_CODE_MAX; dsi++) {
        initp->drv_dipfun[dsi] = drv_defalt_func;
    }

    list_init(&initp->drv_alldevlist);
    initp->drv_entry = NULL;
    initp->drv_exit = NULL;
    initp->drv_userdata = NULL;
    initp->drv_extdata = NULL;
    initp->drv_name = NULL;
    return;
}

void init_krldevice()
{
    // 初始化系统全局设备表
    devtable_t_init(&osdevtable);
    return;
}

// 驱动程序入口函数
drvstus_t krlrun_driverentry(drventyexit_t drventry)
{
    driver_t *drvp = new_driver_dsc();              // 建立driver_t实例变量
    if (drvp == NULL) {
        return DFCERRSTUS;
    }

    if (drventry(drvp, 0, NULL) == DFCERRSTUS) {    // 运行驱动程序入口函数
        return DFCERRSTUS;
    }

    if (krldriver_add_system(drvp) == DFCERRSTUS) { // 把驱动程序加入系统
        return DFCERRSTUS;
    }

    return DFCOKSTUS;
}

// 驱动程序的初始化
void init_krldriver()
{
    // 遍历驱动程序表中的每个驱动程序入口函数
    for (uint_t ei = 0; osdrvetytabl[ei] != NULL; ei++) {
        // 运行一个驱动程序入口
        if (krlrun_driverentry(osdrvetytabl[ei]) == DFCERRSTUS) {
            hal_sysdie("init driver err");
        }
    }

    kprint("设备驱动初始化成功\n");
    return;
}


drvstus_t del_driver_dsc(driver_t *drvp)
{
    if (krldelete((adr_t)drvp, sizeof(driver_t)) == FALSE) {
        return DFCERRSTUS;
    }

    return DFCOKSTUS;
}

/**
 * @brief 建立一个 driver_t 结构实例
 * 
 * @return driver_t* 实例
 */
driver_t *new_driver_dsc()
{
    driver_t *dp = (driver_t *)krlnew(sizeof(driver_t));
    if (dp == NULL) {
        return NULL;
    }

    driver_t_init(dp);

    return dp;
}

/**
 * @brief 释放设备资源
 * 
 * @param devp 设备指针
 * @return drvstus_t 
 */
drvstus_t del_device_dsc(device_t *devp)
{
    if (krldelete((adr_t)devp, sizeof(device_t)) == FALSE) {
        return DFCERRSTUS;
    }

    return DFCOKSTUS;
}

/**
 * @brief 新建一个设备
 * 
 * @return device_t* 
 */
device_t *new_device_dsc()
{
    device_t *dp = (device_t *)krlnew(sizeof(device_t));
    if (dp == NULL) {
        return NULL;
    }

    device_t_init(dp);

    return dp;
}

/**
 * @brief 驱动默认方法
 * 
 * @param devp 
 * @param iopack 
 * @return drvstus_t 
 */
drvstus_t drv_defalt_func(device_t *devp, void *iopack)
{
    return DFCERRSTUS;
}

/**
 * @brief 设备比较
 * 
 * @param sdidp 
 * @param cdidp 
 * @return bool_t 
 */
bool_t krlcmp_devid(devid_t *sdidp, devid_t *cdidp)
{
    if (sdidp->dev_mtype != cdidp->dev_mtype) {
        return FALSE;
    }

    if (sdidp->dev_stype != cdidp->dev_stype) {
        return FALSE;
    }

    if (sdidp->dev_nr != cdidp->dev_nr) {
        return FALSE;
    }

    return TRUE;
}

drvstus_t krlnew_devid(devid_t *devid)
{
    device_t *findevp;
    drvstus_t rets = DFCERRSTUS;
    cpuflg_t cpufg;
    list_h_t *lstp;
    devtable_t *dtbp = &osdevtable;
    uint_t devmty = devid->dev_mtype;
    uint_t devidnr = 0;

    if (devmty >= DEVICE_MAX) {
        return DFCERRSTUS;
    }

    krlspinlock_cli(&dtbp->devt_lock, &cpufg);
    if (devmty != dtbp->devt_devclsl[devmty].dtl_type) {
        rets = DFCERRSTUS;
        goto return_step;
    }

    if (list_is_empty(&dtbp->devt_devclsl[devmty].dtl_list) == TRUE) {
        rets = DFCOKSTUS;
        devid->dev_nr = 0;
        goto return_step;
    }

    list_for_each(lstp, &dtbp->devt_devclsl[devmty].dtl_list) {
        findevp = list_entry(lstp, device_t, dev_intbllst);
        if (findevp->dev_id.dev_nr > devidnr) {
            devidnr = findevp->dev_id.dev_nr;
        }
    }

    devid->dev_nr = devidnr++;
    rets = DFCOKSTUS;
return_step:
    krlspinunlock_sti(&dtbp->devt_lock, &cpufg);
    return rets;
}

/**
 * @brief 把驱动加入到系统中
 * 
 * @param drvp 驱动
 * @return drvstus_t 
 */
drvstus_t krldriver_add_system(driver_t *drvp)
{
    cpuflg_t cpufg;
    devtable_t *dtbp = &osdevtable;
    krlspinlock_cli(&dtbp->devt_lock, &cpufg);

    list_add(&drvp->drv_list, &dtbp->devt_drvlist);
    dtbp->devt_drvnr++;

    krlspinunlock_sti(&dtbp->devt_lock, &cpufg);
    return DFCOKSTUS;
}

/**
 * @brief 把设备加入到驱动的设备链表中
 * 
 * @param devp 设备指针
 * @param drvp 驱动指针
 * @return drvstus_t 
 */
drvstus_t krldev_add_driver(device_t *devp, driver_t *drvp)
{
    list_h_t *lst;
    device_t *fdevp;
    if (devp == NULL || drvp == NULL) {
        return DFCERRSTUS;
    }
    
    // 遍历驱动上的设备链表，如果找到相同的就代表已经加入了链表
    list_for_each(lst, &drvp->drv_alldevlist) {
        fdevp = list_entry(lst, device_t, dev_indrvlst);
        if (krlcmp_devid(&devp->dev_id, &fdevp->dev_id) == TRUE) {
            return DFCERRSTUS;
        }
    }

    list_add(&devp->dev_indrvlst, &drvp->drv_alldevlist);
    devp->dev_drv = drvp;
    return DFCOKSTUS;
}

/**
 * @brief 向内核(设备表)注册设备
 * 
 * @param devp 
 * @return drvstus_t 
 */
drvstus_t krlnew_device(device_t *devp)
{
    device_t *findevp;
    drvstus_t rets = DFCERRSTUS;
    cpuflg_t cpufg;
    list_h_t *lstp;
    devtable_t *dtbp = &osdevtable;
    uint_t devmty = devp->dev_id.dev_mtype;
    if (devp == NULL) {
        return DFCERRSTUS;
    }

    // 没有驱动的设备不行
    if (devp->dev_drv == NULL) {
        return DFCERRSTUS;
    }

    if (devmty >= DEVICE_MAX) {
        return DFCERRSTUS;
    }

    krlspinlock_cli(&dtbp->devt_lock, &cpufg);  // 加锁
    if (devmty != dtbp->devt_devclsl[devmty].dtl_type) {
        rets = DFCERRSTUS;
        goto return_step;
    }
    
    // 往设备表里挂载设备
    list_for_each(lstp, &dtbp->devt_devclsl[devmty].dtl_list) {
        findevp = list_entry(lstp, device_t, dev_intbllst);
        // 不能有设备ID相同的设备，如果有则出错
        if (krlcmp_devid(&devp->dev_id, &findevp->dev_id) == TRUE) {
            rets = DFCERRSTUS;
            goto return_step;
        }
    }

    // 先把设备加入设备表的全局设备链表
    list_add(&devp->dev_intbllst, &dtbp->devt_devclsl[devmty].dtl_list);
    // 将设备加入对应设备类型的链表中
    list_add(&devp->dev_list, &dtbp->devt_devlist);
    dtbp->devt_devclsl[devmty].dtl_nr++;    // 设备计数加一
    dtbp->devt_devnr++;                     // 总的设备数加一
    rets = DFCOKSTUS;
return_step:
    krlspinunlock_sti(&dtbp->devt_lock, &cpufg);    // 解锁
    return rets;
}

/**
 * @brief 增加设备计数
 * 
 * @param devp 设备指针
 * @return drvstus_t 
 */
drvstus_t krldev_inc_devcount(device_t *devp)
{

    if (devp->dev_count >= (~0UL)) {
        return DFCERRSTUS;
    }

    cpuflg_t cpufg;
    hal_spinlock_saveflg_cli(&devp->dev_lock, &cpufg);
    devp->dev_count++;
    hal_spinunlock_restflg_sti(&devp->dev_lock, &cpufg);
    return DFCOKSTUS;
}

/**
 * @brief 减少设备计数
 * 
 * @param devp 
 * @return drvstus_t 
 */
drvstus_t krldev_dec_devcount(device_t *devp)
{

    if (devp->dev_count < (1)) {
        return DFCERRSTUS;
    }

    cpuflg_t cpufg;
    krlspinlock_cli(&devp->dev_lock, &cpufg);
    devp->dev_count--;
    krlspinunlock_sti(&devp->dev_lock, &cpufg);
    return DFCOKSTUS;
}

drvstus_t krldev_add_request(device_t *devp, objnode_t *request)
{
    cpuflg_t cpufg;
    objnode_t *np = (objnode_t *)request;
    krlspinlock_cli(&devp->dev_lock, &cpufg);
    list_add_tail(&np->on_list, &devp->dev_rqlist);
    devp->dev_rqlnr++;
    krlspinunlock_sti(&devp->dev_lock, &cpufg);
    return DFCOKSTUS;
}

drvstus_t krldev_complete_request(device_t *devp, objnode_t *request)
{
    if (devp == NULL || request == NULL) {
        return DFCERRSTUS;
    }

    if (devp->dev_rqlnr < 1) {
        hal_sysdie("krldev_complete_request err devp->dev_rqlnr<1");
    }

    cpuflg_t cpufg;
    krlspinlock_cli(&devp->dev_lock, &cpufg);
    list_del(&request->on_list);
    devp->dev_rqlnr--;
    krlspinunlock_sti(&devp->dev_lock, &cpufg);
    krlsem_up(&request->on_complesem);
    return DFCOKSTUS;
}

drvstus_t krldev_retn_request(device_t *devp, uint_t iocode, objnode_t **retreq)
{
    if (retreq == NULL || iocode >= IOIF_CODE_MAX) {
        return DFCERRSTUS;
    }

    cpuflg_t cpufg;
    objnode_t *np;
    list_h_t *list;
    drvstus_t rets = DFCERRSTUS;
    krlspinlock_cli(&devp->dev_lock, &cpufg);

    list_for_each(list, &devp->dev_rqlist) {
        np = list_entry(list, objnode_t, on_list);
        if (np->on_opercode == (sint_t)iocode) {
            *retreq = np;
            rets = DFCOKSTUS;
            goto return_step;
        }
    }
    rets = DFCERRSTUS;
    *retreq = NULL;
return_step:
    krlspinunlock_sti(&devp->dev_lock, &cpufg);
    return rets;
}

drvstus_t krldev_wait_request(device_t *devp, objnode_t *request)
{
    if (devp == NULL || request == NULL) {
        return DFCERRSTUS;
    }

    krlsem_down(&request->on_complesem);
    return DFCOKSTUS;
}

void krldev_wait_intupt(device_t *devp)
{
    return;
}

void krldev_up_intupt(device_t *devp)
{
    return;
}

drvstus_t krldev_retn_rqueparm(void *request, buf_t *retbuf, uint_t *retcops, uint_t *retlen, uint_t *retioclde, uint_t *retbufcops, size_t *retbufsz)
{
    objnode_t *ondep = (objnode_t *)request;
    if (ondep == NULL) {
        return DFCERRSTUS;
    }

    if (retbuf != NULL) {
        *retbuf = ondep->on_buf;
    }

    if (retcops != NULL) {
        *retcops = ondep->on_currops;
    }

    if (retlen != NULL) {
        *retlen = ondep->on_len;
    }

    if (retioclde != NULL) {
        *retioclde = ondep->on_ioctrd;
    }

    if (retbufcops != NULL) {
        *retbufcops = ondep->on_bufcurops;
    }

    if (retbufsz != NULL) {
        *retbufsz = ondep->on_bufsz;
    }

    return DFCOKSTUS;
}

device_t *krlonidfl_retn_device(void *dfname, uint_t flgs)
{
    device_t *findevp;
    cpuflg_t cpufg;
    list_h_t *lstp;
    devtable_t *dtbp = &osdevtable;

    if (dfname == NULL || flgs != DIDFIL_IDN) {
        return NULL;
    }

    devid_t *didp = (devid_t *)dfname;
    uint_t devmty = didp->dev_mtype;
    if (devmty >= DEVICE_MAX) {
        return NULL;
    }

    krlspinlock_cli(&dtbp->devt_lock, &cpufg);
    if (devmty != dtbp->devt_devclsl[devmty].dtl_type) {
        findevp = NULL;
        goto return_step;
    }

    list_for_each(lstp, &dtbp->devt_devclsl[devmty].dtl_list) {
        findevp = list_entry(lstp, device_t, dev_intbllst);
        if (krlcmp_devid(didp, &findevp->dev_id) == TRUE) {
            // findevp = findevp;
            goto return_step;
        }
    }

    findevp = NULL;
return_step:
    krlspinunlock_sti(&dtbp->devt_lock, &cpufg);
    return findevp;
}

/**
 * @brief 安装中断回调函数接口
 * 
 * @param devp 设备实例
 * @param handle 设备中断回调函数
 * @param phyiline 中断向量号
 * @return drvstus_t 
 */
drvstus_t krlnew_devhandle(device_t *devp, intflthandle_t handle, uint_t phyiline)
{
    // 调用内核层中断框架接口函数
    intserdsc_t *sdp = krladd_irqhandle(devp, handle, phyiline);
    if (sdp == NULL){
        return DFCERRSTUS;
    }

    cpuflg_t cpufg;
    krlspinlock_cli(&devp->dev_lock, &cpufg);

    // 将中断服务描述符结构挂入这个设备结构中
    list_add(&sdp->s_indevlst, &devp->dev_intserlst);
    devp->dev_intlnenr++;
    krlspinunlock_sti(&devp->dev_lock, &cpufg);
    return DFCOKSTUS;
}

/**
 * @brief 发送设备IO
 * 
 * @param nodep 
 * @return drvstus_t 
 */
drvstus_t krldev_io(objnode_t *nodep)
{
    // 获取设备对象
    device_t *devp = (device_t *)(nodep->on_objadr);
    // 检查操作对象类型是不是文件或者设备，对象地址是不是为空
    if ((nodep->on_objtype != OBJN_TY_DEV && nodep->on_objtype != OBJN_TY_FIL) || nodep->on_objadr == NULL) {
        return DFCERRSTUS;
    }

    // 检查IO操作码是不是合乎要求
    if (nodep->on_opercode < 0 || nodep->on_opercode >= IOIF_CODE_MAX) {
        return DFCERRSTUS;
    }

    return krldev_call_driver(devp, nodep->on_opercode, 0, 0, NULL, nodep);
}

/**
 * @brief 调用设备驱动
 *  1. IO 操作码为索引调用驱动程序功能分派函数数组中的函数，并把设备和 objnode_t 结构传递进去
 * 
 * @param devp 设备指针
 * @param iocode 操作码
 * @param val1 
 * @param val2 
 * @param p1 
 * @param p2 
 * @return drvstus_t 
 */
drvstus_t krldev_call_driver(device_t *devp, uint_t iocode, uint_t val1, uint_t val2, void *p1, void *p2)
{
    driver_t *drvp = NULL;
    // 检查设备和IO操作码
    if (devp == NULL || iocode >= IOIF_CODE_MAX) {
        return DFCERRSTUS;
    }

    drvp = devp->dev_drv;
    // 检查设备是否有驱动程序
    if (drvp == NULL) {
        return DFCERRSTUS;
    }

    // 用IO操作码为索引调用驱动程序功能分派函数数组中的函数
    return drvp->drv_dipfun[iocode](devp, p2);
}
