BBB_KERNEL := /home/hungphan/leaning/beagle-bone/kernelbuildscripts/KERNEL
TOOLCHAIN := /home/hungphan/leaning/beagle-bone/gcc-11.3.0-nolibc/arm-linux-gnueabi/bin/arm-linux-gnueabi-

EXTRA_CFLAGS=-Wall
obj-m := spi_ssd1306.o

all:
	make ARCH=arm CROSS_COMPILE=$(TOOLCHAIN) -C $(BBB_KERNEL) M=$(shell pwd) modules
	
clean:
	make -C $(BBB_KERNEL) M=$(shell pwd) clean