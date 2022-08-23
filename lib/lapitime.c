/**********************************************************
    文件管理API文件lapitime.c
***********************************************************/
#include "libtypes.h"
#include "libheads.h"


/**
 * @brief 时间API调用
 *  1. time->api_time->API_ENTRY_PARE1(INR_TIME, rets, ttime)
 *  2. 功能编号为INR_TIME
 *  3. 设置返回值
 *  4. 设置参数
 *  5. 触发 int 255
 * 
 * @param ttime 
 * @return sysstus_t 
 */
sysstus_t api_time(buf_t ttime)
{
    sysstus_t rets;
    API_ENTRY_PARE1(INR_TIME, rets, ttime); // 处理参数，执行int指令
    return rets;
}