sudo dmesg -C
sudo insmod chr_drv.ko irqLine=5
sudo mknod /dev/chrdevbase1 c 511 1
sudo mknod /dev/chrdevbase0 c 511 0
sudo chmod 666 /dev/chrdevbase*
./test /dev/chrdevbase0

ls -l /dev/chrdevbase0
ls -l /dev/chrdevbase1
cat /proc/devices | grep chr

dmesg | tail -5
sudo rmmod chr_drv
sudo rm /dev/chrdevbase*
