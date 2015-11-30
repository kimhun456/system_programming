

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define DRIVER_NAME		"/dev/cnkey"

void doHelp(void)
{
	printf("Usage:\n");
	printf("key \n");
	printf("return   0 ~15 (push button nubmer)\n"); 
}



int main(int argc , char **argv)
{
	int rdata ;
	
	int fd;
	
	if (argc > 1 )
	{
		perror(" Args number is more than 1\n");
		doHelp();
		return 1;
	}
	

	// open  driver 
	fd = open(DRIVER_NAME,O_RDWR);
	if ( fd < 0 )
	{
		perror("driver  open error.\n");
		return 1;
	}

	read(fd,&rdata,4);

	printf("button NO:%d\n",rdata);
	close(fd);
	
	return rdata;
}
