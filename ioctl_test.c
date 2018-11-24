#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "queue_ioctl.h"

int main(int argc, char **argv) {
	int fd = open("/dev/queue0", O_RDONLY);
	
	if (fd == -1) 
		return 0;
		
	char* arg = malloc(sizeof(char)*100);
		
	if (ioctl(fd, QUEUE_POP, &arg) < 0) {
		return 0;
	}
	
	printf("Popped string: %s\n", arg);
	
	free(arg);
	return 0;
}
