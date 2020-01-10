#ifndef _OPTIONS_H
#define _OPTIONS_H
#include <getopt.h>

#define MAX_MONITOR_N 10

typedef struct {
    char list_monitors;

    char* monitor[MAX_MONITOR_N];
    char* else_monitor;
} we_option;

enum we_long_opt {
    WE_LIST_MONITORS = 1,
    WE_ELSE,
};

extern we_option opts;


void parse_opts(int argc, char**argv);
#endif

