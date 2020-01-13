#include <X11/Xlib.h>

#include "imtools.h"
#include "debug.h"
#include "options.h"

Display *disp = NULL;
Visual *vis = NULL;
Screen *scr = NULL;
Pixmap oldclient;
int depth;
Atom wmDeleteWindow;
Window root = 0;
WEMonitor *monitor_l;
int monitor_n;

static int X_error_handler(Display *d, XErrorEvent *e) {
    if (e->error_code == BadPixmap) {
        assert(!opts.fifo, "Bad Pixmap, try running without --fifo");
    }
    return 0;
}

void init_x_and_imtools() {
    XSetErrorHandler(X_error_handler);

    disp = XOpenDisplay(NULL);
    assert(disp, "Can't open X display");

    vis   = DefaultVisual  (disp, DefaultScreen(disp));
    depth = DefaultDepth   (disp, DefaultScreen(disp));
    root  = RootWindow     (disp, DefaultScreen(disp));
    scr   = ScreenOfDisplay(disp, DefaultScreen(disp));

    oldclient = 0;
    imlib_context_set_display(disp);
    imlib_context_set_visual(vis);
    imlib_context_set_color_modifier(NULL);
    imlib_context_set_progress_function(NULL);
    imlib_context_set_operation(IMLIB_OP_COPY);

    wmDeleteWindow = XInternAtom(disp, "WM_DELETE_WINDOW", False);

    imlib_set_cache_size(4 * 1024 * 1024);

    WEGetMonitorList(disp, root, &monitor_l, &monitor_n);

    // free all if exit abnormally
    XSetCloseDownMode(disp, DestroyAll);
}

void destruct_imtools() {
    // do not free pixmap after exit
    XSetCloseDownMode(disp, RetainPermanent);
    if (oldclient) XKillClient(disp, oldclient);
    XCloseDisplay(disp);

    WEFreeMonitorList(monitor_l);
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

Imlib_Image image_set_alpha(Imlib_Image im, int alpha) {
    static Imlib_Image im_aph = 0, im_cpy = 0;
    if (im_aph) image_free(im_aph);
    if (im_cpy) image_free(im_cpy);

    imlib_context_set_image(im);
    int h = imlib_image_get_height();
    int w = imlib_image_get_width();

    im_aph = imlib_create_image(w, h);
    imlib_context_set_image(im_aph);
    imlib_image_set_has_alpha(1);
    imlib_context_set_color(0, 0, 0, alpha);
    imlib_image_fill_rectangle(0, 0, w, h);

    im_cpy = imlib_create_image(w, h);
    imlib_context_set_image(im_cpy);
    imlib_image_set_has_alpha(1);
    imlib_blend_image_onto_image(im, 1, 0, 0, w, h, 0, 0, w, h);
    imlib_image_copy_alpha_to_image(im_aph, 0, 0);

    // static int cnt = 0;
    // char s[100];
    // sprintf(s, "frame_%02d.png", cnt++);
    // imlib_save_image(s);
    return im_cpy;
}

void image_free(Imlib_Image im) {
    imlib_context_set_image(im);
    imlib_free_image();
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

void copy_pixmap(Pixmap pm_d, Pixmap pm_s) {
    static XGCValues gcvalues;
    static GC gc;

    D("copy pixmap %lu to %lu", pm_s, pm_d);
    gcvalues.fill_style = FillTiled;
    gcvalues.tile = pm_s;
    gc = XCreateGC(disp, pm_d, GCFillStyle | GCTile, &gcvalues);
    XFillRectangle(disp, pm_d, gc, 0, 0, scr->width, scr->height);
    XFreeGC(disp, gc);
}
