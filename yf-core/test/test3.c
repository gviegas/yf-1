/*
 * YF
 * test3.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "yf-core.h"
#include "test.h"

#undef YF_WINWID
#undef YF_WINHEI
#define YF_WINWID 480
#define YF_WINHEI 384

/* Local variables. */
struct L_vars {
  YF_context ctx;
  YF_window win;
  YF_buffer buf;
  YF_dtable dtb;
  YF_pass pass;
  YF_image img;
  YF_target *tgts;
  YF_gstate gst;
};
static struct L_vars l_vars = {0};

/* Vertex. */
struct L_vertex {
  float pos[3];
  float col[4];
};

/* Initializes content. */
static void init(void);

/* Updates content. */
static void update(void);

/* Runs the main loop. */
static void run(void);

int yf_test_3(void) {
  init();
  run();
  return 0;
}

static void init(void) {
  // Context
  YF_context ctx = yf_context_init();
  assert(ctx != NULL);

  // Window
  const YF_dim2 win_dim = {YF_WINWID, YF_WINHEI};
  YF_window win = yf_window_init(ctx, win_dim, "Test 3");
  assert(win != NULL);

  // Buffer
  YF_buffer buf = yf_buffer_init(ctx, 2048);
  assert(buf != NULL);

  // Stages
  YF_modid vmod, fmod;
  if (yf_loadmod(ctx, getenv("VMOD_T3"), &vmod) != 0)
    assert(0);
  if (yf_loadmod(ctx, getenv("FMOD_T3"), &fmod) != 0)
    assert(0);
  const YF_stage stgs[2] = {
    {YF_STAGE_VERT, vmod, "main"},
    {YF_STAGE_FRAG, fmod, "main"}
  };

  // DTable
  const YF_dentry entry = {0, YF_DTYPE_UNIFORM, 1, NULL};
  YF_dtable dtb = yf_dtable_init(ctx, &entry, 1);
  assert(dtb != NULL);
  const unsigned allocs = 1;
  if (yf_dtable_alloc(dtb, allocs) != 0)
    assert(0);

  // VInput
  const YF_vattr attrs[2] = {
    {0, YF_TYPEFMT_FLOAT3, 0},
    {1, YF_TYPEFMT_FLOAT4, offsetof(struct L_vertex, col)}
  };
  const YF_vinput vin = {attrs, 2, sizeof(struct L_vertex), YF_VRATE_VERT};

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
  const YF_gconf conf = {
    pass,
    stgs,
    2,
    &dtb,
    1,
    &vin,
    1,
    YF_PRIMITIVE_TRIANGLE,
    YF_POLYMODE_FILL,
    YF_CULLMODE_NONE,
    YF_FRONTFACE_CCW
  };
  YF_gstate gst = yf_gstate_init(ctx, &conf);
  assert(gst != NULL);

  // Buffer & dtable data copy
  const float m[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };
  const struct L_vertex verts[3] = {
    {{-1.0f,  1.0f, 0.5f}, {1.0f, 1.0f, 0.0f, 1.0f}},
    {{ 1.0f,  1.0f, 0.5f}, {0.0f, 1.0f, 1.0f, 1.0f}},
    {{ 0.0f, -1.0f, 0.5f}, {1.0f, 0.0f, 1.0f, 1.0f}}
  };
  if (yf_buffer_copy(buf, 0, m, sizeof m) != 0)
    assert(0);
  if (yf_buffer_copy(buf, sizeof m, verts, sizeof verts) != 0)
    assert(0);
  const YF_slice elems = {0, 1};
  const size_t buf_offs = 0;
  const size_t buf_sz = sizeof m;
  if ((yf_dtable_copybuf(dtb, 0, 0, elems, &buf, &buf_offs, &buf_sz)) != 0)
    assert(0);

  l_vars.ctx = ctx;
  l_vars.win = win;
  l_vars.buf = buf;
  l_vars.dtb = dtb;
  l_vars.pass = pass;
  l_vars.img = img;
  l_vars.tgts = tgts;
  l_vars.gst = gst;
}

static void update(void) {
  // Event polling
  yf_event_poll();

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
  yf_cmdbuf_setvbuf(cb, 0, l_vars.buf, sizeof(float[16]));
  yf_cmdbuf_clearcolor(cb, 0, YF_COLOR_BLACK);
  yf_cmdbuf_cleardepth(cb, 1.0f);
  yf_cmdbuf_draw(cb, 0, 0, 3, 1, 0, 0);
  if (yf_cmdbuf_end(cb) != 0)
    assert(0);
  if (yf_cmdbuf_exec(l_vars.ctx) != 0)
    assert(0);
  if (yf_window_present(l_vars.win) != 0)
    assert(0);
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
    printf(
      "[clock] %lds %ldns (%.0f fps)\n",
      now.tv_sec,
      now.tv_nsec,
      (double)999999999 / (double)dt);
  } while (1);
}
