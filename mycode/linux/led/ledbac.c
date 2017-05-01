#include <stdio.h>
#include <linux/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>

#define I2C_RETRIES 0x0701	/* number of times a device address should
				   be polled when not acknowledging */
#define I2C_TIMEOUT 0x0702	/* set timeout in units of 10 ms */
/* NOTE: Slave address is 7 or 10 bits, but 10-bit addresses
 * are NOT supported! (due to code brokenness)
 */
#define I2C_SLAVE   0x0703	/* Use this slave address */
#define I2C_SLAVE_FORCE 0x0706	/* Use this slave address, even if it
				   is already in use by a driver! */
#define I2C_TENBIT  0x0704	/* 0 for 7 bit addrs, != 0 for 10 bit */
#define I2C_FUNCS   0x0705	/* Get the adapter functionality mask */
#define I2C_RDWR    0x0707	/* Combined R/W transfer (one STOP only) */
#define I2C_PEC     0x0708	/* != 0 to use PEC with SMBus */
#define I2C_SMBUS   0x0720	/* SMBus transfer */
#define I2C_M_TEN 0x0010
#define I2C_M_RD 0x0001

#define I2C_ADAPTER_PATH  "/dev/i2c-1"

struct i2c_msg {
	unsigned short addr;
	unsigned short flags;
	unsigned short len;
	unsigned char *buf;
};

struct i2c_rdwr_ioctl_data {
	struct i2c_msg *msgs;
	int nmsgs;
};

static int tm1650_open()
{
	int fd;
	fd = open(I2C_ADAPTER_PATH, O_RDWR);
	if (fd < 0) {
		perror("open error");
		return -1;
	}

	return fd;
}

static int tm1650_config(int fd)
{
	if(ioctl(fd, I2C_TIMEOUT, 1) < 0) {
		perror("ioctl:set timeout failed");
		return -1;
	}
	
	if(ioctl(fd, I2C_RETRIES, 2) < 0) {
		perror("ioctl:set retry failed");
		return -1;
	}
	
	if(ioctl(fd, I2C_TENBIT, 0) < 0) {
		perror("ioctl:set addr:7bit failed");
		return -1;
	}

	if (ioctl(fd,I2C_SLAVE_FORCE, 0x24) < 0) {
		perror("ioctl:set slave address failed");
		return -1;
	}
	
	return 1;
}

static int tm1650_init_display(int fd, struct i2c_rdwr_ioctl_data *tm1650_data)
{
	int ret;

	tm1650_data->nmsgs = 1;
	(tm1650_data->msgs[0]).len = 1;
	(tm1650_data->msgs[0]).addr = 0x24;
	(tm1650_data->msgs[0]).flags = 0;
	(tm1650_data->msgs[0]).buf = (unsigned char *)malloc(1);
	(tm1650_data->msgs[0]).buf[0] = 0x31;
	
	ret = ioctl(fd, I2C_RDWR, (unsigned long)tm1650_data);
	if (ret < 0) {
		perror("ioctl write display error");
		return -1;
	}

	return 1;
}

static void tm1650_set_time(int fd, struct i2c_rdwr_ioctl_data* tm1650_data)
{
	/*get system time*/

	/*parse the time*/

	/*config the i2c packege*/

	/*send the i2c data*/

	//now just set a 7 to tm1650, for test !
	int ret;
	tm1650_data->nmsgs = 1;
	(tm1650_data->msgs[0]).len = 1;
	(tm1650_data->msgs[0]).addr = 0x34;
	(tm1650_data->msgs[0]).flags = 0;
	(tm1650_data->msgs[0]).buf = (unsigned char *)malloc(1);
	(tm1650_data->msgs[0]).buf[0] = 0x7;
	ret = ioctl(fd, I2C_RDWR, (unsigned long)tm1650_data);
	if (ret < 0) {
		perror("ioctl write time error");
	}
}

int main()
{
	int tm1650_fd, ret;
	struct i2c_rdwr_ioctl_data *tm1650_data;

	tm1650_fd = tm1650_open();
	ret = tm1650_config(tm1650_fd);
	if (ret < 0) {
		return -1;
	}

	tm1650_data = (struct i2c_rdwr_ioctl_data* )malloc(sizeof(struct i2c_rdwr_ioctl_data));
	tm1650_data->msgs = (struct i2c_msg *)malloc(tm1650_data->nmsgs * sizeof(struct i2c_msg));
	ret = tm1650_init_display(tm1650_fd, tm1650_data);
	if (ret < 0) {
		return -1;
	}

	/*function1 set system time to tm1650*/
	tm1650_set_time(tm1650_fd, tm1650_data);
	
	free(tm1650_data->msgs);
	free(tm1650_data);
	close(tm1650_fd);
	return 0;
}
