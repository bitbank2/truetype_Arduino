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

//
// Read a Windows BMP file into memory
//
uint8_t * ReadBMP(const char *fname, int *width, int *height, int *bpp, unsigned char *pPal)
{
    int y, w, h, bits, offset;
    uint8_t *s, *d, *pTemp, *pBitmap;
    int pitch, bytewidth;
    int iSize, iDelta;
    FILE *infile;
    
    infile = fopen(fname, "r+b");
    if (infile == NULL) {
        printf("Error opening input file %s\n", fname);
        return NULL;
    }
    // Read the bitmap into RAM
    fseek(infile, 0, SEEK_END);
    iSize = (int)ftell(infile);
    fseek(infile, 0, SEEK_SET);
    pBitmap = (uint8_t *)malloc(iSize);
    pTemp = (uint8_t *)malloc(iSize);
    fread(pTemp, 1, iSize, infile);
    fclose(infile);
    
    if (pTemp[0] != 'B' || pTemp[1] != 'M' || pTemp[14] < 0x28) {
        free(pBitmap);
        free(pTemp);
        printf("Not a Windows BMP file!\n");
        return NULL;
    }
    w = *(int32_t *)&pTemp[18];
    h = *(int32_t *)&pTemp[22];
    bits = *(int16_t *)&pTemp[26] * *(int16_t *)&pTemp[28];
    if (bits <= 8 && pPal) { // it has a palette, copy it
        uint8_t *p = pPal;
        for (int i=0; i<(1<<bits); i++)
        {
           *p++ = pTemp[54+i*4];
           *p++ = pTemp[55+i*4];
           *p++ = pTemp[56+i*4];
        }
    }
    offset = *(int32_t *)&pTemp[10]; // offset to bits
    bytewidth = (w * bits) >> 3;
    pitch = (bytewidth + 3) & 0xfffc; // DWORD aligned
// move up the pixels
    d = pBitmap;
    s = &pTemp[offset];
    iDelta = pitch;
    if (h > 0) {
        iDelta = -pitch;
        s = &pTemp[offset + (h-1) * pitch];
    } else {
        h = -h;
    }
    for (y=0; y<h; y++) {
        if (bits == 32) {// need to swap red and blue
            for (int i=0; i<bytewidth; i+=4) {
                d[i] = s[i+2];
                d[i+1] = s[i+1];
                d[i+2] = s[i];
                d[i+3] = s[i+3];
            }
        } else {
            memcpy(d, s, bytewidth);
        }
        d += bytewidth;
        s += iDelta;
    }
    *width = w;
    *height = h;
    *bpp = bits;
    free(pTemp);
    return pBitmap;
    
} /* ReadBMP() */

void DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
    
} /* DrawHLine() */

int main(int argc, const char * argv[]) {
    bb_truetype truetype = bb_truetype();
    FILE *f;
    uint8_t *pBitmap, *pFile;
    int iSize, width, height, bpp;
    
//    pBitmap = (uint8_t *)malloc((BITMAP_WIDTH * BITMAP_HEIGHT * bpp)/8);
    pBitmap = ReadBMP("/Users/laurencebank/Downloads/squirrel.bmp", &width, &height, &bpp, NULL);
    printf("TrueType font rendering test\n");
    printf(argv[1]);
    printf("\n");
    f = fopen(argv[1], "r+b");
//    f = fopen("/Users/laurencebank/Downloads/Roboto/Roboto-Black.ttf", "r+b");
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
    truetype.setFramebuffer(width, height, bpp, pBitmap);
    //truetype.setTtfDrawLine(DrawLine);
    truetype.setCharacterSize(36);
    truetype.setCharacterSpacing(0);
    truetype.setTextBoundary(0, width, height);
    truetype.setTextColor(0, 0xff);
#ifndef DEBUG
    for (int i=0; i<10000; i++)
#endif
    {
        truetype.textDraw(280, 420, "This is a squirrel");
    }
  //  truetype.end();
    
    WriteBMP((char *)"/Users/laurencebank/Downloads/ttf.bmp", pBitmap, NULL, width, height, bpp);
    return 0;
}
