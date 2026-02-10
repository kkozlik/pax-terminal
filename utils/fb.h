#pragma once

#include <stdio.h>
#include <stdlib.h>


#define FB_PATH      "/dev/fb"

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define BPP           2      // bytes per pixel (RGB565)

typedef unsigned short color_t;

color_t *fb_init(int *out_fd);

void fb_close(color_t *fbp,int fb_fd);
