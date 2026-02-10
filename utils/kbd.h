#pragma once

#include <stdio.h>
#include <stdlib.h>


#define TTY_PATH     "/dev/tty0"

int kbd_init(void);
void kbd_close(int tty_fd);
int kbd_read_char(int tty_fd,char *out_char);
void clear_kbd_buffer(int tty_fd);
