# VirtualBox 虚拟机安装Ubuntu14
配置内存为 4GB, 4CPU，硬盘大小20GB.桥接网络

# Ubuntu14网络设置
## 为Ubuntu14.04设置静态IP
sudo vim /etc/network/interfaces. 然后填写以下东西
```
auto eth0
iface eth0 inet static
address 192.168.0.133
gateway 192.168.0.1
netmask 255.255.255.0

dns-nameservers 8.8.8.8
```

## 为Ubuntu14.04设置固定DNS
sudo vim /etc/resolvconf/resolv.conf.d/base . 然后填写以下内容
```
nameserver 8.8.8.8
nameserver 8.8.4.4
```
修改好，然后保存
``
sudo resolvconf -u
``
## 重启服务
可以使用 `sudo reboot`重启整个系统

还可以使用 `sudo NetworkManager restart` 
```
root@ivan-VirtualBox:~# sudo NetworkManager restart
NetworkManager 已正运行(pid 930)
kill -9 930
sudo NetworkManager restart
```

或者
```
sudo /etc/init.d/networking restart 
```
# Ubuntu14 设置root用户
## 为root用户设置密码
```
Password: <--- 输入你当前用户的密码
Enter new UNIX password: <--- 新的Root用户密码
Retype new UNIX password: <--- 重复新的Root用户密码
passwd：已成功更新密码
```

## 将 /etc/passwd中的选项做修改
sudo vim /usr/share/lightdm/lightdm.conf.d/50-ubuntu.conf 填写以下内容
```
[SeatDefaults]
user-session=ubuntu
greeter-session=unity-greeter
greeter-show-manual-login=true  #手工输入登陆系统的用户名和密码
```

# Ubuntu14 设置ssh服务
## 安装SSH服务
sudo apt-get install openssh-server

## 配置ssh服务
sudo vim /etc/ssh/sshd_config
```
LoginGraceTime 120
PermitRootLogin yes
StrictModes yes
```

## 重启ssh
`sudo reboot` 或者 `sudo /etc/init.d/ssh restart`

# Ubuntu14 安装gcc9
## 首先更新包列表
sudo apt update

## 安装build-essential软件包
sudo apt install build-essential

## 安装多个GCC版本
因为个别工程需要多个GCC的编译器或者是库来支持，我们可能需要在同一个Linux系统当中安装多个GCC版本来实现支持的目的

从5.x.x到8.x.x. 最新版本的GCC是9.1.0，可从Ubuntu Toolchain PPA获得
### 添加软件源
```
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
```
如果是从来没有安装出错，则需要安装必要的依赖
```
udo apt-get install software-properties-common
```

### 安装gcc9
```
sudo apt-get upgrade
sudo apt-get install gcc-9 g++-9
```

### 切换版本
```
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 90 --slave /usr/bin/g++ g++ /usr/bin/g+±9
```

# GDB8 安装
## GDB8 下载
```
wget https://mirror.bjtu.edu.cn/gnu/gdb/gdb-8.3.tar.gz
tar -zxvf gdb-8.3.tar.gz
cd gdb-8.3
```

## 修改 gdb/remote.c 代码
sudo vim gdb/remote.c
```
/* Further sanity checks, with knowledge of the architecture.  */
// if (buf_len > 2 * rsa->sizeof_g_packet)
//   error (_("Remote 'g' packet reply is too long (expected %ld bytes, got %d "
//      "bytes): %s"),
//    rsa->sizeof_g_packet, buf_len / 2,
//    rs->buf.data ());

if (buf_len > 2 * rsa->sizeof_g_packet) {
    rsa->sizeof_g_packet = buf_len;
    for (i = 0; i < gdbarch_num_regs(gdbarch); i++) {
        if (rsa->regs[i].pnum == -1)
            continue;
        if (rsa->regs[i].offset >= rsa->sizeof_g_packet)
            rsa->regs[i].in_g_packet = 0;
        else
            rsa->regs[i].in_g_packet = 1;
    }
}
```
## 编译
```
./configure
make -j8
```

# qemu 安装
## 前提
```
apt-get install vim tmux openssh-server git -y
apt install build-essential flex bison libssl-dev libelf-dev libncurses-dev -y
```
## qemu-system-x86_64 安装
```
apt install qemu libc6-dev-i386
```

## qemu-system-x86_64 参数
### 例子
```
qemu-system-x86_64 -nographic -drive format=raw,file=hd.img -m 512M -cpu kvm64 -s -S
```

### 参数解析
#### -nographic 参数
- <作用>: 不启动图形界面，调试信息输出到终端与参数 console=ttyS0 组合使用

#### -cpu 参数
- <作用>：设置CPU模型
- <格式>：-cpu cpu select CPU ('-cpu help' for list)

#### -m 参数
<作用>：设置内存大小
<格式>：-m [size=]megs[,slots=n,maxmem=size]

#### -drive 参数
<作用>：详细定义一个存储驱动器
<格式>：-drive [file=file][,if=type][,bus=n][,unit=m][,media=d]

#### -s 参数
- <作用>: gdb监听端口

#### -S 参数
- <作用>: 表示启动后就挂起，等待 gdb 连接

# gdb + qemu调试
终端1启动 qemu命令
```
qemu-system-x86_64 -nographic -drive format=raw,file=hd.img -m 512M -cpu kvm64 -s 
```

终端2启动 gdb
```
gdb -silent

(gdb) set architecture i386:x86-64:intel                        // 设置64位Kernel
The target architecture is assumed to be i386:x86-64:intel
(gdb) symbol-file /data/webapp/icosmos/build/Cosmos.elf          //  通过symbol-file 设置符号表
Reading symbols from /data/webapp/icosmos/build/Cosmos.elf...
(gdb) target remote:1234                                      // 远程调试
Remote debugging using :1234
```

# 参考资料
- [2021年全网最细 VirtualBox 虚拟机安装 Ubuntu 20.04.2.0 LTS及Ubuntu的相关配置](https://blog.csdn.net/xw1680/article/details/115434578)
- [ubuntu14.04 网络配置](https://www.cnblogs.com/lmg-jie/p/10071282.html)
- [ubuntu14.04 root用户登录方法](https://blog.csdn.net/baidu_18660987/article/details/46931537)
- [ubuntu14.04安装SSH服务及配置](https://blog.csdn.net/q283614346/article/details/81586102)
- [Ubuntu如何安装最新版安装gcc](https://blog.csdn.net/qq_43504064/article/details/101010507)
- [ubuntu14.04 升级gcc的方法](https://blog.csdn.net/qingrenufo/article/details/78661513)
- [qemu-system-x86_64命令总结](http://blog.leanote.com/post/7wlnk13/%E5%88%9B%E5%BB%BAKVM%E8%99%9A%E6%8B%9F%E6%9C%BA)
- [使用qemu搭建内核开发环境](https://www.cnblogs.com/hellogc/p/7482066.html)
- [使用 GDB + Qemu 调试 Linux 内核](https://cloud.tencent.com/developer/article/1793157)
- [Linux 内核终于可以 debug 了！](https://www.cnblogs.com/Chary/p/15682360.html)