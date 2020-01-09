#ifndef _OPTIONS_H
#define _OPTIONS_H
#include <getopt.h>

#define MAX_MONITOR_N 10

typedef struct {
    char list_monitors;

    char  monitor_specific;
    char* monitor[MAX_MONITOR_N];
} we_option;

enum we_long_opt {
    LIST_MONITORS = 1,
};

extern we_option opts;


void parse_opts(int argc, char**argv);
#endif

