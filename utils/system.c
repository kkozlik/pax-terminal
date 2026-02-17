#include "system.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define CP_PATH       "/data/app/MAINAPP/bin/cp"
#define LIB_DIR       "/data/app/MAINAPP/lib"
#define LIB_OSAL_PATH "/data/app/MAINAPP/lib/libosal.so"
#define LOADER_PATH   "/data/app/MAINAPP/Loader"
#define L_PATH        "/data/app/MAINAPP/L"

double get_available_space_mb(const char *path) {
    struct statvfs stat;
    if (statvfs(path,&stat) != 0) {
        perror("Chyba při volání statvfs");
        fflush(stderr);
        return -1.0;
    }
    return (double)stat.f_bsize * stat.f_bavail / (1024.0 * 1024.0);
}

int read_line_from_file(const char *path,char *buf,size_t bufsize) {
    FILE *f = fopen(path,"r");
    if (!f) {
        perror("Chyba při otevírání souboru");
        fflush(stderr);
        return -1;
    }
    if (!fgets(buf,(int)bufsize,f)) {
        perror("Chyba při čtení souboru");
        fflush(stderr);
        fclose(f);
        return -1;
    }
    fclose(f);
    size_t len = strlen(buf);
    if (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r')) {
        buf[len-1] = '\0';
    }
    return 0;
}

int read_battery_info(BatteryInfo *info) {
    if (!info) return -1;
    memset(info,0,sizeof(*info));
    char buf[256];

    if (read_line_from_file("/sys/class/power_supply/battery/capacity",buf,sizeof(buf)) == 0) {
        info->capacity_percent = atoi(buf);
        info->valid_capacity   = 1;
    }
    if (read_line_from_file("/sys/class/power_supply/battery/voltage_now",buf,sizeof(buf)) == 0) {
        info->voltage_raw   = atoi(buf);
        info->valid_voltage = 1;
    }
    if (read_line_from_file("/sys/class/power_supply/battery/status",buf,sizeof(buf)) == 0) {
        strncpy(info->status,buf,sizeof(info->status)-1);
        info->status[sizeof(info->status)-1] = '\0';
        info->valid_status = 1;
    }
    return (info->valid_capacity || info->valid_voltage || info->valid_status) ? 0 : -1;
}

static int copy_module_to_libosal(const char *module_name) {
    char src[256];
    snprintf(src,sizeof(src),"%s/%s",LIB_DIR,module_name);

    printf("Start... fork (copy %s)\n",module_name);
    fflush(stdout);

    pid_t pid = fork();
    if (pid == -1) {
        perror("Error calling fork");
        fflush(stderr);
        return -1;
    }
    else if (pid == 0) {
        printf("Start... execv CP (%s -> %s)\n",src,LIB_OSAL_PATH);
        fflush(stdout);

        execl(CP_PATH, CP_PATH, src, LIB_OSAL_PATH, (char *)0);

        perror("Error executing cp using execv");
        printf("End... execv CP (error)\n");
        fflush(stdout);
        _exit(1);
    }
    else {
        int status = 0;
        waitpid(pid, &status, 0);
        printf("Child (cp %s) exited with return code: %d\n",
               module_name, WEXITSTATUS(status));
        fflush(stdout);
    }

    return 0;
}

void run_Loader(void) {
    printf("Start... execl Loader (it's a script) including copying\n");
    fflush(stdout);

    execl(LOADER_PATH, "Loader", "", (char *)0);

    perror("Error executing Loader using execl");
    fflush(stderr);
    _exit(1);
}

static void run_L(void) {
    pid_t pid;
    int status;

    printf("Start... fork (L)\n");
    fflush(stdout);

    pid = fork();
    if (pid == -1) {
        perror("Error calling fork");
        fflush(stderr);
        _exit(1);
    }
    else if (pid == 0) {
        printf("Start... execv L (original renamed Loader)\n");
        fflush(stdout);

        execl(L_PATH, "L", "", (char *)0);

        perror("Error executing L using execl");
        printf("End... execv L (error)\n");
        fflush(stdout);
        _exit(1);
    }
    else {
        waitpid(pid, &status, 0);

        printf("Child (L) exited with return code: %d\n", WEXITSTATUS(status));
        fflush(stdout);
    }

    printf("End... fork\n");
    fflush(stdout);
}

void launch_single_module(const char *module_name) {

    if (copy_module_to_libosal(module_name) == 0) {
        run_L();
        run_Loader();
    }

    printf("launch_single_module(%s) - error: Loader did not start.\n", module_name);
    fflush(stdout);
}

void launch_L_module(const char *module_name) {

    if (copy_module_to_libosal(module_name) == 0) {
        run_L();
    }

    printf("launch_single_module(%s) - error: Loader did not start.\n", module_name);
    fflush(stdout);
}
