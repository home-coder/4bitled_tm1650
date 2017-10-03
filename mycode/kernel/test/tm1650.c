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

#define BUF_SIZE  256
static const char *procdir = "/proc";

static int isdigit_string(const char *name)
{
	while (*name) {
		if ( isxdigit(*name)==0 ) {
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

	FILE* fp = fopen(status, "r");
	if (NULL != fp) {
		while (fgets(buf, BUF_SIZE-1, fp) != NULL) {
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
	tm1650_proc *tmproc = (tm1650_proc *)malloc(sizeof(tm1650_proc));
	tmproc->pid = strtol(identity, (char **)&identity, 10);
	sprintf(proc_status, "/proc/%d/status", tmproc->pid);
	if ( access(proc_status, F_OK)!=0 ) {
		dbgprint("%s is not exsit\n", proc_status);
		return ;
	}
	get_proc_name((const char *)proc_status, tmproc->name);

	dbgprint("%d %s\n", tmproc->pid, tmproc->name);
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
		return ;
	}
	while ((entry = readdir(dp)) != NULL) {
		lstat(entry->d_name, &statbuf); 
		if (S_ISDIR(statbuf.st_mode)) { 
			if (strcmp(entry->d_name, ".") == 0 ||
					strcmp(entry->d_name, "..") == 0 ) {
				continue;
			}
			if ( !isdigit_string(entry->d_name) ) {
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
		dbgprint("%d:%s\n", ptmproc->pid, ptmproc->name);
	}
}

int main()
{
	creat_tm1650_list();
	print_tm1650_list();
		
	return 0;
}
