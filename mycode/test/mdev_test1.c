#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>//MKDEV
#include <linux/fs.h>//register_* unregister_*

dev_t dev_no;

static __init int cdev_test_init(void)
{
	int ret;

	dev_no = MKDEV(222, 133);	

	ret = register_chrdev_region(dev_no, 1, "mydev250");
	if(ret)
		return ret;

	return 0;
}

static __exit void cdev_test_exit(void)
{
	unregister_chrdev_region(dev_no, 1);
}

//insmod xxx.ko
module_init(cdev_test_init);
//rmmod xxx.ko
//rmmod xxx
module_exit(cdev_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZhiYong Li");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("Class test for module");

















