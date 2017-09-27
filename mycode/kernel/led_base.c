#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/mutex.h>

#define TM1650_SIZE		256
#define debug()   printk("%s, %d \n", __func__, __LINE__)
struct tm1650_data {
	unsigned char data[TM1650_SIZE];
};
struct i2c_client* gclient;

/*
* 使得显示器亮起来
*/
static inline tm1650_init_display(struct i2c_client *client)
{
	struct tm1650_data *data = i2c_get_clientdata(client);
	client->addr = 0x24;
	data->data[0] = 0x31;
	i2c_master_send(client, data->data, 1);
}

/*
* 显示器显示一个7的操作，图 [_ _ _ 7]
*/
void tm1650_set_time()
{
	struct tm1650_data *data = i2c_get_clientdata(gclient);
	gclient->addr = 0x34;
	data->data[0] = 0x7;
	i2c_master_send(gclient, data->data, 1);
}
EXPORT_SYMBOL_GPL(tm1650_set_time);

static const struct i2c_device_id tm1650_i2c_id[] = { 
	{ "tm1650", 0 },
	{}, 
};
MODULE_DEVICE_TABLE(i2c, tm1650_i2c_id);

static int __devinit tm1650_i2c_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
	struct tm1650_data* data = NULL;
	data = kmalloc(sizeof(struct tm1650_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	memset(data->data, 0xff, TM1650_SIZE);
	i2c_set_clientdata(client, data);
	gclient = client;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE | I2C_FUNC_I2C))
	return -EIO;
	//TODO do some thing func
	tm1650_init_display(client);
	tm1650_set_time();

	return 0;
}

static struct i2c_driver tm1650_i2c_driver = { 
	.driver = { 
		.name = "hello tm1650",
	},
	.probe = tm1650_i2c_probe,
	//TODO remove
	.id_table = tm1650_i2c_id,
};

static __init int module_tm1650_init(void)
{
	printk("Hello World\n");
	return i2c_add_driver(&tm1650_i2c_driver);
}

static __exit void module_tm1650_exit(void)
{
	i2c_del_driver(&tm1650_i2c_driver);
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
