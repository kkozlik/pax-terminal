#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "fb.h"
#include "bmp.h"

color_t *init_draw_buffer();
void free_draw_buffer(color_t *buffer);
void draw_buffer(color_t *buffer, color_t *fbp);

void draw_pixel(color_t *buffer, int x, int y, color_t color);
void draw_line(color_t *buffer,int x0,int y0,int x1,int y1,color_t color);
void draw_circle(color_t *buffer,int xc,int yc,int r,color_t color);
void clear_buffer(color_t *buffer, color_t color);
void draw_char(color_t *buffer,int x,int y,char c,color_t color);
void draw_text(color_t *buffer,int x,int y,const char *text,color_t color);
void draw_digit(color_t *buffer,int x,int y,int digit, color_t color);
void draw_bmp_image(color_t *buffer, int offset_x, int offset_y, BITMAPINFOHEADER bih, uint8_t *bmp_data);
