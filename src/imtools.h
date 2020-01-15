#ifndef _IMTOOLS_H
#define _IMTOOLS_H

#include <Imlib2.h>
#include "mmonitor.h"


void init_x_and_imtools();

void destruct_imtools();

int image_get_width(Imlib_Image im);

int image_get_height(Imlib_Image im);

Imlib_Image image_set_alpha(Imlib_Image im, int alpha);

void image_free(Imlib_Image im);

void render_image_part_on_drawable_at_size(
    Drawable d,
    Imlib_Image im,
    int sx, int sy,
    int sw, int sh,
    int dx, int dy,
    int dw, int dh,
    char dither,
    char blend,
    char alias
);

void copy_pixmap(Display *disp, Pixmap pm_d, Pixmap pm_s);


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

