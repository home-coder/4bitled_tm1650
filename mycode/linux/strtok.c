#include <stdio.h>
#include <string.h>


int main(int argc,char **argv)
{


	char * buf0="aaa,bb,cc";
	int len = strlen(buf0) + 1;
	char buf1[len];
	memcpy(buf1, buf0, len);
	/* Establish string and get the first token: */
	char* token = strtok( buf1, ",");
	while( token != NULL )
	{
		/* While there are tokens in "string" */
		printf( "%s ", token );
		/* Get next token: */
		token = strtok( NULL, ",");
	}
	return 0;

}
