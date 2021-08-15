/*
 * YF
 * test-event.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#include "yf/com/yf-clock.h"

#include "yf-event.h"
#include "yf-window.h"
#include "yf-pointer.h"
#include "yf-keyboard.h"

/* Variables & functions used by 'test_event'. */
static unsigned l_mask = YF_EVT_ANY;
static int l_quit = 0;
static int l_close = 1;

static void close_wd(YF_window win, void *data)
{
    printf("close_wd: %p, %p\n", (void *)win, data);
    yf_sleep(1.0);
    l_quit = 1;
}

static void resize_wd(YF_window win, unsigned width, unsigned height,
                      void *data)
{
    printf("resize_wd: %p, %u, %u, %p\n", (void *)win, width, height, data);
    unsigned w, h;
    yf_window_getsize(win, &w, &h);
    assert(w == width && h == height);
}

static void enter_kb(YF_window win, void *data)
{
    printf("enter_kb: %p, %p\n", (void *)win, data);
}

static void leave_kb(YF_window win, void *data)
{
    printf("leave_kb: %p, %p\n", (void *)win, data);
}

static void key_kb(int key, int state, unsigned mod_mask, void *data)
{
    printf("key_kb: %d, %d, %xh, %p\n", key, state, mod_mask, data);

    if (state == YF_KEYSTATE_PRESSED)
        return;

    switch (key) {
    case YF_KEY_ESC:
        puts("<quit>\n");
        l_quit = 1;
        break;
    case YF_KEY_BACKSPACE:
        puts("<close>\n");
        l_close = 1;
        break;
    case YF_KEY_1:
        puts("<mask: close/resize wd>\n");
        l_mask = YF_EVT_CLOSEWD | YF_EVT_RESIZEWD;
        break;
    case YF_KEY_2:
        puts("<mask: enter/leve/key kb>\n");
        l_mask = YF_EVT_ENTERKB | YF_EVT_LEAVEKB | YF_EVT_KEYKB;
        break;
    case YF_KEY_3:
        puts("<mask: enter/leave/motion/button pt>\n");
        l_mask =
            YF_EVT_ENTERPT | YF_EVT_LEAVEPT | YF_EVT_MOTIONPT | YF_EVT_BUTTONPT;
        break;
    case YF_KEY_9:
        puts("<mask: any>\n");
        l_mask = YF_EVT_ANY;
        break;
    case YF_KEY_0:
        puts("<mask: none>\n");
        l_mask = YF_EVT_NONE;
        break;
    }
}

static void enter_pt(YF_window win, int x, int y, void *data)
{
    printf("enter_pt: %p, %d, %d, %p\n", (void *)win, x, y, data);
}

static void leave_pt(YF_window win, void *data)
{
    printf("leave_pt: %p, %p\n", (void *)win, data);
}

static void motion_pt(int x, int y, void *data)
{
    printf("motion_pt: %d, %d, %p\n", x, y, data);
}

static void button_pt(int btn, int state, int x, int y, void *data)
{
    printf("button_pt: %d, %d, %d, %d, %p\n", btn, state, x, y, data);

    if (state == YF_BTNSTATE_PRESSED)
        return;

    if (btn == YF_BTN_LEFT)
        l_mask |= YF_EVT_KEYKB;
    else if (btn == YF_BTN_RIGHT)
        l_mask &= ~YF_EVT_KEYKB;
}

struct T_fn { int evt; YF_evtfn fn; };
static const struct T_fn l_fns[] = {
    { YF_EVT_CLOSEWD,  {.close_wd = close_wd}   },
    { YF_EVT_RESIZEWD, {.resize_wd = resize_wd} },
    { YF_EVT_ENTERKB,  {.enter_kb = enter_kb}   },
    { YF_EVT_LEAVEKB,  {.leave_kb = leave_kb}   },
    { YF_EVT_KEYKB,    {.key_kb = key_kb}       },
    { YF_EVT_ENTERPT,  {.enter_pt = enter_pt}   },
    { YF_EVT_LEAVEPT,  {.leave_pt = leave_pt}   },
    { YF_EVT_MOTIONPT, {.motion_pt = motion_pt} },
    { YF_EVT_BUTTONPT, {.button_pt = button_pt} }
};

/* Tests event. */
int yf_test_event(void)
{
    YF_window win1 = yf_window_init(400, 240, "EVT1", YF_WINCREAT_HIDDEN);
    YF_window win2 = yf_window_init(360, 360, "EVT2", YF_WINCREAT_HIDDEN);

    const unsigned mask = yf_getevtmask();
    printf("(getevtmask)\n 0x%x\n\n", mask);
    if (mask != YF_EVT_NONE)
        return -1;

    for (size_t i = 0; i < (sizeof l_fns / sizeof l_fns[0]); i++) {
        printf("(setevtfn %d, ...)\n\n", l_fns[i].evt);
        if (yf_getevtmask() & l_fns[i].evt)
            return -1;
        yf_setevtfn(l_fns[i].evt, l_fns[i].fn,
                    (void *)((uintptr_t)l_fns[i].evt));
        if (!(yf_getevtmask() & l_fns[i].evt))
            return -1;
    }

    printf("win1 is %p\nwin2 is %p\n\n", (void *)win1, (void *)win2);

    while (!l_quit) {
        printf("(pollevt 0x%x)\n\n", l_mask);
        yf_pollevt(l_mask);
        yf_sleep(0.0333);

        if (l_close) {
            yf_window_close(win1);
            yf_window_close(win2);
            yf_sleep(2.0);
            yf_window_open(win2);
            yf_window_open(win1);
            l_close = 0;
        }
    }

    yf_window_deinit(win1);
    yf_window_deinit(win2);

    return 0;
}
