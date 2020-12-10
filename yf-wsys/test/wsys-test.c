/*
 * YF
 * wsys-test.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "test.h"

#include "yf-platform.h"
#include "yf-window.h"
#include "yf-event.h"
#include "yf-keyboard.h"
#include "yf-pointer.h"

#include <yf/com/yf-clock.h>

static int l_quit = 0;
static int l_close = 1;

static void close_wd(YF_window win, void *data) {
  printf("close_wd: %p, %p\n", (void *)win, data);
}

static void resize_wd(YF_window win, unsigned width, unsigned height,
    void *data)
{
  printf("resize_wd: %p, %u, %u, %p\n", (void *)win, width, height, data);
}

static void enter_kb(YF_window win, void *data) {
  printf("enter_kb: %p, %p\n", (void *)win, data);
}

static void leave_kb(YF_window win, void *data) {
  printf("leave_kb: %p, %p\n", (void *)win, data);
}

static void key_kb(int key, int state, unsigned mod_mask, void *data) {
  printf("key_kb: %d, %d, %xh, %p\n", key, state, mod_mask, data);

  if (key == YF_KEY_ESC) {
    l_quit = 1;
  } else if (key == YF_KEY_BACKSPACE) {
    l_close = 1;
    yf_window_close((YF_window)data);
  }
}

static void enter_pt(YF_window win, int x, int y, void *data) {
  printf("enter_pt: %p, %d, %d, %p\n", (void *)win, x, y, data);
}

static void leave_pt(YF_window win, void *data) {
  printf("leave_pt: %p, %p\n", (void *)win, data);
}

static void motion_pt(int x, int y, void *data) {
  printf("motion_pt: %d, %d, %p\n", x, y, data);
}

static void button_pt(int btn, int state, int x, int y, void *data) {
  printf("button_pt: %d, %d, %d, %d, %p\n", btn, state, x, y, data);
}

struct L_fn { int evt; YF_evtfn fn; };
static const struct L_fn l_fns[] = {
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

static int test_window(void) {
  /* TODO */
  return -1;
}

static int test_event(void) {
  /* TODO */
  return -1;
}

static int test(int argc, char *argv[]) {
  /* TODO */
  return 0;
}

const YF_test yf_g_test = {"wsys", test};
