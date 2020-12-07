/*
 * YF
 * yf-event.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_EVENT_H
#define YF_YF_EVENT_H

#include <yf/com/yf-defs.h>

YF_DECLS_BEGIN

typedef struct YF_window_o *YF_window;

/* Type defining event handlers. */
typedef union {
#define YF_EVT_NONE 0

  /* Window. */
#define YF_EVT_CLOSEWD  0x0001
#define YF_EVT_RESIZEWD 0x0002
  void (*close_wd)(YF_window *win);
  void (*resize_wd)(YF_window *win, unsigned width, unsigned height);

  /* Keyboard. */
#define YF_EVT_ENTERKB 0x0010
#define YF_EVT_LEAVEKB 0x0020
#define YF_EVT_KEYKB   0x0040
  void (*enter_kb)(YF_window *win);
  void (*leave_kb)(YF_window *win);
  void (*key_kb)(int key, int state, unsigned mod_mask);

  /* Pointer. */
#define YF_EVT_ENTERPT  0x0100
#define YF_EVT_LEAVEPT  0x0200
#define YF_EVT_MOTIONPT 0x0400
#define YF_EVT_BUTTONPT 0x0800
  void (*enter_pt)(YF_window *win, int x, int y);
  void (*leave_pt)(YF_window *win);
  void (*motion_pt)(int x, int y);
  void (*button_pt)(int btn, int state, int x, int y);

#define YF_EVT_OTHER 1<<31
#define YF_EVT_ANY   ~0
} YF_evtfn;

/* Polls for events.
   Events not present in 'evt_mask' or with no handler set will be ignored. */
int yf_pollevt(unsigned evt_mask);

/* Sets an event handler. */
void yf_setevtfn(int evt, YF_evtfn fn, void *data);

/* Gets an event handler. */
void yf_getevtfn(int evt, YF_evtfn *fn, void **data);

/* Gets a mask of 'YF_EVT_*' bits indicating which handlers are set. */
unsigned yf_getevtmask(void);

YF_DECLS_END

#endif /* YF_YF_EVENT_H */
