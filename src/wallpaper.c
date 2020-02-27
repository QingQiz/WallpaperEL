#include <X11/Xatom.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "wallpaper.h"
#include "mmonitor.h"
#include "imtools.h"
#include "options.h"
#include "atools.h"
#include "common.h"

static Pixmap current_wallpaper = 0, origin_pixmap = 0, current_pixmap = 0;
static pthread_t bgm_pthread, wallpaper_render_pthread, pixmap_free_pthread;
static pixmap_list *pmap_l_now, *pmap_l_head;
static sem_t sem_preloaded, sem_preload_limit, sem_start, sem_used, sem_xlock;

int sig_exit;

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

    current_wallpaper = pmap;
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
    pmap_l->status = FREED;
    pixmap_list *iter= pmap_l;

    int cnt = opts.fifo ? FIFO_SETP - 1 : 0;
    while (cnt--) {
        iter->next = (pixmap_list*)malloc(sizeof(pixmap_list));
        iter->next->prev = iter;
        iter->next->pmap = 0;
        iter->next->status = FREED;
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

        int cnt = opts.fifo ? FIFO_SETP : 1;
        while (cnt--) {
            iter->next = (pixmap_list*)malloc(sizeof(pixmap_list));
            iter->next->prev = iter;
            iter->next->pmap = 0;
            iter->next->status = FREED;
            iter = iter->next;
        }
    }
    iter->next = pmap_l;
    iter->next->prev = iter;
    return pmap_l;
}

static void WERenderWallpaperAsync() {
    Pixmap origin = WEGetCurrentWallpaperOrCreate();

    if (origin_pixmap == 0) origin_pixmap = origin, current_pixmap = origin;
    D("origin wallpaper: %lu", origin_pixmap);

    pixmap_list *iter = NULL;
    pixmap_list *head = NULL;

    head = WEBuildWallpaperList();
    pmap_l_head = head;
    pmap_l_now = head;

    int num_wallpaper = 1;
    for (iter = head; iter->next != head; iter = iter->next) {
        num_wallpaper++;
    }
    if (opts.loop && num_wallpaper == (opts.fifo ? FIFO_SETP: 1)) {
        DI("There is only one wallpaper, so --loop is disabled");
        opts.loop = 0;
    }
    iter = head;

    int num_to_preload = min(num_wallpaper, opts.max_preload);
    if (opts.fifo) num_to_preload = opts.fifo ? FIFO_SETP : 2;
    num_to_preload = min(num_to_preload, num_wallpaper);
    sem_init(&sem_preloaded, 0, 0);
    sem_init(&sem_preload_limit, 0, num_to_preload);
    D("wallpaper to preload: %d", num_to_preload);

    while (1) {
        WELoadNextImage();
        int cnt = opts.fifo ? FIFO_SETP : 1;
        while (cnt--) {
            P(sem_preload_limit);
            if (sig_exit) return;

            if (iter->status != FREED) {
                V(sem_preload_limit);
                iter = iter->next;
                continue;
            }

            P(sem_xlock);
            iter->pmap = XCreatePixmap(disp, root, scr->width, scr->width, depth);
            if (opts.fifo) WECopyPixmap(disp, iter->pmap, origin);

            WERenderCurrentImageToPixmap(
                iter->pmap,
                cnt == 0 ? 255 : (int)(255. - 255. / FIFO_SETP * cnt)
            );
            V(sem_xlock);

            iter->status = UNUSED;

            V(sem_preloaded);

            static char started = 0;
            if (!started) {
                int num_preloaded;
                sem_getvalue(&sem_preloaded, &num_preloaded);
                if (num_preloaded == num_to_preload) {
                    started = 1;
                    V(sem_start);
                }
            }
            iter = iter->next;
        }
        origin = iter->prev->pmap;
        current_pixmap = origin;
    }
}

static void WEFreePixmapAsync() {
    pixmap_list *iter = pmap_l_head;

    while (1) {
        P(sem_used);

        if (sig_exit) break;

        if (iter->status != USED) {
            V(sem_used);
            iter = iter->next;
            continue;
        }
        if (iter->pmap != current_pixmap) {
            D("XFreePixmap: %lu", iter->pmap);

            P(sem_xlock);
            XFreePixmap(disp, iter->pmap);
            V(sem_xlock);

            iter->pmap = 0;
            iter->status = FREED;

            V(sem_preload_limit);
            P(sem_preloaded);
        }
        iter = iter->next;
    }

    iter = pmap_l_head;
    do {
        if (iter->pmap && iter->pmap != current_wallpaper) {
            D("Free Pixmap %lu", iter->pmap);
            P(sem_xlock);
            XFreePixmap(disp, iter->pmap);
            V(sem_xlock);
            iter->pmap = 0;
            // do NOT set status to FREED
        }
        iter = iter->next;
    } while (iter != pmap_l_head);

    D("Free Origin Pixmap %lu", origin_pixmap);
    XFreePixmap(disp, origin_pixmap);
}

static Pixmap WEGetNextWallpaper() {
    static pixmap_list *iter = NULL;
    if (iter == NULL) iter = pmap_l_head;

    if (iter->pmap == 0) return 0;

    pmap_l_now = iter;
    iter = iter->next;
    return iter->prev->pmap;
}

static void WEExit() {
    sig_exit = 1;
    V(sem_used);
    V(sem_preload_limit);

    Await(wallpaper_render_pthread);
    Await(pixmap_free_pthread);
    if (opts.bgm) Await(bgm_pthread);

    if (current_wallpaper == 0) return;
    Pixmap tofree = current_wallpaper;

    // open new display
    Display *disp2 = XOpenDisplay(NULL);
    assert(disp2, "Can not reopen display");
    Window root2 = RootWindow(disp2, DefaultScreen(disp2));

    // copy current pixmap to new display
    Pixmap pmap_final = XCreatePixmap(disp2, root2, scr->width, scr->height, depth);

    WECopyPixmap(disp2, pmap_final, tofree);

    XSync(disp, False);
    XSync(disp2, False);

    // set new pixmap as wallpaper
    WESetWallpaper(disp2, pmap_final);
    XSetCloseDownMode(disp2, RetainPermanent);

    D("Free pixmap %lu", tofree);
    XFreePixmap(disp, tofree);

    XCloseDisplay(disp2);

    sem_destroy(&sem_preloaded);
    sem_destroy(&sem_preload_limit);
    sem_destroy(&sem_start);
    sem_destroy(&sem_used);
    sem_destroy(&sem_xlock);
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
    // fix options
    if (opts.dt < MIN_FIFO_ENABLE_TIME) {
        DI("--fifo is disabled");
        opts.fifo = 0;
    }
    opts.dt -= 0.0007;
    opts.dt = opts.dt < 0 ? 0: opts.dt;
    // catch SIGING
    if (signal(SIGINT, WESigintHandler) == SIG_ERR) {
        DW("Can not catch SIGINT");
    }

    // steps for a wallpaper
    int steps = opts.fifo ? FIFO_SETP : 1;

    // thread bgm
    if (opts.bgm != NULL) {
        WEAtoolsInit(opts.bgm);
        Async_call(WEAtoolsPlay, bgm_pthread);
    }

    // init semaphore
    sem_init(&sem_start, 0, 0);
    sem_init(&sem_used , 0, 0);
    sem_init(&sem_xlock, 0, 1);
    sig_exit = 0;

    // thread load image
    Async_call(WERenderWallpaperAsync, wallpaper_render_pthread);

    // waiting to start
    P(sem_start);
    Async_call(WEFreePixmapAsync, pixmap_free_pthread);
    if (opts.bgm != NULL) V(sem_bgm_start);

    // set wallpaper
    // operate with different displays to avoid blocking WESetWallpaper
    Display *disp2 = XOpenDisplay(NULL);
    assert(disp2, "can not reopen X display");
    while (1) {
        if (pmap_l_now->next == pmap_l_head && current_wallpaper) {
            if (!opts.loop) break;
        }
        if (current_wallpaper) fsleep(opts.dt);
        for (int i = 0; i < steps; ++i) {
            Pixmap todraw = WEGetNextWallpaper();
            if (pmap_l_now->status == FREED || todraw == 0) {
                DW("Frame loss");
            } else {
                WESetWallpaper(disp2, todraw);

                if (pmap_l_now->prev->status == USING) {
                    pmap_l_now->prev->status = USED;
                    V(sem_used);
                }
                pmap_l_now->status = USING;
            }
            if (i != steps - 1) fsleep(FIFO_DURATION / FIFO_SETP);
        }
    }
    XCloseDisplay(disp2);

    WEExit();
}

