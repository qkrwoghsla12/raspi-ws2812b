obj-m   := ws2812b_dd.o
obj   	:= ws2812b_app.c ws2812b.c
obj-out := 1_led_example.out

KDIR    := /lib/modules/$(shell uname -r)/build
PWD     := $(shell pwd)

all :
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
	gcc $(obj) -o $(obj-out)
	
clean :
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean
                            
