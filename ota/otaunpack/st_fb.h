/*
* st_fb.h- Sigmastar
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
#ifndef _ST_FB_H
#define _ST_FB_H

#include "mstarFb.h"

#ifdef __cplusplus
extern "C"{
#endif	// __cplusplus

#define RGB2PIXEL1555(r,g,b)	\
	((((r) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((b) & 0xf8) >> 3) | 0x8000)

#if 0
#define PIXEL1555BLUE(pixelval)		((pixelval) & 0x1f)
#define PIXEL1555GREEN(pixelval)	(((pixelval) >> 5) & 0x1f)
#define PIXEL1555RED(pixelval)		(((pixelval) >> 10) & 0x1f)
#endif
#define PIXEL1555RED(pixelval)		(((pixelval) >> 7) & 0xf8)
#define PIXEL1555GREEN(pixelval)		(((pixelval) >> 2) & 0xf8)
#define PIXEL1555BLUE(pixelval)			(((pixelval) << 3) & 0xf8)

#define ARGB1555_BLACK  RGB2PIXEL1555(0,0,0)
#define ARGB1555_RED    RGB2PIXEL1555(255,0,0)


#define ARGB2PIXEL8888(a,r,g,b)	\
	(((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

#define PIXEL8888ALPHA(pixelval)	(((pixelval) >> 24) & 0xff)
#define PIXEL8888RED(pixelval)  	(((pixelval) >> 16) & 0xff)
#define PIXEL8888GREEN(pixelval)	(((pixelval) >> 8) & 0xff)
#define PIXEL8888BLUE(pixelval) 	((pixelval) & 0xff)

#define ARGB888_BLACK   ARGB2PIXEL8888(255,0,0,0)
#define ARGB888_WHITE   ARGB2PIXEL8888(255,255,255,255)
#define ARGB888_RED     ARGB2PIXEL8888(255,255,0,0)
#define ARGB888_GREEN   ARGB2PIXEL8888(255,0,255,0)
#define ARGB888_BLUE    ARGB2PIXEL8888(255,0,0,255)
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif
#define ERR(fmt, args...) ({do{fprintf(stderr, fmt, ##args);}while(0);})

typedef struct stRect_s
{
    unsigned short u16X;
    unsigned short u16Y;
    unsigned short u16Width;
    unsigned short u16Height;
}stRect_t;

int ST_Fb_Init();
int ST_Fb_Deinit(void);
int ST_Fb_FillRect(const stRect_t *pRect, unsigned int u32ColorVal);
int ST_Fb_ClearRect(const stRect_t *pRect);
int ST_Fb_DrawRect(const stRect_t *pRect, unsigned int u32ColorVal);
int ST_Fb_FillPoint(unsigned short x, unsigned short y, unsigned int u32ColorVal);
int ST_Fb_FillLine(unsigned short x0, unsigned short y0, unsigned short x1, unsigned short y1, unsigned int u32ColorVal);
int ST_Fb_FillCircle(unsigned short x, unsigned short y, unsigned short r, unsigned int u32ColorVal);
int ST_Fb_DrawText(unsigned short u16X, unsigned short u16Y, unsigned int u32Color, const char *pText);
int ST_Fb_DrawPicture(const char *pFilePath);
int ST_Fb_GetFontSz(unsigned short *pW, unsigned short *pH);
int ST_Fb_GetLayerSz(unsigned short *pW, unsigned short *pH);
int ST_Fb_DeInit();
int ST_Fb_InitMouse(int s32MousePicW, int s32MousePicH, int s32BytePerPixel, unsigned char *pu8MouseFile);
int ST_Fb_MouseSet(unsigned int u32X, unsigned int u32Y);
int ST_FB_Show(MI_BOOL bShow);
void ST_FB_GetAlphaInfo(MI_FB_GlobalAlpha_t *pstAlphaInfo);
void ST_FB_SetAlphaInfo(MI_FB_GlobalAlpha_t *pstAlphaInfo);
void ST_FB_ChangeResolution(int width, int height);
void ST_Fb_SetColorFmt(MI_FB_ColorFmt_e enFormat);
int ST_Fb_SetColorKey(unsigned int u32ColorKeyVal);
int ST_Fb_GetColorKey(unsigned int *pu32ColorKeyVal);
int ST_FB_SyncDirtyDown();
int ST_FB_SyncDirtyUp(const stRect_t *pRect);
int ST_FB_CombineRect(stRect_t *pRectDst, stRect_t *pRectA, stRect_t *pRectB);

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif//_ST_FB_H
