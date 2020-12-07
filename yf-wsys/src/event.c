/*
 * YF
 * event.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stddef.h>
#include <assert.h>

#include "event.h"

/* Type defining an event function and its 'data' argument. */
typedef struct {
  YF_evtfn fn;
  void *data;
} L_evtfn;

/* Initial dummy implementation functions. */
static int dummy_poll(unsigned evt_mask);
static void dummy_changed(int evt);

/* Event implementation instance. */
static YF_evt_imp l_imp = {dummy_poll, dummy_changed};

/* Event handlers. */
static L_evtfn l_closewd  = { {.close_wd  = NULL}, NULL };
static L_evtfn l_resizewd = { {.resize_wd = NULL}, NULL };
static L_evtfn l_enterkb  = { {.enter_kb  = NULL}, NULL };
static L_evtfn l_leavekb  = { {.leave_kb  = NULL}, NULL };
static L_evtfn l_keykb    = { {.key_kb    = NULL}, NULL };
static L_evtfn l_enterpt  = { {.enter_pt  = NULL}, NULL };
static L_evtfn l_leavept  = { {.leave_pt  = NULL}, NULL };
static L_evtfn l_motionpt = { {.motion_pt = NULL}, NULL };
static L_evtfn l_buttonpt = { {.button_pt = NULL}, NULL };

/* Mask of installed handlers. */
static int l_mask = YF_EVT_NONE;

int yf_pollevt(unsigned evt_mask) {
  return l_imp.poll(evt_mask);
}

void yf_setevtfn(int evt, YF_evtfn fn, void *data) {
  switch (evt) {
    case YF_EVT_CLOSEWD:
      l_closewd.fn.close_wd = fn.close_wd;
      l_closewd.data = data;
      l_mask = fn.close_wd != NULL ? (l_mask | evt) : (l_mask & ~evt);
      break;
    case YF_EVT_RESIZEWD:
      l_resizewd.fn.resize_wd = fn.resize_wd;
      l_resizewd.data = data;
      l_mask = fn.resize_wd != NULL ? (l_mask | evt) : (l_mask & ~evt);
      break;
    case YF_EVT_ENTERKB:
      l_enterkb.fn.enter_kb = fn.enter_kb;
      l_enterkb.data = data;
      l_mask = fn.enter_kb != NULL ? (l_mask | evt) : (l_mask & ~evt);
      break;
    case YF_EVT_LEAVEKB:
      l_leavekb.fn.leave_kb = fn.leave_kb;
      l_leavekb.data = data;
      l_mask = fn.leave_kb != NULL ? (l_mask | evt) : (l_mask & ~evt);
      break;
    case YF_EVT_KEYKB:
      l_keykb.fn.key_kb = fn.key_kb;
      l_keykb.data = data;
      l_mask = fn.key_kb != NULL ? (l_mask | evt) : (l_mask & ~evt);
      break;
    case YF_EVT_ENTERPT:
      l_enterpt.fn.enter_pt = fn.enter_pt;
      l_enterpt.data = data;
      l_mask = fn.enter_pt != NULL ? (l_mask | evt) : (l_mask & ~evt);
      break;
    case YF_EVT_LEAVEPT:
      l_leavept.fn.leave_pt = fn.leave_pt;
      l_leavept.data = data;
      l_mask = fn.leave_pt != NULL ? (l_mask | evt) : (l_mask & ~evt);
      break;
    case YF_EVT_MOTIONPT:
      l_motionpt.fn.motion_pt = fn.motion_pt;
      l_motionpt.data = data;
      l_mask = fn.motion_pt != NULL ? (l_mask | evt) : (l_mask & ~evt);
      break;
    case YF_EVT_BUTTONPT:
      l_buttonpt.fn.button_pt = fn.button_pt;
      l_buttonpt.data = data;
      l_mask = fn.button_pt != NULL ? (l_mask | evt) : (l_mask & ~evt);
      break;
    default:
      assert(0);
  }

  l_imp.changed(evt);
}

void yf_getevtfn(int evt, YF_evtfn *fn, void **data) {
  assert(fn != NULL && data != NULL);

  switch (evt) {
    case YF_EVT_CLOSEWD:
      fn->close_wd = l_closewd.fn.close_wd;
      *data = l_closewd.data;
      break;
    case YF_EVT_RESIZEWD:
      fn->resize_wd = l_resizewd.fn.resize_wd;
      *data = l_resizewd.data;
      break;
    case YF_EVT_ENTERKB:
      fn->enter_kb = l_enterkb.fn.enter_kb;
      *data = l_enterkb.data;
      break;
    case YF_EVT_LEAVEKB:
      fn->leave_kb = l_leavekb.fn.leave_kb;
      *data = l_leavekb.data;
      break;
    case YF_EVT_KEYKB:
      fn->key_kb = l_keykb.fn.key_kb;
      *data = l_keykb.data;
      break;
    case YF_EVT_ENTERPT:
      fn->enter_pt = l_enterpt.fn.enter_pt;
      *data = l_enterpt.data;
      break;
    case YF_EVT_LEAVEPT:
      fn->leave_pt = l_leavept.fn.leave_pt;
      *data = l_leavept.data;
      break;
    case YF_EVT_MOTIONPT:
      fn->motion_pt = l_motionpt.fn.motion_pt;
      *data = l_motionpt.data;
      break;
    case YF_EVT_BUTTONPT:
      fn->button_pt = l_buttonpt.fn.button_pt;
      *data = l_buttonpt.data;
      break;
    default:
      assert(0);
      fn->close_wd = NULL;
      *data = NULL;
  }
}

unsigned yf_getevtmask(void) {
  return l_mask;
}

static int dummy_poll(unsigned evt_mask) {
  yf_getevtimp(&l_imp);
  return l_imp.poll(evt_mask);
  /* never called again */
}

static void dummy_changed(int evt) {
  yf_getevtimp(&l_imp);
  l_imp.changed(evt);
  /* never called again */
}
