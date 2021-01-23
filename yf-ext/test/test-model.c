/*
 * YF
 * test-model.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include <yf/com/yf-util.h>
#include <yf/wsys/yf-event.h>
#include <yf/wsys/yf-keyboard.h>

#include "yf-ext.h"

#define YF_WINW 960
#define YF_WINH 600
#define YF_WINT "Model"
#define YF_FPS 60

/* Local variables. */
struct L_vars {
  YF_window win;
  YF_view view;

#define YF_SCNN 2
  YF_scene scn[YF_SCNN];

#define YF_MDLN 12
  YF_model mdl[YF_MDLN];
  YF_mesh mesh[YF_MDLN];
  YF_texture tex[YF_MDLN];

  struct {
    int quit;
    int swap;
    float speed;
  } input;
};
static struct L_vars l_vars = {0};

/* Transformation matrices for each model. */
static const YF_mat4 l_xforms[] = {
  {
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
  },
  {
    2.0, 0.0, 0.0, 0.0,
    0.0, 2.0, 0.0, 0.0,
    0.0, 0.0, 2.0, 0.0,
    -6.0, -2.0, 4.0, 1.0
  },
  {
    1.5, 0.0, 0.0, 0.0,
    0.0, 1.5, 0.0, 0.0,
    0.0, 0.0, 1.5, 0.0,
    3.0, -3.0, 2.0, 1.0
  },
  {
    0.7, 0.0, 0.0, 0.0,
    0.0, 0.7, 0.0, 0.0,
    0.0, 0.0, 0.7, 0.0,
    4.0, -2.0, -1.0, 1.0
  },
  {
    1.4, 0.0, 0.0, 0.0,
    0.0, 1.4, 0.0, 0.0,
    0.0, 0.0, 1.4, 0.0,
    -5.5, 3.0, 5.0, 1.0
  },
  {
    0.4, 0.0, 0.0, 0.0,
    0.0, 0.4, 0.0, 0.0,
    0.0, 0.0, 0.4, 0.0,
    3.0, 2.0, -2.0, 1.0
  },
  {
    0.1, 0.0, 0.0, 0.0,
    0.0, 0.1, 0.0, 0.0,
    0.0, 0.0, 0.1, 0.0,
    0.0, 0.0, 0.0, 1.0
  },
  {
    0.1, 0.0, 0.0, 0.0,
    0.0, 0.1, 0.0, 0.0,
    0.0, 0.0, 0.1, 0.0,
    -2.5, -2.5, 0.0, 1.0
  },
  {
    0.2, 0.0, 0.0, 0.0,
    0.0, 0.2, 0.0, 0.0,
    0.0, 0.0, 0.2, 0.0,
    1.0, 3.0, 3.0, 1.0
  },
  {
    0.2, 0.0, 0.0, 0.0,
    0.0, 0.2, 0.0, 0.0,
    0.0, 0.0, 0.2, 0.0,
    2.5, 0.0, -1.0, 1.0
  },
  {
    0.3, 0.0, 0.0, 0.0,
    0.0, 0.3, 0.0, 0.0,
    0.0, 0.0, 0.3, 0.0,
    0.5, -4.0, 2.0, 1.0
  },
  {
    1.2, 0.0, 0.0, 0.0,
    0.0, 1.2, 0.0, 0.0,
    0.0, 0.0, 1.2, 0.0,
    -1.0, 2.0, 1.5, 1.0
  }
};

/* Handle key events. */
static void on_key(int key, int state,
    YF_UNUSED unsigned mod_mask, YF_UNUSED void *arg)
{
  if (state == YF_KEYSTATE_RELEASED)
    return;

  switch (key) {
    case YF_KEY_SPACE:
      l_vars.input.swap = 1;
      break;
    case YF_KEY_1:
      l_vars.input.speed -= 0.25f;
      break;
    case YF_KEY_2:
      l_vars.input.speed += 0.25f;
      break;
    default:
      l_vars.input.quit = 1;
  }
}

/* Updates content. */
static void update(double elapsed_time) {
  static double tm = 0.0;
  tm += elapsed_time;
  printf("update (%.6f)\n", elapsed_time);

  /* Stop view's rendering loop when requested */
  if (l_vars.input.quit)
    yf_view_stop(l_vars.view);

  /* Swap scenes after a while or when requested */
  static unsigned scn_i = 0;
  static double chg_tm = 0.0;
  chg_tm += elapsed_time;
  if (l_vars.input.swap || chg_tm >= 10.0) {
    l_vars.input.swap = 0;
    chg_tm = 0.0;
    scn_i = (scn_i+1) % YF_SCNN;
    yf_view_setscene(l_vars.view, l_vars.scn[scn_i]);
  }

  /* Rotate models */
  static YF_float ang = 0.0;
  ang += 0.01 * l_vars.input.speed;
  YF_mat4 rot;
  YF_vec3 axis = {0.7071, -0.7071, 0.0};
  yf_mat4_rot(rot, ang, axis);
  for (unsigned i = 0; i < YF_MDLN; ++i)
    yf_mat4_mul(*yf_model_getxform(l_vars.mdl[i]), l_xforms[i], rot);
}

/* Tests model rendering. */
int yf_test_model(void) {
  srand(time(NULL));
  const int instanced = rand() & 1;
  printf("## %s rendering ##\n\n", instanced ? "Instanced" : "Non-instanced");

  YF_evtfn evtfn = {.key_kb = on_key};
  yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);
  l_vars.input.quit = 0;
  l_vars.input.swap = 0;
  l_vars.input.speed = 1.0f;

  /* Create view */
  l_vars.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
  assert(l_vars.win != NULL);
  l_vars.view = yf_view_init(l_vars.win);
  assert(l_vars.view != NULL);

  /* Create scene */
  for (unsigned i = 0; i < YF_SCNN; ++i) {
    l_vars.scn[i] = yf_scene_init();
    assert(l_vars.scn[0] != NULL);
    yf_scene_setcolor(l_vars.scn[i], YF_COLOR_LIGHTGREY);
  }

  if (instanced) {
    /* Create mesh */
    l_vars.mesh[0] = yf_mesh_init(YF_FILETYPE_OBJ, "tmp/cube.obj");
    l_vars.mesh[1] = yf_mesh_init(YF_FILETYPE_OBJ, "tmp/cone.obj");
    assert(l_vars.mesh[0] != NULL);
    assert(l_vars.mesh[1] != NULL);

    /* Create texture */
    l_vars.tex[0] = yf_texture_init(YF_FILETYPE_BMP, "tmp/cube.bmp");
    l_vars.tex[1] = yf_texture_init(YF_FILETYPE_BMP, "tmp/cone.bmp");
    assert(l_vars.tex[0] != NULL);
    assert(l_vars.tex[1] != NULL);

    /* Create model, assign resources and insert into a scene */
    for (unsigned i = 0; i < YF_MDLN; ++i) {
      l_vars.mdl[i] = yf_model_init();
      assert(l_vars.mdl[i] != NULL);

      yf_model_setmesh(l_vars.mdl[i], l_vars.mesh[i % 2]);
      yf_model_settex(l_vars.mdl[i], l_vars.tex[i % 2]);

      yf_node_insert(yf_scene_getnode(l_vars.scn[YF_MIN(i, YF_SCNN-1)]),
          yf_model_getnode(l_vars.mdl[i]));
    }
  } else {
    /* Create mesh */
    for (unsigned i = 0; i < YF_MDLN; ++i) {
      if (i % 2 != 0)
        l_vars.mesh[i] = yf_mesh_init(YF_FILETYPE_OBJ, "tmp/cube.obj");
      else
        l_vars.mesh[i] = yf_mesh_init(YF_FILETYPE_OBJ, "tmp/cone.obj");
      assert(l_vars.mesh[i] != NULL);
    }

    /* Create texture */
    for (unsigned i = 0; i < YF_MDLN; ++i) {
      if (i % 2 != 0)
        l_vars.tex[i] = yf_texture_init(YF_FILETYPE_BMP, "tmp/cube.bmp");
      else
        l_vars.tex[i] = yf_texture_init(YF_FILETYPE_BMP, "tmp/cone.bmp");
      assert(l_vars.tex[i] != NULL);
    }

    /* Create model, assign resources and insert into a scene */
    for (unsigned i = 0; i < YF_MDLN; ++i) {
      l_vars.mdl[i] = yf_model_init();
      assert(l_vars.mdl[i] != NULL);

      yf_model_setmesh(l_vars.mdl[i], l_vars.mesh[i]);
      yf_model_settex(l_vars.mdl[i], l_vars.tex[i]);

      yf_node_insert(yf_scene_getnode(l_vars.scn[YF_MIN(i, YF_SCNN-1)]),
          yf_model_getnode(l_vars.mdl[i]));
    }
  }

  /* Set which scene to render */
  yf_view_setscene(l_vars.view, l_vars.scn[0]);

  /* Start the view's rendering loop */
  if (yf_view_start(l_vars.view, YF_FPS, update) != 0)
    assert(0);

  /* TODO: Deinitialization. */

  return 0;
}
