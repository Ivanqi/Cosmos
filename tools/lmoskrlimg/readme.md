# 使用案例
lmoskrlimg -m k -lhf GRUB头文件 -o 映像文件 -f 输入的文件列表
```
-m 表示模式 只能是k内核模式
-lhf 表示后面跟上GRUB头文件
-o 表示输出的映像文件名 
-f 表示输入文件列表

例如：lmoskrlimg -m k -lhf grubhead.bin -o kernel.img -f file1.bin file2.bin file3.bin file4.bin
```