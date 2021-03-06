MAKEFLAGS = -s
HEADFILE_PATH = ../include/
KRNLBOOT_PATH = ../ldrkrl/

CCBUILDPATH	= $(KRNLBOOT_PATH)
# 加载其他mh文件
include krnlbuidcmd.mh
include ldrobjs.mh

.PHONY : all everything  build_kernel
all: build_kernel 

build_kernel: everything
	
everything : $(INITLDRIMH_OBJS) $(INITLDRKRL_OBJS) $(INITLDRSVE_OBJS)

include krnlbuidrule.mh
