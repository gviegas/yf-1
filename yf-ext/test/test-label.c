/*
 * YF
 * test-label.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include <yf/wsys/yf-event.h>
#include <yf/wsys/yf-keyboard.h>

#include "yf-ext.h"

#define YF_WINW 960
#define YF_WINH 600
#define YF_WINT "Label"
#define YF_FPS  30

/* Local variables. */
struct L_vars {
  YF_window win;
  YF_view view;
  YF_scene scn;
  YF_label labl;
  YF_font font;

  struct {
    int quit;
  } input;
};
static struct L_vars l_vars = {0};

/* Handles key events. */
static void on_key(int key, int state,
    YF_UNUSED unsigned mod_mask, YF_UNUSED void *arg)
{
  if (state == YF_KEYSTATE_RELEASED)
    return;

  switch (key) {
    default:
      l_vars.input.quit = 1;
  }
}

/* Updates content. */
static void update(double elapsed_time) {
  printf("update (%.4f)\n", elapsed_time);

  if (l_vars.input.quit) {
    printf("quit\n");
    yf_view_stop(l_vars.view);
  }
}

/* Tests label rendering. */
int yf_test_label(void) {
  YF_evtfn evtfn = {.key_kb = on_key};
  yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);

  l_vars.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
  assert(l_vars.win != NULL);

  l_vars.view = yf_view_init(l_vars.win);
  assert(l_vars.view != NULL);

  l_vars.scn = yf_scene_init();
  assert(l_vars.scn != NULL);

  l_vars.labl = yf_label_init();
  assert(l_vars.labl != NULL);

  l_vars.font = yf_font_init(YF_FILETYPE_TTF, "tmp/font.ttf");
  assert(l_vars.font != NULL);

  yf_label_setfont(l_vars.labl, l_vars.font);
  yf_label_setstr(l_vars.labl, L"Label");
  yf_label_setpt(l_vars.labl, 72);

  yf_node_insert(yf_scene_getnode(l_vars.scn), yf_label_getnode(l_vars.labl));

  yf_view_setscene(l_vars.view, l_vars.scn);

  if (yf_view_start(l_vars.view, YF_FPS, update) != 0)
    assert(0);

  yf_view_deinit(l_vars.view);
  yf_scene_deinit(l_vars.scn);
  yf_label_deinit(l_vars.labl);
  yf_font_deinit(l_vars.font);
  yf_window_deinit(l_vars.win);

  return 0;
}
