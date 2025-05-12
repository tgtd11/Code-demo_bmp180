ifneq ($(KERNELRELEASE),)
# Khi duoc goi tu kernel
obj-m := bmp180_driver.o bmp180_ioctl.o

else
# Khi goi tu user (terminal)

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules
	gcc -o test_bmp test_bmp.c

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f test_bmp

endif
