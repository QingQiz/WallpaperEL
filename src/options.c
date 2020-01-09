#include <stdio.h>
#include <stdlib.h>
#include "string.h"

#include "options.h"
#include "debug.h"

static struct option long_options[] = {
    {"list-monitors", no_argument, 0, LIST_MONITORS},
    {"help",          no_argument, 0, 'h'},
    {0,               0,           0, 0},
};
we_option opts;


static void init_opts() {
    opts.list_monitors = 0;
    opts.monitor_specific = 0;

    memset(opts.monitor, 0, sizeof(opts.monitor));
}

static void usage() {
    fputs(
      #include "help.inc"
    , stdout);
    exit(-1);
}

void parse_opts(int argc, char**argv) {
    if (argc == 1) usage();

    init_opts();

    int optret, l_optidx;
    int monitor_n;

    while (1) {
        optret = getopt_long(argc, argv, "hm:", long_options, &l_optidx);
        if (optret == -1) break;

        switch (optret) {
            case LIST_MONITORS:
                opts.list_monitors = 1;
                D("opt list-monitors set");
                break;
            case 'm':
                if (*optarg == 'l') {
                    opts.list_monitors = 1;
                    break;
                }

                monitor_n = atoi(optarg);

                assert(monitor_n < MAX_MONITOR_N && monitor_n >= 0,
                        "-m%d: index error", monitor_n);

                assert(optind < argc && argv[optind][0] != '-',
                        "-m%d: require a file", monitor_n);

                opts.monitor[monitor_n] = argv[optind];
                D("monitor %d set to %s", monitor_n, argv[optind]);
                opts.monitor_specific = 1;
                break;
            case 'h':
            default:
                usage();
        }
    }
    if (!opts.monitor_specific) {
        opts.monitor[0] = argv[argc - 1];
    }
}
