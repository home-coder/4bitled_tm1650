

#include <stdio.h>  
#include <string.h>  

int main(void) {  
	char source[] = "hello, world! welcome to china!";  
	char delim[] = " ,!";  

	char *s = strdup(source);  
	char *token;  
	for(token = strsep(&s, delim); token != NULL; token = strsep(&s, delim)) {  
		printf(token);  
		printf("+");  
	}  
	printf("\n");  
	return 0;  
}  
