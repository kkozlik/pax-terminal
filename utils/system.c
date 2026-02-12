#include "system.h"
#include <string.h>

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
