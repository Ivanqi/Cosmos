#ifndef _PARAM_H
#define _PARAM_H
#define KRNEL_MODE 1    // 内核模式
#define BOOT_MODE 2     // BOOT模式
#define LDSK_MODE 3     // LDSK模式
#define UNDO_MODE 4     // UNDO模式
typedef struct s_mparam {
    int mp_argc;        // 输入的参数的数量
    char **mp_argv;     // 输出的参数的内容
    uint_t mp_sifnr;    // 输入文件的开始地址
    uint_t mp_eifnr;    // 输入文件的结束地址
    uint_t mp_sofnr;    // 输出的映像文件名
    uint_t mp_ifcurrnr;
    uint_t mp_ildrhnr;  // GRUB头文件
    uint_t mp_imgmode;  // 模式
    uint_t mp_ifnr;
} mparam_t;

void init_param();
void exit_param();
void mparam_t_init(mparam_t *initp);
void limg_param(int argc, char *argv[]);
void limg_param_is_inputfile();
void limg_param_is_outfile();
void limg_param_is_ldhfile();
void limg_param_is_mode();
uint_t limg_ret_allfilblk();
uint_t limg_ret_allifimglen();
uint_t limg_ret_allinfilesz();
char *limg_retnext_ipathname();
char *limg_ret_ldrhpathname();
char *limg_retnext_opathname();
uint_t limg_ret_infilenr();
uint_t limg_ret_imgmode();

#endif