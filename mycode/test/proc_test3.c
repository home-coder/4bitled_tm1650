#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>//kzalloc kfree
#include <linux/list.h>
#include <asm/uaccess.h>//copy_from_user

struct proc_dir_entry *test_dir;
struct proc_dir_entry *test_att;

struct head_st {
	int count;
	struct list_head head;
}phead;

struct item_st {
	int no;
	int len;
	char *content;
	struct list_head item;	
};

//page-->4K
int att_read(char *page, char **start, off_t off, \
		int count, int *eof, void *data)
{
	struct item_st *tmp_item;
	int ret = 0;
#if 0
	struct list_head *l;
	for(l = phead.head.next; l != &phead.head; l = l->item.next){
		tmp_item = container_of(l, struct item_st, item);	
	}
#endif
	list_for_each_entry(tmp_item, &phead.head, item){
		ret += sprintf(page + ret, "No:%d	Len:%d	Content:%s", tmp_item->no, tmp_item->len, tmp_item->content);	
	}

	*eof = 1;

	return ret;
}

int att_write(struct file *file, const char \
__user *buffer, unsigned long count, void *data)
{
	struct item_st *tmp_item;
	char *tmp_content;
	int ret;

	tmp_item = kzalloc(sizeof(struct item_st), GFP_KERNEL);	
	if(!tmp_item)
		return -ENOMEM;

	tmp_content = kzalloc(count, GFP_KERNEL);
	if(!tmp_content){
		ret = -ENOMEM;
		goto alloc_content_error;	
	}

	ret = copy_from_user(tmp_content, buffer, count);
	if(ret){
		ret = -ENOMEM;
		goto copy_error;
	}

	tmp_item->no = phead.count;
	tmp_item->len = count;
	tmp_item->content = tmp_content;
	phead.count++;

	list_add_tail(&tmp_item->item, &phead.head);

	return count;
copy_error:
	kfree(tmp_content);
alloc_content_error:
	kfree(tmp_item);	
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
	test_att = create_proc_entry("test_att", 0, test_dir);
	if(!test_att){
		ret = -ENOMEM;
		goto create_att_error;
	}
	test_att->read_proc = att_read;
	test_att->write_proc = att_write;
	test_att->data = (void *)100;

	INIT_LIST_HEAD(&phead.head);
	phead.count = 0;
	
	return 0;
create_att_error:
	remove_proc_entry("test_dir", NULL);	
mkdir_error:
	return ret;
}

static __exit void proc_test_exit(void)
{
	struct item_st *tmp_item, *tmp;

	list_for_each_entry_safe(tmp_item, tmp, &phead.head, item){
		kfree(tmp_item->content);
		kfree(tmp_item);
	}
	remove_proc_entry("test_att", test_dir);
	remove_proc_entry("test_dir", NULL);
}

module_init(proc_test_init);
module_exit(proc_test_exit);

MODULE_LICENSE("GPL");





