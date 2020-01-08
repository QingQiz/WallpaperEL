#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "wallpaper.h"
#include "mmonitor.h"
#include "debug.h"
#include "imtools.h"

static void bg_filled(Pixmap pmap, Imlib_Image im, int x, int y, int w, int h) {
    int img_w = image_get_width(im);
    int img_h = image_get_height(im);

    int cut_x = (((img_w * h) > (img_h * w)) ? 1 : 0);

    int render_w = (  cut_x ? ((img_h * w) / h) : img_w);
    int render_h = ( !cut_x ? ((img_w * h) / w) : img_h);

    int render_x = (  cut_x ? ((img_w - render_w) >> 1) : 0);
    int render_y = ( !cut_x ? ((img_h - render_h) >> 1) : 0);

    render_image_part_on_drawable_at_size(pmap, im,
        render_x, render_y,
        render_w, render_h,
        x, y, w, h,
        1, 1, 1);
}

void WESetWallpaper(Imlib_Image im) {
    Pixmap pmap = XCreatePixmap(disp, root, scr->width, scr->height, depth);

    // render image to pixmap
    D("Requiring Monitor list");
    WEMonitor *wms;
    int wmn;
    WEGetMonitorList(disp, root, &wms, &wmn);
    for (int i = 0; i < wmn; ++i) {
        D("Rendering image on monitor %d (%dx%d+%d+%d)",
                i, wms[i].width, wms[i].height, wms[i].x, wms[i].y);

        bg_filled(pmap, im, wms[i].x, wms[i].y, wms[i].width, wms[i].height);
    }
    WEFreeMonitorList(wms);

    // get property if has, or return None
    Atom prop_root = XInternAtom(disp, "_XROOTPMAP_ID", True);
    Atom prop_esetroot = XInternAtom(disp, "ESETROOT_PMAP_ID", True);

    // has property ?
    if (prop_root != None && prop_esetroot != None) {
        Atom type_root, type_esetroot;
        unsigned long length, after;
        int format;
        unsigned char *data_root = NULL, *data_esetroot = NULL;

        XGetWindowProperty(disp, root, prop_root, 0L, 1L, False, AnyPropertyType,
                    &type_root, &format, &length, &after, &data_root);
        XGetWindowProperty(disp, root, prop_esetroot, 0L, 1L, False, AnyPropertyType,
                    &type_esetroot, &format, &length, &after, &data_esetroot);

        // same pixmap ?
        if (type_root == XA_PIXMAP && type_esetroot == XA_PIXMAP) {
            if (*(Pixmap*)data_root == *(Pixmap*)data_esetroot) {
                // Free useless pixmap
                D("Free usless pixmap");
                XKillClient(disp, *((Pixmap *) data_root));
            }
        }
        if (data_root) XFree(data_root);
        if (data_esetroot) XFree(data_esetroot);
    }

    // get or create property
    prop_root = XInternAtom(disp, "_XROOTPMAP_ID", False);
    prop_esetroot = XInternAtom(disp, "ESETROOT_PMAP_ID", False);

    assert(prop_root != None && prop_esetroot != None, "creation of pixmap property failed.");

    // replace pixmap to pmap
    XChangeProperty(disp, root, prop_root, XA_PIXMAP, 32,
                    PropModeReplace, (unsigned char *) &pmap, 1);
    XChangeProperty(disp, root, prop_esetroot, XA_PIXMAP, 32,
                    PropModeReplace, (unsigned char *) &pmap, 1);
    XSetWindowBackgroundPixmap(disp, root, pmap);

    D("Draw");
    // clear old wallpaper
    XClearWindow(disp, root);
    // draw new wallpaper
    XFlush(disp);
    // do not free pmap after exit
    XSetCloseDownMode(disp, RetainPermanent);
}

