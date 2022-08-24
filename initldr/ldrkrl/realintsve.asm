%include "ldrasm.inc"
global _start
[section .text]
[bits 16]

;;; 功能介绍
;;;;  首先从_16_mode:标号处进入实模式，然后根据传递进来(有ax寄存器传入)的函数号，到函数表中调用对应函数
;;;;  里面的的函数执行完成后，再次进入保护模式，加载EIP和ESP寄存器从而回到realadr_call_entry函数中
;;;;  GDT还是imginithead 汇编代码文件中的GDT，这没有变，因为它是有GDTR寄存器指向的

_start:
_16_mode:
    mov bp, 0x20        ; 0x20是指向GDT中的16位数据段描述符
    mov ds, bp
    mov es, bp
    mov ss, bp
    mov ebp, cr0
    and ebp, 0xfffffffe ; ebp为 1, 0xfffffffe = 1111 1111 1111 1111 | 1111 1111 1111 1110
    mov	cr0, ebp        ; CR0.P=0 关闭保护模式
    jmp	0:real_entry    ; 刷新CS影子寄存器，真正进入实模式

real_entry:
	mov bp, cs
	mov ds, bp
	mov es, bp
	mov ss, bp          			; 重新设置实模式下的段寄存器 都是CS中值，即为0
	mov sp, 08000h      			; 设置栈
	mov bp, func_table				; 然后到real_entry这里，通过传入的参数ax，判断调用func_table哪个方法
	add bp, ax						; 当前参数位0，ax就是0，也就是调用了func_table的第一个函数_getmmap
	call [bp]           			; 调用函数表中的汇编函数，ax是C函数中传递进来的
	cli
	call disable_nmi
	mov	ebp, cr0
	or	ebp, 1
	mov	cr0, ebp					; CRO.P=1 开始保护模式
	jmp dword 0x8:_32bits_mode		; dword(double word， 双字，4个字节)

[BITS 32]
_32bits_mode:
	mov bp, 0x10
	mov ds, bp
	mov ss, bp					; 重新设置保护摸下的段寄存器0x10是32为数据段描述符的索引
	mov esi, [PM32_EIP_OFF]		; 加载先前保存的EIP
	mov esp, [PM32_ESP_OFF]		; 加载先前保存的ESP
	jmp esi						; eip=esi 回到了realadr_call_entry函数中

[BITS 16]
DispStr:
	mov bp, ax
	mov cx, 23
	mov ax, 01301h
	mov bx, 000ch
	mov dh, 10
	mov dl, 25
	mov bl, 15
	int 10h
	ret

cleardisp:
	mov ax, 0600h				; 这段代码是为了清屏
	mov bx, 0700h
	mov cx, 0
	mov dx, 0184fh
	int 10h						; 调用的BOIS的10号
	ret

; 获取内存布局试图的函数
; 	_getmmap和loop在迭代执行中断，每次中断都要输出一个20字节大小数据项
; 	最后会形成一个s_e820的的结构体
_getmmap:
	push ds
	push es
	push ss
	mov esi, 0
	mov dword[E80MAP_NR], esi				; 把esi的机器码赋予给内存地址 E80MAP_NR。用于计算内存页个数
	mov dword[E80MAP_ADRADR], E80MAP_ADR	; e820map结构体开始地址

	xor ebx, ebx							; ebx设为0
	mov edi, E80MAP_ADR

loop:
	mov eax, 0e820h							; eax必须为0e820h。遍历主机上全部内存。获取e820map结构参数
	mov ecx, 20								; 输出结果数据项的大小为20字节：8字节内存基地址，8字节内存长度，4字节内存类型。e820map结构大小
	mov edx, 0534d4150h						; edx必须为0534d4150h。获取e820map结构参数必须是这个数据
	int 15h									; 执行中断
	jc .1									; 如果flags寄存器的C位置1，则表示出错

	add edi, 20								; 更新下一次输出结果的地址
	cmp edi, E80MAP_ADR + 0x1000
	jg .1

	inc esi									; 计算内存页个数

	cmp ebx, 0								; 如ebx为0，则表示循环迭代结束
	jne loop								; 还有结果项，继续迭代。循环获取e820map结构

	jmp .2

.1:
	mov esi, 0

.2:
	mov dword[E80MAP_NR], esi
	pop ss
	pop es
	pop ds
	ret

; 获取硬盘的函数
_read:
	push ds
	push es
	push ss
	xor eax, eax
	mov ah, 0x42				; 66
	mov dl, 0x80				; 128
	mov si, RWHDPACK_ADR
	int 0x13					; 硬盘中断
	jc .err						; jc 进位则跳转，关联CF位
	pop ss
	pop es
	pop ds
	ret

; 错误信息
.err:
	mov ax, int131errmsg
	call DispStr
	jmp $
	pop ss
	pop es
	pop ds
	ret

; 获取显卡VBE模式
_getvbemode:
	push es
	push ax
	push di
	mov di, VBEINFO_ADR
	mov ax, 0
	mov es, ax
	mov ax, 0x4f00				; VBE标准
	int 0x10
	cmp ax, 0x004f				; 若有VBE，AX应该为0x004f
	jz .ok						; 为0则跳转
	mov ax, getvbmodeerrmsg
	call DispStr
	jmp $

.ok:
	pop di
	pop ax
	pop es
	ret

_getvbeonemodeinfo:
	push es
	push ax
	push di
	push cx
	mov di, VBEMINFO_ADR
	mov ax, 0
	mov es, ax
	mov cx, 0x118
	mov ax, 0x4f01
	int 0x10
	cmp ax, 0x004f
	jz .ok
	mov ax, getvbmodinfoeerrmsg
	call DispStr
	jmp $

.ok:
	pop cx
	pop di
	pop ax
	pop es
	ret

_setvbemode:
	push ax
	push bx
	mov bx, 0x4118
	mov ax, 0x4f02
	int 0x10
	cmp ax, 0x004f
	jz .ok
	mov ax, setvbmodeerrmsg
	call DispStr
	jmp $

.ok:
	pop bx
	pop ax
	ret

; NMI中断
disable_nmi:
	push ax
	in al, 0x70		; port 0x70NMI_EN_PORT
	or al, 0x80		; disable all NMI source
	out 0x70, al	; NMI_EN_PORT
	pop ax
	ret

func_table:						; 函数表
	dw _getmmap					; 获取内存布局视图的函数
	dw _read					; 读取硬盘的函数
	dw _getvbemode				; 获取显卡VBE模式
	dw _getvbeonemodeinfo		; 获取显卡VBE模式的数据
	dw _setvbemode				; 设置显卡VBE模式

BootM:  db     "pengdong, OS is loading"
	db 0

int131errmsg: db "int 13 read hdsk  error"
	db 0

getvbmodeerrmsg: db "get vbemode err"
	db 0

getvbmodinfoeerrmsg: db "get vbemodeinfo err"
	db 0

setvbmodeerrmsg: db "set vbemode err"
    db 0

