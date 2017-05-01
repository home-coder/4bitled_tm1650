#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>//copy_from_user

char buf[1024];

int att_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int ret = 0;
	
	ret += sprintf(page + ret, "%s", buf);
	
	*eof = 1;

	return ret;
}

//ret = write(fd, buf, len) FILE *fopen()
//[4G-1,3G][3G-1,0]
//buf--->buffer
//len--->count
int att_write(struct file *file, const char __user *buffer, unsigned long count, void *data)
{
	//buffer-->buf
	//min(count, 1024)
	//max(count, 1024)
	if(copy_from_user(buf, buffer, count))
		return -EFAULT;

	return count;
}

struct proc_dir_entry *test_dir;
struct proc_dir_entry *test_att;

static __init int proc_test_init(void)
{
	int ret;

	//NULL--->/proc/test_dir
	test_dir = proc_mkdir("test_dir", NULL);
	if(!test_dir){
		ret = -ENOMEM;
		goto mkdir_error;
	}
	test_att = create_proc_entry("test_att", 0, test_dir);
	if(!test_att){
		ret = -ENOMEM;
		goto create_att_error;
	}
	test_att->read_proc = att_read;
	test_att->write_proc = att_write;
	test_att->data = (void *)100;
	
	return 0;
create_att_error:
	remove_proc_entry("test_dir", NULL);	
mkdir_error:
	return ret;
}

static __exit void proc_test_exit(void)
{
	remove_proc_entry("test_att", test_dir);
	remove_proc_entry("test_dir", NULL);
}

module_init(proc_test_init);
module_exit(proc_test_exit);

MODULE_LICENSE("GPL");





