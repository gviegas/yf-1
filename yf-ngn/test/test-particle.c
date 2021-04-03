/*
 * YF
 * test-particle.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "yf/wsys/yf-event.h"
#include "yf/wsys/yf-keyboard.h"

#include "yf-ngn.h"

#define YF_WINW 960
#define YF_WINH 600
#define YF_WINT "Particle"
#define YF_FPS  30

/* Local variables. */
struct T_vars {
  YF_window win;
  YF_view view;
  YF_scene scn;
  YF_particle part;
  YF_texture tex;

  struct {
    int move[4];
    int turn[4];
    int once;
    int rgb[3];
    int quit;
  } input;
};
static struct T_vars l_vars = {0};

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
  case YF_KEY_1:
    l_vars.input.once = state;
    break;
  case YF_KEY_R:
    if (state)
      l_vars.input.rgb[0] = ~l_vars.input.rgb[0];
    break;
  case YF_KEY_G:
    if (state)
      l_vars.input.rgb[1] = ~l_vars.input.rgb[1];
    break;
  case YF_KEY_B:
    if (state)
      l_vars.input.rgb[2] = ~l_vars.input.rgb[2];
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

  YF_psys *sys = yf_particle_getsys(l_vars.part);
  sys->lifetime.once = l_vars.input.once;
  sys->color.max[0] = l_vars.input.rgb[0] ? 0.0 : 1.0;
  sys->color.max[1] = l_vars.input.rgb[1] ? 0.0 : 1.0;
  sys->color.max[2] = l_vars.input.rgb[2] ? 0.0 : 1.0;

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

  l_vars.tex = yf_texture_init(YF_FILETYPE_PNG, "tmp/sprite.png");
  assert(l_vars.tex != NULL);

  yf_particle_settex(l_vars.part, l_vars.tex);
  yf_mat4_scale(*yf_node_getxform(yf_particle_getnode(l_vars.part)),
      0.5, 1.0, 0.25);

  YF_psys *sys = yf_particle_getsys(l_vars.part);
  sys->velocity.min[1] = -0.001;
  sys->velocity.max[1] = 0.25;
  sys->lifetime.spawn_min = 0.1;
  sys->lifetime.spawn_max = 0.5;
  sys->lifetime.duration_min = 0.75;
  sys->lifetime.duration_max = 2.5;

  yf_node_insert(yf_scene_getnode(l_vars.scn),
      yf_particle_getnode(l_vars.part));

  YF_camera cam = yf_scene_getcam(l_vars.scn);
  const YF_vec3 pos = {0.0, 0.0, 20.0};
  const YF_vec3 tgt = {0.0, 6.0, 0.0};
  yf_camera_place(cam, pos);
  yf_camera_point(cam, tgt);

  yf_view_setscene(l_vars.view, l_vars.scn);

  if (yf_view_start(l_vars.view, YF_FPS, update) != 0)
    assert(0);

  yf_view_deinit(l_vars.view);
  yf_scene_deinit(l_vars.scn);
  yf_particle_deinit(l_vars.part);
  yf_texture_deinit(l_vars.tex);
  yf_window_deinit(l_vars.win);

  return 0;
}
