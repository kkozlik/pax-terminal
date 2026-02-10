#include "bmp.h"

/**
 * Read BMP image into memory
 */
uint8_t* loadBMP(const char *filename, BITMAPINFOHEADER *bih) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening BMP file");
        return NULL;
    }

    BITMAPFILEHEADER bfh;
    fread(&bfh, sizeof(BITMAPFILEHEADER), 1, file);
    fread(bih, sizeof(BITMAPINFOHEADER), 1, file);

    if (bfh.bfType != 0x4D42) {
        fclose(file);
        printf("Not a valid BMP file: %s\n", filename);
        return NULL;
    }

    fseek(file, bfh.bfOffBits, SEEK_SET);

    uint8_t *data = (uint8_t*)malloc(bih->biSizeImage);
    fread(data, 1, bih->biSizeImage, file);

    fclose(file);
    return data;
}

void freeBMP(uint8_t *data){
    free(data);
}
