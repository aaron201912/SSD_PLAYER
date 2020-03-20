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
#ifndef _ST_FONT_H
#define _ST_FONT_H

#ifdef __cplusplus
extern "C"{
#endif	// __cplusplus

int display_text(char *fb, int x, int y, int stride, int color, int pix_size_of_fmt, const char *ascii_text, int len);
int get_font_size(int *w, int *h);
#ifdef __cplusplus
}
#endif	// __cplusplus

#endif//_ST_FONT_H
