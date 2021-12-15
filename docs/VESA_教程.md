# VESA 发展历程
VESA是制定 VBE 或 VESA BIOS 扩展标准的人。顾名思义，它是一个可选的 BIOS 扩展。但我认为可以肯定地说，自 1990 年代后期以来制造的 99% 的 PC 都支持 VESA

请注意，UEFI GOP 继承了 VESA BIOS 扩展，但 UEFI 实现还具有传统的 VESA 兼容 BIOS 以实现向后兼容性

1991年，VESA发布了VBE 1.2版本，该版本大部分与之前的1.0和1.1兼容，但更受欢迎。

在这里，VESA 定义了一些“视频模式编号”，它们与 VGA 模式编号大致相同（例如 0x03 是文本 80x25，0x13 是图形 320x200x8bpp，...）并且他们还定义了“库切换”

这个想法是这样工作的：你调用 VESA BIOS，它为你设置一个视频模式。然后，因为 1991 年仍然被认为是“DOS 时代”，所以您写入 0xA000:0x0000（线性 0xA0000）以在屏幕上绘制

但是那个缓冲区最多只有 64 KB，因为它是一个 16 位段……所以他们发明了库交换；其中 BIOS 将视频内存划分为更小的块，称为组，您将在绘图时根据需要切换组

例如，在 640x480x8bpp VESA 模式下，我们需要 300 KB 的显存。但是很遗憾，我们只能以64 KB的segment来访问显存……所以如果我们想绘制到整个屏幕，首先我们切换到bank 0，也就是屏幕的第一个64 KB，然后我们绘制。然后我们切换到bank 1，也就是屏幕的第二个64 KB，然后我们画到同一个地址

然后我们切换到 bank 2，依此类推...这会导致严重的性能损失，并且今天不推荐使用 bank 切换，而本教程后面将讨论的线性帧缓冲区已经成功。

# VBE 1.x 定义
```
MODE    RESOLUTION  BITS PER PIXEL  MAXIMUM COLORS
0x0100  640x400     8               256
0x0101  640x480     8               256
0x0102  800x600     4               16
0x0103  800x600     8               256
0x010D  320x200     15              32k
0x010E  320x200     16              64k
0x010F  320x200     24/32*          16m
0x0110  640x480     15              32k
0x0111  640x480     16              64k
0x0112  640x480     24/32*          16m
0x0113  800x600     15              32k
0x0114  800x600     16              64k
0x0115  800x600     24/32*          16m
0x0116  1024x768    15              32k
0x0117  1024x768    16              64k
0x0118  1024x768    24/32*          16m
```
## 注意点
- 有些BIOS支持24位颜色，其中每种颜色为一个RGB值，其中每种颜色成分为8位；总共提供 1600 万种可用颜色
- 其他 BIOS 支持 32 位颜色，其中颜色也是一个 RGB 值，每个颜色分量仍然是 8 位，但顶部有一个空的 8 位，称为 alpha 通道
- 所以 32 位颜色通常称为 RGBA 颜色，而 24 位颜色通常称为 RGB 颜色
- 这样做有两个原因：作为实现 alpha 混合的软件的存根，以及通过 32 位对齐来加速内存操作
- 请注意，这些值是小端的

# VBE2.0
然后在 1994 年，VESA 定义了 VBE 2.0，这是一个重大改进，尽管“大多数”VBE 2.0+ BIOS 与 VBE 1.x 兼容

在 VBE 2.0 中，VESA 定义了“线性帧缓冲区”，它是高内存（3 到 4 GB）中的一个位置，具有完全连续的帧缓冲区；虽然 BIOS 仍支持向后兼容，但库(bank)切换已被弃用

VESA 还表示，他们在 VBE 1.x 中定义的所有模式也已弃用，硬件制造商不需要支持它们，任何人都可以组成他们喜欢的任何模式

大多数硬件供应商仍然支持标准的 VBE 1.x 模式以实现向后兼容，但您永远不要依赖它，因为有一天您会发现无法运行你的代码的PC

但是等一下......我们如何在不知道模式编号的情况下设置 VESA 模式？VESA 给了我们两件事：一个函数返回所有可用模式编号的数组，另一个函数获取指定模式编号的详细信息（宽度、高度、bpp、线性帧缓冲区地址等...）

基本思想是我们查询BIOS中每一个可用模式的信息，当我们找到适合我们需要的模式时，我们就可以使用它

由于模式号不是标准的，你永远不应该假设模式号、模式的宽度、模式的高度或模式的 bpp。

例如，我的笔记本电脑的 VESA 模式 0x0118 为 1024x768x32，而较旧的软件假定它是 24 位模式。较新的软件可能会假定它是 32 位模式。而 Bochs 和 QEMU 将其模拟为 24 位模式。无论如何，您永远不应该采用 VESA 模式，并且应该始终查询 BIOS 以了解它支持的内容。

我想今天的理论就足够了。让我们来看看如何实际使用 VESA BIOS 扩展！VESA 将其 VBE 函数置于 BIOS 中断 0x10 的函数 0x4F。

你把函数号0x4F放在AH寄存器，子函数号放在AL寄存器，参数放在其他寄存器，调用INT 0x10。成功时，所有 VESA 调用都会在 AX 中返回 0x004F。任何其他返回码都应视为错误。

# VBE 2.0+ 的有用函数
## 功能：获取 VESA BIOS 信息
### 功能码
0x4F00

### 描述
返回 VESA BIOS 信息，包括制造商、支持的模式、可用的视频内存等...输入：AX = 0x4F00

### 输入
ES:DI = 段:偏移,指向存储 VESA BIOS 信息结构的位置

### 输出
AX = 0x004F 成功，其他值表示不支持 VESA BIOS

无论如何，上述函数返回以下结构并将其存储在 ES:DI 中，就像它们在入口处一样。进入时，ES:DI 应该包含一个指向以下结构的指针：
```
vbe_info_structure:
	.signature		db "VBE2"	; 表示支持 VBE 2.0+
	.table_data:		resb 512-4	; 为下表预留空间
```

在 BIOS 调用之后，如果它成功（AX 是 0x004F），那么上面相同的结构现在包含以下内容：
```
struct vbe_info_structure {
	char[4] signature = "VESA";	// 必须是“VESA”以指示有效的 VBE 支持
	uint16 version;			// VBE版本；高字节是主要版本，低字节是次要版本
	uint32 oem;			// OEM的段:偏移地址
	uint32 capabilities;		// 描述卡功能的位域
	uint32 video_modes;		// segment:offset 指向支持的视频模式列表的指针
	uint16 video_memory;		// 以 64KB 块为单位的视频内存量
	uint16 software_rev;		// 软件版本
	uint32 vendor;			// 段:偏移到卡供应商字符串
	uint32 product_name;		// 段:偏移到卡型号名称
	uint32 product_rev;		// 指向产品版本的段:偏移指针
	char reserved[222];		// 为未来扩展预留
	char oem_data[256];		// OEM BIOS 将其字符串存储在此区域
} __attribute__ ((packed));
```
注意所有segment:offset字段都是little-endian，这意味着低字是偏移量，高字是段

从上述结构中您可能感兴趣的事情
- “signature”将从“VBE2”更改为“VESA”
    - 条目上必须是“VBE2”，以表明对 VBE 2.0 的软件支持。如果它包含“VBE2”，BIOS 将返回 VBE 2.0+ 的 512 字节数据
    - 如果它包含“VESA”，则 BIOS 将为 VBE 1.x 返回 256 字节的数据。
    - 如果调用后不是“VESA”，则应假设VESA BIOS Extensions不可用
- “version”告诉你VBE的版本
    - 0x100 为 1.0，0x101 为 1.1，0x102 为 1.2，0x200 为 2.0，0x300 为 3.0（最新版本）
    - VBE 1.x 在上述结构中返回 256 字节的数据，如果“签名”字段在条目上包含“VBE2”，则 VBE 2.0 和 3.0 返回 512 字节的数据
- “video_modes”是一个segment:offset指针，指向支持的视频模式列表
    - 数组中的每个条目都是一个 16 位字，并以 0xFFFF 结尾。如果在搜索您的模式时发现 0xFFFF，则该模式不受支持。
    - “video_memory”包含 PC 在 64 KB 块中具有多少 VGA RAM
    - 因此，要以 KB 为单位，请将“video_memory”中的值乘以 64

无论如何，关于支持的模式数组，如果 PC 支持模式 0x0103、0x0115 和 0x0118，则该数组在十六进制转储中看起来像这样
```
03 01 15 01 18 01 FF FF
```

## 功能：获取VESA模式信息
### 功能码
0x4F01

### 描述
此函数返回指定模式的模式信息结构。模式编号应从支持的模式数组中获取。

### 输入
AX = 0x4F01

CX = 视频模式数组中的 VESA 模式编号

ES:DI = Segment:Offset 存储 VESA 模式信息结构的位置的指针，如下所示

### 输出
AX = 0x004F 成功，其他值表示 BIOS 错误或模式不支持的错误

下面是这个函数在 ES:DI 中返回的结构：
```
struct vbe_mode_info_structure {
	uint16 attributes;		// 已弃用，您应该只对第 7 位感兴趣，它表示该模式支持线性帧缓冲区
	uint8 window_a;			// 已弃用
	uint8 window_b;			// 已弃用
	uint16 granularity;		// 已弃用; 在计算bank号码时使用
	uint16 window_size;
	uint16 segment_a;
	uint16 segment_b;
	uint32 win_func_ptr;		// 已弃用; 用于在不返回实模式的情况下从保护模式切换组
	uint16 pitch;			// 每条水平线的字节数
	uint16 width;			// 像素宽度
	uint16 height;			// 像素高度
	uint8 w_char;			// 没用过...
	uint8 y_char;			// ...
	uint8 planes;
	uint8 bpp;			// 在这种模式下每像素位数
	uint8 banks;			// 已弃用; 此模式下的bank总数
	uint8 memory_model;
	uint8 bank_size;		// 已弃用; bank的大小，几乎总是 64 KB，但可能是 16 KB

	uint8 image_pages;
	uint8 reserved0;
 
	uint8 red_mask;
	uint8 red_position;
	uint8 green_mask;
	uint8 green_position;
	uint8 blue_mask;
	uint8 blue_position;
	uint8 reserved_mask;
	uint8 reserved_position;
	uint8 direct_color_attributes;
 
	uint32 framebuffer;		// 线性帧缓冲区的物理地址；在此处写入以绘制到屏幕
	uint32 off_screen_mem_off;
	uint16 off_screen_mem_size;	// 帧缓冲区中的内存大小但未显示在屏幕上
	uint8 reserved1[206];
} __attribute__ ((packed));
```

唯一感兴趣的是：“attributes”位 7（值 0x80）表示该模式支持线性帧缓冲区

"width", "height" 是 "bpp" 在搜索我们要使用的模式时使用

“framebuffer”是指向线性帧缓冲区的 32 位物理指针

设置 VBE 模式时必须启用线性帧缓冲区，这将在下一个函数中讨论。如果您使用分页，请确保将帧缓冲区映射到虚拟地址空间中已知的某个位置！

## 功能：设置 VBE 模式
### 功能码
0x4F02

### 描述
此函数设置 VBE 模式

### 输入
AX = 0x4F02

BX = 位 0-13 模式号；位 14 是 LFB 位：设置时，它启用线性帧缓冲区，清除时，软件必须使用组切换。位 15 是 DM 位：设置后，BIOS 不会清除屏幕。位 15 通常被忽略，应始终清除。

ES:DI = Segment:Offset 存储 VESA 模式信息结构的位置的指针，如下所示

### 输出
AX = 0x004F 成功，其他值表示错误。例如 BIOS 错误、显存太少、不支持 VBE 模式、模式不支持线性帧缓冲区或任何其他错误


所以这意味着，如果 VBE 模式 0x0118 是 1024x768x32bpp，并且我们想设置此模式并要求 BIOS 为我们清除屏幕，我们可以这样做
```
mov ax, 0x4F02	; 设置 VBE 模式
mov bx, 0x4118	; VBE 模式编号；请注意，位 0-13 包含模式编号，位 14 (LFB) 已设置，位 15 (DM) 已清除
int 0x10		; 调用 VBE BIOS
cmp ax, 0x004F	; 测试错误
jne error
 
after:
	; ...
```

无论如何，就像我提到的至少一百次一样，您应该首先从视频模式数组中获取模式编号。这些模式编号只有普通模式编号（0x0118、0x0103 等...），您应该在设置 VBE 模式时设置位 14。

## 功能：获取当前 VBE 模式
### 功能码
0x4F03

### 描述
该函数返回当前的 VBE 模式

### 输入
AX = 0x4F03

### 输出
BX = 位 0-13 模式号；位 14 是 LFB 位：设置时，系统使用线性帧缓冲区，清除时，系统使用组切换。第 15 位是 DM 位：清除时，当 VBE 模式设置时，视频内存被清除。

## 功能：显示窗口控件（已弃用）
### 功能码
0x4F05

### 描述
设置/获取当前bank（已弃用）

### 输入
AX = 0x4F05
BH = 0x00 设置bank，0x01 获取bank
DX = 以窗口粒度为单位的库号

### 输出
AX = 0x004F 成功，其他值表示错误；例如 BIOS 错误、无法使用的银行、不存在的银行、未使用银行模式

由于bank转换已被弃用，我不会讨论如何计算bank编号。

## 功能：返回保护模式接口
### 功能码
0x4F0A

### 描述
返回 VBE 2.0 保护模式接口

### 输入
AX = 0x4F0A
BL = 0x00

### 输出
AX = 0x004F 成功，其他值表示错误；例如 BIOS 错误、不支持保护模式接口或 VBE 版本低于 2.0
ES:DI = Segment:Offset 指向保护模式表的指针
CX = 表的长度，包括用于复制目的的代码（以字节为单位）

此功能允许软件从保护模式切换组（功能 0x4F05）、设置显示开始（功能 0x4F07）和设置主调色板数据（功能 0x4F09），而无需返回实模式。

这在一定程度上提高了性能，但总体上已被弃用，并且在某些 BIOS 中可能不受支持

反正 ES:DI 返回的表是这样的
```
struct vbe2_pmi_table {
	uint16 set_window;		// 功能 0x4F05 的保护模式代码表中的偏移量
	uint16 set_display_start;	// 功能 0x4F07 的保护模式代码表中的偏移量
	uint16 set_pallette;		// 功能 0x4F09 的保护模式代码表中的偏移量
} __attribute__ ((packed));

```

# 参考资料
- [User:Omarrx024/VESA Tutorial](https://wiki.osdev.org/User:Omarrx024/VESA_Tutorial)
- [VESA Video Modes](https://wiki.osdev.org/VBE)