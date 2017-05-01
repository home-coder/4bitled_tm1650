#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
struct devinfo {
	char dev_no;
	char dev_name[32];
	struct list_head dev;
};

int main()
{
	int i = 0;
	struct devinfo *ptr;
	struct list_head *head = (struct list_head *)malloc(sizeof(*head));
	struct list_head *list = NULL;

	INIT_LIST_HEAD(head);

	srand(time(NULL));
	for (; i < 9; i++) {
		struct devinfo *df = (struct devinfo *)malloc(sizeof(*df));
		memset(df, 0x0, sizeof(*df));
		if (i == 6) {
			df->dev_no = 90;
			memcpy(df->dev_name, "hello list", 32);
		} else {
			df->dev_no = random();
		//	printf("no = %d\t", df->dev_no);
			sprintf(df->dev_name, "001:%d", df->dev_no);
		//	printf("name = %s\n", df->dev_name);
		}
		list_add_tail(&df->dev, head);
	}
	
	list_for_each(list, head) {
		ptr = list_entry(list, struct devinfo, dev);
		if (ptr->dev_no == 90)
			printf("devname = %s\n\n", ptr->dev_name);
	}

	list_for_each_entry(ptr, head, dev) {
		printf("no = %d,name = %s\n", ptr->dev_no, ptr->dev_name);
	}

	return 0;
}
