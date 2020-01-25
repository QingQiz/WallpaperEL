#include <stdio.h>

#include "wallpaper.h"
#include "imtools.h"
#include "options.h"
#include "mmonitor.h"


int main(int argc, char** argv) {
    WEImtoolsInit();

    WEParseOpts(argc, argv);
    if (opts.list_monitors) {
        printf("Monitor List:\n");
        for (int i = 0; i < monitor_n; ++i) {
            printf("\tm%d : %dx%d\t%d+%d\n", i,
                    monitor_l[i].width, monitor_l[i].height, monitor_l[i].x, monitor_l[i].y);
        }
        return 0;
    }
    WESetWallpaperByOptions();

    WEImtoolsDestruct();
    return 0;
}

