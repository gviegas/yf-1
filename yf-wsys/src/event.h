/*
 * YF
 * event.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_EVENT_H
#define YF_EVENT_H

#include "yf-event.h"

/* Specific event implementation. */
typedef struct {
    int (*poll)(unsigned evt_mask);

    /* Called when an event handler is set.
       The new value can be queried from 'yf_getevtfn()'. */
    void (*changed)(int evt);
} YF_evt_imp;

/* Gets the event implementation. */
void yf_getevtimp(YF_evt_imp *imp);

#endif /* YF_EVENT_H */
