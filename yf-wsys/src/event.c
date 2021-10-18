/*
 * YF
 * event.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stddef.h>
#include <assert.h>

#include "event.h"

/* Event function/generic argument pair. */
typedef struct {
    YF_evtfn fn;
    void *arg;
} T_evtfn;

/* Initial dummy implementation functions. */
static int dummy_poll(unsigned evt_mask);
static void dummy_changed(int evt);

/* Event implementation instance. */
static YF_evt_imp imp_ = {dummy_poll, dummy_changed};

/* Event handlers. */
static T_evtfn closewd_  = { {.close_wd  = NULL}, NULL };
static T_evtfn resizewd_ = { {.resize_wd = NULL}, NULL };
static T_evtfn enterkb_  = { {.enter_kb  = NULL}, NULL };
static T_evtfn leavekb_  = { {.leave_kb  = NULL}, NULL };
static T_evtfn keykb_    = { {.key_kb    = NULL}, NULL };
static T_evtfn enterpt_  = { {.enter_pt  = NULL}, NULL };
static T_evtfn leavept_  = { {.leave_pt  = NULL}, NULL };
static T_evtfn motionpt_ = { {.motion_pt = NULL}, NULL };
static T_evtfn buttonpt_ = { {.button_pt = NULL}, NULL };

/* Mask of installed handlers. */
static int mask_ = YF_EVT_NONE;

int yf_pollevt(unsigned evt_mask)
{
    return imp_.poll(evt_mask);
}

void yf_setevtfn(int evt, YF_evtfn fn, void *arg)
{
    switch (evt) {
    case YF_EVT_CLOSEWD:
        closewd_.fn.close_wd = fn.close_wd;
        closewd_.arg = arg;
        mask_ = fn.close_wd != NULL ? (mask_ | evt) : (mask_ & ~evt);
        break;
    case YF_EVT_RESIZEWD:
        resizewd_.fn.resize_wd = fn.resize_wd;
        resizewd_.arg = arg;
        mask_ = fn.resize_wd != NULL ? (mask_ | evt) : (mask_ & ~evt);
        break;
    case YF_EVT_ENTERKB:
        enterkb_.fn.enter_kb = fn.enter_kb;
        enterkb_.arg = arg;
        mask_ = fn.enter_kb != NULL ? (mask_ | evt) : (mask_ & ~evt);
        break;
    case YF_EVT_LEAVEKB:
        leavekb_.fn.leave_kb = fn.leave_kb;
        leavekb_.arg = arg;
        mask_ = fn.leave_kb != NULL ? (mask_ | evt) : (mask_ & ~evt);
        break;
    case YF_EVT_KEYKB:
        keykb_.fn.key_kb = fn.key_kb;
        keykb_.arg = arg;
        mask_ = fn.key_kb != NULL ? (mask_ | evt) : (mask_ & ~evt);
        break;
    case YF_EVT_ENTERPT:
        enterpt_.fn.enter_pt = fn.enter_pt;
        enterpt_.arg = arg;
        mask_ = fn.enter_pt != NULL ? (mask_ | evt) : (mask_ & ~evt);
        break;
    case YF_EVT_LEAVEPT:
        leavept_.fn.leave_pt = fn.leave_pt;
        leavept_.arg = arg;
        mask_ = fn.leave_pt != NULL ? (mask_ | evt) : (mask_ & ~evt);
        break;
    case YF_EVT_MOTIONPT:
        motionpt_.fn.motion_pt = fn.motion_pt;
        motionpt_.arg = arg;
        mask_ = fn.motion_pt != NULL ? (mask_ | evt) : (mask_ & ~evt);
        break;
    case YF_EVT_BUTTONPT:
        buttonpt_.fn.button_pt = fn.button_pt;
        buttonpt_.arg = arg;
        mask_ = fn.button_pt != NULL ? (mask_ | evt) : (mask_ & ~evt);
        break;
    default:
        assert(0);
    }

    imp_.changed(evt);
}

void yf_getevtfn(int evt, YF_evtfn *fn, void **arg)
{
    assert(fn != NULL && arg != NULL);

    switch (evt) {
    case YF_EVT_CLOSEWD:
        fn->close_wd = closewd_.fn.close_wd;
        *arg = closewd_.arg;
        break;
    case YF_EVT_RESIZEWD:
        fn->resize_wd = resizewd_.fn.resize_wd;
        *arg = resizewd_.arg;
        break;
    case YF_EVT_ENTERKB:
        fn->enter_kb = enterkb_.fn.enter_kb;
        *arg = enterkb_.arg;
        break;
    case YF_EVT_LEAVEKB:
        fn->leave_kb = leavekb_.fn.leave_kb;
        *arg = leavekb_.arg;
        break;
    case YF_EVT_KEYKB:
        fn->key_kb = keykb_.fn.key_kb;
        *arg = keykb_.arg;
        break;
    case YF_EVT_ENTERPT:
        fn->enter_pt = enterpt_.fn.enter_pt;
        *arg = enterpt_.arg;
        break;
    case YF_EVT_LEAVEPT:
        fn->leave_pt = leavept_.fn.leave_pt;
        *arg = leavept_.arg;
        break;
    case YF_EVT_MOTIONPT:
        fn->motion_pt = motionpt_.fn.motion_pt;
        *arg = motionpt_.arg;
        break;
    case YF_EVT_BUTTONPT:
        fn->button_pt = buttonpt_.fn.button_pt;
        *arg = buttonpt_.arg;
        break;
    default:
        assert(0);
        fn->close_wd = NULL;
        *arg = NULL;
    }
}

unsigned yf_getevtmask(void)
{
    return mask_;
}

static int dummy_poll(unsigned evt_mask)
{
    yf_getevtimp(&imp_);
    return imp_.poll(evt_mask);
    /* never called again */
}

static void dummy_changed(int evt)
{
    yf_getevtimp(&imp_);
    imp_.changed(evt);
    /* never called again */
}
