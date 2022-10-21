/**********************************************************
    cpu控制文件cpuctrl.c
***********************************************************/

#include "cosmostypes.h"
#include "cosmosmctrl.h"


void hal_wbinvd()
{
	__asm__ __volatile__("wbinvd": : :"memory");
}

void hal_invd()
{
	__asm__ __volatile__("invd": : :"memory");
}

void hal_sti_cpuflag(cpuflg_t* cpuflg)
{
    restore_flags_sti(cpuflg);
}

void hal_cli_cpuflag(cpuflg_t* cpuflg)
{
    save_flags_cli(cpuflg);
}

// CPU 暂存
void hal_cpuflag_sti(cpuflg_t* cpuflg)
{
    save_flags_sti(cpuflg);
}

// CPU 释放
void hal_cpuflag_cli(cpuflg_t* cpuflg)
{
    restore_flags_cli(cpuflg);
}

void hal_spinlock_init(spinlock_t *lock)
{
    lock->lock = 0;
    return;
}

/**
 * @brief 自旋锁加锁
 *  1. 首先读取锁变量，判断其值是否已经加锁，如果未加锁则执行加锁，然后返回，表示加锁成功
 *  2. 如果已经加锁了，就要返回第一步继续执行后续步骤，因而得名自旋锁
 * 
 * @other
 *  1. xchg，它可以让寄存器里的一个值跟内存空间中的一个值做交换
 *  2. 重新定义一个代码段所以jnz 2f下面并不是cmpl $0,%1
 *      1. 事实上下面的代码不会常常执行
 *      2. 这是为了不在cpu指令高速缓存中填充无用代码
 *      3. 要知道那可是用每位6颗晶体管做的双极性静态储存器,比内存条快几千个数量级
 * 
 * @param lock 
 */
void hal_spinlock_lock(spinlock_t *lock)
{
    __asm__ __volatile__ (
        "1:         \n"
        "lock; xchg  %0, %1 \n"     // 把值为1的寄存器和lock内存中的值进行交换
        "cmpl   $0, %0      \n"     // 用0和交换回来的值进行比较
        "jnz    2f      \n"         // 不等于0则跳转后面2标号处运行
        ".section .spinlock.text,"
        "\"ax\""
        "\n"                    
        "2:         \n"          
        "cmpl   $0, %1      \n"     // 用0和lock内存中的值进行比较
        "jne    2b      \n"         // 若不等于0则跳转到前面2标号处运行继续比较
        "jmp    1b      \n"         // 若等于0则跳转到前面1标号处运行，交换并加锁
        ".previous      \n"
        :
        : "r"(1), "m"(*lock)
    );

    return;
}

/**
 * @brief 自旋锁解锁
 *  1. 修改spinlock_t里的值，把值改成0
 * 
 * @param lock 
 */
void hal_spinlock_unlock(spinlock_t *lock)
{
    __asm__ __volatile__(
        "movl   $0, %0\n" // 解锁把lock内存中的值设为0就行
        :
        : "m"(*lock)
    );

    return;
}

/**
 * @brief 关中断自旋锁加锁
 * 
 * @param lock 自旋锁
 * @param cpuflg 
 */
void hal_spinlock_saveflg_cli(spinlock_t *lock, cpuflg_t *cpuflg)
{
    __asm__ __volatile__(
        "pushfq             \n\t"   // 把eflags寄存器压入当前栈顶
        "cli                \n\t"   // 关闭中断
        "popq %0            \n\t"   // 把当前栈顶弹出到cpuflg为地址的内存中

        "1:                 \n\t"
        "lock; xchg  %1, %2 \n\t"   // 把值为1的寄存器和lock内存中的值进行交换 
        "cmpl   $0,%1       \n\t"   // 用0和交换回来的值进行比较
        "jnz    2f          \n\t"   // 不等于0则跳转后面2标号处运行
        ".section .spinlock.text,"
        "\"ax\""
        "\n\t"                 
        "2:                 \n\t"   
        "cmpl   $0,%2       \n\t"   // 用0和lock内存中的值进行比较
        "jne    2b          \n\t"   // 若不等于0则跳转到前面2标号处运行继续比较
        "jmp    1b          \n\t"   // 若等于0则跳转到前面1标号处运行，交换并加锁
        ".previous          \n\t"
        : "=m"(*cpuflg)
        : "r"(1), "m"(*lock)
    );

    return;
}

/**
 * @brief 关中断自旋锁解锁
 * 
 * @param lock 
 * @param cpuflg 
 */
void hal_spinunlock_restflg_sti(spinlock_t *lock, cpuflg_t *cpuflg)
{
    __asm__ __volatile__(
        "movl   $0, %0\n\t" // 解锁把lock内存中的值设为0就行
        "pushq %1 \n\t"     // 把cpuflg为地址处的值寄存器压入当前栈顶 
        "popfq \n\t"        // 把当前栈顶弹出到eflags寄存器中
        :
        : "m"(*lock), "m"(*cpuflg)
    );

    return;
}


void knl_spinlock(spinlock_t * lock)
{
	hal_spinlock_lock(lock);
	return;
}

void knl_spinunlock(spinlock_t * lock)
{
	hal_spinlock_unlock(lock);
	return;
}

void knl_spinlock_init(spinlock_t *lock)
{
    lock->lock = 0;
    return;
}

/**
 * @brief 自旋锁加锁
 *  1. 首先读取锁变量，判断其值是否已经加锁，如果未加锁则执行加锁，然后返回，表示加锁成功
 *  2. 如果已经加锁了，就要返回第一步继续执行后续步骤，因而得名自旋锁
 * 
 * @other
 *  1. xchg，它可以让寄存器里的一个值跟内存空间中的一个值做交换
 *  2. 重新定义一个代码段所以jnz 2f下面并不是cmpl $0,%1
 *      1. 事实上下面的代码不会常常执行
 *      2. 这是为了不在cpu指令高速缓存中填充无用代码
 *      3. 要知道那可是用每位6颗晶体管做的双极性静态储存器,比内存条快几千个数量级
 * 
 * @param lock 
 */
void knl_spinlock_lock(spinlock_t *lock)
{
    __asm__ __volatile__(
        "1:         \n"
        "lock; xchg  %0, %1 \n"     // 把值为1的寄存器和lock内存中的值进行交换
        "cmpl   $0, %0      \n"     // 用0和交换回来的值进行比较
        "jnz    2f      \n"         // 不等于0则跳转后面2标号处运行
        ".section .spinlock.text,"
        "\"ax\""
        "\n"                    
        "2:         \n"             
        "cmpl   $0, %1      \n"     // 用0和lock内存中的值进行比较
        "jne    2b      \n"         // 若不等于0则跳转到前面2标号处运行继续比较
        "jmp    1b      \n"         // 若等于0则跳转到前面1标号处运行，交换并加锁
        ".previous      \n"
        :
        : "r"(1), "m"(*lock)
    );

    return;
}

/**
 * @brief 自旋锁解锁
 *  1. 修改spinlock_t里的值，把值改成0
 * 
 * @param lock 
 */
void knl_spinlock_unlock(spinlock_t *lock)
{
    __asm__ __volatile__(
        "movl   $0, %0\n"   // 解锁把lock内存中的值设为0就行
        :
        : "m"(*lock)
    );

    return;
}


/**
 * @brief 关中断自旋锁加锁
 * 
 * @param lock 自旋锁
 * @param cpuflg 
 */
void knl_spinlock_cli(spinlock_t *lock, cpuflg_t *cpuflg)
{
    __asm__ __volatile__(
        "pushfq             \n\t"   // 把eflags寄存器压入当前栈顶
        "cli                \n\t"   // 关闭中断
        "popq %0            \n\t"   // 把当前栈顶弹出到cpuflg为地址的内存中

        "1:                 \n\t"
        "lock; xchg  %1, %2 \n\t"   // 把值为1的寄存器和lock内存中的值进行交换 
        "cmpl   $0,%1       \n\t"   // 用0和交换回来的值进行比较
        "jnz    2f          \n\t"   // 不等于0则跳转后面2标号处运行
        ".section .spinlock.text,"
        "\"ax\""
        "\n\t"                 
        "2:                 \n\t"   
        "cmpl   $0,%2       \n\t"   // 用0和lock内存中的值进行比较
        "jne    2b          \n\t"   // 若不等于0则跳转到前面2标号处运行继续比较
        "jmp    1b          \n\t"   // 若等于0则跳转到前面1标号处运行，交换并加锁
        ".previous          \n\t"
        : "=m"(*cpuflg)
        : "r"(1), "m"(*lock)
    );

    return;
}

/**
 * @brief 关中断自旋锁解锁
 * 
 * @param lock 
 * @param cpuflg 
 */
void knl_spinunlock_sti(spinlock_t *lock, cpuflg_t *cpuflg)
{
    __asm__ __volatile__(
        "movl   $0, %0\n\t" // 解锁把lock内存中的值设为0就行
        "pushq %1 \n\t"     // 把cpuflg为地址处的值寄存器压入当前栈顶 
        "popfq \n\t"        // 把当前栈顶弹出到eflags寄存器中
        :
        : "m"(*lock), "m"(*cpuflg)
    );

    return;
}

/**
 * @brief 内存设置
 * 
 * @param setp 目标内存
 * @param setval 要写入的信息
 * @param n 信息的长度
 */
void hal_memset(void *setp, u8_t setval, size_t n)
{
    u8_t *_p = (u8_t *)setp;
    for (uint_t i = 0; i < n; i++) {
        _p[i] = setval;
    }

    return;
}

/**
 * @brief 内存拷贝
 * 
 * @param src 源内存
 * @param dst 目标内存
 * @param n 复制的内存长度
 */
void hal_memcpy(void *src, void *dst, size_t n)
{
    u8_t *_s = (u8_t *)src, *_d = (u8_t *)dst;
    for (uint_t i = 0; i < n; i++) {
        _d[i] = _s[i];
    }

    return;
}

/**
 * @brief hal层中断函数
 * 
 * @param errmsg 中断信息
 */
void hal_sysdie(char_t *errmsg)
{
    kprint("COSMOS SYSTEM IS DIE %s", errmsg);
    for (;;)
        ;

    return;
}

/**
 * @brief 系统错误函数
 * 
 * @param errmsg 系统错误信息
 */
void system_error(char_t *errmsg)
{
    hal_sysdie(errmsg);
    return;
}

/**
 * @brief 获取CPU id
 * 
 * @return uint_t 
 */
uint_t hal_retn_cpuid()
{
    return 0;
}