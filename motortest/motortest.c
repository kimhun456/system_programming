

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

#define DRIVER_NAME		"/dev/cnmotor"


void doHelp(void)
{
	printf("Usage:\n");
	printf("motortest velocity   sec\n");
	printf("velocity :60 ~ 3600 \n");
	printf("Exit  => 'q'\n");
	printf("ex) motortest 500  3 ; 30 velocity  for 3 Sec\n");
}

#define  CYCLE_STEP		4
const unsigned short stepValue[CYCLE_STEP] = { 0x0009, 0x0003,0x0006, 0x000C};


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


#define BASE_RPM		400

#define CHANGE_RPM_CYCLE_NUM		5

// exit return => 0 , success return => 1
int controlMotor(int filedriver, int targetRpm , int durationTime )
{
	int stepValueCounter;

	int stepIntervalTime;
	int curRpm;
	int reachVelFlag ;
	int wdata;
	int cycleCounter;
	int exitCounter; 
	
	stepValueCounter = 0;
	reachVelFlag = 0;
	cycleCounter = 0;

	if ( targetRpm < BASE_RPM )
	{
		curRpm = targetRpm ;
		stepIntervalTime = 15000000/curRpm;  // uT = (60*1000000) /(4 * targetRpm) = 15000000/targetRpm;

	}
	else 
	{
		curRpm = BASE_RPM ;
		stepIntervalTime = 15000000/curRpm;  // uT = (60*1000000) /(4 * targetRpm) = 15000000/targetRpm;
	}


	while (1)
	{
		wdata = stepValue[stepValueCounter];
		write(filedriver,&wdata,4);

		stepValueCounter++;
		if ( stepValueCounter >= CYCLE_STEP )
		{
			stepValueCounter = 0;
			cycleCounter++;
			if (reachVelFlag )
			{
				if ( cycleCounter > exitCounter ) 
				{
					wdata = stepValue[0];
					write(filedriver,&wdata,4);
					break;
				}
			}
			else
			{
				if ( cycleCounter > CHANGE_RPM_CYCLE_NUM )
				{
					cycleCounter = 0;
					if ( targetRpm <= (curRpm + BASE_RPM))
					{
						curRpm = targetRpm;
						stepIntervalTime = 15000000/curRpm;  // uT = (60*1000000) /(4 * targetRpm) = 15000000/targetRpm;

						exitCounter = durationTime * (1000000 / stepIntervalTime);
						reachVelFlag = 1;
					}
					else 
					{
						curRpm += BASE_RPM;
						stepIntervalTime = 15000000/curRpm;  // uT = (60*1000000) /(4 * targetRpm) = 15000000/targetRpm;
					}
				}
			}
		}
		
		usleep(stepIntervalTime);

		if (kbhit())
		{
			if ( getchar() == (int)'q')
			{
				printf("Exit mledtest\n");
				return 0;
			}

		}
	}

	return 1;
}


int main(int argc , char **argv)
{
	int time, rpm ;
	
	int fd;
	
	if (argc < 3 )
	{
		perror(" Args number is less than 3\n");
		doHelp();
		return 1;
	}
	
	rpm = atoi(argv[1]);

	if ( rpm == 0 ) 
		rpm = 60;
	if (rpm > 3600)
		rpm = 3600;

	time = atoi(argv[2]);

	if (time == 0)
		time = 3;


	changemode(1);
	// open  driver 
	fd = open(DRIVER_NAME,O_RDWR);
	if ( fd < 0 )
	{
		perror("driver open error.\n");
		return 1;
	}

	controlMotor(fd, rpm , time);

	close(fd);
	changemode(0);
	
	return 0;
}
