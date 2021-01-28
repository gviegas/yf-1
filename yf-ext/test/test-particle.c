/*
 * YF
 * test-particle.c
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
#define YF_WINT "Particle"
#define YF_FPS  30

/* Local variables. */
struct L_vars {
  YF_window win;
  YF_view view;
  YF_scene scn;
  YF_particle part;
  YF_texture tex;

  struct {
    int move[4];
    int turn[4];
    int quit;
  } input;
};
static struct L_vars l_vars = {0};

/* Handles key events. */
static void on_key(int key, int state,
    YF_UNUSED unsigned mod_mask, YF_UNUSED void *arg)
{
  switch (key) {
    case YF_KEY_W:
      l_vars.input.move[0] = state;
      break;
    case YF_KEY_S:
      l_vars.input.move[1] = state;
      break;
    case YF_KEY_A:
      l_vars.input.move[2] = state;
      break;
    case YF_KEY_D:
      l_vars.input.move[3] = state;
      break;
    case YF_KEY_UP:
      l_vars.input.turn[0] = state;
      break;
    case YF_KEY_DOWN:
      l_vars.input.turn[1] = state;
      break;
    case YF_KEY_LEFT:
      l_vars.input.turn[2] = state;
      break;
    case YF_KEY_RIGHT:
      l_vars.input.turn[3] = state;
      break;
    default:
      l_vars.input.quit |= state;
  }
}

/* Updates content. */
static void update(double elapsed_time) {
  printf("update (%.4f)\n", elapsed_time);

  if (l_vars.input.quit) {
    printf("quit\n");
    yf_view_stop(l_vars.view);
  }

  YF_camera cam = yf_scene_getcam(l_vars.scn);
  static const YF_float md = 1.0;
  static const YF_float td = 0.1;

  if (l_vars.input.move[0])
    yf_camera_movef(cam, md);
  if (l_vars.input.move[1])
    yf_camera_moveb(cam, md);
  if (l_vars.input.move[2])
    yf_camera_movel(cam, md);
  if (l_vars.input.move[3])
    yf_camera_mover(cam, md);
  if (l_vars.input.turn[0])
    yf_camera_turnu(cam, td);
  if (l_vars.input.turn[1])
    yf_camera_turnd(cam, td);
  if (l_vars.input.turn[2])
    yf_camera_turnl(cam, td);
  if (l_vars.input.turn[3])
    yf_camera_turnr(cam, td);

  yf_particle_simulate(l_vars.part, elapsed_time);
}

/* Tests particle rendering. */
int yf_test_particle(void) {
  YF_evtfn evtfn = {.key_kb = on_key};
  yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);

  l_vars.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
  assert(l_vars.win != NULL);

  l_vars.view = yf_view_init(l_vars.win);
  assert(l_vars.view != NULL);

  l_vars.scn = yf_scene_init();
  assert(l_vars.scn != NULL);

  l_vars.part = yf_particle_init(1000);
  assert(l_vars.part != NULL);

  l_vars.tex = yf_texture_init(YF_FILETYPE_BMP, "tmp/sprite.bmp");
  assert(l_vars.tex != NULL);

  yf_particle_settex(l_vars.part, l_vars.tex);
  yf_mat4_scale(*yf_particle_getxform(l_vars.part), 0.5, 1.0, 0.25);

  yf_node_insert(yf_scene_getnode(l_vars.scn),
      yf_particle_getnode(l_vars.part));

  yf_view_setscene(l_vars.view, l_vars.scn);

  if (yf_view_start(l_vars.view, YF_FPS, update) != 0)
    assert(0);

  /* TODO: Deinitialization. */

  return 0;
}
