/*
 * YF
 * test-misc.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include <yf/wsys/yf-wsys.h>

#include "yf-ext.h"

#define YF_WINW 800
#define YF_WINH 600
#define YF_WINT "Misc"
#define YF_FPS  30

/* Local variables. */
struct L_vars {
  YF_window win;
  YF_view view;
  YF_scene scn;
  YF_quad quads[3];
  YF_label labls[4];

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

/* Updates content */
static void update(double elapsed_time) {
  printf("update (%.4f)\n", elapsed_time);

  if (l_vars.input.quit) {
    puts("quit");
    yf_view_stop(l_vars.view);
  }
}

/* Tests miscellany. */
int yf_test_misc(void) {
  YF_evtfn evtfn = {.key_kb = on_key};
  yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);

  l_vars.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
  assert(l_vars.win != NULL);

  l_vars.view = yf_view_init(l_vars.win);
  assert(l_vars.view != NULL);

  l_vars.scn = yf_scene_init();
  assert(l_vars.scn != NULL);

  YF_texture texs[] = {yf_texture_init(YF_FILETYPE_BMP, "tmp/quad.bmp")};
  const size_t tex_n = sizeof texs / sizeof texs[0];
  for (size_t i = 0; i < tex_n; ++i)
    assert(texs[i] != NULL);

  YF_font fonts[] = {yf_font_init(YF_FILETYPE_TTF, "tmp/font.ttf")};
  const size_t font_n = sizeof fonts / sizeof fonts[0];
  for (size_t i = 0; i < font_n; ++i)
    assert(fonts[i] != NULL);

  const size_t quad_n = sizeof l_vars.quads / sizeof l_vars.quads[0];
  for (size_t i = 0; i < quad_n; ++i) {
    l_vars.quads[i] = yf_quad_init();
    assert(l_vars.quads[i] != NULL);

    yf_quad_settex(l_vars.quads[i], texs[i%tex_n]);

    YF_mat4 *m = yf_quad_getxform(l_vars.quads[i]);
    (*m)[12] = i<<1;

    yf_node_insert(yf_scene_getnode(l_vars.scn),
        yf_quad_getnode(l_vars.quads[i]));
  }

  const size_t labl_n = sizeof l_vars.labls / sizeof l_vars.labls[0];
  for (size_t i = 0; i < labl_n; ++i) {
    l_vars.labls[i] = yf_label_init();
    assert(l_vars.labls[i] != NULL);

    yf_label_setfont(l_vars.labls[i], fonts[i%font_n]);
    yf_label_setstr(l_vars.labls[i], L"label");

    YF_mat4 *m = yf_label_getxform(l_vars.labls[i]);
    (*m)[12] = i<<1;
    (*m)[13] = i<<1;
    (*m)[14] = i<<1;

    yf_node_insert(yf_scene_getnode(l_vars.scn),
        yf_label_getnode(l_vars.labls[i]));
  }

  yf_view_setscene(l_vars.view, l_vars.scn);

  if (yf_view_start(l_vars.view, YF_FPS, update) != 0)
    assert(0);

  yf_view_deinit(l_vars.view);
  yf_scene_deinit(l_vars.scn);
  yf_window_deinit(l_vars.win);
  for (size_t i = 0; i < quad_n; ++i)
    yf_quad_deinit(l_vars.quads[i]);
  for (size_t i = 0; i < labl_n; ++i)
    yf_label_deinit(l_vars.labls[i]);
  for (size_t i = 0; i < tex_n; ++i)
    yf_texture_deinit(texs[i]);
  for (size_t i = 0; i < font_n; ++i)
    yf_font_deinit(fonts[i]);

  return 0;
}
