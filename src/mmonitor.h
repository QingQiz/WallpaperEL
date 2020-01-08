#ifndef _MMONITOR_H
#define _MMONITOR_H

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

typedef struct {
    int width, height;
    int x, y;
} WEMonitor;

int WEGetMonitorList(Display* display, Window window, WEMonitor** monitors, int* nmonitor);

void WEFreeMonitorList(WEMonitor *monitors);

#endif

