

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

#define DRIVER_NAME		"/dev/cndipsw"

void doHelp(void)
{
	printf("Usage:\n");
	printf("dipsw \n");
	printf("return  int  (0~ 15 bit are valid). \n");
}



int main(int argc , char **argv)
{
	
	int fd;
	int retvalue;
	
	if (argc >1 )
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
	read(fd,&retvalue,4);
	retvalue &= 0xF;
	printf("retvalue:0x%X\n",retvalue);

	close(fd);
	
	return retvalue;
}
