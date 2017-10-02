ARCH=arm
#与硬件相关，需要联调
#ARCH=x86
#$(info, $(ARCH))//内核 androidmk使用
ifeq ($(ARCH), arm)
    CC = ~/basic/cross_compile/gcc-4.6.2-glibc-2.13-linaro-multilib-2011.12/fsl-linaro-toolchain/bin/arm-fsl-linux-gnueabi-gcc
    CFLAGS = -O2  -Wall --static
else
    CC = gcc
    CFLAGS = -O2  -Wall --static
endif

INCLUDE = -I ./include 

tm1650:tm1650.c
		${CC} tm1650.c  -o $@ ${INCLUDE}  $(CFLAGS)
	   rm -rf *.o
clean:
	rm -rf *.o tm1650 
