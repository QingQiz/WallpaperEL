#ifndef _OPTIONS_H
#define _OPTIONS_H
#include <getopt.h>

#define MAX_MONITOR_N 10

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
    char ignore_current;
    char less_memory;
    char *bgm;
    char bgm_loop;
    double dt;
    int max_preload;
} we_option;

enum we_long_opt {
    WE_LIST_MONITORS = 1,
    WE_ELSE,
    WE_FADE_IN_FADE_OUT,
    WE_LOOP,
    WE_IGNORE_CURRENT,
    WE_LESS_MEMORY,
    WE_BGM,
    WE_BGM_LOOP,
    WE_WALLPAPER_MAX_PRELOAD
};

extern we_option opts;


void WEParseOpts(int argc, char **argv);
void usage();
#endif

