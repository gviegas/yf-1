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

#include "test.h"
#include "yf-event.h"
#include "yf-window.h"
#include "yf-pointer.h"
#include "yf-keyboard.h"

/* Variables & functions used by 'test_event'. */
static unsigned mask_ = YF_EVT_ANY;
static int quit_ = 0;
static int close_ = 1;

static void close_wd(YF_window win, void *data)
{
    printf(" close_wd: %p, %p\n", (void *)win, data);
    yf_sleep(1.0);
    quit_ = 1;
}

static void resize_wd(YF_window win, unsigned width, unsigned height,
                      void *data)
{
    printf(" resize_wd: %p, %u, %u, %p\n", (void *)win, width, height, data);
    unsigned w, h;
    yf_window_getsize(win, &w, &h);
    assert(w == width && h == height);
}

static void enter_kb(YF_window win, void *data)
{
    printf(" enter_kb: %p, %p\n", (void *)win, data);
}

static void leave_kb(YF_window win, void *data)
{
    printf(" leave_kb: %p, %p\n", (void *)win, data);
}

static void key_kb(int key, int state, unsigned mod_mask, void *data)
{
    printf(" key_kb: %d, %d, %xh, %p\n", key, state, mod_mask, data);

    if (state == YF_KEYSTATE_PRESSED)
        return;

    switch (key) {
    case YF_KEY_ESC:
        puts(" <quit>");
        quit_ = 1;
        break;
    case YF_KEY_BACKSPACE:
        puts(" <close>");
        close_ = 1;
        break;
    case YF_KEY_1:
        puts(" <mask: close/resize wd>");
        mask_ = YF_EVT_CLOSEWD | YF_EVT_RESIZEWD;
        break;
    case YF_KEY_2:
        puts(" <mask: enter/leve/key kb>");
        mask_ = YF_EVT_ENTERKB | YF_EVT_LEAVEKB | YF_EVT_KEYKB;
        break;
    case YF_KEY_3:
        puts(" <mask: enter/leave/motion/button pt>");
        mask_ = YF_EVT_ENTERPT | YF_EVT_LEAVEPT |
                YF_EVT_MOTIONPT | YF_EVT_BUTTONPT;
        break;
    case YF_KEY_9:
        puts(" <mask: any>");
        mask_ = YF_EVT_ANY;
        break;
    case YF_KEY_0:
        puts(" <mask: none>");
        mask_ = YF_EVT_NONE;
        break;
    }
}

static void enter_pt(YF_window win, int x, int y, void *data)
{
    printf(" enter_pt: %p, %d, %d, %p\n", (void *)win, x, y, data);
}

static void leave_pt(YF_window win, void *data)
{
    printf(" leave_pt: %p, %p\n", (void *)win, data);
}

static void motion_pt(int x, int y, void *data)
{
    printf(" motion_pt: %d, %d, %p\n", x, y, data);
}

static void button_pt(int btn, int state, int x, int y, void *data)
{
    printf(" button_pt: %d, %d, %d, %d, %p\n", btn, state, x, y, data);

    if (state == YF_BTNSTATE_PRESSED)
        return;

    if (btn == YF_BTN_LEFT)
        mask_ |= YF_EVT_KEYKB;
    else if (btn == YF_BTN_RIGHT)
        mask_ &= ~YF_EVT_KEYKB;
}

struct T_fn { int evt; YF_evtfn fn; };
static const struct T_fn fns_[] = {
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
    assert(win1 != NULL);
    YF_window win2 = yf_window_init(360, 360, "EVT2", YF_WINCREAT_HIDDEN);
    assert(win2 != NULL);

    char s[64] = {0};
    snprintf(s, sizeof s, "0x%x", yf_getevtmask());
    YF_TEST_PRINT("getevtmask", "", s);
    if (yf_getevtmask() != YF_EVT_NONE)
        return -1;

    for (size_t i = 0; i < (sizeof fns_ / sizeof *fns_); i++) {
        snprintf(s, sizeof s, "0x%x, <fn>, 0x%x", fns_[i].evt, fns_[i].evt);
        YF_TEST_PRINT("setevtfn", s, "");
        if (yf_getevtmask() & fns_[i].evt)
            return -1;
        yf_setevtfn(fns_[i].evt, fns_[i].fn, (void *)((uintptr_t)fns_[i].evt));
        if (!(yf_getevtmask() & fns_[i].evt))
            return -1;
    }

    printf("\nwin1 is %p\nwin2 is %p\n", (void *)win1, (void *)win2);

    while (!quit_) {
        snprintf(s, sizeof s, "0x%x", mask_);
        YF_TEST_PRINT("pollevt", s, "");
        yf_pollevt(mask_);
        yf_sleep(0.0333);

        if (close_) {
            yf_window_close(win1);
            yf_window_close(win2);
            yf_sleep(2.0);
            yf_window_open(win2);
            yf_window_open(win1);
            close_ = 0;
        }
    }

    yf_window_deinit(win1);
    yf_window_deinit(win2);

    return 0;
}
