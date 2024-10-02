//
//  main.cpp
//  truetype_test
//
//  Created by Laurence Bank on 10/2/24.
//

#include "truetype.h"

#define BITMAP_WIDTH 800
#define BITMAP_HEIGHT 480

/* Windows BMP header for RGB565 images */
uint8_t winbmphdr_rgb565[138] =
        {0x42,0x4d,0,0,0,0,0,0,0,0,0x8a,0,0,0,0x7c,0,
         0,0,0,0,0,0,0,0,0,0,1,0,8,0,3,0,
         0,0,0,0,0,0,0x13,0x0b,0,0,0x13,0x0b,0,0,0,0,
         0,0,0,0,0,0,0,0xf8,0,0,0xe0,0x07,0,0,0x1f,0,
         0,0,0,0,0,0,0x42,0x47,0x52,0x73,0,0,0,0,0,0,
         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
         0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,
         0,0,0,0,0,0,0,0,0,0};

/* Windows BMP header for 8/24/32-bit images (54 bytes) */
uint8_t winbmphdr[54] =
        {0x42,0x4d,
         0,0,0,0,         /* File size */
         0,0,0,0,0x36,4,0,0,0x28,0,0,0,
         0,0,0,0, /* Xsize */
         0,0,0,0, /* Ysize */
         1,0,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,       /* number of planes, bits per pel */
         0,0,0,0};
//
// Minimal code to save frames as Windows BMP files
//
void WriteBMP(char *fname, uint8_t *pBitmap, uint8_t *pPalette, int cx, int cy, int bpp)
{
FILE * oHandle;
int i, bsize, lsize;
uint32_t *l;
uint8_t *s;
uint8_t *ucTemp;
uint8_t *pHdr;
int iHeaderSize;

    ucTemp = (uint8_t *)malloc(cx * 4);

    if (bpp == 16) {
        pHdr = winbmphdr_rgb565;
        iHeaderSize = sizeof(winbmphdr_rgb565);
    } else {
        pHdr = winbmphdr;
        iHeaderSize = sizeof(winbmphdr);
    }
    
    oHandle = fopen(fname, "w+b");
    bsize = (cx * bpp) >> 3;
    lsize = (bsize + 3) & 0xfffc; /* Width of each line */
    pHdr[26] = 1; // number of planes
    pHdr[28] = (uint8_t)bpp;

   /* Write the BMP header */
   l = (uint32_t *)&pHdr[2];
    i =(cy * lsize) + iHeaderSize;
    if (bpp <= 8)
        i += 1024;
   *l = (uint32_t)i; /* Store the file size */
   l = (uint32_t *)&pHdr[34]; // data size
   i = (cy * lsize);
   *l = (uint32_t)i; // store data size
   l = (uint32_t *)&pHdr[18];
   *l = (uint32_t)cx;      /* width */
   *(l+1) = (uint32_t)cy;  /* height */
    l = (uint32_t *)&pHdr[10]; // OFFBITS
    if (bpp <= 8) {
        *l = iHeaderSize + 1024;
    } else { // no palette
        *l = iHeaderSize;
    }
   fwrite(pHdr, 1, iHeaderSize, oHandle);
    if (bpp <= 8) {
    if (pPalette == NULL) {// create a grayscale palette
        int iDelta, iCount = 1<<bpp;
        int iGray = 0;
        iDelta = 255/(iCount-1);
        for (i=0; i<iCount; i++) {
            ucTemp[i*4+0] = (uint8_t)iGray;
            ucTemp[i*4+1] = (uint8_t)iGray;
            ucTemp[i*4+2] = (uint8_t)iGray;
            ucTemp[i*4+3] = 0;
            iGray += iDelta;
        }
    } else {
        for (i=0; i<256; i++) // change palette to WinBMP format
        {
            ucTemp[i*4 + 0] = pPalette[(i*3)+2];
            ucTemp[i*4 + 1] = pPalette[(i*3)+1];
            ucTemp[i*4 + 2] = pPalette[(i*3)+0];
            ucTemp[i*4 + 3] = 0;
        }
    }
    fwrite(ucTemp, 1, 1024, oHandle);
    } // palette write
   /* Write the image data */
   for (i=cy-1; i>=0; i--)
    {
        s = &pBitmap[i*bsize];
        if (bpp == 24) { // swap R/B for Windows BMP byte order
            int j, iBpp = bpp/8;
            uint8_t *d = ucTemp;
            for (j=0; j<cx; j++) {
                d[0] = s[2]; d[1] = s[1]; d[2] = s[0];
                d += iBpp; s += iBpp;
            }
            fwrite(ucTemp, 1, (size_t)lsize, oHandle);
        } else {
            fwrite(s, 1, (size_t)lsize, oHandle);
        }
    }
    free(ucTemp);
    fclose(oHandle);
} /* WriteBMP() */

int main(int argc, const char * argv[]) {
    truetypeClass truetype = truetypeClass();
    FILE *f;
    uint8_t *pBitmap, *pFile;
    int iSize;
    
    pBitmap = (uint8_t *)malloc(BITMAP_WIDTH * BITMAP_HEIGHT);
    printf("TrueType font rendering test\n");
    f = fopen("/Users/laurencebank/Downloads/Roboto/Roboto-Black.ttf", "r+b");
    fseek(f, 0, SEEK_END);
    iSize = (int)ftell(f);
    fseek(f, 0, SEEK_SET);
    pFile = (uint8_t *)malloc(iSize);
    fread(pFile, 1, iSize, f);
    fclose(f);
    
    if (!truetype.setTtfPointer(pFile, iSize)) {
      printf("read ttf failed\n");
      return -1;
    }
    truetype.setFramebuffer(BITMAP_WIDTH, BITMAP_HEIGHT, 1, pBitmap);
    truetype.setCharacterSize(150);
    truetype.setCharacterSpacing(0);
    truetype.setTextBoundary(0, BITMAP_WIDTH, BITMAP_HEIGHT);
    

    truetype.setTextColor(0xff, 0xff);
    truetype.textDraw(0, 400, "12:34");
    
    WriteBMP("/Users/laurencebank/Downloads/ttf.bmp", pBitmap, NULL, BITMAP_WIDTH, BITMAP_HEIGHT, 1);
    return 0;
}
