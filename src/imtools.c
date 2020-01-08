#include <X11/Xlib.h>

#include "imtools.h"
#include "debug.h"

Display *disp = NULL;
Visual *vis = NULL;
Screen *scr = NULL;
Colormap cm;
int depth;
Atom wmDeleteWindow;
Window root = 0;


void init_x_and_imtools() {
    disp = XOpenDisplay(NULL);
    assert(disp, "Can't open X display");

    vis   = DefaultVisual  (disp, DefaultScreen(disp));
    depth = DefaultDepth   (disp, DefaultScreen(disp));
    cm    = DefaultColormap(disp, DefaultScreen(disp));
    root  = RootWindow     (disp, DefaultScreen(disp));
    scr   = ScreenOfDisplay(disp, DefaultScreen(disp));

    imlib_context_set_display(disp);
    imlib_context_set_visual(vis);
    imlib_context_set_colormap(cm);
    imlib_context_set_color_modifier(NULL);
    imlib_context_set_progress_function(NULL);
    imlib_context_set_operation(IMLIB_OP_COPY);

    wmDeleteWindow = XInternAtom(disp, "WM_DELETE_WINDOW", False);

    imlib_set_cache_size(4 * 1024 * 1024);
}

void destruct_imtools() {
    XCloseDisplay(disp);
    disp = NULL, vis = NULL, scr = NULL, root = 0;
}

int image_get_width(Imlib_Image im) {
   imlib_context_set_image(im);
   return imlib_image_get_width();
}

int image_get_height(Imlib_Image im) {
   imlib_context_set_image(im);
   return imlib_image_get_height();
}

void render_image_part_on_drawable_at_size(
        Drawable d,
        Imlib_Image im,
        int sx, int sy,
        int sw, int sh,
        int dx, int dy,
        int dw, int dh,
        char dither,
        char blend,
        char alias)
{
    imlib_context_set_image(im);
    imlib_context_set_drawable(d);
    imlib_context_set_anti_alias(alias);
    imlib_context_set_dither(dither);
    imlib_context_set_blend(blend);
    imlib_context_set_angle(0);
    imlib_render_image_part_on_drawable_at_size(sx, sy, sw, sh, dx, dy, dw, dh);
}
