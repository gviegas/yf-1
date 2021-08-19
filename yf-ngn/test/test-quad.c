/*
 * YF
 * test-quad.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "yf/wsys/yf-event.h"
#include "yf/wsys/yf-keyboard.h"

#include "test.h"
#include "yf-ngn.h"

#define YF_WINW 640
#define YF_WINH 480
#define YF_WINT "Quad"
#define YF_FPS  60

/* Shared variables. */
struct T_vars {
    YF_window win;
    YF_view view;
    YF_scene scn;
    YF_quad quad;
    YF_texture tex;

    struct {
        int quit;
    } input;
};
static struct T_vars vars_ = {0};

/* Handles key events. */
static void on_key(int key, int state,
                   YF_UNUSED unsigned mod_mask, YF_UNUSED void *arg)
{
    if (state == YF_KEYSTATE_RELEASED)
        return;

    switch (key) {
    default:
        vars_.input.quit = 1;
    }
}

/* Updates content. */
static void update(YF_UNUSED double elapsed_time)
{
    if (vars_.input.quit)
        yf_view_stop(vars_.view);
}

/* Tests quad. */
/* TODO: More tests. */
int yf_test_quad(void)
{
    YF_evtfn evtfn = {.key_kb = on_key};
    yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);

    vars_.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
    assert(vars_.win != NULL);

    vars_.view = yf_view_init(vars_.win);
    assert(vars_.view != NULL);

    vars_.scn = yf_scene_init();
    assert(vars_.scn != NULL);

    vars_.tex = yf_texture_init(YF_FILETYPE_PNG, "tmp/quad.png");
    assert(vars_.tex != NULL);

    YF_TEST_PRINT("init", "", "quad");
    vars_.quad = yf_quad_init();
    if (vars_.quad == NULL)
        return -1;

    YF_TEST_PRINT("getnode", "quad", "");
    YF_node node = yf_quad_getnode(vars_.quad);
    if (node == NULL)
        return -1;

    YF_TEST_PRINT("getmesh", "quad", "");
    if (yf_quad_getmesh(vars_.quad) == NULL)
        return -1;

    YF_TEST_PRINT("gettex", "quad", "");
    if (yf_quad_gettex(vars_.quad) != NULL)
        return -1;

    YF_TEST_PRINT("settex", "quad, tex", "");
    yf_quad_settex(vars_.quad, vars_.tex);

    YF_TEST_PRINT("gettex", "quad", "");
    if (yf_quad_gettex(vars_.quad) != vars_.tex)
        return -1;

    yf_node_insert(yf_scene_getnode(vars_.scn), node);

    yf_view_setscene(vars_.view, vars_.scn);

    if (yf_view_start(vars_.view, YF_FPS, update) != 0)
        assert(0);

    YF_TEST_PRINT("deinit", "quad", "");
    yf_quad_deinit(vars_.quad);

    yf_view_deinit(vars_.view);
    yf_scene_deinit(vars_.scn);
    yf_texture_deinit(vars_.tex);
    yf_window_deinit(vars_.win);

    return 0;
}
