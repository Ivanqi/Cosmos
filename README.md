# Cosmos
简单操作系统

- 代码来源
  - 《操作系统实战45讲》-作者: 彭东

# 目录结构
```
├── Makefile                  // Makefile 文件
├── Makefile.x86              // x86 Makefile 文件
├── README.md
├── build                    // 工程构建目录
│   ├── lmkfbuild
│   ├── pretreatment.mkf
│   └── vbox.mkf             // 虚拟机脚本
├── drivers                  // 设备和驱动
│   ├── drvrfs.c
│   ├── drvtick.c
│   ├── drvuart.c
│   └── net                 // 网络设备
├── exckrnl
├── hal                     // 硬件抽象层
│   └── x86                 // x86架构
├── hd.img
├── hdisk
├── include                 // 各部件头文件
│   ├── bastypeinc
│   ├── drvinc
│   ├── halinc
│   ├── knlinc
│   ├── libinc
│   └── script
│       └── buildfile.h
├── initldr               // 启动目录
│   ├── Makefile
│   ├── build
│   ├── include
│   └── ldrkrl
├── kernel                // 内核目录
├── lib                   // 服务接口目录
├── release
├── script                // 工程构建脚步
├── tools
```

# 最终效果图
![avatar](./images/os_1.png)