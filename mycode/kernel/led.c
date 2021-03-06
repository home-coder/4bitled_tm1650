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
#include <asm/gpio.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/string.h>

#define TM_NAME         32
#define TM1650_SIZE		256
#define debug()			printk("%s, %d \n", __func__, __LINE__)
#define deerro(errmsg)  printk("%s: %s, %d \n", errmsg, __func__, __LINE__)
#define deinfo(msg)     printk("%s\n", msg);
#define DEV_MAJOR    	0 // meaning major is not set
#define DEV_MINOR       0 //meaning set minor to be 0
#define TM1650_SWITCH   IMX_GPIO_NR(5, 18)
#define TM1650_SET_1111 IMX_GPIO_NR(5, 19)
#define TIMER_LIST		32

#define TM1650_IOC_MAGIC 'm' //定义类型
#define TM1650_DIS_LEV _IOW(TM1650_IOC_MAGIC,0,int)
#define TM1650_SET_DATA _IOW(TM1650_IOC_MAGIC, 1, int)
#define TM1650_DIS_FLICK _IOW(TM1650_IOC_MAGIC, 2, int)

enum {GET_YEAR = 0, GET_DAY, GET_SECOND, GET_BACK};
static int flag;

unsigned char tm1650_tab[]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,\
	0x77,0x7C,0x39,0x5E,0x79,0x71};
unsigned char tm1650_addr[] = {0x37, 0x36, 0x35, 0x34};

struct tm1650_data {
	char   name[TM_NAME];
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
	struct work_struct work_switch_time;
	struct workqueue_struct *wqueue_on_cpu0;
	struct mutex tm1650_mutex;
	struct work_struct work_nomutex_8888;
	struct semaphore sem;
	spinlock_t lock;
	struct work_struct request_proc_work;
	void *alarm_data;
	struct work_struct work_uevent;
};

struct proc_time_list {
	int no;
	void *alarm_data;
	struct list_head list;
	struct timer_list *timer_data;
};

struct i2c_client* gclient;
struct timer_list tm1650_timer, tm1650_switch_timer;

static LIST_HEAD(alarm_timer_list);

static inline void tm1650_init_display(struct i2c_client *client)
{
	struct tm1650_data *data = i2c_get_clientdata(client);
	
	printk("addr= 0x%x\n", client->addr); //0x24
	client->addr = 0x24; //for test, double addr: 0x24, 0x34....
	data->data[0] = 0x31;
	i2c_master_send(client, data->data, 1);
}

static inline void set_data_core(unsigned char *tm1650_addr, unsigned \
	char *single_data)
{
	int i = 0, j = 0;
	struct tm1650_data *data = i2c_get_clientdata(gclient);
	
	for (i = 0; i < 4; i++) {
		j = single_data[i];
		gclient->addr = tm1650_addr[i];
		data->data[0] = tm1650_tab[j];
		i2c_master_send(gclient, data->data, 1);
	}
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

	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec, &tm);
	printk("%02d:%02d:%02d\n", \
		tm.tm_hour, tm.tm_min, tm.tm_sec); //hour + 8
	single_data[3] = tm.tm_min / 10;
	single_data[2] = tm.tm_min % 10;
	single_data[1] = tm.tm_sec / 10;
	single_data[0] = tm.tm_sec % 10;
	
	set_data_core(tm1650_addr, single_data);
	flag = GET_BACK;
}

/*like a interrupt cotex, here can just operate the print_data and some \
	inner data like ts, tm
*/
void tm1650_set_time(unsigned long data)
{
	struct tm1650_data *tmdata = (struct tm1650_data *)data;
	
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

/* this function just support minutes and seconds. Not yet Hour ! 
*  like suspend.c, time to jiffies ! 
*  http://blog.chinaunix.net/uid-790245-id-2037312.html
*  Now, i don`t use a rtc ops
*/
static void data_to_jiffies(const char *data, const char *delim, \
				unsigned long *pend)
{
#define ASSERT_ARGS(a)
	char *token;
	int len = 0;
	const char *tmp = data;
	len = strlen(tmp) + 1;
	char buf[len];
	char *pbuf = buf;
	struct timespec ts;
	struct rtc_time tm;
	unsigned long set_time, now_time, alarm_time;
	unsigned long set_data[2]; // min:second
	int i = 0;
	
	memcpy(pbuf, tmp, len);
	/* Establish string and get the first token: */
	for(token = strsep(&pbuf, delim); token != NULL; \
			token = strsep(&pbuf, delim), i++) {  
		/* str to int, gettime, to jiffies */
		if (token != NULL) {
			set_data[i] = simple_strtoul(token, NULL, 10);
		} else {
			set_data[i] = 0;
		}
	}
	
	set_time = set_data[0] * 60 + set_data[1];
	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec, &tm);

	now_time = tm.tm_min * 60 + tm.tm_sec;
	alarm_time = set_time - now_time;
	if (alarm_time < 0) {
		*pend = (unsigned long)-1 * HZ; //FIXME should set it 24 hours - xxx .
		return ;
	}

	*pend = msecs_to_jiffies(alarm_time * 1000);
}

static void add_uevent_work(struct work_struct *ws)
{
	struct tm1650_data *tmdata = container_of(ws, struct tm1650_data,\
									work_uevent);

#if TM1650_UEVENT
	{
		char event_str[16];
		char *envp[] = { event_str, NULL };

		sprintf(event_str, "ALARM=%s", (char *)tmdata->alarm_data);
		printk("---kobj name: %s\n", kobject_name(&tmdata->tmdevice->kobj));
		
		kobject_uevent_env(&tmdata->tmdevice->kobj, KOBJ_CHANGE, envp);
	}	
#endif
}

/*
 * print "Hello,Alarm", and spangled 5 times in 1s, \
 * until 10 seconds; and mod time every 24 hours 
 */
static void alarm_work_func(unsigned long data)
{
	struct proc_time_list *proc_node;	
	struct tm1650_data *tmdata = i2c_get_clientdata(gclient);
	
	proc_node = (struct proc_time_list *)data;
	printk("--alarm !-->>%s\n", (char *)proc_node->alarm_data);

	/* TODO spangled and keep 10 seconds ! */
	
	mod_timer(proc_node->timer_data, jiffies + 24 * 60 * 60 * HZ);
	
	/* add uevent */
#if TM1650_UEVENT
	tmdata->alarm_data = proc_node->alarm_data;
	queue_work(tmdata->wqueue, &tmdata->work_uevent);
#endif
}

/*
 *Here : hakuna make  timers nodes.
 */
static void request_work_process(struct work_struct *ws)
{
	unsigned long pend;	
	struct list_head *ptr;
	struct proc_time_list *proc_node;	
	struct tm1650_data *tmdata;
	struct timer_list *alarm_timer;

	tmdata = container_of(ws, struct tm1650_data, request_proc_work);

	/* get the alarm time from time list */
	ptr = alarm_timer_list.prev;
	proc_node = list_entry(ptr, struct proc_time_list, list);
	if (proc_node == NULL) {
		printk("proc_node is null\n");
		return ;
	}
	printk("data : %s\n", (char *)proc_node->alarm_data);
	//TODO lock
	
//FIXME :will free after retrun,proc_node->timer_data = &alarm_timer;
	alarm_timer = kzalloc(sizeof(struct timer_list), GFP_KERNEL);
	
	data_to_jiffies(proc_node->alarm_data, ":", &pend);
	printk("pend = %lu\n", pend);
	if (pend > 0) {
		/* set timer */
		setup_timer(alarm_timer, alarm_work_func, \
					(unsigned long)proc_node);
		alarm_timer->expires = jiffies + pend;
		add_timer(alarm_timer);
	} else {
		/* del from list */
	}
	
	proc_node->timer_data = alarm_timer;

return ;	
}

/*put the start outof the probe, to be
  public by 'EXPORT_SYMBOL_GPL(tm1650_trigger_workqueue);'
  */
void tm1650_trigger_workqueue(void)
{
	struct tm1650_data *data = i2c_get_clientdata(gclient);
	
	queue_work(data->wqueue, &data->work_set_time);
}

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

static void set_single_data(unsigned long tmdata, unsigned char *single_data)
{
	int i, cent = 1000;
	for (i = 0; i < 4; i++) {
		single_data[3 - i] = tmdata / cent;
		tmdata %= cent;
		cent /= 10;
	}
}

/*TODO cmd1:show level
  cmd2:show what
*/
static long tm1650_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned long tmdata;
	struct tm1650_data *data = filp->private_data;
	struct i2c_client *client = data->tmclient;
	unsigned char single_data[4] = {0};
	int retry = 0;

	printk("tm1650 ioctl cmd = %d\n", cmd);

	/*if userspace like 0x33, use follow*/
	//tmdata = arg;
	/*if userspace like &buf, use follow	like the write() func param: buf*/
	get_user(tmdata, (unsigned long __user*)arg);
	printk("tmdata is %ld\n", tmdata);
	
	switch (cmd) {
		case TM1650_DIS_LEV:
			client->addr = 0x24;
			data->data[0] = tmdata;
			i2c_master_send(client, data->data, 1);
			break;
		case TM1650_SET_DATA:
			set_single_data(tmdata, single_data);	
			set_data_core(tm1650_addr, single_data);
			break;
		case TM1650_DIS_FLICK:
			deinfo("flick kernel start ...\n");
			client->addr = 0x24;
			for (; retry < 4; retry++) {
				data->data[0] = 0x0;
				i2c_master_send(client, data->data, 1);
				msleep(200);
				data->data[0] = 0x31;
				i2c_master_send(client, data->data, 1);
				msleep(200);
			}
			break;
		default:
			return -1;
	}
	
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
		ret = alloc_chrdev_region(&data->devno, dev_minor, 1, \
			dev_name(&client->dev));
	}
	if (ret < 0)
		return ret;
	
	deinfo(dev_name(&client->dev));
	
	cdev_init(&data->cdev, &tm1650_ops);
	ret = cdev_add(&data->cdev, data->devno, 1);
	if (ret)
		deinfo("error add a char dev");
	//TODO how to get other module`s class, like make the i2c-usr to /sys/class/i2c-dev	
	data->tmclass = class_create(THIS_MODULE, "i2c-usr");
	data->tmdevice = device_create(data->tmclass, NULL, \
					data->devno, NULL, dev_name(&client->dev));
	
	return 0;
}

/*
===========================================================================
||不是，open是vfs的标准API，所以传进来的inode不是proc_inode，而是\
||&proc_inodevfs_inode。这里把标准的inode结构，嵌入在一个更大的proc_inode\
||结构里面，有了里面node指针，通过偏移（参见container_of），\
||就能找到外层的proc_inode。
||static inline struct proc_inode *PROC_I(const struct inode *inode)
||{
||	return container_of(inode, struct proc_inode, vfs_inode);
||}
||有了proc_inode，就能找到proc_dir_entry，这个不是内嵌结构，只是一个普通的指针而 \
||static inline struct proc_dir_entry *PDE(const struct inode *inode)
||{
||	return PROC_I(inode)->pde;
||}
||有了proc_dir_entry，就能访问私有数据pde->data 
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
	list_for_each_entry(pos, &alarm_timer_list, list) {
		seq_printf(seq, "%d\t\t%s", pos->no, (char *)pos->alarm_data);
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
	struct tm1650_data *data = filp->private_data;

	new_time = kzalloc(sizeof(struct proc_time_list), GFP_KERNEL);
	if (!new_time)
		return -ENOMEM;
	
	tmcontent = kzalloc(l, GFP_KERNEL);
	if (!tmcontent) {
		ret = -ENOMEM;
		goto alloc_tmcontent_err;
	}	

//FIXME lock	
	ret = copy_from_user(tmcontent, (char __user*)buf, l);
	if (ret) {
		ret = -ENOMEM;
		goto copy_err;
	}

	new_time->no = no++;
	new_time->alarm_data = tmcontent;

	list_add_tail(&new_time->list, &alarm_timer_list);

#if PROC_ALARM
	/* because the [return l] will free filp->private_data */
	data = i2c_get_clientdata(gclient);
	if (!data) {
		ret = -1;
		printk("proc: i2c client err!\n");
		goto i2c_client_err;
	}
	queue_work(data->wqueue, &data->request_proc_work);
#endif

	return l;

i2c_client_err:
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

static void tm1650_get_year_work(void)
{
	struct timespec ts;
	struct rtc_time tm;
	unsigned char single_data[4];
	unsigned int now_year;
	unsigned int i;
	unsigned int cent = 1000;

	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec, &tm);
	printk("%02d\n", tm.tm_year + 1900);
	now_year = tm.tm_year + 1900;
	
	for (i = 0; i < 4; i++) {
		single_data[3 - i] = now_year / cent;
		now_year %= cent;
		cent /= 10;
	}
	
	set_data_core(tm1650_addr, single_data);
}

/*
	month and day
*/
static void tm1650_get_day_work(void)
{
	struct timespec ts;
	struct rtc_time tm;
	unsigned char single_data[4];

	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec, &tm);
	printk("%02d\n", tm.tm_mon); //[0, 11]
	
	tm.tm_mon += 1;
	single_data[3] = tm.tm_mon / 10;
	single_data[2] = tm.tm_mon % 10;
	single_data[1] = tm.tm_mday / 10;
	single_data[0] = tm.tm_mday % 10;

	set_data_core(tm1650_addr, single_data);
}

/*
	wake up the thread work_set_time who`s function is tm1650_set_time_work 
*/
static void tm1650_get_second_work(void)
{
	mod_timer(&tm1650_timer, jiffies + HZ / 100);
}

/*
	hold up the thread work_set_time . may be i should use a timer .
*/
static void tm1650_switch_time_work(struct work_struct *ws)
{
	static int retry = GET_YEAR;
	
	if (retry != GET_SECOND) {
		//hakuna hold work
		mod_timer(&tm1650_timer, jiffies + (-1)* HZ); // long long time
	}
	
	/*if the show is second, touch the button, the show must be YEAR*/
	if (flag == GET_BACK) {
		retry = GET_YEAR;	
	}

	switch (retry % 3) {
		case GET_YEAR:
			tm1650_get_year_work();
			mod_timer(&tm1650_timer, jiffies + 5 * HZ);
			flag = -1;
			break;
		case GET_DAY:
			tm1650_get_day_work();
			mod_timer(&tm1650_timer, jiffies + 5 * HZ);
			flag = -1;
			break;
		case GET_SECOND:
			tm1650_get_second_work();
			break;
		default:
			break;
	}

	retry++;
	
}

/*
	switch the show cotent by the irq retry times .
	NOTE: the operations should be in a kthread .
*/
static void tm1650_switch_function(unsigned long data)
{
	struct tm1650_data *tmdata = (struct tm1650_data *)data;
	
	if (!work_pending(&tmdata->work_switch_time)) {
		queue_work(tmdata->wqueue, &tmdata->work_switch_time);
	}
}

/*
	creat a switch timer .
*/
static void tm1650_init_switch_timer(void)
{
	struct tm1650_data *data = i2c_get_clientdata(gclient);

	init_timer(&tm1650_switch_timer);
	tm1650_switch_timer.data = (unsigned long)data;
	tm1650_switch_timer.function = tm1650_switch_function;
	add_timer(&tm1650_switch_timer);
}

/*
	retry: 1, 2, 3---> year , day, hour ...etc
*/
static irqreturn_t tm1650_switch_show(int irq, void *data)
{
	int ret;
	
	ret = mod_timer(&tm1650_switch_timer, jiffies + (HZ) / 2); //500ms
	if (ret < 0) {
		deerro("mod_timer");
	}
	
	return IRQ_HANDLED;
}

static void tm1650_set_8888_work(struct work_struct *ws)
{
	int i;
	struct tm1650_data *data = i2c_get_clientdata(gclient);

	while (1) {
#if MUTEX_THREAD
		mutex_lock(&data->tm1650_mutex);
#endif		
		data->data[0] = 0x7f;
		msleep(1);
		printk("---%d\n", data->data[0]);
		for (i = 0; i < 4; i++) {
			gclient->addr = tm1650_addr[i];
			i2c_master_send(gclient, data->data, 1);
		}
#if MUTEX_THREAD
		mutex_unlock(&data->tm1650_mutex);
#endif		
	}	
}

static void tm1650_set_1111_work(void)
{
	struct tm1650_data *data = i2c_get_clientdata(gclient);
	
	while (1) {
#if MUTEX_THREAD
		mutex_lock(&data->tm1650_mutex);
#endif		
		data->data[0] = 0x06;
		msleep(1);
		printk("-------------------------\n\n\n");
#if MUTEX_THREAD
		mutex_unlock(&data->tm1650_mutex);
#endif		
	}	
}

static int tm1650_set_0000_8888(void *p)
{
	int i, j;
	struct tm1650_data *data = i2c_get_clientdata(gclient);
	
	data->data[0] = tm1650_tab[0]; 
	while (true) {
#if SYNC_THREAD		
		down(&data->sem);
#endif	
		msleep(1);
		for (j = 0; j < sizeof(tm1650_tab)/sizeof(char); j++) {
			if (tm1650_tab[j] == data->data[0]) {
				printk("------data: %d\n", j);
			}
		}
		
		for (i = 0; i < 4; i++) {
			gclient->addr = tm1650_addr[i];
			i2c_master_send(gclient, data->data, 1);
		}
#if SYNC_THREAD		
		up(&data->sem);
#endif		
	}

	return 0;
}

static int tm1650_add_0000_8888(void *p)
{
	int i = 1;
	struct tm1650_data *data = i2c_get_clientdata(gclient);

	while (true) {
#if SYNC_THREAD		
		down(&data->sem);
#endif		
		msleep(1);
		data->data[0] = tm1650_tab[i++ % 16];
#if SYNC_THREAD		
		up(&data->sem);
#endif		
	}

	return 0;
}
/*
+*spin_lock_irqsave等等自旋锁用处：1在中断中， 2在下半部
+*但是请记住所有的锁都是为了竟态同步而设计的, 中断中也需要保护共享数据
+*FIXME:下面的处理需要将共享数据加以保护
*/
static irqreturn_t tm1650_set_1111(int irq, void *data)
{
	struct tm1650_data *tmdata = (struct tm1650_data *)data;

	tmdata->data[0] = 0x6;
	printk("interrupt\n");

	return IRQ_HANDLED;
}

/* http://www.xuebuyuan.com/1661637.html
*  http://blog.csdn.net/wesleyluo/article/details/8807919
*/
static int tm1650_set_2222(void *p)
{
	int i;
	unsigned long flags;
	struct tm1650_data *data = i2c_get_clientdata(gclient);
	
	while (true) {
#if SYNC_INTERRUPT //if single cpu, i can use local_irq_disable ? \
					but here is SMP .
	spin_lock_irqsave(&data->lock, flags);
#endif
		data->data[0] = 0x5B;
//		msleep(2); //FIXME do not use sleep, can not sleep in spin_lock
		udelay(1000);
		for (i = 0; i < 4; i++) {
			gclient->addr = tm1650_addr[i];
			printk("----data: %d\n", data->data[0]);
//			i2c_master_send(gclient, data->data, 1); //FIXME will sleep .
		}
#if SYNC_INTERRUPT
	spin_unlock_irqrestore(&data->lock, flags);
#endif
	}

	return 0;
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
	//TODO FIXME  或许这样做更加抽象
	// 在tm1650_data结构体中的struct device *tmdevice = &client->dev
	// 以后就完全在驱动中操作更加抽象和封装的tm1650_data 结构体就好了，不必还拿着client这种低级抽象对象满天飞了
	dev_set_name(&client->dev, "tm1650_led");
	memcpy(data->name, dev_name(&client->dev), sizeof(dev_name(&client->dev)));

	/* this just for the function like EXPORT_SYMBOL_GPL, who has no param */
	gclient = client;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE | \
		I2C_FUNC_I2C))
	return -EIO;
	//TODO TODO do some thing func like 1, 2, 3, 4....
	tm1650_init_display(client);

	/* 1.base:  this function now is old ! */
#if BASE_TEST	
	//tm1650_set_time_work();
#endif

	/* 2.single workqueue: */
#if SINGLE_WORK_QUEUE	
	data->wqueue = create_singlethread_workqueue(dev_name(&(client->dev)));
	INIT_WORK(&data->work_set_time, tm1650_set_time_work);
	tm1650_trigger_workqueue();
#endif

	/* 3.chardev, use class,device */
#if CHR_DEV
	ret = tm1650_setup_cdev(data);
	if(ret < 0)
		deinfo("tm1650_setup_cdev error");
#endif

	/* 4.fillops, like open, read, write */
#if USR_SPACE
	// -->userspace test 
#endif

	/* 5.proc filesystem, and jiffies usr space touch a file: \
		time: 08:23 \
		action:print something \
		support space " "
	*/	
#if CREAT_PROC
	ret = tm1650_create_procfs(data);
#endif
#if PROC_ALARM
	data->wqueue = create_singlethread_workqueue(dev_name(&(client->dev)));
	INIT_WORK(&data->request_proc_work, request_work_process);
#endif

	/* 6.interrupt, gpio: once-->2016, twice-->12 22, etc */
#if TIMER_WORK
	tm1650_timer_work();
#endif

#if SWITCH_TIMER
	data->wqueue = create_singlethread_workqueue(dev_name(&(client->dev)));
	INIT_WORK(&data->work_switch_time, tm1650_switch_time_work);
	tm1650_init_switch_timer();
	ret = request_irq(gpio_to_irq(TM1650_SWITCH), tm1650_switch_show,\
		IRQF_TRIGGER_FALLING, dev_name(&client->dev), data);		
#endif

	/* 7.Mafile create a MACRO like MUTEX_THREAD..NOSYNC_INTERRUPT*/
#if MAKEFILE_MACRO
	// -->modify the makefile
#endif

	/* 8.0  some methods for 2 kthreads mutex MUTEX  */
#if MUTEX_THREAD
	mutex_init(&data->tm1650_mutex);
#endif

	/* 8.0  create two kernel work (include the main) for testing \
		the no mutex problems */
#if NOMUTEX_THREAD
	data->wqueue_on_cpu0 = create_workqueue("not a single thread");
	INIT_WORK(&data->work_nomutex_8888, tm1650_set_8888_work);
	queue_work(data->wqueue_on_cpu0, &data->work_nomutex_8888);
	tm1650_set_1111_work();
#endif
	
	/* 8.1 some methods for 2 kthreads sync, for showing 1111,2222,3333.... */
#if SYNC_THREAD
	sema_init(&data->sem, 1);
#endif
	/* 8.1  create two kernel work (include the main) for testing \
		the no sync problems*/
#if NOSYNC_THREAD
	kernel_thread(tm1650_set_0000_8888, NULL, 0);
	kernel_thread(tm1650_add_0000_8888, NULL, 0);
#endif

	/* 9.modify  issue  10, make it working correctly in sync  */
#if SYNC_INTERRUPT
	// Please search tm1650_set_2222 ! 
#endif
	
	/* 10.create one kthread and one interrupt, testing the no sync problems */
#if NOSYNC_INTERRUPT
	ret = request_irq(gpio_to_irq(TM1650_SET_1111), tm1650_set_1111,\
		IRQF_TRIGGER_FALLING, dev_name(&client->dev), data);
	//10.1 tm1650_set_2222 is no sleep thread .	
	kernel_thread(tm1650_set_2222, NULL, 0);
	//TODO
	//10.2 tm1650_set_3333 is a sleep thread .	
#endif
	
	/*11. uevent, use struct list to do it, or use the kernel method like \
		chang rui shi dai*/
#if TM1650_UEVENT
	INIT_WORK(&data->work_uevent, add_uevent_work);
#endif

	/* 12.do something, must use the sysfs*/
#if TM1650_SYSFS
	//TODO
#endif
	
	/* 13.take a long time to do the poll relations, IIInclude userspace and \
		the kernel space*/
#if TM1650_POLL
	//TODO
#endif

	/* 14.UUUUUUUUUUUUUUUUUUUUUse a loop buffer for sending data to userspace */
#if TM1650_LOOP_BUF //POLL
	//TODO
#endif	

	/* 15. just test my coding skills, use listseq_file interface, is a\
		abstract obj */
#if TM1650_LIST
	//TODO
#endif

	/* 15. that`s all, and the next thing is "The Native & Framework & App" */
	//TODO TODO TODO TODO TODO

	return 0;
tmclient_err:
	kfree(data);
	return ret;
}

static int tm1650_i2c_remove(struct i2c_client* client)
{
	struct tm1650_data* data = i2c_get_clientdata(client);
	
	debug();
#if SINGLE_WORK_QUEUE	
	flush_workqueue(data->wqueue);
	destroy_workqueue(data->wqueue);
#endif

#if NOSYNC_THREAD
	flush_workqueue(data->wqueue_on_cpu0);
	destroy_workqueue(data->wqueue_on_cpu0);
#endif

	/*if there is no follow step, the kernel will dump*/
#if CHR_DEV
	device_destroy(data->tmclass, data->devno);
	class_destroy(data->tmclass);
	cdev_del(&data->cdev);
	unregister_chrdev_region(data->devno, 1);
#endif

#if CREAT_PROC
	remove_proc_entry("tm1650_att", data->tm1650_proc_dir);
	remove_proc_entry("tm1650_proc", NULL);
#endif

	kfree(data);
#if TIMER_WORK	
	del_timer(&tm1650_timer);
#endif

#if SWITCH_TIMER
	del_timer(&tm1650_switch_timer);
	free_irq(gpio_to_irq(TM1650_SWITCH), data);
#endif

#if TM1650_UEVENT	
	flush_workqueue(data->wqueue);
	destroy_workqueue(data->wqueue);
#endif
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
MODULE_AUTHOR("Hakuna Matata"); 
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("tm1650 i2c led");

//FIXME: static function
EXPORT_SYMBOL_GPL(tm1650_set_time_work);
EXPORT_SYMBOL_GPL(tm1650_timer_work);
EXPORT_SYMBOL_GPL(tm1650_trigger_workqueue);
