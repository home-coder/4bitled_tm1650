#include <linux/init.h>  
#include <linux/module.h>  
#include <linux/mutex.h>  
#include <linux/semaphore.h>  
#include <linux/sched.h>  
#include <linux/delay.h>  

static DEFINE_MUTEX(g_mutex);  
static DEFINE_SEMAPHORE(g_semaphore);  

static int fun1(void *p)  
{  
	while (true) {  
		mutex_lock(&g_mutex);  
		msleep(1000);  
		printk("1\n");  
		mutex_unlock(&g_mutex);  
	}  
	return 0;  
}  

static int fun2(void *p)  
{  
	while (true) {  
		mutex_lock(&g_mutex);  
		msleep(1000);  
		printk("2\n");  
		mutex_unlock(&g_mutex);  
	}  
	return 0;  
}  

static int fun3(void *p)  
{  
	while (true) {  
		down(&g_semaphore);  
		msleep(1000);  
		printk("3\n");  
		up(&g_semaphore);  
	}  
	return 0;  
}  

static int fun4(void *p)  
{  
	while (true) {  
		down(&g_semaphore);  
		msleep(1000);  
		printk("4\n");  
		up(&g_semaphore);  
	}  
	return 0;  
}  

static int hello_init(void)  
{  
	kernel_thread(fun1, NULL, 0);  
	kernel_thread(fun2, NULL, 0);  
	kernel_thread(fun3, NULL, 0);  
	kernel_thread(fun4, NULL, 0);  
	return 0;  
}  

module_init(hello_init);
MODULE_LICENSE("GPL");                     
MODULE_AUTHOR("Hakuna Matata");            
MODULE_VERSION("1.0");                                                                         
MODULE_DESCRIPTION("tm1650 i2c led"); 
