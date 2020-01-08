#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "wallpaper.h"
#include "mmonitor.h"
#include "debug.h"
#include "imtools.h"

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

