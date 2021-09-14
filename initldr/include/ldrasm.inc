
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
E80MAP_NR EQU (ETYBAK_ADR + 64)
E80MAP_ADRADR EQU (ETYBAK_ADR + 68)
E80MAP_ADR EQU (0x5000)                ; 20480

VBEINFO_ADR EQU (0x6000)               ; VBE信息地址
VBEMINFO_ADR EQU (0x6400)              ; 25600

READHD_BUFADR EQU 0x3000               ; 12288