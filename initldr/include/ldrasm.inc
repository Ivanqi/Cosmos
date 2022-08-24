; 描述符
; usage: Descriptor Base, Limit, Attr
;        Base:  dd
;        Limit: dd (low 20 bits available)
;        Attr:  dw (lower 4 bits of higher byte are always 0)
%macro Pmdsc 3
	dw	%2 & 0FFFFh				; 段界限 1				(2 字节)0-15
	dw	%1 & 0FFFFh				; 段基址 1				(2 字节)16-31
	db	(%1 >> 16) & 0FFh			; 段基址 2				(1 字节)32-39
	dw	((%2 >> 8) & 0F00h) | (%3 & 0F0FFh)	; 属性 1 + 段界限 2 + 属性 2		(2 字节)40-55
	db	(%1 >> 24) & 0FFh			; 段基址 3				(1 字节)56-63
%endmacro ; 共 8 字节


; usage: Descriptor Base, Limit, Attr
%macro Gmdsc 3
	dw	%2 & 0FFFFh				;(0-15)
	dw	%1 & 0FFFFh				;(16-31)
	db	((%1 >> 16) & 0FFh)			;(32-39)
	db	(%3 & 0FFh)				;(40-47)
	db	((%3 >> 8) & 0F0h)|((%2 >> 16) & 0Fh)	;(48-55)
	db	((%1 >> 24) & 0FFh)			;(56-63)
%endmacro

RWUPNA_DATE EQU 0x2                     ; 2
ECMRNA_CODE EQU 0xe                     ; 14
CD_TYPE EQU 0x8                         ; 8
DPL_0 EQU 0                             ; 0
DPL_1 EQU 0x20                          ; 32
DPL_2 EQU 0x40                          ; 64
DPL_3 EQU 0x60                          ; 96
PES EQU 0x80                            ; 128
L_MODE EQU 0x2000                       ; 8192
C_MODE EQU 0                            ; 0
D32OP EQU 0x4000                        ; 16384
D16OP EQU 0                             ; 0
G4K EQU 0x8000                          ; 32768
G1B EQU 0                               ; 0

MIN_BASE EQU 0                          ; 0
MAX_LIMT EQU 0xfffff                    ; 1048575

CODE_FORMADR_SEC EQU 0x7c0              ; 1984
CODE_FORMADR_OFF EQU 0                  ; 0
CODE_NEWADR_SEC EQU 0x1000              ; 4096
CODE_NEWADR_OFF EQU 0                   ; 0

ETYBAK_ADR EQU 0x2000                   ; 特定存储空间
PM32_EIP_OFF EQU (ETYBAK_ADR)           ; 32位EIP暂存空间
PM32_ESP_OFF EQU (ETYBAK_ADR + 4)       ; 32位ESP暂存空间
RM16_EIP_OFF EQU (ETYBAK_ADR + 8)       ; 16位EIP暂存空间
RM16_ESP_OFF EQU (ETYBAK_ADR + 12)      ; 16位ESP暂存空间

RWHDPACK_ADR EQU (ETYBAK_ADR + 32)

; e820内存地址
E80MAP_NR EQU (ETYBAK_ADR + 64)
E80MAP_ADRADR EQU (ETYBAK_ADR + 68)
E80MAP_ADR EQU (0x5000)                ; edi设为存放输出结果的1MB内的物理内存地

VBEINFO_ADR EQU (0x6000)               ; VBE信息地址
VBEMINFO_ADR EQU (0x6400)              ; 具体VBE模式信息地址

READHD_BUFADR EQU 0x3000               ; 12288