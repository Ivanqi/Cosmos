/**********************************************************
    内核对象节点头文件krlobjnode_t.h
***********************************************************/
#ifndef _KRLOBJNODE_T_H
#define _KRLOBJNODE_T_H

typedef struct s_OBJNODE {
    spinlock_t  on_lock;        // 自旋锁
    list_h_t    on_list;        // 链表
    sem_t       on_complesem;   // 完成信号量
    uint_t      on_flgs;        // 标志
    uint_t      on_stus;        // 状态
    sint_t      on_opercode;    // 操作码
    uint_t      on_objtype;     // 对象类型
    void*       on_objadr;      // 对象地址
    uint_t      on_acsflgs;     // 访问设备、文件标志
    uint_t      on_acsstus;     // 访问设备、文件状态
    uint_t      on_currops;     // 对应于读写数据的当前位置
    uint_t      on_len;         // 对应于读写数据的长度
    uint_t      on_ioctrd;      // IO控制码
    buf_t       on_buf;         // 对应于读写数据的缓冲区
    uint_t      on_bufcurops;   // 对应于读写数据的缓冲区的当前位置
    size_t      on_bufsz;       // 对应于读写数据的缓冲区的大小
    uint_t      on_count;       // 对应于对象节点的计数
    void*       on_safedsc;     // 对应于对象节点的安全描述符
    void*       on_fname;       // 对应于访问数据文件的名称
    void*        on_finode;     // 对应于访问数据文件的结点
    void*       on_extp;        // 用于扩展
} objnode_t;


#define OBJN_TY_DEV 1           // 设备类型
#define OBJN_TY_FIL 2           // 文件类型
#define OBJN_TY_NUL 0           // 默认类型

#endif // KRLOBJNODE_T_H
