# GRUB 的概念
GUN GRUB (GRand Unified Bootloader 简称 "GRUB") 是一个来自GNU项目的多操作系统启动程序

GRUB是多启动规范的实现，它允许用户可以在计算机内同时拥有多个操作系统，并在计算机启动时选择希望运行的操作系统

GRUB可用于操作系统分区上的不同内核，也可用于向这些内核传递启动参数

# GRUB加载内核的过程
## GRUB 的作用
- 加载操作系统的内核
- 拥有一个可以让用户选择的的菜单，来选择到底启动哪个系统
- 可以调用其他的启动引导程序，来实现多系统引导

## GRUB执行过程
### 概述
按照启动流程，BIOS在自检完成后，会到第一个启动设备的MBR中读取GRUB

在MBR中用来放置启动引导程序的空间只有446Byte，那么GRUB可以放到这里吗？答案是空间不够，GRUB的功能非常强大，MBRM空间是不够用的

那么Linux的解决办法是把GRUB的程序分成了三个阶段来执行

### Stage1: 执行GRUB主程序
第一阶段是用来执行GRUB主程序的，这个主程序必须放在启动区中(也就是MBR或引导扇区中)

但是MBR太小了，所以只能安装GRUB的最小程序，而不能安装GRUB的相关配置文件

这个主程序主要是用来启动Stage1.5 和 Stage2的

### Stage 1.5: 识别不同的文件系统
State2 比较大，只能放在文件系统中(分区),但是State1 不能识别不同的文件系统，所以不能直接加载State2

这时需要先加载Stage1.5，由Stage1.5来加载不同文件系统中的State2

还有一个问题，难道 Stage 1.5 不是放在文件系统中的吗？
- 如果是，那么 Stage 1 同样不能找到 Stage 1.5
- 其实，Stage 1.5 还真没有放在文件系统中，而是在安装 GRUB 时，直接安装到紧跟 MBR 之后的 32KB 的空间中，这段硬盘空间是空白无用的，而且是没有文件系统的，所以 Stage 1 可以直接读取 Stage 1.5
- 读取了 Stage 1.5 就能识别不同的文件系统，才能加载 Stage 2

### 加载GRUB的配置文件
Stage 2 阶段主要就是加载 GRUB 的配置文件 /boot/grub/grub.conf，然后根据配置文件中的定义，加载内核和虚拟文件系统

接下来内核就可以接管启动过程，继续自检与加载硬件模块了

# GRUB 安装
```
第一步挂载虚拟硬盘文件为loop0回环设备
sudo losetup /dev/loop0 hd.img
sudo mount -o loop ./hd.img ./hdisk/ ;挂载硬盘文件
sudo mkdir ./hdisk/boot/ ;建立boot目录
第二步安装GRUB
sudo grub-install --boot-directory=./hdisk/boot/ --force --allow-floppy /dev/loop0
；--boot-directory 指向先前我们在虚拟硬盘中建立的boot目录。
；--force --allow-floppy ：指向我们的虚拟硬盘设备文件/dev/loop0
```
这里是把GRUB安装到虚拟硬盘上，通过一个Liunx系统，通过GRUB的安装程序，把GRUB安装到指定的设备上(虚拟硬盘)

# GRUB 配置文件
在 /hdisk/boot/grub/ 目录下建立一个 grub.cfg 文本文件，然后写入如下内容
```
menuentry 'HelloOS' {
insmod part_msdos
insmod ext2
set root='hd0,msdos1' #我们的硬盘只有一个分区所以是'hd0,msdos1'
multiboot2 /boot/HelloOS.eki #加载boot目录下的HelloOS.eki文件
boot #引导启动
}
set timeout_style=menu
if [ "${timeout}" = 0 ]; then
  set timeout=10 #等待10秒钟自动启动
fi
```
文件系统为 ext2

硬盘只有一个分区所以是'hd0, msdos1'. 通过mrb的分区设置grub的启动程序


# 参考资料
- [GRUB——系统的引导程序简单介绍](https://www.cnblogs.com/yinheyi/p/7279508.html)
- [Linux系统启动管理](http://c.biancheng.net/linux_tutorial/12/)
- [GRUB (简体中文)](https://wiki.archlinux.org/title/GRUB_(%E7%AE%80%E4%BD%93%E4%B8%AD%E6%96%87))