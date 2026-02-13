#include "printer.h"
#include "FreeSans15pt8b.h"
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define pgm_read_byte(addr) (*(const uint8_t *)(addr))

static int get_byte_width(const int width_px){
    return (width_px + (8-1)) / 8; // round the division up
}

// Dekodér UTF-8 for the GFX font
// fixme: shall it be in a separate file??
static uint16_t get_utf8_char(const char **str) {
    uint8_t c = (uint8_t)**str;
    if (c < 0x80) { (*str)++; return c; } // ASCII

    if ((c & 0xE0) == 0xC0) { // 2-bytové UTF-8
        uint8_t c1 = (uint8_t)*((*str)++);
        uint8_t c2 = (uint8_t)*((*str)++);
        uint32_t unicode = ((c1 & 0x1F) << 6) | (c2 & 0x3F);

        switch(unicode) {
            case 0x00C1: return 0xA1; // Á
            case 0x00E1: return 0xC1; // á
            case 0x010C: return 0xA8; // Č
            case 0x010D: return 0xC8; // č
            case 0x010E: return 0xAF; // Ď
            case 0x010F: return 0xCF; // ď
            case 0x00C9: return 0xA9; // É
            case 0x00E9: return 0xC9; // é
            case 0x011A: return 0xAC; // Ě
            case 0x011B: return 0xCC; // ě
            case 0x00CD: return 0xAD; // Í
            case 0x00ED: return 0xCD; // í
            case 0x0147: return 0xB2; // Ň
            case 0x0148: return 0xD2; // ň
            case 0x00D3: return 0xB3; // Ó
            case 0x00F3: return 0xD3; // ó
            case 0x0158: return 0xB8; // Ř
            case 0x0159: return 0xD8; // ř
            case 0x0160: return 0x89; // Š
            case 0x0161: return 0x99; // š
            case 0x0164: return 0x8B; // Ť
            case 0x0165: return 0x9B; // ť
            case 0x00DA: return 0xBA; // Ú
            case 0x00FA: return 0xDA; // ú
            case 0x016E: return 0xB9; // Ů
            case 0x016F: return 0xD9; // ů
            case 0x00DD: return 0xBD; // Ý
            case 0x00FD: return 0xDD; // ý
            case 0x017D: return 0x8E; // Ž
            case 0x017E: return 0x9E; // ž
            default: return 0x7F; // Unknown char
        }
    }
    (*str)++; return 0x7F;
}


PrintBuffer *create_print_buffer(const int width, const int height) {

    int width_b = get_byte_width(width);

    PrintBuffer *pb = malloc(
        sizeof(PrintBuffer) + width_b * height * sizeof(uint8_t)
    );

    if (!pb) return NULL;

    pb->width = width;
    pb->width_b = width_b;
    pb->height = height;

    memset(pb->buffer, 0, width_b * pb->height);

    return pb;
}

void free_print_buffer(PrintBuffer *pb) {
    free(pb);
}

void print_pixel(PrintBuffer *pb, const int x, const int y) {
    if (x < 0 || x >= pb->width || y < 0 || y >= pb->height) return;
    pb->buffer[y * pb->width_b + x/8] |= (0x80 >> (x % 8));
}

void print_horizontal_line(PrintBuffer *pb, const int y) {
    for (int x = 0; x < pb->width; x++) {
        print_pixel(pb, x, y);
    }
}

int print_gfx_char(PrintBuffer *pb, const int x, const int y, uint16_t c) {
    GFXfont *f = (GFXfont *)&FreeSans15pt8b;
    if (c < f->first || c > f->last) return 0;

    GFXglyph *g = &(((GFXglyph *)f->glyph)[c - f->first]);
    uint8_t *bitmap = f->bitmap;
    uint16_t bo = g->bitmapOffset;
    uint8_t w = g->width, h = g->height, bits = 0, bit = 0;

    for(int yy=0; yy<h; yy++) {
        for (int xx=0; xx<w; xx++) {
            if(!(bit++ & 7)) bits = pgm_read_byte(&bitmap[bo++]);
            if(bits & 0x80) print_pixel(pb, x + g->xOffset + xx, y + g->yOffset + yy);
            bits <<= 1;
        }
    }

    return g->xAdvance;
}

void print_text(PrintBuffer *pb, const int x, const int y, const char *str) {
    const char *p = str;
    int cx = x;
    while (*p) {
        uint16_t c = get_utf8_char(&p);
        cx += print_gfx_char(pb, cx, y, c);
    }
}

void print_text_right(PrintBuffer *pb, const int right_x, const int y, const char *str) {
    int total_w = 0;
    GFXfont *f = (GFXfont *)&FreeSans15pt8b;
    const char *p = str;

    while (*p) {
        uint16_t c = get_utf8_char(&p);
        if (c >= f->first && c <= f->last)
            total_w += f->glyph[c - f->first].xAdvance;
    }

    print_text(pb, right_x - total_w, y, str);
}


void print_bitmap(PrintBuffer *pb, const int x, const int y, const unsigned char *bmp, const int bmp_width_b, const int bmp_height){

    for (int bmp_y = 0; bmp_y < bmp_height; bmp_y++) {
        int pb_y = bmp_y+y;

        if (pb_y >= pb->height) break;

        for (int bmp_x = 0; bmp_x < bmp_width_b; bmp_x++) {
            int pb_x = bmp_x+x;
            if (pb_x >= pb->width_b) break;

            pb->buffer[(pb_y * pb->width_b) + pb_x] = bmp[(bmp_y * bmp_width_b) + bmp_x];
        }
    }
}


void send_to_printer(const PrintBuffer *pb) {
    int total_size = 2 + (pb->height * 48);
    uint8_t *prnbuf = (uint8_t*)malloc(total_size);
    if(!prnbuf) {
        perror("Cannot allocate memory for print.");
        fflush(stderr);
        return;
    }

    memset(prnbuf, 0, total_size);
    prnbuf[0] = 0x04; prnbuf[1] = 0x00;
    for (int i = 0; i < pb->height; i++) {
        memcpy(&prnbuf[2 + (i * 48)], &pb->buffer[i * pb->width_b], pb->width_b);
    }
    int pfd = open(PRINTER_DEVICE, O_WRONLY);
    if (pfd >= 0) {
        ssize_t written = write(pfd, prnbuf, total_size);
        if (written >= 0) {
            printf("[ATM] Do tiskarny odeslano: %ld bajtu\n", (long)written);
        }
        fsync(pfd);
        close(pfd);
    }
    free(prnbuf);
}
