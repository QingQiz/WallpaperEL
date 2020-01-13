#ifndef _WALLPAPER_H
#define _WALLPAPER_H
#include <Imlib2.h>
#include <X11/Xlib.h>

#define FIFO_SETP 12
#define MIN_FIFO_ENABLE_TIME 1.5


typedef struct image_list_s {
    Imlib_Image im;
    struct image_list_s *next;
} image_list;

typedef struct pixmap_list_s {
    Pixmap pmap;
    struct pixmap_list_s *next;
} pixmap_list;
// void WESetWallpaper(Pixmap img);
void WESetWallpaperByOptions();

#endif

