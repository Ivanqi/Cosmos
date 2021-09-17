# 位测试和设置指令
BT DEST, SRC

BTC DEST, SRC

BTR DEST, SRC

BTS DESt, SRC

# 功能
按照源操作指定的位号，测试目的操作数，当指令执行时，被测试的状态被复制到进位标志CF

BT将SRC指定的DEST中一位的数值复制到CF

BTC将SRC指定的DEST中一位的数值复制到CF，且将DEST中该位取反

BTR将SRC指定的DEST中一位的数值复制到CF，且将DEST中该位复位

BTS将SRC指定的DEST中一位数值复制到CF，且将DEST中该位置位

目的操作数为16位或32位通用寄存器或存储器，源操作数为16位或32位通用寄存器，以及8位立即数，当源操作数为通用寄存器时，必须同目的操作数类型一致。

源操作数SRC以两种方式给出目的操作数的位号，即
- `SRC为8位立即数，以二进制形式直接给出要操作的位号`
- `SRC为通用寄存器，如果DEST为通用寄存器，则SRC中二进制直接给出要操作的位号`
    - 如果DES为存储器操作数，通用寄存器SRC为带符号整数，SRC的值除以DEST的长度所得到的商作为DEST的相对偏移量，余数直接作为要操作的位号
    - DEST的有效地址为DEST给出偏移地址和DEST相对偏移量之和
    
BT, BTC, BTR, BTS 指令影响CF标志位，其他标志位无定义


# 例子
```
MOV AX，1234H
MOV ECX，5
BT AX，CX　　　　　　　；CF=1AX=1234H
BTC AX，5　　　　　　　；CF=1；AX=1214H
BTS AX，CX；　　　　　 ；CF=0AX=1234H
BTR EAX，ECX　　　　　 ；CF=1EAX=00001214H

 AT&T  格式 

movl  $0x1234 ,&ecx   // 0001 0010 0011 0100B 

bt  $0x03,%ecx   //  第一位是0  ， 0x03 是第四位 

jnc  somewhere   //CF 位是0 则转移 
```

