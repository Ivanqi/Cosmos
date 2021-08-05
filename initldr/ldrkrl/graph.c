#include "cmctl.h"

void write_pixcolor(machbstart_t *mbsp, u32_t x, u32_t y, pixl_t pix)
{
    u8_t *p24bas;
    if (mbsp->mb_ghparm.gh_onepixbits == 24) {
        u32_t p24adr = (x + (y * mbsp->mb_ghparm.gh_x)) * 3;
        p24bas = (u8_t *)(p24adr + mbsp->mb_ghparm.gh_framphyadr);

        p24bas[0] = (u8_t)(pix);
        p24bas[1] = (u8_t)(pix >> 8);
        p24bas[2] = (u8_t)(pix >> 16);

        return;
    }

    u32_t *phybas = (u32_t *)mbsp->mb_ghparm.gh_framphyadr;
    phybas[x + (y * mbsp->mb_ghparm.gh_x)] = pix;

    return;
}

void bmp_print(void *bmfile, machbstart_t *mbsp)
{
    if (NULL == bmfile) {
        return;
    }

    pixl_t pix = 0;
    bmdbgr_t *bpixp = NULL;
    bmfhead_t *bmphdp = (bmfhead_t*)bmfile;
    bitminfo_t *binfp = (bitminfo_t*)(bmphdp + 1);
    u32_t img = (u32_t)bmfile + bmphdp->bf_off;

    bpixp = (bmdbgr_t *)img;

    int l = 0;
    int k = 0;
    int ilinebc = (((binfp->bi_w * 24) + 31) >> 5) << 2;

    for (int y = 639; y >= 129; y--, l++) {
        k = 0;
        for (int x = 322; x < 622; x++) {
            pix = BGRA(bpixp[k].bmd_r, bpixp[k].bmd_g, bpixp[k].bmd_b);
            write_pixcolor(mbsp, x, y, pix);
            k++;
        }

        bpixp = (bmdbgr_t *)(((int)bpixp) + ilinebc);
    }

    return;
}

void logo(machbstart_t *mbsp)
{
    u32_t retadr = 0, sz = 0;
    get_file_rpadrandsz("logo.bmp", mbsp, &retadr, &sz);

    if (0 == retadr) {
        kerror("log getfilerpadrsz err");
    }

    bmp_print((void*)retadr, mbsp);

    return;
}

void init_graph(machbstart_t *mbsp)
{
    graph_t_init(&mbsp->mb_ghparm);
    init_bgadevice(mbsp);

    if (mbsp->mb_ghparm.gh_mode != BGAMODE) {
        get_vbemode(mbsp);
        get_vbemodeinfo(mbsp);
        set_vbemodeinfo();
    }

    init_kinitfvram(mbsp);
    logo(mbsp);
    return;
}