obj-m   := 1_ws2812b_dd.o

KDIR    := /lib/modules/$(shell uname -r)/build
PWD             := $(shell pwd)

all :
        $(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
clean :
        $(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean
~                                                                                                                                                           
~                                                                      