#ifndef _WALLPAPER_H
#define _WALLPAPER_H
#include <Imlib2.h>
#include <X11/Xlib.h>
#include <semaphore.h>

#define MAX_WALLPAPER_PRELOAD 128

#define FIFO_SETP 12
#define FIFO_DURATION 0.61f
#define MIN_FIFO_ENABLE_TIME 1.5


typedef enum {
    UNUSED = 0,
    USING,
    USED
} pmapl_status_t;

typedef struct pixmap_list_s {
    Pixmap pmap;
    struct pixmap_list_s *next, *prev;
    pmapl_status_t status;
} pixmap_list;

void WESetWallpaperByOptions();
#endif

