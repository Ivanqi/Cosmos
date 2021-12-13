/**********************************************************
    COSMOS内存管理API文件lapimm.c
***********************************************************/
#include "libtypes.h"
#include "libheads.h"

// 请求分配内存服务
void* api_mallocblk(size_t blksz)
{
    void* retadr;
    // 把系统服务号，返回变量和请求分配的内存大小
    API_ENTRY_PARE1(INR_MM_ALLOC, retadr, blksz);
    return retadr;
}

sysstus_t api_mfreeblk(void* fradr, size_t blksz)
{
    sysstus_t retstus;
    API_ENTRY_PARE2(INR_MM_FREE, retstus, fradr, blksz);
    return retstus;
}