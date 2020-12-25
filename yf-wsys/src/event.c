/*
 * YF
 * event.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stddef.h>
#include <assert.h>

#include "event.h"

/* Type defining an event function and its generic argument. */
typedef struct {
  YF_evtfn fn;
  void *arg;
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

void yf_setevtfn(int evt, YF_evtfn fn, void *arg) {
  switch (evt) {
    case YF_EVT_CLOSEWD:
      l_closewd.fn.close_wd = fn.close_wd;
      l_closewd.arg = arg;
      l_mask = fn.close_wd != NULL ? (l_mask | evt) : (l_mask & ~evt);
      break;
    case YF_EVT_RESIZEWD:
      l_resizewd.fn.resize_wd = fn.resize_wd;
      l_resizewd.arg = arg;
      l_mask = fn.resize_wd != NULL ? (l_mask | evt) : (l_mask & ~evt);
      break;
    case YF_EVT_ENTERKB:
      l_enterkb.fn.enter_kb = fn.enter_kb;
      l_enterkb.arg = arg;
      l_mask = fn.enter_kb != NULL ? (l_mask | evt) : (l_mask & ~evt);
      break;
    case YF_EVT_LEAVEKB:
      l_leavekb.fn.leave_kb = fn.leave_kb;
      l_leavekb.arg = arg;
      l_mask = fn.leave_kb != NULL ? (l_mask | evt) : (l_mask & ~evt);
      break;
    case YF_EVT_KEYKB:
      l_keykb.fn.key_kb = fn.key_kb;
      l_keykb.arg = arg;
      l_mask = fn.key_kb != NULL ? (l_mask | evt) : (l_mask & ~evt);
      break;
    case YF_EVT_ENTERPT:
      l_enterpt.fn.enter_pt = fn.enter_pt;
      l_enterpt.arg = arg;
      l_mask = fn.enter_pt != NULL ? (l_mask | evt) : (l_mask & ~evt);
      break;
    case YF_EVT_LEAVEPT:
      l_leavept.fn.leave_pt = fn.leave_pt;
      l_leavept.arg = arg;
      l_mask = fn.leave_pt != NULL ? (l_mask | evt) : (l_mask & ~evt);
      break;
    case YF_EVT_MOTIONPT:
      l_motionpt.fn.motion_pt = fn.motion_pt;
      l_motionpt.arg = arg;
      l_mask = fn.motion_pt != NULL ? (l_mask | evt) : (l_mask & ~evt);
      break;
    case YF_EVT_BUTTONPT:
      l_buttonpt.fn.button_pt = fn.button_pt;
      l_buttonpt.arg = arg;
      l_mask = fn.button_pt != NULL ? (l_mask | evt) : (l_mask & ~evt);
      break;
    default:
      assert(0);
  }

  l_imp.changed(evt);
}

void yf_getevtfn(int evt, YF_evtfn *fn, void **arg) {
  assert(fn != NULL && arg != NULL);

  switch (evt) {
    case YF_EVT_CLOSEWD:
      fn->close_wd = l_closewd.fn.close_wd;
      *arg = l_closewd.arg;
      break;
    case YF_EVT_RESIZEWD:
      fn->resize_wd = l_resizewd.fn.resize_wd;
      *arg = l_resizewd.arg;
      break;
    case YF_EVT_ENTERKB:
      fn->enter_kb = l_enterkb.fn.enter_kb;
      *arg = l_enterkb.arg;
      break;
    case YF_EVT_LEAVEKB:
      fn->leave_kb = l_leavekb.fn.leave_kb;
      *arg = l_leavekb.arg;
      break;
    case YF_EVT_KEYKB:
      fn->key_kb = l_keykb.fn.key_kb;
      *arg = l_keykb.arg;
      break;
    case YF_EVT_ENTERPT:
      fn->enter_pt = l_enterpt.fn.enter_pt;
      *arg = l_enterpt.arg;
      break;
    case YF_EVT_LEAVEPT:
      fn->leave_pt = l_leavept.fn.leave_pt;
      *arg = l_leavept.arg;
      break;
    case YF_EVT_MOTIONPT:
      fn->motion_pt = l_motionpt.fn.motion_pt;
      *arg = l_motionpt.arg;
      break;
    case YF_EVT_BUTTONPT:
      fn->button_pt = l_buttonpt.fn.button_pt;
      *arg = l_buttonpt.arg;
      break;
    default:
      assert(0);
      fn->close_wd = NULL;
      *arg = NULL;
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
