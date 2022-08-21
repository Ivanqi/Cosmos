/**********************************************************
    内核层中断处理文件krlintupt.c
***********************************************************/
#include "cosmostypes.h"
#include "cosmosmctrl.h"

/**
 * @brief 创建了一个 intserdsc_t 结构，用来保存设备和其驱动程序提供的中断回调函数
 * 
 * @param device 
 * @param handle 
 * @param phyiline 
 * @return intserdsc_t* 
 */
intserdsc_t *krladd_irqhandle(void *device, intflthandle_t handle, uint_t phyiline)
{

    if (device == NULL || handle == NULL) {
        return NULL;
    }

    intfltdsc_t *intp = hal_retn_intfltdsc(phyiline);
    if (intp == NULL) {
        return NULL;
    }

    intserdsc_t *serdscp = (intserdsc_t *)krlnew(sizeof(intserdsc_t));
    if (serdscp == NULL) {
        return NULL;
    }

    // 初始化intserdsc_t结构体实例变量，并把设备指针和回调函数放入其中
    intserdsc_t_init(serdscp, 0, intp, device, handle);

    // 把intserdsc_t结构体实例变量挂载到中断异常描述符结构中
    if (hal_add_ihandle(intp, serdscp) == FALSE) {
        if (krldelete((adr_t)serdscp, sizeof(intserdsc_t)) == FALSE) {
            hal_sysdie("krladd_irqhandle ERR");
        }
        return NULL;
    }

    return serdscp;
}

/**
 * @brief 开启中断请求
 * 
 * @param ifdnr 
 * @return drvstus_t 
 */
drvstus_t krlenable_intline(uint_t ifdnr)
{
   return hal_enable_intline(ifdnr);
}

drvstus_t krldisable_intline(uint_t ifdnr)
{
    return hal_disable_intline(ifdnr);
}
