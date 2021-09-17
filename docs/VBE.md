# 概念
IBM的VGA标准是显示卡发展史上的一块丰碑

但后来无法满足人们的需要，于是市场上出现了TVGA、S3系列、Cirrus Logic、ET等为首的一批显示卡，提供了比VGA分辨率更高，颜色更丰富的显示模式，又兼容VGA显示卡，它们被统称为Super VGA（SVGA）

各种不同的SVGA之间的显示控制各不相同，带来软件兼容性问题，为此视频电子学标准协会VESA（Video Electronics Standards Association）提出了一组附加的BIOS功能调用借口——VBE（VESA BIOS EXTENSION）标准，从而在软件接口层次上实现了各种SVGA显示卡之间的兼容性

时至今日，所有的显示卡OEM厂商都提供了符合VESA SUPER标准的扩展BIOS

通过一组INT 10H中断调用（AH=4FH），可以方便地使用VESA SVGA的扩展功能而不必了解各种显示卡的硬件细节

Super VGA的扩充显示能力关键取决于对较大显示存储器的寻址能力

各Super VGA 卡提供的分辨率远高于VGA，VESA VBE均赋予一个标准的16位模式号（实际上是9位，其他各位为标志位或留给以后扩充）

# VBE功能调用和返回值
## VBE功能调用的共同点
1. AH必须等于4FH，表明VBE标准
2. AL等于VBE功能号，0 <= AL <= 0BH
3. BL等于子功能号, 也可以没有子功能
4. 调用INT 10H
5. 返回值

## VBE返回值一般在AX中
1. AL=4FH: 支持该功能
2. AL!=4FH: 不支持该功能
3. AH=00H: 调用成功
4. AH=01H: 调用失败
5. AH=02H: 当前硬件配置不支持该功能
6. AH=03H: 当前的显示模式不支持该功能

# VBE功能
```
-----------------------------------------------------------
				功能0x00：返回VBE信息
------------------------------------------------------
入口：
	AX			0x4F00
	ES：DI		指向VBE信息块的指针
出口：
	AX			VBE返回值
------------------------------------------------------------
 
-----------------------------------------------------------
			功能0x01：返回VBE特定模式信息
------------------------------------------------------
入口：
	AX			0x4F01
	CX			模式号
	ES：DI		指向VBE特定模式信息块的指针
出口：
	AX			VBE返回值
------------------------------------------------------------
 
-----------------------------------------------------------
			功能0x02：设置VESA VBE 模式
------------------------------------------------------
入口：
	AX			0x4F02
	BX			模式号
出口：
	AX			VBE返回值
------------------------------------------------------------
当设置模式失败时，返回错误代码，一般返回AH=01H
 
VESA 2.0以上增加了BX中D14，D15的位定义，完整定义如下：
BX = 模式号
	D0～D8：9位模式号
	D9～D13：保留，必须为0
	D14 = 0：使用普通的窗口页面缓存模式，用VBE功能05H切换显示页面
		= 1：使用大的线性缓存区，其地址可从VBE功能01H的返回信息ModeInfo获得
	D15 = 0：清除显示内存
		= 1：不清除显示内存
------------------------------------------------------------
```