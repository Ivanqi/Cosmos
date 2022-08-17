# for macOS
sudo cp hd.img /Users/lvan/VirtualBox\ VMs
cd /Users/lvan/VirtualBox\ VMs
sudo rm -rf hd.vdi
VBoxManage convertfromraw ./hd.img --format VDI ./hd.vdi
VBoxManage internalcommands sethduuid  "./hd.vdi"  "5e0eb114-ebc0-47e0-b782-15be479c8a24"