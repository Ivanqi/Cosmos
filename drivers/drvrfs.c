/**********************************************************
    内存文件系统文件drvfs.c
***********************************************************/
#include "cosmostypes.h"
#include "cosmosmctrl.h"

void rfsdevext_t_init(rfsdevext_t *initp)
{
    krlspinlock_init(&initp->rde_lock);
    list_init(&initp->rde_list);
    initp->rde_flg = 0;
    initp->rde_stus = 0;
    initp->rde_mstart = NULL;
    initp->rde_msize = 0;
    initp->rde_ext = NULL;
    return;
}

// 初始化根目录
void rfsdir_t_init(rfsdir_t *initp)
{
    initp->rdr_stus = 0;            // 目录状态
    initp->rdr_type = RDR_NUL_TYPE; // 目录类型
    initp->rdr_blknr = 0;
    for (uint_t i = 0; i < DR_NM_MAX; i++) {
        initp->rdr_name[i] = 0;
    }
    return;
}

void filblks_t_init(filblks_t *initp)
{
    initp->fb_blkstart = 0;
    initp->fb_blknr = 0;
    return;
}

/**
 * 初始化超级块
 *  1. 文件系统的超级块，保存在储存设备的第一个 4KB 大小的逻辑储存块
 *  2. 但是它本身的大小没有 4KB，多余的空间用于以后扩展
 */
void rfssublk_t_init(rfssublk_t *initp)
{
    krlspinlock_init(&initp->rsb_lock);
    // 标志就是一个数字而已，无其它意义
    initp->rsb_mgic = 0x142422;
    // 文件系统版本为1
    initp->rsb_vec = 1;
    initp->rsb_flg = 0;
    initp->rsb_stus = 0;
    // 超级块本身的大小
    initp->rsb_sz = sizeof(rfssublk_t);
    // 超级块占用多少个逻辑储存块
    initp->rsb_sblksz = 1;
    // 逻辑储存块的大小为4KB
    initp->rsb_dblksz = FSYS_ALCBLKSZ;
    // 位图块从第1个逻辑储存块开始，超级块占用第0个逻辑储存块
    initp->rsb_bmpbks = 1;
    initp->rsb_bmpbknr = 0;
    initp->rsb_fsysallblk = 0;
    // 初始化根目录
    rfsdir_t_init(&initp->rsb_rootdir);
    return;
}

// 文件管理头. 512 字节空间
void fimgrhd_t_init(fimgrhd_t *initp)
{
    initp->fmd_stus = 0;
    initp->fmd_type = FMD_NUL_TYPE;
    initp->fmd_flg = 0;
    initp->fmd_sfblk = 0;
    initp->fmd_acss = 0;
    initp->fmd_newtime = 0;
    initp->fmd_acstime = 0;
    initp->fmd_fileallbk = 0;
    initp->fmd_filesz = 0;
    initp->fmd_fileifstbkoff = 0x200;
    initp->fmd_fileiendbkoff = 0;
    initp->fmd_curfwritebk = 0;
    initp->fmd_curfinwbkoff = 0;
    
    for (uint_t i = 0; i < FBLKS_MAX; i++) {
        filblks_t_init(&initp->fmd_fleblk[i]);
    }

    initp->fmd_linkpblk = 0;
    initp->fmd_linknblk = 0;
    return;
}

/**
 * 分配模拟储存设备的内存空间
 *  1. 分配了一个内存空间和一个 rfsdevext_t 结构实例变量
 *  2. rfsdevext_t 结构中保存了内存空间的地址和大小
 *  3. 而 rfsdevext_t 结构的地址放在了 device_t 结构的 dev_extdata 字段中
 */
drvstus_t new_rfsdevext_mmblk(device_t *devp, size_t blksz)
{
    // 分配模拟储存介质的内存空间，大小为4MB
    adr_t blkp = krlnew(blksz);
    // 分配rfsdevext_t结构实例的内存空间
    rfsdevext_t *rfsexp = (rfsdevext_t *)krlnew(sizeof(rfsdevext_t));
    if (blkp == NULL || rfsexp == NULL) {
        return DFCERRSTUS;
    }

    // 初始化rfsdevext_t结构
    rfsdevext_t_init(rfsexp);
    rfsexp->rde_mstart = (void *)blkp;
    rfsexp->rde_msize = blksz;
    // 把rfsdevext_t结构的地址放入device_t 结构的dev_extdata字段中，这里dev_extdata字段就起作用了
    devp->dev_extdata = (void *)rfsexp;
    return DFCOKSTUS;
}

// 返回设备扩展数据结构
rfsdevext_t *ret_rfsdevext(device_t *devp)
{
    return (rfsdevext_t *)devp->dev_extdata;
}

/**
 * 根据块号返回储存设备的块地址
 *  1. 这个函数和 ret_rfsdevblk 函数将会一起根据块号，计算出内存地址
 */
void *ret_rfsdevblk(device_t *devp, uint_t blknr)
{
    rfsdevext_t *rfsexp = ret_rfsdevext(devp);
    // 块号乘于块大小的结果再加上开始地址（用于模拟储存设备的内存空间的开始地址）
    void *blkp = rfsexp->rde_mstart + (blknr * FSYS_ALCBLKSZ);
    // 如果该地址没有落在储存入设备的空间中，就返回NULL表示出错
    if (blkp >= (void *)((size_t)rfsexp->rde_mstart + rfsexp->rde_msize)) {
        return NULL;
    }
    // 返回块地址
    return blkp;
}

// 设备扩展数据结构长度
uint_t ret_rfsdevmaxblknr(device_t *devp)
{
    rfsdevext_t *rfsexp = ret_rfsdevext(devp);
    return (uint_t)(((size_t)rfsexp->rde_msize) / FSYS_ALCBLKSZ);
}

// 把逻辑储存块中的数据，读取到4KB大小的缓冲区中
drvstus_t read_rfsdevblk(device_t *devp, void *rdadr, uint_t blknr)
{
    // 获取逻辑储存块地址
    void *p = ret_rfsdevblk(devp, blknr);
    if (p == NULL) {
        return DFCERRSTUS;
    }
    // 把逻辑储存块中的数据复制到缓冲区中
    hal_memcpy(p, rdadr, FSYS_ALCBLKSZ);
    return DFCOKSTUS;
}

// 把4KB大小的缓冲区中的内容，写入到储存设备的某个逻辑储存块中
drvstus_t write_rfsdevblk(device_t *devp, void *weadr, uint_t blknr)
{
    // 返回储存设备中第blknr块的逻辑存储块的地址
    void *p = ret_rfsdevblk(devp, blknr);
    if (p == NULL) {
        return DFCERRSTUS;
    }
    // 复制数据到逻辑储存块中
    hal_memcpy(weadr, p, FSYS_ALCBLKSZ);
    return DFCOKSTUS;
}

void *new_buf(size_t bufsz)
{
    // 分配缓冲区
    return (void *)krlnew(bufsz);
}

void del_buf(void *buf, size_t bufsz)
{
    // 释放缓冲区
    if (krldelete((adr_t)buf, bufsz) == FALSE) {
        hal_sysdie("del buf err");
    }
    return;
}

void rfs_set_driver(driver_t *drvp)
{
    drvp->drv_dipfun[IOIF_CODE_OPEN] = rfs_open;
    drvp->drv_dipfun[IOIF_CODE_CLOSE] = rfs_close;
    drvp->drv_dipfun[IOIF_CODE_READ] = rfs_read;
    drvp->drv_dipfun[IOIF_CODE_WRITE] = rfs_write;
    drvp->drv_dipfun[IOIF_CODE_LSEEK] = rfs_lseek;
    drvp->drv_dipfun[IOIF_CODE_IOCTRL] = rfs_ioctrl;
    drvp->drv_dipfun[IOIF_CODE_DEV_START] = rfs_dev_start;
    drvp->drv_dipfun[IOIF_CODE_DEV_STOP] = rfs_dev_stop;
    drvp->drv_dipfun[IOIF_CODE_SET_POWERSTUS] = rfs_set_powerstus;
    drvp->drv_dipfun[IOIF_CODE_ENUM_DEV] = rfs_enum_dev;
    drvp->drv_dipfun[IOIF_CODE_FLUSH] = rfs_flush;
    drvp->drv_dipfun[IOIF_CODE_SHUTDOWN] = rfs_shutdown;
    drvp->drv_name = "rfsdrv";
    return;
}

void rfs_set_device(device_t *devp, driver_t *drvp)
{
    // 设备类型为文件系统类型
    devp->dev_flgs = DEVFLG_SHARE;
    devp->dev_stus = DEVSTS_NORML;
    devp->dev_id.dev_mtype = FILESYS_DEVICE;
    devp->dev_id.dev_stype = 0;
    devp->dev_id.dev_nr = 0;

    // 设备名称为rfs
    devp->dev_name = "rfs";
    return;
}

// rfs驱动程序入口
drvstus_t rfs_entry(driver_t *drvp, uint_t val, void *p)
{
    if (drvp == NULL) {
        return DFCERRSTUS;
    }

    // 分配device_t结构并对其进行初级初始化
    device_t *devp = new_device_dsc();
    if (devp == NULL) {
        return DFCERRSTUS;
    }

    rfs_set_driver(drvp);
    rfs_set_device(devp, drvp);
    // 分配模拟储存设备的内存空间
    if (new_rfsdevext_mmblk(devp, FSMM_BLK) == DFCERRSTUS){
        // 注意释放资源。
        if (del_device_dsc(devp) == DFCERRSTUS){
            return DFCERRSTUS;
        }
        return DFCERRSTUS;
    }

    // 把设备加入到驱动程序之中
    if (krldev_add_driver(devp, drvp) == DFCERRSTUS) {
        // 注意释放资源。
        if (del_device_dsc(devp) == DFCERRSTUS) {
            return DFCERRSTUS;
        }
        return DFCERRSTUS;
    }

    // 向内核注册设备
    if (krlnew_device(devp) == DFCERRSTUS) {
        // 注意释放资源
        if (del_device_dsc(devp) == DFCERRSTUS) {
            return DFCERRSTUS;
        }
        return DFCERRSTUS;
    }

    // 初始化rfs
    init_rfs(devp);
    // 测试文件系统超级块
    test_rfs_rootdir(devp);
    test_rfs_bitmap(devp);
    test_rfs_superblk(devp);
    return DFCOKSTUS;
}

drvstus_t rfs_exit(driver_t *drvp, uint_t val, void *p)
{
    return DFCERRSTUS;
}

// 串联整合
drvstus_t rfs_open(device_t *devp, void *iopack)
{
    objnode_t *obp = (objnode_t *)iopack;

    // 根据objnode_t结构中的访问标志进行判断
    if (obp->on_acsflgs == FSDEV_OPENFLG_OPEFILE) {
        return rfs_open_file(devp, iopack);
    }

    if (obp->on_acsflgs == FSDEV_OPENFLG_NEWFILE) {
        return rfs_new_file(devp, obp->on_fname, 0);
    }

    return DFCERRSTUS;
}

drvstus_t rfs_close(device_t *devp, void *iopack)
{
    return rfs_close_file(devp, iopack);
}

drvstus_t rfs_read(device_t *devp, void *iopack)
{
    // 调用读文件操作的接口函数
    return rfs_read_file(devp, iopack);
}

drvstus_t rfs_write(device_t *devp, void *iopack)
{
    // 调用写文件操作的接口函数
    return rfs_write_file(devp, iopack);
}

drvstus_t rfs_lseek(device_t *devp, void *iopack)
{
    return DFCERRSTUS;
}

drvstus_t rfs_ioctrl(device_t *devp, void *iopack)
{
    objnode_t *obp = (objnode_t *)iopack;
    // 根据objnode_t结构中的控制码进行判断
    if (obp->on_ioctrd == FSDEV_IOCTRCD_DELFILE) {
        // 调用删除文件操作的接口函数
        return rfs_del_file(devp, obp->on_fname, 0);
    }

    return DFCERRSTUS;
}

drvstus_t rfs_dev_start(device_t *devp, void *iopack)
{
    return DFCERRSTUS;
}

drvstus_t rfs_dev_stop(device_t *devp, void *iopack)
{
    return DFCERRSTUS;
}

drvstus_t rfs_set_powerstus(device_t *devp, void *iopack)
{
    return DFCERRSTUS;
}

drvstus_t rfs_enum_dev(device_t *devp, void *iopack)
{
    return DFCERRSTUS;
}

drvstus_t rfs_flush(device_t *devp, void *iopack)
{
    return DFCERRSTUS;
}

drvstus_t rfs_shutdown(device_t *devp, void *iopack)
{
    return DFCERRSTUS;
}

/**
 * 新建文件的接口函数
 *  1. 从文件路径名中提取出纯文件名，检查储存设备上是否已经存在这个文件
 *  2. 分配一个空闲的逻辑储存块，并在根目录文件的末尾写入这个新建文件对应的 rfsdir_t 结构
 *  3. 在一个新的 4KB 大小的缓冲区中，初始化新建文件对应的 fimgrhd_t 结构
 *  4. 把第 3 步对应的缓冲区里的数据，写入到先前分配的空闲逻辑储存块中
 */
drvstus_t rfs_new_file(device_t *devp, char_t *fname, uint_t flg)
{
    // 在栈中分配一个字符缓冲区并清零
    char_t fne[DR_NM_MAX];
    hal_memset((void *)fne, 0, DR_NM_MAX);
    // 从文件路径名中提取出纯文件名
    if (rfs_ret_fname(fne, fname) != 0) {
        return DFCERRSTUS;
    }

    // 检查储存介质上是否已经存在这个新建的文件，如果是则返回错误
    if (rfs_chkfileisindev(devp, fne) != 0) {
        return DFCERRSTUS;
    }

    // 调用实际建立文件的函数
    return rfs_new_dirfileblk(devp, fne, RDR_FIL_TYPE, 0);
}

/**
 * 文件删除的接口函数
 *  1. 从文件路径名中提取出纯文件名
 *  2. 获取根目录文件，从根目录文件中查找待删除文件的 rfsdir_t 结构，然后释放该文件占用的逻辑储存块
 *  3. 初始化与待删除文件相对应的 rfsdir_t 结构，并设置 rfsdir_t 结构的类型为 RDR_DEL_TYPE
 *  4. 释放根目录文件
 */
drvstus_t rfs_del_file(device_t *devp, char_t *fname, uint_t flg)
{
    if (flg != 0) {
        return DFCERRSTUS;
    }

    drvstus_t rets = rfs_del_dirfileblk(devp, fname, RDR_FIL_TYPE, 0);
    return rets;
}

/**
 * 读取文件数据的接口函数
 *  1. 检查 objnode_t 结构中用于存放文件数据的缓冲区及其大小
 *  2. 检查 imgrhd_t 结构中文件相关的信息
 *  3. 把文件的数据读取到 objnode_t 结构中指向的缓冲区中
 */
drvstus_t rfs_read_file(device_t *devp, void *iopack)
{
    objnode_t *obp = (objnode_t *)iopack;
    // 检查文件是否已经打开，以及用于存放文件数据的缓冲区和它的大小是否合理
    if (obp->on_finode == NULL || obp->on_buf == NULL || obp->on_bufsz != FSYS_ALCBLKSZ) {
        return DFCERRSTUS;
    }

    return rfs_readfileblk(devp, (fimgrhd_t *)obp->on_finode, obp->on_buf, obp->on_len);
}

/**
 * 写入文件数据的接口函数
 *  1. 检查 objnode_t 结构中用于存放文件数据的缓冲区及其大小
 *  2. 检查 imgrhd_t 结构中文件相关的信息
 *  3. 把文件的数据读取到 objnode_t 结构中指向的缓冲区中
 */
drvstus_t rfs_write_file(device_t *devp, void *iopack)
{
    objnode_t *obp = (objnode_t *)iopack;
    // 检查文件是否已经打开，以及用于存放文件数据的缓冲区和它的大小是否合理
    if (obp->on_finode == NULL || obp->on_buf == NULL || obp->on_bufsz != FSYS_ALCBLKSZ) {
        return DFCERRSTUS;
    }

    return rfs_writefileblk(devp, (fimgrhd_t *)obp->on_finode, obp->on_buf, obp->on_len);
}

/**
 * 打开文件的接口函数
 *  1. 从 objnode_t 结构的文件路径提取文件名
 *  2. 获取根目录文件，在该文件中搜索对应的 rfsdir_t 结构，看看文件是否存在
 *  3. 分配一个 4KB 缓存区，把该文件对应的 rfsdir_t 结构中指向的逻辑储存块读取到缓存区中，然后释放根目录文件
 *  4. 把缓冲区中的 fimgrhd_t 结构的地址，保存到 objnode_t 结构的 on_finode 域中
 */
drvstus_t rfs_open_file(device_t *devp, void *iopack)
{
    objnode_t *obp = (objnode_t *)iopack;
    // 检查objnode_t中的文件路径名
    if (obp->on_fname == NULL) {
        return DFCERRSTUS;
    }

    // 调用打开文件的核心函数
    void *fmdp = rfs_openfileblk(devp, (char_t *)obp->on_fname);
    if (fmdp == NULL) {
        return DFCERRSTUS;
    }

    // 把返回的fimgrhd_t结构的地址保存到objnode_t中的on_finode字段中
    obp->on_finode = fmdp;
    return DFCOKSTUS;
}

/**
 * 关闭文件的接口函数
 *  1. 首先检查文件是否已经打开
 *  2. 然后把文件写入到对应的逻辑储存块中，完成数据的同步
 *  3. 最后释放文件数据占用的缓冲区
 */
drvstus_t rfs_close_file(device_t *devp, void *iopack)
{
    objnode_t *obp = (objnode_t *)iopack;
    // 检查文件是否已经打开了
    if (obp->on_finode == NULL) {
        return DFCERRSTUS;
    }

    return rfs_closefileblk(devp, obp->on_finode);
}

sint_t rfs_strcmp(char_t *str_s, char_t *str_d)
{
    for (;;) {
        if (*str_s != *str_d) {
            return 0;
        }

        if (*str_s == 0) {
            break;
        }
        str_s++;
        str_d++;
    }
    return 1;
}

// 获取字符串长度
sint_t rfs_strlen(char *str_s)
{
    sint_t chaidx = 0;
    while (*str_s != 0) {
        str_s++;
        chaidx++;
    }
    return chaidx;
}

// 字符复制
sint_t rfs_strcpy(char_t *str_s, char_t *str_d)
{
    sint_t chaidx = 0;
    while (*str_s != 0) {
        *str_d = *str_s;
        str_s++;
        str_d++;
        chaidx++;
    }
    *str_d = *str_s;
    return chaidx;
}

// rfs初始化
void init_rfs(device_t *devp)
{
    // 格式化rfs
    rfs_fmat(devp);
    return;
}

// 格式化rfs
void rfs_fmat(device_t *devp)
{
    // 建立超级块
    if (create_superblk(devp) == FALSE) {
        hal_sysdie("create superblk err");
    }

    // 建立位图
    if (create_bitmap(devp) == FALSE) {
        hal_sysdie("create bitmap err");
    }

    // 建立根目录
    if (create_rootdir(devp) == FALSE) {
        hal_sysdie("create rootdir err");
    }
    //test_rfs(devp);
    //test_dir(devp);
    //test_file(devp);
    return;
}

// 实际写入文件数据的函数
drvstus_t rfs_writefileblk(device_t *devp, fimgrhd_t *fmp, void *buf, uint_t len)
{
    // 检查文件的相关信息是否合理
    if (fmp->fmd_sfblk != fmp->fmd_curfwritebk || fmp->fmd_curfwritebk != fmp->fmd_fleblk[0].fb_blkstart) {
        return DFCERRSTUS;
    }

    // 检查当前将要写入数据的偏移量加上写入数据的长度，是否大于等于4KB
    if ((fmp->fmd_curfinwbkoff + len) >= FSYS_ALCBLKSZ) {
        return DFCERRSTUS;
    }

    // 指向将要写入数据的内存空间
    void *wrp = (void *)((uint_t)fmp + fmp->fmd_curfinwbkoff);
    // 把buf缓冲区中的数据复制len个字节到wrp指向的内存空间中去
    hal_memcpy(buf, wrp, len);
    // 增加文件大小
    fmp->fmd_filesz += len;
    // 使fmd_curfinwbkoff指向下一次将要写入数据的位置
    fmp->fmd_curfinwbkoff += len;
    // 把文件数据写入到相应的逻辑储存块中，完成数据同步
    write_rfsdevblk(devp, (void *)fmp, fmp->fmd_curfwritebk);
    return DFCOKSTUS;
}

// 实际读取文件数据的函数
drvstus_t rfs_readfileblk(device_t *devp, fimgrhd_t *fmp, void *buf, uint_t len)
{
    // 检查文件的相关信息是否合理
    if (fmp->fmd_sfblk != fmp->fmd_curfwritebk || fmp->fmd_curfwritebk != fmp->fmd_fleblk[0].fb_blkstart) {
        return DFCERRSTUS;
    }

    // 检查读取文件数据的长度是否大于（4096-512）
    if (len > (FSYS_ALCBLKSZ - fmp->fmd_fileifstbkoff)) {
        return DFCERRSTUS;
    }

    // 指向文件数据的开始地址
    void *wrp = (void *)((uint_t)fmp + fmp->fmd_fileifstbkoff);
    // 把文件开始处的数据复制len个字节到buf指向的缓冲区中
    hal_memcpy(wrp, buf, len);
    return DFCOKSTUS;
}

// 关闭文件的核心函数
drvstus_t rfs_closefileblk(device_t *devp, void *fblkp)
{
    // 指向文件的fimgrhd_t结构
    fimgrhd_t *fmp = (fimgrhd_t *)fblkp;
    // 完成文件数据的同步
    write_rfsdevblk(devp, fblkp, fmp->fmd_sfblk);
    // 释放缓冲区
    del_buf(fblkp, FSYS_ALCBLKSZ);
    return DFCOKSTUS;
}

// 打开文件的核心函数
void *rfs_openfileblk(device_t *devp, char_t *fname)
{
    char_t fne[DR_NM_MAX];
    void *rets = NULL, *buf = NULL;
    hal_memset((void *)fne, 0, DR_NM_MAX);
    // 从文件路径名中提取纯文件名
    if (rfs_ret_fname(fne, fname) != 0) {
        return NULL;
    }

    // 获取根目录文件
    void *rblkp = get_rootdirfile_blk(devp);
    if (rblkp == NULL) {
        return NULL;
    }

    fimgrhd_t *fmp = (fimgrhd_t *)rblkp;
    
    // 判断根目录文件的类型是否合理
    if (fmp->fmd_type != FMD_DIR_TYPE) {
        rets = NULL;
        goto err;
    }

    // 判断根目录文件里有没有数据
    if (fmp->fmd_curfwritebk == fmp->fmd_fleblk[0].fb_blkstart && fmp->fmd_curfinwbkoff == fmp->fmd_fileifstbkoff) {
        rets = NULL;
        goto err;
    }

    rfsdir_t *dirp = (rfsdir_t *)((uint_t)(fmp) + fmp->fmd_fileifstbkoff);
    void *maxchkp = (void *)((uint_t)rblkp + FSYS_ALCBLKSZ - 1);
    // 开始遍历文件对应的rfsdir_t结构
    for (; (void *)dirp < maxchkp;) {
        if (dirp->rdr_type == RDR_FIL_TYPE) {
            // 如果文件名相同就跳转到opfblk标号处运行
            if (rfs_strcmp(dirp->rdr_name, fne) == 1) {
                goto opfblk;
            }
        }
        dirp++;
    }

    // 如果到这里说明没有找到该文件对应的rfsdir_t结构，所以设置返回值为NULL
    rets = NULL;
    goto err;
opfblk:
    // 分配4KB大小的缓冲区
    buf = new_buf(FSYS_ALCBLKSZ);
    if (buf == NULL) {
        rets = NULL;
        goto err;
    }

    // 读取该文件占用的逻辑储存块
    if (read_rfsdevblk(devp, buf, dirp->rdr_blknr) == DFCERRSTUS) {
        rets = NULL;
        goto err1;
    }

    fimgrhd_t *ffmp = (fimgrhd_t *)buf;
    // 判断将要打开的文件是否合法
    if (ffmp->fmd_type == FMD_NUL_TYPE || ffmp->fmd_fileifstbkoff != 0x200) {
        rets = NULL;
        goto err1;
    }
    rets = buf;
    // 设置缓冲区首地址为返回值
    goto err;

err1:
    // 上面的步骤若出现问题就要释放缓冲区
    del_buf(buf, FSYS_ALCBLKSZ);
err:
    // 释放根目录文件
    del_rootdirfile_blk(devp, rblkp);
    return rets;
}

/**
 * 真正新建文件的函数
 *  1. 分配一个空闲的逻辑储存块，并在根目录文件的末尾写入这个新建文件对应的 rfsdir_t 结构
 *  2. 在一个新的 4KB 大小的缓冲区中，初始化新建文件对应的 fimgrhd_t 结构
 */
drvstus_t rfs_new_dirfileblk(device_t *devp, char_t *fname, uint_t flgtype, uint_t val)
{
    drvstus_t rets = DFCERRSTUS;
    if (flgtype != RDR_FIL_TYPE) {
        return DFCERRSTUS;
    }

    // 分配一个4KB大小的缓冲区
    void *buf = new_buf(FSYS_ALCBLKSZ);
    if (buf == NULL) {
        return DFCERRSTUS;
    }

    // 清零该缓冲区
    hal_memset(buf, 0, FSYS_ALCBLKSZ);
    // 分配一个新的空闲逻辑储存块
    uint_t fblk = rfs_new_blk(devp);
    if (fblk == 0) {
        rets = DFCERRSTUS;
        goto err1;
    }

    // 获取根目录文件
    void *rdirblk = get_rootdirfile_blk(devp);
    if (rdirblk == NULL) {
        rets = DFCERRSTUS;
        goto err1;
    }

    fimgrhd_t *fmp = (fimgrhd_t *)rdirblk;
    // 指向文件当前的写入地址，因为根目录文件已经被读取到内存中了
    rfsdir_t *wrdirp = (rfsdir_t *)((uint_t)rdirblk + fmp->fmd_curfinwbkoff);
    // 对文件当前的写入地址进行检查
    if (((uint_t)wrdirp) >= ((uint_t)rdirblk + FSYS_ALCBLKSZ)) {
        rets = DFCERRSTUS;
        goto err;
    }

    wrdirp->rdr_stus = 0;
    // 设为文件类型
    wrdirp->rdr_type = flgtype;
    // 设为刚刚分配的空闲逻辑储存块
    wrdirp->rdr_blknr = fblk;
    // 把文件名复制到rfsdir_t结构
    rfs_strcpy(fname, wrdirp->rdr_name);
    // 增加根目录文件的大小
    fmp->fmd_filesz += (uint_t)(sizeof(rfsdir_t));
    // 增加根目录文件当前的写入地址，保证下次不被覆盖
    fmp->fmd_curfinwbkoff += (uint_t)(sizeof(rfsdir_t));

    // 指向新分配的缓冲区
    fimgrhd_t *ffmp = (fimgrhd_t *)buf;
    // 调用fimgrhd_t结构默认的初始化函数
    fimgrhd_t_init(ffmp);
    // 因为建立的是文件，所以设为文件类型
    ffmp->fmd_type = FMD_FIL_TYPE;
    // 把自身所在的块，设为分配的逻辑储存块
    ffmp->fmd_sfblk = fblk;
    // 把当前写入的块，设为分配的逻辑储存块
    ffmp->fmd_curfwritebk = fblk;
    // 把当前写入块的写入偏移量设为512
    ffmp->fmd_curfinwbkoff = 0x200;
    // 把文件储存块数组的第1个元素的开始块，设为刚刚分配的空闲逻辑储存块
    ffmp->fmd_fleblk[0].fb_blkstart = fblk;
    // 因为只分配了一个逻辑储存块，所以设为1
    ffmp->fmd_fleblk[0].fb_blknr = 1;

    // 把缓冲区中的数据写入到刚刚分配的空闲逻辑储存块中
    if (write_rfsdevblk(devp, buf, fblk) == DFCERRSTUS) {
        rets = DFCERRSTUS;
        goto err;
    }
    rets = DFCOKSTUS;
err:
    // 释放根目录文件
    del_rootdirfile_blk(devp, rdirblk);
err1:
    // 释放缓冲区
    del_buf(buf, FSYS_ALCBLKSZ);
    return rets;
}

/**
 * 删除文件
 *  1. 从文件路径名中提取出纯文件名
 *  2. 获取根目录文件，从根目录文件中查找待删除文件的 rfsdir_t 结构，然后释放该文件占用的逻辑储存块
 *  3. 初始化与待删除文件相对应的 rfsdir_t 结构，并设置 rfsdir_t 结构的类型为 RDR_DEL_TYPE
 *  4. 释放根目录文件
 */
drvstus_t rfs_del_dirfileblk(device_t *devp, char_t *fname, uint_t flgtype, uint_t val)
{
    if (flgtype != RDR_FIL_TYPE || val != 0) {
        return DFCERRSTUS;
    }

    char_t fne[DR_NM_MAX];
    hal_memset((void *)fne, 0, DR_NM_MAX);
    // 提取纯文件名
    if (rfs_ret_fname(fne, fname) != 0) {
        return DFCERRSTUS;
    }

    // 调用删除文件的核心函数
    if (del_dirfileblk_core(devp, fne) != 0) {
        return DFCERRSTUS;
    }

    return DFCOKSTUS;
}

/**
 * 删除文件的核心函数
 *  1. 获取根目录文件，从根目录文件中查找待删除文件的 rfsdir_t 结构，然后释放该文件占用的逻辑储存块
 *  2. 始化与待删除文件相对应的 rfsdir_t 结构，并设置 rfsdir_t 结构的类型为 RDR_DEL_TYPE
 *  3. 释放根目录文件
 */
sint_t del_dirfileblk_core(device_t *devp, char_t *fname)
{
    sint_t rets = 6;
    // 获取根目录文件
    void *rblkp = get_rootdirfile_blk(devp);
    if (rblkp == NULL) {
        return 5;
    }

    fimgrhd_t *fmp = (fimgrhd_t *)rblkp;

    // 检查根目录文件的类型
    if (fmp->fmd_type != FMD_DIR_TYPE) {
        rets = 4;
        goto err;
    }

    if (fmp->fmd_curfwritebk == fmp->fmd_fleblk[0].fb_blkstart && fmp->fmd_curfinwbkoff == fmp->fmd_fileifstbkoff) {
        rets = 3;
        goto err;
    }

    rfsdir_t *dirp = (rfsdir_t *)((uint_t)(fmp) + fmp->fmd_fileifstbkoff);
    void *maxchkp = (void *)((uint_t)rblkp + FSYS_ALCBLKSZ - 1);
    // 检查其类型是否为文件类型
    for (; (void *)dirp < maxchkp;) {
        if (dirp->rdr_type == RDR_FIL_TYPE) {
            // 如果文件名相同，就执行以下删除动作
            if (rfs_strcmp(dirp->rdr_name, fname) == 1) {
                // 释放rfsdir_t结构的rdr_blknr中指向的逻辑储存块
                rfs_del_blk(devp, dirp->rdr_blknr);
                // 初始化rfsdir_t结构，实际上是清除其中的数据
                rfsdir_t_init(dirp);
                // 设置rfsdir_t结构的类型为删除类型，表示它已经删除
                dirp->rdr_type = RDR_DEL_TYPE;
                rets = 0;
                goto err;
            }
        }
        // 下一个rfsdir_t
        dirp++;
    }
    rets = 1;
err:
    // 释放根目录文件
    del_rootdirfile_blk(devp, rblkp);
    return rets;
}

/**
 * 获取根目录文件
 *  1. 返回根目录对应的逻辑块
 */
void *get_rootdirfile_blk(device_t *devp)
{
    void *retptr = NULL;
    // 获取根目录文件的rfsdir_t结构
    rfsdir_t *rtdir = get_rootdir(devp);
    if (rtdir == NULL) {
        return NULL;
    }
    // 分配4KB大小的缓冲区并清零
    void *buf = new_buf(FSYS_ALCBLKSZ);
    if (buf == NULL) {
        retptr = NULL;
        goto errl1;
    }

    hal_memset(buf, 0, FSYS_ALCBLKSZ);
    // 读取根目录文件的逻辑储存块到缓冲区中
    if (read_rfsdevblk(devp, buf, rtdir->rdr_blknr) == DFCERRSTUS) {
        retptr = NULL;
        goto errl;
    }
    // 设置缓冲区的首地址为返回值
    retptr = buf;
    goto errl1;
errl:
    del_buf(buf, FSYS_ALCBLKSZ);
errl1:
    // 释放根目录文件的rfsdir_t结构
    del_rootdir(devp, rtdir);

    return retptr;
}

/**
 * 释放根目录文件
 *  1. 释放根目录文件，就是把根目录文件的储存块回写到储存设备中去，最后释放对应的缓冲区
 */
void del_rootdirfile_blk(device_t *devp, void *blkp)
{
    // 因为逻辑储存块的头512字节的空间中，保存的就是fimgrhd_t结构
    fimgrhd_t *fmp = (fimgrhd_t *)blkp;
    // 把根目录文件回写到储存设备中去，块号为fimgrhd_t结构自身所在的块号
    if (write_rfsdevblk(devp, blkp, fmp->fmd_sfblk) == DFCERRSTUS) {
        hal_sysdie("del_rootfile_blk err");
    }
    // 释放缓冲区
    del_buf(blkp, FSYS_ALCBLKSZ);
    return;
}

/**
 * 读取根目录
 *  1. get_rootdir 函数的作用就是读取文件系统超级块中 rfsdir_t 结构到一个缓冲区中
 */
rfsdir_t *get_rootdir(device_t *devp)
{
    rfsdir_t *retptr = NULL;
    rfssublk_t *sbp = get_superblk(devp);
    if (sbp == NULL) {
        return NULL;
    }

    void *buf = new_buf(sizeof(rfsdir_t));
    if (buf == NULL) {
        retptr = NULL;
        goto errl;
    }

    hal_memcpy((void *)(&sbp->rsb_rootdir), buf, sizeof(rfsdir_t));
    retptr = (rfsdir_t *)buf;
errl:
    del_superblk(devp, sbp);
    return retptr;
}

/**
 * 删除根目录的内存
 *  1. del_rootdir 函数则是用来释放这个缓冲区
 */
void del_rootdir(device_t *devp, rfsdir_t *rdir)
{
    del_buf((void *)rdir, sizeof(rfsdir_t));
    return;
}

// 获取超级块
rfssublk_t *get_superblk(device_t *devp)
{
    // 分配4KB大小的缓冲区
    void *buf = new_buf(FSYS_ALCBLKSZ);
    if (buf == NULL) {
        return NULL;
    }

    // 清零缓冲区
    hal_memset(buf, 0, FSYS_ALCBLKSZ);
    // 读取第0个逻辑储存块中的数据到缓冲区中，如果读取失败则释放缓冲区
    if (read_rfsdevblk(devp, buf, 0) == DFCERRSTUS) {
        del_buf(buf, FSYS_ALCBLKSZ);
        return NULL;
    }
    // 返回超级块数据结构的地址，即缓冲区的首地址
    return (rfssublk_t *)buf;
}

// 释放超级块
void del_superblk(device_t *devp, rfssublk_t *sbp)
{
    // 回写超级块，因为超级块中的数据可能已经发生了改变，如果出错则死机
    // 释放先前分配的4KB大小的缓冲区
    if (write_rfsdevblk(devp, (void *)sbp, 0) == DFCERRSTUS){
        hal_sysdie("del superblk err");
    }
    del_buf((void *)sbp, FSYS_ALCBLKSZ);
    return;
}

// 获取位图块
u8_t *get_bitmapblk(device_t *devp)
{
    // 获取超级块
    rfssublk_t *sbp = get_superblk(devp);
    if (sbp == NULL) {
        return NULL;
    }

    // 分配4KB大小的缓冲区
    void *buf = new_buf(FSYS_ALCBLKSZ);
    if (buf == NULL) {
        return NULL;
    }

    // 缓冲区清零
    hal_memset(buf, 0, FSYS_ALCBLKSZ);
    // 读取sbp->rsb_bmpbks块（位图块），到缓冲区中
    if (read_rfsdevblk(devp, buf, sbp->rsb_bmpbks) == DFCERRSTUS) {
        del_buf(buf, FSYS_ALCBLKSZ);
        // 释放超级块
        del_superblk(devp, sbp);
        return NULL;
    }
    // 释放超级块
    del_superblk(devp, sbp);
    // 返回缓冲区的首地址
    return (u8_t *)buf;
}

// 释放位图块
void del_bitmapblk(device_t *devp, u8_t *bitmap)
{   
    // 获取超级块
    rfssublk_t *sbp = get_superblk(devp);
    if (sbp == NULL) {
        hal_sysdie("del superblk err");
        return;
    }

    // 回写位图块，因为位图块中的数据可能已经发生改变
    if (write_rfsdevblk(devp, (void *)bitmap, sbp->rsb_bmpbks) == DFCERRSTUS) {
        del_superblk(devp, sbp);
        hal_sysdie("del superblk err1");
    }
    // 释放超级块和存放位图块的缓冲区
    del_superblk(devp, sbp);
    del_buf((void *)bitmap, FSYS_ALCBLKSZ);
    return;
}

/**
 * 分配新的空闲逻辑储存块
 *  1. rfs_new_blk 函数会返回新分配的逻辑储存块号，如果没有空闲的逻辑储存块了，就会返回 0
 */
uint_t rfs_new_blk(device_t *devp)
{
    uint_t retblk = 0;
    // 获取位图块
    u8_t *bitmap = get_bitmapblk(devp);
    if (bitmap == NULL) {
        return 0;
    }

    for (uint_t blknr = 2; blknr < FSYS_ALCBLKSZ; blknr++) {
        // 找到一个为0的字节就置为1，并返回该字节对应的空闲块号
        if (bitmap[blknr] == 0) {
            bitmap[blknr] = 1;
            retblk = blknr;
            goto retl;
        }
    }
    // 如果到这里就说明没有空闲块了，所以返回0
    retblk = 0;
retl:
    // 释放位图块
    del_bitmapblk(devp, bitmap);
    return retblk;
}

void rfs_del_blk(device_t *devp, uint_t blknr)
{
    if (blknr > FSYS_ALCBLKSZ) {
        hal_sysdie("rfs del blk err");
        return;
    }

    u8_t *bitmap = get_bitmapblk(devp);
    if (bitmap == NULL) {
        hal_sysdie("rfs del blk err1");
        return;
    }

    bitmap[blknr] = 0;
    del_bitmapblk(devp, bitmap);
    return;
}

// 检查文件路径名
sint_t rfs_chkfilepath(char_t *fname)
{
    char_t *chp = fname;
    // 检查文件路径名的第一个字符是否为“/”，不是则返回2
    if (chp[0] != '/') {
        return 2;
    }

    for (uint_t i = 1;; i++) {
        // 检查除第1个字符外其它字符中还有没有为“/”的，有就返回3
        if (chp[i] == '/') {
            return 3;
        }

        // 如果这里i大于等于文件名称的最大长度，就返回4
        if (i >= DR_NM_MAX) {
            return 4;
        }

        // 到文件路径字符串的末尾就跳出循环
        if (chp[i] == 0 && i > 1) {
            break;
        }
    }
    // 返回0表示正确
    return 0;
}

// 提取纯文件名
sint_t rfs_ret_fname(char_t *buf, char_t *fpath)
{
    if (buf == NULL || fpath == NULL) {
        return 6;
    }

    // 检查文件路径名是不是“/xxxx”的形式
    sint_t stus = rfs_chkfilepath(fpath);
    // 如果不为0就直接返回这个状态值表示错误
    if (stus != 0) {
        return stus;
    }

    // 从路径名字符串的第2个字符开始复制字符到buf中
    rfs_strcpy(&fpath[1], buf);
    return 0;
}

/**
 * 判断文件是否存在
 *  1.首先是检查文件名的长度
 *  2. 接着获取了根目录文件
 *  3. 然后遍历根其中的所有 rfsdir_t 结构并比较文件名是否相同，相同就返回 1
 *  4. 不同就返回其它值，最后释放了根目录文件
 */
sint_t rfs_chkfileisindev(device_t *devp, char_t *fname)
{
    sint_t rets = 6;
    // 获取文件名的长度，注意不是文件路径名
    sint_t ch = rfs_strlen(fname);
    if (ch < 1 || ch >= (sint_t)DR_NM_MAX) {
        return 4;
    }

    void *rdblkp = get_rootdirfile_blk(devp);
    if (rdblkp == NULL) {
        return 2;
    }

    fimgrhd_t *fmp = (fimgrhd_t *)rdblkp;

    // 检查该fimgrhd_t结构的类型是不是FMD_DIR_TYPE，即这个文件是不是目录文件
    if (fmp->fmd_type != FMD_DIR_TYPE) {
        rets = 3;
        goto err;
    }

    // 检查根目录文件是不是为空，即没有写入任何数据，所以返回0，表示根目录下没有对应的文件
    if (fmp->fmd_curfwritebk == fmp->fmd_fleblk[0].fb_blkstart && fmp->fmd_curfinwbkoff == fmp->fmd_fileifstbkoff) {
        rets = 0;
        goto err;
    }

    rfsdir_t *dirp = (rfsdir_t *)((uint_t)(fmp) + fmp->fmd_fileifstbkoff);
    // 指向根目录文件的结束地址
    void *maxchkp = (void *)((uint_t)rdblkp + FSYS_ALCBLKSZ - 1);
    // 当前的rfsdir_t结构的指针比根目录文件的结束地址小，就继续循环
    for (; (void *)dirp < maxchkp;) {
        // 如果这个rfsdir_t结构的类型是RDR_FIL_TYPE，说明它对应的是文件而不是目录，所以下面就继续比较其文件名
        if (dirp->rdr_type == RDR_FIL_TYPE) {
            // 比较其文件名
            if (rfs_strcmp(dirp->rdr_name, fname) == 1) {
                rets = 1;
                goto err;
            }
        }
        dirp++;
    }
    // 到了这里说明没有找到相同的文件
    rets = 0;
err:
    // 释放根目录文件
    del_rootdirfile_blk(devp, rdblkp);
    return rets;
}

/**
 * 建立超级块
 *  1. 分配4KB大小的缓冲区，清零
 *  2. 使rfssublk_t结构的指针指向缓冲区并进行初始化
 *  3. 把缓冲区中超级块的数据写入到储存设备的第0个逻辑储存块中
 *  4. 释放缓冲区
 */
bool_t create_superblk(device_t *devp)
{
    // 分配4KB大小的缓冲区，清零
    void *buf = new_buf(FSYS_ALCBLKSZ);
    if (buf == NULL) {
        return FALSE;
    }
    hal_memset(buf, 0, FSYS_ALCBLKSZ);

    // 使rfssublk_t结构的指针指向缓冲区并进行初始化
    rfssublk_t *sbp = (rfssublk_t *)buf;
    rfssublk_t_init(sbp);
    // 获取储存设备的逻辑储存块数并保存到超级块中
    sbp->rsb_fsysallblk = ret_rfsdevmaxblknr(devp);
    // 把缓冲区中超级块的数据写入到储存设备的第0个逻辑储存块中
    if (write_rfsdevblk(devp, buf, 0) == DFCERRSTUS) {
        return FALSE;
    }
    // 释放缓冲区
    del_buf(buf, FSYS_ALCBLKSZ);
    return TRUE;
}

/**
 * 建立位图
 *  1. 利用一块储存空间中所有位的状态，达到映射逻辑储存块状态（是否已分配）的目的
 *  2. 第 0 块是超级块，第 1 块是位图块本身，所以代码从缓冲区中的第 3 个字节开始清零，一直到 devmaxblk 个字节
 *      devmaxblk 就是储存介质的逻辑储存块总数
 *  3. 这个缓冲区中的数据写入到储存介质中的第 bitmapblk 个逻辑储存块中，就完成了位图的建立
 */
bool_t create_bitmap(device_t *devp)
{
    bool_t rets = FALSE;
    // 获取超级块，失败则返回FALSE
    rfssublk_t *sbp = get_superblk(devp);
    if (sbp == NULL) {
        return FALSE;
    }

    // 分配4KB大小的缓冲区
    void *buf = new_buf(FSYS_ALCBLKSZ);
    if (buf == NULL) {
        return FALSE;
    }

    // 获取超级块中位图块的开始块号
    uint_t bitmapblk = sbp->rsb_bmpbks;
    // 获取超级块中储存介质的逻辑储存块总数
    uint_t devmaxblk = sbp->rsb_fsysallblk;
    // 如果逻辑储存块总数大于4096，就认为出错了
    if (devmaxblk > FSYS_ALCBLKSZ) {
        rets = FALSE;
        goto errlable;
    }

    // 把缓冲区中每个字节都置成1
    hal_memset(buf, 1, FSYS_ALCBLKSZ);
    u8_t *bitmap = (u8_t *)buf;
    // 把缓冲区中的第3个字节到第devmaxblk个字节都置成0
    // 0 代表逻辑存储块0(超级块)，1代表逻辑存储块0(根目录)
    for (uint_t bi = 2; bi < devmaxblk; bi++) {
        bitmap[bi] = 0;
    }

    // 把缓冲区中的数据写入到储存介质中的第bitmapblk个逻辑储存块中，即位图块中
    if (write_rfsdevblk(devp, buf, bitmapblk) == DFCERRSTUS) {
        rets = FALSE;
        goto errlable;
    }

    // 设置返回状态
    rets = TRUE;
errlable:
    // 释放超级块
    del_superblk(devp, sbp);
    // 释放缓冲区
    del_buf(buf, FSYS_ALCBLKSZ);
    return rets;
}

/**
 * 建立根目录
 *  1. 首先，分配一块新的逻辑储存块
 *  2. 接着，设置超级块中的 rfsdir_t 结构中的名称以及类型和块号
 *  3. 然后设置文件管理头，由于根目录是目录文件，所以文件管理头的类型为 FMD_DIR_TYPE，表示文件数据存放的是目录结构
 *  4. 最后，回写对应的逻辑储存块即可
 */
bool_t create_rootdir(device_t *devp)
{
    bool_t rets = FALSE;
    // 获取超级块
    rfssublk_t *sbp = get_superblk(devp);
    if (sbp == NULL) {
        return FALSE;
    }

    // 分配4KB大小的缓冲区
    void *buf = new_buf(FSYS_ALCBLKSZ);
    if (buf == NULL) {
        rets = FALSE;
        goto errlable1;
    }

    // 缓冲区清零
    hal_memset(buf, 0, FSYS_ALCBLKSZ);
    // 分配一个空闲的逻辑储存块
    uint_t blk = rfs_new_blk(devp);
    if (blk == 0) {
        rets = FALSE;
        goto errlable;
    }

    // 设置超级块中的rfsdir_t结构中的名称为“/”
    sbp->rsb_rootdir.rdr_name[0] = '/';
    // 设置超级块中的rfsdir_t结构中的类型为目录类型
    sbp->rsb_rootdir.rdr_type = RDR_DIR_TYPE;
    // 设置超级块中的rfsdir_t结构中的块号为新分配的空闲逻辑储存块的块号
    sbp->rsb_rootdir.rdr_blknr = blk;

    // 文件管理头
    fimgrhd_t *fmp = (fimgrhd_t *)buf;
    // 初始化fimgrhd_t结构
    fimgrhd_t_init(fmp);

    // 因为这是目录文件所以fimgrhd_t结构的类型设置为目录类型
    fmp->fmd_type = FMD_DIR_TYPE;
    // fimgrhd_t结构自身所在的块设置为新分配的空闲逻辑储存块
    fmp->fmd_sfblk = blk;
    // fimgrhd_t结构中正在写入的块设置为新分配的空闲逻辑储存块
    fmp->fmd_curfwritebk = blk;
    // fimgrhd_t结构中正在写入的块的偏移设置为512字节
    fmp->fmd_curfinwbkoff = 0x200;

    // 设置文件数据占有块数组的第0个元素
    fmp->fmd_fleblk[0].fb_blkstart = blk;
    fmp->fmd_fleblk[0].fb_blknr = 1;

    // 把缓冲区中的数据写入到新分配的空闲逻辑储存块中，其中包含已经设置好的 fimgrhd_t结构
    if (write_rfsdevblk(devp, buf, blk) == DFCERRSTUS) {
        rets = FALSE;
        goto errlable;
    }

    rets = TRUE;
errlable:
    // 释放缓冲区
    del_buf(buf, FSYS_ALCBLKSZ);
errlable1:
    // 释放超级块
    del_superblk(devp, sbp);

    return rets;
}

// 测试文件系统位图
void test_rfs_bitmap(device_t *devp)
{
    kprint("开始文件系统位图测试\n");
    void *buf = new_buf(FSYS_ALCBLKSZ);
    if (buf == NULL) {
        hal_sysdie("chkbitmap err");
    }

    hal_memset(buf, 0, FSYS_ALCBLKSZ);
    // 读取位图块
    if (read_rfsdevblk(devp, buf, 1) == DFCERRSTUS) {
        hal_sysdie("chkbitmap err1");
    }

    u8_t *bmp = (u8_t *)buf;
    uint_t b = 0;
    // 扫描位图块
    for (uint_t i = 0; i < FSYS_ALCBLKSZ; i++) {
        if (bmp[i] == 0) {
            b++;    // 记录空闲块
        }
    }

    kprint("文件系统空闲块数:%d\n", b);
    del_buf(buf, FSYS_ALCBLKSZ);
    kprint("结束文件系统位图测试\n");   // 死机用于观察测试结果
    die(0x400);
    return;
}

void test_allocblk(device_t *devp)
{
    uint_t ai[64];
    uint_t i = 0, j = 0;
    for (;;) {
        for (uint_t k = 0; k < 64; k++) {
            i = rfs_new_blk(devp);
            if (i == 0) {
                hal_sysdie("alloc blkk err");
            }

            kprint("alloc blk:%x\n", i);
            ai[k] = i;
            j++;
        }

        for (uint_t m = 0; m < 64; m++) {
            rfs_del_blk(devp, ai[m]);
            kprint("free blk:%x\n", ai[m]);
        }
    }
    kprint("alloc indx:%x\n", j);
    return;
}

// 测试文件系统根目录
void test_rfs_rootdir(device_t *devp)
{
    kprint("开始文件系统根目录测试\n");
    rfsdir_t *dr = get_rootdir(devp);
    void *buf = new_buf(FSYS_ALCBLKSZ);
    if (buf == NULL) {
        hal_sysdie("testdir err");
    }

    hal_memset(buf, 0, FSYS_ALCBLKSZ);
    if (read_rfsdevblk(devp, buf, dr->rdr_blknr) == DFCERRSTUS) {
        hal_sysdie("testdir1 err1");
    }

    fimgrhd_t *fmp = (fimgrhd_t *)buf;
    kprint("文件管理头类型:%d 文件数据大小:%d 文件在开始块中偏移:%d 文件在结束块中的偏移:%d\n",
            fmp->fmd_type, fmp->fmd_filesz, fmp->fmd_fileifstbkoff, fmp->fmd_fileiendbkoff);

    kprint("文件第一组开始块号:%d 块数:%d\n", fmp->fmd_fleblk[0].fb_blkstart, fmp->fmd_fleblk[0].fb_blknr);

    del_buf(buf, FSYS_ALCBLKSZ);
    del_rootdir(devp, dr);
    kprint("结束文件系统根目录测试\n"); // 死机用于观察测试结果
    die(0x400);
    return;
}

// 测试文件系统超级块
void test_rfs_superblk(device_t *devp)
{
    kprint("开始文件系统超级块测试\n");
    rfssublk_t *sbp = get_superblk(devp);
    kprint("文件系统标识:%d,版本:%d\n", sbp->rsb_mgic, sbp->rsb_vec);
    kprint("文件系统超级块块数:%d,逻辑储存块大小:%d\n", sbp->rsb_sblksz, sbp->rsb_dblksz);
    kprint("文件系统位图块号:%d,文件系统整个逻辑储存块数:%d\n", sbp->rsb_bmpbks, sbp->rsb_fsysallblk);
    kprint("文件系统根目录块号:%d 类型:%d\n", sbp->rsb_rootdir.rdr_blknr, sbp->rsb_rootdir.rdr_type);
    kprint("文件系统根目录名称:%s\n", sbp->rsb_rootdir.rdr_name);
    del_superblk(devp, sbp);
    kprint("结束文件系统超级块测试");   // 死机用于观察测试结果
    die(0x400);
    return;
}

void test_file(device_t *devp)
{
    //test_rfs(devp);
    //chk_rfsbitmap(devp);
    //test_dir(devp);
    test_fsys(devp);
    hal_sysdie("test file run");
    return;
}

void test_fsys(device_t *devp)
{
    // 分配缓冲区
    void *rwbuf = new_buf(FSYS_ALCBLKSZ);
    if (rwbuf == NULL) {
        hal_sysdie("rwbuf is NULL");
    }

    // 把缓冲区中的所有字节都置为0xff
    hal_memset(rwbuf, 0xff, FSYS_ALCBLKSZ);
    // 新建一个objnode_t结构
    objnode_t *ondp = krlnew_objnode();
    if (ondp == NULL) {
        hal_sysdie("ondp is NULL");
    }

    // 设置新建文件标志
    ondp->on_acsflgs = FSDEV_OPENFLG_NEWFILE;
    // 设置新建文件名
    ondp->on_fname = "/testfile";
    // 设置缓冲区
    ondp->on_buf = rwbuf;
    // 设置缓冲区大小
    ondp->on_bufsz = FSYS_ALCBLKSZ;
    // 设置读写多少字节
    ondp->on_len = 512;
    // 设置控制码
    ondp->on_ioctrd = FSDEV_IOCTRCD_DELFILE;

    // 新建文件
    if (rfs_open(devp, ondp) == DFCERRSTUS) {
        hal_sysdie("新建文件错误");
    }

    // 设置打开文件标志
    ondp->on_acsflgs = FSDEV_OPENFLG_OPEFILE;

    // 打开文件
    if (rfs_open(devp, ondp) == DFCERRSTUS) {
        hal_sysdie("打开文件错误");
    }

    // 把数据写入文件
    if (rfs_write(devp, ondp) == DFCERRSTUS) {
        hal_sysdie("写入文件错误");
    }

    // 清零缓冲区
    hal_memset(rwbuf, 0, FSYS_ALCBLKSZ);

    // 读取文件数据
    if (rfs_read(devp, ondp) == DFCERRSTUS) {
        hal_sysdie("读取文件错误");
    }

    // 关闭文件
    if (rfs_close(devp, ondp) == DFCERRSTUS) {
        hal_sysdie("关闭文件错误");
    }

    // 指向缓冲区
    u8_t *cb = (u8_t *)rwbuf;
    // 检查缓冲区空间中的头512个字节的数据，是否为0xff
    for (uint_t i = 0; i < 512; i++) {
        // 如果不等于0xff就死机
        if (cb[i] != 0xff) {
            hal_sysdie("检查文件内容错误");
        }
        // 打印文件内容
        kprint("testfile文件第[%x]个字节数据:%x\n", i, (uint_t)cb[i]);
    }

    // 删除文件
    if (rfs_ioctrl(devp, ondp) == DFCERRSTUS) {
        hal_sysdie("删除文件错误");
    }

    // 再次设置打开文件标志
    ondp->on_acsflgs = FSDEV_OPENFLG_OPEFILE;

    // 再次打开文件
    if (rfs_open(devp, ondp) == DFCERRSTUS) {
        kprint("再次打开文件失败");
    }

    kprint("结束文件操作测试");
    die(0x400);
    return;
}
