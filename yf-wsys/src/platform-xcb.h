/*
 * YF
 * platform-xcb.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_PLATFORM_XCB_H
#define YF_PLATFORM_XCB_H

#include <xcb/xcb.h>

#include "window.h"
#include "event.h"

/* Loads the xcb platform. */
int yf_loadxcb(void);

/* Unloads the xcb platform. */
void yf_unldxcb(void);

/* Implementations. */
extern const yf_win_imp_t yf_g_winxcb;
extern const yf_evt_imp_t yf_g_evtxcb;

/* Global xcb variables. */
typedef struct yf_varsxcb {
    xcb_connection_t *conn;
    xcb_visualid_t visual;
    xcb_window_t root_win;
    uint32_t white_px;
    uint32_t black_px;
    struct {
        xcb_atom_t proto;
        xcb_atom_t del;
        xcb_atom_t title;
        xcb_atom_t utf8;
        xcb_atom_t clss;
    } atom;
} yf_varsxcb_t;

/* Global variables instance.
   This data is initialized by 'yf_loadxcb()'. */
extern yf_varsxcb_t yf_g_varsxcb;

#endif /* YF_PLATFORM_XCB_H */
