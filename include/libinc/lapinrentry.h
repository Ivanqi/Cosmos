#ifndef _LAPINRENTRY_H
#define _LAPINRENTRY_H

/*;Push(EAX);
;Push(ECX);
;Push(EDX);
;Push(EBX);
;Push(Temp);
;Push(EBP);
;Push(ESI);
;Push(EDI);
	push rbx
	push rcx
	push rdx
	push rbp
	push rsi
	push rdi
*/

/**
 * 传递一个参数所用的宏
 *  1. 系统服务号: movq %[inr],%%rax
 *  2. 第一个参数: movq %[prv1],%%rbx
 *  3. 触发中断: int $255
 *  4. 处理返回结果
 */
#define API_ENTRY_PARE1(intnr, rets, pval1) \
__asm__ __volatile__(\
    "movq %[inr], %%rax\n\t"\
    "movq %[prv1], %%rbx\n\t"\
    "int $255 \n\t"\
    "movq %%rax, %[retval] \n\t"\
    :[retval] "=r" (rets)\
    :[inr] "r" (intnr), [prv1]"r" (pval1)\
    :"rax", "rbx", "cc", "memory"\
)

#define API_ENTRY_PARE2(intnr, rets, pval1, pval2)\
__asm__ __volatile__(\
    "movq %[inr], %%rax \n\t"\
    "movq %[prv1], %%rbx \n\t"\
    "movq %[prv2], %%rcx \n\t"\
    "int $255 \n\t"\
    "movq %%rax, %[retval] \n\t"\
    :[retval] "=r" (rets)\
    :[inr] "r" (intnr),[prv1]"r" (pval1),\
    [prv2] "r" (pval2)\
    :"rax", "rbx", "rcx", "cc", "memory"\
)
    
    
#define API_ENTRY_PARE3(intnr, rets, pval1, pval2, pval3)\
__asm__ __volatile__(\
    "movq %[inr], %%rax \n\t"\
    "movq %[prv1], %%rbx \n\t"\
    "movq %[prv2], %%rcx \n\t"\
    "movq %[prv3], %%rdx \n\t"\
    "int $255 \n\t"\
    "movq  %%rax,%[retval] \n\t"\
    :[retval] "=r" (rets)\
    :[inr] "r" (intnr),[prv1]"g" (pval1),\
    [prv2] "g" (pval2),[prv3]"g" (pval3)\
    :"rax", "rbx", "rbx", "rcx", "rdx", "cc", "memory"\
)

/**
 * 传递四个参数所用的宏
 *  1. 系统服务号: movq %[inr], %%rax
 *  2. 第一个参数: movq %[prv1], %%rbx
 *  3. 第二个参数: movq %[prv2], %%rcx
 *  4. 第三个参数: movq %[prv3], %%rdx
 *  5. 第四个参数: movq %[prv4], %%rsi
 *  6. 触发中: int $255
 *  7. 处理返回结果: movq %%rax, %[retval]
 */
#define API_ENTRY_PARE4(intnr, rets, pval1, pval2, pval3, pval4)\
__asm__ __volatile__(\
    "movq %[inr], %%rax \n\t"\
    "movq %[prv1], %%rbx \n\t"\
    "movq %[prv2], %%rcx \n\t"\
    "movq %[prv3], %%rdx \n\t"\
    "movq %[prv4], %%rsi \n\t"\
    "int $255 \n\t"\
    "movq %%rax, %[retval] \n\t"\
    :[retval] "=r" (rets)\
    :[inr] "r" (intnr), [prv1]"g" (pval1),\
    [prv2] "g" (pval2), [prv3]"g" (pval3),\
    [prv4] "g" (pval4)\
    :"rax", "rbx", "rcx", "rdx", "rsi", "cc", "memory"\
)



#endif
