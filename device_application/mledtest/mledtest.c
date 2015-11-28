

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
#include <termios.h>


#define DRIVER_NAME		"/dev/cnmled"

void doHelp(void)
{
	printf("Usage:\n");
	printf("mledtest option displayTime number\n");
	printf("exit 'q' \n");
	printf("ex) mledtest s 3 71 ; display '71' for 3 Seconds\n");
	printf("ex) mledtest c 3 71 ; counting  from 0 to 71  with 3 Seconds interval.\n");

}
#define MAX_COLUMN_NUM	5
// 0 ~ 9
const unsigned short NumData[10][MAX_COLUMN_NUM]=
{
	{0xfe00,0xfd7F,0xfb41,0xf77F,0xef00}, // 0
	{0xfe00,0xfd42,0xfb7F,0xf740,0xef00}, // 1
	{0xfe00,0xfd79,0xfb49,0xf74F,0xef00}, // 2
	{0xfe00,0xfd49,0xfb49,0xf77F,0xef00}, // 3
	{0xfe00,0xfd0F,0xfb08,0xf77F,0xef00}, // 4
	{0xfe00,0xfd4F,0xfb49,0xf779,0xef00}, // 5
	{0xfe00,0xfd7F,0xfb49,0xf779,0xef00}, // 6
	{0xfe00,0xfd07,0xfb01,0xf77F,0xef00}, // 7
	{0xfe00,0xfd7F,0xfb49,0xf77F,0xef00}, // 8
	{0xfe00,0xfd4F,0xfb49,0xf77F,0xef00}  // 9
};

static struct termios oldt, newt;
void changemode(int dir)
{
	if( dir == 1)
	{
		tcgetattr(STDIN_FILENO , &oldt);
		newt = oldt;
		newt.c_lflag &= ~(ICANON | ECHO );
		tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	}
	else
	{
		tcsetattr(STDIN_FILENO , TCSANOW, &oldt);

	}
}


int kbhit(void)
{
	struct timeval tv;
	fd_set rdfs;

	tv.tv_sec = 0;
	tv.tv_usec = 0;
	
	FD_ZERO(&rdfs);
	FD_SET(STDIN_FILENO , &rdfs);

	select(STDIN_FILENO + 1 , &rdfs , NULL, NULL, &tv);

	return FD_ISSET(STDIN_FILENO , &rdfs);
}



#define ONE_LINE_TIME_U 	1000
// exit return => 0 , success return => 1
int displayDotLed(int driverfile , int num ,int timeS)
{
	int cSelCounter,loopCounter;
	int highChar , lowChar;
	int temp , totalCount ;
	unsigned short wdata[2];

	temp = num % 100;

	highChar = temp / 10;
	lowChar = temp % 10;

	
	totalCount = timeS*(1000000 / ONE_LINE_TIME_U);
	printf("totalcounter: %d\n",totalCount);
	cSelCounter = 0;
	loopCounter = 0;
	while(1)
	{
		// high byte display 
		wdata[0] = NumData[highChar][cSelCounter];

		// low byte display 
		wdata[1] = NumData[lowChar][cSelCounter];

		write(driverfile,(unsigned char*)wdata,4);

		cSelCounter++;
		if ( cSelCounter >= (MAX_COLUMN_NUM-1))
			cSelCounter = 1;

		usleep(ONE_LINE_TIME_U);

		loopCounter++;
		if ( loopCounter > totalCount )
			break;
		
		if (kbhit())
		{
			if ( getchar() == (int)'q')
			{
		
				wdata[0]= 0;
				wdata[1]= 0;
				write(driverfile,(unsigned char*)wdata,4);
				printf("Exit mledtest\n");
				return 0;
			}

		}

	}
	wdata[0]= 0;
	wdata[1]= 0;
	write(driverfile,(unsigned char*)wdata,4);

	return 1;
}

int main(int argc , char **argv)
{
	int durationTime ,Num ;
	
	int fd;
	int counterFlag = 0;
	int counter;
	
	if (argc < 4 )
	{
		perror(" Args number is less than 4\n");
		doHelp();
		return 1;
	}
	
	if ( (argv[1][0] == 's' ) || ( argv[1][0] == 'c' ))
	{
		if (  argv[1][0] == 'c' )
			counterFlag = 1;

	}
	else
	{
		perror("option parameter are not 's' and 'c' .\n");
		doHelp();
		return 1;
	}

	printf("exit 'q' \n");

	durationTime = atoi(argv[2]);

	Num = atoi(argv[3]);

	if (durationTime == 0 )
		durationTime =1;

	changemode(1);
	// open  driver 
	fd = open(DRIVER_NAME,O_RDWR);
	if ( fd < 0 )
	{
		perror("driver  open error.\n");
		return 1;
	}

	if ( counterFlag )
	{
		counter = 0;

		for ( counter  = 0; counter <= Num ; counter++)
		{
			if(!displayDotLed(fd , counter ,durationTime))
				break;

		}
		

	}
	else
	{
		displayDotLed(fd , Num ,durationTime);
	}

	changemode(0);
	close(fd);
	
	return 0;
}
