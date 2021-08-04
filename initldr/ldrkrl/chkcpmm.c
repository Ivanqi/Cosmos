/**********************************************************
	系统全局内存检查文件chkcpmm.c
***********************************************************/

#include "cmctl.h"

unsigned int acpi_get_bios_ebda()
{
    unsigned int address = *(unsigned short *)0x40E;
    address <<= 4;
    return address;
}

int acpi_checksum(unsigned char *ap, s32_t len)
{
    int sum = 0;
    while (len--) {
        sum += *ap++;
    }

    return sum & 0xFF;
}

mrsdp_t *acpi_rsdp_isok(mrsdp_t *rdp)
{
    if (rdp->rp_len == 0 || rdp->rp_revn == 0) {
        return NULL;
    }

    if (0 == acpi_checksum((unsigned char *)rdp, (s32_t)rdp->rp_len)) {
        return rdp;
    }

    return NULL;
}

mrsdp_t *findacpi_rsdp_core(void *findstart, u32_t findlen)
{
    if (NULL == findstart || 1024 > findlen) {
        return NULL;
    }

    u8_t *tmpdp = (u8_t *)findstart;

    mrsdp_t *retdrp = NULL;

    for (u64_t i = 0; i <= findlen; i++) {
        if (('R' == tmpdp[i]) && ('S' == tmpdp[i + 1]) && ('D' == tmpdp[i + 2]) && (' ' == tmpdp[i + 3]) 
            && ('P' == tmpdp[i + 4]) && ('T' == tmpdp[i + 5]) && ('R' == tmpdp[i + 6]) && (' ' == tmpdp[i + 7])) 
        {
            retdrp = acpi_rsdp_isok((mrsdp_t *)(&tmpdp[i]));
            if (NULL != retdrp) {
                return retdrp;
            }
        }
    }

    return NULL;
}

PUBLIC mrsdp_t *find_acpi_rsdp()
{
    void *fndp = (void *)acpi_get_bios_ebda();
    mrsdp_t *rdp = findacpi_rsdp_core(fndp, 1024);
    
    if (NULL != rdp) {
        return rdp;
    }

    //0E0000h和0FFFFFH
    fndp = (void *)(0xe0000);
    rdp = findacpi_rsdp_core(fndp, (0xfffff - 0xe0000));

    if (NULL != rdp) {
        return rdp;
    }

    return NULL;
}

PUBLIC void init_acpi(machbstart_t *mbsp)
{
    mrsdp_t *rdp = NULL;
    rdp = find_acpi_rsdp();

    if (NULL == rdp) {
        kerror("Your computer is not support ACPI!");
    }

    m2mcopy(rdp, &mbsp->mb_mrsdp, (sint_t)((sizeof(mrsdp_t))));

    if (acpi_rsdp_isok(&mbsp->mb_mrsdp) == NULL) {
        kerror("Your computer is not support ACPI!");
    }

    return ;
}

void init_mem(machbstart_t *mbsp)
{
    e820map_t *retemp;
    u32_t retemnr = 0;

    mbsp->mb_ebdaphyadr = acpi_get_bios_ebda();
    mmap(&retemp, &retemnr);

    if (retemnr == 0) {
        kerror("no e820map\n");
    }

    if (chk_memsize(retemp, retemnr, 0x100000, 0x8000000) == NULL) {
        kerror("Your computer is low on memory, the memory cannot be less than 128MB!");
    }

    mbsp->mb_e820expadr = (u64_t)((u32_t)(retemp));
    mbsp->mb_e820nr = (u64_t)retemnr;
    mbsp->mb_e820sz = retemnr * (sizeof(e820map_t));
    mbsp->mb_memsz = get_memsize(retemp, retemnr);
    init_acpi(mbsp);

    return ;
}

void init_chkcpu(machbstart_t *mbsp)
{
    if (!chk_cpuid()) {
        kerror("Your CPU is not support CPUID sys is die!");
        CLI_HALT();
    }

    if (!chk_cpu_longmode()) {
        kerror("Your CPU is not support 64bits mode sys is die!");
        CLI_HALT();
    }

    mbsp->mb_cpumode = 0x40;
    return ;
}

void init_krlinitstack(machbstart_t *mbsp)
{
    if (1 > move_krlimg(mbsp, (u64_t)(0x8f000), 0x1001)) {
        kerror("iks_moveimg err");
    }

    mbsp->mb_krlinitstack = IKSTACK_PHYADR;
    mbsp->mb_krlitstacksz = IKSTACK_SIZE;

    return ;
}

void init_bstartpages(machbstart_t *mbsp)
{
    u64_t *p = (u64_t *)(KINITPAGE_PHYADR);
    u64_t *pdpte = (u64_t *)(KINITPAGE_PHYADR + 0x1000);
    u64_t *pde = (u64_t *)(KINITPAGE_PHYADR + 0x2000);

    u64_t adr = 0;

    if (1 > move_krlimg(mbsp, (u64_t)(KINITFRVM_PHYADR), (0x1000 * 16 + 0x2000))) {
        kerror("move_krlimg err");
    }

    for (uint_t mi = 0; mi < PGENTY_SIZE; mi++) {
        p[mi] = 0;
        pdpte[mi] = 0;
    }

    for (uint_t pdei = 0; pdei < 16; pdei++) {
        pdpte[pdei] = (u64_t)((u32_t)pde | KPDPTE_RW | KPDPTE_P);
        for (uint_t pdeii = 0; pdeii < PGENTY_SIZE; pdeii++) {
            pde[pdeii] = 0 | adr | KPDE_PS | KPDE_RW | KPDE_P;
            adr += 0x200000;
        }
        pde = (u64_t *)((u32_t)pde + 0x1000);
    }

    p[((KRNL_VIRTUAL_ADDRESS_START) >> KPML4_SHIFT) & 0x1ff] = (u64_t)((u32_t)pdpte | KPML4_RW | KPML4_P);
    p[0] = (u64_t)((u32_t)pdpte | KPML4_RW | KPML4_P);
    mbsp->mb_pml4padr = (u64_t)(KINITPAGE_PHYADR);
    mbsp->mb_subpageslen = (u64_t)(0x1000 * 16 + 0x2000);
    mbsp->mb_kpmapphymemsz = (u64_t)(0x400000000);

    return ;
}

void init_meme820(machbstart_t *mbsp)
{
    e820map_t *semp = (e820map_t *)((u32_t)(mbsp->mb_e820padr));
    u64_t senr = mbsp->mb_e820nr;
    e820map_t *demp = (e820map_t *)((u32_t)(mbsp->mb_nextwtpadr));

    if (1 > move_krlimg(mbsp, (u64_t)((u32_t)demp), (senr * (sizeof(e820map_t))))) {
        kerror("move_krlimg err");
    }

    m2mcopy(semp, demp, (sint_t)(senr * (sizeof(e820map_t))));
    mbsp->mb_e820padr = (u64_t)((u32_t)(demp));
    mbsp->mb_e820sz = senr * (sizeof(e820map_t));
    mbsp->mb_nextwtpadr = mbsp->mb_e820exnr + mbsp->mb_e820sz;
    return ;
}

void mmap(e820map_t **retemp, u32_t *retemnr)
{
    realadr_call_entry(RLINTNR(0), 0, 0);
    *retemnr = *((u32_t *)(E80MAP_NR));
    *retemp = (e820map_t *)(*((u32_t *)(E80MAP_ADRADR)));
    return ;
}