/**********************************************************
    信号量文件krlsem.c
***********************************************************/
#include "cosmostypes.h"
#include "cosmosmctrl.h"

/**
 * @brief 信号量使用例子
 *     sem_t sem;
 *     krlsem_t_init(&sem);
 *     // 加锁，减少信号量，如果信号量已经为 0
 *     // 则加锁失败，当前线程会改变为 sleeping 状态
 *     // 并让出 CPU 执行权 
 *     krlsem_down(&sem);
 *     // 处理一些数据同步、协同场景
 *     doing_something();
 *     // 解锁，增加信号量，唤醒等待队列中的其它线程（若存在）
 *     krlsem_up(&sem);
 * 
 * 
 * 信号量解决多CPU环境时自旋锁中其他得不到执行的进程一直在轮询的问题
 * 这个一直轮询会导致CPU无法切换到其他不需要执行该临界区的进程执行，效率低下
 * 所以引入能睡眠的机制，得不到的进程不让他们继续等了，先睡觉，负责其他进程执行的CPU去切换到别的进程执行
 */

/**
 * @brief 信号初始化
 * 
 * @param initp 信号结构体
 */
void krlsem_t_init(sem_t* initp)
{
    krlspinlock_init(&initp->sem_lock);
    initp->sem_flg = 0;
    initp->sem_count = 0;
    kwlst_t_init(&initp->sem_waitlst);
    return;
}

/**
 * @brief 信号设置
 * 
 * @param setsem 信号结构体
 * @param flg 信号标识
 * @param conut 信号次数
 */
void krlsem_set_sem(sem_t* setsem, uint_t flg, sint_t conut)
{
    cpuflg_t cpufg;
    krlspinlock_cli(&setsem->sem_lock, &cpufg);
    setsem->sem_flg = flg;
    setsem->sem_count = conut;
    krlspinunlock_sti(&setsem->sem_lock, &cpufg);
    return;
}    

/**
 * @brief 获取信号量
 *  1. 首先对用于保护信号量自身的自旋锁 sem_lock 进行加锁
 *  2. 对信号值 sem_count 执行“减 1”操作，并检查其值是否小于 0
 *  3. 上步中检查 sem_count 如果小于 0，就让进程进入等待状态并且将其挂入 sem_waitlst 中，然后调度其它进程运行
 *  4. 否则表示获取信号量成功。当然最后别忘了对自旋锁 sem_lock 进行解锁
 * 
 * @param sem 信号结构体
 */
void krlsem_down(sem_t* sem)
{
    cpuflg_t cpufg;
start_step:    
    krlspinlock_cli(&sem->sem_lock, &cpufg);
    if (sem->sem_count < 1) {
        // 如果信号量值小于1,则让代码执行流（线程）睡眠
        krlwlst_wait(&sem->sem_waitlst);
        krlspinunlock_sti(&sem->sem_lock, &cpufg);
        // 切换代码执行流，下次恢复执行时依然从下一行开始执行，所以要goto开始处重新获取信号量
        krlschedul();
        goto start_step; 
    }

    // 信号量值减1,表示成功获取信号量
    sem->sem_count--;
    krlspinunlock_sti(&sem->sem_lock, &cpufg);
    return;
}

/**
 * @brief 释放信号量
 *  1. 首先对用于保护信号量自身的自旋锁 sem_lock 进行加锁
 *  2. 对信号值 sem_count 执行“加 1”操作，并检查其值是否大于 0
 *  3. 上步中检查 sem_count 值如果大于 0，就执行唤醒 sem_waitlst 中进程的操作
 *  4. 并且需要调度进程时就执行进程调度操作，不管 sem_count 是否大于 0（通常会大于 0）都标记信号量释放成功
 *  5. 当然最后别忘了对自旋锁 sem_lock 进行解锁
 * 
 * @param sem 
 */
void krlsem_up(sem_t* sem)
{
    cpuflg_t cpufg;

    krlspinlock_cli(&sem->sem_lock, &cpufg);
    // 释放信号量
    sem->sem_count++;
    if (sem->sem_count < 1) {
        // 如果小于1,则说数据结构出错了，挂起系统
        krlspinunlock_sti(&sem->sem_lock, &cpufg);
        hal_sysdie("sem up err");
    }

    // 唤醒该信号量上所有等待的代码执行流（线程）
    krlwlst_allup(&sem->sem_waitlst);
    krlspinunlock_sti(&sem->sem_lock, &cpufg);
    
    krlsched_set_schedflgs();
    return;
}

