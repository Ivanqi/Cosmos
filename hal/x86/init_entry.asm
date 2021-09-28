;**************************************************************************
;*		内核初始化入口文件init_entry.asm				    			 	 *
;**************************************************************************

%define MBSP_ADR 0x100000
%define IA32_EFER 0C0000080H
%define PML4T_BADR 0x1000000
%define KRLVIRADR 0xffff800000000000	; 内核虚拟空间
%define KINITSTACK_OFF 16

global _start
global x64_GDT
extern hal_start

[section .start.text]
[BITS 32]

; 1. 关闭中断
; 2. 通过LGDT命令，指定长度和初始位置，加载GDT
; 3. 设置页表，开始PAE[CR4第5位设置为1]，将页表顶级目录放入CR3
; 4. 读取EFER，将第8位设置为1，写回EFER，设置为长模式
; 5. 开启保护模式[CR0第0位设置为1]，开始分页[CR0第31位设置位1]，开始CACHE[CR0第30位设置为0]，开始WriteThrough[CR0第29位设置为0]
; 6. 初始化寄存器们
; 7. 将之前复制到Cosmos.bin之后的mbsp地址，放入rsp
; 8. 0入栈，0x8入栈, hal_start入栈
; 9. 调用机器指令 "oxcb48"，做一个"返回"操作，同时从栈中弹出两个数据[0x8:hal_start 函数地址]到[CR:RIP],长模式下,指令寄存器为RIP，
;	也就是下一个指令为hal_start地址.CS中为0x8，对应到ekml_c64_dsc，对应到内核代码段，可以执行。CPU继续缓冲RIP地址执行
; 10. hal_start.c, 执行hal_start函数
_start:
	cli
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov fs, ax
	mov gs, ax
    lgdt [eGdtPtr]        
;开启 PAE
    mov eax, cr4
    bts eax, 5                      ; CR4.PAE = 1
    mov cr4, eax
    mov eax, PML4T_BADR				; 加载MMU顶级页目录
    mov cr3, eax	
;开启 64bits long-mode
    mov ecx, IA32_EFER
    rdmsr
    bts eax, 8                      ; IA32_EFER.LME =1
    wrmsr
;开启 PE 和 paging
    mov eax, cr0
    bts eax, 0                      ; CR0.PE =1
    bts eax, 31
;开启 CACHE       
    btr eax, 29						; CR0.NW=0
    btr eax, 30						; CR0.CD=0  CACHE
        
    mov cr0, eax                    ; IA32_EFER.LMA = 1
    jmp 08:entry64

[BITS 64]
entry64:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov fs, ax
	mov gs, ax
	xor rax, rax
	xor rbx, rbx
	xor rbp, rbp
	xor rcx, rcx
	xor rdx, rdx
	xor rdi, rdi
	xor rsi, rsi
	xor r8, r8
	xor r9, r9
	xor r10, r10
	xor r11, r11
	xor r12, r12
	xor r13, r13
	xor r14, r14
	xor r15, r15
    mov rbx, MBSP_ADR
    mov rax, KRLVIRADR
    mov rcx, [rbx + KINITSTACK_OFF]
    add rax, rcx
    xor rcx, rcx
	xor rbx, rbx
	mov rsp, rax
	push 0
	push 0x8
    mov rax, hal_start   			; 调用内核主函数
	push rax
    dw 0xcb48						; iret返回指令
    jmp $

[section .start.data]
[BITS 32]
ex64_GDT:
enull_x64_dsc:	dq 0	
ekrnl_c64_dsc:  dq 0x0020980000000000           ; 64-bit 内核代码段
ekrnl_d64_dsc:  dq 0x0000920000000000           ; 64-bit 内核数据段

euser_c64_dsc:  dq 0x0020f80000000000           ; 64-bit 用户代码段
euser_d64_dsc:  dq 0x0000f20000000000           ; 64-bit 用户数据段
eGdtLen			equ	$ - enull_x64_dsc			; GDT长度
eGdtPtr:		dw eGdtLen - 1					; GDT界限
				dq ex64_GDT
