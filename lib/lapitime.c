/**********************************************************
    文件管理API文件lapitime.c
***********************************************************/
#include "libtypes.h"
#include "libheads.h"


sysstus_t api_time(buf_t ttime)
{
    sysstus_t rets;
    API_ENTRY_PARE1(INR_TIME, rets, ttime); // 处理参数，执行int指令
    return rets;
}