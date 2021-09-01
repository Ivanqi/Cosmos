; GRUB头汇编部分
; 它主要工作是初始化 CPU 的寄存器，加载 GDT，切换到 CPU 的保护模式

; EQU 伪指令把一个符号名称与一个整数表达式或一个任意文本连接起来
MBT_HDR_FLAGS	EQU 0x00010003	; 65539
MBT_HDR_MAGIC	EQU 0x1BADB002	; 多引导协议头魔数, 464367618
MBT2_MAGIC	EQU 0xe85250d6		; 第二版多引导协议头魔数

global _start					; 导出_start符号
extern inithead_entry			; 导入外部的inithead_entry函数符号
[section .text]					; 定义.text代码节
[bits 32]						; 汇编成32位代码
_start:
	jmp _entry
align 4

; dd/DD （汇编语言中的伪操作命令）
; DD作为汇编语言中的伪操作命令，它用来定义操作数占用的字节数(DoubleWord的缩写)，即4个字节（32位）
; DW定义字类型变量，一个字数据占2个字节单元，读完一个，偏移量加2

; 用汇编定义的GRUB的多引导协议头，之所以有两个引导头，是为了兼容GRUB1和GRUB2
; GRUB所需要的头, 定义字节数据
mbt_hdr:
	dd MBT_HDR_MAGIC
	dd MBT_HDR_FLAGS
	dd -(MBT_HDR_MAGIC + MBT_HDR_FLAGS)
	dd mbt_hdr
	dd _start
	dd 0
	dd 0
	dd _entry
	;
	; multiboot header
	;
ALIGN 8

;  GRUB2所需要的头
; 包含两个头是为了同时兼容GRUB、GRUB2
mbhdr:
	DD	0xE85250D6				; 3897708758
	DD	0
	DD	mhdrend - mbhdr
	DD	-(0xE85250D6 + 0 + (mhdrend - mbhdr))
	DW	2, 0
	DD	24
	DD	mbhdr
	DD	_start
	DD	0
	DD	0
	DW	3, 0
	DD	12
	DD	_entry 
	DD      0  
	DW	0, 0
	DD	8
mhdrend:

; 关掉中断(无打扰状态)，设定CPU的工作模式
; 关中断并加载 GDT
; 读端口用IN指令，写端口用OUT指令
_entry:
	; 关中断
	cli

	; 读取CMOS/RTC地址，设置最高位为1，写入0x70端口，关闭不可屏蔽中断
	; 	CMOS RAM的访问，需要通过两个端口进行.0x70或者0x74是索引端口，用来指定CMOS RAM内的单元
	; 	端口0x70的最高位(bit 7)是控制NMI中断开关。当它为0时，允许NMI中断到达处理器，为1时，则阻断所有的NMI信号
	; 	其他7个比特，即0~6位，则实际上用于指定CMOS RAM单元的索引号
	in al, 0x70
	or al, 0x80	
	out 0x70, al				; 关掉不可屏蔽中断

	lgdt [GDT_PTR]				; 加载GDT地址到GDTR寄存器
	jmp dword 0x8:_32bits_mode	; 长跳转刷新CS影子寄存器

; 初始化段寄存器和通用寄存器、栈寄存器，这是为了给调用 inithead_entry 这个 C 函数做准备
_32bits_mode:
	mov ax, 0x10
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	xor eax, eax
	xor ebx, ebx
	xor ecx, ecx
	xor edx, edx
	xor edi, edi
	xor esi, esi
	xor ebp, ebp
	xor esp, esp
	mov esp, 0x7c00		; 设置栈顶为0x7c00
	call inithead_entry
	; 跳转到物理内存的 0x200000 地址处
	; 这时地址还是物理地址，这个地址正是在 inithead.c 中由 write_ldrkrlfile() 函数放置的 initldrkrl.bin 文件，这一跳就进入了二级引导器的主模块了
	jmp 0x200000


; 从GDT_START开始是CPU工作模式所需要的数据
GDT_START:
knull_dsc: dq 0
kcode_dsc: dq 0x00cf9e000000ffff
kdata_dsc: dq 0x00cf92000000ffff
k16cd_dsc: dq 0x00009e000000ffff
k16da_dsc: dq 0x000092000000ffff
GDT_END:
GDT_PTR:
GDTLEN	dw GDT_END-GDT_START-1		; GDT界限
GDTBASE	dd GDT_START
