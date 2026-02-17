#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <sys/statvfs.h>

typedef struct {
    int  capacity_percent;
    int  voltage_raw;
    char status[128];
    int  valid_capacity;
    int  valid_voltage;
    int  valid_status;
} BatteryInfo;


double get_available_space_mb(const char *path);
int read_battery_info(BatteryInfo *info);
int read_line_from_file(const char *path,char *buf,size_t bufsize);

void run_Loader(void);
void launch_single_module(const char *module_name);
void launch_L_module(const char *module_name);

