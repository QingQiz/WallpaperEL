#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "wallpaper.h"
#include "mmonitor.h"
#include "debug.h"

Display *disp = NULL;
Visual *vis = NULL;
Screen *scr = NULL;
Colormap cm;
int depth;
Atom wmDeleteWindow;
// XContext xid_context = 0;
Window root = 0;

void init_x() {
    disp = XOpenDisplay(NULL);
    assert(disp, "Can't open X display");

    vis = DefaultVisual(disp, DefaultScreen(disp));
    depth = DefaultDepth(disp, DefaultScreen(disp));
    cm = DefaultColormap(disp, DefaultScreen(disp));
    root = RootWindow(disp, DefaultScreen(disp));
    scr = ScreenOfDisplay(disp, DefaultScreen(disp));

    imlib_context_set_display(disp);
    imlib_context_set_visual(vis);
    imlib_context_set_colormap(cm);
    imlib_context_set_color_modifier(NULL);
    imlib_context_set_progress_function(NULL);
    imlib_context_set_operation(IMLIB_OP_COPY);
    wmDeleteWindow = XInternAtom(disp, "WM_DELETE_WINDOW", False);

    imlib_set_cache_size(4 * 1024 * 1024);

    return;
}
int image_get_width(Imlib_Image im)
{
   imlib_context_set_image(im);
   return imlib_image_get_width();
}

int image_get_height(Imlib_Image im)
{
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


static void bg_filled(Pixmap pmap, Imlib_Image im, int x, int y, int w, int h) {
    int img_w, img_h, cut_x;
    int render_w, render_h, render_x, render_y;

    img_w = image_get_width(im);
    img_h = image_get_height(im);

    cut_x = (((img_w * h) > (img_h * w)) ? 1 : 0);

    render_w = (  cut_x ? ((img_h * w) / h) : img_w);
    render_h = ( !cut_x ? ((img_w * h) / w) : img_h);

    render_x = (  cut_x ? ((img_w - render_w) >> 1) : 0);
    render_y = ( !cut_x ? ((img_h - render_h) >> 1) : 0);

    render_image_part_on_drawable_at_size(pmap, im,
        render_x, render_y,
        render_w, render_h,
        x, y, w, h,
        1, 1, 1);

    return;
}

void WESetWallpaper(Imlib_Image im) {
    init_x();

    XGCValues gcvalues;
    GC gc;

    Atom prop_root, prop_esetroot, type;
    int format, wmn;
    WEMonitor *wms;
    unsigned long length, after;
    unsigned char *data_root = NULL, *data_esetroot = NULL;
    Pixmap pmap_d1, pmap_d2;

    /* local display to set closedownmode on */
    Display *disp2;
    Window root2;
    int depth2;

    D("Use XSetRootWindowPixmap");

    XColor color;
    Colormap cmap = DefaultColormap(disp, DefaultScreen(disp));
    XAllocNamedColor(disp, cmap, "black", &color, &color);

    pmap_d1 = XCreatePixmap(disp, root, scr->width, scr->height, depth);

    WEGetMonitorList(disp, root, &wms, &wmn);

    D("Monitors:");
    for (int i = 0; i < wmn; ++i) {
        bg_filled(pmap_d1, im,
                wms[i].x, wms[i].y,
                wms[i].width, wms[i].height);
        D("\t%d\t%d\t%d\t%d", wms[i].x, wms[i].y, wms[i].width, wms[i].height);
    }
    WEFreeMonitorList(wms);


    disp2 = XOpenDisplay(NULL);
    assert(disp2, "Can't reopen X display.");

    root2 = RootWindow(disp2, DefaultScreen(disp2));
    depth2 = DefaultDepth(disp2, DefaultScreen(disp2));
    XSync(disp, False);
    pmap_d2 = XCreatePixmap(disp2, root2, scr->width, scr->height, depth2);
    gcvalues.fill_style = FillTiled;
    gcvalues.tile = pmap_d1;
    gc = XCreateGC(disp2, pmap_d2, GCFillStyle | GCTile, &gcvalues);
    XFillRectangle(disp2, pmap_d2, gc, 0, 0, scr->width, scr->height);
    XFreeGC(disp2, gc);
    XSync(disp2, False);
    XSync(disp, False);
    XFreePixmap(disp, pmap_d1);

    prop_root = XInternAtom(disp2, "_XROOTPMAP_ID", True);
    prop_esetroot = XInternAtom(disp2, "ESETROOT_PMAP_ID", True);

    if (prop_root != None && prop_esetroot != None) {
        XGetWindowProperty(disp2, root2, prop_root, 0L, 1L, False, AnyPropertyType, &type, &format, &length, &after, &data_root);
        if (type == XA_PIXMAP) {
            XGetWindowProperty(disp2, root2,
                       prop_esetroot, 0L, 1L,
                       False, AnyPropertyType,
                       &type, &format, &length, &after, &data_esetroot);
            if (data_root && data_esetroot) {
                if (type == XA_PIXMAP && *((Pixmap *) data_root) == *((Pixmap *) data_esetroot)) {
                    XKillClient(disp2, *((Pixmap *) data_root));
                }
            }
        }
    }

    if (data_root) XFree(data_root);
    if (data_esetroot) XFree(data_esetroot);

    /* This will locate the property, creating it if it doesn't exist */
    prop_root = XInternAtom(disp2, "_XROOTPMAP_ID", False);
    prop_esetroot = XInternAtom(disp2, "ESETROOT_PMAP_ID", False);

    assert(prop_root != None && prop_esetroot != None, "creation of pixmap property failed.");

    XChangeProperty(disp2, root2, prop_root, XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pmap_d2, 1);
    XChangeProperty(disp2, root2, prop_esetroot, XA_PIXMAP, 32,
            PropModeReplace, (unsigned char *) &pmap_d2, 1);

    XSetWindowBackgroundPixmap(disp2, root2, pmap_d2);
    XClearWindow(disp2, root2);
    XFlush(disp2);
    XSetCloseDownMode(disp2, RetainPermanent);
    XCloseDisplay(disp2);
}

