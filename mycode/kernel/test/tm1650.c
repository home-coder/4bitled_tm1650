#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <common.h>
#include <ctype.h>
#include <tm1650.h>
#include <unistd.h>
#include <poll.h>
#include <linux/input.h>
#include <pthread.h>

#define BUF_SIZE  256
#define DEBUG_PRINT
#define SABRESD_TM1650_NEXT 1
#define SABRESD_TM1650_PREV 5
#define SABRESD_TM1650_DEL  9

enum {TM1650_DIS_LEV = 0, TM1650_SET_DATA};

static const char *procdir = "/proc";
static const char *tmpath = "/dev/tm1650_led";

struct list_head *g_current_node;

static int isdigit_string(const char *name)
{
	while (*name) {
		if (isxdigit(*name) == 0) {
			return 0;
		}
		name++;
	}

	return 1;
}

/*
根据进程的pid，找到status，然后解析/proc/xxpid/status里面的Name
*/
static void get_proc_name(const char *status, char *task_name)
{
	char title[128];
	char buf[BUF_SIZE];

	FILE *fp = fopen(status, "r");
	if (NULL != fp) {
		while (fgets(buf, BUF_SIZE - 1, fp) != NULL) {
			sscanf(buf, "%s %*s", title);
			if (!strncasecmp(title, "Name", strlen("Name"))) {
				sscanf(buf, "%*s %s", task_name);
				break;
			}
		}
		fclose(fp);
	}
}

/*
将pid 和 Name组成的结构添加到链表中
*/
static void add_proc(const char *identity)
{
	char proc_status[32];
	tm1650_proc *tmproc = (tm1650_proc *) malloc(sizeof(tm1650_proc));
	tmproc->pid = strtol(identity, (char **)&identity, 10);
	sprintf(proc_status, "/proc/%d/status", tmproc->pid);
	if (access(proc_status, F_OK) != 0) {
		dbgprint("%s is not exsit\n", proc_status);
		return;
	}
	get_proc_name((const char *)proc_status, tmproc->name);

	//dbgprint("%d %s\n", tmproc->pid, tmproc->name);
	list_add_tail(&tmproc->lproc, &tm1650_proc_list);
}

/*
根据 /proc/目录下数字名字作为进程，/proc/xxx/status中的name作为进程名字
将生成的结构体作为链表节点
*/
static void creat_tm1650_list()
{
	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;

	if ((dp = opendir(procdir)) == NULL) {
		fprintf(stderr, "Can`t open directory %s\n", procdir);
		return;
	}
	while ((entry = readdir(dp)) != NULL) {
		lstat(entry->d_name, &statbuf);
		if (S_ISDIR(statbuf.st_mode)) {
			if (strcmp(entry->d_name, ".") == 0 ||
			    strcmp(entry->d_name, "..") == 0) {
				continue;
			}
			if (!isdigit_string(entry->d_name)) {
				continue;
			}
			add_proc(entry->d_name);
		}
	}
}

static void print_tm1650_list()
{
	tm1650_proc *ptmproc = NULL;
	list_for_each_entry(ptmproc, &tm1650_proc_list, lproc) {
		//dbgprint("%d:%s\n", ptmproc->pid, ptmproc->name);
	}
}

/*
具体数据的解析和发送，将会在kernel中处理
*/
static void show_keyevent_proc(struct list_head *current)
{
	tm1650_proc *tmproc;

	int tmfd = open(tmpath, O_RDWR);
	if (tmfd < 0) {
		dbgprint("open failed\n");
		return ;
	}

	tmproc = list_entry(current, tm1650_proc, lproc);
	if (!tmproc) {
		dbgprint("list_entry error\n");	
	}
	dbgprint("pid = %d\n", tmproc->pid);
	if (ioctl(tmfd, TM1650_SET_DATA, &tmproc->pid)) {
		dbgprint("set failed\n");
		return ;
	}
}

/*
按键切换显示进程id流程的实现
*/
static void set_data_core(int keyvalue)
{
	switch (keyvalue) {
		case SABRESD_TM1650_NEXT:
			g_current_node = g_current_node->next;
			break;
		case SABRESD_TM1650_PREV:
			g_current_node = g_current_node->prev;
			break;
		case SABRESD_TM1650_DEL:
			break;
		default:
			break;
	}
	show_keyevent_proc(g_current_node);
}

/*
将当前led显示的数据闪烁两次
*/
static void set_flick()
{

}

/*
网络链接出处：http://blog.csdn.net/liuyi_lab/article/details/53956623
*/
#define  KEY_EVENT_DEV0_NAME    "/dev/input/event0"
#define  KEY_EVENT_DEV1_NAME    "/dev/input/event1"

static int show_next_by_key(void)
{
	int l_ret = -1;
	int i = 0;

	int key_fd[2] = { 0 };
	struct pollfd key_fds[2];
	struct input_event key_event;

	key_fd[0] = open(KEY_EVENT_DEV0_NAME, O_RDONLY);
	if (key_fd[0] <= 0) {
		dbgprint("---open /dev/input/event0 device error!---\n");
		return l_ret;
	}

	key_fd[1] = open(KEY_EVENT_DEV1_NAME, O_RDONLY);
	if (key_fd[1] <= 0) {
		dbgprint("---open /dev/input/event1 device error!---\n");
		return l_ret;
	}

	for (i = 0; i < 2; i++) {
		key_fds[i].fd = key_fd[i];
		key_fds[i].events = POLLIN;
	}
	
	dbgprint("key event listen...\n");
	while (1) {
		l_ret = poll(key_fds, 2, -1);
		if (l_ret < 0) {
			dbgprint("poll error\n");
			exit(-1);
		}

		for (i = 0; i < 2; i++) {
			if (key_fds[i].revents & POLLERR) {
				dbgprint("device error\n");
				exit(-1);
			}

			if (key_fds[i].revents & POLLIN) {
				lseek(key_fd[i], 0, SEEK_SET);
				l_ret = read(key_fd[i], &key_event, sizeof(key_event));
				//printf("l_ret = %d\n", l_ret);
				if (l_ret) {
					if (key_event.type == EV_KEY) {
						if (key_event.value == 1) {
							printf("key value(%d) %s\n", key_event.code, key_event.value ? "press" : "release");
							fflush(stdout);
							set_data_core(key_event.code);
						} else if (key_event.value == 1) {
							set_flick();
						}
					}
				}
			}
		}
	}

	close(key_fd[0]);
	close(key_fd[1]);

	return l_ret;
}

static void show_by_listen_key()
{
	pthread_t pidl;
	int ret;
	ret = pthread_create(&pidl, NULL, (void *)show_next_by_key, NULL);
	if (ret != 0) {
		dbgprint("creat pthread error\n");
	}
	pthread_join(pidl, NULL);
}

/*
不考虑进程是否已经结束，只是简单的将list中的进程pid显示到led上面,第一次显示第一个pid，通过NEXT来切换
*/
static void display_proc_list()
{
	g_current_node = tm1650_proc_list.next;
	show_keyevent_proc(g_current_node);

	/*创建一个线程来处理按键事件*/
	show_by_listen_key();
}

int main()
{
	creat_tm1650_list();
#ifdef DEBUG_PRINT
	print_tm1650_list();
#endif
	display_proc_list();

	return 0;
}
