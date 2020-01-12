#include <X11/Xatom.h>
#include <unistd.h>

#include "wallpaper.h"
#include "mmonitor.h"
#include "debug.h"
#include "imtools.h"
#include "options.h"

static image_list *im_m_now[MAX_MONITOR_N + 1] = {(image_list*)NULL};

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
            // useless pixmap
            int cnt = 0;
            if (opts.monitor[MAX_MONITOR_N] != NULL) {
                oldclient = *(Pixmap*)data_root;
                goto _killed;
            }
            for (int i = 0; i < monitor_n; ++i) {
                if (opts.monitor[i]) cnt++;
            }
            if (cnt == monitor_n) {
                oldclient = *(Pixmap*)data_root;
                goto _killed;
            }

            Pixmap pmap = *(Pixmap*)data_root;

            if (data_root) XFree(data_root);
            if (data_esetroot) XFree(data_esetroot);

            return pmap;
        }
_killed:
        if (data_root) XFree(data_root);
        if (data_esetroot) XFree(data_esetroot);
    }
    return XCreatePixmap(disp, root, scr->width, scr->height, depth);
}

void WELoadImageByStep() {
    static image_list *im_m[MAX_MONITOR_N + 1] = {(image_list*)NULL};
    static char im_loaded = 0;

    // load images
    for (int i = 0; i < MAX_MONITOR_N + 1; ++i) {
        if (opts.monitor[i] == NULL) continue;

        if (im_m[i] == NULL) {
            im_m[i] = (image_list*)malloc(sizeof(image_list));
            im_m[i]->im = 0;
            im_m[i]->next = NULL;
        }

        // load all if dt < 1.5 else load when necessary
        if (!im_loaded) {
            file_list *fhead = opts.monitor[i];
            file_list *fiter = fhead;

            image_list *phead = im_m[i];
            image_list *piter = phead;

            if (opts.dt < 1.5) {
                piter->im = imlib_load_image(fiter->file_name);
                assert(piter->im, "Can not load %s", opts.monitor[i]->file_name);
            } else {
                piter->im = NULL;
            }
            fiter = fiter->next;

            while (fiter != fhead) {
                piter->next = (image_list*)malloc(sizeof(image_list));
                piter = piter->next;
                if (opts.dt < 1.5) {
                    piter->im = imlib_load_image(fiter->file_name);
                    assert(piter->im, "Can not load %s", opts.monitor[i]->file_name);
                } else {
                    piter->im = NULL;
                }
                fiter = fiter->next;
            }
            piter->next = phead;
            im_m[i] = phead;

            im_loaded = 1;
        }

        if (im_m[i]->im == NULL) {
            im_m[i]->im = imlib_load_image(opts.monitor[i]->file_name);
            assert(im_m[i]->im, "Can not load %s", opts.monitor[i]->file_name);
        }

        im_m_now[i] = im_m[i];
        im_m[i] = im_m[i]->next;
        opts.monitor[i] = opts.monitor[i]->next;
    }
}

void WERenderASetp(Pixmap pmap, int alpha) {
    Imlib_Image im;
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

        if (alpha != 255) image_set_alpha(im, alpha);

        bg_filled(pmap, im, monitor_l[i].x, monitor_l[i].y, monitor_l[i].width, monitor_l[i].height);
    }
}

Pixmap WERenderImageToPixmap(Pixmap origin) {
    static pixmap_list *pmap_l = NULL;
    static char is_list_build = 0;

    if (!is_list_build) {
        is_list_build = 1;

        file_list *fiter[MAX_MONITOR_N + 1];

        for (int i = 0; i < MAX_MONITOR_N + 1; ++i) fiter[i] = opts.monitor[i];

        pmap_l = (pixmap_list*)malloc(sizeof(pixmap_list));
        pmap_l->pmap = 0;
        pixmap_list *iter= pmap_l;

        while (1) {
            for (int i = 0; i < MAX_MONITOR_N + 1; ++i) {
                if (fiter[i]) fiter[i] = fiter[i]->next;
            }

            int is_end = 1;
            for (int i = 0; i < MAX_MONITOR_N + 1; ++i) {
                if (opts.monitor[i] != fiter[i]) {
                    is_end = 0;
                    break;
                }
            }
            if (is_end) break;

            int cnt = (opts.fifo && opts.dt >= 1.5) ? FIFO_SETP : 1;
            while (cnt--) {
                iter->next = (pixmap_list*)malloc(sizeof(pixmap_list));
                iter = iter->next;
                iter->pmap = 0;
            }
        }
        iter->next = pmap_l;
    }

    if (pmap_l->pmap) {
        Pixmap retval = pmap_l->pmap;
        pmap_l = pmap_l->next;
        return retval;
    }

    pixmap_list *head = pmap_l;
    if (opts.dt < 1.5) {
        pmap_l->pmap = XCreatePixmap(disp, root, scr->width, scr->width, depth);
        WELoadImageByStep();
        copy_pixmap(pmap_l->pmap, origin);
        WERenderASetp(pmap_l->pmap, 255);

        pmap_l = pmap_l->next;

        while (pmap_l != head) {
            pmap_l->pmap = XCreatePixmap(disp, root, scr->width, scr->width, depth);
            WELoadImageByStep();

            copy_pixmap(pmap_l->pmap, origin);
            WERenderASetp(pmap_l->pmap, 255);

            pmap_l = pmap_l->next;
        }
        pmap_l = pmap_l->next;
        return head->pmap;
    } else {
        int cnt = opts.fifo ? FIFO_SETP : 1;
        WELoadImageByStep();

        while (cnt--) {
            pmap_l->pmap = XCreatePixmap(disp, root, scr->width, scr->width, depth);
            if (cnt == 0) {
                WERenderASetp(origin, 255);
            } else {
                WERenderASetp(origin, 32);
            }
            copy_pixmap(pmap_l->pmap, origin);
            pmap_l = pmap_l->next;
        }
        return head->pmap;
    }
}


void WESetWallpaperByOptions() {
    Pixmap origin = WEGetCurrentPixmapOrCreate();
    Pixmap head = WERenderImageToPixmap(origin);
    Pixmap iter = head, org = origin;

    while (1) {
        int cnt = (opts.fifo && opts.fifo >= 1.5) ? FIFO_SETP : 1;

        while (cnt--) {
            WESetWallpaper(iter);
            usleep((int)(0.05f * 1000000));
        }

        org = iter;
        iter = WERenderImageToPixmap(org);

        if (iter == head) break;
        usleep((int)(opts.dt * 1000000));
    }

    while (iter != head) {
        if (iter != org) XFreePixmap(disp, iter);
        iter = WERenderImageToPixmap(origin);
    }
    XFreePixmap(disp, origin);
}


