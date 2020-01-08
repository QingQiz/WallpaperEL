#ifndef _IMTOOLS_H
#define _IMTOOLS_H

#include <Imlib2.h>


void init_x_and_imtools();

void destruct_imtools();

int image_get_width(Imlib_Image im);

int image_get_height(Imlib_Image im);

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


extern Display *disp;
extern Visual *vis;
extern Screen *scr;
extern Colormap cm;
extern int depth;
extern Atom wmDeleteWindow;
extern Window root;

#endif
