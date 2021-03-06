#ifndef _IMTOOLS_H
#define _IMTOOLS_H

#include <Imlib2.h>
#include "mmonitor.h"

typedef struct image_list_s {
    Imlib_Image im;
    struct image_list_s *next, *prev;
} image_list;


// init & deinit
void WEImtoolsInit();
void WEImtoolsDestruct();

// image information
int WEGetImageWidth(Imlib_Image im);
int WEGetImageHeight(Imlib_Image im);

// operations on image
void WEFreeImage(Imlib_Image im);
void WELoadNextImage();
void WECopyPixmap(Display* disp, Pixmap pm_d, Pixmap pm_s);
Imlib_Image WESetImageAlpha(Imlib_Image im, int alpha);

// rendering
void WERenderImagePartOnDrawableAtSize(
        Drawable d, Imlib_Image im,
        int sx, int sy, int sw, int sh,
        int dx, int dy, int dw, int dh,
        char dither, char blend, char alias
);
void WERenderCurrentImageToPixmap(Pixmap pmap, int alpha);

// filling method
void WEBgFilled(Pixmap pmap, Imlib_Image im, int x, int y, int w, int h);

// extern variables
extern Display *disp;
extern Visual *vis;
extern Screen *scr;
extern int depth;
extern Atom wmDeleteWindow;
extern Window root;
extern WEMonitor *monitor_l;
extern int monitor_n;
extern Pixmap oldclient;
#endif

