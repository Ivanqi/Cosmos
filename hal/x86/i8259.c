/**********************************************************
	i8259中断控制器源文件i8259.c
***********************************************************/

#include "cosmostypes.h"
#include "cosmosmctrl.h"

/**
 * @brief 8259中断的使用
 * 	8259A 中的 INTR 输入引脚是一个带有中断缓冲寄存器的输入引脚，它起到了 CPU 硬中断的触发作用
 * 	当 8259A 中断控制器检测到一个可处理中断时，会在 INTR 引脚上发送一个高电平信号，以通知 CPU 引发一次硬中断
 * 	通过这种方式，CPU 可以在快速响应外设中断的同时，减轻了处理器上轮询中断询问的负担
 * 
 * 	下面以实现键盘中断为例，说明如何使用 8259A 中的 INTR 引脚触发 CPU 硬中断
 * 		1. 在操作系统启动时，使用 outb 方法向控制器的特殊端口（Master: 0x20，Slave: 0xA0）
 * 			写入 0x11 来初始化 8259A 中断控制器
 * 
 * 		2. 向控制器特殊端口写入两个字节的中断向量表基址，用于存储中断服务程序的地址
 * 			例如，将主片的基址设置为 0x20，从片的基址设置为 0x28
 * 
 * 		3. 向主从片的控制端口（Master: 0x21，Slave: 0xA1）写入控制字，控制中断的开启和屏蔽，具体控制字格式参考 8259A 芯片手册
 * 			例如，对于键盘中断 IRQ1，可以将相关的 IRQ1 位设置为 1，将其他位设置为 0，表示只允许 IRQ1 中断
 * 
 * 		4. 程序进入循环，等待外设产生可处理中断
 * 			当键盘输入事件产生时，会向代表键盘中断的 IRQ1 引脚发送一个高电平信号，控制器会向 CPU 发送一个硬件中断请求信号
 * 			这个信号会通过 INTR 引脚输入到 CPU，引发硬中断处理程序的执行
 * 
 * 		5. 在中断服务程序中，程序可以读取键盘输入的数据并进行处理
 * 			处理结束后，必须写入 EOI（End Of Interrupt）指令到 控制器的相应端口
 * 			向控制器发送一个中断结束信号，以便管理其他挂起的中断
 */


/**
 * @brief  初始化8529芯片中断
 * 	1. ICW1和OCW2、OCW3是用偶地址端口0x20(主片)或0xA0(从片)写入
 * 	2. ICW2～ICW4和OCW1是用奇地址端口0x21(主片)或0xA1(从片)写入
 */
void init_i8259()
{
	// 初始化主从8259a
	out_u8_p(ZIOPT, ICW1);
	out_u8_p(SIOPT, ICW1);

	out_u8_p(ZIOPT1, ZICW2);
	out_u8_p(SIOPT1, SICW2);

	out_u8_p(ZIOPT1, ZICW3);
	out_u8_p(SIOPT1, SICW3);
	
	out_u8_p(ZIOPT1, ICW4);
	out_u8_p(SIOPT1, ICW4);

	// 屏蔽全部中断源
	out_u8_p(ZIOPT1, 0xff);
	out_u8_p(SIOPT1, 0xff);
	
	return;
}

/**
 * @brief 中断结束
 * 	1. 理结束后，必须写入 EOI（End Of Interrupt）指令到 控制器的相应端口
 * 	2. 向控制器发送一个中断结束信号，以便管理其他挂起的中断
 * 
 */
void i8259_send_eoi()
{
	out_u8_p(_INTM_CTL, _EOI);
	out_u8_p(_INTS_CTL, _EOI);
	return;
}

/**
 * @brief 此功能启用Intel 8259可编程中断控制器（PIC）上的特定中断线路
 * 
 * @param line 该参数是要启用的中断行的编号（从0开始）
 * 
 * 	1. 该功能首先通过保存当前标志并清除中断标志来禁用中断。这样做是为了防止在配置中断控制器时触发任何中断
 * 	2. 如果请求的中断线在前8条线（即IRQ0至IRQ7）内，则通过清除主中断屏蔽寄存器（_INTM_CTLMASK）中的相应位来启用该中断线
 * 	3. 如果请求的中断线超出前8条线（即IRQ8至IRQ15），则通过清除从中断屏蔽寄存器中的相应位（_INTS_CTLMASK）
 * 		和清除主中断屏蔽寄存器（_INTM_CTLMASK）中的第二位来启用
 * 
 * 启用中断后，函数将恢复以前的标志（将中断标志设置回其原始值）并返回
 */
void i8259_enabled_line(u32_t line)
{
	cpuflg_t flags;
	save_flags_cli(&flags);
	if (line < 8) {
		u8_t amtmp = in_u8(_INTM_CTLMASK);
		amtmp &= (u8_t)(~(1 << line));
		out_u8_p(_INTM_CTLMASK, amtmp);
	} else {
		u8_t astmp = in_u8(_INTM_CTLMASK);
		astmp &= (u8_t)(~(1 << 2));
		out_u8_p(_INTM_CTLMASK, astmp);
		astmp = in_u8(_INTS_CTLMASK);
		astmp &= (u8_t)(~(1 << (line - 8)));
		out_u8_p(_INTS_CTLMASK, astmp);
	}

	restore_flags_sti(&flags);

	return;
}

/**
 * @brief 此功能取消Intel 8259可编程中断控制器（PIC）上的特定中断线路
 * 
 * @param line 
 */
void i8259_disable_line(u32_t line)
{
	cpuflg_t flags;
	save_flags_cli(&flags);

	if (line < 8) {
		u8_t amtmp = in_u8(_INTM_CTLMASK);
		amtmp |= (u8_t)((1 << line));
		out_u8_p(_INTM_CTLMASK, amtmp);
	} else {
		u8_t astmp = in_u8(_INTM_CTLMASK);
		astmp |= (u8_t)((1 << 2));
		out_u8_p(_INTM_CTLMASK, astmp);
		astmp = in_u8(_INTS_CTLMASK);
		astmp |= (u8_t)((1 << (line - 8)));
		out_u8_p(_INTS_CTLMASK, astmp);
	}

	restore_flags_sti(&flags);
	return;
}

/**
 * @brief 保存一个中断控制器的状态，并禁用指定中断线
 * 
 * @param svline 需要保存的中断线状态的指针
 * @param line 表示需要禁用的中断线的编号
 */
void i8259_save_disableline(u64_t *svline, u32_t line)
{
	u32_t intftmp;
	cpuflg_t flags;
	// 保存 CPU 的标志寄存器，并将其清除，以关闭中断
	save_flags_cli(&flags);

	// 从 _INTM_CTLMASK 端口读取主片的控制寄存器的值，并将其存储到 altmp 变量中， 同时将其也存储到 intftmp 变量中
	// 这里使用的是 in_u8 函数，表示从 8 位输入端口读取数据
	u8_t altmp = in_u8(_INTM_CTLMASK);
	intftmp = altmp;
	
	// 从 _INTS_CTLMASK 端口读取从片的控制寄存器的值，并将其存储到 altmp 变量中
	// 然后将主片和从片的寄存器值拼接成一个 16 位的整数，并保存到 intftmp 变量中
	altmp = in_u8(_INTS_CTLMASK);
	intftmp = (intftmp << 8) | altmp;
	*svline = intftmp;
	
	// 通过指针传入的中断线编号，调用 i8259_disable_line 函数来禁用该中断线
	i8259_disable_line(line);

	// 将 CPU 的标志寄存器的值清除以重新开始
	restore_flags_sti(&flags);
	return;
}

/**
 * @brief 从保存的状态中恢复中断控制器的状态，并开启指定中断线
 * 
 * @param svline 表示保存了中断线状态的指针
 * @param line 需要恢复的中断线的编号
 */
void i8259_rest_enabledline(u64_t *svline, u32_t line)
{
	cpuflg_t flags;
	// 保存 CPU 的标志寄存器，并将其清除，以开启中断
	save_flags_cli(&flags);

	// 将指针传入的 svline 地址转换为 u32_t 类型的整数，并存储到 intftmp 变量中
	u32_t intftmp = (u32_t)(*svline);

	// 将整数 intftmp 中低 8 位的值取出，并存储到 altmp 变量中
	u8_t altmp = (intftmp & 0xff);
	// 使用 out_u8_p 函数，将 altmp 的值写入从片的 _INTS_CTLMASK 端口中
	out_u8_p(_INTS_CTLMASK, altmp);

	// 将整数 intftmp 右移 8 位，取出高 8 位的值，并将其存储到 altmp 变量中
	altmp = (u8_t)(intftmp >> 8);
	// 使用 out_u8_p 函数，将 altmp 的值写入主片的 _INTM_CTLMASK 端口中
	out_u8_p(_INTM_CTLMASK, altmp);

	// 使用 restore_flags_sti 函数恢复 CPU 标志寄存器的值，并开启中断
	restore_flags_sti(&flags);

	return;
}
