/*
 * YF
 * view.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include <yf/com/yf-error.h>
#include <yf/core/yf-window.h>
#include <yf/core/yf-image.h>
#include <yf/core/yf-event.h>

#include "view.h"
#include "scene.h"
#include "coreobj.h"
#include "clock.h"

struct YF_view_o {
  YF_context ctx;
  YF_window win;
  YF_image depth_img;
  YF_pass pass;
  YF_target *tgts;
  unsigned tgt_n;
  YF_scene scn;
  int started;
};

/* Event callbacks. */
typedef struct {
  struct {
    void (*fn)(int btn, int state, void *data);
    void *data;
  } button;
  struct {
    void (*fn)(YF_coord2 coord, void *data);
    void *data;
  } motion;
  struct {
    void (*fn)(int key, int state, unsigned mod_mask, void *data);
    void *data;
  } key;
  struct {
    void (*fn)(int in, void *data);
    void *data;
  } focus;
} L_callb;

/* Global pass instance. */
YF_pass yf_g_pass = NULL;

/* Callback instance. */
static L_callb l_callb = {0};

/* Sets functions for event catching. */
static void set_eventfn(void);

/* Catches events & dispatches to installed handlers. */
static void catch_wd_resize(YF_window win, YF_dim2 dim, void *data);
static void catch_wd_close(YF_window win, void *data);
static void catch_pt_enter(YF_window win, YF_coord2 coord, void *data);
static void catch_pt_leave(YF_window win, void *data);
static void catch_pt_motion(YF_coord2 coord, void *data);
static void catch_pt_button(int btn, int state, void *data);
static void catch_kb_enter(YF_window win, void *data);
static void catch_kb_leave(YF_window win, void *data);
static void catch_kb_key(int key, int state, unsigned mod_mask, void *data);

YF_view yf_view_init(YF_dim2 dim, const char *name) {
  /* TODO: Support for multi-view events. */
  set_eventfn();

  YF_view view = calloc(1, sizeof(struct YF_view_o));
  if (view == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  if ((view->ctx = yf_getctx()) == NULL) {
    yf_view_deinit(view);
    return NULL;
  }
  if ((view->win = yf_window_init(view->ctx, dim, name)) == NULL) {
    yf_view_deinit(view);
    return NULL;
  }

  const YF_dim2 wdim = yf_window_getdim(view->win);
  const YF_dim3 ddim = {wdim.width, wdim.height, 1};
  view->depth_img = yf_image_init(view->ctx, YF_PIXFMT_D16UNORM, ddim, 1, 1, 1);
  if (view->depth_img == NULL) {
    yf_view_deinit(view);
    return NULL;
  }

  if (yf_g_pass == NULL) {
    const YF_colordsc *cdsc = yf_window_getdsc(view->win);
    const YF_depthdsc ddsc = {
      .pixfmt = YF_PIXFMT_D16UNORM,
      .samples = 1,
      .depth_loadop = YF_LOADOP_UNDEF,
      .depth_storeop = YF_STOREOP_UNDEF,
      .stencil_loadop = YF_LOADOP_UNDEF,
      .stencil_storeop = YF_STOREOP_UNDEF
    };
    if ((view->pass = yf_pass_init(view->ctx, cdsc, 1, NULL, &ddsc)) == NULL) {
      yf_view_deinit(view);
      return NULL;
    }
    yf_g_pass = view->pass;
  } else {
    /* TODO: Make sure that the global pass is compatible with this view. */
    view->pass = yf_g_pass;
  }

  const YF_attach *catts = yf_window_getatts(view->win, &view->tgt_n);
  const YF_attach datt = {
    .img = view->depth_img,
    .layer_base = 0
  };
  if ((view->tgts = calloc(view->tgt_n, sizeof *view->tgts)) == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    yf_view_deinit(view);
    return NULL;
  }
  for (unsigned i = 0; i < view->tgt_n; ++i) {
    view->tgts[i] = yf_pass_maketarget(
      view->pass,
      wdim,
      1,
      catts+i,
      1,
      NULL,
      &datt);
    if (view->tgts[i] == NULL) {
      yf_view_deinit(view);
      return NULL;
    }
  }

  return view;
}

void yf_view_setscene(YF_view view, YF_scene scn) {
  assert(view != NULL);
  view->scn = scn;
}

int yf_view_render(YF_view view) {
  assert(view != NULL);

  yf_event_poll();
  if (view->scn == NULL) {
    yf_seterr(YF_ERR_NILPTR, __func__);
    return -1;
  }
  int next = yf_window_next(view->win);
  if (next < 0) {
    switch (yf_geterr()) {
      case YF_ERR_INVWIN:
        /* TODO: Recreate window. */
      default:
        assert(0);
    }
  }
  YF_dim2 dim = yf_window_getdim(view->win);
  if (yf_scene_render(view->scn, view->pass, view->tgts[next], dim) != 0)
    return -1;
  if (yf_window_present(view->win) != 0) {
    switch (yf_geterr()) {
      case YF_ERR_INVWIN:
        /* TODO: Recreate window. */
      default:
        assert(0);
    }
  }

  return 0;
}

int yf_view_start(
  YF_view view,
  unsigned fps,
  void (*update)(double elapsed_time))
{
  assert(view != NULL);

  if (view->started) {
    yf_seterr(YF_ERR_BUSY, __func__);
    return -1;
  }
  view->started = 1;

  const double rate = (fps == 0) ? (0.0) : (1.0 / (double)fps);
  double dt = 0.0;
  double tm = yf_gettime();
  int r = 0;
  do {
    if (update != NULL)
      update(dt);
    if ((r = yf_view_render(view)) != 0)
      break;
    dt = yf_gettime() - tm;
    if (dt < rate) {
      yf_sleep(rate - dt);
      dt = yf_gettime() - tm;
    }
    tm += dt;
  } while (view->started);

  return r;
}

void yf_view_stop(YF_view view) {
  assert(view != NULL);
  view->started = 0;
}

void yf_view_deinit(YF_view view) {
  if (view == NULL)
    return;

  free(view->tgts);
  yf_pass_deinit(view->pass);
  yf_image_deinit(view->depth_img);
  yf_window_deinit(view->win);
  free(view);
}

void yf_view_onbutton(void (*fn)(int btn, int state, void *data), void *data) {
  l_callb.button.fn = fn;
  l_callb.button.data = data;
}

void yf_view_onmotion(void (*fn)(YF_coord2 coord, void *data), void *data) {
  l_callb.motion.fn = fn;
  l_callb.motion.data = data;
}

void yf_view_onkey(
  void (*fn)(int key, int state, unsigned mod_mask, void *data),
  void *data)
{
  l_callb.key.fn = fn;
  l_callb.key.data = data;
}

void yf_view_onfocus(void (*fn)(int in, void *data), void *data) {
  l_callb.focus.fn = fn;
  l_callb.focus.data = data;
}

static void set_eventfn(void) {
  static int done = 0;
  if (done) return;
  done = 1;

  YF_eventfn efn;

  efn.wd_resize = catch_wd_resize;
  yf_event_setfn(YF_EVENT_WD_RESIZE, efn, NULL);

  efn.wd_close = catch_wd_close;
  yf_event_setfn(YF_EVENT_WD_CLOSE, efn, NULL);

  efn.pt_enter = catch_pt_enter;
  yf_event_setfn(YF_EVENT_PT_ENTER, efn, NULL);

  efn.pt_leave = catch_pt_leave;
  yf_event_setfn(YF_EVENT_PT_LEAVE, efn, NULL);

  efn.pt_motion = catch_pt_motion;
  yf_event_setfn(YF_EVENT_PT_MOTION, efn, NULL);

  efn.pt_button = catch_pt_button;
  yf_event_setfn(YF_EVENT_PT_BUTTON, efn, NULL);

  efn.kb_enter = catch_kb_enter;
  yf_event_setfn(YF_EVENT_KB_ENTER, efn, NULL);

  efn.kb_leave = catch_kb_leave;
  yf_event_setfn(YF_EVENT_KB_LEAVE, efn, NULL);

  efn.kb_key = catch_kb_key;
  yf_event_setfn(YF_EVENT_KB_KEY, efn, NULL);
}

static void catch_wd_resize(YF_window win, YF_dim2 dim, void *data) {
  /* TODO */
}

static void catch_wd_close(YF_window win, void *data) {
  /* TODO */
}

static void catch_pt_enter(YF_window win, YF_coord2 coord, void *data) {
  /* TODO */
}

static void catch_pt_leave(YF_window win, void *data) {
  /* TODO */
}

static void catch_pt_motion(YF_coord2 coord, void *data) {
  if (l_callb.motion.fn != NULL)
    l_callb.motion.fn(coord, l_callb.motion.data);
}

static void catch_pt_button(int btn, int state, void *data) {
  if (l_callb.button.fn != NULL)
    l_callb.button.fn(btn, state, l_callb.button.data);
}

static void catch_kb_enter(YF_window win, void *data) {
  if (l_callb.focus.fn != NULL)
    l_callb.focus.fn(1, l_callb.focus.data);
}

static void catch_kb_leave(YF_window win, void *data) {
  if (l_callb.focus.fn != NULL)
    l_callb.focus.fn(0, l_callb.focus.data);
}

static void catch_kb_key(int key, int state, unsigned mod_mask, void *data) {
  if (l_callb.key.fn != NULL)
    l_callb.key.fn(key, state, mod_mask, l_callb.key.data);
}
