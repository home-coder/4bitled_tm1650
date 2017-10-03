#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

int main()
{
	int fd = open("/dev/tm1650_led", O_RDWR);
	unsigned char lev = 0x31;
	if (ioctl(fd, 0, &lev) < 0)
		return -1;
	unsigned char data = 0x7;
	if (ioctl(fd, 1, &data))
		return -1;
	return 0;
}
