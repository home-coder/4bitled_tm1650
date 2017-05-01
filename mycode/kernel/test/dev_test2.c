#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main()
{
	int fd = open("/dev/tm1650_led", O_RDWR);
	unsigned char buf[2] = {0x24, 0x31};
	unsigned char show[2] = {0x34, 0x07};
//	write(fd, buf, 2);
//	write(fd, show, 2);

	unsigned char lev = 0x31;
	if (ioctl(fd, 0, &lev) < 0)
		return -1;
	unsigned char data = 0x7;
	if (ioctl(fd, 1, &data))
		return -1;
	return 0;
}
