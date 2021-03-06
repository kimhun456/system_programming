#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> // for O_RDWR
#include <sys/ioctl.h> // for ioctl
#include <sys/mman.h>
#include <linux/fb.h>
#include "bitmap.h"

#define FBDEV_FILE "/dev/fb0"

#define BIT_VALUE_24BIT 24


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




int main ()
{
    int i, j, k, t;
    int fbfd;
    int screen_width;
    int screen_height;
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


    struct fb_var_screeninfo fbvar;
    struct fb_fix_screeninfo fbfix;
    printf("=================================\n");
    printf("Frame buffer Application - Bitmap\n");
    printf("=================================\n\n");


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

    if( (fbfd = open(FBDEV_FILE, O_RDWR)) < 0) //
    {
        printf("%s: open error\n", FBDEV_FILE);
        exit(1);
    }

    if( ioctl(fbfd, FBIOGET_VSCREENINFO, &fbvar) )
    {
        printf("%s: ioctl error - FBIOGET_VSCREENINFO \n", FBDEV_FILE);
        exit(1);
    }


    if( ioctl(fbfd, FBIOGET_FSCREENINFO, &fbfix) ) // screen info를 얻어옴
    {
        printf("%s: ioctl error - FBIOGET_FSCREENINFO \n", FBDEV_FILE);
        exit(1);
    }


    if (fbvar.bits_per_pixel != 32)
    {
        fprintf(stderr, "bpp is not 32\n");
        exit(1);
    }

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
    close( fbfd);
    return 0;
}
