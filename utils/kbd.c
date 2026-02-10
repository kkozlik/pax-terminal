#include "kbd.h"

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>

/*
 * Keyboard
 */
int kbd_init(void) {
    int tty_fd = open(TTY_PATH,O_RDONLY | O_NONBLOCK);
    if (tty_fd == -1) {
        perror("Error opening TTY device");
        fflush(stderr);
        return -1;
    }
    struct termios term;
    if (tcgetattr(tty_fd,&term) == 0) {
        term.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(tty_fd,TCSANOW,&term);
    }
    return tty_fd;
}

void kbd_close(int tty_fd) {
    if (tty_fd >= 0) close(tty_fd);
}

int kbd_read_char(int tty_fd,char *out_char) {
    if (tty_fd < 0 || !out_char) return -1;
    fd_set set;
    struct timeval timeout;

    FD_ZERO(&set);
    FD_SET(tty_fd,&set);
    timeout.tv_sec  = 0;
    timeout.tv_usec = 20000;

    int rv = select(tty_fd+1,&set,NULL,NULL,&timeout);
    if (rv == -1) {
        perror("select");
        fflush(stderr);
        return -1;
    } else if (rv > 0 && FD_ISSET(tty_fd,&set)) {
        ssize_t r = read(tty_fd,out_char,1);
        if (r == 1) return 1;
    }
    return 0;
}

void clear_kbd_buffer(int tty_fd) {
    char key;
    int k = 1;

    printf("clear_kbd_buffer\n");

    while(k > 0){
        k = kbd_read_char(tty_fd,&key);

        if (k > 0){
            printf("Flushing %c key from buffer\n",key);
        }
    }
}
