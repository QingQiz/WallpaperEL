#ifndef _OPTIONS_H
#define _OPTIONS_H
#include <getopt.h>

#define MAX_MONITOR_N 10
#define FIFO_SETP 12
#define MIN_FIFO_ENABLE_TIME 1.5

typedef struct file_list_s {
    char *file_name;
    struct file_list_s *next;
} file_list;

typedef struct {
    int argc;
    char **argv;
    file_list* monitor[MAX_MONITOR_N + 1];
    char list_monitors;
    char fifo;
    char loop;
    float dt;
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

