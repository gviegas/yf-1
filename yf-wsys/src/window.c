/*
 * YF
 * window.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#include "yf/com/yf-error.h"

#include "window.h"

struct YF_window_o { void *data; };

/* Window implementation instance.
   This will not change during runtime, so it is retrieved only once. */
static YF_win_imp l_imp = {0};

YF_window yf_window_init(unsigned width, unsigned height, const char *title,
                         unsigned creat_mask)
{
    if (l_imp.init == NULL)
        yf_getwinimp(&l_imp);
    assert(l_imp.init != NULL);

    YF_window win = malloc(sizeof(struct YF_window_o));
    if (win == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }

    win->data = l_imp.init(width, height, title, creat_mask, win);
    if (win->data == NULL) {
        free(win);
        return NULL;
    }

    return win;
}

int yf_window_open(YF_window win)
{
    assert(l_imp.open != NULL);
    assert(win != NULL);

    return l_imp.open(win->data);
}

int yf_window_close(YF_window win)
{
    assert(l_imp.close != NULL);
    assert(win != NULL);

    return l_imp.close(win->data);
}

int yf_window_resize(YF_window win, unsigned width, unsigned height)
{
    assert(l_imp.resize != NULL);
    assert(win != NULL);

    return l_imp.resize(win->data, width, height);
}

int yf_window_toggle(YF_window win)
{
    assert(l_imp.toggle != NULL);
    assert(win != NULL);

    return l_imp.toggle(win->data);
}

int yf_window_settitle(YF_window win, const char *title)
{
    assert(l_imp.settitle != NULL);
    assert(win != NULL);

    return l_imp.settitle(win->data, title);
}

void yf_window_getsize(YF_window win, unsigned *width, unsigned *height)
{
    assert(l_imp.getsize != NULL);
    assert(win != NULL);

    l_imp.getsize(win->data, width, height);
}

void yf_window_deinit(YF_window win)
{
    assert(l_imp.deinit != NULL);

    if (win != NULL) {
        l_imp.deinit(win->data);
        free(win);
    }
}
