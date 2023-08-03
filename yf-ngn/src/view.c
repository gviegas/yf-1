/*
 * YF
 * view.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
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

struct yf_view {
    yf_context_t *ctx;
    yf_wsi_t *wsi;
    yf_window_t *win;
    yf_image_t *depth_img;
    yf_pass_t *pass;
    yf_target_t **tgts;
    unsigned tgt_n;
    yf_scene_t *scn;
};

/* Global pass instance. */
yf_pass_t *yf_g_pass = NULL;

/* Flag to disallow the creation of multiple views. */
static atomic_flag flag_ = ATOMIC_FLAG_INIT;

yf_view_t *yf_view_init(yf_window_t *win)
{
    assert(win != NULL);

    if (atomic_flag_test_and_set(&flag_)) {
        yf_seterr(YF_ERR_EXIST, __func__);
        return NULL;
    }

    yf_view_t *view = calloc(1, sizeof(yf_view_t));
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

    const yf_dim3_t dim3 = {width, height, 1};
    view->depth_img = yf_image_init(view->ctx, YF_PIXFMT_D16UNORM, dim3, 1,
                                    1, 1);
    if (view->depth_img == NULL) {
        yf_view_deinit(view);
        return NULL;
    }

    unsigned pres_img_n;
    yf_image_t *const *pres_imgs = yf_wsi_getimages(view->wsi, &pres_img_n);
    if (pres_imgs == NULL || pres_img_n == 0) {
        yf_view_deinit(view);
        return NULL;
    }

    if (yf_g_pass == NULL) {
        int pres_fmt;
        unsigned pres_spl;
        yf_image_getval(pres_imgs[0], &pres_fmt, NULL, NULL, NULL, &pres_spl);

        const yf_colordsc_t clr_dsc = {
            .pixfmt = pres_fmt,
            .samples = pres_spl,
            /* TODO */
            .loadop = YF_LOADOP_LOAD,
            .storeop = YF_STOREOP_STORE
        };
        const yf_depthdsc_t dep_dsc = {
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

    const yf_dim2_t dim2 = {width, height};
    const yf_attach_t dep_att = {view->depth_img, 0};

    yf_attach_t *clr_atts = malloc(pres_img_n * sizeof(yf_attach_t));
    view->tgts = calloc(pres_img_n, sizeof(yf_target_t *));
    if (clr_atts == NULL || view->tgts == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        yf_view_deinit(view);
        free(clr_atts);
        return NULL;
    }

    for (size_t i = 0; i < pres_img_n; i++) {
        clr_atts[i] = (yf_attach_t){pres_imgs[i], 0};
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

/* TODO: Replace 'fps' arg with 'vsync' and fix the pacing. */
int yf_view_loop(yf_view_t *view, yf_scene_t *scn, unsigned fps,
                 int (*update)(double elapsed_time, void *arg), void *arg)
{
    assert(view != NULL);
    assert(scn != NULL);
    assert(update != NULL);

    if (view->scn != NULL) {
        yf_seterr(YF_ERR_BUSY, __func__);
        return -1;
    }

    view->scn = scn;
    const double rate = (fps == 0) ? (0.0) : (1.0 / (double)fps);
    double dt = 0.0;
    double tm = yf_gettime();
    int r = 0;

    while (update(dt, arg) == 0) {
        if ((r = yf_view_render(view, view->scn)) != 0)
            break;

        if ((dt = yf_gettime() - tm) < rate) {
            yf_sleep(rate - dt);
            dt = yf_gettime() - tm;
        }

        tm += dt;
    }

    view->scn = NULL;
    return r;
}

yf_scene_t *yf_view_swap(yf_view_t *view, yf_scene_t *scn)
{
    assert(view != NULL);
    assert(scn != NULL);
    assert(view->scn != NULL);

    yf_scene_t *cur = view->scn;
    view->scn = scn;
    return cur;
}

int yf_view_render(yf_view_t *view, yf_scene_t *scn)
{
    assert(view != NULL);
    assert(scn != NULL);

    yf_pollevt(YF_EVT_ANY);

    int next = yf_wsi_next(view->wsi, 0);
    if (next < 0) {
        switch (yf_geterr()) {
        case YF_ERR_INVWIN:
            /* TODO: Recreate window. */
        default:
            assert(0);
        }
    }

    yf_dim2_t dim;
    yf_window_getsize(view->win, &dim.width, &dim.height);
    if (yf_scene_render(scn, view->pass, view->tgts[next], dim) != 0)
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

void yf_view_deinit(yf_view_t *view)
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
