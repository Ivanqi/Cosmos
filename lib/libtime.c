#include "libc.h"
#include "libdev.h"

/**
 * @brief 时间API
 * 
 * @param ttime 
 * @return sysstus_t 
 */
sysstus_t time(times_t *ttime)
{
    sysstus_t rets = api_time(ttime);
    return rets;
}

/**
 * @brief 时间设置
 * 
 * @param ttime 
 * @return sysstus_t 
 */
sysstus_t settime(times_t *ttime)
{

    if (ttime == NULL) {
        return SYSSTUSERR;
    }

    devid_t dev;
    dev.dev_mtype = RTC_DEVICE;
    dev.dev_stype = 0;
    dev.dev_nr = 0;
    hand_t fd = open(&dev, RW_FLG | FILE_TY_DEV, 0);
    if (fd == -1) {
        return SYSSTUSERR;
    }

    ioctrl(fd, ttime, IOCTRCODE_SETTIME, 0);

    close(fd);

    return SYSSTUSOK;
}

/**
 * @brief 时间获取
 * 
 * @param ttime 
 * @return sysstus_t 
 */
sysstus_t gettime(times_t *ttime)
{
    if (ttime == NULL) {
        return SYSSTUSERR;
    }

    devid_t dev;
    dev.dev_mtype = RTC_DEVICE;
    dev.dev_stype = 0;
    dev.dev_nr = 0;
    hand_t fd = open(&dev, RW_FLG | FILE_TY_DEV, 0);
    if (fd == -1) {
        return SYSSTUSERR;
    }

    read(fd, ttime, sizeof(times_t), 0);

    close(fd);

    return SYSSTUSOK;
}

sysstus_t synsecalarm(uint_t sec)
{
    uint_t tmpsec = sec;
    if (tmpsec >= 60) {
        return SYSSTUSERR;
    }

    devid_t dev;
    dev.dev_mtype = RTC_DEVICE;
    dev.dev_stype = 0;
    dev.dev_nr = 0;
    hand_t fd = open(&dev, RW_FLG | FILE_TY_DEV, 0);
    if (fd == -1) {
        return SYSSTUSERR;
    }

    ioctrl(fd, &tmpsec, IOCTRCODE_SET_SYNSECALM, 0);

    close(fd);
    return SYSSTUSOK;
}

long tick(uint_t id)
{
    return (long)api_tick(id);
}