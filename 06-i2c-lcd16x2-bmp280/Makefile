BBB_KERNEL := /home/hungphan/leaning/kernel_beagle/kernelbuildscripts/KERNEL
TOOLCHAIN := /home/hungphan/leaning/kernel_beagle/gcc-11.3.0-nolibc/arm-linux-gnueabi/bin/arm-linux-gnueabi-

EXTRA_CFLAGS=-Wall
obj-m := lcd16x2_driver.o
obj-m := bmp280_driver.o

all:
	make ARCH=arm CROSS_COMPILE=$(TOOLCHAIN) -C $(BBB_KERNEL) M=$(shell pwd) modules
	dtc -O dtb -o i2c1_enable.dtbo -b 0 -@ i2c1_enable.dts
	arm-linux-gnueabihf-gcc -o bmp280_to_lcd bmp280_to_lcd.c
clean:
	make -C $(BBB_KERNEL) M=$(shell pwd) clean
	rm i2c1_enable.dtbo 
	rm bmp280_to_lcd
