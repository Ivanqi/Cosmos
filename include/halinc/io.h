/**********************************************************
	输入/输出头文件io.h
***********************************************************/

#ifndef _IO_H
#define _IO_H

#define ICW1 0x11
#define ZICW2 0x20
#define SICW2 0x28

#define ZICW3 0x04
#define SICW3 0x02

#define ICW4 0x01

#define ZIOPT 0x20
#define ZIOPT1 0x21

#define SIOPT 0xA0
#define SIOPT1 0xA1

#define _INTM_CTL 0x20      // I/O port for interrupt controller         <Master>
#define _INTM_CTLMASK 0x21  // setting bits in this port disables ints   <Master>
#define _INTS_CTL 0xA0      // ; I/O port for second interrupt controller  <Slave>
#define _INTS_CTLMASK 0xA1  // ; setting bits in this port disables ints   <Slave>
#define _EOI 0x20

#define PTIPROT1 0x40
#define PTIPROT2 0x41
#define PTIPROT3 0x42
#define PTIPROTM 0x43

#define TIMEMODE 0x34      // ;00-11-010-0
#define TIMEJISU 1194000UL // 1193182UL
#define HZ 1000UL          // 0x3e8

#define HZLL ((TIMEJISU / HZ) & 0xff)        // 0xa9//0x9b     //;1001  1011
#define HZHH (((TIMEJISU / HZ) >> 8) & 0xff) // 0x04//0x2e     //;0010  1110

#define NORETURN __attribute__((noreturn))
#define SYSRCALL __attribute__((regparm(3)))
#define HINTCALL __attribute__((regparm(2)))
#define FAUTCALL __attribute__((regparm(2)))

#define REGCALL __attribute__((regparm(3))) //让GCC使用(EAX, EDX, ECX)寄存器传递参数


#endif