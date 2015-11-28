

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
#include "../../inc/cn310.h"


#define DRIVER_NAME		"/dev/cnbtsw"


void doHelp(void)
{
	printf("Usage:\n");
	printf("btswtest \n");
	printf("return value :one nibble hex value !\n");
}



int main(int argc , char **argv)
{
	int rdata;
	
	int fd;
	
	if (argc > 1 )
	{
		perror("There is no args.\n");
		doHelp();
		return 0;
	}
	
	// open  driver 
	fd = open(DRIVER_NAME,O_RDWR);
	if ( fd < 0 )
	{
		perror("driver  open error.\n");
		return 0;
	}

	rdata = 0;

	read(fd,&rdata,4);
	printf("rdata:0x%X.\n",rdata);

	close(fd);
	
	return rdata;
}
