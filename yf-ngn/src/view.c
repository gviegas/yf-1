/*
 * YF
 * view.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#ifndef __STDC_NO_ATOMICS__
# include <stdatomic.h>
#else
# error "C11 atomics required"
#endif

#include "yf/com/yf-clock.h"
#include "yf/com/yf-error.h"
#include "yf/core/yf-image.h"
#include "yf/core/yf-wsi.h"
#include "yf/wsys/yf-event.h"

#include "yf-view.h"
#include "coreobj.h"
#include "scene.h"

struct YF_view_o {
    YF_context ctx;
    YF_wsi wsi;
    YF_window win;
    YF_image depth_img;
    YF_pass pass;
    YF_target *tgts;
    unsigned tgt_n;
    YF_scene scn;
    int started;
};

/* Global pass instance. */
YF_pass yf_g_pass = NULL;

/* Flag to disallow the creation of multiple views. */
static atomic_flag flag_ = ATOMIC_FLAG_INIT;

YF_view yf_view_init(YF_window win)
{
    assert(win != NULL);

    if (atomic_flag_test_and_set(&flag_)) {
        yf_seterr(YF_ERR_EXIST, __func__);
        return NULL;
    }

    YF_view view = calloc(1, sizeof(struct YF_view_o));
    if (view == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }
    if ((view->ctx = yf_getctx()) == NULL) {
        yf_view_deinit(view);
        return NULL;
    }
    if ((view->wsi = yf_wsi_init(view->ctx, win)) == NULL) {
        yf_view_deinit(view);
        return NULL;
    }
    view->win = win;

    unsigned width, height;
    yf_window_getsize(view->win, &width, &height);

    const YF_dim3 dim3 = {width, height, 1};
    view->depth_img = yf_image_init(view->ctx, YF_PIXFMT_D16UNORM, dim3, 1,
                                    1, 1);
    if (view->depth_img == NULL) {
        yf_view_deinit(view);
        return NULL;
    }

    unsigned pres_img_n;
    const YF_image *pres_imgs = yf_wsi_getimages(view->wsi, &pres_img_n);
    if (pres_imgs == NULL || pres_img_n == 0) {
        yf_view_deinit(view);
        return NULL;
    }

    if (yf_g_pass == NULL) {
        int pres_fmt;
        unsigned pres_spl;
        yf_image_getval(pres_imgs[0], &pres_fmt, NULL, NULL, NULL, &pres_spl);

        const YF_colordsc clr_dsc = {
            .pixfmt = pres_fmt,
            .samples = pres_spl,
            /* TODO */
            .loadop = YF_LOADOP_LOAD,
            .storeop = YF_STOREOP_STORE
        };
        const YF_depthdsc dep_dsc = {
            .pixfmt = YF_PIXFMT_D16UNORM,
            .samples = 1,
            .depth_loadop = YF_LOADOP_UNDEF,
            .depth_storeop = YF_STOREOP_UNDEF,
            .stencil_loadop = YF_LOADOP_UNDEF,
            .stencil_storeop = YF_STOREOP_UNDEF
        };

        view->pass = yf_pass_init(view->ctx, &clr_dsc, 1, NULL, &dep_dsc);
        if (view->pass == NULL) {
            yf_view_deinit(view);
            return NULL;
        }
        yf_g_pass = view->pass;

    } else {
        /* TODO: Ensure that the global pass is compatible with this view. */
        view->pass = yf_g_pass;
    }

    const YF_dim2 dim2 = {width, height};
    const YF_attach dep_att = {view->depth_img, 0};

    YF_attach *clr_atts = malloc(pres_img_n * sizeof(YF_attach));
    view->tgts = calloc(pres_img_n, sizeof(YF_target));
    if (clr_atts == NULL || view->tgts == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        yf_view_deinit(view);
        free(clr_atts);
        return NULL;
    }

    for (size_t i = 0; i < pres_img_n; i++) {
        clr_atts[i] = (YF_attach){pres_imgs[i], 0};
        view->tgts[i] = yf_pass_maketarget(view->pass, dim2, 1, clr_atts+i,
                                           NULL, &dep_att);
        if (view->tgts[i] == NULL) {
            yf_view_deinit(view);
            free(clr_atts);
            return NULL;
        }
    }

    view->tgt_n = pres_img_n;
    free(clr_atts);
    return view;
}

YF_scene yf_view_getscene(YF_view view)
{
    assert(view != NULL);
    return view->scn;
}

void yf_view_setscene(YF_view view, YF_scene scn)
{
    assert(view != NULL);
    view->scn = scn;
}

int yf_view_render(YF_view view)
{
    assert(view != NULL);

    yf_pollevt(YF_EVT_ANY);

    if (view->scn == NULL) {
        yf_seterr(YF_ERR_NILPTR, __func__);
        return -1;
    }

    int next = yf_wsi_next(view->wsi, 0);
    if (next < 0) {
        switch (yf_geterr()) {
        case YF_ERR_INVWIN:
            /* TODO: Recreate window. */
        default:
            assert(0);
        }
    }

    YF_dim2 dim;
    yf_window_getsize(view->win, &dim.width, &dim.height);
    if (yf_scene_render(view->scn, view->pass, view->tgts[next], dim) != 0)
        return -1;

    if (yf_wsi_present(view->wsi, next) != 0) {
        switch (yf_geterr()) {
        case YF_ERR_INVWIN:
            /* TODO: Recreate window. */
        default:
            assert(0);
            abort();
        }
    }

    return 0;
}

int yf_view_start(YF_view view, unsigned fps,
                  void (*update)(double elapsed_time, void *arg), void *arg)
{
    assert(view != NULL);
    assert(update != NULL);

    if (view->started) {
        yf_seterr(YF_ERR_BUSY, __func__);
        return -1;
    }
    view->started = 1;

    const double rate = (fps == 0) ? (0.0) : (1.0 / (double)fps);
    double dt = 0.0, tm = yf_gettime();
    int r = 0;
    do {
        update(dt, arg);

        if ((r = yf_view_render(view)) != 0)
            break;

        dt = yf_gettime() - tm;
        if (dt < rate) {
            yf_sleep(rate - dt);
            dt = yf_gettime() - tm;
        }
        tm += dt;
    } while (view->started);

    return r;
}

void yf_view_stop(YF_view view)
{
    assert(view != NULL);
    view->started = 0;
}

void yf_view_deinit(YF_view view)
{
    if (view == NULL)
        return;

    for (unsigned i = 0; i < view->tgt_n; i++)
        yf_pass_unmktarget(view->pass, view->tgts[i]);
    free(view->tgts);

    /* XXX: Pass deinitialization handled on 'coreobj'. */

    yf_image_deinit(view->depth_img);
    yf_wsi_deinit(view->wsi);

    free(view);

    atomic_flag_clear(&flag_);
}
