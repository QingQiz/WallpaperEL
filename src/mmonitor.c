#include <stdlib.h>
#include "mmonitor.h"

int WEGetMonitorList(Display* display, Window window, WEMonitor** monitors, int* nmonitor) {
    XRRScreenResources *xrrr = XRRGetScreenResources(display, window);
    XRRCrtcInfo *crtc_info;

    if (xrrr == NULL) return -1;

    *monitors = (WEMonitor*)malloc(xrrr->ncrtc * sizeof(WEMonitor));

    int cnt = 0;
    for (int i = 0; i < xrrr->ncrtc; ++i) {
        crtc_info = XRRGetCrtcInfo(display, xrrr, xrrr->crtcs[i]);
        if (crtc_info == NULL) continue;
        if (crtc_info->height != 0 && crtc_info->width != 0) {
            (*monitors)[cnt].height = crtc_info->height;
            (*monitors)[cnt].width = crtc_info->width;
            (*monitors)[cnt].x = crtc_info->x;
            (*monitors)[cnt].y = crtc_info->y;
            cnt++;
        }
        XRRFreeCrtcInfo(crtc_info);
    }
    *nmonitor = cnt;
    XRRFreeScreenResources(xrrr);

    return 0;
}

void WEFreeMonitorList(WEMonitor *monitors) {
    free(monitors);
}
