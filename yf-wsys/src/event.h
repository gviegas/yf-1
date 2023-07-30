/*
 * YF
 * event.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_EVENT_H
#define YF_EVENT_H

#include "yf-event.h"

/* Specific event implementation. */
typedef struct yf_evt_imp {
    int (*poll)(unsigned evt_mask);

    /* Called when an event handler is set.
       The new value can be queried from 'yf_getevtfn()'. */
    void (*changed)(int evt);
} yf_evt_imp_t;

/* Gets the event implementation. */
void yf_getevtimp(yf_evt_imp_t *imp);

#endif /* YF_EVENT_H */
