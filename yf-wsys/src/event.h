/*
 * YF
 * event.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_EVENT_H
#define YF_EVENT_H

/* Type defining a specific event implementation. */
typedef struct {
  int (*poll)(unsigned);

  /* Called when an event handler is set.
     The new value can be obtained using 'yf_getevtfn'. */
  void (*changed)(int evt);
} YF_evt_imp;

/* Gets the event implementation. */
const YF_evt_imp *yf_getevtimp(void);

#endif /* YF_EVENT_H */
