/*
 * YF
 * test5.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include <yf/core/yf-core.h>
#include "yf-ext.h"

#include "test.h"
#include "coreobj.h"
#include "vertex.h"
#include "mesh.h"
#include "texture.h"

#define YF_WINWID 960
#define YF_WINHEI 600

#define YF_GRIDWID 48
#define YF_GRIDDEP 48

#define YF_ZOOMFAC 0.1
#define YF_MOVEFAC 0.225
#define YF_TURNFAC 0.065
#define YF_TGTVEC (YF_vec3){0.0, 0.0, 1.0}
#define YF_ORIGVEC (YF_vec3){0.0, 0.0, 0.0}

/* Local variables. */
struct L_vars {
  YF_context ctx;
  YF_window win;
  YF_buffer buf;
  YF_modid vmod;
  YF_modid fmod;
  YF_dtable dtb;
  YF_pass pass;
  YF_image img;
  YF_target *tgts;
  YF_gstate gst;
  YF_camera cam;
  YF_mesh mesh;
  int got_input;
  struct {
    int q, e, w, s, a, d, r, f, up, down, left, right, space, esc;
  } keys_pressed;
};
static struct L_vars l_vars = {0};

/* Initializes content. */
static void init(void);

/* Updates content. */
static void update(void);

/* Handles key events. */
static void on_key(int key, int state, unsigned mod_mask, void *data);

/* Runs the main loop. */
static void run(void);

int yf_test_5(void) {
  init();
  run();
  return 0;
}

static void init(void) {
  // Context
  YF_context ctx = yf_getctx();
  assert(ctx != NULL);

  // Window
  const YF_dim2 win_dim = {YF_WINWID, YF_WINHEI};
  YF_window win = yf_window_init(ctx, win_dim, "Test 5");
  assert(win != NULL);

  // Buffer
  YF_buffer buf = yf_buffer_init(ctx, 4096);
  assert(buf != NULL);

  // Stages
  YF_modid vmod, fmod;
  if (yf_loadmod(ctx, getenv("VMOD_T5"), &vmod) != 0)
    assert(0);
  if (yf_loadmod(ctx, getenv("FMOD_T5"), &fmod) != 0)
    assert(0);
  YF_stage stgs[2] = {
    {YF_STAGE_VERT, vmod, "main"},
    {YF_STAGE_FRAG, fmod, "main"}
  };

  // DTable
  YF_dentry entries[3] = {
    {0, YF_DTYPE_UNIFORM, 1, NULL},
    {3, YF_DTYPE_ISAMPLER, 1, NULL},
    {2, YF_DTYPE_ISAMPLER, 1, NULL}
  };
  YF_dtable dtb = yf_dtable_init(ctx, entries, 3);
  assert(dtb);
  if (yf_dtable_alloc(dtb, 1) != 0)
    assert(0);

  // VInput
  YF_vattr attrs[3] = {
    {0, YF_TYPEFMT_FLOAT3, 0},
    {1, YF_TYPEFMT_FLOAT2, offsetof(YF_vterr, tc)},
    {2, YF_TYPEFMT_FLOAT3, offsetof(YF_vterr, norm)}
  };
  YF_vinput vin = {attrs, 3, sizeof(YF_vterr), YF_VRATE_VERT};

  // Pass
  const YF_colordsc *col_dsc = yf_window_getdsc(win);
  assert(col_dsc != NULL);
  const YF_depthdsc dep_dsc = {
    YF_PIXFMT_D16UNORM,
    1,
    YF_LOADOP_UNDEF,
    YF_STOREOP_UNDEF,
    YF_LOADOP_UNDEF,
    YF_STOREOP_UNDEF
  };
  YF_pass pass = yf_pass_init(ctx, col_dsc, 1, NULL, &dep_dsc);
  assert(pass != NULL);

  // Image (depth)
  const YF_dim3 img_dim = {win_dim.width, win_dim.height, 1};
  YF_image img = yf_image_init(ctx, YF_PIXFMT_D16UNORM, img_dim, 1, 1, 1);
  assert(img != NULL);

  // Targets
  unsigned col_att_n;
  const YF_attach *col_atts = yf_window_getatts(win, &col_att_n);
  assert(col_atts != NULL && col_att_n > 0);
  const YF_attach dep_att = {img, 0};
  YF_target *tgts = malloc(sizeof(YF_target) * col_att_n);
  assert(tgts != NULL);
  for (unsigned i = 0; i < col_att_n; ++i) {
    tgts[i] = yf_pass_maketarget(
      pass,
      win_dim,
      1,
      col_atts+i,
      1,
      NULL,
      &dep_att);
    assert(tgts[i] != NULL);
  }

  // Graph. state
  YF_gconf conf = {
    pass,
    stgs,
    2,
    &dtb,
    1,
    &vin,
    1,
    YF_PRIMITIVE_TRIANGLE,
    YF_POLYMODE_FILL,//LINE,
    YF_CULLMODE_BACK,
    YF_FRONTFACE_CCW
  };
  YF_gstate gst = yf_gstate_init(ctx, &conf);
  assert(gst != NULL);

  // Buffer & shader data copy
  YF_vec3 cam_orig = {0.0, -2.0, -3.0};
  YF_vec3 cam_tgt = {0.0, 0.0, 0.0};
  YF_float cam_asp = (YF_float)YF_WINWID / (YF_float)YF_WINHEI;
  YF_camera cam = yf_camera_init(cam_orig, cam_tgt, cam_asp);
  YF_mat4 m;
  yf_mat4_copy(m, *yf_camera_getxform(cam));
  if (yf_buffer_copy(buf, 0, m, sizeof m) != 0)
    assert(0);
  const YF_slice elems = {0, 1};
  const size_t buf_offs = 0;
  const size_t buf_sz = sizeof m;
  if (yf_dtable_copybuf(dtb, 0, 0, elems, &buf, &buf_offs, &buf_sz) != 0)
    assert(0);

  // Terrain object
  YF_terrain terr = yf_terrain_init(YF_GRIDWID, YF_GRIDDEP);
  assert(terr != NULL);
  YF_mesh mesh = yf_terrain_getmesh(terr);
  assert(mesh != NULL);
  YF_texture hmap = yf_texture_init(YF_FILETYPE_BMP, getenv("HMAP"));
  assert(hmap != NULL);
  yf_terrain_sethmap(terr, hmap);
  if (yf_texture_copyres(yf_terrain_gethmap(terr), dtb, 0, 3, 0) != 0)
    assert(0);
  YF_texture tex = yf_texture_init(YF_FILETYPE_BMP, getenv("TERRAIN"));
  assert(tex != NULL);
  yf_terrain_settex(terr, tex);
  if (yf_texture_copyres(yf_terrain_gettex(terr), dtb, 0, 2, 0) != 0)
    assert(0);

  // Key events handler
  const YF_eventfn kb_key = {.kb_key = on_key};
  yf_event_setfn(YF_EVENT_KB_KEY, kb_key, NULL);

  l_vars.ctx = ctx;
  l_vars.win = win;
  l_vars.buf = buf;
  l_vars.vmod = vmod;
  l_vars.fmod = fmod;
  l_vars.dtb = dtb;
  l_vars.pass = pass;
  l_vars.img = img;
  l_vars.tgts = tgts;
  l_vars.gst = gst;
  l_vars.cam = cam;
  l_vars.mesh = mesh;
}

static void update(void) {
  // Event polling
  yf_event_poll();

  if (l_vars.got_input) {
    if (l_vars.keys_pressed.q)
      yf_camera_zoomo(l_vars.cam, YF_ZOOMFAC);
    if (l_vars.keys_pressed.e)
      yf_camera_zoomi(l_vars.cam, YF_ZOOMFAC);
    if (l_vars.keys_pressed.w)
      yf_camera_movef(l_vars.cam, YF_MOVEFAC);
    if (l_vars.keys_pressed.s)
      yf_camera_moveb(l_vars.cam, YF_MOVEFAC);
    if (l_vars.keys_pressed.a)
      yf_camera_movel(l_vars.cam, YF_MOVEFAC);
    if (l_vars.keys_pressed.d)
      yf_camera_mover(l_vars.cam, YF_MOVEFAC);
    if (l_vars.keys_pressed.r)
      yf_camera_moveu(l_vars.cam, YF_MOVEFAC);
    if (l_vars.keys_pressed.f)
      yf_camera_moved(l_vars.cam, YF_MOVEFAC);
    if (l_vars.keys_pressed.up)
      yf_camera_turnu(l_vars.cam, YF_TURNFAC);
    if (l_vars.keys_pressed.down)
      yf_camera_turnd(l_vars.cam, YF_TURNFAC);
    if (l_vars.keys_pressed.left)
      yf_camera_turnl(l_vars.cam, YF_TURNFAC);
    if (l_vars.keys_pressed.right)
      yf_camera_turnr(l_vars.cam, YF_TURNFAC);
    if (l_vars.keys_pressed.space)
      yf_camera_point(l_vars.cam, YF_TGTVEC);
    if (l_vars.keys_pressed.esc)
      yf_camera_place(l_vars.cam, YF_ORIGVEC);

    // Buffer & shader data update
    YF_mat4 m;
    yf_mat4_copy(m, *yf_camera_getxform(l_vars.cam));
    if (yf_buffer_copy(l_vars.buf, 0, m, sizeof m) != 0)
      assert(0);
    const YF_slice elems = {0, 1};
    const size_t buf_offs = 0;
    const size_t buf_sz = sizeof m;
    if (yf_dtable_copybuf(
      l_vars.dtb, 0, 0, elems, &l_vars.buf, &buf_offs, &buf_sz) != 0)
    {
      assert(0);
    }
  }

  // Command buffer
  static const YF_viewport vp = {0.0f, 0.0f, YF_WINWID, YF_WINHEI, 0.0f, 1.0f};
  static const YF_rect sciss = {{0, 0}, {YF_WINWID, YF_WINHEI}};
  int next_tgt_i = yf_window_next(l_vars.win);
  assert(next_tgt_i >= 0);

  YF_cmdbuf cb = yf_cmdbuf_begin(l_vars.ctx, YF_CMDB_GRAPH);
  assert(cb != NULL);
  yf_cmdbuf_setgstate(cb, l_vars.gst);
  yf_cmdbuf_settarget(cb, l_vars.tgts[next_tgt_i]);
  yf_cmdbuf_setvport(cb, 0, &vp);
  yf_cmdbuf_setsciss(cb, 0, sciss);
  yf_cmdbuf_setdtable(cb, 0, 0);
  yf_cmdbuf_clearcolor(cb, 0, YF_COLOR_DARKGREY);
  yf_cmdbuf_cleardepth(cb, 1.0f);
  yf_mesh_draw(l_vars.mesh, cb, 1, 0);
  if (yf_cmdbuf_end(cb) != 0)
    assert(0);
  if (yf_cmdbuf_exec(l_vars.ctx) != 0)
    assert(0);
  if (yf_window_present(l_vars.win) != 0)
    assert(0);
}

static void on_key(int key, int state, unsigned mod_mask, void *data) {
  switch (key) {
    case YF_KEY_Q:
      l_vars.keys_pressed.q = state == YF_KEYSTATE_PRESSED;
      break;
    case YF_KEY_E:
      l_vars.keys_pressed.e = state == YF_KEYSTATE_PRESSED;
      break;
    case YF_KEY_W:
      l_vars.keys_pressed.w = state == YF_KEYSTATE_PRESSED;
      break;
    case YF_KEY_S:
      l_vars.keys_pressed.s = state == YF_KEYSTATE_PRESSED;
      break;
    case YF_KEY_A:
      l_vars.keys_pressed.a = state == YF_KEYSTATE_PRESSED;
      break;
    case YF_KEY_D:
      l_vars.keys_pressed.d = state == YF_KEYSTATE_PRESSED;
      break;
    case YF_KEY_R:
      l_vars.keys_pressed.r = state == YF_KEYSTATE_PRESSED;
      break;
    case YF_KEY_F:
      l_vars.keys_pressed.f = state == YF_KEYSTATE_PRESSED;
      break;
    case YF_KEY_UP:
      l_vars.keys_pressed.up = state == YF_KEYSTATE_PRESSED;
      break;
    case YF_KEY_DOWN:
      l_vars.keys_pressed.down = state == YF_KEYSTATE_PRESSED;
      break;
    case YF_KEY_LEFT:
      l_vars.keys_pressed.left = state == YF_KEYSTATE_PRESSED;
      break;
    case YF_KEY_RIGHT:
      l_vars.keys_pressed.right = state == YF_KEYSTATE_PRESSED;
      break;
    case YF_KEY_SPACE:
      l_vars.keys_pressed.space = state == YF_KEYSTATE_PRESSED;
      break;
    case YF_KEY_ESC:
      l_vars.keys_pressed.esc = state == YF_KEYSTATE_PRESSED;
      break;
    default:
      return;
  }
  state == YF_KEYSTATE_RELEASED ? l_vars.got_input++ : l_vars.got_input--;
}

static void run(void) {
  const long frame_tm = 1.0 / 60.0 * 1000000000.0;
  long dt;
  time_t sec;
  struct timespec before, now, idle;
  clock_gettime(CLOCK_MONOTONIC, &before);
  do {
    update();
    clock_gettime(CLOCK_MONOTONIC, &now);
    sec = now.tv_sec - before.tv_sec;
    dt = sec > 0 ? ((long)999999999 * sec) : 0;
    dt = dt - before.tv_nsec + now.tv_nsec;
    if (dt < frame_tm) {
      idle.tv_sec = 0;
      idle.tv_nsec = frame_tm - dt;
      clock_nanosleep(CLOCK_MONOTONIC, 0, &idle, NULL);
      clock_gettime(CLOCK_MONOTONIC, &now);
      dt = frame_tm;
    }
    before = now;
#ifdef YF_SHOWFPS
    printf(
      "[clock] %lds %ldns (%.0f fps)\n",
      now.tv_sec,
      now.tv_nsec,
      (double)999999999 / (double)dt);
#endif
  } while (1);
}
