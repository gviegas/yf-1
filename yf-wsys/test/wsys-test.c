/*
 * YF
 * wsys-test.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "yf/com/yf-clock.h"

#include "test.h"
#include "yf-wsys.h"

#define YF_TEST_ALL "all"
#define YF_TEST_SUBL "................................"
#define YF_TEST_SUBT \
  printf("%s\n%.*s\n", __func__, (int)strlen(__func__), YF_TEST_SUBL)

/* Variables & functions used by tests. */
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
    l_quit = 1;
    break;
  case YF_KEY_BACKSPACE:
    l_close = 1;
    break;
  case YF_KEY_1:
    l_mask = YF_EVT_CLOSEWD | YF_EVT_RESIZEWD;
    break;
  case YF_KEY_2:
    l_mask = YF_EVT_ENTERKB | YF_EVT_LEAVEKB | YF_EVT_KEYKB;
    break;
  case YF_KEY_3:
    l_mask =
      YF_EVT_ENTERPT | YF_EVT_LEAVEPT | YF_EVT_MOTIONPT | YF_EVT_BUTTONPT;
    break;
  case YF_KEY_9:
    l_mask = YF_EVT_ANY;
    break;
  case YF_KEY_0:
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

/* Window test. */
#define YF_TEST_WINDOW "window"

static int test_window(void)
{
  YF_TEST_SUBT;

  YF_window wins[] = {
    yf_window_init(400, 240, "#1", 0),
    yf_window_init(240, 400, "#2", 0),
    yf_window_init(480, 480, "#3", YF_WINCREAT_HIDDEN)
  };
  const size_t n = sizeof wins / sizeof wins[0];

  for (size_t i = 0; i < n; ++i) {
    if (wins[i] == NULL)
      return -1;
  }

  yf_sleep(1.0);
  yf_window_open(wins[2]);
  yf_sleep(1.0);
  yf_window_resize(wins[0], 640, 360);
  yf_sleep(1.0);
  yf_window_settitle(wins[1], "WIN2");
  yf_sleep(1.0);
  yf_window_close(wins[1]);
  yf_window_close(wins[2]);
  yf_sleep(1.0);
  yf_window_settitle(wins[2], "WIN3");
  yf_window_resize(wins[2], 800, 600);
  yf_window_open(wins[2]);
  yf_sleep(1.0);

  unsigned w, h;

  yf_window_getsize(wins[0], &w, &h);
  if (w != 640 || h != 360)
    return -1;
  yf_window_getsize(wins[1], &w, &h);
  if (w != 240 || h != 400)
    return -1;
  yf_window_getsize(wins[2], &w, &h);
  if (w != 800 || h != 600)
    return -1;

  for (size_t i = 0; i < n; ++i)
    yf_window_deinit(wins[i]);

  puts("");
  return 0;
}

/* Event test. */
#define YF_TEST_EVENT "event"

static int test_event(void)
{
  YF_TEST_SUBT;

  YF_window win1 = yf_window_init(400, 240, "EVT1", YF_WINCREAT_HIDDEN);
  YF_window win2 = yf_window_init(360, 360, "EVT2", YF_WINCREAT_HIDDEN);

  if (yf_getevtmask() != YF_EVT_NONE)
    return -1;

  for (size_t i = 0; i < (sizeof l_fns / sizeof l_fns[0]); ++i) {
    if (yf_getevtmask() & l_fns[i].evt)
      return -1;
    yf_setevtfn(l_fns[i].evt, l_fns[i].fn, (void *)((uintptr_t)l_fns[i].evt));
    if (!(yf_getevtmask() & l_fns[i].evt))
      return -1;
  }

  printf("\nwin1 is %p\nwin2 is %p\n\n", (void *)win1, (void *)win2);

  while (!l_quit) {
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

  puts("");
  return 0;
}

static const char *l_ids[] = {YF_TEST_WINDOW, YF_TEST_EVENT, YF_TEST_ALL};

/* Test function. */
static int test(int argc, char *argv[])
{
  assert(argc > 0);

  size_t test_n;
  size_t results;

  if (strcmp(argv[0], YF_TEST_WINDOW) == 0) {
    test_n = 1;
    results = test_window() == 0;
  } else if (strcmp(argv[0], YF_TEST_EVENT) == 0) {
    test_n = 1;
    results = test_event() == 0;
  } else if (strcmp(argv[0], YF_TEST_ALL) == 0) {
    int (*const tests[])(void) = {test_window, test_event};
    test_n = sizeof tests / sizeof tests[0];
    results = 0;
    for (size_t i = 0; i < test_n; ++i)
      results += tests[i]() == 0;
  } else {
    printf("! Error: no test named '%s'\n", argv[0]);
    printf("\nTry one of the following:\n");
    for (size_t i = 0; i < (sizeof l_ids / sizeof l_ids[0]); ++i)
      printf("* %s\n", l_ids[i]);
    printf("\n! No tests executed\n");
    return -1;
  }

  printf("\nDONE!\n\nNumber of tests executed: %lu\n", test_n);
  printf("> #%lu passed\n", results);
  printf("> #%lu failed\n", test_n - results);
  printf("\n(%.0f%% coverage)\n",(double)results / (double)test_n * 100.0);

  return 0;
}

const YF_test yf_g_test = {"wsys", test, l_ids, sizeof l_ids / sizeof l_ids[0]};
