#include <X11/Xatom.h>

#include "wallpaper.h"
#include "mmonitor.h"
#include "debug.h"
#include "imtools.h"
#include "options.h"

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

Pixmap WEGetCurrentPixmapOrCreate() {
    // get property if has, or return None
    Atom prop_root = XInternAtom(disp, "_XROOTPMAP_ID", True);
    Atom prop_esetroot = XInternAtom(disp, "ESETROOT_PMAP_ID", True);

    // has property ?
    if (prop_root != None && prop_esetroot != None) {
        Atom type_root, type_esetroot;
        unsigned long length, after;
        int format;
        unsigned char *data_root = NULL, *data_esetroot = NULL;

        XGetWindowProperty(disp, root, prop_root, 0L, 1L, False, XA_PIXMAP,
                    &type_root, &format, &length, &after, &data_root);
        XGetWindowProperty(disp, root, prop_esetroot, 0L, 1L, False, XA_PIXMAP,
                    &type_esetroot, &format, &length, &after, &data_esetroot);

        // same pixmap ?
        if (data_root && *(Pixmap*)data_root == *(Pixmap*)data_esetroot) {
            Pixmap pmap = *(Pixmap*)data_root;

            if (data_root) XFree(data_root);
            if (data_esetroot) XFree(data_esetroot);

            return pmap;
        }
        if (data_root) XFree(data_root);
        if (data_esetroot) XFree(data_esetroot);
    }
    return XCreatePixmap(disp, root, scr->width, scr->height, depth);
}


void WESetWallpaperByOptions() {
    WEMonitor *wms;
    int wmn;
    Pixmap pmap = WEGetCurrentPixmapOrCreate();

    // render image to pixmap
    D("Requiring Monitor list");
    WEGetMonitorList(disp, root, &wms, &wmn);

    Imlib_Image im, im_else;

    if (opts.else_monitor != NULL) {
        im_else = imlib_load_image(opts.else_monitor);
        assert(im_else, "Can not load %s", opts.else_monitor);
    }

    for (int i = 0; i < wmn; ++i) {
        if (opts.monitor[i] == NULL) {
            if (opts.else_monitor == NULL) {
                continue;
            } else {
                D("Rendering image %s on monitor %d (%dx%d+%d+%d)", opts.else_monitor, i, wms[i].width, wms[i].height, wms[i].x, wms[i].y);
                im = im_else;
            }
        } else {
            im = imlib_load_image(opts.monitor[i]);
            assert(im, "Can not load %s", opts.monitor[i]);
            D("Rendering image %s on monitor %d (%dx%d+%d+%d)", opts.monitor[i], i, wms[i].width, wms[i].height, wms[i].x, wms[i].y);
        }

        bg_filled(pmap, im, wms[i].x, wms[i].y, wms[i].width, wms[i].height);
    }
    WESetWallpaper(pmap);

    WEFreeMonitorList(wms);
}


void WESetWallpaper(Pixmap pmap) {
    Atom prop_root = XInternAtom(disp, "_XROOTPMAP_ID", False);
    Atom prop_esetroot = XInternAtom(disp, "ESETROOT_PMAP_ID", False);

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

