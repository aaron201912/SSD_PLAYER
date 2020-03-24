/*
* st_font.c- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: malloc.peng <malloc.peng@sigmastar.com.cn>
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
#include <stdio.h>
#include <string.h>
//#include "font_32x32.h"
//#include "font_24x24.h"
//#include "font_24x32.h"
//#include "font_16x16.h"
#include "font_16x28.h"
//#include "font_8x16.h"

int display_text(char *fb, int x, int y, int stride, int color, int pix_byte_of_fmt, const char *ascii_text, int len) {
    char *p = NULL, *q = NULL, *c = NULL;
    unsigned char word;
    int print_start = 0;
    int line_offset = 0;
    int i = 0, j = 0, k = 0, l = 0;

    p = fb + x * pix_byte_of_fmt + y * stride;
    for (i = 0; i < len; i++) {
        print_start = font_w * i * pix_byte_of_fmt;
        for (j = 0; j < font_h * font_w / 8; j++) {
            word = font[(unsigned char)ascii_text[i]][j];
            if (word){
                line_offset = print_start + ((j * 8) % font_w) * pix_byte_of_fmt + (j * 8)/ font_w * stride;
                for (k = 0; k < sizeof(word) * 8; k++) {
                    q = p + k * pix_byte_of_fmt + line_offset;
                    if (word & (1 << k)) {
                        c = (char *)&color;
                        for (l = 0; l < pix_byte_of_fmt; l++) {
                            *q++ = *c++;
                        }
                    }
                } 
            }
        }
    }

    return 0;
}
int get_font_size(int *w, int *h) {
    
    *w = font_w;
    *h = font_h;
    return 0;
}
