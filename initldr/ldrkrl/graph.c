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
        for (int x = 322; x < 662; x++) {
            pix = BGRA(bpixp[k].bmd_r, bpixp[k].bmd_g, bpixp[k].bmd_b);
            write_pixcolor(mbsp, x, y, pix);
            k++;
        }

        bpixp = (bmdbgr_t *)(((int)bpixp) + ilinebc);
    }

    return;
}

/**
 * os logo. logo文件是个24位的位图文件
 *  调用get_file_rpadrandsz定位到位图文件
 *  调用bmp_print，读入像素点，BGRA转换
 *  最后调用write_pixcolor，写入到mbsp->mb_ghparm正确的位置，图像就显示出来了
 */
void logo(machbstart_t *mbsp)
{
    u32_t retadr = 0, sz = 0;
    get_file_rpadrandsz("logo.bmp", mbsp, &retadr, &sz);

    if (0 == retadr) {
        kerror("logo getfilerpadrsz err");
    }

    bmp_print((void*)retadr, mbsp);

    return;
}

/**
 * 选择VBE的118h模式，该模式下屏幕分辨率为1024 * 768,显存大小是16.8MB。显存开始地址一般为0xe00000000
 * 
 * 屏幕分辨率为1024 * 768，即把屏幕分成768行，每行1024个像素点，但每个像素点占用显存32位数据(4字节，红、绿、蓝、透明各占8位)
 * 
 * 只要往对应的显存地址写入相应的像素数据，屏幕对应的位置就能显示了
 */
void init_graph(machbstart_t *mbsp)
{
    // 初始化图形数据结构
    graph_t_init(&mbsp->mb_ghparm);
    init_bgadevice(mbsp);

    /**
     * 如果不是图形模式，要通过BIOS中断进行切换，设置显示参数，并将参数保存到mbsp结构中：
     *  获取VBE模式，通过BIOS中断
     *  获取一个具体VBE模式的信息，通过BIOS中断
     *  设置VBE模式，通过BIOS中断
     *  这三个方法同样用到了realadr_call_entry
     */
    if (mbsp->mb_ghparm.gh_mode != BGAMODE) {
        // 获取VBE模式，通过BIOS中断
        get_vbemode(mbsp);
        // 获取一个具体VBE模式信息，通过BIOS中断
        get_vbemodeinfo(mbsp);
        // 设置VBE模式，通过BIOS
        set_vbemodeinfo();
    }

    init_kinitfvram(mbsp);
    logo(mbsp);
    return;
}

void graph_t_init(graph_t *initp)
{
    memset(initp, 0, sizeof(graph_t));
    return;
}

void init_kinitfvram(machbstart_t *mbsp)
{
    mbsp->mb_fvrmphyadr = KINITFRVM_PHYADR;
    mbsp->mb_fvrmsz = KINITFRVM_SZ;
    memset((void *)KINITFRVM_PHYADR, 0, KINITFRVM_SZ);

    return;
}

u32_t vfartolineadr(u32_t vfar)
{
    u32_t tmps = 0, sec = 0, off = 0;
    off = vfar & 0xffff;
    tmps = (vfar >> 16) & 0xffff;
    sec = tmps << 4;
    return (sec + off);
}

/**
 * 从BIOS获取数据
 */
void get_vbemode(machbstart_t *mbsp)
{
    realadr_call_entry(RLINTNR(2), 0, 0);
    vbeinfo_t *vbeinfoptr = (vbeinfo_t *)VBEINFO_ADR;
    u16_t *mnm;

    if (vbeinfoptr->vbesignature[0] != 'V' || vbeinfoptr->vbesignature[1] != 'E' || 
        vbeinfoptr->vbesignature[2] != 'S' || vbeinfoptr->vbesignature[3] != 'A') {
        kerror("vbe is not VESA");
    }

    kprint("vbe vbever: %x\n", vbeinfoptr->vbeversion);

    if (vbeinfoptr->vbeversion < 0x0200) {
        kerror("vbe version not vbe3");
    }

    if (vbeinfoptr->videomodeptr > 0xffff) {
        mnm = (u16_t *)vfartolineadr(vbeinfoptr->videomodeptr);
    } else {
        mnm = (u16_t *)(vbeinfoptr->videomodeptr);
    }

    int bm = 0;
    for (int i = 0; mnm[i] != 0xffff; i++) {
        if (mnm[i] == 0x118) {
            bm = 1;
        }

        if (i > 0x1000) {
            break;
        }
    }

    if (bm == 0) {
        kerror("getvbemode not 18");
    }

    mbsp->mb_ghparm.gh_mode = VBEMODE;
    mbsp->mb_ghparm.gh_vbemodenr = 0x118;
    mbsp->mb_ghparm.gh_vifphyadr = VBEINFO_ADR;

    m2mcopy(vbeinfoptr, &mbsp->mb_ghparm.gh_vbeinfo, sizeof(vbeinfo_t));

    return;
}

void bga_write_reg(u16_t index, u16_t data)
{
    out_u16(VBE_DISPI_IOPORT_INDEX, index);
    out_u16(VBE_DISPI_IOPORT_DATA, data);
    return;
}

u16_t bga_read_reg(u16_t index)
{
    out_u16(VBE_DISPI_IOPORT_INDEX, index);
    return in_u16(VBE_DISPI_IOPORT_DATA);
}

u32_t get_bgadevice()
{
    u16_t bgaid = bga_read_reg(VBE_DISPI_INDEX_ID);
    if (BGA_DEV_ID0 <= bgaid && BGA_DEV_ID5 >= bgaid) {
        bga_write_reg(VBE_DISPI_INDEX_ID, bgaid);
        if (bga_read_reg(VBE_DISPI_INDEX_ID) != bgaid) {
            return 0;
        }

        return (u32_t)bgaid;
    }

    return 0;
}

u32_t chk_bgamaxver()
{
    bga_write_reg(VBE_DISPI_INDEX_ID,BGA_DEV_ID5);
    if(bga_read_reg(VBE_DISPI_INDEX_ID)==BGA_DEV_ID5) {
        return (u32_t)BGA_DEV_ID5;
    }

    bga_write_reg(VBE_DISPI_INDEX_ID,BGA_DEV_ID4);
    if(bga_read_reg(VBE_DISPI_INDEX_ID)==BGA_DEV_ID4) {
        return (u32_t)BGA_DEV_ID4;
    }
    
    bga_write_reg(VBE_DISPI_INDEX_ID,BGA_DEV_ID3);
    if(bga_read_reg(VBE_DISPI_INDEX_ID)==BGA_DEV_ID3) {
        return (u32_t)BGA_DEV_ID3;
    }

    bga_write_reg(VBE_DISPI_INDEX_ID,BGA_DEV_ID2);
    if(bga_read_reg(VBE_DISPI_INDEX_ID)==BGA_DEV_ID2) {
        return (u32_t)BGA_DEV_ID2;
    }

    bga_write_reg(VBE_DISPI_INDEX_ID,BGA_DEV_ID1);
    if(bga_read_reg(VBE_DISPI_INDEX_ID)==BGA_DEV_ID1) {
        return (u32_t)BGA_DEV_ID1;
    }

    bga_write_reg(VBE_DISPI_INDEX_ID,BGA_DEV_ID0);
    if(bga_read_reg(VBE_DISPI_INDEX_ID)==BGA_DEV_ID0) {
        return (u32_t)BGA_DEV_ID0;
    }
    return 0;
}

/**
 * 初始化graph_t 结构体
 *  首先获取GBA设备ID
 *  检查设备最大分辨率
 *  设置显示参数，并将参数保存到mbsp结构中
 * 
 * 选择VBE的118h模式，该模式下屏幕分辨率为1024 * 768,显存大小是16.8MB。显存开始地址一般为0xe00000000
 * 
 * 屏幕分辨率为1024 * 768，即把屏幕分成768行，每行1024个像素点，但每个像素点占用显存32位数据(4字节，红、绿、蓝、透明各占8位)
 */
void init_bgadevice(machbstart_t* mbsp)
{
    u32_t retdevid = get_bgadevice();
    if(0 == retdevid) {
        return;
    }

    retdevid = chk_bgamaxver();
    if (0 == retdevid) {
        return;
    }

    // 往vbe相关端口写入数据
    bga_write_reg(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    bga_write_reg(VBE_DISPI_INDEX_XRES, 1024);
    bga_write_reg(VBE_DISPI_INDEX_YRES, 768);
    bga_write_reg(VBE_DISPI_INDEX_BPP, 0x20);
    bga_write_reg(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | (VBE_DISPI_LFB_ENABLED)); 

    mbsp->mb_ghparm.gh_mode = BGAMODE;
    mbsp->mb_ghparm.gh_vbemodenr = retdevid;
    mbsp->mb_ghparm.gh_x = 1024;
    mbsp->mb_ghparm.gh_y = 768;
    mbsp->mb_ghparm.gh_framphyadr = 0xe0000000;
    mbsp->mb_ghparm.gh_onepixbits = 0x20;
    mbsp->mb_ghparm.gh_bank = 4;
    mbsp->mb_ghparm.gh_curdipbnk = 0;
    mbsp->mb_ghparm.gh_nextbnk = 0;
    mbsp->mb_ghparm.gh_banksz = (mbsp->mb_ghparm.gh_x * mbsp->mb_ghparm.gh_x * 4);
    //test_bga();
    return;
}

void  test_bga()
{
    int *p = (int*)(0xe0000000);
    int *p2 = (int*)(0xe0000000 + (1024 * 768 * 4));
    int *p3 = (int*)(0xe0000000 + (1024 * 768 * 4) * 2);

    for (int i= 0; i < (1024 * 768); i++) {
        p2[i] = 0x00ff00ff;
    }

    for (int i = 0;i < (1024 * 768); i++) {
        p[i] = 0x0000ff00;
    }

    for(int i = 0;i < (1024 * 768); i++) {
        p3[i] = 0x00ff0000;
    }

    for(;;) {
        bga_write_reg(VBE_DISPI_INDEX_X_OFFSET, 0);
        bga_write_reg(VBE_DISPI_INDEX_Y_OFFSET, 0);
        bga_write_reg(VBE_DISPI_INDEX_VIRT_WIDTH, 1024);
        bga_write_reg(VBE_DISPI_INDEX_VIRT_HEIGHT, 768);
        die(0x400);

        bga_write_reg(VBE_DISPI_INDEX_X_OFFSET, 0);
        bga_write_reg(VBE_DISPI_INDEX_Y_OFFSET, 768);
        bga_write_reg(VBE_DISPI_INDEX_VIRT_WIDTH, 1024);
        bga_write_reg(VBE_DISPI_INDEX_VIRT_HEIGHT, 768 * 2);
        die(0x400);

        bga_write_reg(VBE_DISPI_INDEX_X_OFFSET, 0);
        bga_write_reg(VBE_DISPI_INDEX_Y_OFFSET, 768 * 2);
        bga_write_reg(VBE_DISPI_INDEX_VIRT_WIDTH, 1024);
        bga_write_reg(VBE_DISPI_INDEX_VIRT_HEIGHT, 768 * 3);
        die(0x400);
    }

    for(;;);
    return;
}

void get_vbemodeinfo(machbstart_t* mbsp)
{
    realadr_call_entry(RLINTNR(3), 0, 0);
    vbeominfo_t *vomif=(vbeominfo_t*)VBEMINFO_ADR;

    u32_t x = vomif->XResolution, y = vomif->YResolution;
    u32_t* phybass = (u32_t*)(vomif->PhysBasePtr);

    if (vomif->BitsPerPixel < 24) {
        kerror("vomif->BitsPerPixel!=32");
    }

    if (x != 1024 || y != 768) {
        kerror("xy not");
    }

    if ((u32_t)phybass < 0x100000) {
        kerror("phybass not");
    }

    mbsp->mb_ghparm.gh_x = vomif->XResolution;
    mbsp->mb_ghparm.gh_y = vomif->YResolution;
    mbsp->mb_ghparm.gh_framphyadr = vomif->PhysBasePtr;
    mbsp->mb_ghparm.gh_onepixbits = vomif->BitsPerPixel;
    mbsp->mb_ghparm.gh_vmifphyadr = VBEMINFO_ADR;
    m2mcopy(vomif, &mbsp->mb_ghparm.gh_vminfo, sizeof(vbeominfo_t));

    return;
}

void set_vbemodeinfo()
{
    realadr_call_entry(RLINTNR(4), 0, 0);
    return;
}

u32_t utf8_to_unicode(utf8_t *utfp,int *retuib)
{
    u8_t uhd = utfp->utf_b1,ubyt = 0;
    u32_t ucode=0,tmpuc=0;

    // 0xbf&&uhd<=0xbf
    if(0x80 > uhd) {
        ucode=utfp->utf_b1&0x7f;
        *retuib=1;
        return ucode;
    }

    // 0xdf
    if(0xc0 <= uhd&&uhd <= 0xdf) {
        ubyt = utfp->utf_b1 & 0x1f;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b2 & 0x3f;
        ucode = (tmpuc << 6) | ubyt;
        *retuib = 2;
        return ucode;
    }

    // 0xef
    if(0xe0 <= uhd&&uhd <= 0xef) {
        ubyt = utfp->utf_b1 & 0x0f;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b2 & 0x3f;
        tmpuc <<= 6;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b3 & 0x3f;
        ucode = (tmpuc << 6) | ubyt;
        *retuib = 3;
        return ucode;
    }

    //0xf7
    if(0xf0 <= uhd && uhd <= 0xf7) {
        ubyt = utfp->utf_b1 & 0x7;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b2 & 0x3f;
        tmpuc <<= 6;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b3 & 0x3f;
        tmpuc <<= 6;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b4 & 0x3f;
        ucode = (tmpuc << 6) | ubyt;
        *retuib = 4;
        return ucode;
    }

    //0xfb
    if(0xf8 <= uhd && uhd <= 0xfb) {
        ubyt = utfp->utf_b1 & 0x3;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b2 & 0x3f;
        tmpuc <<= 6;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b3 & 0x3f;
        tmpuc <<= 6;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b4 & 0x3f;
        tmpuc <<= 6;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b5 & 0x3f;
        ucode = (tmpuc << 6) | ubyt;
        *retuib = 5;
        return ucode;
    }

    //0xfd
    if(0xfc <= uhd && uhd <= 0xfd) {
        ubyt = utfp->utf_b1 & 0x1;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b2 & 0x3f;
        tmpuc <<= 6;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b3 & 0x3f;
        tmpuc <<= 6;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b4 & 0x3f;
        tmpuc <<= 6;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b5 & 0x3f;
        tmpuc <<= 6;
        tmpuc |= ubyt;
        ubyt = utfp->utf_b6 & 0x3f;
        ucode = (tmpuc << 6) | ubyt;
        *retuib = 6;
        return ucode;
    }

    *retuib = 0;
    return 0;
}