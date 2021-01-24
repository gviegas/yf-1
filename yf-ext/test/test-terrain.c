/*
 * YF
 * test-terrain.c
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
#define YF_WINT "Terrain"
#define YF_FPS  30

/* Local variables. */
struct L_vars {
  YF_window win;
  YF_view view;
  YF_scene scn;
  YF_terrain terr;
  YF_texture hmap;
  YF_texture tex;

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
  /* TODO */

  printf("update (%.4f)\n", elapsed_time);

  if (l_vars.input.quit) {
    printf("quit\n");
    yf_view_stop(l_vars.view);
  }
}

/* Tests terrain rendering. */
int yf_test_terrain(void) {
  /* TODO */

  YF_evtfn evtfn = {.key_kb = on_key};
  yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);

  l_vars.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
  assert(l_vars.win != NULL);

  l_vars.view = yf_view_init(l_vars.win);
  assert(l_vars.view != NULL);

  l_vars.scn = yf_scene_init();
  assert(l_vars.scn != NULL);

  l_vars.terr = yf_terrain_init(32, 48);
  assert(l_vars.terr != NULL);

  l_vars.hmap = yf_texture_init(YF_FILETYPE_BMP, "tmp/hmap.bmp");
  assert(l_vars.hmap != NULL);

  l_vars.tex = yf_texture_init(YF_FILETYPE_BMP, "tmp/terrain.bmp");
  assert(l_vars.tex != NULL);

  yf_terrain_sethmap(l_vars.terr, l_vars.hmap);
  yf_terrain_settex(l_vars.terr, l_vars.tex);
  yf_node_insert(yf_scene_getnode(l_vars.scn), yf_terrain_getnode(l_vars.terr));
  yf_view_setscene(l_vars.view, l_vars.scn);

  if (yf_view_start(l_vars.view, YF_FPS, update) != 0)
    assert(0);

  return 0;
}
