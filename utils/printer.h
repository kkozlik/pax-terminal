#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define PRINTER_DEVICE "/dev/printer"
#define PRINTER_WIDTH  384
#define PRINTER_BOTTOM_BORDER 120

typedef struct {
    int  width;
    int  height;
    int  width_b;
    uint8_t buffer[];
} PrintBuffer;

PrintBuffer *create_print_buffer(const int width, const int height);
void free_print_buffer(PrintBuffer *buffer);
void print_pixel(PrintBuffer *pb, const int x, const int y);
void print_horizontal_line(PrintBuffer *pb, const int y);
void print_text(PrintBuffer *pb, const int x, const int y, const char *str);
void print_text_right(PrintBuffer *pb, const int right_x, const int y, const char *str);
void print_bitmap(PrintBuffer *pb, const int x, const int y, const unsigned char *bmp, const int bmp_width_b, const int bmp_height);
void send_to_printer(const PrintBuffer *pb);

