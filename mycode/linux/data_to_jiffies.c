#include <stdio.h>
#include <string.h>

int main()
{
	char *haystack = "08:03";
	char *needle = ":";
	char* buf = strstr( haystack, needle);
	while( buf != NULL )
	{
		buf[0]='\0';
		printf( "%s\n ", haystack);
		haystack = buf + strlen(needle);
		/* Get next token: */
		buf = strstr( haystack, needle);
	}
		

	return 0;
}
