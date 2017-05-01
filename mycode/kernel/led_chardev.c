#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/fs.h>

#define TM1650_SIZE		256
#define debug()   printk("%s, %d \n", __func__, __LINE__)
#define deinfo(msg)  printk("%s\n", msg);
#define DEV_MAJOR	0 // meaning major is not set
#define DEV_MINOR   0 //meaning set minor to be 0
struct tm1650_data {
	struct workqueue_struct* wqueue;
	struct work_struct work_set_time;
	struct cdev cdev;
	dev_t devno;
	struct class *tmclass;
	struct device *tmdevice;
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

/*put the start outof the probe, to be
  public by 'EXPORT_SYMBOL_GPL(tm1650_trigger_workqueue);'
  */
void tm1650_trigger_workqueue()
{
	//get data by client
	struct tm1650_data *data = i2c_get_clientdata(gclient);
	queue_work(data->wqueue, &(data->work_set_time));
}
EXPORT_SYMBOL_GPL(tm1650_trigger_workqueue);

int tm1650_open(struct inode *ino, struct file *filp)
{
	
	return 0;
}

loff_t tm1650_llseek(struct file *filp, loff_t offset, int orig)
{
	loff_t ret;
	
	return ret;
}

ssize_t tm1650_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset)
{

	return 0;
}

long tm1650_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	
	return 0;
}

int tm1650_release(struct inode *ino, struct file *filp)
{

	return 0;
}

struct file_operations tm1650_ops = {
	.open = tm1650_open,
	.llseek = tm1650_llseek,
	.write = tm1650_write,
	.unlocked_ioctl = tm1650_ioctl,
	.release = tm1650_release,
};

/*static mode is register_chrdev_region()
  dynomic mode is alloc_chrdev_region()
*/
static int tm1650_setup_cdev(struct tm1650_data* data)
{
	int ret;
	static int dev_major = DEV_MAJOR;
	static int dev_minor = DEV_MINOR;
	data->devno = MKDEV(dev_major, dev_minor);
	if(dev_major) {
		ret = register_chrdev_region(data->devno, 1, dev_name(&gclient->dev));	
	} else {
		ret = alloc_chrdev_region(&data->devno, dev_minor, 1, dev_name(&gclient->dev));
	}
	deinfo(dev_name(&gclient->dev));
	if (ret < 0)
		return ret;
	cdev_init(&data->cdev, &tm1650_ops);
	ret = cdev_add(&data->cdev, data->devno, 1);
	if (ret)
		deinfo("error add a char dev");
	data->tmclass = class_create(THIS_MODULE, dev_name(&gclient->dev));
	data->tmdevice = device_create(data->tmclass, NULL, data->devno, NULL, dev_name(&gclient->dev));
	
	return 0;
}

static int __devinit tm1650_i2c_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
	int ret;
	struct tm1650_data* data = NULL;
	data = kmalloc(sizeof(struct tm1650_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	memset(data->data, 0xff, TM1650_SIZE);
	i2c_set_clientdata(client, data);
	dev_set_name(&client->dev, "tm1650_led");
	gclient = client;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE | I2C_FUNC_I2C))
	return -EIO;
	//TODO do some thing func like 1, 2, 3, 4....
	tm1650_init_display(client);
	//1.base: 
	//tm1650_set_time();
	
	INIT_WORK(&(data->work_set_time), tm1650_set_time_work);
	data->wqueue = create_singlethread_workqueue(dev_name(&(client->dev)));
	//2.workqueue: 
	//tm1650_trigger_workqueue();

	//3.chardev
	ret = tm1650_setup_cdev(data);
	if(ret < 0)
		deinfo("tm1650_setup_cdev error");

	return 0;
}

static int tm1650_i2c_remove(struct i2c_client* client)
{
	debug();
	struct tm1650_data* data = i2c_get_clientdata(client);
	flush_workqueue(data->wqueue);
	destroy_workqueue(data->wqueue);
	
	/*if there is no follow step, the kernel will dump*/
	device_destroy(data->tmclass, data->devno);
	class_destroy(data->tmclass);
	cdev_del(&data->cdev);
	unregister_chrdev_region(data->devno, 1);

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
		.name = "hello_tm1650",
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
