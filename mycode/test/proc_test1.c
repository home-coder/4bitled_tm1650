#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>

struct proc_dir_entry *test_dir;
struct proc_dir_entry *test_att;

//fd = open("/proc/test_dir/test_att", xx)
//read(fd, buf, len)
//内核分配一页内存(4K)(page)
//len--->count
//*eof = 1
//content--->page---->buf
int read_att(char *page, char **start, off_t off, \
	int count, int *eof, void *data)
{
	int ret = 0;
	
	ret += sprintf(page + ret, "Hello Proc\n");
	ret += sprintf(page + ret, "Nihao Proc\n");

	*eof = 1;

	return ret;
}

static __init int proc_test_init(void)
{
	int ret;

	//NULL--->/proc/test_dir
	test_dir = proc_mkdir("test_dir", NULL);
	if(!test_dir){
		ret = -ENOMEM;
		goto mkdir_error;
	}
	test_att = create_proc_read_entry("test_att", 0, test_dir, read_att, (void *)100);
	if(!test_att){
		ret = -ENOMEM;
		goto create_att_error;
	}
	
	return 0;
create_att_error:
	remove_proc_entry("test_dir", NULL);	
mkdir_error:
	return ret;
}

static __exit void proc_test_exit(void)
{
	printk("hello\n");
	remove_proc_entry("test_att", test_dir);
	remove_proc_entry("test_dir", NULL);
}

module_init(proc_test_init);
module_exit(proc_test_exit);

MODULE_LICENSE("GPL");





