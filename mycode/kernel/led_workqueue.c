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
	struct workqueue_struct* wqueue;
	struct work_struct work_set_time;
	unsigned char data[TM1650_SIZE];
};
struct i2c_client* gclient;

static inline tm1650_init_display(struct i2c_client *client)
{
	struct tm1650_data *data = i2c_get_clientdata(client);
	printk("addr= 0x%x\n", client->addr); //0x24
	client->addr = 0x24; //for test, double addr: 0x24, 0x34....
	data->data[0] = 0x31;
	i2c_master_send(client, data->data, 1);
}

void tm1650_set_time()
{
	struct tm1650_data *data = i2c_get_clientdata(gclient);
	gclient->addr = 0x34;
	data->data[0] = 0x7;
	i2c_master_send(gclient, data->data, 1);
}
EXPORT_SYMBOL_GPL(tm1650_set_time);

static void tm1650_set_time_work()
{
	tm1650_set_time();
}

void trigger_workqueue()
{
	//get data by client
	struct tm1650_data *data = i2c_get_clientdata(gclient);
	queue_work(data->wqueue, &(data->work_set_time));
}
EXPORT_SYMBOL_GPL(trigger_workqueue);

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
	//tm1650_set_time();
	
	INIT_WORK(&(data->work_set_time), tm1650_set_time_work);
	data->wqueue = create_singlethread_workqueue(dev_name(&(client->dev)));
	/*put the start outof the probe, to be
	public by 'EXPORT_SYMBOL_GPL(trigger_workqueue);'*/
	//trigger_workqueue();
	
	return 0;
}

static int tm1650_i2c_remove(struct i2c_client* client)
{
	debug();
	struct tm1650_data* data = i2c_get_clientdata(client);
	flush_workqueue(data->wqueue);
	destroy_workqueue(data->wqueue);
	kfree(data);

	return 0;
}

static const struct i2c_device_id tm1650_i2c_id[] = { 
	{ "tm1650", 0 },
	{}, 
};
MODULE_DEVICE_TABLE(i2c, tm1650_i2c_id);

static struct i2c_driver tm1650_i2c_driver = { 
	.driver = { 
		.name = "hello tm1650",
	},
	.probe = tm1650_i2c_probe,
	.remove = tm1650_i2c_remove,
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
