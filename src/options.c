#include <stdio.h>
#include <stdlib.h>
#include "string.h"

#include "wallpaper.h"
#include "options.h"
#include "common.h"
#include "imtools.h"

static struct option long_options[] = {
    {"list-monitors", no_argument,       0, WE_LIST_MONITORS},
    {"else",          no_argument,       0, WE_ELSE},
    {"fifo",          no_argument,       0, WE_FADE_IN_FADE_OUT},
    {"loop",          no_argument,       0, WE_LOOP},
    {"help",          no_argument,       0, 'h'},
    {"ignore-current",no_argument,       0, WE_IGNORE_CURRENT},
    {"less-memory",   no_argument,       0, WE_LESS_MEMORY},
    {"bgm",           required_argument, 0, WE_BGM},
    {"bgm-loop",      no_argument,       0, WE_BGM_LOOP},
    {"max-preload",   required_argument, 0, WE_WALLPAPER_MAX_PRELOAD},
    {0,               0,                 0, 0},
};
we_option opts;


static void init_opts() {
    memset(&opts, 0, sizeof(opts));
    opts.dt = 60;
    opts.max_preload = MAX_WALLPAPER_PRELOAD;
}

void usage() {
    fputs(
      #include "help.inc"
    , stdout);
}

static file_list* create_file_list_by_options() {
    file_list *head = (file_list*)malloc(sizeof(file_list));
    file_list *iter = head;
    iter->file_name = opts.argv[optind++];

    while (optind < opts.argc && opts.argv[optind][0] != '-') {
        iter->next = (file_list*)malloc(sizeof(file_list));
        iter = iter->next;
        iter->file_name = opts.argv[optind++];
    }
    iter->next = head;
    return head;
}

void WEParseOpts(int argc, char **argv) {
    char is_opt_set = 0;

    init_opts();

    int optret, l_optidx;
    int mi;

    opts.argc = argc, opts.argv = argv;
    while (1) {
        optret = getopt_long(argc, argv, "hm:t:", long_options, &l_optidx);
        if (optret == -1) break;

        is_opt_set = 1;
        switch (optret) {
            case WE_LIST_MONITORS:
                opts.list_monitors = 1;
                D("opt list-monitors set");
                break;
            case WE_ELSE:
                opts.monitor[MAX_MONITOR_N] = create_file_list_by_options();
                break;
            case WE_FADE_IN_FADE_OUT:
                opts.fifo = 1;
                break;
            case WE_LOOP:
                opts.loop = 1;
                break;
            case WE_IGNORE_CURRENT:
                opts.ignore_current = 1;
                break;
            case WE_LESS_MEMORY:
                opts.less_memory = 1;
                break;
            case WE_BGM:
                opts.bgm = optarg;
                break;
            case WE_BGM_LOOP:
                opts.bgm_loop = 1;
                break;
            case WE_WALLPAPER_MAX_PRELOAD:
                opts.max_preload = atoi(optarg);
                break;
            case 'm':
                if (*optarg == 'l') {
                    opts.list_monitors = 1;
                    break;
                }
                mi = atoi(optarg);

                assert(mi < monitor_n && mi >= 0, "-m%d: index error, no such monitor", mi);
                assert(optind < argc && argv[optind][0] != '-', "-m%d: require a file or file list", mi);

                opts.monitor[mi] = create_file_list_by_options();

                break;
            case 't':
                opts.dt = atof(optarg);
                break;
            case 'h':
                usage();
                exit(0);
            default:
                usage();
                exit(-1);
        }
    }
    if (!is_opt_set){
        usage();
        exit(-1);
    }
}
