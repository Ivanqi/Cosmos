/****************************************************************
    Cosmos HAL全局数据结构头文件halglobal_t.h
*****************************************************************/
#ifndef _HALGLOBAL_T_H
#define _HALGLOBAL_T_H

/**
 * @brief 定义全局变量， EXTERN(extern) 告诉编译器这是外部文件的变量，避免发生错误
 *  1. 宏使用了 attribute((section(".data"))) ，这是 GCC 编译器的一个特性，表示将宏定义的变量放置到 .data 段中
 *  2. HAL_DEFGLOB_VARIABLE(int, myVar); // 定义一个 int 类型的全局变量 myVar
 *  3. 这将在代码中定义一个名为 myVar 的 int 类型的全局变量，并将其放置在数据段中。
 *  4. 宏定义中使用了 attribute((section(".data"))) ，这确保了 myVar 变量在数据段中的位置，而不是在代码段中或其他的存储空间中
 *  5. 在系统开发中，使用全局变量可以方便地在程序中共享数据
 *  6. 同时宏定义 HAL_DEFGLOB_VARIABLE 可以简化全局变量的定义过程，提高代码的可读性和可维护性
 * 
 */
#define HAL_DEFGLOB_VARIABLE(vartype, varname) \
EXTERN  __attribute__((section(".data"))) vartype varname

#endif // HALGLOBAL_T_H