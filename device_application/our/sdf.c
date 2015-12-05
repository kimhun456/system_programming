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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define TLCD_TRUE        1
#define TLCD_FALSE        0

#define TLCD_SUCCESS        0
#define TLCD_FAIL        1

static int  tlcd_fd ;

#define TLCD_LINE_NUM            2
#define TLCD_COLUMN_NUM            16


#define TLCD_CMD_TXT_WRITE        0
#define TLCD_CMD_CURSOR_POS        1
#define TLCD_CMD_CEAR_SCREEN        2


#define TLCD_DRIVER_NAME        "/dev/cntlcd"
/******************************************************************************
*
*      TEXT LCD FUNCTION
*
******************************************************************************/
#define TLCD_CLEAR_DISPLAY        0x0001
#define TLCD_CURSOR_AT_HOME        0x0002

// Entry Mode set
#define TLCD_MODE_SET_DEF        0x0004
#define TLCD_MODE_SET_DIR_RIGHT    0x0002
#define TLCD_MODE_SET_SHIFT        0x0001

// Display on off
#define TLCD_DIS_DEF                0x0008
#define TLCD_DIS_LCD                0x0004
#define TLCD_DIS_CURSOR            0x0002
#define TLCD_DIS_CUR_BLINK        0x0001

// shift
#define TLCD_CUR_DIS_DEF            0x0010
#define TLCD_CUR_DIS_SHIFT        0x0008
#define TLCD_CUR_DIS_DIR            0x0004

// set DDRAM  address
#define TLCD_SET_DDRAM_ADD_DEF    0x0080

// read bit
#define TLCD_BUSY_BIT            0x0080
#define TLCD_DDRAM_ADD_MASK        0x007F


#define TLCD_DDRAM_ADDR_LINE_1    0x0000
#define TLCD_DDRAM_ADDR_LINE_2    0x0040


#define TLCD_SIG_BIT_E            0x0400
#define TLCD_SIG_BIT_RW            0x0200
#define TLCD_SIG_BIT_RS            0x0100

/*
  TLCD function
*/
int tlcd_isbusy(void)
{
    unsigned short wdata, rdata;

    wdata = TLCD_SIG_BIT_RW;
    write(tlcd_fd ,&wdata,2);

    wdata = TLCD_SIG_BIT_RW | TLCD_SIG_BIT_E;
    write(tlcd_fd ,&wdata,2);

    read(tlcd_fd,&rdata ,2);

    wdata = TLCD_SIG_BIT_RW;
    write(tlcd_fd,&wdata,2);

    if (rdata &  TLCD_BUSY_BIT)
        return TLCD_TRUE;

    return TLCD_FALSE;
}
int tlcd_writeCmd(unsigned short cmd)
{
    unsigned short wdata ;

    if ( tlcd_isbusy())
        return TLCD_FALSE;

    wdata = cmd;
    write(tlcd_fd ,&wdata,2);

    wdata = cmd | TLCD_SIG_BIT_E;
    write(tlcd_fd ,&wdata,2);

    wdata = cmd ;
    write(tlcd_fd ,&wdata,2);

    return TLCD_TRUE;
}

int tlcd_setDDRAMAddr(int x , int y)
{
    unsigned short cmd = 0;
//    printf("x :%d , y:%d \n",x,y);
    if(tlcd_isbusy())
    {
        perror("setDDRAMAddr busy error.\n");
        return TLCD_FALSE;

    }

    if ( y == 1 )
    {
        cmd = TLCD_DDRAM_ADDR_LINE_1 +x;
    }
    else if(y == 2 )
    {
        cmd = TLCD_DDRAM_ADDR_LINE_2 +x;
    }
    else
        return TLCD_FALSE;

    if ( cmd >= 0x80)
        return TLCD_FALSE;


//    printf("setDDRAMAddr w1 :0x%X\n",cmd);

    if (!tlcd_writeCmd(cmd | TLCD_SET_DDRAM_ADD_DEF))
    {
        perror("setDDRAMAddr error\n");
        return TLCD_FALSE;
    }
//    printf("setDDRAMAddr w :0x%X\n",cmd|SET_DDRAM_ADD_DEF);
    usleep(1000);
    return TLCD_TRUE;
}


int tlcd_writeCh(unsigned short ch)
{
    unsigned short wdata =0;

    if ( tlcd_isbusy())
        return TLCD_FALSE;

    wdata = TLCD_SIG_BIT_RS | ch;
    write(tlcd_fd ,&wdata,2);

    wdata = TLCD_SIG_BIT_RS | ch | TLCD_SIG_BIT_E;
    write(tlcd_fd ,&wdata,2);

    wdata = TLCD_SIG_BIT_RS | ch;
    write(tlcd_fd ,&wdata,2);
    usleep(1000);
    return TLCD_TRUE;

}



int tlcd_functionSet(void)
{
    unsigned short cmd = 0x0038; // 5*8 dot charater , 8bit interface , 2 line

    if (!tlcd_writeCmd(cmd))
        return TLCD_FALSE;
    return TLCD_TRUE;
}

int tlcd_writeStr(char* str)
{
    unsigned char wdata;
    int i;
    for(i =0; i < strlen(str) ;i++ )
    {
        if (str[i] == '_')
            wdata = (unsigned char)' ';
        else
            wdata = str[i];
        tlcd_writeCh(wdata);
    }
    return TLCD_TRUE;

}


int tlcd_clearScreen(int nline)
{
	int i;
	if (nline == 0)
	{
		if(tlcd_isbusy())
		{
			perror("clearScreen error\n");
			return TLCD_FALSE;
		}
		if (!tlcd_writeCmd(TLCD_CLEAR_DISPLAY))
			return TLCD_FALSE;
		return TLCD_TRUE;
	}
	else if (nline == 1)
	{
		tlcd_setDDRAMAddr(0,1);
		for(i = 0; i <= TLCD_COLUMN_NUM ;i++ )
		{
			tlcd_writeCh((unsigned char)' ');
		}
		tlcd_setDDRAMAddr(0,1);

	}
	else if (nline == 2)
	{
		tlcd_setDDRAMAddr(0,2);
		for(i = 0; i <= TLCD_COLUMN_NUM ;i++ )
		{
			tlcd_writeCh((unsigned char)' ');
		}
		tlcd_setDDRAMAddr(0,2);
	}
	return TLCD_TRUE;
}



void textLcd(int row1 , char *str1 , int row2 , char *str2)
{

    int nCmdMode;
    int bCursorOn, bBlink, nline ;
    int nColumn=0;
    char strWtext1[TLCD_COLUMN_NUM+1];
    char strWtext2[TLCD_COLUMN_NUM+1];


        nCmdMode =  TLCD_CMD_TXT_WRITE ;

        if (strlen(str1) > TLCD_COLUMN_NUM )
        {
            strncpy(strWtext1,str1,TLCD_COLUMN_NUM);
            strWtext1[TLCD_COLUMN_NUM] = '\0';
        }

        else
            strcpy(strWtext1,str1);


        if(strlen(str2) > TLCD_COLUMN_NUM)
        {

            strncpy(strWtext2,str2,TLCD_COLUMN_NUM);
            strWtext2[TLCD_COLUMN_NUM] = '\0';

        }
        else
            strcpy(strWtext2,str2);


    // open  driver
    tlcd_fd = open(TLCD_DRIVER_NAME,O_RDWR);
    if ( tlcd_fd < 0 )
    {
        perror("driver open error.\n");
        return 1;
    }

    tlcd_functionSet();
    tlcd_clearScreen(1);
    tlcd_clearScreen(2);

    tlcd_setDDRAMAddr(nColumn, row1);
    usleep(2000);
    tlcd_writeStr(strWtext1);
    tlcd_setDDRAMAddr(nColumn, row2);
    usleep(2000);
    tlcd_writeStr(strWtext2);

    close(tlcd_fd);

}


int main(){

  textLcd( 1 , "asdfasdf" ,  2 , "qwerqwer");
  return 0;
}
