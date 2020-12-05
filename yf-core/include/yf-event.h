/*
 * YF
 * yf-event.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_EVENT_H
#define YF_YF_EVENT_H

#include "yf-common.h"
#include "yf-window.h"

YF_DECLS_BEGIN

/* Type defining functions for event handling. */
typedef union YF_eventfn {
  /* Window resize event. */
  void (*wd_resize)(YF_window win, YF_dim2 dim, void *data);

  /* Window close event. */
  void (*wd_close)(YF_window win, void *data);

  /* Pointer enter event. */
  void (*pt_enter)(YF_window win, YF_coord2 coord, void *data);

  /* Pointer leave event. */
  void (*pt_leave)(YF_window win, void *data);

  /* Pointer motion event. */
  void (*pt_motion)(YF_coord2 coord, void *data);

  /* Pointer button press/release event. */
  void (*pt_button)(int btn, int state, void *data);

  /* Keyboard enter (focus in) event. */
  void (*kb_enter)(YF_window win, void *data);

  /* Keyboard leave (focus out) event. */
  void (*kb_leave)(YF_window win, void *data);

  /* Keyboard key press/release event. */
  void (*kb_key)(int key, int state, unsigned mod_mask, void *data);
} YF_eventfn;

/* Sets the event function to use when handling events of a given type. */
void yf_event_setfn(int event, YF_eventfn fn, void *data);

/* Gets the event function currently set to handle events of a given type. */
void yf_event_getfn(int event, YF_eventfn *fn, void **data);

/* Dequeues events and calls installed handlers. */
int yf_event_poll(void);

/* Event types. */
#define YF_EVENT_NONE      0x0001
#define YF_EVENT_WD_RESIZE 0x0002
#define YF_EVENT_WD_CLOSE  0x0004
#define YF_EVENT_PT_ENTER  0x0008
#define YF_EVENT_PT_LEAVE  0x0010
#define YF_EVENT_PT_MOTION 0x0020
#define YF_EVENT_PT_BUTTON 0x0040
#define YF_EVENT_KB_ENTER  0x0080
#define YF_EVENT_KB_LEAVE  0x0100
#define YF_EVENT_KB_KEY    0x0200

YF_DECLS_END

#endif /* YF_YF_EVENT_H */
