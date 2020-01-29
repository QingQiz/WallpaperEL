#include <X11/Xatom.h>
#include <unistd.h>
#include <signal.h>

#include "wallpaper.h"
#include "mmonitor.h"
#include "debug.h"
#include "imtools.h"
#include "options.h"

static Pixmap current_pixmap = 0;

static void WESetWallpaper(Display *display, Pixmap pmap) {
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

static Pixmap WEGetCurrentWallpaperOrCreate() {
    // get property if has, or return None
    Atom prop_root = XInternAtom(disp, "_XROOTPMAP_ID", True);
    Atom prop_esetroot = XInternAtom(disp, "ESETROOT_PMAP_ID", True);
    Pixmap pmap = 0;
    Pixmap pmap_ret = XCreatePixmap(disp, root, scr->width, scr->height, depth);

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
    if (!pmap || opts.ignore_current) {
        return pmap_ret;
    }

    // flg: is NOT all monitors are set
    int cnt = 0, flg = 1;
    if (opts.monitor[MAX_MONITOR_N] != NULL) flg = 0;
    for (int i = 0; i < MAX_MONITOR_N; ++i) if (opts.monitor[i] != NULL) cnt++;
    if (cnt == monitor_n) flg = 0;

    // fifo or not set all monitor, use old pixmap
    if (flg || opts.fifo) {
        WECopyPixmap(disp, pmap_ret, pmap);
    }
    return pmap_ret;
}

static Pixmap WEGetNextWallpaper(Pixmap origin) {
    static pixmap_list *pmap_l = NULL;
    static pixmap_list *pmap_l_head = NULL;
    static char is_list_build = 0;

    if (!is_list_build) {
        // build a linked list
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
        pmap_l_head = pmap_l;
    }

    // return if the wallpaper has been rendered
    if (pmap_l->pmap) {
        Pixmap retval = pmap_l->pmap;
        pmap_l = pmap_l->next;
        return retval;
    }

    pixmap_list *head = pmap_l;
    if (opts.dt < MIN_FIFO_ENABLE_TIME) { // render immediately
        // render out all possible wallpapers
        // ignore fifo
        do {
            pmap_l->pmap = XCreatePixmap(disp, root, scr->width, scr->width, depth);

            WELoadNextImage();
            WECopyPixmap(disp, pmap_l->pmap, origin);
            WERenderCurrentImageToPixmap(pmap_l->pmap, 255);

            pmap_l = pmap_l->next;
        } while (pmap_l != head);

        pmap_l = pmap_l->next;
        return head->pmap;
    } else { // lazy rendering
        int cnt = opts.fifo ? FIFO_SETP : 1;
        static char is_first_cycle = 1;

        WELoadNextImage();

        while (cnt--) {
            pmap_l->pmap = XCreatePixmap(disp, root, scr->width, scr->width, depth);
            WECopyPixmap(disp, pmap_l->pmap, origin);
            if (cnt == 0) {
                WERenderCurrentImageToPixmap(pmap_l->pmap, 255);
            } else {
                WERenderCurrentImageToPixmap(pmap_l->pmap, (int)(255. - 255. / FIFO_SETP * cnt));
            }
            pmap_l = pmap_l->next;
        }
        if (is_first_cycle && pmap_l == pmap_l_head && opts.fifo) {
            // the next is the second cycle
            // the next pixmap is the first pixmap, free first pixmap and re-render
            cnt = FIFO_SETP;
            pixmap_list *iter = pmap_l;

            while (cnt--) {
                D("Free pixmap %lu", iter->pmap);
                XFreePixmap(disp, iter->pmap);
                iter->pmap = 0;
                iter = iter->next;
            }
            is_first_cycle = 0;
        }
        pmap_l = head->next;
        return head->pmap;
    }
}

static void WEExit() {
    if (current_pixmap == 0) return;

    // open new display
    Display *disp2 = XOpenDisplay(NULL);
    assert(disp2, "Can not reopen display");
    Window root2 = RootWindow(disp2, DefaultScreen(disp2));

    // copy current pixmap to new display
    Pixmap pmap_final = XCreatePixmap(disp2, root2, scr->width, scr->height, depth);

    WECopyPixmap(disp2, pmap_final, current_pixmap);

    XSync(disp, False);
    XSync(disp2, False);
    D("Free pixmap %lu", current_pixmap);
    XFreePixmap(disp, current_pixmap);

    // set new pixmap as wallpaper
    WESetWallpaper(disp2, pmap_final);
    XSetCloseDownMode(disp2, RetainPermanent);

    XCloseDisplay(disp2);
}

static void WESigintHandler(int signo) {
    if (signo == SIGINT) {
        puts("");
        DW("SIGINT recived, exit.");
        WEExit();
        WEImtoolsDestruct();
        exit(-1);
    }
}

void WESetWallpaperByOptions() {
    if (signal(SIGINT, WESigintHandler) == SIG_ERR) {
        DW("Can not catch SIGINT");
    }
    Pixmap origin = WEGetCurrentWallpaperOrCreate();
    D("current: %lu", origin);
    Pixmap head = WEGetNextWallpaper(origin);
    Pixmap iter = head, org = origin;

    while (1) {
        int cnt = (opts.fifo && opts.dt >= MIN_FIFO_ENABLE_TIME) ? FIFO_SETP : 1;

        while (cnt--) {
            WESetWallpaper(disp, iter);
            org = iter;
            iter = WEGetNextWallpaper(org);
            if (cnt) usleep((int)(0.61f / FIFO_SETP * 1000000));
        }

        if (iter == head && !opts.loop) break;
        usleep((int)(opts.dt * 1000000));
    }

    do {
        if (iter != org) {
            D("Free pixmap %lu", iter);
            XFreePixmap(disp, iter);
        }
        iter = WEGetNextWallpaper(origin);
    } while (iter != head);
    D("Free pixmap %lu", origin);
    XFreePixmap(disp, origin);

    WEExit();
}

