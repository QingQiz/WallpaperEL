#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>

#include "imtools.h"
#include "common.h""
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

static image_list *im_m_now[MAX_MONITOR_N + 1] = {(image_list*)NULL};

static int WEXErrorHandler(Display *d, XErrorEvent *e) {
    // Assume the error occurred only in the first call to
    // WECopyPixmap after WEGetCurrentWallpaperOrCreate was called
    if (e->error_code == BadPixmap || e->error_code == BadGC) {
        // FIXME using argc and argv to restart program will
        // cause XOpenDisply to fail
        DE("Bad Pixmap, try runing with --ignore-current.");
    }
    return 0;
}

void WEImtoolsInit() {
    // XSetErrorHandler(WEXErrorHandler);

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

void WEImtoolsDestruct() {
    if (oldclient) XKillClient(disp, oldclient);
    XCloseDisplay(disp);
    WEFreeMonitorList(monitor_l);
    disp = NULL, vis = NULL, scr = NULL, root = 0;
}

int WEGetImageWidth(Imlib_Image im) {
   imlib_context_set_image(im);
   return imlib_image_get_width();
}

int WEGetImageHeight(Imlib_Image im) {
   imlib_context_set_image(im);
   return imlib_image_get_height();
}

Imlib_Image WESetImageAlpha(Imlib_Image im, int alpha) {
    static Imlib_Image im_aph = 0, im_cpy = 0;
    if (im_aph) WEFreeImage(im_aph);
    if (im_cpy) WEFreeImage(im_cpy);

    imlib_context_set_image(im);
    int h = imlib_image_get_height();
    int w = imlib_image_get_width();

    im_aph = imlib_create_image(w, h);
    imlib_context_set_image(im_aph);
    imlib_image_clear();

    imlib_image_set_has_alpha(1);
    imlib_context_set_color(0, 0, 0, alpha);
    imlib_image_fill_rectangle(0, 0, w, h);

    im_cpy = imlib_create_image(w, h);
    imlib_context_set_image(im_cpy);
    imlib_image_clear();

    imlib_image_set_has_alpha(1);
    imlib_blend_image_onto_image(im, 1, 0, 0, w, h, 0, 0, w, h);
    imlib_image_copy_alpha_to_image(im_aph, 0, 0);

    return im_cpy;
}

void WEFreeImage(Imlib_Image im) {
    imlib_context_set_image(im);
    imlib_free_image();
}

void WERenderImagePartOnDrawableAtSize(
        Drawable d, Imlib_Image im,
        int sx, int sy, int sw, int sh,
        int dx, int dy, int dw, int dh,
        char dither, char blend, char alias)
{
    imlib_context_set_image(im);
    imlib_context_set_drawable(d);
    imlib_context_set_anti_alias(alias);
    imlib_context_set_dither(dither);
    imlib_context_set_blend(blend);
    imlib_context_set_angle(0);
    imlib_render_image_part_on_drawable_at_size(sx, sy, sw, sh, dx, dy, dw, dh);
}

void WECopyPixmap(Display* disp, Pixmap pm_d, Pixmap pm_s) {
    static XGCValues gcvalues;
    static GC gc;

    // D("copy pixmap %lu to %lu", pm_s, pm_d);
    gcvalues.fill_style = FillTiled;
    gcvalues.tile = pm_s;
    gc = XCreateGC(disp, pm_d, GCFillStyle | GCTile, &gcvalues);
    XFillRectangle(disp, pm_d, gc, 0, 0, scr->width, scr->height);
    XFreeGC(disp, gc);
}

void WEBgFilled(Pixmap pmap, Imlib_Image im, int x, int y, int w, int h) {
    int img_w = WEGetImageWidth(im);
    int img_h = WEGetImageHeight(im);

    int cut_x = (((img_w * h) > (img_h * w)) ? 1 : 0);

    int render_w = (  cut_x ? ((img_h * w) / h) : img_w);
    int render_h = ( !cut_x ? ((img_w * h) / w) : img_h);

    int render_x = (  cut_x ? ((img_w - render_w) >> 1) : 0);
    int render_y = ( !cut_x ? ((img_h - render_h) >> 1) : 0);

    WERenderImagePartOnDrawableAtSize(pmap, im,
        render_x, render_y,
        render_w, render_h,
        x, y, w, h,
        1, 1, 1);
}

void WELoadNextImage() {
    static image_list *im_m[MAX_MONITOR_N + 1] = {(image_list*)NULL};
    static char im_loaded = 0;

    // load images
    for (int i = 0; i < MAX_MONITOR_N + 1; ++i) {
        if (opts.monitor[i] == NULL) continue;

        if (im_m[i] == NULL) {
            im_m[i] = (image_list*)malloc(sizeof(image_list));
            im_m[i]->im = 0;
            im_m[i]->next = im_m[i];
            im_m[i]->prev = im_m[i];
        }

        // load all if dt < MIN_FIFO_ENABLE_TIME else load when necessary
        if (!im_loaded) {
            file_list *fhead = opts.monitor[i];
            file_list *fiter = fhead;

            image_list *phead = im_m[i];
            image_list *piter = phead;

            piter->im = NULL;
            fiter = fiter->next;

            while (fiter != fhead) {
                piter->next = (image_list*)malloc(sizeof(image_list));
                piter->next->prev = piter;
                piter = piter->next;
                piter->im = NULL;
                fiter = fiter->next;
            }
            piter->next = phead;
            piter->next->prev = piter;
            im_m[i] = phead;

            im_loaded = 1;
        }

        if (im_m[i]->im == NULL) {
            D("loading %s", opts.monitor[i]->file_name);
            im_m[i]->im = imlib_load_image(opts.monitor[i]->file_name);
            assert(im_m[i]->im, "Can not load %s", opts.monitor[i]->file_name);
        }

        if (opts.less_memory) {
            image_list *to_free = im_m[i]->prev;
            do {
                if (to_free->im == NULL) break;
                if (to_free == im_m[i]) break;
                D("Free Image %lu", (unsigned long)to_free->im);
                WEFreeImage(to_free->im);
                to_free->im = NULL;
            } while (0);
        }

        im_m_now[i] = im_m[i];
        im_m[i] = im_m[i]->next;
        opts.monitor[i] = opts.monitor[i]->next;
    }
}

void WERenderCurrentImageToPixmap(Pixmap pmap, int alpha) {
    assert(0 <= alpha && alpha <= 255, "Bad alpha %d", alpha);
    static Imlib_Image im;
    for (int i = 0; i < monitor_n; ++i) {
        if (opts.monitor[i] == NULL) {
            if (opts.monitor[MAX_MONITOR_N] == NULL) {
                continue;
            } else {
                im = im_m_now[MAX_MONITOR_N]->im;
            }
        } else {
            im = im_m_now[i]->im;
        }

        D("Rendering pixmap %lu with alpha %d", pmap, alpha);
        if (alpha != 255) {
            im = WESetImageAlpha(im, alpha);
        }
        WEBgFilled(pmap, im, monitor_l[i].x, monitor_l[i].y, monitor_l[i].width, monitor_l[i].height);
    }
}

