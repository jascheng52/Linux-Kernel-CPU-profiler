obj-m := perftop.o
# obj-m += lkp2.o # add multiple files if necessary
CONFIG_MODULE_SIG=n
KDIR := /home/user/linux
# KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
all: perftop.c # add lkp2.c if necessary
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean

install:
	make -C $(KDIR) M=$(PWD) modules
	insmod perftop.ko

uninstall:
	rmmod perftop.ko

