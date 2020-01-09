#include <stdio.h>
#include <stdlib.h>

#include "options.h"
#include "debug.h"

static struct option long_options[] = {
    {"list-monitors", no_argument, 0, LIST_MONITORS},
    {"help",          no_argument, 0, 'h'},
    {0,               0,           0, 0},
};
we_option opts;


static void init_opts() {
    int i;
    opts.list_monitors = 0;

    for (i = 0; i < MAX_MONITOR_N; ++i)
        opts.monitor[i] = NULL;
}

static void usage() {
    fputs(
      #include "help.inc"
    , stdout);
    exit(-1);
}

void proc_opts(int argc, char**argv) {
    if (argc == 1) usage();

    init_opts();

    int optret, l_optidx;
    int monitor_n;

    while (1) {
        optret = getopt_long(argc, argv, "hm:", long_options, &l_optidx);
        if (optret == -1) return;

        switch (optret) {
            case LIST_MONITORS:
                opts.list_monitors = 1;
                D("opt list-monitors set");
                break;
            case 'm':
                monitor_n = atoi(optarg);

                assert(monitor_n < MAX_MONITOR_N && monitor_n >= 0,
                        "-m%d: index error", monitor_n);

                assert(optind < argc && argv[optind][0] != '-',
                        "-m%d: require a file", monitor_n);

                opts.monitor[monitor_n] = argv[optind + 1];
                D("monitor %d set to %s", monitor_n, argv[optind]);
                break;
            case 'h':
            default:
                usage();
        }
    }
}
