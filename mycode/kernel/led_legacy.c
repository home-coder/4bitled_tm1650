#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>

#define debug()   printk("%s, %d \n", __func__, __LINE__)

static inline void tm1650_init_display(void)
{
	struct i2c_client client;
	unsigned char data;

	client.addr = 0x24;
	data = 0x31;
	debug();
	//i2c_master_send(&client, &data, 1);
}

static inline void tm1650_set_time(void)
{
	struct i2c_client client;
	unsigned char data;
	
	client.addr = 0x34;
	data = 0x7;
	//i2c_master_send(&client, &data, 1);
}
EXPORT_SYMBOL_GPL(tm1650_set_time);

static __init int module_tm1650_init(void)
{
	printk("Hello World\n");
	tm1650_init_display();
	tm1650_set_time();
		
	return 0;
}

static __exit void module_tm1650_exit(void)
{
	printk("Byebye\n");
}

//insmod xxx.ko
module_init(module_tm1650_init);
//rmmod xxx.ko
module_exit(module_tm1650_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Norco"); 
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("tm1650 i2c led");
