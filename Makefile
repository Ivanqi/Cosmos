###################################################################
#		主控自动化编译配置文件 Makefile			                      #
###################################################################
MAKEFLAGS = -sR

# 命令
MKDIR = mkdir
RMDIR = rmdir
CP = cp
CD = cd
DD = dd
RM = rm

LKIMG = ./lmoskrlimg -m k

# qemu-system-x86_64 使用 qemu-system-x86 来启动 x86 架构的虚拟机
VM = qemu-system-x86_64

# bochs是一个x86平台模拟器
DBUGVM = bochs -q

# qemu-img是创建、转换、修改磁盘映像的工具
IMGTOVDI = qemu-img convert -f raw -O vdi
IMGTOVMDK = qemu-img convert -f raw -O vmdk
MAKE = make

X86BARD = -f ./Makefile.x86

IMGSECTNR = 204800
PHYDSK = /dev/sdb
VDIFNAME = hd.vdi
VMDKFNAME = hd.img
KRNLEXCIMG = Cosmos.bin

# 图片
LOGOFILE = logo.bmp background.bmp
# 字体
FONTFILE = font.fnt

BUILD_PATH = ./build
EXKNL_PATH = ./exckrnl
DSTPATH = ../exckrnl
RELEDSTPATH = ../release
INITLDR_BUILD_PATH =./initldr/build/
INITLDR_PATH =./initldr/
CPLILDR_PATH =./release/
INSTALL_PATH =/boot/
INSTALLSRCFILE_PATH =./release/Cosmos.eki


VVMRLMOSFLGS = -C $(BUILD_PATH) -f vbox.mkf

VBOXVMFLGS = -C $(VM_PATH) -f vbox.mkf
VMFLAGES = -smp 4 -hda $(VMDKFNAME) -m 256 -enable-kvm


SRCFILE = $(BOOTEXCIMG) $(KRNLEXCIMG) $(LDEREXCIMG) $(SHELEXCIMG)
RSRCFILE = $(BOOTEXCIMG) $(KRNLEXCIMG) $(LDEREXCIMG) $(SHELEXCIMG) #$(VDIFNAME) $(VMDKFNAME)

INITLDRIMH = initldrimh.bin
INITLDRKRL = initldrkrl.bin
INITLDRSVE = initldrsve.bin

CPLILDRSRC = $(INITLDR_BUILD_PATH)$(INITLDRSVE) $(INITLDR_BUILD_PATH)$(INITLDRKRL) $(INITLDR_BUILD_PATH)$(INITLDRIMH)

LKIMG_INFILE = $(INITLDRSVE) $(INITLDRKRL) $(KRNLEXCIMG) $(FONTFILE) $(LOGOFILE)
.PHONY: install x32 build print clean all krnlexc cpkrnl wrimg phymod exc vdi vdiexc cprelease release createimg

# 移动文件. 把 ./initldr/build/ 的文件移动到 ./release/
cplmildr:
	$(CP) $(CPFLAGES) $(CPLILDRSRC) $(CPLILDR_PATH)

# 进入./build目录，把 Cosmos.bin 文件移动到  ../exckrnl
cpkrnl:
	$(CD) $(BUILD_PATH) && $(CP) $(CPFLAGES) $(SRCFILE) $(DSTPATH)

# 进入 ./exckrnl目录, 把 Cosmos.bin 文件 移动到 ../release
cprelease:
	$(CD) $(EXKNL_PATH) && $(CP) $(CPFLAGES) $(RSRCFILE) $(RELEDSTPATH)

# 进入./release/，通过 lmoskrlimg 把 initldrimh.bin initldrsve.bin initldrkrl.bin Cosmos.bin font.fnt logo.bmp background.bmp 打包成 Cosmos.eki
KIMG:
	@echo '正在生成Cosmos内核映像文件，请稍后……'
	$(CD) $(CPLILDR_PATH) && $(LKIMG) -lhf $(INITLDRIMH) -o Cosmos.eki -f $(LKIMG_INFILE)

VBOXRUN:
	$(MAKE) $(VVMRLMOSFLGS)

all:
	$(MAKE) $(X86BARD)

clean:
	$(CD) $(INITLDR_PATH); $(MAKE) clean
	$(CD) $(BUILD_PATH); $(RM) -f *.o *.bin *.i *.krnl *.s *.map *.lib *.btoj *.vdi *.elf *vmdk *.lds *.mk *.mki krnlobjs.mh
	$(CD) $(EXKNL_PATH); $(RM) -f *.o *.bin *.i *.krnl *.s *.map *.lib *.btoj *.vdi *.elf *vmdk *.lds *.mk *.mki krnlobjs.mh
	$(CD) $(CPLILDR_PATH); $(RM) -f *.o *.bin *.i *.krnl *.s *.eki *.map *.lib *.btoj *.elf *.vdi *vmdk *.lds *.mk *.mki krnlobjs.mh
	@echo 'Cosmos:清理全部已构建文件... ^_^'

print:
	@echo $(LKIMG_INFILE)
	@echo '*********正在开始编译构建系统*************'

#cpkrnl cprelease
release: clean all cplmildr cpkrnl cprelease KIMG 

vboxtest: release VBOXRUN
