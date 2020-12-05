/*
 * YF
 * test2.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "yf-core.h"
#include "clock.h"
#include "test.h"

#undef YF_WINWID
#undef YF_WINHEI
#define YF_WINWID 600
#define YF_WINHEI 600

/* Vertex type. */
struct L_vertex {
  float pos[3];
  float tcoord[2];
};

/* Drawing data. */
const struct L_vertex l_verts[4] = {
  {{-1.0f, -1.0f, 0.5f}, {0.0f, 0.0f}},
  {{-1.0f, 1.0f, 0.5f}, {0.0f, 1.0f}},
  {{1.0f, 1.0f, 0.5}, {1.0f, 1.0f}},
  {{1.0f, -1.0f, 0.5}, {1.0f, 0.0f}}
};
const unsigned short l_inds[6] = {0, 1, 2, 0, 2, 3};
const float l_mats[3][4][4] = {
  {
    {0.35f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.35f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.35f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f}
  },
  {
    {0.2f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.2f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.2f, 0.0f},
    {-0.7f, -0.6f, 0.0f, 1.0f}
  },
  {
    {0.1f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.1f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.1f, 0.0f},
    {0.7f, 0.6f, 0.0f, 1.0f}
  }
};

/* Event-related variables. */
static int l_close_requested = 0;
static int l_resize_needed = 0;
static int l_force_resize = 0;
static YF_dim2 l_new_size = {800, 600};

/* Sets event callbacks. */
static void set_handlers(void);

/* Event callbacks. */
static void on_wd_close(YF_window win, void *data);
static void on_wd_resize(YF_window win, YF_dim2 dim, void *data);
static void on_pt_enter(YF_window win, YF_coord2 coord, void *data);
static void on_pt_leave(YF_window win, void *data);
static void on_pt_motion(YF_coord2 coord, void *data);
static void on_pt_button(int btn, int state, void *data);
static void on_kb_enter(YF_window win, void *data);
static void on_kb_leave(YF_window win, void *data);
static void on_kb_key(int key, int state, unsigned mods, void *data);

int yf_test_2(void) {
  // Context
  YF_context ctx = yf_context_init();
  assert(ctx != NULL);

  // Window
  YF_dim2 dim = {YF_WINWID, YF_WINHEI};
  YF_window win = yf_window_init(ctx, dim, "Test 2");
  assert(win != NULL);

  // Buffer
  YF_buffer buf = yf_buffer_init(ctx, 2097152);
  assert(buf != NULL);

  // Stages
  YF_modid vmod, fmod;
  if (yf_loadmod(ctx, getenv("VMOD_T2"), &vmod) != 0)
    assert(0);
  if (yf_loadmod(ctx, getenv("FMOD_T2"), &fmod) != 0)
    assert(0);
  const YF_stage stgs[2] = {
    {YF_STAGE_VERT, vmod, "main"},
    {YF_STAGE_FRAG, fmod, "main"}
  };

  // DTable
  const YF_dentry entries[2] = {
    {0, YF_DTYPE_UNIFORM, 1, NULL},
    {1, YF_DTYPE_ISAMPLER, 1, NULL}
  };
  YF_dtable dtb = yf_dtable_init(ctx, entries, 2);
  assert(dtb != NULL);
  const unsigned allocs = 3;
  if (yf_dtable_alloc(dtb, allocs) != 0)
    assert(0);

  // VInput
  const YF_vattr attrs[2] = {
    {0, YF_TYPEFMT_FLOAT3, 0},
    {1, YF_TYPEFMT_FLOAT2, offsetof(struct L_vertex, tcoord)}
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

  // Image (shader res.)
  const YF_dim3 shd_img_dim = {2, 2, 1};
  YF_image shd_img = yf_image_init(
    ctx,
    YF_PIXFMT_RGBA8UNORM,
    shd_img_dim,
    3,
    1,
    1);
  assert(shd_img != NULL);

  // Image (depth att.)
  YF_dim3 dep_img_dim = {dim.width, dim.height, 1};
  YF_image dep_img = yf_image_init(
    ctx,
    YF_PIXFMT_D16UNORM,
    dep_img_dim,
    1,
    1,
    1);
  assert(dep_img != NULL);

  // Targets
  unsigned col_att_n;
  const YF_attach *col_atts = yf_window_getatts(win, &col_att_n);
  assert(col_atts != NULL && col_att_n > 0);
  YF_attach dep_att = {dep_img, 0};
  YF_target *tgts = malloc(sizeof(YF_target) * col_att_n);
  assert(tgts != NULL);
  for (unsigned i = 0; i < col_att_n; ++i) {
    tgts[i] = yf_pass_maketarget(pass, dim, 1, col_atts+i, 1, NULL, &dep_att);
    assert(tgts[i] != NULL);
  }

  // Graphics state
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

  // Image data
  const unsigned char pixs[3][4][4] = {
    {
      {0, 255, 0, 255},
      {255, 0, 0, 255},
      {0, 0, 255, 255},
      {0, 0, 0, 255}
    },
    {
      {0, 0, 0, 255},
      {255, 255, 255, 255},
      {255, 255, 0, 255},
      {0, 0, 255, 255}
    },
    {
      {255, 255, 0, 255},
      {0, 0, 255, 255},
      {255, 0, 255, 255},
      {255, 255, 255, 255}
    }
  };

  YF_cmdbuf cb;

  // Data copy
  size_t buf_offs = 0;
  if (yf_buffer_copy(buf, buf_offs, l_mats, sizeof l_mats) != 0)
    assert(0);
  buf_offs += sizeof l_mats;
  if (yf_buffer_copy(buf, buf_offs, l_verts, sizeof l_verts) != 0)
    assert(0);
  buf_offs += sizeof l_verts;
  if (yf_buffer_copy(buf, buf_offs, l_inds, sizeof l_inds) != 0)
    assert(0);
  buf_offs += sizeof l_inds;

  ////
  // Buffer-to-buffer copy
  cb = yf_cmdbuf_begin(ctx, YF_CMDB_GRAPH);
  assert(cb != NULL);
  YF_buffer tmp_buf = yf_buffer_init(ctx, buf_offs);
  assert(tmp_buf != NULL);
  yf_cmdbuf_copybuf(cb, tmp_buf, 0, buf, 0, buf_offs);
  if (yf_cmdbuf_end(cb) != 0)
    assert(0);
  if (yf_cmdbuf_exec(ctx) != 0)
    assert(0);
  yf_buffer_deinit(buf);
  buf = tmp_buf;
  cb = NULL;
  ////

  const size_t sz = sizeof l_mats[0];
  size_t offs;
  const YF_slice elems = {0, 1};
  YF_slice lays = {0, 1};
  for (unsigned i = 0; i < allocs; ++i) {
    offs = i * sz;
    if (yf_dtable_copybuf(dtb, i, 0, elems, &buf, &offs, &sz) != 0)
      assert(0);
    if (yf_image_copy(shd_img, lays, pixs[i], sizeof pixs[0]) != 0)
      assert(0);
    if (yf_dtable_copyimg(dtb, i, 1, elems, &shd_img, &lays.i) != 0)
      assert(0);
    ++lays.i;
  }

  // Main loop
  set_handlers();
  const double tm = yf_gettime();
  int invalid_win = 0;
  int next_i = -1;
  do {
    yf_event_poll();

    if (l_close_requested) {
      invalid_win = 1;
      l_close_requested = 0;
      yf_window_close(win);
    }

    if (l_resize_needed) {
      invalid_win = 1;
      l_resize_needed = 0;
    }

    if (l_force_resize) {
      invalid_win = 1;
      l_force_resize = 0;
      yf_window_resize(win, l_new_size);
    }

    if (!invalid_win) {
      next_i = yf_window_next(win);
      invalid_win = next_i < 0;
    }

    dim = yf_window_getdim(win);
    YF_viewport vp = {0.0f, 0.0f, dim.width, dim.height, 0.0f, 1.0f};
    YF_rect sciss = {{vp.x, vp.y}, {vp.width, vp.height}};

    if (invalid_win) {
      // Window and targets must be recreated
      if (yf_window_recreate(win) != 0)
        assert(0);

      dep_img_dim.width = dim.width;
      dep_img_dim.height = dim.height;
      yf_image_deinit(dep_img);
      dep_img = yf_image_init(ctx, YF_PIXFMT_D16UNORM, dep_img_dim, 1, 1, 1);
      assert(dep_img != NULL);

      for (unsigned i = 0; i < col_att_n; ++i)
        yf_pass_unmktarget(pass, tgts[i]);
      free(tgts);
      col_atts = yf_window_getatts(win, &col_att_n);
      assert(col_atts != NULL && col_att_n > 0);
      dep_att = (YF_attach){dep_img, 0};
      tgts = malloc(sizeof(YF_target) * col_att_n);
      assert(tgts != NULL);
      for (unsigned i = 0; i < col_att_n; ++i) {
        tgts[i] = yf_pass_maketarget(
          pass,
          dim,
          1,
          col_atts+i,
          1,
          NULL,
          &dep_att);
        assert(tgts[i] != NULL);
      }
      invalid_win = 0;
      continue;
    }

    // Command buffer
    cb = yf_cmdbuf_begin(ctx, YF_CMDB_GRAPH);
    assert(cb != NULL);
    yf_cmdbuf_setgstate(cb, gst);
    yf_cmdbuf_settarget(cb, tgts[next_i]);
    yf_cmdbuf_setvport(cb, 0, &vp);
    yf_cmdbuf_setsciss(cb, 0, sciss);
    yf_cmdbuf_setvbuf(cb, 0, buf, sizeof l_mats);
    yf_cmdbuf_setibuf(cb, buf, buf_offs - sizeof l_inds, sizeof l_inds[0]);
    yf_cmdbuf_clearcolor(cb, 0, YF_COLOR_WHITE);
    yf_cmdbuf_cleardepth(cb, 1.0f);
    for (unsigned i = 0; i < allocs; ++i) {
      yf_cmdbuf_setdtable(cb, 0, i);
      yf_cmdbuf_draw(cb, 1, 0, 6, 1, 0, 0);
    }
    if (yf_cmdbuf_end(cb) != 0)
      assert(0);
    if (yf_cmdbuf_exec(ctx) != 0)
      assert(0);

    if (yf_window_present(win) != 0) {
      switch (yf_geterr()) {
        case YF_ERR_INVWIN:
          invalid_win = 1;
          break;
        default:
          assert(0);
      }
    }
  } while (yf_gettime()-tm < 2.0);

  free(tgts);
  yf_gstate_deinit(gst);
  yf_pass_deinit(pass);
  yf_dtable_deinit(dtb);
  yf_image_deinit(dep_img);
  yf_image_deinit(shd_img);
  yf_unldmod(ctx, fmod);
  yf_unldmod(ctx, vmod);
  yf_buffer_deinit(buf);
  yf_window_deinit(win);
  yf_context_deinit(ctx);
  return 0;
}

static void set_handlers(void) {
  static const YF_eventfn wd_close = {.wd_close = on_wd_close};
  static const YF_eventfn wd_resize = {.wd_resize = on_wd_resize};
  static const YF_eventfn pt_enter = {.pt_enter = on_pt_enter};
  static const YF_eventfn pt_leave = {.pt_leave = on_pt_leave};
  static const YF_eventfn pt_motion = {.pt_motion = on_pt_motion};
  static const YF_eventfn pt_button = {.pt_button = on_pt_button};
  static const YF_eventfn kb_enter = {.kb_enter = on_kb_enter};
  static const YF_eventfn kb_leave = {.kb_leave = on_kb_leave};
  static const YF_eventfn kb_key = {.kb_key = on_kb_key};
  yf_event_setfn(YF_EVENT_WD_CLOSE, wd_close, "a");
  yf_event_setfn(YF_EVENT_WD_RESIZE, wd_resize, "b");
  yf_event_setfn(YF_EVENT_PT_ENTER, pt_enter, "c");
  yf_event_setfn(YF_EVENT_PT_LEAVE, pt_leave, "d");
  yf_event_setfn(YF_EVENT_PT_MOTION, pt_motion, "e");
  yf_event_setfn(YF_EVENT_PT_BUTTON, pt_button, "f");
  yf_event_setfn(YF_EVENT_KB_ENTER, kb_enter, "g");
  yf_event_setfn(YF_EVENT_KB_LEAVE, kb_leave, "h");
  yf_event_setfn(YF_EVENT_KB_KEY, kb_key, "i");
}

static void on_wd_close(YF_window win, void *data) {
  printf("wd close: @%p (%s)\n", (void *)win, (char *)data);
  l_close_requested = 1;
}

static void on_wd_resize(YF_window win, YF_dim2 dim, void *data) {
  printf(
    "wd resize: @%p %ux%u (%s)\n",
    (void *)win, dim.width, dim.height, (char *)data);
  l_resize_needed = 1;
}

static void on_pt_enter(YF_window win, YF_coord2 coord, void *data) {
  printf(
    "pt enter: @%p %.2f, %.2f (%s)\n",
    (void *)win, coord.x, coord.y, (char *)data);
}

static void on_pt_leave(YF_window win, void *data) {
  printf("pt leave: @%p (%s)\n", (void *)win, (char *)data);
}

static void on_pt_motion(YF_coord2 coord, void *data) {
  printf("motion: %.2f, %.2f (%s)\n", coord.x, coord.y, (char *)data);
}

static void on_pt_button(int btn, int state, void *data) {
  printf("btn: %d state: %d (%s)\n", btn, state, (char *)data);
  if (btn == YF_BTN_LEFT && state == YF_BTNSTATE_PRESSED) {
    l_force_resize = 1;
    l_new_size = (YF_dim2){800, 600};
  } else if (btn == YF_BTN_RIGHT && state == YF_BTNSTATE_RELEASED) {
    l_force_resize = 1;
    l_new_size = (YF_dim2){300, 450};
  }
}

static void on_kb_enter(YF_window win, void *data) {
  printf("kb enter: @%p (%s)\n", (void *)win, (char *)data);
}

static void on_kb_leave(YF_window win, void *data) {
  printf("kb leave: @%p (%s)\n", (void *)win, (char *)data);
}

static void on_kb_key(int key, int state, unsigned mods, void *data) {
  printf("key: %d state: %d mods: %u (%s)\n", key, state, mods, (char *)data);
}
