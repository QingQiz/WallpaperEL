#include <X11/Xatom.h>
#include <unistd.h>
#include <signal.h>

#include "wallpaper.h"
#include "mmonitor.h"
#include "debug.h"
#include "imtools.h"
#include "options.h"

static image_list *im_m_now[MAX_MONITOR_N + 1] = {(image_list*)NULL};
static Pixmap current_pixmap = 0;

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

void WESetWallpaper(Display *display, Pixmap pmap) {
    Atom prop_root = XInternAtom(display, "_XROOTPMAP_ID", False);
    Atom prop_esetroot = XInternAtom(display, "ESETROOT_PMAP_ID", False);

    assert(prop_root != None && prop_esetroot != None, "creation of pixmap property failed.");

    // replace pixmap to pmap
    XChangeProperty(display, root, prop_root, XA_PIXMAP, 32,
                    PropModeReplace, (unsigned char *) &pmap, 1);
    XChangeProperty(display, root, prop_esetroot, XA_PIXMAP, 32,
                    PropModeReplace, (unsigned char *) &pmap, 1);
    XSetWindowBackgroundPixmap(display, root, pmap);

    // clear old wallpaper
    XClearWindow(display, root);
    // draw new wallpaper
    D("Draw %lu", pmap);
    XFlush(display);
    current_pixmap = pmap;
}

Pixmap WEGetCurrentPixmapOrCreate() {
    // get property if has, or return None
    Atom prop_root = XInternAtom(disp, "_XROOTPMAP_ID", True);
    Atom prop_esetroot = XInternAtom(disp, "ESETROOT_PMAP_ID", True);
    Pixmap pmap = 0;

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
            pmap = *(Pixmap*)data_root;
            oldclient = pmap;
        }
        if (data_root) XFree(data_root);
        if (data_esetroot) XFree(data_esetroot);
    }
    if (!pmap) {
        return XCreatePixmap(disp, root, scr->width, scr->height, depth);
    }

    int cnt = 0, flg = 1;
    if (opts.monitor[MAX_MONITOR_N] != NULL) flg = 0;
    for (int i = 0; i < MAX_MONITOR_N; ++i) if (opts.monitor[i] != NULL) cnt++;
    if (cnt == monitor_n) flg = 0;

    // fifo or not set all monitor, use old pixmap
    if (flg || opts.fifo) {
        oldclient = 0;
        return pmap;
    }
    return XCreatePixmap(disp, root, scr->width, scr->height, depth);
}

void WEGetNextImageList() {
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

        // load all if dt < MIN_FIFO_ENABLE_TIME else load when necessary
        if (!im_loaded) {
            file_list *fhead = opts.monitor[i];
            file_list *fiter = fhead;

            image_list *phead = im_m[i];
            image_list *piter = phead;

            if (opts.dt < MIN_FIFO_ENABLE_TIME) {
                D("loading %s", fiter->file_name);
                piter->im = imlib_load_image(fiter->file_name);
                assert(piter->im, "Can not load %s", opts.monitor[i]->file_name);
            } else {
                piter->im = NULL;
            }
            fiter = fiter->next;

            while (fiter != fhead) {
                piter->next = (image_list*)malloc(sizeof(image_list));
                piter = piter->next;
                if (opts.dt < MIN_FIFO_ENABLE_TIME) {
                    D("loading %s", fiter->file_name);
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
            D("loading %s", opts.monitor[i]->file_name);
            im_m[i]->im = imlib_load_image(opts.monitor[i]->file_name);
            assert(im_m[i]->im, "Can not load %s", opts.monitor[i]->file_name);
        }

        im_m_now[i] = im_m[i];
        im_m[i] = im_m[i]->next;
        opts.monitor[i] = opts.monitor[i]->next;
    }
}

void WERenderImageListToPixmap(Pixmap pmap, int alpha) {
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

        D("Rendering pixmap %lu with alpha %d", pmap, alpha);
        if (alpha != 255) {
            im = image_set_alpha(im, alpha);

            bg_filled(pmap, im, monitor_l[i].x, monitor_l[i].y, monitor_l[i].width, monitor_l[i].height);
        } else {
            bg_filled(pmap, im, monitor_l[i].x, monitor_l[i].y, monitor_l[i].width, monitor_l[i].height);
        }
    }
}

Pixmap WEGetNextPixmap(Pixmap origin) {
    static pixmap_list *pmap_l = NULL;
    static char is_list_build = 0;

    if (!is_list_build) {
        is_list_build = 1;

        file_list *fiter[MAX_MONITOR_N + 1];

        for (int i = 0; i < MAX_MONITOR_N + 1; ++i) fiter[i] = opts.monitor[i];

        pmap_l = (pixmap_list*)malloc(sizeof(pixmap_list));
        pmap_l->pmap = 0;
        pixmap_list *iter= pmap_l;

        int cnt = (opts.fifo && opts.dt >= MIN_FIFO_ENABLE_TIME) ? FIFO_SETP - 1 : 0;
        while (cnt--) {
            iter->next = (pixmap_list*)malloc(sizeof(pixmap_list));
            iter = iter->next;
            iter->pmap = 0;
        }

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

            int cnt = (opts.fifo && opts.dt >= MIN_FIFO_ENABLE_TIME) ? FIFO_SETP : 1;
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
    if (opts.dt < MIN_FIFO_ENABLE_TIME) {
        pmap_l->pmap = XCreatePixmap(disp, root, scr->width, scr->width, depth);
        WEGetNextImageList();
        copy_pixmap(disp, pmap_l->pmap, origin);
        WERenderImageListToPixmap(pmap_l->pmap, 255);

        pmap_l = pmap_l->next;

        while (pmap_l != head) {
            pmap_l->pmap = XCreatePixmap(disp, root, scr->width, scr->width, depth);
            WEGetNextImageList();

            copy_pixmap(disp, pmap_l->pmap, origin);
            WERenderImageListToPixmap(pmap_l->pmap, 255);

            pmap_l = pmap_l->next;
        }
        pmap_l = pmap_l->next;
        return head->pmap;
    } else {
        int cnt = opts.fifo ? FIFO_SETP : 1;
        WEGetNextImageList();

        while (cnt--) {
            pmap_l->pmap = XCreatePixmap(disp, root, scr->width, scr->width, depth);
            copy_pixmap(disp, pmap_l->pmap, origin);
            if (cnt == 0) {
                WERenderImageListToPixmap(pmap_l->pmap, 255);
            } else {
                WERenderImageListToPixmap(pmap_l->pmap, (255 - 255 / FIFO_SETP * cnt));
            }
            pmap_l = pmap_l->next;
        }
        pmap_l = head->next;
        return head->pmap;
    }
}

void WEExit() {
    if (current_pixmap == 0) return;

    // open new display
    Display *disp2 = XOpenDisplay(NULL);
    assert(disp2, "Can not reopen display");
    Window root2 = RootWindow(disp2, DefaultScreen(disp2));

    // copy current pixmap to new display
    Pixmap pmap_final = XCreatePixmap(disp2, root2, scr->width, scr->height, depth);

    copy_pixmap(disp2, pmap_final, current_pixmap);

    XSync(disp, False);
    XSync(disp2, False);
    XFreePixmap(disp, current_pixmap);

    // set new pixmap as wallpaper
    WESetWallpaper(disp2, pmap_final);
    XSetCloseDownMode(disp2, RetainPermanent);

    XCloseDisplay(disp2);
}

void WESigintHandler(int signo) {
    if (signo == SIGINT) {
        DW("SIGINT recived, exit.");
        WEExit();
        destruct_imtools();
        exit(-1);
    }
}


void WESetWallpaperByOptions() {
    if (signal(SIGINT, WESigintHandler) == SIG_ERR) {
        DW("Can not catch SIGINT");
    }
    Pixmap origin = WEGetCurrentPixmapOrCreate();
    Pixmap head = WEGetNextPixmap(origin);
    Pixmap iter = head, org = origin;

    while (1) {
        int cnt = (opts.fifo && opts.dt >= MIN_FIFO_ENABLE_TIME) ? FIFO_SETP : 1;

        while (cnt--) {
            WESetWallpaper(disp, iter);
            org = iter;
            iter = WEGetNextPixmap(org);
            if (cnt) usleep((int)(0.61f / FIFO_SETP * 1000000));
            // usleep((int)(1 * 1000000));
        }

        if (iter == head && !opts.loop) break;
        usleep((int)(opts.dt * 1000000));
    }

    do {
        if (iter != org) XFreePixmap(disp, iter);
        iter = WEGetNextPixmap(origin);
    } while (iter != head);
    XFreePixmap(disp, origin);

    WEExit();
}

