export ARCH=arm
export CROSS_COMPILE=/home/java/basic/cross_compile/gcc-4.6.2-glibc-2.13-linaro-multilib-2011.12/fsl-linaro-toolchain/bin/arm-fsl-linux-gnueabi-
LINUX_PATH :=/home/java/opt/hhzn/kernel_imx

##### TM1650  FUNCTION #######
#####     MACRO       #######

EXTRA_CFLAGS += -DBASE_TEST \
	-DSINGLE_WORK_QUEUE=0 \
	-DCHR_DEV=1 \
	-DUSR_SPACE=0 \
	-DCREAT_PROC=0 \
	-DPROC_ALARM=0 \
	-DTIMER_WORK=0 \
	-DSWITCH_TIMER=0 \
	-DMAKEFILE_MACRO=0 \
	-DMUTEX_THREAD=0 \
	-DNOMUTEX_THREAD=0 \
	-DSYNC_THREAD=0 \
	-DNOSYNC_THREAD=0 \
	-DSYNC_INTERRUPT=0 \
	-DNOSYNC_INTERRUPT=0 \
	-DSYNC_ACTION=0 \
	-DTM1650_UEVENT=0 \
	-DTM1650_POLL=0 \
	-DTM1650_SYSFS=0 \
	-DTM1650_LOOP_BUF \
	-DTM1650_LIST=0

obj-m += led.o
#obj-m += led_timer.o
obj-m += mutex_sem.o

all:
	make -C $(LINUX_PATH) M=`pwd` modules
clean:
	make -C $(LINUX_PATH) M=`pwd` modules clean
#	rm -f *.ko *.o *.mod.o *.mod.c *.symvers *.order *.o.cmd *.swp
