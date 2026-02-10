#include "kbd.h"

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include <errno.h>
#include <string.h>

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

/* Complete ESC sequence bytes without blocking; returns 1 complete, 0 incomplete, -1 error. */
static int kbd_complete_esc_sequence(int tty_fd, unsigned char *esc_buf, int *esc_len) {
    while (*esc_len < 4) {
        ssize_t r = read(tty_fd, &esc_buf[*esc_len], 1);
        if (r == 1) {
            (*esc_len)++;
            continue;
        }
        if (r == 0 || (r == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))) {
            return 0;
        }
        perror("read");
        fflush(stderr);
        return -1;
    }
    return 1;
}

/* Handle a completed ESC sequence; returns 1 and sets out_char, or -1 on error. */
static int kbd_handle_esc_sequence(unsigned char *esc_buf, int *esc_len,
                                   unsigned char *pending, int *pending_len,
                                   int pending_size, char *out_char) {

    /* Detect FUNC key - sequence: 27, 91, 49, 126 */
    if (esc_buf[0] == 27 && esc_buf[1] == 91 && esc_buf[2] == 49 && esc_buf[3] == 126) {
        *out_char = 'F';
        *esc_len = 0;
        return 1;
    }

    /* Unknown ESC sequence: return the first byte now and buffer the rest. */
    *out_char = (char)esc_buf[0];
    if (*pending_len < pending_size) {
        int to_copy = 3;
        if (to_copy > pending_size - *pending_len) {
            to_copy = pending_size - *pending_len;
        }
        memcpy(pending + *pending_len, esc_buf + 1, (size_t)to_copy);
        *pending_len += to_copy;
    }
    *esc_len = 0;
    return 1;
}

/* Read a single byte from TTY (non-blocking). Returns 1 on success, 0 if no data, -1 on error. */
static int kbd_read_char_raw(int tty_fd, char *out_char) {
    if (tty_fd < 0 || !out_char) return -1;
    fd_set set;
    struct timeval timeout;

    FD_ZERO(&set);
    FD_SET(tty_fd, &set);
    timeout.tv_sec  = 0;
    timeout.tv_usec = 20000;

    int rv = select(tty_fd+1, &set, NULL, NULL, &timeout);
    if (rv == -1) {
        perror("select");
        fflush(stderr);
        return -1;
    }
    else if (rv > 0 && FD_ISSET(tty_fd, &set)) {
        ssize_t r = read(tty_fd, out_char, 1);
        if (r == 1) return 1;
    }
    return 0;
}

/**
 * Read a key with mappings/ESC handling.
 *
 * out_char could be set to one of:
 *  - 0-9 for numeric keys
 *  - F for FUNC key
 *  - R for RED key
 *  - Y for YELLOW key
 *  - G for GREEN key
 *
 *  ALPHA key is not recognized by this function
 *
 * Returns 1 on success, 0 if no data or incomplete ESC, -1 on error.
 */
int kbd_read_char(int tty_fd, char *out_char) {
    if (tty_fd < 0 || !out_char) return -1;
    static unsigned char pending[8];
    static int pending_len = 0;
    static unsigned char esc_buf[4];
    static int esc_len = 0;

    /* Return buffered bytes first (from a previously split sequence). */
    if (pending_len > 0) {
        *out_char = (char)pending[0];
        if (pending_len > 1) {
            memmove(pending, pending + 1, (size_t)(pending_len - 1));
        }
        pending_len--;
        return 1;
    }

    /* Continue an in-progress ESC sequence without blocking. */
    if (esc_len > 0) {
        int r = kbd_complete_esc_sequence(tty_fd, esc_buf, &esc_len);
        if (r <= 0) return r;
        return kbd_handle_esc_sequence(
            esc_buf, &esc_len, pending, &pending_len, (int)sizeof(pending), out_char);
    }

    /* Read one byte via the raw reader and apply simple mappings. */
    unsigned char ch;
    int r = kbd_read_char_raw(tty_fd, (char *)&ch);
    if (r <= 0) return r;

    if (ch == 0) { // Red key
        *out_char = 'R';
        return 1;
    }

    if (ch == 127) { // Yellow key
        *out_char = 'Y';
        return 1;
    }

    if (ch == 10) { // Green key
        *out_char = 'G';
        return 1;
    }

    if (ch != 27) { // Other keys not using ESC sequences - the numbers 0-9
        *out_char = (char)ch;
        return 1;
    }

    /* Start parsing ESC [ 1 ~ into a single 'F'. For the 'Func' key */
    esc_buf[0] = ch;
    esc_len = 1;
    r = kbd_complete_esc_sequence(tty_fd, esc_buf, &esc_len);
    if (r <= 0) return r;

    return kbd_handle_esc_sequence(
        esc_buf, &esc_len, pending, &pending_len, (int)sizeof(pending), out_char);
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
