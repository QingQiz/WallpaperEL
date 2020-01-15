#ifndef _OPTIONS_H
#define _OPTIONS_H
#include <getopt.h>

#define MAX_MONITOR_N 10

typedef struct file_list_s {
    char *file_name;
    struct file_list_s *next;
} file_list;

typedef struct {
    char list_monitors;

    file_list* monitor[MAX_MONITOR_N + 1];

    char fifo;

    float dt;

    char loop;
} we_option;

enum we_long_opt {
    WE_LIST_MONITORS = 1,
    WE_ELSE,
    WE_FADE_IN_FADE_OUT,
    WE_LOOP,
};

extern we_option opts;


void parse_opts(int argc, char**argv);
#endif

