# obj-m += simple_driver.o
# all:
# 	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
# clean:
# 	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

obj-m += simple_driver.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	$(CROSS_COMPILE)gcc -o simple_driver_test simple_driver_test.c
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules clean
	rm -rf modules.order
	rm -r simple_driver_test

