/*
 * YF
 * test-misc.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include <yf/wsys/yf-wsys.h>
#include <yf/core/yf-core.h>

#include "yf-matrix.h"
#include "data-sfnt.h"

#define YF_WINW 484
#define YF_WINH 363
#define YF_WINT "Misc"

/* Shared variables. */
struct L_vars {
  YF_context ctx;
  YF_wsi wsi;
  YF_buffer buf;
  YF_image img;
  YF_dtable dtb;
  YF_pass pass;
  YF_target *tgts;
  YF_gstate gst;

  YF_image bitmap;

  YF_window win;
  int key;
};
static struct L_vars l_vars = {0};

/* Vertex type. */
typedef struct {
  float pos[3];
  float tc[2];
} L_vertex;

/* Key event function. */
static void key_kb(int key, int state, unsigned mod_mask, void *data) {
  if (state == YF_KEYSTATE_RELEASED)
    l_vars.key = key;
}

/* Initializes content. */
static void init(void) {
  /* Context */
  YF_context ctx = yf_context_init();
  assert(ctx != NULL);

  /* Buffer */
  YF_buffer buf = yf_buffer_init(ctx, 2048);
  assert(buf != NULL);

  /* Stages */
  YF_modid vmod, fmod;

  if (yf_loadmod(ctx, "tmp/vert", &vmod) != 0)
    assert(0);
  if (yf_loadmod(ctx, "tmp/frag", &fmod) != 0)
    assert(0);

  const YF_stage stgs[] = {
    {YF_STAGE_VERT, vmod, "main"},
    {YF_STAGE_FRAG, fmod, "main"}
  };

  /* DTable */
  const YF_dentry entries[2] = {
    {0, YF_DTYPE_UNIFORM, 1, NULL},
    {1, YF_DTYPE_ISAMPLER, 1, NULL}
  };
  YF_dtable dtb = yf_dtable_init(ctx, entries, 2);
  assert(dtb != NULL);

  const unsigned alloc_n = 1;
  if (yf_dtable_alloc(dtb, alloc_n) != 0)
    assert(0);

  /* VInput */
  const YF_vattr attrs[2] = {
    {0, YF_TYPEFMT_FLOAT3, 0},
    {1, YF_TYPEFMT_FLOAT2, offsetof(L_vertex, tc)}
  };
  const YF_vinput vin = {attrs, 2, sizeof(L_vertex), YF_VRATE_VERT};

  /* Wsi */
  YF_window win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
  assert(win != NULL);

  YF_wsi wsi = yf_wsi_init(ctx, win);
  assert(wsi != NULL);

  unsigned pres_img_n;
  const YF_image *pres_imgs = yf_wsi_getimages(wsi, &pres_img_n);
  assert(pres_imgs != NULL && pres_img_n != 0);

  /* Image */
  const YF_dim3 img_dim = {YF_WINW, YF_WINH, 1};
  YF_image img = yf_image_init(ctx, YF_PIXFMT_D16UNORM, img_dim, 1, 1, 1);
  assert(img != NULL);

  /* Pass */
  int pres_fmt;
  unsigned pres_spl;
  yf_image_getval(pres_imgs[0], &pres_fmt, NULL, NULL, NULL, &pres_spl);

  const YF_colordsc clr_dsc = {
    pres_fmt,
    pres_spl,
    YF_LOADOP_UNDEF,
    YF_STOREOP_UNDEF
  };

  const YF_depthdsc dep_dsc = {
    YF_PIXFMT_D16UNORM,
    1,
    YF_LOADOP_UNDEF,
    YF_STOREOP_UNDEF,
    YF_LOADOP_UNDEF,
    YF_STOREOP_UNDEF
  };

  YF_pass pass = yf_pass_init(ctx, &clr_dsc, 1, NULL, &dep_dsc);
  assert(pass != NULL);

  /* Targets */
  const YF_dim2 tgt_dim = {YF_WINW, YF_WINH};
  const YF_attach dep_att = {img, 0};

  YF_attach *clr_atts = malloc(pres_img_n * sizeof(YF_attach));
  YF_target *tgts = malloc(pres_img_n * sizeof(YF_target));
  assert(clr_atts != NULL && tgts != NULL);

  for (size_t i = 0; i < pres_img_n; ++i) {
    clr_atts[i] = (YF_attach){pres_imgs[i], 0};
    tgts[i] = yf_pass_maketarget(pass, tgt_dim, 1, clr_atts+i, 1, NULL,
        &dep_att);
    assert(tgts[i] != NULL);
  }
  free(clr_atts);

  /* Graphics state */
  const YF_gconf conf = {
    pass,
    stgs,
    sizeof stgs / sizeof stgs[0],
    &dtb,
    1,
    &vin,
    1,
    YF_PRIMITIVE_TRIANGLE,
    YF_POLYMODE_FILL,
    YF_CULLMODE_NONE,
    YF_WINDING_CCW
  };

  YF_gstate gst = yf_gstate_init(ctx, &conf);
  assert(gst != NULL);

  /* Bitmap */
/*
  YF_fontdt fdt;
  assert(yf_loadsfnt("tmp/font.ttf", &fdt) == 0);
*/
  YF_font font = yf_font_init(YF_FILETYPE_TTF, "tmp/font.ttf");
  assert(font != NULL);
  YF_fontdt fdt = *((YF_fontdt *)font);
  YF_glyph glyph;
  if (fdt.glyph(fdt.font, L'?'/*L'ń'*/, 16, 72, &glyph) != 0)
    assert(0);
  assert(glyph.bpp == 8);

  const YF_off3 bitmap_off = {0};
  const void *bitmap_dt = glyph.bitmap.u8;
  const YF_dim3 bitmap_dim = {glyph.width, glyph.height, 1};

  YF_image bitmap = yf_image_init(ctx, YF_PIXFMT_R8UNORM, bitmap_dim, 1, 1, 1);
  assert(bitmap != NULL);

  /* Data copy */
  YF_mat4 m, s, vp, v, p;
  const YF_vec3 eye = {0.0, 0.0, -3.0}, center = {0}, up = {0.0, -1.0, 0.0};

  yf_mat4_persp(p, 0.79, (YF_float)YF_WINW / (YF_float)YF_WINH, 0.1, 100.0);
  yf_mat4_lookat(v, eye, center, up);
  yf_mat4_mul(vp, p, v);

  YF_float ratio = (YF_float)bitmap_dim.width / (YF_float)bitmap_dim.height;
  if (ratio > 1.0)
    yf_mat4_scale(s, 1.0, 1.0/ratio, 1.0);
  else
    yf_mat4_scale(s, ratio, 1.0, 1.0);

  yf_mat4_mul(m, vp, s);

  const L_vertex verts[4] = {
    {{-1.0f, -1.0f, 0.5f}, {0.0f, 1.0f}},
    {{-1.0f,  1.0f, 0.5f}, {0.0f, 0.0f}},
    {{ 1.0f,  1.0f, 0.5f}, {1.0f, 0.0f}},
    {{ 1.0f, -1.0f, 0.5f}, {1.0f, 1.0f}}
  };

  const unsigned short inds[6] = {0, 1, 2, 0, 2, 3};

  if (yf_buffer_copy(buf, 0, m, sizeof m) != 0)
    assert(0);
  if (yf_buffer_copy(buf, sizeof m, verts, sizeof verts) != 0)
    assert(0);
  if (yf_buffer_copy(buf, sizeof m + sizeof verts, inds, sizeof inds) != 0)
    assert(0);

  const YF_slice elems = {0, 1};
  const size_t buf_off = 0;
  const size_t buf_sz = sizeof m;

  if (yf_dtable_copybuf(dtb, 0, 0, elems, &buf, &buf_off, &buf_sz) != 0)
    assert(0);

  if (yf_image_copy(bitmap, bitmap_off, bitmap_dim, 0, 0, bitmap_dt) != 0)
    assert(0);

  const unsigned layer = 0;
  if (yf_dtable_copyimg(dtb, 0, 1, elems, &bitmap, &layer) != 0)
    assert(0);

  l_vars.ctx = ctx;
  l_vars.wsi = wsi;
  l_vars.buf = buf;
  l_vars.img = img;
  l_vars.dtb = dtb;
  l_vars.pass = pass;
  l_vars.tgts = tgts;
  l_vars.gst = gst;

  l_vars.bitmap = bitmap;

  l_vars.win = win;
  l_vars.key = YF_KEY_UNKNOWN;

  YF_evtfn fn = {.key_kb = key_kb};
  yf_setevtfn(YF_EVT_KEYKB, fn, NULL);
}

/* Updates content. */
static void update(void) {
  /* Event polling */
  yf_pollevt(YF_EVT_KEYKB);

  /* Command buffer */
  static const YF_viewport vp = {0.0f, 0.0f, YF_WINW, YF_WINH, 0.0f, 1.0f};
  static const YF_rect sciss = {{0, 0}, {YF_WINW, YF_WINH}};

  int tgt_i = yf_wsi_next(l_vars.wsi, 0);
  assert(tgt_i >= 0);

  YF_cmdbuf cb = yf_cmdbuf_get(l_vars.ctx, YF_CMDBUF_GRAPH);
  assert(cb != NULL);

  yf_cmdbuf_setgstate(cb, l_vars.gst);
  yf_cmdbuf_settarget(cb, l_vars.tgts[tgt_i]);
  yf_cmdbuf_setvport(cb, 0, &vp);
  yf_cmdbuf_setsciss(cb, 0, sciss);
  yf_cmdbuf_setdtable(cb, 0, 0);
  yf_cmdbuf_setvbuf(cb, 0, l_vars.buf, sizeof(float[16]));
  yf_cmdbuf_setibuf(cb, l_vars.buf, sizeof(YF_mat4)+sizeof(L_vertex[4]), 2);
  yf_cmdbuf_clearcolor(cb, 0, YF_COLOR_WHITE);
  yf_cmdbuf_cleardepth(cb, 1.0f);
  yf_cmdbuf_draw(cb, 1, 0, 6, 1, 0, 0);

  if (yf_cmdbuf_end(cb) != 0)
    assert(0);
  if (yf_cmdbuf_exec(l_vars.ctx) != 0)
    assert(0);
  if (yf_wsi_present(l_vars.wsi, tgt_i) != 0)
    assert(0);
}

/* Runs the main loop. */
static int run(void) {
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
/*
    printf("\n[time] %lds %ldns (%.0f fps)", now.tv_sec, now.tv_nsec,
        (double)999999999 / (double)dt);
*/
  } while (l_vars.key == YF_KEY_UNKNOWN);

  /* TODO: Deinitialization. */

  return 0;
}

/* Called by the main test. */
int yf_test_misc(void) {
  init();
  return run();
}
