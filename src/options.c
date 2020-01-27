#include <stdio.h>
#include <stdlib.h>
#include "string.h"

#include "options.h"
#include "debug.h"
#include "imtools.h"

static struct option long_options[] = {
    {"list-monitors", no_argument,       0, WE_LIST_MONITORS},
    {"else",          no_argument,       0, WE_ELSE},
    {"fifo",          no_argument,       0, WE_FADE_IN_FADE_OUT},
    {"loop",          no_argument,       0, WE_LOOP},
    {"help",          no_argument,       0, 'h'},
    {0,               0,                 0, 0},
};
we_option opts;


static void init_opts() {
    memset(&opts, 0, sizeof(opts));

    opts.dt = 60;
}

static void usage() {
    fputs(
      #include "help.inc"
    , stdout);
    exit(-1);
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
            default:
                usage();
        }
    }
    if (!is_opt_set) usage();
}
