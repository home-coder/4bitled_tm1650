ARCH := arm
CROSS_COMPILE := /home/java/opt/jb/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-
KSRC := /home/java/opt/3501kk-hhzn/kernel_imx
#CMDLINE := "hello world"
#EXTRA_CFLAGS += -DCMDLINE=\"$(CMDLINE)\" 
#obj-m += sysfs.o mdev_test1.o proc_test1.o
#obj-m += timer.o
obj-m += mutex_sem.o
all:
	make -C $(KSRC) M=`pwd` modules
clean:
	make -C $(KSRC) M=`pwd` modules clean
