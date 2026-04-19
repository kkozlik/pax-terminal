
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "../utils/fb.h"
#include "../utils/kbd.h"
#include "../utils/gfx.h"
#include "../utils/bmp.h"
#include "../utils/system.h"
#include "../utils/printer.h"
#include "plh_quest.h"
#include "mp3.h"


#define BMP_FILENAME1 "iquest/iquest_bg1.bmp"
#define BMP_FILENAME2 "iquest/iquest_bg2.bmp"

#define ACT_ACTIVATE  1
#define ACT_PRINT_PLH 2
#define ACT_MUSIC_1   3
#define ACT_MUSIC_2   4
#define ACT_CLOCK     5

char pin[6];
int pin_len = 0;
int wrong_code = 0;
int activated = 0;

BITMAPINFOHEADER bih1, bih2;
uint8_t *bmp_data1, *bmp_data2;

void activate(){
    activated = 1;

    PrintBuffer *pb = create_print_buffer(PRINTER_WIDTH, 150+PRINTER_BOTTOM_BORDER);

    print_horizontal_line(pb, 0);
    print_horizontal_line(pb, 150);

    print_text(pb, 92, 85, "I.Q:KRADEZOK");

    send_to_printer(pb);
    free_print_buffer(pb);
}

void print_plh(void){
    PrintBuffer *pb = create_print_buffer(PRINTER_WIDTH, PLH_QUEST_HEIGHT_PX+PRINTER_BOTTOM_BORDER);
    if (!pb) {
        perror("Print buffer init failed");
        fflush(stderr);
        return;
    }

    print_bitmap(pb, 0, 0, plh_quest, PLH_QUEST_WIDTH_BYTES, PLH_QUEST_HEIGHT_PX);

    send_to_printer(pb);
    free_print_buffer(pb);
}

int is_activated(){ return activated; }

void code_invalid(){ wrong_code = 20; }

void clear_pin(void){
    pin_len = 0;
}

void read_pin(char key){
    if (key >= '0' && key <= '9' && pin_len < 6){
        pin[pin_len] = key - '0';
        pin_len++;
    }

    if (key == 'Y' && pin_len > 0){
        pin_len--;
    }

    if (key == 'R'){
        clear_pin();
    }
}

void draw_background(color_t *buffer){
    if (is_activated()) {
        draw_bmp_image(buffer, 0, 0, bih2, bmp_data2);
    }
    else {
        draw_bmp_image(buffer, 0, 0, bih1, bmp_data1);
    }
}

void draw_pin(color_t *buffer, int x, int y, color_t color){
    if (wrong_code){
        draw_text(buffer, x+55, y-7, "BAD CODE", color_red);
        return;
    }

    for(int i=0; i<pin_len; i++){
        draw_digit(buffer, x+i*30, y, pin[i], color);
    }

    for(int i=pin_len; i<6; i++){
        draw_line(buffer, x+i*30, y, x+i*30+24, y, color);
    }
}

void draw_battery(color_t *buffer, BatteryInfo *bat, color_t color){
    char line[128];
    int y_pos = SCREEN_HEIGHT - 8;

    if (bat->valid_capacity) {
        snprintf(line, sizeof(line), "Accu: %02d %%",bat->capacity_percent);
        draw_text(buffer, 10, y_pos, line, color);
    }

    if (bat->valid_status) {
        int status_len = (int)strlen(bat->status);
        draw_text(buffer, SCREEN_WIDTH-10-(8*status_len), y_pos, bat->status, color);
    }
}

void play_music(const char *filename, const int tty_fd, color_t *buffer, color_t *fbp){

    clear_buffer(buffer, color_black);
    draw_background(buffer);

    draw_text(buffer, 72, SCREEN_HEIGHT - 54, "LISTEN MUSIC", is_activated() ? color_black : color_white);
    draw_text(buffer, 60, SCREEN_HEIGHT - 44, "press   to stop", is_activated() ? color_black : color_white);
    draw_text(buffer, 108, SCREEN_HEIGHT - 44, "X", color_red);

    draw_buffer(buffer, fbp);

    play_mp3(filename, tty_fd);
}


void switch_to_vhb_clock(){
    // Vytvoří flag file, který Loader použije pro spuštění hodiny místo iquestu
    FILE *f = fopen("/tmp/vhb_flag", "w");
    if (f) {
        fprintf(f, "1");
        fclose(f);
    }
     else {
        perror("Failed to create flag file for clock");
        fflush(stderr);
    }

    run_Loader();
    _exit(0);
}

int decode_pin(void){
    typedef struct {
        char code[6];
        int action;
    } PinAction;

    static const PinAction pin_map[] = {
        {{1,5,9,3,5,7}, ACT_ACTIVATE},
        {{9,7,1,4,5,0}, ACT_PRINT_PLH},
        {{0,3,8,6,6,9}, ACT_MUSIC_1},
        {{2,2,1,5,2,3}, ACT_MUSIC_2},
        {{2,2,3,6,0,6}, ACT_CLOCK},
    };

    if (pin_len < 6) return 0;

    for (size_t i = 0; i < sizeof(pin_map) / sizeof(pin_map[0]); i++) {
        if (memcmp(pin, pin_map[i].code, sizeof(pin_map[i].code)) == 0) {
            return pin_map[i].action;
        }
    }

    return 0;
}

int _init(void) {
    printf("Start... Iquest menu\n");
    fflush(stdout);

    color_t draw_color;

    int fb_fd = -1;
    color_t *fbp = fb_init(&fb_fd);
    if (!fbp) {
        perror("Framebuffer init failed");
        fflush(stderr);

        return EXIT_FAILURE;
    }

    color_t *buffer = init_draw_buffer();
    if (!buffer) {
        fb_close(fbp, fb_fd);
        return EXIT_FAILURE;
    }

    int tty_fd = kbd_init();
    if (tty_fd < 0) {
        perror("Keyboard init failed");
        fflush(stderr);

        free_draw_buffer(buffer);
        fb_close(fbp,fb_fd);
        return EXIT_FAILURE;
    }

    bmp_data1 = loadBMP(BMP_FILENAME1, &bih1);
    if (!bmp_data1) {
        free_draw_buffer(buffer);
        fb_close(fbp,fb_fd);
        kbd_close(tty_fd);
        return EXIT_FAILURE;
    }

    bmp_data2 = loadBMP(BMP_FILENAME2, &bih2);
    if (!bmp_data2) {
        freeBMP(bmp_data1);
        free_draw_buffer(buffer);
        fb_close(fbp,fb_fd);
        kbd_close(tty_fd);
        return EXIT_FAILURE;
    }

    clear_kbd_buffer(tty_fd);

    while (1) {
        clear_buffer(buffer, color_black);

        draw_background(buffer);

        if (is_activated()) draw_color = color_black;
        else                draw_color = color_white;

        BatteryInfo bat;
        if (read_battery_info(&bat) == 0) {
            draw_battery(buffer, &bat, draw_color);
        }

        char key;
        int k = kbd_read_char(tty_fd, &key);
        if (k == -1) break;
        else if (k == 1) {
            printf("KEY: %c\n", key);
            read_pin(key);

            // TODO: remove
            // if (key == 'F') break;

            if (key == 'G'){
                int action = decode_pin();
                clear_pin();

                printf("action: %d\n", action);

                if (!action) code_invalid();
                else if (action == ACT_ACTIVATE){
                    if (!is_activated()) activate();
                    else code_invalid();
                }
                else if (action == ACT_PRINT_PLH){
                    if (is_activated()) print_plh();
                    else code_invalid();
                }
                else if (action == ACT_MUSIC_1){
                    if (is_activated()) play_music("iquest/hudba_1.mp3", tty_fd, buffer, fbp);
                    else code_invalid();
                }
                else if (action == ACT_MUSIC_2){
                    if (is_activated()) play_music("iquest/hudba_2.mp3", tty_fd, buffer, fbp);
                    else code_invalid();
                }
                else if (action == ACT_CLOCK){
                    switch_to_vhb_clock();
                }
            }

            fflush(stdout);
        }

        draw_pin(buffer, 31, SCREEN_HEIGHT - 40, draw_color);
        draw_buffer(buffer, fbp);


        // Mírná pauza po vykreslení, aby se snížila spotřeba CPU
        usleep(20000); // 20ms
        if (wrong_code) wrong_code--;
    }

    freeBMP(bmp_data1);
    freeBMP(bmp_data2);

    free_draw_buffer(buffer);
    fb_close(fbp,fb_fd);
    kbd_close(tty_fd);
    printf("Konec... Iquest menu\n");
    fflush(stdout);
    return EXIT_SUCCESS;
}
