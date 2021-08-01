%include "ldrasm.inc"
global _start
[section .text]
[bits 16]

_start:
_16_mode:
    mov bp, 0x20        ; ;0x20是指向GDT中的16位数据段描述符
    mov ds, bp
    mov es, bp
    mov ss, bp
    mov ebp, cr0
    and ebp, 0xfffffffe ; 0xfffffffe = 4294967294
    mov	cr0, ebp        ; CR0.P=0 关闭保护模式
    jmp	0: real_entry   ; 刷新CS影子寄存器，真正进入实模式

real_entry:
	mov bp, cs
	mov ds, bp
	mov es, bp
	mov ss, bp          ; 重新设置实模式下的段寄存器 都是CS中值，即为0
	mov sp, 08000h      ; 设置栈
	mov bp, func_table
	add bp, ax
	call [bp]           ; 调用函数表中的汇编函数，ax是C函数中传递进来的
	cli
	call disable_nmi
	mov	ebp, cr0
	or	ebp, 1
	mov	cr0, ebp
	jmp dword 0x8 :_32bits_mode