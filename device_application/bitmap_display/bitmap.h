
// BMP file의 헤더의 구조를 구조체로 만들어 놓음;
typedef struct {
  //  unsigned char bfType;
  unsigned int bfSize;
  unsigned short bfReserved1;
  unsigned short bfReserved2;
  unsigned int bfOffBits;

}BITMAPFILEHEADER;

// Image Header
typedef struct {
  unsigned int biSize;
  unsigned int biWidth;
  unsigned int biHeight;
  unsigned short biPlanes;
  unsigned short biBitCount;
  unsigned int biCompression;
  unsigned int biSizeImage;
  unsigned int biXPelsPerMeter;
  unsigned int biYPelsPerMeter;
  unsigned int biClrUsed;
  unsigned int biClrImportant;
} BITMAPINFOHEADER;

typedef struct {
   // windows version 3
  unsigned char rgbBlue; // 1 byte
  unsigned char rgbGreen; // 1 byte
  unsigned char rgbRed; // 1 byte
  unsigned char rgbReserved; // 1 byte

} RGBQUAD;

typedef struct {
  BITMAPINFOHEADER bmiHeader;
  RGBQUAD bmiColors[1];
} BITMAPINFO;
