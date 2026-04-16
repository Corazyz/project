obj-m += chr_drv.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
		$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
		$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
install:
		sudo insmod chr_drv.ko
remove:
		sudo rmmod chr_drv

.PHONY: all clean insall remove