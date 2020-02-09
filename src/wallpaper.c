#include <X11/Xatom.h>
#include <unistd.h>
#include <signal.h>

#include "wallpaper.h"
#include "mmonitor.h"
#include "imtools.h"
#include "options.h"
#include "atools.h"
#include "debug.h"

static Pixmap current_pixmap = 0;
static pthread_t bgm_pthread;

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

static pixmap_list* WEBuildWallpaperList() {
    file_list *fiter[MAX_MONITOR_N + 1];

    for (int i = 0; i < MAX_MONITOR_N + 1; ++i) fiter[i] = opts.monitor[i];

    pixmap_list *pmap_l = (pixmap_list*)malloc(sizeof(pixmap_list));
    pmap_l->pmap = 0;
    pixmap_list *iter= pmap_l;

    int cnt = (opts.fifo && opts.dt >= MIN_FIFO_ENABLE_TIME) ? FIFO_SETP - 1 : 0;
    while (cnt--) {
        iter->next = (pixmap_list*)malloc(sizeof(pixmap_list));
        iter->next->prev = iter;
        iter->next->pmap = 0;
        iter = iter->next;
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
            iter->next->prev = iter;
            iter->next->pmap = 0;
            iter = iter->next;
        }
    }
    iter->next = pmap_l;
    iter->next->prev = iter;
    return pmap_l;
}

static void WERenderAllWallpaper(pixmap_list *pmap_l) {
    pixmap_list *head = pmap_l;
    do {
        pmap_l->pmap = XCreatePixmap(disp, root, scr->width, scr->width, depth);

        WELoadNextImage();
        WERenderCurrentImageToPixmap(pmap_l->pmap, 255);

        pmap_l = pmap_l->next;
    } while (pmap_l != head);
}

static pixmap_list* WERenderAStep(pixmap_list *pmap_l, Pixmap origin) {
    WELoadNextImage();
    int cnt = opts.fifo ? FIFO_SETP : 1;
    while (cnt--) {
        if (pmap_l->pmap == 0) {
            // render only if not rendered
            pmap_l->pmap = XCreatePixmap(disp, root, scr->width, scr->width, depth);
            WECopyPixmap(disp, pmap_l->pmap, origin);
            if (cnt == 0) {
                WERenderCurrentImageToPixmap(pmap_l->pmap, 255);
            } else {
                WERenderCurrentImageToPixmap(pmap_l->pmap, (int)(255. - 255. / FIFO_SETP * cnt));
            }
        }
        pmap_l = pmap_l->next;
    }
    return pmap_l;
}

static Pixmap WEGetNextWallpaper(Pixmap origin, pixmap_list **current_pointer) {
    static pixmap_list *pmap_l = NULL;
    static pixmap_list *pmap_l_head = NULL;
    static char is_list_build = 0;
    static char more_than_one_wallpaper = 1;

    if (!is_list_build) {
        is_list_build = 1;
        // build a doubly linked list
        pmap_l = WEBuildWallpaperList();
        pmap_l_head = pmap_l;

        // render immediately
        if (opts.dt < MIN_FIFO_ENABLE_TIME) {
            // render out all possible wallpapers
            // ignore fifo
            WERenderAllWallpaper(pmap_l);
        }

        pixmap_list *iter = pmap_l;
        for (int i = 0; i < (opts.fifo ? FIFO_SETP : 1); ++i) {
            iter = iter->next;
        }
        if (iter == pmap_l) more_than_one_wallpaper = 0;
    }

    pixmap_list *head = pmap_l;
    if (pmap_l->pmap == 0) { // lazy rendering
        pixmap_list *next_image_first_pixmap = WERenderAStep(pmap_l, origin);

        static char is_first_cycle = 1;
        if (is_first_cycle && next_image_first_pixmap == pmap_l_head && opts.loop) {
            // the next cycle is the second cycle
            // the next pixmap is the first pixmap
            pixmap_list *iter = next_image_first_pixmap;

            if (head == pmap_l_head) {
                // current pixmap is the first pixmap
                // this means there is only one possible wallpaper
                DI("only one wallpaper, --loop was disabled.");
                opts.loop = 0;
            } else {
                // free first pixmap and mark for re-rendering
                int cnt = opts.fifo ? FIFO_SETP : 0;
                while (cnt--) {
                    do {
                        if (iter->pmap == current_pixmap) break;
                        if (iter->pmap == 0) break;

                        D("Free pixmap %lu", iter->pmap);
                        XFreePixmap(disp, iter->pmap);
                        iter->pmap = 0;
                    } while (0);
                    iter = iter->next;
                }
            }
            is_first_cycle = 0;
        }
    }
    if (opts.less_memory && more_than_one_wallpaper) {
        pixmap_list *to_free = pmap_l->prev->prev;
        do {
            if (opts.dt < MIN_FIFO_ENABLE_TIME) break;
            if (to_free->pmap == current_pixmap) break;
            if (to_free == pmap_l) break;
            if (to_free->pmap == 0) break;

            D("Free pixmap %lu", to_free->pmap);
            XFreePixmap(disp, to_free->pmap);
            to_free->pmap = 0;
        } while (0);
    }
    if (current_pointer != NULL) *current_pointer = pmap_l;
    pmap_l = pmap_l->next;
    return pmap_l->prev->pmap;
}

static void WEExit() {
    if (current_pixmap == 0) return;
    Pixmap to_free = current_pixmap;

    // open new display
    Display *disp2 = XOpenDisplay(NULL);
    assert(disp2, "Can not reopen display");
    Window root2 = RootWindow(disp2, DefaultScreen(disp2));

    // copy current pixmap to new display
    Pixmap pmap_final = XCreatePixmap(disp2, root2, scr->width, scr->height, depth);

    WECopyPixmap(disp2, pmap_final, current_pixmap);

    XSync(disp, False);
    XSync(disp2, False);

    // set new pixmap as wallpaper
    WESetWallpaper(disp2, pmap_final);
    XSetCloseDownMode(disp2, RetainPermanent);

    D("Free pixmap %lu", to_free);
    XFreePixmap(disp, to_free);

    XCloseDisplay(disp2);

    if (opts.bgm != NULL){
        // pthread_kill(bgm_pthread, SIGINT);
        WEAtoolsDestruct();
    }
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
    pixmap_list *pmap_head = NULL, *current_pmap_pointer = NULL;
    Pixmap iter = 0;

    while (1) {
        int cnt = (opts.fifo && opts.dt >= MIN_FIFO_ENABLE_TIME) ? FIFO_SETP : 1;

        while (cnt--) {
            if (pmap_head == NULL) {
                iter = WEGetNextWallpaper(origin, &current_pmap_pointer);
                pmap_head = current_pmap_pointer;
                if (opts.bgm != NULL) {
                    WEAtoolsInit(opts.bgm);
                    pthread_create(&bgm_pthread, NULL, (void*)WEAtoolsPlay, NULL);
                }
            } else {
                iter = WEGetNextWallpaper(current_pixmap, &current_pmap_pointer);
            }

            WESetWallpaper(disp, iter);
            if (cnt) usleep((int)(0.61f / FIFO_SETP * 1000000));
        }

        if (current_pmap_pointer->next == pmap_head && !opts.loop) break;
        usleep((int)(opts.dt * 1000000));
    }

    current_pmap_pointer = pmap_head;
    do {
        do {
            if (current_pmap_pointer->pmap == current_pixmap) break;
            if (current_pmap_pointer->pmap == 0) break;

            D("Free pixmap %lu", current_pmap_pointer->pmap);
            XFreePixmap(disp, current_pmap_pointer->pmap);
        } while (0);
        current_pmap_pointer = current_pmap_pointer->next;
    } while (current_pmap_pointer != pmap_head);

    D("Free pixmap %lu", origin);
    XFreePixmap(disp, origin);

    WEExit();
}

