/*
 * YF
 * test-view.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "yf/com/yf-error.h"

#include "test.h"
#include "yf-view.h"

static YF_view view_ = NULL;
static YF_window wins_[2] = {0};
static YF_scene scns_[2] = {0};

static int update(double elapsed_time, void *arg)
{
    static double tm = 0.0;
    static int swapped = 0;

    if (tm > 1.25) {
        tm = 0.0;
        swapped = 0;
        return -1;
    }

    if (tm > 0.625 && !swapped) {
        size_t new = (size_t)arg;
        size_t cur = (new + 1) % 2;
        char s1[128], *s2 = s1 + 64;
        snprintf(s1, 64, "view_, scns_[%zu]", new);
        snprintf(s2, 64, "scns_[%zu]", cur);
        YF_TEST_PRINT("swap", s1, s2);
        if (yf_view_swap(view_, scns_[new]) != scns_[cur])
            assert(0);
        swapped = 1;
    }

    tm += elapsed_time;
    return 0;
}

/* Tests view. */
int yf_test_view(void)
{
    wins_[0] = yf_window_init(480, 300, "View (a)", 0);
    wins_[1] = yf_window_init(360, 360, "View (b)", 0);
    assert(wins_[0] != NULL && wins_[1] != NULL);

    scns_[0] = yf_scene_init();
    scns_[1] = yf_scene_init();
    assert(scns_[0] != NULL && scns_[1] != NULL);

    yf_scene_setcolor(scns_[0], YF_COLOR_RED);
    yf_scene_setcolor(scns_[1], YF_COLOR_GREEN);

    YF_TEST_PRINT("init", "wins_[0]", "view_");
    view_ = yf_view_init(wins_[0]);
    if (view_ == NULL)
        return -1;

    YF_TEST_PRINT("loop", "view_, scns_[0], 30, update, 1", "");
    if (yf_view_loop(view_, scns_[0], 30, update, (void *)1) != 0)
        return -1;

    YF_TEST_PRINT("init", "wins_[1]", "(nil)");
    yf_seterr(YF_ERR_UNKNOWN, NULL);
    if (yf_view_init(wins_[1]) != NULL || yf_geterr() != YF_ERR_EXIST)
        return -1;
    yf_printerr();

    YF_TEST_PRINT("deinit", "view_", "");
    yf_view_deinit(view_);

    YF_TEST_PRINT("init", "wins_[1]", "view_");
    view_ = yf_view_init(wins_[1]);
    if (view_ == NULL)
        return -1;

    YF_TEST_PRINT("loop", "view_, scns_[1], 30, update, 0", "");
    if (yf_view_loop(view_, scns_[1], 30, update, 0) != 0)
        return -1;

    YF_TEST_PRINT("deinit", "view_", "");
    yf_view_deinit(view_);

    yf_scene_deinit(scns_[0]);
    yf_scene_deinit(scns_[1]);
    yf_window_deinit(wins_[0]);
    yf_window_deinit(wins_[1]);

    return 0;
}
