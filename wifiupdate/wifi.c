#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/wait.h>

// #define HOST "my-server.com"
// #define PORT 80
// #define BASE_URL_PATH "/pax/"
#define HOST "example.org"
#define PORT 80
#define BASE_URL_PATH "/pax/"
#define LIST_FILE_REMOTE "komplet.txt"
#define LIST_FILE_LOCAL "komplet.txt"
#define LOG_FILE "err.txt"
#define BUFFER_SIZE 4096
#define TARGET_DIR "/data/app/MAINAPP"

#ifndef DT_DIR
#define DT_DIR 4
#endif

// --- FRAMEBUFFER ---
#define FB_PATH "/dev/fb"
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define DOT_SIZE 7
#define DOT_SPACING 1
#define MAX_GRADIENT_SIZE 300000

int fbfd = 0;
unsigned short *fbp = 0;
long int screensize = 0;
int cursor_x = 0;
int cursor_y = 0;

// --- DEKLARACE ---
int mkdir(const char *pathname, mode_t mode);
int chmod(const char *pathname, mode_t mode);
int symlink(const char *target, const char *linkpath);

// --- LOGOVÁNÍ ---
void my_log(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);

    char log_path[256];
    snprintf(log_path, sizeof(log_path), "%s/%s", TARGET_DIR, LOG_FILE);
    FILE *flog = fopen(log_path, "a");
    if (flog) {
        va_start(args, format);
        vfprintf(flog, format, args);
        va_end(args);
        fclose(flog);
    }
}

// --- GRAFIKA ---

void fb_init() {
    fbfd = open(FB_PATH, O_RDWR);
    if (fbfd == -1) return;
    screensize = SCREEN_WIDTH * SCREEN_HEIGHT * 2;
    fbp = (unsigned short *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((long)fbp == -1) { close(fbfd); fbfd = 0; return; }
    memset(fbp, 0, screensize);
}

void fb_close() {
    if (fbp && fbfd) { munmap(fbp, screensize); close(fbfd); }
}

// INVERZE BAREV (NEGATIV)
void fb_invert() {
    if (!fbp) return;

    // Projdeme celý buffer a znegujeme bity (bitwise NOT)
    // Protože screensize je v bajtech a fbp je short (2 byty), dělíme 2
    long int pixels = screensize / 2;
    for (long int i = 0; i < pixels; i++) {
        fbp[i] = ~fbp[i];
    }
}

unsigned short rgb565(int r, int g, int b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

unsigned short size_to_color(int size) {
    int r = 0, g = 0, b = 0;
    if (size > MAX_GRADIENT_SIZE) size = MAX_GRADIENT_SIZE;
    int val = (size * 1024) / MAX_GRADIENT_SIZE;

    if (val < 256) { b = 255; g = val; r = 0; }
    else if (val < 512) { val -= 256; b = 255 - val; g = 255; r = 0; }
    else if (val < 768) { val -= 512; b = 0; g = 255; r = val; }
    else { val -= 768; b = 0; g = 255 - val; r = 255; }
    return rgb565(r, g, b);
}

void draw_status_dot(int file_size) {
    if (!fbp) return;
    unsigned short color = size_to_color(file_size);
    int start_x = cursor_x;
    int start_y = cursor_y;

    for (int y = 0; y < DOT_SIZE; y++) {
        for (int x = 0; x < DOT_SIZE; x++) {
            int pos_x = start_x + x;
            int pos_y = start_y + y;
            if (pos_x < SCREEN_WIDTH && pos_y < SCREEN_HEIGHT) {
                fbp[pos_y * SCREEN_WIDTH + pos_x] = color;
            }
        }
    }
    int step = DOT_SIZE + DOT_SPACING;
    cursor_x += step;
    if (cursor_x + DOT_SIZE > SCREEN_WIDTH) { cursor_x = 0; cursor_y += step; }
    if (cursor_y + DOT_SIZE > SCREEN_HEIGHT) cursor_y = 0;
}


// --- SYMLINK LOGIC ---

void create_manual_symlink(const char *target_bin, const char *link_dir, const char *link_name) {
    char link_path[512];
    snprintf(link_path, sizeof(link_path), "%s/%s", link_dir, link_name);

    unlink(link_path);

    if (symlink(target_bin, link_path) == 0) {
        printf("   [LINK] %s -> %s\n", link_name, target_bin);
        fb_invert(); // BLIK!
        usleep(20000); // 20ms pauza, aby to bylo vidět
    } else {
        my_log("   [ERR] Link fail: %s (Err: %d)\n", link_path, errno);
    }
}

void generate_symlinks_from_binary(const char *binary_path, const char *link_dir) {
    int pipefd[2];
    pid_t pid;
    char buffer[32768];
    ssize_t count;

    my_log("--- GENEROVANI LINKU PRO: %s ---\n", binary_path);

    if (access(binary_path, X_OK) != 0) {
        my_log("   [SKIP] Binarka neexistuje: %s\n", binary_path);
        return;
    }

    if (pipe(pipefd) == -1) {
        my_log("   [ERR] Pipe failed\n");
        return;
    }

    pid = fork();
    if (pid == -1) {
        my_log("   [ERR] Fork failed\n");
        close(pipefd[0]); close(pipefd[1]);
        return;
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        execl(binary_path, binary_path, "--list", (char *)NULL);
        exit(127);
    } else {
        close(pipefd[1]);
        memset(buffer, 0, sizeof(buffer));
        int total_read = 0;
        while ((count = read(pipefd[0], buffer + total_read, sizeof(buffer) - 1 - total_read)) > 0) {
            total_read += count;
            if (total_read >= sizeof(buffer) - 1) break;
        }
        close(pipefd[0]);
        waitpid(pid, NULL, 0);

        if (total_read <= 0) {
            my_log("   [WARN] Zadny vystup.\n");
            return;
        }

        char *p = buffer;
        char *token;
        char *saveptr;

        token = strtok_r(buffer, "\n\r \t,", &saveptr);
        while (token != NULL) {
            if (strlen(token) > 1 && token[0] != '-' && isalpha(token[0])) {
                if (strcmp(token, "BusyBox") != 0 && strcmp(token, "usage:") != 0) {
                     char link_path[512];
                     snprintf(link_path, sizeof(link_path), "%s/%s", link_dir, token);
                     unlink(link_path);
                     if (symlink(binary_path, link_path) == 0) {
                         // printf("Link: %s\n", token);
                         fb_invert(); // BLIK!
                         usleep(10000); // 10ms pauza (u busyboxu je toho hodně, tak kratší)
                     }
                }
            }
            token = strtok_r(NULL, "\n\r \t,", &saveptr);
        }
        my_log("   [OK] Hotovo.\n");
    }
}


// --- SYSTEMOVÉ FUNKCE ---
int remove_directory_recursive(const char *path, int is_root) {
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = -1;
    if (d) {
        struct dirent *p;
        r = 0;
        while (!r && (p = readdir(d))) {
            int r2 = -1;
            char *buf;
            size_t len;
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) continue;
            len = path_len + strlen(p->d_name) + 2;
            buf = malloc(len);
            if (buf) {
                snprintf(buf, len, "%s/%s", path, p->d_name);
                if (p->d_type == DT_DIR) r2 = remove_directory_recursive(buf, 0);
                else {
                    r2 = unlink(buf);
                    if (r2 != 0 && (errno == EISDIR || errno == EPERM)) r2 = remove_directory_recursive(buf, 0);
                }
                free(buf);
            }
            r = r2;
        }
        closedir(d);
    }
    if (!r && !is_root) r = rmdir(path);
    return r;
}

void set_executable(const char *filename) {
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", TARGET_DIR, filename);
    if (chmod(full_path, 0755) == 0) {
        my_log("   [OK] +x: %s\n", filename);
    }
}

// --- POMOCNÉ FUNKCE ---
int my_mkdir(const char *path) { return mkdir(path, 0755); }

int mkpath(char *file_path) {
    char *p;
    char *start = file_path;
    if (file_path[0] == '/') start = file_path + 1;
    for (p = strchr(start, '/'); p; p = strchr(p + 1, '/')) {
        *p = '\0';
        if (my_mkdir(file_path) == -1 && errno != EEXIST) { *p = '/'; return -1; }
        *p = '/';
    }
    if (my_mkdir(file_path) == -1 && errno != EEXIST) return -1;
    return 0;
}

void trim_end(char *str) {
    int len = strlen(str);
    while(len > 0 && isspace((unsigned char)str[len-1])) {
        str[len-1] = '\0';
        len--;
    }
}

// --- NETWORK ---
int connect_socket() {
    struct hostent *host = gethostbyname(HOST);
    if (!host) return -1;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) return -1;
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    memset(&(server_addr.sin_zero), 0, 8);
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        close(sockfd);
        return -1;
    }
    return sockfd;
}

int download_file(const char *remote_path, const char *local_path) {
    int sockfd = connect_socket();
    if (sockfd == -1) { my_log("   [ERR] Socket fail: %s\n", remote_path); return -1; }

    char request[512];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "Connection: close\r\n\r\n",
             remote_path, HOST, PORT);
    send(sockfd, request, strlen(request), 0);

    FILE *fp = fopen(local_path, "wb");
    if (!fp) { my_log("   [ERR] Write fail: %s\n", local_path); close(sockfd); return -1; }

    char buffer[BUFFER_SIZE];
    int bytes_received, header_ended = 0, total_bytes = 0;
    while ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0) {
        char *data_ptr = buffer;
        int data_len = bytes_received;
        if (!header_ended) {
            for (int i = 0; i < bytes_received - 3; i++) {
                if (buffer[i] == '\r' && buffer[i+1] == '\n' && buffer[i+2] == '\r' && buffer[i+3] == '\n') {
                    header_ended = 1; data_ptr = buffer + i + 4; data_len = bytes_received - (i + 4); break;
                }
            }
            if (!header_ended) data_len = 0;
        }
        if (header_ended && data_len > 0) { fwrite(data_ptr, 1, data_len, fp); total_bytes += data_len; }
    }
    fclose(fp);
    close(sockfd);
    draw_status_dot(total_bytes);
    return 0;
}

// --- INIT ---
int _init(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    fb_init();

    if (chdir(TARGET_DIR) != 0) { mkdir(TARGET_DIR, 0755); chdir(TARGET_DIR); }



    my_log("--- START UPDATE ---\n");

    char list_url[256];
    snprintf(list_url, sizeof(list_url), "%s%s", BASE_URL_PATH, LIST_FILE_REMOTE);
    if (download_file(list_url, LIST_FILE_LOCAL) != 0) { my_log("FATAL: Seznam nestazen.\n"); fb_close(); return -1; }

    FILE *fp = fopen(LIST_FILE_LOCAL, "r");
    if (!fp) { fb_close(); return -1; }

    char line[512];
    char remote_dir_context[256] = "";
    char local_dir_context[256] = "";
    const char *keyword = "MAINAPP";

    printf("--- MAZU STAROU VERZI - mame seznam ke stazeni ---\n");
    remove_directory_recursive(TARGET_DIR, 1);

    while (fgets(line, sizeof(line), fp)) {
        trim_end(line);
        int len = strlen(line);
        if (len == 0) continue;

        if (line[len - 1] == ':') {
            line[len - 1] = '\0';
            strcpy(remote_dir_context, line);
            char *found = strstr(line, keyword);
            if (found) {
                char *rest = found + strlen(keyword);
                if (*rest == '/') rest++;
                if (strlen(rest) == 0) strcpy(local_dir_context, ".");
                else strcpy(local_dir_context, rest);
            } else local_dir_context[0] = '\0';

            if (strlen(local_dir_context) > 0 && strcmp(local_dir_context, ".") != 0) mkpath(local_dir_context);
        }
        else if (line[0] == '-') {
            if (strlen(local_dir_context) == 0) continue;
            char *filename = strrchr(line, ' ');
            if (filename) {
                filename++;
                if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) continue;
                char remote_url[512], local_path[512];
                snprintf(remote_url, sizeof(remote_url), "%s%s/%s", BASE_URL_PATH, remote_dir_context, filename);
                snprintf(local_path, sizeof(local_path), "%s/%s", local_dir_context, filename);
                printf("[GET] %s -> %s\n", filename, local_path);
                download_file(remote_url, local_path);
                usleep(5000);
            }
        }
    }
    fclose(fp);

    set_executable("Loader");
    set_executable("L");
    set_executable("bin/busybox");
    set_executable("bin/lrz");
    set_executable("bin/lsz");

    char bin_path[256];
    snprintf(bin_path, sizeof(bin_path), "%s/bin", TARGET_DIR);

    char bb_path[256];
    snprintf(bb_path, sizeof(bb_path), "%s/bin/busybox", TARGET_DIR);

    char lrz_path[256];
    snprintf(lrz_path, sizeof(lrz_path), "%s/bin/lrz", TARGET_DIR);

    char lsz_path[256];
    snprintf(lsz_path, sizeof(lsz_path), "%s/bin/lsz", TARGET_DIR);

    generate_symlinks_from_binary(bb_path, bin_path);

    create_manual_symlink(lrz_path, bin_path, "rz");
    create_manual_symlink(lrz_path, bin_path, "rx");
    create_manual_symlink(lrz_path, bin_path, "rb");

    create_manual_symlink(lsz_path, bin_path, "sz");
    create_manual_symlink(lsz_path, bin_path, "sx");
    create_manual_symlink(lsz_path, bin_path, "sb");

    my_log("--- KONEC ---\n");
    fb_close();
    return 0;
}
