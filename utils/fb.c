#include "fb.h"

#include <fcntl.h>
#include <linux/fb.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

// -----------------------------------------------------------------------------
// Framebuffer
// -----------------------------------------------------------------------------
color_t *fb_init(int *out_fd) {
    int fb_fd = open(FB_PATH,O_RDWR);
    if (fb_fd == -1) {
        perror("Error opening framebuffer device");
        fflush(stderr);
        return NULL;
    }
    struct fb_var_screeninfo vinfo;
    if (ioctl(fb_fd,FBIOGET_VSCREENINFO,&vinfo) < 0) {
        perror("Error reading variable information");
        fflush(stderr);
        close(fb_fd);
        return NULL;
    }
    (void)vinfo;

    size_t fb_size = SCREEN_HEIGHT * SCREEN_WIDTH * BPP;
    color_t *fbp = (color_t *)mmap(0,fb_size,PROT_READ|PROT_WRITE,MAP_SHARED,fb_fd,0);
    if (fbp == MAP_FAILED) {
        perror("Error mapping framebuffer device to memory");
        fflush(stderr);
        close(fb_fd);
        return NULL;
    }
    if (out_fd) *out_fd = fb_fd;
    return fbp;
}

void fb_close(color_t *fbp,int fb_fd) {
    if (fbp) munmap(fbp,SCREEN_HEIGHT * SCREEN_WIDTH * BPP);
    if (fb_fd >= 0) close(fb_fd);
}
