BBB_KERNEL := /home/hungphan/leaning/kernel_beagle/kernelbuildscripts/KERNEL
TOOLCHAIN := /home/hungphan/leaning/kernel_beagle/gcc-11.3.0-nolibc/arm-linux-gnueabi/bin/arm-linux-gnueabi-

EXTRA_CFLAGS=-Wall
obj-m := spi_ssd1306.o

all:
	make ARCH=arm CROSS_COMPILE=$(TOOLCHAIN) -C $(BBB_KERNEL) M=$(shell pwd) modules
	
clean:
	make -C $(BBB_KERNEL) M=$(shell pwd) clean