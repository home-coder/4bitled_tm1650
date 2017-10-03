#ifndef _TM1650_H
#define _TM1650_H

#include <list.h>

typedef unsigned char u_char;

typedef struct _tm1650_proc{
	   int  pid;
       char name[64];
       struct list_head lproc;
}tm1650_proc;

LIST_HEAD(tm1650_proc_list);

#endif
