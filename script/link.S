###################################################################
#		krnllink自动化编译配置文件 Makefile                          #
###################################################################
MAKEFLAGS =

include krnlbuidcmd.mk
include krnlobjs.mk

.PHONY : all everything build_kernel
all: build_kernel

build_kernel:everything
#$(LDER_EXC_BIN)

everything:   
# 通过ld 把 BUILD_MK_LINK_OBJS 打包成 KERNL_MK_ELFF_FILE(Cosmos.elf)
# ld -s -static -T cosmoslink.lds -n -Map cosmos.map -o Cosmos.elf init_entry.o hal_start.o
[$(LD) $(LDFLAGS) -o $(KERNL_MK_ELFF_FILE) $(BUILD_MK_LINK_OBJS)

# objcopy 将一个目标文件的内容拷贝到另外一个目标文件. 通过 objcopy 把 KERNL_MK_ELFF_FILE(Cosmos.elf) 拷贝成 KERNL_MK_BINF_FILE
# objcopy -S -O binary Cosmos.elf Cosmos.bin
[$(OBJCOPY) $(OJCYFLAGS) $(KERNL_MK_ELFF_FILE) $(KERNL_MK_BINF_FILE)
	
#$(BOOT_EXC_ELF) $(BOOT_EXC_BIN)

$(BOOT_EXC_ELF): $(LMOSEM_LINK_OBJS)
[$(LD) $(LDFLAGS) -o $(BOOT_EXC_ELF)  $(LMOSEM_LINK_OBJS)
[@echo 'LD -[M] 正在构建...' $@
$(BOOT_EXC_BIN):
	
//$(BUILD_MK_LMKR_EBIN): $(BUILD_MK_LMKR_LINK)
//	$(LD) $(KERNELLDFLAGS) -o $(BUILD_MK_LMKR_EBIN) $(BUILD_MK_LMKR_LINK)