#include <stdio.h>
#include <string.h>

static void data_to_jiffies(const char *data, unsigned long pend)
{
	char *token;
	int len = 0;
	char buf1[len];
	printf("--len %d\n", len);
	const char * buf0 = data;
	const char *delim = ":";
	len = strlen(buf0) + 1;
	char *s = buf1;
	int tem;
	memcpy(buf1, buf0, len);
	/* Establish string and get the first token: */
	for(token = strsep(&s, delim); token != NULL; \
			token = strsep(&s, delim)) {   
		printf("---%s", token);
		token = NULL;
		tem = atoi(token);
		printf(": %d\n", tem);
	}   
}

int main()
{
	const char *s = "08:03";
	data_to_jiffies(s, 3);

	return 0;
}
