#ifndef _WALLPAPER_H
#define _WALLPAPER_H
#include <Imlib2.h>
#include <X11/Xlib.h>

typedef struct pixmap_list_s {
    Pixmap pmap;
    struct pixmap_list_s *next;
} pixmap_list;

void WESetWallpaperByOptions();
#endif

