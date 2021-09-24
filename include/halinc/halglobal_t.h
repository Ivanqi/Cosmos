/****************************************************************
    Cosmos HAL全局数据结构头文件halglobal_t.h
*****************************************************************/
#ifndef _HALGLOBAL_T_H
#define _HALGLOBAL_T_H

// 定义全局变量， EXTERN(extern) 告诉编译器这是外部文件的变量，避免发生错误
#define HAL_DEFGLOB_VARIABLE(vartype, varname) \
EXTERN  __attribute__((section(".data"))) vartype varname

#endif // HALGLOBAL_T_H