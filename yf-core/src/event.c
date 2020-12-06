/*
 * YF
 * event.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stddef.h>
#include <assert.h>

#include "event.h"
#include "window.h"

/* Type holding an event handler and associated generic data. */
struct L_fn {
  YF_eventfn fn;
  void *data;
};

/* Event handlers & data installed for each event type. */
struct L_fn l_wd_resize = {.fn.wd_resize = NULL, .data = NULL};
struct L_fn l_wd_close = {.fn.wd_close = NULL, .data = NULL};
struct L_fn l_pt_enter = {.fn.pt_enter = NULL, .data = NULL};
struct L_fn l_pt_leave = {.fn.pt_leave = NULL, .data = NULL};
struct L_fn l_pt_motion = {.fn.pt_motion = NULL, .data = NULL};
struct L_fn l_pt_button = {.fn.pt_button = NULL, .data = NULL};
struct L_fn l_kb_enter = {.fn.kb_enter = NULL, .data = NULL};
struct L_fn l_kb_leave = {.fn.kb_leave = NULL, .data = NULL};
struct L_fn l_kb_key = {.fn.kb_key = NULL, .data = NULL};

void yf_event_setfn(int event, YF_eventfn fn, void *data) {
  switch (event) {
    case YF_EVENT_WD_RESIZE:
      l_wd_resize.fn = fn;
      l_wd_resize.data = data;
      break;
    case YF_EVENT_WD_CLOSE:
      l_wd_close.fn = fn;
      l_wd_close.data = data;
      break;
    case YF_EVENT_PT_ENTER:
      l_pt_enter.fn = fn;
      l_pt_enter.data = data;
      break;
    case YF_EVENT_PT_LEAVE:
      l_pt_leave.fn = fn;
      l_pt_leave.data = data;
      break;
    case YF_EVENT_PT_MOTION:
      l_pt_motion.fn = fn;
      l_pt_motion.data = data;
      break;
    case YF_EVENT_PT_BUTTON:
      l_pt_button.fn = fn;
      l_pt_button.data = data;
      break;
    case YF_EVENT_KB_ENTER:
      l_kb_enter.fn = fn;
      l_kb_enter.data = data;
      break;
    case YF_EVENT_KB_LEAVE:
      l_kb_leave.fn = fn;
      l_kb_leave.data = data;
      break;
    case YF_EVENT_KB_KEY:
      l_kb_key.fn = fn;
      l_kb_key.data = data;
      break;
  }
}

void yf_event_getfn(int event, YF_eventfn *fn, void **data) {
  assert(fn != NULL);
  assert(data != NULL);
  switch (event) {
    case YF_EVENT_WD_RESIZE:
      *fn = l_wd_resize.fn;
      *data = l_wd_resize.data;
      break;
    case YF_EVENT_WD_CLOSE:
      *fn = l_wd_close.fn;
      *data = l_wd_close.data;
      break;
    case YF_EVENT_PT_ENTER:
      *fn = l_pt_enter.fn;
      *data = l_pt_enter.data;
      break;
    case YF_EVENT_PT_LEAVE:
      *fn = l_pt_leave.fn;
      *data = l_pt_leave.data;
      break;
    case YF_EVENT_PT_MOTION:
      *fn = l_pt_motion.fn;
      *data = l_pt_motion.data;
      break;
    case YF_EVENT_PT_BUTTON:
      *fn = l_pt_button.fn;
      *data = l_pt_button.data;
      break;
    case YF_EVENT_KB_ENTER:
      *fn = l_kb_enter.fn;
      *data = l_kb_enter.data;
      break;
    case YF_EVENT_KB_LEAVE:
      *fn = l_kb_leave.fn;
      *data = l_kb_leave.data;
      break;
    case YF_EVENT_KB_KEY:
      *fn = l_kb_key.fn;
      *data = l_kb_key.data;
      break;
    default:
      fn->wd_resize = NULL;
      *data = NULL;
  }
}

int yf_event_poll(void) {
  return yf_wsi_poll();
}
