#ifndef _WALLPAPER_H
#define _WALLPAPER_H
#include <Imlib2.h>
#include <X11/Xlib.h>
#define FIFO_SETP 12

void WESetWallpaper(Pixmap img);
void WESetWallpaperByOptions();

#endif

