#ifndef _TM1650_H
#define _TM1650_H

#include <list.h>

typedef unsigned char u_char;

typedef struct _tm1650st{
       u_char tmno;
       char tmdata[64];
       struct list_head tmlist;
}tm1650st;

LIST_HEAD(alarm_timer_list);
#endif
