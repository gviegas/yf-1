/*
 * YF
 * window.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#include "yf/com/yf-error.h"

#include "window.h"

struct yf_window { void *data; };

/* Window implementation instance.
   This will not change during runtime, so it is retrieved only once. */
static yf_win_imp_t imp_ = {0};

yf_window_t *yf_window_init(unsigned width, unsigned height, const char *title,
                            unsigned creat_mask)
{
    if (imp_.init == NULL)
        yf_getwinimp(&imp_);

    assert(imp_.init != NULL);

    yf_window_t *win = malloc(sizeof(yf_window_t));
    if (win == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }

    win->data = imp_.init(width, height, title, creat_mask, win);
    if (win->data == NULL) {
        free(win);
        return NULL;
    }

    return win;
}

int yf_window_open(yf_window_t *win)
{
    assert(imp_.open != NULL);
    assert(win != NULL);

    return imp_.open(win->data);
}

int yf_window_close(yf_window_t *win)
{
    assert(imp_.close != NULL);
    assert(win != NULL);

    return imp_.close(win->data);
}

int yf_window_resize(yf_window_t *win, unsigned width, unsigned height)
{
    assert(imp_.resize != NULL);
    assert(win != NULL);

    return imp_.resize(win->data, width, height);
}

int yf_window_toggle(yf_window_t *win)
{
    assert(imp_.toggle != NULL);
    assert(win != NULL);

    return imp_.toggle(win->data);
}

int yf_window_settitle(yf_window_t *win, const char *title)
{
    assert(imp_.settitle != NULL);
    assert(win != NULL);

    return imp_.settitle(win->data, title);
}

void yf_window_getsize(yf_window_t *win, unsigned *width, unsigned *height)
{
    assert(imp_.getsize != NULL);
    assert(win != NULL);

    imp_.getsize(win->data, width, height);
}

void yf_window_deinit(yf_window_t *win)
{
    assert(imp_.deinit != NULL);

    if (win != NULL) {
        imp_.deinit(win->data);
        free(win);
    }
}
