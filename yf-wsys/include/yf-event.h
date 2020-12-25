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

/**
 * Type defining event handlers.
 */
typedef union {
#define YF_EVT_NONE 0

  /* Window. */
#define YF_EVT_CLOSEWD  0x0001
#define YF_EVT_RESIZEWD 0x0002
  void (*close_wd)(YF_window win, void *arg);
  void (*resize_wd)(YF_window win, unsigned width, unsigned height, void *arg);

  /* Keyboard. */
#define YF_EVT_ENTERKB 0x0010
#define YF_EVT_LEAVEKB 0x0020
#define YF_EVT_KEYKB   0x0040
  void (*enter_kb)(YF_window win, void *arg);
  void (*leave_kb)(YF_window win, void *arg);
  void (*key_kb)(int key, int state, unsigned mod_mask, void *arg);

  /* Pointer. */
#define YF_EVT_ENTERPT  0x0100
#define YF_EVT_LEAVEPT  0x0200
#define YF_EVT_MOTIONPT 0x0400
#define YF_EVT_BUTTONPT 0x0800
  void (*enter_pt)(YF_window win, int x, int y, void *arg);
  void (*leave_pt)(YF_window win, void *arg);
  void (*motion_pt)(int x, int y, void *arg);
  void (*button_pt)(int btn, int state, int x, int y, void *arg);

#define YF_EVT_OTHER 1<<31
#define YF_EVT_ANY   ~0
} YF_evtfn;

/**
 * Polls for events.
 *
 * Events not present in 'evt_mask' or with no handler set will be ignored.
 *
 * @param evt_mask: A mask of 'YF_EVT' values indicating the events of interest.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_pollevt(unsigned evt_mask);

/**
 * Sets an event handler.
 *
 * @param evt: The 'YF_EVT' value indicating the event type.
 * @param fn: The function to set. It must match the provided 'evt' type.
 * @param arg: The generic argument to pass on 'fn' calls. Can be 'NULL'.
 */
void yf_setevtfn(int evt, YF_evtfn fn, void *arg);

/**
 * Gets an event handler.
 *
 * @param evt: The 'YF_EVT' value indicating the event to get.
 * @param fn: The destination for the event handler.
 * @param arg: The destination for the generic argument.
 */
void yf_getevtfn(int evt, YF_evtfn *fn, void **arg);

/**
 * Gets a mask of 'YF_EVT' bits indicating which handlers are set.
 *
 * @return: A mask of 'YF_EVT' values indicating which events have a non-null
 *  handler set.
 */
unsigned yf_getevtmask(void);

YF_DECLS_END

#endif /* YF_YF_EVENT_H */
