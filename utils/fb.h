#pragma once

#include <stdio.h>
#include <stdlib.h>


#define FB_PATH      "/dev/fb"

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define BPP           2      // bytes per pixel (RGB565)

typedef unsigned short color_t;

static const color_t color_blue   = 0x001F;
static const color_t color_red    = 0xF800;
static const color_t color_yellow = 0xFFE0;
static const color_t color_black  = 0x0000;
static const color_t color_white  = 0xFFFF;


color_t *fb_init(int *out_fd);

void fb_close(color_t *fbp,int fb_fd);
