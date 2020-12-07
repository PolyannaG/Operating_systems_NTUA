#include <stdio.h>
#include <unistd.h>

void zing(){
	char *name;
	name=getlogin();
	printf("Hi %s\n", name);
}
