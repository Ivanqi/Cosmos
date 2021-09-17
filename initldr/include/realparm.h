#ifndef REALPARM_H
#define REALPARM_H

#define ETYBAK_ADR 0x2000 // 特定存储空间

#define PM32_EIP_OFF (ETYBAK_ADR)
#define PM32_ESP_OFF (ETYBAK_ADR + 4)
#define RM16_EIP_OFF (ETYBAK_ADR + 8)
#define RM16_ESP_OFF (ETYBAK_ADR + 12)

#define RWHDPACK_ADR (ETYBAK_ADR + 32)

// e820map内存地址
#define E80MAP_NR (ETYBAK_ADR + 64)
#define E80MAP_ADRADR (ETYBAK_ADR + 68)
#define E80MAP_ADR (0x5000)     // edi设为存放输出结果的1MB内的物理内存地

#define VBEINFO_ADR (0x6000)    // 24576
#define VBEMINFO_ADR (0x6400)   // 24576

#define READHD_BUFADR 0x3000    // 12288

#endif