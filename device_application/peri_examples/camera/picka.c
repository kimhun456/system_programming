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
#include <sys/mman.h>
#include <errno.h>
#include <sys/poll.h>
#include <linux/fb.h>
#include <linux/videodev2.h>
#include "videodev2.h"
#include "SecBuffer.h"
#include "camera.h"
#include "bitmap.h"

#define DIPSW_DRIVER_NAME		"/dev/cndipsw"
#define BUS_LED_ON	1
#define BUS_LED_OFF	0
#define BUS_MAX_LED_NO		8
#define BUS_LED_DRIVER_NAME		"/dev/cnled"



#define COLOR_DRIVER_NAME		"/dev/cncled"


#define COLOR_INDEX_LED		0
#define COLOR_INDEX_REG_LED		1
#define COLOR_INDEX_GREEN_LED		2
#define COLOR_INDEX_BLUE_LED		3
#define COLOR_INDEX_MAX			4



#define SPEED_BUZZER_DRIVER_NAME		"/dev/cnbuzzer"

#define PW_BUZZER_DRIVER_NAME		"/dev/cnbuzzer"

#define BUZZER_MAX_BUZZER_NUMBER		36



#define TEXTLCD_TRUE		1
#define TEXTLCD_FALSE		0

#define TEXTLCD_SUCCESS		0
#define TEXTLCD_FAIL		1

static int  text_fd ;

#define TEXTLCD_DRIVER_NAME		"/dev/cntlcd"
/******************************************************************************
*
*      TEXT LCD FUNCTION
*
******************************************************************************/
#define TEXTLCD_CLEAR_DISPLAY		0x0001
#define TEXTLCD_CURSOR_AT_HOME		0x0002

// Entry Mode set
#define TEXTLCD_MODE_SET_DEF		0x0004
#define TEXTLCD_MODE_SET_DIR_RIGHT	0x0002
#define TEXTLCD_MODE_SET_SHIFT		0x0001

// Display on off
#define TEXTLCD_DIS_DEF				0x0008
#define TEXTLCD_DIS_LCD				0x0004
#define TEXTLCD_DIS_CURSOR			0x0002
#define TEXTLCD_DIS_CUR_BLINK		0x0001

// shift
#define TEXTLCD_CUR_DIS_DEF			0x0010
#define TEXTLCD_CUR_DIS_SHIFT		0x0008
#define TEXTLCD_CUR_DIS_DIR			0x0004

// set DDRAM  address
#define TEXTLCD_SET_DDRAM_ADD_DEF	0x0080

// read bit
#define TEXTLCD_BUSY_BIT			0x0080
#define TEXTLCD_DDRAM_ADD_MASK		0x007F


#define TEXTLCD_DDRAM_ADDR_LINE_1	0x0000
#define TEXTLCD_DDRAM_ADDR_LINE_2	0x0040


#define TEXTLCD_SIG_BIT_E			0x0400
#define TEXTLCD_SIG_BIT_RW			0x0200
#define TEXTLCD_SIG_BIT_RS			0x0100


//****************************keymatrix*************************
#define KEY_MATRIX_DRIVER_NAME		"/dev/cnkey"



int keyMatrix(void)
{
	int rdata ;
        int pwArr[4]= {1,3,5,7};
	int fd;
	int save[4]={0};
	int i=0;
	int counter=0;
	int flag=0;


	// open  driver
	fd = open(KEY_MATRIX_DRIVER_NAME,O_RDWR);
	if ( fd < 0 )
	{
		perror("driver  open error.\n");
		return 1;
	}
	printf("pw:");
	while(1)
 	{
		read(fd,&rdata,4);

		if(rdata){
                printf("*");

		save[i]=rdata;
		i++;

		usleep(300000);
		fflush(stdout);
	 	}

		if(i==4){
			printf("\n");
			break;
			}
	}
	for(i=0;i<4;i++){
		if(save[i]!=pwArr[i]){

			//printf("ERROR!!!!!!\n");
			close(fd);
			return 0;
		}

	}
	printf("good  pw : %d %d %d %d \n",save[0],save[1],save[2],save[3]);
	close(fd);
	return 1;




}




//&&*************************************7segment********************************


/*
		a
	f		b
		g
	e		c
		d		dp


*/
#define FND_DRIVER_NAME		"/dev/cnfnd"

#define FND_MAX_FND_NUM		6

#define  FND_DOT_OR_DATA	0x80

const unsigned short segNum[10] =
{
	0x3F, // 0
	0x06,
	0x5B,
	0x4F,
	0x66,
	0x6D,
	0x7D,
	0x27,
	0x7F,
	0x6F  // 9
};
const unsigned short segSelMask[FND_MAX_FND_NUM] =
{
	0xFE00,
	0xFD00,
	0xFB00,
	0xF700,
	0xEF00,
	0xDF00
};
static struct termios oldt, newt;
void fndchangemode(int dir)
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

int fndkbhit(void)
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



#define FND_ONE_SEG_DISPLAY_TIME_USEC	1000
// return 1 => exit  , 0 => success
int fndDisp(int driverfile, int num , int dotflag,int durationSec)
{
	int cSelCounter,loopCounter;
	int temp , totalCount, i ;
	unsigned short wdata;
	int dotEnable[FND_MAX_FND_NUM];
	int fndChar[FND_MAX_FND_NUM];

	for (i = 0; i < FND_MAX_FND_NUM ; i++ )
	{
		dotEnable[i] = dotflag & (0x1 << i);
	}
	// if 6 fnd
	temp = num % 1000000;
	fndChar[0]= temp /100000;

	temp = num % 100000;
	fndChar[1]= temp /10000;

	temp = num % 10000;
	fndChar[2] = temp /1000;

	temp = num %1000;
	fndChar[3] = temp /100;

	temp = num %100;
	fndChar[4] = temp /10;

	fndChar[5] = num %10;

	totalCount = durationSec*(1000000 / FND_ONE_SEG_DISPLAY_TIME_USEC);
	//printf("totalcounter: %d\n",totalCount);
	cSelCounter = 0;
	loopCounter = 0;
	while(1)
	{
		wdata = segNum[fndChar[cSelCounter]]  | segSelMask[cSelCounter] ;
		if (dotEnable[cSelCounter])
			wdata |= FND_DOT_OR_DATA;

		write(driverfile,&wdata,2);

		cSelCounter++;
		if ( cSelCounter >= FND_MAX_FND_NUM )
			cSelCounter = 0;

		//usleep(FND_ONE_SEG_DISPLAY_TIME_USEC);

		loopCounter++;
		if ( loopCounter > totalCount )
			break;

		if (fndkbhit())
		{
			if ( getchar() == (int)'q')
			{

				wdata= 0;
				write(driverfile,&wdata,2);
				//printf("Exit fndtest\n");
				return 0;
			}

		}
	}

	wdata= 0;
	write(driverfile,&wdata,2);

	return 1;
}

#define FND_MODE_STATIC_DIS		0
#define FND_MODE_TIME_DIS		1
#define FND_MODE_COUNT_DIS		2
void fndLed(int distance)
{
	int fd;
	int mode ;
	int number,counter;
	int durationtime;




		mode = FND_MODE_STATIC_DIS;

		durationtime = 3;

		number = distance;




	printf("distance :%d\n",number);
	// open  driver
	fd = open(FND_DRIVER_NAME,O_RDWR);

	fndchangemode(1);

		fndDisp(fd, number , 0,durationtime);



		//LABEL_ERR:

	fndchangemode(0);
	close(fd);


}





//**************************************OLED**************************************

#define OLED_TRUE	1
#define OLED_FALSE	0

#define OLED_DRIVER_NAME		"/dev/cnoled"

static  int  OLED_fd ;


unsigned long simple_strtoul(char *cp, char **endp,unsigned int base)
{
	unsigned long result = 0,value;

	if (*cp == '0') {
		cp++;
		if ((*cp == 'x') && isxdigit(cp[1])) {
			base = 16;
			cp++;
		}
		if (!base) {
			base = 8;
		}
	}
	if (!base) {
		base = 10;
	}
	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
								? toupper(*cp) : *cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (char *)cp;
	return result;
}

unsigned long read_hex(const char* str){
	char addr[128];
	strcpy(addr,str);
	return simple_strtoul(addr, NULL, 16);
}


// signal form
//	12bit	11bit	10bit	9bit	8bit	7bit	6bit	5bit	4bit	3bit	2bit	1bit	0bit
//	RST#	CS#		D/C#	WD#		RD#		D7		D6		D5		D4		D3		D2		D1		D0
// trigger => WD or RD rising edge
/************************************************************************************************





************************************************************************************************/
#define OLED_RST_BIT_MASK	0xEFFF
#define OLED_CS_BIT_MASK		0xF7FF
#define OLED_DC_BIT_MASK		0xFBFF
#define OLED_WD_BIT_MASK		0xFDFF
#define OLED_RD_BIT_MASK		0xFEFF
#define OLED_DEFAULT_MASK	0xFFFF


#define OLED_CMD_SET_COLUMN_ADDR		0x15
#define OLED_CMD_SET_ROW_ADDR		0x75
#define OLED_CMD_WRITE_RAM			0x5C
#define OLED_CMD_READ_RAM			0x5D
#define OLED_CMD_LOCK				0xFD

int oledreset(void)
{
	unsigned short wdata ;

	wdata = OLED_RST_BIT_MASK;
	write(OLED_fd,&wdata , 2 );
	//usleep(2000);
	wdata = OLED_DEFAULT_MASK;
	write(OLED_fd,&wdata , 2 );
	return OLED_TRUE;
}

int oledwriteCmd(int size , unsigned short* cmdArr)
{
	int i ;
	unsigned short wdata;

	//printf("wCmd : [0x%02X]",cmdArr[0]);
	//wdata = CS_BIT_MASK;
	//write(OLED_fd,&wdata,2);

	wdata = OLED_CS_BIT_MASK & OLED_DC_BIT_MASK;
	write(OLED_fd,&wdata,2);

	wdata = OLED_CS_BIT_MASK & OLED_DC_BIT_MASK & OLED_WD_BIT_MASK ;
	write(OLED_fd,&wdata,2);

	wdata = OLED_CS_BIT_MASK & OLED_DC_BIT_MASK & OLED_WD_BIT_MASK & (cmdArr[0]|0xFF00) ;
	write(OLED_fd,&wdata,2);

	wdata = OLED_CS_BIT_MASK & OLED_DC_BIT_MASK & (cmdArr[0] | 0xFF00) ;
	write(OLED_fd,&wdata,2);

	wdata = OLED_CS_BIT_MASK & ( cmdArr[0] | 0xFF00);
	write(OLED_fd,&wdata,2);

	for (i = 1; i < size ; i++ )
	{
	//	wdata = CS_BIT_MASK ;
	//	write(OLED_fd,&wdata,2);

	//	wdata = CS_BIT_MASK ;
	//	write(OLED_fd,&wdata,2);

		wdata = OLED_CS_BIT_MASK & OLED_WD_BIT_MASK ;
		write(OLED_fd,&wdata,2);

		wdata = OLED_CS_BIT_MASK & OLED_WD_BIT_MASK & (cmdArr[i] | 0xFF00) ;
		write(OLED_fd,&wdata,2);

		wdata = OLED_CS_BIT_MASK & (cmdArr[i] | 0xFF00);
		write(OLED_fd,&wdata,2);

	//	wdata = CS_BIT_MASK & (cmdArr[i] | 0xFF00);
	//	write(OLED_fd,&wdata,2);
	//	printf("[0x%02X]",cmdArr[i]);

	}
	wdata= OLED_DEFAULT_MASK;
	write(OLED_fd,&wdata,2);
	//printf("\n");
	return OLED_TRUE;
}

int oledwriteData(int size , unsigned char* dataArr)
{
	int i ;
	unsigned short wdata;

	//wdata = CS_BIT_MASK;
	//write(OLED_fd,&wdata,2);

	wdata = OLED_CS_BIT_MASK & OLED_DC_BIT_MASK;
	write(OLED_fd,&wdata,2);

	//wdata = CS_BIT_MASK & DC_BIT_MASK & WD_BIT_MASK ;
	//write(OLED_fd,&wdata,2);

	wdata = OLED_CS_BIT_MASK & OLED_DC_BIT_MASK & OLED_WD_BIT_MASK & (OLED_CMD_WRITE_RAM | 0xFF00) ;
	write(OLED_fd,&wdata,2);

	wdata = OLED_CS_BIT_MASK & OLED_DC_BIT_MASK & (OLED_CMD_WRITE_RAM | 0xFF00);
	write(OLED_fd,&wdata,2);

	wdata = OLED_CS_BIT_MASK &  (OLED_CMD_WRITE_RAM | 0xFF00);
	write(OLED_fd,&wdata,2);

	for (i = 0; i < size ; i++ )
	{
		wdata = OLED_CS_BIT_MASK & OLED_WD_BIT_MASK ;
		write(OLED_fd,&wdata,2);

		wdata = OLED_CS_BIT_MASK & OLED_WD_BIT_MASK & ((unsigned char)dataArr[i] | 0xFF00 );
		write(OLED_fd,&wdata,2);

		wdata = OLED_CS_BIT_MASK & ( (unsigned char)dataArr[i] | 0xFF00);
		write(OLED_fd,&wdata,2);


	}
	wdata = OLED_DEFAULT_MASK;
	write(OLED_fd,&wdata,2);

	return OLED_TRUE;

}

int oledreadData(int size , unsigned short* dataArr)
{

	int i ;
	unsigned short wdata;

	wdata = OLED_CS_BIT_MASK & OLED_DC_BIT_MASK;
	write(OLED_fd,&wdata,2);

	wdata = OLED_CS_BIT_MASK & OLED_DC_BIT_MASK & ( OLED_CMD_READ_RAM| 0xFF00) ;
	write(OLED_fd,&wdata,2);

	wdata = OLED_CS_BIT_MASK & OLED_DC_BIT_MASK & OLED_WD_BIT_MASK &( OLED_CMD_READ_RAM| 0xFF00);
	write(OLED_fd,&wdata,2);

	wdata = OLED_CS_BIT_MASK & OLED_DC_BIT_MASK & (OLED_CMD_READ_RAM | 0xFF00);
	write(OLED_fd,&wdata,2);

	wdata = OLED_CS_BIT_MASK &  (OLED_CMD_READ_RAM | 0xFF00);
	write(OLED_fd,&wdata,2);


	for (i = 0; i < size ; i++ )
	{
		//wdata = CS_BIT_MASK ;
		//write(OLED_fd,&wdata,2);

		wdata = OLED_CS_BIT_MASK ;
		write(OLED_fd,&wdata,2);

		wdata = OLED_CS_BIT_MASK & OLED_RD_BIT_MASK ;
		write(OLED_fd,&wdata,2);

		wdata = OLED_CS_BIT_MASK & OLED_RD_BIT_MASK ;
		write(OLED_fd,&wdata,2);

		wdata = OLED_CS_BIT_MASK ;
		write(OLED_fd,&wdata,2);

		read(OLED_fd,&dataArr[i],2);

		//wdata = CS_BIT_MASK ;
		//write(OLED_fd,&wdata,2);

	}
	wdata = OLED_DEFAULT_MASK;
	write(OLED_fd,&wdata ,2);

	return OLED_TRUE;
}

int oledsetAddressDefalut(void)
{
	unsigned short  cmd[3];
	cmd[0] = OLED_CMD_SET_COLUMN_ADDR;
	cmd[1] = 0;
	cmd[2] = 127;
	oledwriteCmd(3,cmd);

	cmd[0] = OLED_CMD_SET_ROW_ADDR;
	cmd[1] = 0;
	cmd[2] = 127;
	oledwriteCmd(3,cmd);

	return OLED_TRUE;
}

// to send cmd  , must unlock
int oledsetCmdLock(int bLock)
{
	unsigned short  cmd[3];

	cmd[0] = OLED_CMD_LOCK;
	if (bLock)
	{
		cmd[1] = 0x16; // lock
		oledwriteCmd(2,cmd);

	}
	else
	{
		cmd[1] = 0x12; // lock
		oledwriteCmd(2,cmd);

		// A2,B1,B3,BB,BE accessible
		cmd[1] = 0xB1;
		oledwriteCmd(2,cmd);
	}
	return OLED_TRUE;
}

int oledimageLoading(char* fileName)
{
	int imgfile;
	unsigned char* data =NULL;
	int  width , height;

	imgfile = open(fileName , O_RDONLY );
	if ( imgfile < 0 )
	{
		//printf ("imageloading(%s)  file is not exist . err.\n",fileName);
		return OLED_FALSE;
	}
	oledsetCmdLock(OLED_FALSE);


	read(imgfile ,&width , sizeof(unsigned char));
	read(imgfile ,&height , sizeof(unsigned char));

	data = malloc( 128 * 128 * 3 );

	read(imgfile, data , 128 * 128 *3 );

	close(imgfile);

	oledwriteData(128 * 128 *3 , data );

	oledsetCmdLock(OLED_TRUE);
	return OLED_TRUE;
}

static unsigned short gamma[64]=
{
0xB8,
0x02, 0x03, 0x04, 0x05,
0x06, 0x07, 0x08, 0x09,
0x0A, 0x0B, 0x0C, 0x0D,
0x0E, 0x0F, 0x10, 0x11,
0x12, 0x13, 0x15, 0x17,
0x19, 0x1B, 0x1D, 0x1F,
0x21, 0x23, 0x25, 0x27,
0x2A, 0x2D, 0x30, 0x33,
0x36, 0x39, 0x3C, 0x3F,
0x42, 0x45, 0x48, 0x4C,
0x50, 0x54, 0x58, 0x5C,
0x60, 0x64, 0x68, 0x6C,
0x70, 0x74, 0x78, 0x7D,
0x82, 0x87, 0x8C, 0x91,
0x96, 0x9B, 0xA0, 0xA5,
0xAA, 0xAF, 0xB4

};


int OledInit(void)
{
	unsigned short wdata[10];
	unsigned char  wcdata[10];
	int i,j;
	wdata[0]= 0xFD;
	wdata[1] = 0x12;
	oledwriteCmd(2,wdata);


	wdata[0] = 0xFD;
	wdata[1] = 0xB1;
	oledwriteCmd(2,wdata);

	wdata[0] = 0xAE;
	oledwriteCmd(1,wdata);

	wdata[0] = 0xB3;
	wdata[1] = 0xF1;
	oledwriteCmd(2,wdata);

	wdata[0] = 0xCA;
	wdata[1] = 0x7F;
	oledwriteCmd(2,wdata);

	wdata[0] = 0xA2;
	wdata[1] = 0x00;
	oledwriteCmd(2,wdata);

	wdata[0]= 0xA1;
	wdata[1]=0x00;
	oledwriteCmd(2,wdata);

	wdata[0]= 0xA0;
	wdata[1] = 0xB4;
	oledwriteCmd(2,wdata);

	wdata[0] = 0xAB;
	wdata[1] = 0x01;
	oledwriteCmd(2,wdata);

	wdata[0] = 0xB4;
	wdata[1] = 0xA0;
	wdata[2] = 0xB5;
	wdata[3] = 0x55;
	oledwriteCmd(4,wdata);

	wdata[0] = 0xC1;
	wdata[1] = 0xC8;
	wdata[2] = 0x80;
	wdata[3] = 0xC8;
	oledwriteCmd(4,wdata);

	wdata[0] = 0xC7;
	wdata[1] = 0x0F;
	oledwriteCmd(2,wdata);

	// gamma setting
	oledwriteCmd(64,gamma);


	wdata[0] = 0xB1;
	wdata[1] = 0x32;
	oledwriteCmd(2,wdata);

	wdata[0] = 0xB2;
	wdata[1] = 0xA4;
	wdata[2] = 0x00;
	wdata[3] = 0x00;
	oledwriteCmd(4,wdata);

	wdata[0] = 0xBB;
	wdata[1] = 0x17;
	oledwriteCmd(2,wdata);

	wdata[0] = 0xB6;
	wdata[1] = 0x01;
	oledwriteCmd(2, wdata);

	wdata[0]= 0xBE;
	wdata[1] = 0x05;
	oledwriteCmd(2, wdata);

	wdata[0] = 0xA6;
	oledwriteCmd(1,wdata);


	for (i = 0; i < 128;i++ )
	{
		for(j = 0; j < 128; j++ )
		{
			wcdata[0]= 0x3F;
			wcdata[1]= 0;
			wcdata[2] = 0;
			oledwriteData(3,wcdata);
		}

	}

	wdata[0] = 0xAF;
	oledwriteCmd(1,wdata);





	return OLED_TRUE;
}

//#define OLED_MODE_WRITE		0
//#define OLED_MODE_READ		1
//#define OLED_MODE_CMD		2
//#define OLED_MODE_RESET		3
#define OLED_MODE_IMAGE		4
//#define OLED_MODE_INIT		5
#define OLED_RIGHT 1
#define OLED_LEFT 0

static int OLED_Mode;

void oLed(int direction)
{
	int writeNum;
	unsigned char wdata[10];
	int readNum;
	unsigned short* rdata = NULL;
	unsigned short wCmd[10];




	/*if ( argv[1][0] == 'w')
	{
		int i ,j;
		OLED_Mode = OLED_MODE_WRITE;
		if (argc < 3)
		{
			perror(" Args number is less than 3\n");
			doHelp();
			return 1;
		}
		j = 0;
		for ( i  = 2; i < argc ; i++ )
		{
			wdata[j] = (unsigned char)read_hex(argv[i]);
			j++;
		}
		writeNum = j;
	}
	else if ( argv[1][0] == 'r')
	{
		OLED_Mode = OLED_MODE_READ;
		if ( argc < 3 )
		{
			perror(" Args number is less than 3\n");
			doHelp();
			return 1;
		}
		readNum = read_hex(argv[2]);

		rdata = malloc(readNum);
	}
	else if ( argv[1][0] == 'c')
	{
		int i ,j;
		OLED_Mode = OLED_MODE_CMD;
		if (argc < 3)
		{
			perror(" Args number is less than 3\n");
			doHelp();
			return 1;
		}
		j = 0;
		for ( i  = 2; i < argc ; i++ )
		{

			wCmd[j] = (unsigned short)read_hex(argv[i]);
			j++;
		}
		writeNum = j;
	}
	else if ( argv[1][0] == 't')
	{
		OLED_Mode = OLED_MODE_RESET;
	}  */

		OLED_Mode = OLED_MODE_IMAGE;

/*
	else if (argv[1][0] == 'i')
	{
		OLED_Mode = OLED_MODE_INIT;
	}
	else
	{
		perror("No supported options.\n");
		doHelp();
		return 1;

	}   */

	// open  driver
	OLED_fd = open(OLED_DRIVER_NAME,O_RDWR);


	/*switch ( OLED_Mode )
	{
	case OLED_MODE_WRITE:
		writeData(writeNum, wdata);
		break;
	case OLED_MODE_READ:
		{
			int i;
			readData(readNum, rdata);
			printf("Read Data:\n");
			for(i =0 ; i < readNum ; i++ )
			{
				printf("[%02X]",(unsigned char)rdata[i]);
			}
			printf("\n");
		}
		break;
	case OLED_MODE_CMD:
		writeCmd(writeNum , wCmd);
		break;
	case OLED_MODE_RESET:
		oledreset();
		break;
*/
		if(direction == OLED_RIGHT)
		oledimageLoading("right.img");
		else
		oledimageLoading("left.img");

	/*case OLED_MODE_INIT:
		OledInit();
		break;
	}*/



	close(OLED_fd);

	/*if ( OLED_Mode == OLED_MODE_READ)
	{
		if ( rdata != NULL)
			free(rdata);

	}*/



}






/***************************************************
read /write  sequence  text!!
write cycle
RS,(R/W) => E (rise) => Data => E (fall)

***************************************************/
int textIsBusy(void)
{
	unsigned short wdata, rdata;

	wdata = TEXTLCD_SIG_BIT_RW;
	write(text_fd ,&wdata,2);

	wdata = TEXTLCD_SIG_BIT_RW | TEXTLCD_SIG_BIT_E;
	write(text_fd ,&wdata,2);

	read(text_fd,&rdata ,2);

	wdata = TEXTLCD_SIG_BIT_RW;
	write(text_fd,&wdata,2);

	if (rdata &  TEXTLCD_BUSY_BIT)
		return TEXTLCD_TRUE;

	return TEXTLCD_FALSE;
}
int text_writeCmd(unsigned short cmd)
{
	unsigned short wdata ;

	if ( textIsBusy())
		return TEXTLCD_FALSE;

	wdata = cmd;
	write(text_fd ,&wdata,2);

	wdata = cmd | TEXTLCD_SIG_BIT_E;
	write(text_fd ,&wdata,2);

	wdata = cmd ;
	write(text_fd ,&wdata,2);

	return TEXTLCD_TRUE;
}

int text_setDDRAMAddr(int x , int y)
{
	unsigned short cmd = 0;
//	printf("x :%d , y:%d \n",x,y);
	if(textIsBusy())
	{
		perror("setDDRAMAddr busy error.\n");
		return TEXTLCD_FALSE;

	}
	if(y == 1){
	cmd = TEXTLCD_DDRAM_ADDR_LINE_1 +x;
	//printf("line1 %d %d\n",x,y);
	}
	else if(y==2){
	cmd = TEXTLCD_DDRAM_ADDR_LINE_2 +x;
	//printf("line2 %d %d\n",x,y);
	}

	if ( cmd >= 0x80)
		return TEXTLCD_FALSE;


//	printf("setDDRAMAddr w1 :0x%X\n",cmd);

	if (!text_writeCmd(cmd | TEXTLCD_SET_DDRAM_ADD_DEF))
	{
		perror("setDDRAMAddr error\n");
		return TEXTLCD_FALSE;
	}
//	printf("setDDRAMAddr w :0x%X\n",cmd|SET_DDRAM_ADD_DEF);
	//usleep(1000);
	return TEXTLCD_TRUE;
}

int text_displayMode(int bCursor, int bCursorblink, int blcd  )
{
	unsigned short cmd  = 0;

	if ( bCursor)
	{
		cmd = TEXTLCD_DIS_CURSOR;
	}

	if (bCursorblink )
	{
		cmd |= TEXTLCD_DIS_CUR_BLINK;
	}

	if ( blcd )
	{
		cmd |= TEXTLCD_DIS_LCD;
	}

	if (!text_writeCmd(cmd | TEXTLCD_DIS_DEF))
		return TEXTLCD_FALSE;

	return TEXTLCD_TRUE;
}

int writeCh(unsigned short ch)
{
	unsigned short wdata =0;

	if ( textIsBusy())
		return TEXTLCD_FALSE;

	wdata = TEXTLCD_SIG_BIT_RS | ch;
	write(text_fd ,&wdata,2);

	wdata = TEXTLCD_SIG_BIT_RS | ch | TEXTLCD_SIG_BIT_E;
	write(text_fd ,&wdata,2);

	wdata = TEXTLCD_SIG_BIT_RS | ch;
	write(text_fd ,&wdata,2);
	//usleep(1000);
	return TEXTLCD_TRUE;

}


int setCursorMode(int bMove , int bRightDir)
{
	unsigned short cmd = TEXTLCD_MODE_SET_DEF;

	if (bMove)
		cmd |=  TEXTLCD_MODE_SET_SHIFT;

	if (bRightDir)
		cmd |= TEXTLCD_MODE_SET_DIR_RIGHT;

	if (!text_writeCmd(cmd))
		return TEXTLCD_FALSE;
	return TEXTLCD_TRUE;
}

int functionSet(void)
{
	unsigned short cmd = 0x0038; // 5*8 dot charater , 8bit interface , 2 line

	if (!text_writeCmd(cmd))
		return TEXTLCD_FALSE;
	return TEXTLCD_TRUE;
}

int writeStr(char* str)
{


	unsigned char wdata;
	int i;
	for(i =0; i < strlen(str) ;i++ )
	{
		if (str[i] == '_')
			wdata = (unsigned char)' ';
		else
			wdata = str[i];
		writeCh(wdata);
	}
	return TEXTLCD_TRUE;

}

#define TEXTLCD_LINE_NUM			2
#define TEXTLCD_COLUMN_NUM			16
int clearScreen(int nline)
{
	int i;
	if (nline == 0)
	{
		if(textIsBusy())
		{
			perror("clearScreen error\n");
			return TEXTLCD_FALSE;
		}
		if (!text_writeCmd(TEXTLCD_CLEAR_DISPLAY))
			return TEXTLCD_FALSE;
		return TEXTLCD_TRUE;
	}
	else if (nline == 1)
	{
		text_setDDRAMAddr(0,1);
		for(i = 0; i <= TEXTLCD_COLUMN_NUM ;i++ )
		{
			writeCh((unsigned char)' ');
		}
		text_setDDRAMAddr(0,1);

	}
	else if (nline == 2)
	{
		text_setDDRAMAddr(0,2);
		for(i = 0; i <= TEXTLCD_COLUMN_NUM ;i++ )
		{
			writeCh((unsigned char)' ');
		}
		text_setDDRAMAddr(0,2);
	}
	return TEXTLCD_TRUE;
}


#define TEXTLCD_CMD_TXT_WRITE		0
#define TEXTLCD_CMD_CURSOR_POS		1
#define TEXTLCD_CMD_CEAR_SCREEN		2





#define DOTMATRIX_DRIVER_NAME		"/dev/cnmled"


#define DOTMATRIX_MAX_COLUMN_NUM	5
// 0 ~ 9
const unsigned short NumData[10][DOTMATRIX_MAX_COLUMN_NUM]=
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
void dotMatrixchangemode(int dir)
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


int dotmatrix_kbhit(void)
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

//******************************dotmatrix*********************************

#define DOTMATRIX_ONE_LINE_TIME_U 	1000
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


	totalCount = timeS*(1000000 / DOTMATRIX_ONE_LINE_TIME_U);
	//printf("totalcounter: %d\n",totalCount);
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
		if ( cSelCounter >= (DOTMATRIX_MAX_COLUMN_NUM-1))
			cSelCounter = 1;

		//usleep(DOTMATRIX_ONE_LINE_TIME_U);

		loopCounter++;
		if ( loopCounter > totalCount )
			break;

		if (dotmatrix_kbhit())
		{
			if ( getchar() == (int)'q')
			{

				wdata[0]= 0;
				wdata[1]= 0;
				write(driverfile,(unsigned char*)wdata,4);
				//printf("Exit mledtest\n");
				return 0;
			}

		}

	}
	wdata[0]= 0;
	wdata[1]= 0;
	write(driverfile,(unsigned char*)wdata,4);

	return 1;
}
//***********************************dotmatrix****************************
void dotMatrix(int speed)
{
	int i=0;
	int durationTime=2 ;
	int dotmatrix_fd;

	//printf("exit 'q' \n");

	if (durationTime == 0 )
		durationTime =1;

	dotMatrixchangemode(1);
	// open  driver
	dotmatrix_fd = open(DOTMATRIX_DRIVER_NAME,O_RDWR);
	if ( dotmatrix_fd < 0 )
	{
		perror("driver  open error.\n");
		exit(1);
	}


	displayDotLed(dotmatrix_fd , speed ,durationTime);// i need  int driverfile , int num ,int timeS


	dotMatrixchangemode(0);
	close(dotmatrix_fd);


}

//***********************************textlcd*************************************
int textLED(int row1, char* words1, int row2, char* words2)
{

	int nCmdMode;
	int bCursorOn, bBlink, nline1 , nline2;
	int nColumn1=0;
	int nColumn2=0;
	char strWtext1[TEXTLCD_COLUMN_NUM+1];
	char strWtext2[TEXTLCD_COLUMN_NUM+1];

	//printf("row1:%d,row2:%d \n",row1,row2);
	//printf("words1:%s,words2:%s\n",words1,words2);



		nCmdMode =  TEXTLCD_CMD_TXT_WRITE ;

		nline1 = row1;
		nline2 = row2;
		//printf("nline1 :%d\n",nline1);
		//printf("nline2 :%d\n",nline2);

		if (strlen(words1) > TEXTLCD_COLUMN_NUM )
		{
			strncpy(strWtext1,words1,TEXTLCD_COLUMN_NUM);
			strWtext1[TEXTLCD_COLUMN_NUM] = '\0';
		}
		else
		{
			strcpy(strWtext1,words1);

		}
		if(strlen(words2)>TEXTLCD_COLUMN_NUM)
		{
			strncpy(strWtext2,words2,TEXTLCD_COLUMN_NUM);
			strWtext2[TEXTLCD_COLUMN_NUM] = '\0';
		}
		else
		{

			strcpy(strWtext2,words2);

		}


	/*else if (  argv[1][0] == 'c' )
	{
		nCmdMode =  CMD_CURSOR_POS ;
		if ( argc < 6 )
		{
			perror(" c argument number is short.\n");
			doHelp();
			return 1;
		}
		bCursorOn = atoi(argv[2]);

		bBlink = atoi(argv[3]);

		nline = atoi(argv[4]);
		if ( (nline != 1 ) && (nline != 2 ))
		{
			perror("line para is worng.\n");
			doHelp();
			return 1;
		}
		nColumn = atoi(argv[5]);
		if ( nColumn >15 )
		{
			perror(" nColumn max number is 15.\n");
			doHelp();
			return 1;
		}
	}
	else if ( argv[1][0] == 'r' )
	{
		nCmdMode =  CMD_CEAR_SCREEN;
		nline = 0;
		if ( argc == 3 )
		{
			nline = atoi(argv[2]);
			if ( (nline != 1)&& (nline != 2))
				nline = 0;
		}
		else
			nline = 0;
	}
	else
	{
		perror(" not supported option. \n");
		doHelp();
		return 1;
	}*/

	// open  driver
	text_fd = open(TEXTLCD_DRIVER_NAME,O_RDWR);
	if ( text_fd < 0 )
	{
		perror("driver open error.\n");
		return 1;
	}
	functionSet();

	switch ( nCmdMode )
	{
	case TEXTLCD_CMD_TXT_WRITE:
		clearScreen(0);
		//printf("nline:%d ,nColumn:%d\n",nline,nColumn);
		text_setDDRAMAddr(nColumn1, nline1);
		//usleep(2000);
		writeStr(strWtext1);
		text_setDDRAMAddr(nColumn2, nline2);
		//usleep(2000);
		writeStr(strWtext2);
		break;
	/*case CMD_CURSOR_POS:
		displayMode(bCursorOn, bBlink, TRUE);
		text_setDDRAMAddr(nline-1, nColumn);
		break;*/
	case TEXTLCD_CMD_CEAR_SCREEN:
		clearScreen(nline1);
		clearScreen(nline2);
		break;

	}
	//printf("str1:%s,str2:%s\n",strWtext1,strWtext2);

	close(text_fd);

	return 0;
}

//*****************************************SpeedBuzzer**************************
void speedBuzzer(int speed,int limitSpeed)
{
	int buzzerNumber=0;
	int sbuzzer_fd;



	//buzzerNumber = atoi(argv[1]);

	//printf("buzzer number :%d \n",buzzerNumber);



	// open  driver
	sbuzzer_fd = open(SPEED_BUZZER_DRIVER_NAME,O_RDWR);
	if ( sbuzzer_fd < 0 )
	{
		perror("driver (//dev//cnbuzzer) open error.\n");
		exit(1);
	}

	// control led
	if(speed >= limitSpeed){
  	buzzerNumber = 30 ;
	write(sbuzzer_fd,&buzzerNumber,4);
 	sleep(3);
	buzzerNumber = 0;
	write(sbuzzer_fd,&buzzerNumber,4);
 	}
	close(sbuzzer_fd);


}
//************************************pwError buzzer *********************
void pwBuzzer(void)
{
	int buzzerNumber=0;
	int pwbuzzer_fd;



	//buzzerNumber = atoi(argv[1]);

	//printf("buzzer number :%d \n",buzzerNumber);



	// open  driver
	pwbuzzer_fd = open(PW_BUZZER_DRIVER_NAME,O_RDWR);
	if ( pwbuzzer_fd < 0 )
	{
		perror("driver (//dev//cnbuzzer) open error.\n");
		exit(1);
	}

	// control led

  	buzzerNumber = 30 ;
	write(pwbuzzer_fd,&buzzerNumber,4);
 	sleep(3);
	buzzerNumber = 0;
	write(pwbuzzer_fd,&buzzerNumber,4);

	close(pwbuzzer_fd);


}
//**********************************color led ************************************
void cled(int speed)
{
	unsigned short colorArray[COLOR_INDEX_MAX];

	int cled_fd;


	colorArray[COLOR_INDEX_LED] =(unsigned short)0;

	// open  driver
	cled_fd = open(COLOR_DRIVER_NAME,O_RDWR);
	if ( cled_fd < 0 )
	{
		perror("driver  open error.\n");
		exit(1);
	}

	if(speed== 0){
	colorArray[COLOR_INDEX_REG_LED] =(unsigned short)100;
	colorArray[COLOR_INDEX_GREEN_LED] =(unsigned short)0;
	colorArray[COLOR_INDEX_BLUE_LED] =(unsigned short)0;
	}
	else{
	colorArray[COLOR_INDEX_REG_LED] =(unsigned short)0;
	colorArray[COLOR_INDEX_GREEN_LED] =(unsigned short)0;
	colorArray[COLOR_INDEX_BLUE_LED] =(unsigned short)100;
	}


	//printf("index(%d) r(%d) g(%d) b(%d)\n",colorArray[COLOR_INDEX_LED],colorArray[COLOR_INDEX_REG_LED],colorArray[COLOR_INDEX_GREEN_LED],colorArray[COLOR_INDEX_BLUE_LED]);
	write(cled_fd,&colorArray,6);

	close(cled_fd);


}
//**************************************dipsw************************************

int dipsw(void)
{

	int dipsw_fd;
	int retvalue;




	// open  driver
	dipsw_fd = open(DIPSW_DRIVER_NAME,O_RDWR);
	if ( dipsw_fd < 0 )
	{
		perror("driver  open error.\n");
		return 1;
	}
	read(dipsw_fd,&retvalue,4);
	retvalue &= 0xF;
	//printf("retvalue:0x%X\n",retvalue);
	//printf("******** %d ******",retvalue);
	close(dipsw_fd);

	return retvalue;
}





//********************************************busled*************************************

void busled(int speed)
{
	int ledNo = 0;
	int ledControl = 0;
	int wdata ,rdata,temp ;

	int bus_fd;



	// open  driver
	bus_fd = open(BUS_LED_DRIVER_NAME,O_RDWR);
	if ( bus_fd < 0 )
	{
		perror("driver (//dev//cnled) open error.\n");
		exit(1);
	}
	// control led

	/*if ( ledNo == 0 )
	{
		if ( ledControl ==  1 ) wdata = 0xff;
		else wdata = 0;
	}
	else
	{
		read(fd,&rdata,4);
		temp = 1;

		if ( ledControl ==1 )
		{
			temp <<=(ledNo-1);
			wdata = rdata | temp;
		}
		else
		{
			temp =  ~(temp<<(ledNo-1));
			wdata = rdata & temp;
		}
	}
	printf("wdata:0x%X\n",wdata);
	write(fd,&wdata,4);
        */

	if(speed < 4){
	wdata = 0x01;
	}
	else if(speed >=4 && speed < 8){
	wdata = 0x03;
	}
	else if(speed >=8 && speed <12){
	wdata = 0x07;
	}
	else if(speed >=12 && speed < 16){
	wdata = 0x0f;
	}
	else if(speed >=16 && speed < 20){
	wdata = 0x1f;
	}
	else if(speed >=20 && speed < 24){
	wdata = 0x3f;
	}
	else if(speed >=24 && speed < 28){
	wdata = 0x7f;
	}
	else{
	wdata = 0xff;
	}
	//printf("wdata:0x%X\n",wdata);
	write(bus_fd,&wdata,4);

	close(bus_fd);


}




/****************************************CAMERA********************************************/

#define V4L2_CID_CACHEABLE         (V4L2_CID_BASE+40)
#define FBDEV_FILE "/dev/fb0"
#define BIT_VALUE_24BIT 24

static int m_preview_v4lformat = V4L2_PIX_FMT_RGB565;
//static int m_preview_v4lformat = V4L2_PIX_FMT_YUV422P;
static int  m_cam_fd;
static struct SecBuffer m_buffers_preview[MAX_BUFFERS];
static struct pollfd   m_events_c;
static int   screen_width;
static int   screen_height;
static int   bits_per_pixel;
static int   line_length;

// filename의 파일을 열어 순수 bitmap data은 *data 주소에 저장하고 , pDib은
// malloc 해제하기 위한 메모리 포이트이다.
void read_bmp(char *filename, char **pDib, char **data, int *cols, int *rows)
{
  BITMAPFILEHEADER   bmpHeader;
  BITMAPINFOHEADER   *bmpInfoHeader;
  unsigned int size;
  unsigned char magicNum[2];
  int nread;
  FILE *fp;
  fp = fopen(filename, "rb");
  if(fp == NULL) {
   printf("ERROR\n");
   return;
 }

  magicNum[0] = fgetc(fp);
  magicNum[1] = fgetc(fp);


  printf("magicNum : %c%c\n", magicNum[0], magicNum[1]);

  if(magicNum[0] != 'B' && magicNum[1] != 'M') {
      printf("It's not a bmp file!\n");
      fclose(fp);
      return;
  }

  nread = fread(&bmpHeader.bfSize, 1, sizeof(BITMAPFILEHEADER), fp);

  size = bmpHeader.bfSize - sizeof(BITMAPFILEHEADER);
  *pDib = (unsigned char *)malloc(size);

  fread(*pDib, 1, size, fp);

  bmpInfoHeader = (BITMAPINFOHEADER *)*pDib;

  printf("nread : %d\n", nread);
  printf("size : %d\n", size);

  if(BIT_VALUE_24BIT != (bmpInfoHeader->biBitCount))
  {
    printf("It supports only 24bit bmp!\n");
    fclose(fp);
    return;
  }
  *cols = bmpInfoHeader->biWidth;
  *rows = bmpInfoHeader->biHeight;
  *data = (char *)(*pDib + bmpHeader.bfOffBits - sizeof(bmpHeader) - 2);

  fclose(fp);

}



void close_bmp(char **pDib)
{
  free(*pDib); // 메모리 해제
}

static int close_buffers(struct SecBuffer *buffers, int num_of_buf)
{
	int ret,i,j;

	for ( i = 0; i < num_of_buf; i++) {
		for( j = 0; j < MAX_PLANES; j++) {
			if (buffers[i].virt.extP[j]) {
				ret = munmap(buffers[i].virt.extP[j], buffers[i].size.extS[j]);
				buffers[i].virt.extP[j] = NULL;
			}
		}
	}

	return 0;
}

static int get_pixel_depth(unsigned int fmt)
{
	int depth = 0;

	switch (fmt) {
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_YUV420:
	case V4L2_PIX_FMT_YVU420:
		depth = 12;
		break;

	case V4L2_PIX_FMT_RGB565:
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_VYUY:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
	case V4L2_PIX_FMT_YUV422P:
		depth = 16;
		break;

	case V4L2_PIX_FMT_RGB32:
		depth = 32;
		break;
	}

	return depth;
}


static int fimc_poll(struct pollfd *events)
{
	int ret;

	/* 10 second delay is because sensor can take a long time
	* to do auto focus and capture in dark settings
	*/
	ret = poll(events, 1, 10000);
	if (ret < 0) {
		printf("ERR(%s):poll error\n", __func__);
		return ret;
	}

	if (ret == 0) {
		printf("ERR(%s):No data in 10 secs..\n", __func__);
		return ret;
	}

	return ret;
}

static int fimc_v4l2_querycap(int fp)
{
	struct v4l2_capability cap;

	if (ioctl(fp, VIDIOC_QUERYCAP, &cap) < 0) {
		printf("ERR(%s):VIDIOC_QUERYCAP failed\n", __func__);
		return -1;
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		printf("ERR(%s):no capture devices\n", __func__);
		return -1;
	}

	return 0;
	}

static const __u8* fimc_v4l2_enuminput(int fp, int index)
{
	static struct v4l2_input input;

	input.index = index;
	if (ioctl(fp, VIDIOC_ENUMINPUT, &input) != 0) {
		printf("ERR(%s):No matching index found\n", __func__);
		return NULL;
	}
	printf("Name of input channel[%d] is %s\n", input.index, input.name);

	return input.name;
}

static int fimc_v4l2_s_input(int fp, int index)
{
	struct v4l2_input input;

	input.index = index;

	if (ioctl(fp, VIDIOC_S_INPUT, &input) < 0) {
		printf("ERR(%s):VIDIOC_S_INPUT failed\n", __func__);
		return -1;
	}

	return 0;
}

static int fimc_v4l2_s_fmt(int fp, int width, int height, unsigned int fmt, enum v4l2_field field, unsigned int num_plane)
{
	struct v4l2_format v4l2_fmt;
	struct v4l2_pix_format pixfmt;
	//unsigned int framesize;

	memset(&v4l2_fmt, 0, sizeof(struct v4l2_format));
	v4l2_fmt.type = V4L2_BUF_TYPE;


	memset(&pixfmt, 0, sizeof(pixfmt));

	pixfmt.width = width;
	pixfmt.height = height;
	pixfmt.pixelformat = fmt;
	pixfmt.field = V4L2_FIELD_NONE;

	v4l2_fmt.fmt.pix = pixfmt;
	printf("fimc_v4l2_s_fmt : width(%d) height(%d)\n", width, height);

	/* Set up for capture */
	if (ioctl(fp, VIDIOC_S_FMT, &v4l2_fmt) < 0) {
		printf("ERR(%s):VIDIOC_S_FMT failed\n", __func__);
		return -1;
	}

	return 0;
}

static int fimc_v4l2_s_fmt_cap(int fp, int width, int height, unsigned int fmt)
{
	struct v4l2_format v4l2_fmt;
	struct v4l2_pix_format pixfmt;

	memset(&pixfmt, 0, sizeof(pixfmt));

	v4l2_fmt.type = V4L2_BUF_TYPE;

	pixfmt.width = width;
	pixfmt.height = height;
	pixfmt.pixelformat = fmt;
	if (fmt == V4L2_PIX_FMT_JPEG)
	pixfmt.colorspace = V4L2_COLORSPACE_JPEG;

	v4l2_fmt.fmt.pix = pixfmt;
	printf("fimc_v4l2_s_fmt_cap : width(%d) height(%d)\n", width, height);

	/* Set up for capture */
	if (ioctl(fp, VIDIOC_S_FMT, &v4l2_fmt) < 0) {
		printf("ERR(%s):VIDIOC_S_FMT failed\n", __func__);
		return -1;
	}

	return 0;
}

int fimc_v4l2_s_fmt_is(int fp, int width, int height, unsigned int fmt, enum v4l2_field field)
{
	struct v4l2_format v4l2_fmt;
	struct v4l2_pix_format pixfmt;

	memset(&pixfmt, 0, sizeof(pixfmt));

	v4l2_fmt.type = V4L2_BUF_TYPE_PRIVATE;

	pixfmt.width = width;
	pixfmt.height = height;
	pixfmt.pixelformat = fmt;
	pixfmt.field = field;

	v4l2_fmt.fmt.pix = pixfmt;
	printf("fimc_v4l2_s_fmt_is : width(%d) height(%d)\n", width, height);

	/* Set up for capture */
	if (ioctl(fp, VIDIOC_S_FMT, &v4l2_fmt) < 0) {
		printf("ERR(%s):VIDIOC_S_FMT failed\n", __func__);
		return -1;
	}

	return 0;
}

static int fimc_v4l2_enum_fmt(int fp, unsigned int fmt)
{
	struct v4l2_fmtdesc fmtdesc;
	int found = 0;

	fmtdesc.type = V4L2_BUF_TYPE;
	fmtdesc.index = 0;

	while (ioctl(fp, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
		if (fmtdesc.pixelformat == fmt) {
			printf("passed fmt = %#x found pixel format[%d]: %s\n", fmt, fmtdesc.index, fmtdesc.description);
			found = 1;
			break;
		}

		fmtdesc.index++;
	}

	if (!found) {
		printf("unsupported pixel format\n");
		return -1;
	}

	return 0;
}

static int fimc_v4l2_reqbufs(int fp, enum v4l2_buf_type type, int nr_bufs)
{
	struct v4l2_requestbuffers req;

	req.count = nr_bufs;
	req.type = type;
	req.memory = V4L2_MEMORY_TYPE;

	if (ioctl(fp, VIDIOC_REQBUFS, &req) < 0) {
		printf("ERR(%s):VIDIOC_REQBUFS failed\n", __func__);
		return -1;
	}

	return req.count;
}

static int fimc_v4l2_querybuf(int fp, struct SecBuffer *buffers, enum v4l2_buf_type type, int nr_frames, int num_plane)
{
	struct v4l2_buffer v4l2_buf;

	int i, ret ;

	for (i = 0; i < nr_frames; i++) {
		v4l2_buf.type = type;
		v4l2_buf.memory = V4L2_MEMORY_TYPE;
		v4l2_buf.index = i;

		ret = ioctl(fp, VIDIOC_QUERYBUF, &v4l2_buf); // query video buffer status
		if (ret < 0) {
			printf("ERR(%s):VIDIOC_QUERYBUF failed\n", __func__);
			return -1;
		}

		buffers[i].size.s = v4l2_buf.length;

		if ((buffers[i].virt.p = (char *)mmap(0, v4l2_buf.length, PROT_READ | PROT_WRITE, MAP_SHARED,
			fp, v4l2_buf.m.offset)) < 0) {
			printf("%s %d] mmap() failed",__func__, __LINE__);
			return -1;
		}
		printf("buffers[%d].virt.p = %p v4l2_buf.length = %d\n", i, buffers[i].virt.p, v4l2_buf.length);
	}
	return 0;
}

static int fimc_v4l2_streamon(int fp)
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE;
	int ret;

	ret = ioctl(fp, VIDIOC_STREAMON, &type);
	if (ret < 0) {
		printf("ERR(%s):VIDIOC_STREAMON failed\n", __func__);
		return ret;
	}

	return ret;
}

static int fimc_v4l2_streamoff(int fp)
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE;
	int ret;

	printf("%s :", __func__);
	ret = ioctl(fp, VIDIOC_STREAMOFF, &type);
	if (ret < 0) {
		printf("ERR(%s):VIDIOC_STREAMOFF failed\n", __func__);
		return ret;
	}

	return ret;
}

static int fimc_v4l2_qbuf(int fp, int index )
{
	struct v4l2_buffer v4l2_buf;
	int ret;

	v4l2_buf.type = V4L2_BUF_TYPE;
	v4l2_buf.memory = V4L2_MEMORY_TYPE;
	v4l2_buf.index = index;


	ret = ioctl(fp, VIDIOC_QBUF, &v4l2_buf);
	if (ret < 0) {
		printf("ERR(%s):VIDIOC_QBUF failed\n", __func__);
		return ret;
	}

	return 0;
}

static int fimc_v4l2_dqbuf(int fp, int num_plane)
{
	struct v4l2_buffer v4l2_buf;
	int ret;

	v4l2_buf.type = V4L2_BUF_TYPE;
	v4l2_buf.memory = V4L2_MEMORY_TYPE;

	ret = ioctl(fp, VIDIOC_DQBUF, &v4l2_buf);
	if (ret < 0) {
		printf("ERR(%s):VIDIOC_DQBUF failed, dropped frame\n", __func__);
		return ret;
	}

	return v4l2_buf.index;
}

static int fimc_v4l2_g_ctrl(int fp, unsigned int id)
{
	struct v4l2_control ctrl;
	int ret;

	ctrl.id = id;

	ret = ioctl(fp, VIDIOC_G_CTRL, &ctrl);
	if (ret < 0) {
		printf("ERR(%s): VIDIOC_G_CTRL(id = 0x%x (%d)) failed, ret = %d\n",
			 __func__, id, id-V4L2_CID_PRIVATE_BASE, ret);
		return ret;
	}

	return ctrl.value;
}

static int fimc_v4l2_s_ctrl(int fp, unsigned int id, unsigned int value)
{
	struct v4l2_control ctrl;
	int ret;

	ctrl.id = id;
	ctrl.value = value;

	ret = ioctl(fp, VIDIOC_S_CTRL, &ctrl);
	if (ret < 0) {
		printf("ERR(%s):VIDIOC_S_CTRL(id = %#x (%d), value = %d) failed ret = %d\n",
		 __func__, id, id-V4L2_CID_PRIVATE_BASE, value, ret);

		return ret;
	}

	return ctrl.value;
}

static int fimc_v4l2_s_ext_ctrl(int fp, unsigned int id, void *value)
{
	struct v4l2_ext_controls ctrls;
	struct v4l2_ext_control ctrl;
	int ret;

	ctrl.id = id;

	ctrls.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
	ctrls.count = 1;
	ctrls.controls = &ctrl;

	ret = ioctl(fp, VIDIOC_S_EXT_CTRLS, &ctrls);
	if (ret < 0)
		printf("ERR(%s):VIDIOC_S_EXT_CTRLS failed\n", __func__);

	return ret;
}



static int fimc_v4l2_g_parm(int fp, struct v4l2_streamparm *streamparm)
{
	int ret;

	streamparm->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ret = ioctl(fp, VIDIOC_G_PARM, streamparm);
	if (ret < 0) {
		printf("ERR(%s):VIDIOC_G_PARM failed\n", __func__);
		return -1;
	}

	printf("%s : timeperframe: numerator %d, denominator %d\n", __func__,
			streamparm->parm.capture.timeperframe.numerator,
			streamparm->parm.capture.timeperframe.denominator);

	return 0;
}

static int fimc_v4l2_s_parm(int fp, struct v4l2_streamparm *streamparm)
{
	int ret;

	streamparm->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ret = ioctl(fp, VIDIOC_S_PARM, streamparm);
	if (ret < 0) {
		printf("ERR(%s):VIDIOC_S_PARM failed\n", __func__);
		return ret;
	}

	return 0;
}

int CreateCamera(int index)
{
	printf("%s :\n", __func__);
	int ret = 0;

	m_cam_fd = open(CAMERA_DEV_NAME, O_RDWR);
	if (m_cam_fd < 0) {
		printf("ERR(%s):Cannot open %s (error : %s)\n", __func__, CAMERA_DEV_NAME, strerror(errno));
		return -1;
	}
	printf("%s: open(%s) --> m_cam_fd %d\n", __func__, CAMERA_DEV_NAME, m_cam_fd);

	ret = fimc_v4l2_querycap(m_cam_fd);
	CHECK(ret);
	if (!fimc_v4l2_enuminput(m_cam_fd, index)) {
		printf("m_cam_fd(%d) fimc_v4l2_enuminput fail\n", m_cam_fd);
		return -1;
	}
	ret = fimc_v4l2_s_input(m_cam_fd, index);
	CHECK(ret);


	return 0;
}
void DestroyCamera()
{
	if (m_cam_fd > -1) {
		close(m_cam_fd);
		m_cam_fd = -1;
	}

}

int startPreview(void)
{
	int i;
	//v4l2_streamparm streamparm;
	printf("%s :\n", __func__);

	if (m_cam_fd <= 0) {
		printf("ERR(%s):Camera was closed\n", __func__);
		return -1;
	}

	memset(&m_events_c, 0, sizeof(m_events_c));
	m_events_c.fd = m_cam_fd;
	m_events_c.events = POLLIN | POLLERR;

	/* enum_fmt, s_fmt sample */
	int ret = fimc_v4l2_enum_fmt(m_cam_fd,m_preview_v4lformat);
	CHECK(ret);

	ret = fimc_v4l2_s_fmt(m_cam_fd, CAMERA_PREVIEW_WIDTH, CAMERA_PREVIEW_HEIGHT, m_preview_v4lformat, V4L2_FIELD_ANY, PREVIEW_NUM_PLANE);
	CHECK(ret);

	fimc_v4l2_s_fmt_is(m_cam_fd, CAMERA_PREVIEW_WIDTH, CAMERA_PREVIEW_HEIGHT,
		m_preview_v4lformat, (enum v4l2_field) IS_MODE_PREVIEW_STILL);

	CHECK(ret);

	ret = fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CACHEABLE, 1);
	CHECK(ret);

	ret = fimc_v4l2_reqbufs(m_cam_fd, V4L2_BUF_TYPE, MAX_BUFFERS);
	CHECK(ret);

	ret = fimc_v4l2_querybuf(m_cam_fd, m_buffers_preview, V4L2_BUF_TYPE, MAX_BUFFERS, PREVIEW_NUM_PLANE);
	CHECK(ret);

	/* start with all buffers in queue  to capturer video */
	for (i = 0; i < MAX_BUFFERS; i++) {
		ret = fimc_v4l2_qbuf(m_cam_fd,  i);
		CHECK(ret);
	}

	ret = fimc_v4l2_streamon(m_cam_fd);
	CHECK(ret);

	printf("%s: got the first frame of the preview\n", __func__);

	return 0;
}

int stopPreview(void)
{
	int ret;

	if (m_cam_fd <= 0) {
		printf("ERR(%s):Camera was closed\n", __func__);
		return -1;
	}

	ret = fimc_v4l2_streamoff(m_cam_fd);
	CHECK(ret);

	close_buffers(m_buffers_preview, MAX_BUFFERS);

	fimc_v4l2_reqbufs(m_cam_fd, V4L2_BUF_TYPE, 0);

	return ret;
}

void initScreen(unsigned char *fb_mem, struct fb_var_screeninfo fbvar, struct fb_fix_screeninfo fbfix ,
int fbfd)
{


    int i, j, k, t;
    int bits_per_pixel;
    int line_length;
    int coor_x, coor_y;
    int cols = 0, rows = 0;
int mem_size;
    char *pData, *data;
    char r, g, b;
    unsigned long bmpdata[1280*800];
    unsigned long pixel;
    unsigned char *pfbmap;
    unsigned long *ptr;

    char* file_name = "bike.bmp";

    read_bmp(file_name, &pData, &data, &cols, &rows);

    printf("Bitmap : cols = %d, rows = %d\n", cols, rows);


    for(j = 0; j < rows; j++)
    {
        k = j * cols * 3;
        t = (rows - 1 - j) * cols; // 가로 size가 작을 수도 있다.

        for(i = 0; i < cols; i++)
        {
            b = *(data + (k + i * 3));
            g = *(data + (k + i * 3 + 1));
            r = *(data + (k + i * 3 + 2));

            pixel = ((r<<16) | (g<<8) | b);
            bmpdata[t+i] = pixel;
        }
    }
    close_bmp(&pData); // 메모리 해제


    screen_width = fbvar.xres;
    screen_height = fbvar.yres;
    bits_per_pixel = fbvar.bits_per_pixel;
    line_length  = fbfix.line_length;
    mem_size = line_length * screen_height;

    printf("screen_width : %d\n", screen_width);
    printf("screen_height : %d\n", screen_height);
    printf("bits_per_pixel : %d\n", bits_per_pixel);
    printf("line_length : %d\n", line_length);

    pfbmap = (unsigned char *) mmap(0, mem_size, PROT_READ|PROT_WRITE, MAP_SHARED, fbfd, 0);

	// distance control
	int offset_x = 640;
	int offset_y = (screen_height - rows)/2;



    if ((unsigned)pfbmap == (unsigned)-1)
    {
        perror("fbdev mmap\n");
        exit(1);
    }
    for(coor_y = 0; coor_y < screen_height; coor_y++)
    {

        ptr =  (unsigned long *)pfbmap + (screen_width * coor_y);

        for(coor_x = 0; coor_x < screen_width; coor_x++){
            *ptr++ = 0xFFFFFF;
        }
    }

    for(coor_y = offset_y; coor_y < rows +offset_y; coor_y++) {

        ptr = (unsigned long*)pfbmap + (screen_width * coor_y);

	for(coor_x = 0; coor_x < offset_x; coor_x++){
            *ptr++ = 0xFFFFFF;
        }

        for (coor_x = offset_x; coor_x < cols +offset_x; coor_x++) {

            *ptr++ = bmpdata[coor_x-offset_x + (coor_y-offset_y)*cols];

        }
    }

    munmap( pfbmap, mem_size);
  //  close( fbfd);


}


static void yuv_a_rgb(unsigned char y, unsigned char u, unsigned char v,
               unsigned char* r, unsigned char* g, unsigned char* b)
{
	int amp=250;
	double R,G,B;

	R=amp*(0.004565*y+0.000001*u+0.006250*v-0.872);
	G=amp*(0.004565*y-0.001542*u-0.003183*v+0.531);
	B=amp*(0.004565*y+0.007935*u+/*0.000000*v*/-1.088);
	//printf("\nR = %f   G = %f   B = %f", R, G, B);

	if (R < 0)
		R=0;
	if (G < 0)
		G=0;
	if (B < 0)
		B=0;

	if (R > 255)
		R=255;
	if (G > 255)
		G=255;
	if (B > 255)
		B=255;

	*r=(unsigned char)(R);
	*g=(unsigned char)(G);
	*b=(unsigned char)(B);

}

static void Draw(unsigned char *displayFrame, unsigned char *videoFrame,int videoWidth, int videoHeight, \
				 int dFrameWidth,int dFrameHeight)
{
	int    x,y;
	unsigned char *offsetU;
	unsigned char *offsetV;
	unsigned char Y,U,V;
	unsigned char R,G,B;
	int lineLeng ;
	lineLeng = dFrameWidth*4;

	offsetV = videoFrame + videoWidth*videoHeight;
	offsetU = videoFrame + videoWidth*videoHeight + videoWidth*videoHeight/2;

	for ( y = 0 ; y < videoHeight ; y++ )
	{
		for(x = 0; x < videoWidth ; x++ )
		{
			Y = *(videoFrame + x + y*videoWidth);
			U = *(offsetU + (x + y*videoWidth)/2);
			V = *(offsetV + (x + y*videoWidth)/2);

			yuv_a_rgb(Y, U, V, &R, &G, &B);

			displayFrame[y*lineLeng + x*4 + 0] = R;
			displayFrame[y*lineLeng + x*4 + 1] = G;
			displayFrame[y*lineLeng + x*4 + 2] = B;
		}
	}
}
static void DrawFromRGB565(unsigned char *displayFrame, unsigned char *videoFrame,int videoWidth, int videoHeight, \
				 int dFrameWidth,int dFrameHeight)
{
	int    x,y;
	int lineLeng ;
	unsigned short *videptrTemp;
	unsigned short *videoptr = videoFrame;
	int temp;
	lineLeng = dFrameWidth*4;

	for ( y = 0 ; y < videoHeight ; y++ )
	{
		for(x = 0; x < videoWidth ;)
		{

			videptrTemp =  videoptr + videoWidth*y + x ;
			temp = y*lineLeng + x*4;
			displayFrame[temp + 2] = (unsigned char)((*videptrTemp & 0xF800) >> 8)  ;
			displayFrame[temp + 1] = (unsigned char)((*videptrTemp & 0x07E0) >> 3)  ;
			displayFrame[temp + 0] = (unsigned char)((*videptrTemp & 0x001F) << 3)  ;

			videptrTemp++;
			temp +=4;
			displayFrame[temp + 2] = (unsigned char)((*videptrTemp & 0xF800) >> 8)  ;
			displayFrame[temp + 1] = (unsigned char)((*videptrTemp & 0x07E0) >> 3)  ;
			displayFrame[temp + 0] = (unsigned char)((*videptrTemp & 0x001F) << 3)  ;

			videptrTemp++;
			temp +=4;
			displayFrame[temp + 2] = (unsigned char)((*videptrTemp & 0xF800) >> 8)  ;
			displayFrame[temp + 1] = (unsigned char)((*videptrTemp & 0x07E0) >> 3)  ;
			displayFrame[temp + 0] = (unsigned char)((*videptrTemp & 0x001F) << 3)  ;

			videptrTemp++;
			temp +=4;
			displayFrame[temp + 2] = (unsigned char)((*videptrTemp & 0xF800) >> 8)  ;
			displayFrame[temp + 1] = (unsigned char)((*videptrTemp & 0x07E0) >> 3)  ;
			displayFrame[temp + 0] = (unsigned char)((*videptrTemp & 0x001F) << 3)  ;
			x+=4;
		}
	}
}

#define  FBDEV_FILE "/dev/fb0"


void camera_start(){

  int     fb_fd;
	int	    ret;
	int     index;


	struct  fb_var_screeninfo fbvar;
	struct  fb_fix_screeninfo fbfix;
	unsigned char   *fb_mapped;
	int             mem_size;


	if( access(FBDEV_FILE, F_OK) )
	{
		printf("%s: access error\n", FBDEV_FILE);
		return 1;
	}

	if( (fb_fd = open(FBDEV_FILE, O_RDWR)) < 0)
	{
		printf("%s: open error\n", FBDEV_FILE);
		return 1;
	}

	if( ioctl(fb_fd, FBIOGET_VSCREENINFO, &fbvar) )
	{
		printf("%s: ioctl error - FBIOGET_VSCREENINFO \n", FBDEV_FILE);
		goto fb_err;
	}

	if( ioctl(fb_fd, FBIOGET_FSCREENINFO, &fbfix) )
	{
		printf("%s: ioctl error - FBIOGET_FSCREENINFO \n", FBDEV_FILE);
		goto fb_err;
	}

	screen_width    =   fbvar.xres;
	screen_height   =   fbvar.yres;
	bits_per_pixel  =   fbvar.bits_per_pixel;
	line_length     =   fbfix.line_length;

	printf("screen_width : %d\n", screen_width);
	printf("screen_height : %d\n", screen_height);
	printf("bits_per_pixel : %d\n", bits_per_pixel);
	printf("line_length : %d\n", line_length);

	mem_size    =   screen_width * screen_height * 4;
	fb_mapped   =   (unsigned char *)mmap(0, mem_size,
		 PROT_READ|PROT_WRITE, MAP_SHARED, fb_fd, 0);
	if (fb_mapped < 0)
	{
		printf("mmap error!\n");
		goto fb_err;
	}

	initScreen(fb_mapped,fbvar,fbfix ,fb_fd);

	CreateCamera(0);
	startPreview();

	while(1)
	{
		ret = fimc_poll(&m_events_c);
		CHECK_PTR(ret);
		index = fimc_v4l2_dqbuf(m_cam_fd, 1);

		DrawFromRGB565(fb_mapped, m_buffers_preview[index].virt.p,CAMERA_PREVIEW_WIDTH,\
		CAMERA_PREVIEW_HEIGHT,screen_width,screen_height);

		ret = fimc_v4l2_qbuf(m_cam_fd,index);
	}

	stopPreview();
	DestroyCamera();
	fb_err:
	close(fb_fd);

	exit(1);
}



/*****************************************CAMERA********************************************/



//****************************************main*************************************
int main(void){


	int limitSpeed=0;
	int distance = 920220;
	int direction=1;
	int speed = 22;
	int row1=1;
	int row2=2;
        int j=0;
	char words1[3][15] = {"kangin220","aaaaaaaaa","bbbbbbbbb"};
	char words2[3][15] = {"sunho220","cccccccccc","dddd"};

	/*for(j=0;j<3;j++){
	textLED(row1,words1[j],row2,words2[j]);
	sleep(1);
	}*/

	if(keyMatrix()==1){
	textLED(row1,words1[0],row2,words2[0]);
	dotMatrix(speed);


 	limitSpeed = dipsw();
	if(limitSpeed == 1){
	limitSpeed = 10;
	}
	else if(limitSpeed == 3){
	limitSpeed = 20;
	}
	else if(limitSpeed == 7){
	limitSpeed = 30;
	}
	else{
	limitSpeed = 40;
	}
	fndLed(distance);
	oLed(direction);

	busled(speed);
	cled(speed);
	speedBuzzer(speed,limitSpeed);

}
	else{

	pwBuzzer();
	printf("Wrong passward!!!!");
	}
	

	return 0;
}
