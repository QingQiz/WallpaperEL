#include <X11/Xatom.h>
#include <unistd.h>

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

void WERenderAllMonitorToPixmap(Pixmap pmap, int alpha) {
    static Imlib_Image im_m[MAX_MONITOR_N], im_else, im;
    static char img_loaded = 0;

    // load all images
    if (img_loaded == 0) {
        for (int i = 0; i < MAX_MONITOR_N; ++i) {
            if (opts.monitor[i] == NULL) {
                continue;
            } else {
                im_m[i] = imlib_load_image(opts.monitor[i]->file_name);
                assert(im_m[i], "Can not load %s", opts.monitor[i]->file_name);
            }
        }
        if (opts.else_monitor != NULL) {
            im_else = imlib_load_image(opts.else_monitor->file_name);
            assert(im_else, "Can not load %s", opts.else_monitor->file_name);
        }
        img_loaded = 1;
    }

    for (int j = 0; j < monitor_n; ++j) {
        if (opts.monitor[j] == NULL) {
            if (opts.else_monitor == NULL) {
                continue;
            } else {
                im = im_else;
            }
        } else {
            im = im_m[j];
        }
        image_set_alpha(im, alpha);
        bg_filled(pmap, im, monitor_l[j].x, monitor_l[j].y, monitor_l[j].width, monitor_l[j].height);
    }
}

void WERenderFIFO(Pixmap **pmap_l, int *pmap_n) {
    static Pixmap fifo[FIFO_SETP + 1], pmap;

    pmap = WEGetCurrentPixmapOrCreate();
    fifo[FIFO_SETP] = 0;

    // copy pixmap in use to fifo[0]
    fifo[0] = XCreatePixmap(disp, root, scr->width, scr->height, depth);
    copy_pixmap(fifo[0], pmap);

    // create pixmap for all fade in fade out steps
    for (int i = 1; i < FIFO_SETP; ++i) {
        fifo[i] = XCreatePixmap(disp, root, scr->width, scr->height, depth);
        WERenderAllMonitorToPixmap(fifo[0], 16);
        copy_pixmap(fifo[i], fifo[0]);
    }

    // free useless pixmap
    XFreePixmap(disp, fifo[0]);
    XFreePixmap(disp, pmap);

    *pmap_l = fifo + 1;
    *pmap_n = FIFO_SETP - 1;
}


void WESetWallpaperByOptions() {
    if (opts.fifo) {
        Pixmap *fifo_l;
        int fifo_n;
        WERenderFIFO(&fifo_l, &fifo_n);
        for (int i = 0; i < fifo_n; ++i) {
            if (i != 0) XFreePixmap(disp, fifo_l[i - 1]);
            WESetWallpaper(fifo_l[i]);
            usleep((int)(0.05f * 1000000));
        }
    } else {
        Pixmap pmap = WEGetCurrentPixmapOrCreate();
        WERenderAllMonitorToPixmap(pmap, 255);
        WESetWallpaper(pmap);
    }
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

    // clear old wallpaper
    XClearWindow(disp, root);
    // draw new wallpaper
    XFlush(disp);
}

