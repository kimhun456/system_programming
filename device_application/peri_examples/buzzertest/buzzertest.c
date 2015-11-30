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

#define DRIVER_NAME		"/dev/cnbuzzer"
#define MAX_BUZZER_NUMBER		36

void doHelp(void)
{
	printf("Usage:\n");
	printf("buzzertest <buzzerNo> \n");
	printf("buzzerNo: \n");
	printf("do(1,13,25,37) ,do#(2,14,26)");
	printf("re(3,15,27), re#(4,16,28)\n");
	printf("mi(5,17,29)\n");
	printf("fa(6,18,30),fa#(7,19,31)\n");  
	printf("sol(8,20,32) ,sol#(9,21,33)\n");
	printf("ra(10,22,34), ra#(11,23,35)\n");
	printf("si(12,24,36)\n");  
}

int main(int argc , char **argv)
{
	int buzzerNumber;
	int fd;
	
	if (argc < 2 )
	{
		perror(" Args number is less than 3\n");
		doHelp();
		return 1;
	}
	
	if ( strlen(argv[1]) > 2 )
	{
		perror("buzzerNo length > 2 \n");
		doHelp();
		return 1;
	}
	buzzerNumber = atoi(argv[1]);

	printf("buzzer number :%d \n",buzzerNumber);

	if ( buzzerNumber > MAX_BUZZER_NUMBER )
	{
		perror(" <buzzerNo> over range \n");
		doHelp();
		return 1;
	}
	
	// open  driver 
	fd = open(DRIVER_NAME,O_RDWR);
	if ( fd < 0 )
	{
		perror("driver (//dev//cnbuzzer) open error.\n");
		return 1;
	}
	// control led 
	write(fd,&buzzerNumber,4);

	close(fd);
	
	return 0;
}
