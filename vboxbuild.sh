# for macOS
sudo rm -rf /Users/lvan/VirtualBox\ VMs/hd.img
sudo cp hd.img /Users/lvan/VirtualBox\ VMs
cd /Users/lvan/VirtualBox\ VMs
sudo rm -rf hd.vdi
VBoxManage convertfromraw ./hd.img --format VDI ./hd.vdi
VBoxManage internalcommands sethduuid  "./hd.vdi"  "f602acba-4fec-4600-8fff-4fa7d6ac04ae"