/**********************************************************
    内核服务文件krlservice.c
***********************************************************/
#include "cosmostypes.h"
#include "cosmosmctrl.h"

sysstus_t krlservice(uint_t inr, void* sframe)
{
    // 判断服务号是否大于最大服务号
    if (INR_MAX <= inr) {
        return SYSSTUSERR;
    }

    // 判断是否有服务接口函数
    if (NULL == osservicetab[inr]) {
        return SYSSTUSERR;
    }
    
    return osservicetab[inr](inr, (stkparame_t*)sframe);
}