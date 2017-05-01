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
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/list.h>
#include <linux/rtc.h>

#define TM1650_SIZE		256
#define debug()   printk("%s, %d \n", __func__, __LINE__)
#define deinfo(msg)  printk("%s\n", msg);
#define DEV_MAJOR	0 // meaning major is not set
#define DEV_MINOR   0 //meaning set minor to be 0

enum {TM1650_DIS_LEV = 0, TM1650_SET_TIME};

unsigned char tm1650_tab[]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,\
	0x77,0x7C,0x39,0x5E,0x79,0x71};
unsigned char tm1650_addr[] = {0x37, 0x36, 0x35, 0x34};

struct tm1650_data {
	struct workqueue_struct* wqueue;
	struct work_struct work_set_time;
	struct cdev cdev;
	dev_t devno;
	struct class *tmclass;
	struct device *tmdevice;
	struct i2c_client* tmclient;
	unsigned char data[TM1650_SIZE];
	struct proc_dir_entry *tm1650_proc_dir;
	struct proc_dir_entry *tm1650_att;
	struct work_struct work_time_alarm;
};
struct i2c_client* gclient;
struct timer_list tm1650_timer;

struct proc_time_list {
	int no;
	void *data;
	struct list_head list;
};

static LIST_HEAD(time_list_head);

static inline void tm1650_init_display(struct i2c_client *client)
{
	struct tm1650_data *data = i2c_get_clientdata(client);
	printk("addr= 0x%x\n", client->addr); //0x24
	client->addr = 0x24; //for test, double addr: 0x24, 0x34....
	data->data[0] = 0x31;
	i2c_master_send(client, data->data, 1);
}

static inline void set_time_core(unsigned char addr, unsigned \
	char tab_data)
{
	struct tm1650_data *data = i2c_get_clientdata(gclient);
	gclient->addr = addr;
	data->data[0] = tab_data;

	i2c_master_send(gclient, data->data, 1);
}

//TODO. i have not use the ws, how use it ?
static void tm1650_set_time_work(struct work_struct *ws)
{
#ifdef TEST
	struct tm1650_data *data = i2c_get_clientdata(gclient);
	gclient->addr = 0x34;
	data->data[0] = 0x7;
	i2c_master_send(gclient, data->data, 1);
#endif
	struct timespec ts;
	struct rtc_time tm;
	unsigned char single_data[4];
	unsigned char addr;
	int i, j;

	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec, &tm);
	printk("%02d:%02d:%02d\n", \
		tm.tm_hour, tm.tm_min, tm.tm_sec);
	single_data[3] = tm.tm_min / 10;
	single_data[2] = tm.tm_min % 10;
	single_data[1] = tm.tm_sec / 10;
	single_data[0] = tm.tm_sec % 10;

	for (i = 0; i < 4; i++) {
		j = single_data[i];
		addr = tm1650_addr[i];
		set_time_core(addr, tm1650_tab[j]);
	}
}
EXPORT_SYMBOL_GPL(tm1650_set_time_work);

/*like a interrupt cotex, here can just operate the print_data and some \
	inner data like ts, tm
*/
void tm1650_set_time(unsigned long data)
{
	struct tm1650_data *tmdata = (struct tm1650_data *)data;
	// is ok too	
	//struct tm1650_data *tmdata = i2c_get_clientdata(gclient);
	if (!work_pending(&tmdata->work_set_time)) {
		queue_work(tmdata->wqueue, &tmdata->work_set_time);
	}

	mod_timer(&tm1650_timer, jiffies + 1* HZ);
}

static void tm1650_timer_work(void)
{
	struct tm1650_data *data = i2c_get_clientdata(gclient);

	setup_timer(&tm1650_timer, tm1650_set_time, (unsigned long)data);
	tm1650_timer.expires = jiffies + 3 * HZ;	
	add_timer(&tm1650_timer);	
}
EXPORT_SYMBOL_GPL(tm1650_timer_work);

#if 0
/* print "Hello,Alarm", and set the time to the tm1650 device */
static void alarm_work_func(unsigned long data)
{

}

/* like suspend.c, time to jiffies ! */
static void data_to_jiffies(unsigned char *data, int *pend)
{

}

/*
 TODO first method: sort the time list, and send a alarm message !
 Here  second method: make many timer nodes, setup .
*/
static void tm1650_time_alarm_work(struct work_struct *ws)
{
	struct proc_time_list *pos = NULL;
	struct timer_list tmtimer[TIME_WORK_NR];

	list_for_each_entry(pos, &time_list_head, list) {
	data_to_jiffies(pos->data, pend);
	setup_timer(&tmtimer[i], alarm_work_func, (unsigned long)" \
		Hello, Alarm!");
	tmtimer[i].expires = jiffies + pend;
	add_timer(&tmtimer[i]);
	//TODO del_timer ...
	}
	
	/*list each*/
}
#endif

/*put the start outof the probe, to be
  public by 'EXPORT_SYMBOL_GPL(tm1650_trigger_workqueue);'
  */
void tm1650_trigger_workqueue(void)
{
	//get data by client
	struct tm1650_data *data = i2c_get_clientdata(gclient);
	queue_work(data->wqueue, &data->work_set_time);
#if 0
	queue_work(data->wqueue, &data->work_time_alarm);
#endif
}
EXPORT_SYMBOL_GPL(tm1650_trigger_workqueue);

static int tm1650_open(struct inode *ino, struct file *filp)
{
	struct tm1650_data *data = container_of(ino->i_cdev, \
		struct tm1650_data, cdev);
	filp->private_data = data;

	return 0;
}

static loff_t tm1650_llseek(struct file *filp, loff_t offset, int orig)
{
	loff_t ret = 0;
	
	return ret;
}

static ssize_t tm1650_write(struct file *filp, const char __user *buf, size_t 
count, loff_t *offset)
{
	int ret;
	struct tm1650_data *data = filp->private_data;
	struct i2c_client *client = data->tmclient;	
	
	if (count != 2) //address + data
		return -EIO;
	client->addr = buf[0];
	printk("addr: 0x%x, data: 0x%x\n", buf[0], buf[1]);
	ret = min((int)count, TM1650_SIZE - 1);
	if (copy_from_user(data->data, buf + 1, ret))
		return -EFAULT;
	/*send i2c*/
	i2c_master_send(client, data->data, ret - 1);

	return ret;
}

/*TODO cmd1:show level
  cmd2:show what
*/
static long tm1650_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned char tmdata;
	struct tm1650_data *data = filp->private_data;
	struct i2c_client *client = data->tmclient;
	/*if userspace like 0x33, use follow*/
	//tmdata = arg;
	/*if userspace like &buf, use follow	like the write() func param: buf*/
	get_user(tmdata, (unsigned char __user*)arg);
	printk("tmdata is 0x%x\n", tmdata);
	
	switch (cmd) {
		case TM1650_DIS_LEV:
			client->addr = 0x24;
			break;
		case TM1650_SET_TIME:
			client->addr = 0x34;
			break;
		default:
			return -1;
	}
	data->data[0] = tmdata;
	i2c_master_send(client, data->data, 1);
	
	return 0;
}

static int tm1650_release(struct inode *ino, struct file *filp)
{

	return 0;
}

static const struct file_operations tm1650_ops = {
	.open = tm1650_open,
	//TODO
	.llseek = tm1650_llseek,
	.write = tm1650_write,
	.unlocked_ioctl = tm1650_ioctl,
	//TODO
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
	struct i2c_client *client = data->tmclient;
	
	data->devno = MKDEV(dev_major, dev_minor);
	if(dev_major) {
		ret = register_chrdev_region(data->devno, 1, dev_name(&client->dev));	
	} else {
		ret = alloc_chrdev_region(&data->devno, dev_minor, 1, dev_name(&client->dev));
	}
	deinfo(dev_name(&client->dev));
	if (ret < 0)
		return ret;
	cdev_init(&data->cdev, &tm1650_ops);
	ret = cdev_add(&data->cdev, data->devno, 1);
	if (ret)
		deinfo("error add a char dev");
	//TODO how to get other module`s class, like make the i2c-usr to /sys/class/i2c-dev	
	data->tmclass = class_create(THIS_MODULE, "i2c-usr");
	data->tmdevice = device_create(data->tmclass, NULL, data->devno, NULL, dev_name(&client->dev));
	
	return 0;
}
/*
===========================================================================
不是，open是vfs的标准API，所以传进来的inode不是proc_inode，而是\
&proc_inodevfs_inode。这里把标准的inode结构，嵌入在一个更大的proc_inode\
结构里面，有了里面node指针，通过偏移（参见container_of），\
就能找到外层的proc_inode。
   
static inline struct proc_inode *PROC_I(const struct inode *inode)
{
	return container_of(inode, struct proc_inode, vfs_inode);
	
}
有了proc_inode，就能找到proc_dir_entry，这个不是内嵌结构，只是一个普通的指针而 \

static inline struct proc_dir_entry *PDE(const struct inode *inode)
{
	return PROC_I(inode)->pde;
}
有了proc_dir_entry，就能访问私有数据pde->data 
==========================================================================
*/


/*TODO cat time list like:8:29, 11:33, 12:48
 Q&A: why not use the printk only ?
*/
static int tm1650_proc_show(struct seq_file *seq, void *v)
{
	struct i2c_client *client = NULL;
	struct proc_time_list *pos = NULL;	
	struct tm1650_data *data = seq->private;
	if (!data) {
		deinfo("show err");
		return -1;
	}
	client = data->tmclient;
	
//	seq_printf(seq, "devname: %s\n\n", dev_name(&client->dev));
	printk("time no\t\ttime content\n");
	list_for_each_entry(pos, &time_list_head, list) {
		seq_printf(seq, "%d\t\t%s", pos->no, (char *)pos->data);
	}

	return 0;
}

static int tm1650_proc_open(struct inode *inode, struct file *file)
{
	/*if use the private data, please use PDE_DATA()*/
	return single_open(file, tm1650_proc_show, PDE(inode)->data);
}

/* TODO set time in a list adpter like 08:29 , 11:33, 12:48
   Q&A: How long the param@buf ?
   Note: if return 0; will try and try to write...
*/
static ssize_t tm1650_proc_write(struct file *filp, const char __user*buf,\
	size_t count, loff_t *pos)
{
	int ret = 0;
//	unsigned int l = strlen(buf) + 1;
	unsigned int l = count;
	struct proc_time_list *new_time = NULL;
	char *tmcontent = NULL;
	static int no = 0;
	new_time = kzalloc(sizeof(struct proc_time_list), GFP_KERNEL);
	if (!new_time)
		return -ENOMEM;
	
	tmcontent = kzalloc(l, GFP_KERNEL);
	if (!tmcontent) {
		ret = -ENOMEM;
		goto alloc_tmcontent_err;
	}	
	
	ret = copy_from_user(tmcontent, (char __user*)buf, l);
	if (ret) {
		ret = -ENOMEM;
		goto copy_err;
	}

	new_time->no = no++;
	new_time->data = tmcontent;

	list_add_tail(&new_time->list, &time_list_head);
	
	return l;
copy_err:
	kfree(tmcontent);
alloc_tmcontent_err:
	kfree(new_time);
	return ret;
}

static const struct file_operations tm1650_proc_ops = {
	.open  = tm1650_proc_open,
	//read can`t do, use open for the userspace ops like "cat /proc/version"
	.read  = seq_read,
	.write = tm1650_proc_write,
	//TODO.release
};

static int tm1650_create_procfs(struct tm1650_data *data)
{
	int ret;

	data->tm1650_proc_dir = proc_mkdir("tm1650_proc", NULL);
	if (!data->tm1650_proc_dir) {
		ret = -ENOMEM;
		goto mkdir_err;
	}
	data->tm1650_att = proc_create_data("tm1650_att", S_IRUGO | S_IWUSR, \
		data->tm1650_proc_dir, &tm1650_proc_ops, (void *)data);
	if (!data->tm1650_att) {
		ret = -ENOMEM;
		goto creat_att_err;
	}

	return 0;
creat_att_err:
	//another func is ?
	remove_proc_entry("tm1650_proc", NULL);
mkdir_err:
	return ret;
}

static int __devinit tm1650_i2c_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
	int ret;
	struct tm1650_data* data = NULL;
	
	deinfo("tm1650_i2c_probe");
	data = kmalloc(sizeof(struct tm1650_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	//TODO here is 0x00 or 0xff, which better ?	
	memset(data->data, 0xff, TM1650_SIZE);
	i2c_set_clientdata(client, data);
	data->tmclient = kmalloc(sizeof(struct i2c_client), GFP_KERNEL);
	if (!data->tmclient) {
		ret = -ENOMEM;
		goto tmclient_err;
	}	
	data->tmclient = client;
	dev_set_name(&client->dev, "tm1650_led");
	/* this just for the function like EXPORT_SYMBOL_GPL, who has no param */
	gclient = client;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE | \
		I2C_FUNC_I2C))
	return -EIO;
	//TODO TODO do some thing func like 1, 2, 3, 4....
	tm1650_init_display(client);
	//1.base: 
	//-->tm1650_set_time_work();
	
	//2.workqueue: 
	INIT_WORK(&data->work_set_time, tm1650_set_time_work);
	data->wqueue = create_singlethread_workqueue(dev_name(&(client->dev)));
	//-->tm1650_trigger_workqueue();

	//3.chardev, use class,device 
	ret = tm1650_setup_cdev(data);
	if(ret < 0)
		deinfo("tm1650_setup_cdev error");
	//4.fillops, like open, read, write
	//-->userspace test 
	
	//5.proc filesystem, and jiffies
	ret = tm1650_create_procfs(data);
	tm1650_timer_work();
	
	//6.alarm
	#if 0
	INIT_WORK(&data->work_time_alarm, tm1650_time_alarm_work);
	tm1650_alarm_work();
	#endif
	//6.seq_file interface, is a abstract obj
	//TODO

	//7.sysfs
	
	//8.lock, where should use a lock ?
	
	//6.uevent, use struct list to do it

	//7.poll,fantasy

	//8.irq, use button irq and i2c irq


	//10.
	return 0;

tmclient_err:
	kfree(data);
	return ret;
}

static int tm1650_i2c_remove(struct i2c_client* client)
{
	struct tm1650_data* data = i2c_get_clientdata(client);
	
	debug();
	flush_workqueue(data->wqueue);
	destroy_workqueue(data->wqueue);
	
	/*if there is no follow step, the kernel will dump*/
	device_destroy(data->tmclass, data->devno);
	class_destroy(data->tmclass);
	cdev_del(&data->cdev);
	unregister_chrdev_region(data->devno, 1);
	remove_proc_entry("tm1650_att", data->tm1650_proc_dir);
	remove_proc_entry("tm1650_proc", NULL);

	kfree(data);
	del_timer(&tm1650_timer);

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
