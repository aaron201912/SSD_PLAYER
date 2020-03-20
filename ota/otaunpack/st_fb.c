/*
* st_fb.c- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: XXXX <XXXX@sigmastar.com.cn>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>

#include "st_fb.h"
#include "st_font.h"

#include "bmp.h"
#include "jpeg.h"
#include "mypng.h"
#include "raw.h"

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;

//Start of frame buffer mem
static unsigned char *frameBuffer = NULL;
static int g_fbFd = 0;
static int g_screensize = 0;

#define CHECK_FB_INIT \
    do{ \
        if (frameBuffer == NULL) \
        { \
            ERR("fb init fail\n"); \
            return; \
        } \
    } while(0)

/**
 *dump fix info of Framebuffer
 */ 
#define SWITCH_NEXT_Y_OFFSET(yres, yoffset, yres_virtual) (yoffset = ((yoffset + yres < yres_virtual) ? yoffset + yres : 0))
#define SWITCH_NEXT_Y_AND_CONVERT_TO_BUF_ID(yres, yoffset, yres_virtual) (SWITCH_NEXT_Y_OFFSET(yres, yoffset, yres_virtual) / yres)
#define CUR_FB_BUF_ID(yres, yoffset) (yoffset / yres)

#define FB_MAX_BUF 4
typedef struct ST_FB_DirtyRectangle_s
{
    stRect_t astDirtyRect[FB_MAX_BUF];
}ST_FB_DirtyRectangle_t;

static ST_FB_DirtyRectangle_t stDirtyRect;
static pthread_mutex_t _gstFbMutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned int _gBackGroundColor = ARGB888_WHITE;

void printFixedInfo ()
{
    printf ("Fixed screen info:\n"
            "\tid: %s\n"
            "\tsmem_start: 0x%lx\n"
            "\tsmem_len: %d\n"
            "\ttype: %d\n"
            "\ttype_aux: %d\n"
            "\tvisual: %d\n"
            "\txpanstep: %d\n"
            "\typanstep: %d\n"
            "\tywrapstep: %d\n"
            "\tline_length: %d\n"
            "\tmmio_start: 0x%lx\n"
            "\tmmio_len: %d\n"
            "\taccel: %d\n"
            "\n",
            finfo.id, finfo.smem_start, finfo.smem_len, finfo.type,
            finfo.type_aux, finfo.visual, finfo.xpanstep, finfo.ypanstep,
            finfo.ywrapstep, finfo.line_length, finfo.mmio_start,
            finfo.mmio_len, finfo.accel);
}

/**
 *dump var info of Framebuffer
 */
void printVariableInfo ()
{
    printf ("Variable screen info:\n"
            "\txres: %d\n"
            "\tyres: %d\n"
            "\txres_virtual: %d\n"
            "\tyres_virtual: %d\n"
            "\tyoffset: %d\n"
            "\txoffset: %d\n"
            "\tbits_per_pixel: %d\n"
            "\tgrayscale: %d\n"
            "\tred: offset: %2d, length: %2d, msb_right: %2d\n"
            "\tgreen: offset: %2d, length: %2d, msb_right: %2d\n"
            "\tblue: offset: %2d, length: %2d, msb_right: %2d\n"
            "\ttransp: offset: %2d, length: %2d, msb_right: %2d\n"
            "\tnonstd: %d\n"
            "\tactivate: %d\n"
            "\theight: %d\n"
            "\twidth: %d\n"
            "\taccel_flags: 0x%x\n"
            "\tpixclock: %d\n"
            "\tleft_margin: %d\n"
            "\tright_margin: %d\n"
            "\tupper_margin: %d\n"
            "\tlower_margin: %d\n"
            "\thsync_len: %d\n"
            "\tvsync_len: %d\n"
            "\tsync: %d\n"
            "\tvmode: %d\n"
            "\n",
            vinfo.xres, vinfo.yres, vinfo.xres_virtual, vinfo.yres_virtual,
            vinfo.xoffset, vinfo.yoffset, vinfo.bits_per_pixel,
            vinfo.grayscale, vinfo.red.offset, vinfo.red.length,
            vinfo.red.msb_right, vinfo.green.offset, vinfo.green.length,
            vinfo.green.msb_right, vinfo.blue.offset, vinfo.blue.length,
            vinfo.blue.msb_right, vinfo.transp.offset, vinfo.transp.length,
            vinfo.transp.msb_right, vinfo.nonstd, vinfo.activate,
            vinfo.height, vinfo.width, vinfo.accel_flags, vinfo.pixclock,
            vinfo.left_margin, vinfo.right_margin, vinfo.upper_margin,
            vinfo.lower_margin, vinfo.hsync_len, vinfo.vsync_len,
            vinfo.sync, vinfo.vmode);
}

/**
 *get Color format fo framebuffer
 */
MI_FB_ColorFmt_e getFBColorFmt(struct fb_var_screeninfo *var)
{
    MI_FB_ColorFmt_e fmt = E_MI_FB_COLOR_FMT_INVALID;

    switch (var->bits_per_pixel)
    {
        case 16:
        {
            if (var->transp.length == 0) //RGB565
            {
                fmt = E_MI_FB_COLOR_FMT_RGB565;
            }
            else if (var->transp.length ==1) //ARGB 1555
            {
                fmt = E_MI_FB_COLOR_FMT_ARGB1555;
            }
            else if (var->transp.length == 4) //ARGB4444
            {
                fmt = E_MI_FB_COLOR_FMT_ARGB4444;
            }
        }
        break;
        //ARGB8888
        case 32:
        {
            fmt = E_MI_FB_COLOR_FMT_ARGB8888;
        }
        break;
        default:
            fmt = E_MI_FB_COLOR_FMT_INVALID;
            break;
    }
    return fmt;
}

static unsigned int getBytePerPixel()
{
    MI_FB_ColorFmt_e fmt = getFBColorFmt(&vinfo);

    switch (fmt)
    {
        case E_MI_FB_COLOR_FMT_ARGB8888:
            return 4;
        case E_MI_FB_COLOR_FMT_RGB565:
            return 2;
        case E_MI_FB_COLOR_FMT_ARGB4444:
            return 2;
        case E_MI_FB_COLOR_FMT_ARGB1555:
            return 2;
        default:
            ERR("Warning: drawRect() not implemented for color Fmt %i\n", fmt);
    }

    return 0;
}
int ST_FB_SyncDirtyDown()
{
    char *pCurStartAddr = NULL;
    char *pLoopStartAddr = NULL;
    unsigned short stride = 0;
    unsigned short u16BytePerPixel = 0;
    unsigned short u16LoopBufId = 0;
    unsigned short u16CurBufId = 0;
    unsigned short u16YOffsetTmp = 0;
    unsigned short i = 0;

    pthread_mutex_lock(&_gstFbMutex);
    u16BytePerPixel = getBytePerPixel();
    u16CurBufId = SWITCH_NEXT_Y_AND_CONVERT_TO_BUF_ID(vinfo.yres, vinfo.yoffset, vinfo.yres_virtual);
    stride = finfo.line_length;
    u16YOffsetTmp = vinfo.yoffset;
    u16LoopBufId = SWITCH_NEXT_Y_AND_CONVERT_TO_BUF_ID(vinfo.yres, u16YOffsetTmp, vinfo.yres_virtual);
    while(u16LoopBufId != u16CurBufId)
    {
        if (u16LoopBufId >= FB_MAX_BUF || u16CurBufId >= FB_MAX_BUF)
        {
            ERR("FB MAX buf error %d %d\n", u16LoopBufId, u16CurBufId);

            assert(0);
        }
        pCurStartAddr = (char *)(frameBuffer) + (u16CurBufId * vinfo.yres + stDirtyRect.astDirtyRect[u16LoopBufId].u16Y) * stride 
                      + stDirtyRect.astDirtyRect[u16LoopBufId].u16X * u16BytePerPixel;
        pLoopStartAddr = (char *)(frameBuffer) + (u16LoopBufId * vinfo.yres + stDirtyRect.astDirtyRect[u16LoopBufId].u16Y) * stride 
                      + stDirtyRect.astDirtyRect[u16LoopBufId].u16X * u16BytePerPixel;
        /*printf("Blit from id %d x %d y %d w %d h %d\n", u16LoopBufId, stDirtyRect.astDirtyRect[u16LoopBufId].u16X, stDirtyRect.astDirtyRect[u16LoopBufId].u16Y,
                                                  stDirtyRect.astDirtyRect[u16LoopBufId].u16Width, stDirtyRect.astDirtyRect[u16LoopBufId].u16Height);*/
        for (i = 0; i < stDirtyRect.astDirtyRect[u16LoopBufId].u16Height; i++)
        {
            memcpy(pCurStartAddr, pLoopStartAddr, u16BytePerPixel * stDirtyRect.astDirtyRect[u16LoopBufId].u16Width);
            pCurStartAddr += stride;
            pLoopStartAddr += stride;
        }
        u16LoopBufId = SWITCH_NEXT_Y_AND_CONVERT_TO_BUF_ID(vinfo.yres, u16YOffsetTmp, vinfo.yres_virtual);
    }

    return 0;
}
int ST_FB_SyncDirtyUp(const stRect_t *pRect)
{
    unsigned short u16CurBufId = 0;
    unsigned short u16LoopBufId = 0;
    unsigned short u16YOffsetTmp = 0;

    u16CurBufId = CUR_FB_BUF_ID(vinfo.yres, vinfo.yoffset);
    u16YOffsetTmp = vinfo.yoffset;
    u16LoopBufId = SWITCH_NEXT_Y_AND_CONVERT_TO_BUF_ID(vinfo.yres, u16YOffsetTmp, vinfo.yres_virtual);
    if (u16LoopBufId >= FB_MAX_BUF || u16CurBufId >= FB_MAX_BUF)
    {
        ERR("FB MAX buf error %d %d\n", u16LoopBufId, u16CurBufId);

        assert(0);
    }
    stDirtyRect.astDirtyRect[u16CurBufId] = *pRect;
    if (u16LoopBufId != u16CurBufId)
    {
        if (ioctl(g_fbFd, FBIOPAN_DISPLAY, &vinfo) == -1)
        {
            perror("Error: failed to FBIOPAN_DISPLAY");
            assert(0);
        }
    }
    pthread_mutex_unlock(&_gstFbMutex);

    return 0;
}
int ST_FB_CombineRect(stRect_t *pRectDst, stRect_t *pRectA, stRect_t *pRectB)
{
    if ((pRectA->u16Width == 0 || pRectA->u16Height == 0) 
        && (pRectB->u16Width != 0 && pRectB->u16Height != 0))
    {
        *pRectDst = *pRectB;

        return 0;
    }
    if ((pRectB->u16Width == 0 || pRectB->u16Height == 0) 
        && (pRectA->u16Width != 0 && pRectA->u16Height != 0))
    {
        *pRectDst = *pRectA;

        return 0;
    }
    if ((pRectA->u16Width == 0 || pRectA->u16Height == 0) 
        && (pRectB->u16Width == 0 || pRectB->u16Height == 0))
    {
        memset(pRectDst, 0, sizeof(stRect_t));

        return 0;
    }

    pRectDst->u16X = MIN(pRectA->u16X, pRectB->u16X);
    pRectDst->u16Y = MIN(pRectA->u16Y, pRectB->u16Y);
    pRectDst->u16Width = MAX(pRectA->u16Width + pRectA->u16X - pRectDst->u16X, pRectB->u16Width + pRectB->u16X - pRectDst->u16X);
    pRectDst->u16Height = MAX(pRectA->u16Height + pRectA->u16Y - pRectDst->u16Y, pRectB->u16Height + pRectB->u16Y - pRectDst->u16Y);

    return 0;
}
/**
 * draw Rectangle. the colormat of Framebuffer is ARGB8888
 */
void drawRect_rgb32 (int x0, int y0, int width, int height, int color)
{
    const int bytesPerPixel = 4;
    int stride = 0;
    int *dest = NULL;
    int x = 0, y = 0;

    CHECK_FB_INIT;

    stride = finfo.line_length ;
    dest = (int *)((char *)(frameBuffer)
        + (y0 + vinfo.yoffset) * stride + (x0 + vinfo.xoffset) * bytesPerPixel);

    for (y = 0; y < height; ++y)
    {
        for (x = 0; x < width; ++x)
        {
            dest[x] = color;
        }
        dest = (int *)((char *)dest + stride);
    }
}

/**
 * draw Rectangle. the colormat of Framebuffer is RGB565
 */
void drawRect_rgb16 (int x0, int y0, int width, int height, int color)
{
    const int bytesPerPixel = 2;
    int stride = 0;
    const int red = (color & 0xff0000) >> (16 + 3);
    const int green = (color & 0xff00) >> (8 + 2);
    const int blue = (color & 0xff) >> 3;
    const short color16 = blue | (green << 5) | (red << (5 + 6));
    short *dest = NULL;
    int x = 0, y = 0;

    CHECK_FB_INIT;

    stride = finfo.line_length ;
    dest = (short *)((char *)(frameBuffer)
        + (y0 + vinfo.yoffset) * stride + (x0 + vinfo.xoffset) * bytesPerPixel);

    for (y = 0; y < height; ++y)
    {
        for (x = 0; x < width; ++x)
        {
            dest[x] = color16;
        }
        dest = (short *)((char *)dest + stride);
    }
}

/**
 *draw point. the color format of Framebuffer is ARGB1555
 */
void drawPoint_rgb15 (int x, int y, int color)
{
    const int bytesPerPixel = 2;
    int stride = 0;
    const int red = (color & 0xff0000) >> (16 + 3);
    const int green = (color & 0xff00) >> (8 + 3);
    const int blue = (color & 0xff) >> 3;
    const short color15 = blue | (green << 5) | (red << (5 + 5)) | 0x8000;
    short *dest = NULL;

    CHECK_FB_INIT;

    stride = finfo.line_length;
    dest = (short *)((char *)(frameBuffer)
        + (y + vinfo.yoffset) * stride + (x + vinfo.xoffset) * bytesPerPixel);

    dest[0] = color15;
}

void drawLine_rgb15(int x0, int y0, int x1, int y1, int color)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    int ux = dx >0 ? 1 : -1;
    int uy = dy >0 ? 1 : -1;
    int dx2 = dx << 1;
    int dy2 = dy << 1;
    int e, x, y;

    if(abs(dx) > abs(dy))
    {
        e = -dx;
        x = x0;
        y = y0;
        for (x = x0; x < x1; x += ux)
        {
            // ERR("%d,%d, x1:%d\n",x, y, x1);
            drawPoint_rgb15(x, y, color);
            e = e + dy2;
            if (e > 0)
            {
                y += uy;
                e = e - dx2;
            }
        }
    }
    else
    {
        e = -dy;
        x = x0;
        y = y0;
        for (y = y0; y < y1; y += uy)
        {
            // ERR("%d,%d, y1:%d\n",x, y, y1);
            drawPoint_rgb15(x, y, color);
            e = e + dx2;
            if (e > 0)
            {
                x += ux;
                e= e - dy2;
            }
        }
    }
}

#if 0
void drawCircle_rgb15(int xc, int yc, int r, int color)
{
    int x, y, d;

    x = 0;
    y = r;

    d = 3 - 2 * r;

    drawLine_rgb15(xc, yc, x, y, color);

    while(x < y)
    {
        if(d < 0)
        {
            d = d + 4 * x + 6;
        }
        else
        {
            d = d + 4 * ( x - y ) + 10;
            y --;
        }
        x ++;

        drawLine_rgb15(xc, yc, x, y, color);
    }
}
#endif

void drawCircle_rgb15(int cx, int cy, int r, int color)
{
	int r2 = r * r + r;
	int x = 0, x2 = 0, dx2 = 1;
	int y = r, y2 = y*y, dy2 = 2*y - 1;
	int sum = r2;

	while(x <= y)
    {
        drawLine_rgb15(cx - y, cy + x, cx + y, cy + x, color);
        if (x) drawLine_rgb15(cx - y, cy - x, cx + y, cy - x, color);

		sum -= dx2;
		if (sum <= y2)
        {
			if (x != y)
            {
                drawLine_rgb15(cx - x, cy - y, cx + x, cy - y, color);
				if (y) drawLine_rgb15(cx - x, cy + y, cx + x, cy + y, color);
			} /* if */
			y--; y2 -= dy2; dy2 -= 2;
		} /* if */
		x++;
		x2 += dx2;
		dx2 += 2;
	} /* while */
} /* bh */


/**
 *draw Rectangle. the color format of Framebuffer is ARGB1555
 */
void drawRect_rgb15 (int x0, int y0, int width, int height, int color)
{
    const int bytesPerPixel = 2;
    int stride = 0;
    const int red = (color & 0xff0000) >> (16 + 3);
    const int green = (color & 0xff00) >> (8 + 3);
    const int blue = (color & 0xff) >> 3;
    const short color15 = blue | (green << 5) | (red << (5 + 5)) | 0x8000;
    short *dest = NULL;
    int x = 0, y = 0;

    CHECK_FB_INIT;

    stride = finfo.line_length;
    dest = (short *)((char *)(frameBuffer)
        + (y0 + vinfo.yoffset) * stride + (x0 + vinfo.xoffset) * bytesPerPixel);

    for (y = 0; y < height; ++y)
    {
        for (x = 0; x < width; ++x)
        {
            dest[x] = color15;
        }
        dest = (short *)((char *)dest + stride);
    }
}

/**
 *draw Rectangle. the color format of Framebuffer is ARGB1444
 */
void  drawRect_rgb12(int x0, int y0, int width, int height, int color)
{
    const int bytesPerPixel =2;
    int stride = 0;
    const int red = (color & 0xff0000) >> (16 + 4);
    const int green = (color & 0xff00) >> (8 + 4);
    const int blue = (color & 0xff) >> 4;
    const short color16 = blue | (green << 4) | (red << (4+4)) |0xf000;
    short *dest = NULL;
    int x, y;

    stride = finfo.line_length;
    dest = (short *)((char *)(frameBuffer)
        + (y0 + vinfo.yoffset) * stride + (x0 + vinfo.xoffset) * bytesPerPixel);

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            dest[x] = color16;
        }
        dest = (short *)((char *)dest + stride);
    }
}

void drawPoint (int x, int y, int color)
{
    MI_FB_ColorFmt_e fmt = getFBColorFmt(&vinfo);

    switch (fmt)
    {
        case E_MI_FB_COLOR_FMT_ARGB1555:
         {
            drawPoint_rgb15(x, y, color);
         }
         break;
         default:
             ERR("Warning: drawRect() not implemented for color Fmt %i\n", fmt);
    }
}

void drawLine (int x0, int y0, int x1, int y1, int color)
{
    MI_FB_ColorFmt_e fmt = getFBColorFmt(&vinfo);

    switch (fmt)
    {
        case E_MI_FB_COLOR_FMT_ARGB1555:
         {
            drawLine_rgb15(x0, y0, x1, y1, color);
         }
         break;
         default:
             ERR("Warning: drawRect() not implemented for color Fmt %i\n", fmt);
    }
}

void drawCircle(int xc, int yc, int r, int color)
{
    MI_FB_ColorFmt_e fmt = getFBColorFmt(&vinfo);

    switch (fmt)
    {
        case E_MI_FB_COLOR_FMT_ARGB1555:
         {
            drawCircle_rgb15(xc, yc, r, color);
         }
         break;
         default:
             ERR("Warning: drawRect() not implemented for color Fmt %i\n", fmt);
    }
}

/**
 *draw Rectangle. accroding to Framebuffer format
 */
void drawRect (int x0, int y0, int width, int height, int color)
{
     MI_FB_ColorFmt_e fmt = getFBColorFmt(&vinfo);

     switch (fmt)
     {
         case E_MI_FB_COLOR_FMT_ARGB8888:
         {
            drawRect_rgb32(x0, y0, width, height, color);
         }
         break;
         case E_MI_FB_COLOR_FMT_RGB565:
         {
            drawRect_rgb16(x0, y0, width, height, color);
         }
         break;
         case E_MI_FB_COLOR_FMT_ARGB4444:
         {
            drawRect_rgb12(x0, y0, width, height, color);
         }
         break;
         case E_MI_FB_COLOR_FMT_ARGB1555:
         {
            drawRect_rgb15(x0, y0, width, height, color);
         }
         break;
         default:
             ERR("Warning: drawRect() not implemented for color Fmt %i\n",
                fmt);
     }
}

/**
 *Conver color key value according to color format
 */
void convertColorKeyByFmt(MI_FB_ColorKey_t* colorkey)
{
        MI_FB_ColorFmt_e fmt = getFBColorFmt(&vinfo);
        unsigned char red = colorkey->u8Red;
        unsigned char green = colorkey->u8Green;
        unsigned char blue = colorkey->u8Blue;
        switch (fmt)
        {
            case E_MI_FB_COLOR_FMT_RGB565:
            {
                colorkey->u8Red = (red >> 3)&(0x1f);
                colorkey->u8Green = (green >> 2)&(0x3f);
                colorkey->u8Blue = (blue >> 3)&(0x1f);
            }
            break;
            case E_MI_FB_COLOR_FMT_ARGB4444:
            {
                colorkey->u8Red = (red >> 4)&0xf;
                colorkey->u8Green = (green >> 4)&0xf;
                colorkey->u8Blue = (blue>>4)&0xf;
            }
            break;
            case E_MI_FB_COLOR_FMT_ARGB1555:
            {
                colorkey->u8Red = (red>>3) & 0x1f;
                colorkey->u8Green= (green >>3) & 0x1f;
                colorkey->u8Blue = (blue >>3) &0x1f;
            }
            break;
            default:
                ERR("convertColorKeyByFmt colorfmt is %d\n",fmt);
            break;
        }
}
static void CompileFormat(PixelFormat *format, int bpp,
                   MI_U32 Rmask, MI_U32 Gmask, MI_U32 Bmask, MI_U32 Amask)
{
    MI_U32 mask;

    format->alpha = 0xFF;

    switch(bpp) {
        case 1:
        case 4:
        case 8: {
            assert(0);
            printf("%s %d unsupport\n", __FUNCTION__, __LINE__);
        }

        default:
            format->Rshift = 0;
            format->Rloss = 8;

            if(Rmask) {
                for(mask = Rmask; !(mask & 0x01); mask >>= 1)
                    ++format->Rshift;

                for(; (mask & 0x01); mask >>= 1)
                    --format->Rloss;
            }

            format->Gshift = 0;
            format->Gloss = 8;

            if(Gmask) {
                for(mask = Gmask; !(mask & 0x01); mask >>= 1)
                    ++format->Gshift;

                for(; (mask & 0x01); mask >>= 1)
                    --format->Gloss;
            }

            format->Bshift = 0;
            format->Bloss = 8;

            if(Bmask) {
                for(mask = Bmask; !(mask & 0x01); mask >>= 1)
                    ++format->Bshift;

                for(; (mask & 0x01); mask >>= 1)
                    --format->Bloss;
            }

            format->Ashift = 0;
            format->Aloss = 8;

            if(Amask) {
                for(mask = Amask; !(mask & 0x01); mask >>= 1)
                    ++format->Ashift;

                for(; (mask & 0x01); mask >>= 1)
                    --format->Aloss;
            }

            format->Rmask = Rmask;
            format->Gmask = Gmask;
            format->Bmask = Bmask;
            format->Amask = Amask;
            break;
    }

    /* Calculate some standard bitmasks, if necessary
     * Note:  This could conflict with an alpha mask, if given.
     */
    if((bpp > 8) && !format->Rmask && !format->Gmask && !format->Bmask) {
        /* R-G-B */
        if(bpp > 24)
            bpp = 24;

        format->Rloss = 8 - (bpp / 3);
        format->Gloss = 8 - (bpp / 3) - (bpp % 3);
        format->Bloss = 8 - (bpp / 3);
        format->Rshift = ((bpp / 3) + (bpp % 3)) + (bpp / 3);
        format->Gshift = (bpp / 3);
        format->Bshift = 0;
        format->Rmask = ((0xFF >> format->Rloss) << format->Rshift);
        format->Gmask = ((0xFF >> format->Gloss) << format->Gshift);
        format->Bmask = ((0xFF >> format->Bloss) << format->Bshift);

    }
}

static void syncFormat(BITMAP *bmp, struct fb_var_screeninfo *vinfo)
{
    MI_U32 Rmask;
    MI_U32 Gmask;
    MI_U32 Bmask;
    MI_U32 Amask;
    int i;

    Rmask = 0;

    for(i = 0; i < vinfo->red.length; ++i) {
        Rmask <<= 1;
        Rmask |= (0x00000001 << vinfo->red.offset);
    }

    Gmask = 0;

    for(i = 0; i < vinfo->green.length; ++i) {
        Gmask <<= 1;
        Gmask |= (0x00000001 << vinfo->green.offset);
    }

    Bmask = 0;

    for(i = 0; i < vinfo->blue.length; ++i) {
        Bmask <<= 1;
        Bmask |= (0x00000001 << vinfo->blue.offset);
    }

    Amask = 0;

    for(i = 0; i < vinfo->transp.length; ++i) {
        Amask <<= 1;
        Amask |= (0x00000001 << vinfo->transp.offset);
    }

    CompileFormat(&bmp->pxFmt, bmp->bmBitsPerPixel, Rmask, Gmask, Bmask, Amask);
}
int ST_Fb_Init(void)
{
    const char *devfile = "/dev/fb0";

    /* Open the file for reading and writing */
    g_fbFd = open (devfile, O_RDWR);
    if (g_fbFd == -1)
    {
        perror("Error: cannot open framebuffer device");
        assert(0);
    }

    //get fb_fix_screeninfo
    if (ioctl(g_fbFd, FBIOGET_FSCREENINFO, &finfo) == -1)
    {
        perror("Error reading fixed information");
        assert(0);
    }
    //printFixedInfo();

    //get fb_var_screeninfo
    if (ioctl(g_fbFd, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
        perror("Error reading variable information");
        assert(0);
    }
    //printVariableInfo();

    /* Figure out the size of the screen in bytes */
    g_screensize = finfo.smem_len;

    /* Map the device to memory */
    frameBuffer =
        (unsigned char *) mmap(0, g_screensize, PROT_READ | PROT_WRITE, MAP_SHARED, g_fbFd, 0);
    if (frameBuffer == MAP_FAILED)
    {
        perror("Error: Failed to map framebuffer device to memory");
        assert(0);
    }

    // clear framebuffer
    drawRect(0, 0, vinfo.xres, vinfo.yres_virtual, _gBackGroundColor);
    memset(&stDirtyRect, 0, sizeof(ST_FB_DirtyRectangle_t));
    if (ioctl(g_fbFd, FBIOPAN_DISPLAY, &vinfo) == -1)
    {
        perror("Error: failed to FBIOPAN_DISPLAY");
        assert(0);
    }

    // memset(frameBuffer, ARGB1555_RED, g_screensize);

    return 0;
}
int ST_Fb_Deinit(void)
{
    if (frameBuffer)
        munmap(frameBuffer, g_screensize);

    close(g_fbFd);
    
    return 0;
}

void ST_Fb_SetColorFmt(MI_FB_ColorFmt_e enFormat)
{
    if (g_fbFd < 0)
    {
        return;
    }

    MI_FB_DisplayLayerAttr_t stDisplayAttr;

    // set color format
    memset(&stDisplayAttr, 0, sizeof(MI_FB_DisplayLayerAttr_t));
    if (ioctl(g_fbFd, FBIOGET_DISPLAYLAYER_ATTRIBUTES, &stDisplayAttr))
    {
        perror("Error: failed to FBIOGET_DISPLAYLAYER_ATTRIBUTES");
        return;
    }

    ERR("%s %d, u32Xpos:%d,u32YPos:%d,u32dstWidth:%d,u32dstHeight:%d,u32DisplayWidth:%d,u32DisplayHeight:%d,u32ScreenWidth:%d,u32ScreenHeight:%d\n", __func__, __LINE__,
        stDisplayAttr.u32Xpos, stDisplayAttr.u32YPos, stDisplayAttr.u32dstWidth, stDisplayAttr.u32dstHeight,
        stDisplayAttr.u32DisplayWidth, stDisplayAttr.u32DisplayHeight, stDisplayAttr.u32ScreenWidth, stDisplayAttr.u32ScreenHeight);

    if (enFormat == E_MI_FB_COLOR_FMT_YUV422)
    {
        ERR("not support this format, set default format, ARGB1555\n");
        enFormat = E_MI_FB_COLOR_FMT_ARGB1555;
    }
    stDisplayAttr.u32SetAttrMask = E_MI_FB_DISPLAYLAYER_ATTR_MASK_COLOR_FMB;
    stDisplayAttr.eFbColorFmt = enFormat;
    if (ioctl(g_fbFd, FBIOSET_DISPLAYLAYER_ATTRIBUTES, &stDisplayAttr))
    {
        perror("Error: failed to FBIOSET_DISPLAYLAYER_ATTRIBUTES");
        return;
    }
}

int ST_Fb_DeInit()
{
    if (frameBuffer != NULL)
    {
        munmap (frameBuffer, g_screensize);
        frameBuffer = NULL;
    }

    if (g_fbFd > 0)
    {
        close(g_fbFd);
        g_fbFd = 0;
    }

    return 0;
}

int ST_Fb_FillRect(const stRect_t *pRect, unsigned int u32ColorVal)
{
    drawRect((int)pRect->u16X, (int)pRect->u16Y, (int)pRect->u16Width, (int)pRect->u16Height, (int)u32ColorVal);

    return 0;
}

int ST_Fb_DrawRect(const stRect_t *pRect, unsigned int u32ColorVal)
{

    drawLine(pRect->u16X, pRect->u16Y,
                pRect->u16X + pRect->u16Width, pRect->u16Y, u32ColorVal);

    drawLine(pRect->u16X + pRect->u16Width, pRect->u16Y,
                pRect->u16X + pRect->u16Width, pRect->u16Y + pRect->u16Height, u32ColorVal);

    drawLine(pRect->u16X, pRect->u16Y + pRect->u16Height,
            pRect->u16X + pRect->u16Width, pRect->u16Y + pRect->u16Height,u32ColorVal);

    drawLine(pRect->u16X, pRect->u16Y,
            pRect->u16X, pRect->u16Y + pRect->u16Height,u32ColorVal);

    return 0;
}
int ST_Fb_ClearRect(const stRect_t *pRect)
{
    drawRect((int)pRect->u16X, (int)pRect->u16Y, (int)pRect->u16Width, (int)pRect->u16Height, (int)_gBackGroundColor);

    return 0;
}
int ST_Fb_DrawText(unsigned short u16X, unsigned short u16Y, unsigned int u32Color, const char *pText)
{
    int stride = 0;
    char *dest = NULL;
    int bytesPerPixel = 0;
    int w = 0;
    int h = 0;
    unsigned short u16StrLen = 0;

    if (u16X > vinfo.xres || u16Y > vinfo.yres)
        return -1;
    get_font_size(&w, &h);
    if (h + u16Y > vinfo.yres)
        return -1;
    stride = finfo.line_length;
    bytesPerPixel = getBytePerPixel();
    dest = (char *)((char *)(frameBuffer) + vinfo.yoffset * stride + vinfo.xoffset * bytesPerPixel);
    u16StrLen = strlen(pText);
    if (u16StrLen * w + u16X > vinfo.xres)
    {
        u16StrLen = (vinfo.xres - u16X) / w;
    }
    display_text(dest, u16X, u16Y, stride, u32Color, bytesPerPixel, pText, u16StrLen);

    return 0;
}
#define LOGO_SUFFIX_RAW ".raw"
#define LOGO_SUFFIX_JPEG ".jpg"
#define LOGO_SUFFIX_PNG ".png"
#define LOGO_SUFFIX_BMP ".png"
int ST_Fb_DrawPicture(const char *pFilePath)
{
    BITMAP fb;
    FILE *fp;
    int stride = 0;
    char *dest = NULL;
    int bytesPerPixel = 0;

    bytesPerPixel = getBytePerPixel();
    fb.bmPhyAddr = finfo.smem_start;
    fb.bmHeight = vinfo.yres;
    fb.bmWidth = vinfo.xres;
    fb.bmPitch = finfo.line_length;
    fb.bmBitsPerPixel =  bytesPerPixel * 8;
    fb.bmBytesPerPixel = bytesPerPixel ;
    syncFormat(&fb, &vinfo);

    if((fp = fopen(pFilePath, "r")) == NULL) {
        fprintf(stderr, "can't open %s\n", pFilePath);
        return 0;
    }
    stride = finfo.line_length;
    dest = (char *)((char *)(frameBuffer) + vinfo.yoffset * stride + vinfo.xoffset * bytesPerPixel);
    if(strstr(pFilePath, LOGO_SUFFIX_JPEG) != NULL) {
        load_logo_jpeg(dest, &fb, fp);
    } else if(strstr(pFilePath, LOGO_SUFFIX_PNG) != NULL) {
        load_logo_png(dest, &fb, fp);
    } else if(strstr(pFilePath, LOGO_SUFFIX_BMP) != NULL) {
        load_logo_bmp(dest, &fb, fp);
    } else if(strstr(pFilePath, LOGO_SUFFIX_RAW) != NULL) {
        load_logo_raw(dest, &fb, fp);
    } else {
        fprintf(stderr, "logo format not support");
        fclose(fp);
        return 1;
    }
    fclose(fp);

    return MI_SUCCESS;
}
int ST_Fb_GetFontSz(unsigned short *pW, unsigned short *pH)
{
    int w = 0;
    int h = 0;

    get_font_size(&w, &h);
    *pW = w;
    *pH = h;

    return 0;
}
int ST_Fb_GetLayerSz(unsigned short *pW, unsigned short *pH)
{
    *pW = vinfo.xres;
    *pH = vinfo.yres;

    return 0;
}
int ST_Fb_FillLine(unsigned short x0, unsigned short y0, unsigned short x1, unsigned short y1, unsigned int u32ColorVal)
{
    drawLine(x0, y0, x1, y1, (int)u32ColorVal);

    return 0;
}

int ST_Fb_FillPoint(unsigned short x, unsigned short y, unsigned int u32ColorVal)
{
    drawPoint(x, y, (int)u32ColorVal);

    return 0;
}

int ST_Fb_FillCircle(unsigned short x, unsigned short y, unsigned short r, unsigned int u32ColorVal)
{
    drawCircle(x, y, r, (int)u32ColorVal);

    return 0;
}

int ST_Fb_GetColorKey(unsigned int *pu32ColorKeyVal)
{
    MI_FB_ColorKey_t colorKeyInfo;
    if (ioctl(g_fbFd, FBIOGET_COLORKEY,&colorKeyInfo) < 0) {
        ERR("Error: failed to FBIOGET_COLORKEY\n");
        assert(0);
    }
    *pu32ColorKeyVal = ((colorKeyInfo.bKeyEnable ? 0xFF : 0) << 24)|(colorKeyInfo.u8Red << 16)|(colorKeyInfo.u8Green << 8)|(colorKeyInfo.u8Blue);

    return 0;
}

int ST_Fb_SetColorKey(unsigned int u32ColorKeyVal)
{
    MI_FB_ColorKey_t colorKeyInfo;
    if (ioctl(g_fbFd, FBIOGET_COLORKEY,&colorKeyInfo) < 0) {
        ERR("Error: failed to FBIOGET_COLORKEY\n");
        assert(0);
    }

    colorKeyInfo.bKeyEnable = TRUE;
    colorKeyInfo.u8Red = PIXEL8888RED(u32ColorKeyVal);
    colorKeyInfo.u8Green = PIXEL8888GREEN(u32ColorKeyVal);
    colorKeyInfo.u8Blue = PIXEL8888BLUE(u32ColorKeyVal);

    //convertColorKeyByFmt(&colorKeyInfo);
    if (ioctl(g_fbFd, FBIOSET_COLORKEY, &colorKeyInfo) < 0) {
        ERR("Error: failed to FBIOGET_COLORKEY");
        assert(0);
    }

    return 0;
}

int ST_Fb_InitMouse(int s32MousePicW, int s32MousePicH, int s32BytePerPixel, unsigned char *pu8MouseFile)
{
    MI_FB_CursorAttr_t stCursorAttr;
    FILE *fp = NULL;
    unsigned char *pbuff = NULL;
    pbuff = malloc(s32BytePerPixel * s32MousePicW * s32MousePicW);
    if (!g_fbFd)
    {
        ERR("Please init fb first.\n");
        return -1;
    }
    fp = fopen((const char *)pu8MouseFile, "rb");
    if (fp)
    {
        fread(pbuff, 1, s32BytePerPixel * s32MousePicW * s32MousePicW, fp);
        fclose(fp);
    }
    //set curosr Icon && set positon
    stCursorAttr.stCursorImageInfo.u32Width = s32MousePicW;
    stCursorAttr.stCursorImageInfo.u32Height = s32MousePicW;
    stCursorAttr.stCursorImageInfo.u32Pitch = s32MousePicW; //?????
    stCursorAttr.stCursorImageInfo.eColorFmt = E_MI_FB_COLOR_FMT_ARGB8888;
    stCursorAttr.stCursorImageInfo.data = (char *)pbuff;
    stCursorAttr.u32HotSpotX = 18;
    stCursorAttr.u32HotSpotY = 9;
    stCursorAttr.u32XPos = 100;
    stCursorAttr.u32YPos = 1080;
    stCursorAttr.u16CursorAttrMask = E_MI_FB_CURSOR_ATTR_MASK_ICON
        | E_MI_FB_CURSOR_ATTR_MASK_SHOW | E_MI_FB_CURSOR_ATTR_MASK_POS;
    if (ioctl(g_fbFd, FBIOSET_CURSOR_ATTRIBUTE, &stCursorAttr)) {
        ERR("Error FBIOSET_CURSOR_ATTRIBUTE\n");
        assert(0);
    }

    if (pbuff)
    {
        free(pbuff);
    }

    return 0;
}

int ST_Fb_MouseSet(unsigned int u32X, unsigned int u32Y)
{
    MI_FB_CursorAttr_t stCursorAttr;

    stCursorAttr.u32XPos = u32X;
    stCursorAttr.u32YPos = u32Y;
    stCursorAttr.u16CursorAttrMask = E_MI_FB_CURSOR_ATTR_MASK_POS;
    if (ioctl(g_fbFd, FBIOSET_CURSOR_ATTRIBUTE, &stCursorAttr)) {
        ERR("Error FBIOSET_CURSOR_ATTRIBUTE\n");
        assert(0);
    }

    return 0;
}

int ST_FB_Show(MI_BOOL bShow)
{
    if (g_fbFd < 0)
    {
        return -1;
    }

    if (ioctl(g_fbFd, FBIOSET_SHOW, &bShow)<0)
    {
        perror("Error: failed to FBIOSET_SHOW");
    }

    ERR("%s fb\n", bShow ? "show" : "hide");

    return 0;
}

void ST_FB_SetAlphaInfo(MI_FB_GlobalAlpha_t *pstAlphaInfo)
{
    if (g_fbFd < 0)
    {
        return;
    }

    if (pstAlphaInfo == NULL)
    {
        return;
    }

    if (ioctl(g_fbFd, FBIOSET_GLOBAL_ALPHA, pstAlphaInfo) < 0)
    {
        perror("Error: failed to FBIOGET_GLOBAL_ALPHA");
    }
}

void ST_FB_GetAlphaInfo(MI_FB_GlobalAlpha_t *pstAlphaInfo)
{
    if (g_fbFd < 0)
    {
        return;
    }

    if (pstAlphaInfo == NULL)
    {
        return;
    }

    if (ioctl(g_fbFd, FBIOGET_GLOBAL_ALPHA, pstAlphaInfo) < 0)
    {
        perror("Error: failed to FBIOGET_GLOBAL_ALPHA");
    }
}

void ST_FB_ChangeResolution(int width, int height)
{
    if (g_fbFd < 0)
    {
        return;
    }

    ERR("%s %d, width:%d,height:%d\n", __func__, __LINE__, width, height);
    MI_FB_DisplayLayerAttr_t stDisplayAttr;
    memset(&stDisplayAttr, 0, sizeof(MI_FB_DisplayLayerAttr_t));
    stDisplayAttr.u32SetAttrMask =  E_MI_FB_DISPLAYLAYER_ATTR_MASK_SCREEN_SIZE |
        E_MI_FB_DISPLAYLAYER_ATTR_MASK_BUFFER_SIZE | E_MI_FB_DISPLAYLAYER_ATTR_MASK_DISP_SIZE
        | E_MI_FB_DISPLAYLAYER_ATTR_MASK_DISP_POS;
    stDisplayAttr.u32Xpos = 0;
    stDisplayAttr.u32YPos = 0;
    stDisplayAttr.u32dstWidth = width;
    stDisplayAttr.u32dstHeight = height;
    stDisplayAttr.u32DisplayWidth = (width > 1920) ? 1920 : width;
    stDisplayAttr.u32DisplayHeight = (height > 1080) ? 1080 : height;
    stDisplayAttr.u32ScreenWidth = width;
    stDisplayAttr.u32ScreenHeight = height;
    //E_MI_FB_DISPLAYLAYER_ATTR_MASK_BUFFER_SIZE operaton will change
    //var info and fix.line_lingth, so Need Retrive fixinfo and varinfo
    if (ioctl(g_fbFd, FBIOSET_DISPLAYLAYER_ATTRIBUTES, &stDisplayAttr))
    {
        perror("Error: failed to FBIOSET_DISPLAYLAYER_ATTRIBUTES");
        return;
    }

    // get fb_fix_screeninfo
    if (ioctl(g_fbFd, FBIOGET_FSCREENINFO, &finfo) == -1)
    {
        perror("Error reading fixed information");
        assert(0);
    }

    // get fb_var_screeninfo
    if (ioctl(g_fbFd, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
        perror("Error reading variable information");
        assert(0);
    }
    // printVariableInfo();

    // clear framebuffer
    drawRect(0, 0, vinfo.xres, vinfo.yres, ARGB888_BLUE);
    // memset(frameBuffer, 0x0, finfo.smem_len);

    //Pandisplay
    if (ioctl(g_fbFd, FBIOPAN_DISPLAY, &vinfo) == -1)
    {
        perror("Error reading variable information");
        assert(0);
    }
}

