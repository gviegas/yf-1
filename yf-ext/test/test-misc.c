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
  YF_node labl_node;
  YF_label labls[2];

  struct {
    int camera;
    int move[4];
    int turn[4];
    int toggle;
    int quit;
  } input;
};
static struct L_vars l_vars = {0};

/* Handles key events. */
static void on_key(int key, int state,
    YF_UNUSED unsigned mod_mask, YF_UNUSED void *arg)
{
  switch (key) {
    case YF_KEY_1:
      l_vars.input.camera = 1;
      yf_label_setstr(l_vars.labls[0], L"MODE: Camera");
      break;
    case YF_KEY_2:
      l_vars.input.camera = 0;
      yf_label_setstr(l_vars.labls[0], L"MODE: Object");
      break;
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
    case YF_KEY_T:
      l_vars.input.toggle = state;
      break;
    default:
      l_vars.input.quit |= state;
  }
}

/* Updates content */
static void update(double elapsed_time) {
  printf("update (%.4f)\n", elapsed_time);

  if (l_vars.input.quit) {
    puts("quit");
    yf_view_stop(l_vars.view);
  }

  if (l_vars.input.camera) {
    YF_camera cam = yf_scene_getcam(l_vars.scn);
    const YF_float md = 9.0 * elapsed_time;
    const YF_float td = 1.0 * elapsed_time;

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

  } else {
    const YF_float d = 6.0 * elapsed_time;
    const YF_float a = 3.14159265 * elapsed_time;
    YF_mat4 t, r;
    yf_mat4_iden(t);
    yf_mat4_iden(r);

    if (l_vars.input.move[0])
      yf_mat4_xlate(t, 0.0, 0.0, d);
    if (l_vars.input.move[1])
      yf_mat4_xlate(t, 0.0, 0.0, -d);
    if (l_vars.input.move[2])
      yf_mat4_xlate(t, -d, 0.0, 0.0);
    if (l_vars.input.move[3])
      yf_mat4_xlate(t, d, 0.0, 0.0);

    if (l_vars.input.turn[0]) {
      YF_vec4 q;
      yf_vec4_rotqx(q, a);
      yf_mat4_rotq(r, q);
    }
    if (l_vars.input.turn[1]) {
      YF_vec4 q;
      yf_vec4_rotqx(q, -a);
      yf_mat4_rotq(r, q);
    }
    if (l_vars.input.turn[2]) {
      YF_vec4 q;
      yf_vec4_rotqy(q, a);
      yf_mat4_rotq(r, q);
    }
    if (l_vars.input.turn[3]) {
      YF_vec4 q;
      yf_vec4_rotqy(q, -a);
      yf_mat4_rotq(r, q);
    }

    YF_mat4 m, tr;
    yf_mat4_copy(m, *yf_node_getxform(yf_model_getnode(l_vars.mdl)));
    yf_mat4_mul(tr, t, r);
    yf_mat4_mul(*yf_node_getxform(yf_model_getnode(l_vars.mdl)), m, tr);
  }

  if (l_vars.input.toggle) {
    if (yf_node_descends(l_vars.labl_node, yf_scene_getnode(l_vars.scn)))
      yf_node_drop(l_vars.labl_node);
    else
      yf_node_insert(yf_scene_getnode(l_vars.scn), l_vars.labl_node);
    l_vars.input.toggle = 0;
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

  l_vars.labl_node = yf_node_init();
  assert(l_vars.labl_node != NULL);

  YF_mesh mesh = yf_mesh_init(YF_FILETYPE_GLTF, "tmp/model2.gltf");
  assert(mesh != NULL);

  YF_texture texs[] = {yf_texture_init(YF_FILETYPE_BMP, "tmp/model2.bmp")};
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

  const size_t labl_n = sizeof l_vars.labls / sizeof l_vars.labls[0];
  for (size_t i = 0; i < labl_n; ++i) {
    l_vars.labls[i] = yf_label_init();
    assert(l_vars.labls[i] != NULL);

    yf_label_setfont(l_vars.labls[i], fonts[i%font_n]);
    YF_mat4 *m = yf_node_getxform(yf_label_getnode(l_vars.labls[i]));

    switch (i) {
      case 0:
        yf_label_setstr(l_vars.labls[i], L"MODE: Object");
        yf_label_setpt(l_vars.labls[i], 24);
        (*m)[12] = -0.75;
        (*m)[13] = -0.9;
        break;

      case 1:
        yf_label_setstr(l_vars.labls[i], L"test-misc");
        yf_label_setpt(l_vars.labls[i], 18);
        yf_label_setcolor(l_vars.labls[i], YF_CORNER_ALL, YF_COLOR_BLACK);
        (*m)[12] = 0.85;
        (*m)[13] = 0.9;
        break;

      default:
        yf_label_setstr(l_vars.labls[i], L"label");
        yf_label_setpt(l_vars.labls[i], 24+i*12);
        (*m)[12] = i*-0.15;
        (*m)[13] = i*-0.15;
    }

    yf_node_insert(l_vars.labl_node, yf_label_getnode(l_vars.labls[i]));
  }

  yf_node_insert(yf_scene_getnode(l_vars.scn), l_vars.labl_node);

  YF_camera cam = yf_scene_getcam(l_vars.scn);
  const YF_vec3 pos = {-4.0, 6.0, 15.0};
  const YF_vec3 tgt = {0};
  yf_camera_place(cam, pos);
  yf_camera_point(cam, tgt);

  yf_scene_setcolor(l_vars.scn, YF_COLOR_DARKGREY);
  yf_view_setscene(l_vars.view, l_vars.scn);

  if (yf_view_start(l_vars.view, YF_FPS, update) != 0)
    assert(0);

  yf_view_deinit(l_vars.view);
  yf_scene_deinit(l_vars.scn);
  yf_window_deinit(l_vars.win);
  yf_node_deinit(l_vars.labl_node);

  yf_model_deinit(l_vars.mdl);
  for (size_t i = 0; i < labl_n; ++i)
    yf_label_deinit(l_vars.labls[i]);

  yf_mesh_deinit(mesh);
  for (size_t i = 0; i < tex_n; ++i)
    yf_texture_deinit(texs[i]);
  for (size_t i = 0; i < font_n; ++i)
    yf_font_deinit(fonts[i]);

  return 0;
}
