#include "wallpaper.h"
#include "imtools.h"
#include "options.h"
#include "mmonitor.h"


int main(int argc, char** argv) {
    parse_opts(argc, argv);

    init_x_and_imtools();

    if (opts.list_monitors) {
        WEMonitor *wms;
        int wmn;
        WEGetMonitorList(disp, root, &wms, &wmn);
        printf("Monitor List:\n");
        for (int i = 0; i < wmn; ++i) {
            printf("\tm%d : %dx%d\t%d+%d\n", i,
                    wms[i].width, wms[i].height, wms[i].x, wms[i].y);
        }
        WEFreeMonitorList(wms);
        return 0;
    }
    WESetWallpaperByOptions();

    destruct_imtools();
    return 0;
}

