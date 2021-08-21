%include "ldrasm.inc"
global _start
global realadr_call_entry
global IDT_PTR
global ldrkrl_entry
[section .text]
[bits 32]
_start:
_entry:
	cli
	lgdt [GDT_PTR]                  ; 加载GDT地址到GDTR寄存器
	lidt [IDT_PTR]                  ; 加载IDT地址到IDTR寄存器
	jmp dword 0x8 :_32bits_mode     ; 长跳转刷新CS影子寄存器

; 初始化CPU相关寄存器
_32bits_mode:
	mov ax, 0x10	                ; 数据段选择子(目的)
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
	mov esp, 0x90000
	call ldrkrl_entry
	xor ebx, ebx
	jmp 0x2000000
	jmp $

; PUSHAD, 本指令将EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI 这8个32位通用寄存器依次压入堆栈
; POPAD, 指令依次弹出堆栈中的32位字到 EDI,ESI,EBP,ESP,EBX,EDX,ECX,EAX 中
; push指令, pushl %eax: 将eax数值压入栈中
; popl指令, 将eax数值弹出栈

realadr_call_entry:
	pushad                      ; 保存通用寄存器
	push    ds
	push    es
	push    fs                  ; 保存4个段寄存器
	push    gs
	call save_eip_jmp           ; 调用save_eip_jmp
	pop	gs
	pop	fs
	pop	es                      ; 恢复4个段寄存器
	pop	ds
	popad                       ; 恢复通用寄存器
	ret

save_eip_jmp:
	pop esi                      ; 弹出call save_eip_jmp时保存的eip到esi寄存器中
	mov [PM32_EIP_OFF], esi      ; 把eip保存到特定的内存空间中
	mov [PM32_ESP_OFF], esp      ; 把esp保存到特定的内存空间中

    ; 这个指令是一个长跳转，表示把[cpmty_mode]处的数据装入 CS：EIP，也就是把 0x18：0x1000 装入到 CS：EIP 中
    ; 这个 0x18 就是段描述索引, 它正是指向 GDT 中的 16 位代码段描述符
    ; 0x1000 代表段内的偏移地址，所以在这个地址上，我们必须放一段代码指令，不然 CPU 跳转到这里将没指令可以执行，那样就会发生错误
	jmp dword far [cpmty_mode]  ; 长跳转这里表示把cpmty_mode处的第一个4字节装入eip，把其后的2字节装入cs

GDT_START:
knull_dsc: dq 0
kcode_dsc: dq 0x00cf9a000000ffff    ; a-e, 58434644969848830
kdata_dsc: dq 0x00cf92000000ffff    ; 58425848876826620
k16cd_dsc: dq 0x00009a000000ffff    ; a-e, 169324790743039, 16位代码段描述符
k16da_dsc: dq 0x000092000000ffff    ; 160528697720831, 16位数据段描述符
GDT_END:

GDT_PTR:
GDTLEN	dw GDT_END-GDT_START - 1	; GDT界限
GDTBASE	dd GDT_START