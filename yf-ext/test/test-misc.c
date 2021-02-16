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
#define YF_FPS  60

/* Local variables. */
struct L_vars {
  YF_window win;
  YF_view view;
  YF_scene scn;
  YF_model mdl;
  YF_node quad_node;
  YF_node labl_node;
  YF_quad quads[3];
  YF_label labls[4];

  struct {
    int move[4];
    int turn[4];
    int toggle[2];
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
    case YF_KEY_1:
      l_vars.input.toggle[0] = state;
      break;
    case YF_KEY_2:
      l_vars.input.toggle[1] = state;
      break;
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

  if (l_vars.input.toggle[0]) {
    if (yf_node_descends(l_vars.quad_node, yf_scene_getnode(l_vars.scn)))
      yf_node_drop(l_vars.quad_node);
    else
      yf_node_insert(yf_scene_getnode(l_vars.scn), l_vars.quad_node);
    l_vars.input.toggle[0] = 0;
  }
  if (l_vars.input.toggle[1]) {
    if (yf_node_descends(l_vars.labl_node, yf_scene_getnode(l_vars.scn)))
      yf_node_drop(l_vars.labl_node);
    else
      yf_node_insert(yf_scene_getnode(l_vars.scn), l_vars.labl_node);
    l_vars.input.toggle[1] = 0;
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

  l_vars.quad_node = yf_node_init();
  assert(l_vars.quad_node != NULL);

  l_vars.labl_node = yf_node_init();
  assert(l_vars.labl_node != NULL);

  YF_mesh mesh = yf_mesh_init(YF_FILETYPE_GLTF, "tmp/cube.gltf");
  assert(mesh != NULL);

  YF_texture texs[] = {
    yf_texture_init(YF_FILETYPE_BMP, "tmp/cube_alt.bmp"),
    yf_texture_init(YF_FILETYPE_BMP, "tmp/quad.bmp")
  };
  const size_t tex_n = sizeof texs / sizeof texs[0];
  for (size_t i = 0; i < tex_n; ++i)
    assert(texs[i] != NULL);

  YF_font fonts[] = {yf_font_init(YF_FILETYPE_TTF, "tmp/font.ttf")};
  const size_t font_n = sizeof fonts / sizeof fonts[0];
  for (size_t i = 0; i < font_n; ++i)
    assert(fonts[i] != NULL);

  l_vars.mdl = yf_model_init();
  assert(l_vars.mdl != NULL);

  yf_model_setmesh(l_vars.mdl, mesh);
  yf_model_settex(l_vars.mdl, texs[0]);

  yf_node_insert(yf_scene_getnode(l_vars.scn), yf_model_getnode(l_vars.mdl));

  const size_t quad_n = sizeof l_vars.quads / sizeof l_vars.quads[0];
  for (size_t i = 0; i < quad_n; ++i) {
    l_vars.quads[i] = yf_quad_init();
    assert(l_vars.quads[i] != NULL);

    yf_quad_settex(l_vars.quads[i], texs[1]);
    if (i == quad_n-1) {
      YF_rect rect = *yf_quad_getrect(l_vars.quads[i]);
      rect.size.width >>= 1;
      yf_quad_setrect(l_vars.quads[i], &rect);
    }

    YF_mat4 *m = yf_quad_getxform(l_vars.quads[i]);
    (*m)[12] = i*0.2;
    (*m)[0] = (i+1)*0.65;
    (*m)[5] = (i+1)*0.65;

    yf_node_insert(l_vars.quad_node, yf_quad_getnode(l_vars.quads[i]));
  }
  yf_node_insert(yf_scene_getnode(l_vars.scn), l_vars.quad_node);

  const size_t labl_n = sizeof l_vars.labls / sizeof l_vars.labls[0];
  for (size_t i = 0; i < labl_n; ++i) {
    l_vars.labls[i] = yf_label_init();
    assert(l_vars.labls[i] != NULL);

    yf_label_setfont(l_vars.labls[i], fonts[i%font_n]);
    yf_label_setstr(l_vars.labls[i], L"label");
    yf_label_setpt(l_vars.labls[i], 24+i*12);

    YF_mat4 *m = yf_label_getxform(l_vars.labls[i]);
    (*m)[12] = i*-0.15;
    (*m)[13] = i*-0.15;

    yf_node_insert(l_vars.labl_node, yf_label_getnode(l_vars.labls[i]));
  }
  yf_node_insert(yf_scene_getnode(l_vars.scn), l_vars.labl_node);

  yf_scene_setcolor(l_vars.scn, YF_COLOR_DARKGREY);
  yf_view_setscene(l_vars.view, l_vars.scn);

  if (yf_view_start(l_vars.view, YF_FPS, update) != 0)
    assert(0);

  yf_view_deinit(l_vars.view);
  yf_scene_deinit(l_vars.scn);
  yf_window_deinit(l_vars.win);
  yf_node_deinit(l_vars.quad_node);
  yf_node_deinit(l_vars.labl_node);

  yf_model_deinit(l_vars.mdl);
  for (size_t i = 0; i < quad_n; ++i)
    yf_quad_deinit(l_vars.quads[i]);
  for (size_t i = 0; i < labl_n; ++i)
    yf_label_deinit(l_vars.labls[i]);

  yf_mesh_deinit(mesh);
  for (size_t i = 0; i < tex_n; ++i)
    yf_texture_deinit(texs[i]);
  for (size_t i = 0; i < font_n; ++i)
    yf_font_deinit(fonts[i]);

  return 0;
}
