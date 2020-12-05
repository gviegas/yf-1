/*
 * YF
 * test8.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include <yf/core/yf-core.h>
#include "yf-ext.h"

#include "coreobj.h"
#include "filetype-otf.h"
#include "test.h"

#undef YF_WINWID
#undef YF_WINHEI
#define YF_WINWID 720
#define YF_WINHEI 540

#define YF_ZOOMFAC 0.1
#define YF_MOVEFAC 0.2
#define YF_TURNFAC 0.05
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
  YF_label labl;
  int got_input;
  struct {
    int q, e, w, s, a, d, r, f, up, down, left, right, space, esc;
  } keys_pressed;
};
static struct L_vars l_vars = {0};

/* Vertices. */
static YF_vec3 *l_verts = NULL;
static unsigned l_vert_n = 0;
static size_t l_vert_sz = 0;

/* Indices. */
static unsigned short *l_inds = NULL;
static unsigned l_ind_n = 0;
static size_t l_ind_sz = 0;

/* Initializes content. */
static void init(void);

/* Updates content. */
static void update(void);

/* Handles key events. */
static void on_key(int key, int state, unsigned mod_mask, void *data);

/* Runs the main loop. */
static void run(void);

int yf_test_8(void) {

//////
  yf_filetype_otf_load(getenv("FONTTTF"));

  YF_coord2 *trace_glyph(char, uint16_t *);
  uint16_t coord_n;
  YF_coord2 *coords = trace_glyph('n', &coord_n);
  assert(coords != NULL);

  l_vert_n = coord_n;
  l_vert_sz = l_vert_n * sizeof *l_verts;
  l_verts = malloc(l_vert_sz);
  assert(l_verts != NULL);
  l_ind_n = l_vert_n * 2;
  l_ind_sz = l_ind_n * sizeof *l_inds;
  l_inds = malloc(l_ind_sz);
  assert(l_inds != NULL);

  for (unsigned i = 0; i < l_vert_n; ++i) {
    l_verts[i][0] = coords[i].x / 2048.0;
    l_verts[i][1] = coords[i].y / 2048.0;
    l_verts[i][2] = 0.5;
  }
  for (unsigned i = 0; i < l_vert_n; ++i) {
    l_inds[i*2] = i;
    l_inds[i*2+1] = (i+1) % l_vert_n;
  }
//////

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
  YF_window win = yf_window_init(ctx, win_dim, "Test 8");
  assert(win != NULL);

  // Buffer
  YF_buffer buf = yf_buffer_init(ctx, 32768);
  assert(buf != NULL);

  // Stages
  YF_modid vmod, fmod;
  if (yf_loadmod(ctx, getenv("VMOD_T8"), &vmod) != 0)
    assert(0);
  if (yf_loadmod(ctx, getenv("FMOD_T8"), &fmod) != 0)
    assert(0);
  YF_stage stgs[2] = {
    {YF_STAGE_VERT, vmod, "main"},
    {YF_STAGE_FRAG, fmod, "main"}
  };

  // DTable
  YF_dentry entry = {0, YF_DTYPE_UNIFORM, 1, NULL};
  YF_dtable dtb = yf_dtable_init(ctx, &entry, 1);
  assert(dtb);
  if (yf_dtable_alloc(dtb, 1) != 0)
    assert(0);

  // VInput
  YF_vattr attr = {0, YF_TYPEFMT_FLOAT3, 0};
  YF_vinput vin = {&attr, 1, sizeof(YF_vec3), YF_VRATE_VERT};

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
    YF_PRIMITIVE_LINE,
    YF_POLYMODE_FILL,
    YF_CULLMODE_NONE,
    YF_FRONTFACE_CCW
  };
  YF_gstate gst = yf_gstate_init(ctx, &conf);
  assert(gst != NULL);

  // Buffer & shader data copy
  YF_vec3 cam_orig = {0.0, 0.0, -10.0};
  YF_vec3 cam_tgt = {0.0, 0.0, 0.0};
  YF_float cam_asp = (YF_float)YF_WINWID / (YF_float)YF_WINHEI;
  YF_camera cam = yf_camera_init(cam_orig, cam_tgt, cam_asp);
  YF_mat4 flip;
  yf_mat4_iden(flip);
  flip[5] = -1.0;
  YF_mat4 m;
  yf_mat4_mul(m, *yf_camera_getxform(cam), flip);
  if (yf_buffer_copy(buf, 0, m, sizeof m) != 0)
    assert(0);
  if (yf_buffer_copy(buf, sizeof m, l_verts, l_vert_sz) != 0)
    assert(0);
  if (yf_buffer_copy(buf, sizeof m + l_vert_sz, l_inds, l_ind_sz) != 0)
    assert(0);
  const YF_slice elems = {0, 1};
  const size_t buf_offs = 0;
  const size_t buf_sz = sizeof m;
  if ((yf_dtable_copybuf(dtb, 0, 0, elems, &buf, &buf_offs, &buf_sz)) != 0)
    assert(0);

  // Label object
  YF_label labl = yf_label_init();
  assert(labl != NULL);

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
  l_vars.labl = labl;
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
    YF_mat4 flip;
    yf_mat4_iden(flip);
    flip[5] = -1.0;
    YF_mat4 m;
    yf_mat4_mul(m, *yf_camera_getxform(l_vars.cam), flip);
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
  yf_cmdbuf_setvbuf(cb, 0, l_vars.buf, sizeof(YF_mat4));
  yf_cmdbuf_setibuf(cb, l_vars.buf, sizeof(YF_mat4)+l_vert_sz, sizeof *l_inds);
  yf_cmdbuf_clearcolor(cb, 0, YF_COLOR_DARKGREY);
  yf_cmdbuf_cleardepth(cb, 1.0f);
  yf_cmdbuf_draw(cb, 1, 0, l_ind_n, 1, 0, 0);
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
