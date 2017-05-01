#include <stdio.h>
#include <unistd.h>

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({ \
		const typeof( ((type *)0)->member ) *__mptr = (ptr); \
		(type *)( (char *)__mptr - offsetof(type,member) );})

struct door {
	char no;
	char w;
	char h;
};

int main()
{
	struct door d = {
		1,
		22,
		33,
	};
	struct door *dp;
	char *wr = &d.w;
	printf("dp = [%p]\nwp = [%p]\n", &d, wr);
	dp = container_of(wr, struct door, w);

//	printf("d = [%p]\n[%p]0x%x\n[%p]0x%x\n[%p]0x%x\n",\
		&d, &d.no, d.no, &d.w, d.w, &d.h, d.h);
	printf("dp = [%p]\nwp = [%p]\n", dp, wr);
	printf("dp->no %d\n", dp->no);

	return 0;
}
