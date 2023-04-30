/**********************************************************
    时钟管理文件krltick.c
***********************************************************/
#include "cosmostypes.h"
#include "cosmosmctrl.h"

/**
 * @brief 初始化8254定时器
 *  systick 设备的主要功能和作用是每隔 1ms 产生一个中断，相当于一个定时器
 *  每次时间到达就产生一个中断向系统报告又过了 1ms，相当于千分之一秒，即每秒钟内产生 1000 次中断
 *  PTIPROT1: 计数器0(0x40端口)的作用是产生时钟信号，这个时钟是指连接到主片IRQ0 引脚的那个时钟
 */
void init_8254()
{
    out_u8_p(PTIPROTM, TIMEMODE);

    out_u8_p(PTIPROT1, HZLL);

    out_u8_p(PTIPROT1, HZHH);

    return;
}

// 设置驱动对应的功能(函数)
void systick_set_driver(driver_t *drvp)
{
    drvp->drv_dipfun[IOIF_CODE_OPEN] = systick_open;
    drvp->drv_dipfun[IOIF_CODE_CLOSE] = systick_close;
    drvp->drv_dipfun[IOIF_CODE_READ] = systick_read;
    drvp->drv_dipfun[IOIF_CODE_WRITE] = systick_write;
    drvp->drv_dipfun[IOIF_CODE_LSEEK] = systick_lseek;
    drvp->drv_dipfun[IOIF_CODE_IOCTRL] = systick_ioctrl;
    drvp->drv_dipfun[IOIF_CODE_DEV_START] = systick_dev_start;
    drvp->drv_dipfun[IOIF_CODE_DEV_STOP] = systick_dev_stop;
    drvp->drv_dipfun[IOIF_CODE_SET_POWERSTUS] = systick_set_powerstus;
    drvp->drv_dipfun[IOIF_CODE_ENUM_DEV] = systick_enum_dev;
    drvp->drv_dipfun[IOIF_CODE_FLUSH] = systick_flush;
    drvp->drv_dipfun[IOIF_CODE_SHUTDOWN] = systick_shutdown;
    drvp->drv_name = "systick0drv";
    return;
}

/**
 * @brief 设置设备属性
 * 
 * @param devp 设备指针
 * @param drvp 驱动指针
 */
void systick_set_device(device_t *devp, driver_t *drvp)
{
    devp->dev_flgs = DEVFLG_SHARE;              // 设备标志。共享
    devp->dev_stus = DEVSTS_NORML;              // 设备状态。正常
    devp->dev_id.dev_mtype = SYSTICK_DEVICE;    // 设备类型号。系统TICK设备
    devp->dev_id.dev_stype = 0;                 // 设备子类型
    devp->dev_id.dev_nr = 0;                    // 设备号

    devp->dev_name = "systick0";                // 设置设备名称
    return;
}

/**
 * @brief 驱动程序入口
 *  1. 建立设备描述符结构
 *  2. 将驱动程序的功能函数指针，设置到driver_t结构中的drv_dipfun数组中
 *  3. 将设备挂载到驱动中
 *  4. 调用krlnew_device向内核注册设备
 *  5. 确认没有相同设备ID，注册到对应设备类型的列表以及全局设备列表
 * 
 * @param drvp 驱动程序指针
 * @param val 
 * @param p 
 * @return drvstus_t 
 */
drvstus_t systick_entry(driver_t *drvp, uint_t val, void *p)
{
    // drvp是内核传递进来的参数，不能为NULL
    if (drvp == NULL) {
        return DFCERRSTUS;
    }

    // 新建一个设备
    device_t *devp = new_device_dsc();
    if (devp == NULL) {
        return DFCERRSTUS;
    }

    // 设置驱动对应的功能(函数)
    systick_set_driver(drvp);
    // 设置设备属性
    systick_set_device(devp, drvp);
    
    // 把设备加入到驱动的设备链表中
    if (krldev_add_driver(devp, drvp) == DFCERRSTUS) {
        // 注意释放资源
        if (del_device_dsc(devp) == DFCERRSTUS) {
            return DFCERRSTUS;
        }
        return DFCERRSTUS;
    }

    // 向内核(设备表)注册设备
    if (krlnew_device(devp) == DFCERRSTUS) {
        // 注意释放资源
        if (del_device_dsc(devp) == DFCERRSTUS) {
            return DFCERRSTUS;
        }
        return DFCERRSTUS;
    }

    // 安装中断回调函数接口. 中断号32～255为 Maskable Interrupts
    if (krlnew_devhandle(devp, systick_handle, 0x20) == DFCERRSTUS) {
        //注意释放资源
        return DFCERRSTUS; 
    }

    // 初始化8254定时器
    init_8254();

    // 开启中断请求
    if (krlenable_intline(20) == DFCERRSTUS) {
        return DFCERRSTUS;
    }
    
    return DFCOKSTUS;
}

drvstus_t systick_exit(driver_t *drvp, uint_t val, void *p)
{
    return DFCERRSTUS;
}

/**
 * @brief 设备中断处理函数
 *  1. systick 设备每秒钟产生 1000 次中断
 * 
 * @param ift_nr 
 * @param devp 
 * @param sframe 
 * @return drvstus_t 
 */
drvstus_t systick_handle(uint_t ift_nr, void *devp, void *sframe)
{
    // 更新当前进程的tick
    krlthd_inc_tick(krlsched_retn_currthread());
    krlupdate_times_from_cmos();
    return DFCOKSTUS;
}

/**
 * @brief 打开设备函数
 * 
 * @param devp 设备指针
 * @param iopack IO包
 * @return drvstus_t 
 */
drvstus_t systick_open(device_t *devp, void *iopack)
{
    // 增加设备计数
    krldev_inc_devcount(devp);
    // 返回成功完成的状态
    return DFCOKSTUS;
}

/**
 * @brief 关闭设备
 * 
 * @param devp 
 * @param iopack 
 * @return drvstus_t 
 */
drvstus_t systick_close(device_t *devp, void *iopack)
{
    // 减少设备计数
    krldev_dec_devcount(devp);
    // 返回成功完成的状态
    return DFCOKSTUS;
}

/**
 * @brief 读设备数据函数
 * 
 * @param devp 
 * @param iopack 
 * @return drvstus_t 
 */
drvstus_t systick_read(device_t *devp, void *iopack)
{
    return DFCERRSTUS;
}

/**
 * @brief 写设备数据函数
 * 
 * @param devp 
 * @param iopack 
 * @return drvstus_t 
 */
drvstus_t systick_write(device_t *devp, void *iopack)
{
    return DFCERRSTUS;
}

/**
 * @brief 调整读写设备数据位置函数
 * 
 * @param devp 
 * @param iopack 
 * @return drvstus_t 
 */
drvstus_t systick_lseek(device_t *devp, void *iopack)
{
    return DFCERRSTUS;
}

/**
 * @brief 控制设备函数
 * 
 * @param devp 
 * @param iopack 
 * @return drvstus_t 
 */
drvstus_t systick_ioctrl(device_t *devp, void *iopack)
{
    return DFCERRSTUS;
}

/**
 * @brief 开启设备函数
 * 
 * @param devp 
 * @param iopack 
 * @return drvstus_t 
 */
drvstus_t systick_dev_start(device_t *devp, void *iopack)
{
    return DFCERRSTUS;
}

/**
 * @brief 停止设备函数
 * 
 * @param devp 
 * @param iopack 
 * @return drvstus_t 
 */
drvstus_t systick_dev_stop(device_t *devp, void *iopack)
{
    return DFCERRSTUS;
}

// 设置设备电源函数
drvstus_t systick_set_powerstus(device_t *devp, void *iopack)
{
    return DFCERRSTUS;
}

// 枚举设备函数
drvstus_t systick_enum_dev(device_t *devp, void *iopack)
{
    return DFCERRSTUS;
}

// 刷新设备缓存函数
drvstus_t systick_flush(device_t *devp, void *iopack)
{
    return DFCERRSTUS;
}

// 设备关机函数
drvstus_t systick_shutdown(device_t *devp, void *iopack)
{
    return DFCERRSTUS;
}
