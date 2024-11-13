KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

obj-m += chrdev_kernel.o

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=x86_64 modules

clean:
	rm -rf *.o *.ko *.mod.* .*.cmd *.symvers .tmp_versions modules.order

