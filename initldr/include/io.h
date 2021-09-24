/**********************************************************
	输入/输出头文件io.h
***********************************************************/

#ifndef _IO_H
#define _IO_H

// outb 向I/O端口写入一个字节(8位)（BYTE, HALF-WORD）
KLINE void out_u8(const u16_t port, const u8_t val)
{
    __asm__ __volatile__("outb %1, %0\n"
        :
        : "dN"(port), "a"(val)
    );
}

// inb 从I/O端口读取一个字节(8位)(BYTE, HALF-WORD) ;
KLINE u8_t in_u8(const u16_t port)
{
    u8_t tmp;
    __asm__ __volatile__("inb %1, %0\n"
        : "=a"(tmp)
        : "dN"(port)
    );

    return tmp;
}

// outw 向I/O端口写入一个字(16位)（WORD，即两个字节）
KLINE void out_u16(const u16_t port, const u16_t val)
{
    __asm__ __volatile__("outw %1, %0\n"
        : 
        : "dN"(port), "a"(val)
    );
}

// inw 从I/O端口读取一个字(16位)（WORD，即两个字节)
KLINE u16_t in_u16(const u16_t port)
{
    u16_t tmp;

    __asm__ __volatile__("inw %1, %0\n"
        : "=a"(tmp)
        : "dN"(port)
    );

    return tmp;
};

/**
 * 内存复制
 * 
 * 如果 sadr < dadr, 把sadr的内存复制到dadr
 * 如果 sadr > dadr, 把dadr的内存复制到sadr
 * 如果 sadr == sadr, 返回内存长度
 */
KLINE sint_t m2mcopy(void* sadr, void* dadr, sint_t len)
{
    if (NULL == sadr || NULL == dadr || 1 > len) {
        return 0;
    }

    u8_t *s = (u8_t *) sadr, *d = (u8_t *) dadr;

    if (s < d) {
        for (sint_t i = (len - 1); i >= 0; i--) {
            d[i] = s[i];
        }
        return len;
    }

    if (s > d) {
        for (sint_t j = 0; j < len; j++) {
            d[j] = s[j];
        }
        return len;
    }

    if (s == d) {
        return len;
    }

    return 0;
}

KLINE void memset(void *src, u8_t val, uint_t count)
{
    u8_t *ss = src;
    
    for (uint_t i = 0; i < count; i++) {
        ss[i] = val;
    }

    return ;
}

#endif