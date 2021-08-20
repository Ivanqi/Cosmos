include krnlbuidcmd.mh
include ldrobjs.mh
.PHONY : all everything build_kernel
all: build_kernel 
#INITLDR
build_kernel:everything build_bin

everything : $(INITLDRIMH_ELF) $(INITLDRKRL_ELF) $(INITLDRSVE_ELF)

build_bin:$(INITLDRIMH) $(INITLDRKRL) $(INITLDRSVE)

# ld -s -T initldrimh.lds -n  -Map initldrimh.map -o initldrimh.elf imginithead.o inithead.o vgastr.o
$(INITLDRIMH_ELF): $(INITLDRIMH_LINK)
	$(LD) $(LDRIMHLDFLAGS) -o $@ $(INITLDRIMH_LINK)

# ld -s -T initldrkrl.lds -n  -Map initldrkrl.map -o initldrkrl.elf  ldrkrl32.o ldrkrlentry.o fs.o chkcpmm.o graph.o bstartparm.o vgastr.o
$(INITLDRKRL_ELF): $(INITLDRKRL_LINK)
	$(LD) $(LDRKRLLDFLAGS) -o $@ $(INITLDRKRL_LINK)

# ld -s -T initldrsve.lds -n  -Map initldrsve.map -o initldrsve.elf realintsve.o
$(INITLDRSVE_ELF): $(INITLDRSVE_LINK)
	$(LD) $(LDRSVELDFLAGS) -o $@ $(INITLDRSVE_LINK)

# objcopy -S -O binary initldrimh.elf initldrimh.bin 
$(INITLDRIMH):$(INITLDRIMH_ELF)
	$(OBJCOPY) $(OJCYFLAGS) $< $@
	@echo 'OBJCOPY -[M] 正在构建...' $@  

# objcopy -S -O binary initldrkrl.elf initldrkrl.bin
$(INITLDRKRL):$(INITLDRKRL_ELF)
	$(OBJCOPY) $(OJCYFLAGS) $< $@
	@echo 'OBJCOPY -[M] 正在构建...' $@ 

# objcopy -S -O binary initldrsve.elf initldrsve.bin
$(INITLDRSVE):$(INITLDRSVE_ELF)
	$(OBJCOPY) $(OJCYFLAGS) $< $@
	@echo 'OBJCOPY -[M] 正在构建...' $@ 