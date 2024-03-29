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
struct vars {
    yf_window_t *win;
    yf_view_t *view;
    yf_scene_t *scn;
    yf_quad_t *quad;
    yf_texture_t *tex;

    struct {
        int quit;
    } input;
};
static struct vars vars_ = {0};

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
static int update(YF_UNUSED double elapsed_time, YF_UNUSED void *arg)
{
    if (vars_.input.quit)
        return -1;

    static double tm = 0.0;
    static double tm2 = 0.0;
    tm += elapsed_time;
    tm2 += elapsed_time;

    if (tm > 0.3333) {
        static unsigned idx = 0;
        const struct { char *params; yf_color_t color; } call_info[] = {
            {"quad, CORNER_ALL, COLOR_BLUE", YF_COLOR_BLUE},
            {"quad, CORNER_ALL, COLOR_GREEN", YF_COLOR_GREEN},
            {"quad, CORNER_ALL, COLOR_RED", YF_COLOR_RED},
            {"quad, CORNER_ALL, COLOR_WHITE", YF_COLOR_WHITE},
            {"quad, CORNER_ALL, COLOR_TRANSPARENT", YF_COLOR_TRANSPARENT}
        };

        YF_TEST_PRINT("setcolor", call_info[idx].params, "");
        yf_quad_setcolor(vars_.quad, YF_CORNER_ALL, call_info[idx].color);

        YF_TEST_PRINT("getcolor", "quad, CORNER_ALL", "");
        yf_color_t clr = yf_quad_getcolor(vars_.quad, YF_CORNER_ALL);
        assert(clr.r == call_info[idx].color.r &&
               clr.g == call_info[idx].color.g &&
               clr.b == call_info[idx].color.b &&
               clr.a == call_info[idx].color.a);

        idx = (idx + 1) % (sizeof call_info / sizeof *call_info);
        tm = 0.0;
    }

    if (tm2 > 2.0) {
        static unsigned sub = 0;
        yf_rect_t rect = {{0}, yf_texture_getdim(yf_quad_gettex(vars_.quad))};
        if (++sub & 1) {
            rect.size.width >>= 2;
            rect.size.height >>= 2;
            rect.origin.x = rect.size.width + (rect.size.width >> 1);
            rect.origin.y = rect.size.height + (rect.size.height >> 1);
        }

        YF_TEST_PRINT("setrect", "quad, &rect", "");
        yf_quad_setrect(vars_.quad, &rect);

        YF_TEST_PRINT("getrect", "quad", "");
        const yf_rect_t *other = yf_quad_getrect(vars_.quad);
        assert(other->origin.x == rect.origin.x &&
               other->origin.y == rect.origin.y &&
               other->size.width == rect.size.width &&
               other->size.height == rect.size.height);

        tm2 = 0.0;
    }

    return 0;
}

/* Tests quad. */
int yf_test_quad(void)
{
    yf_evtfn_t evtfn = {.key_kb = on_key};
    yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);

    vars_.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
    assert(vars_.win != NULL);

    vars_.view = yf_view_init(vars_.win);
    assert(vars_.view != NULL);

    vars_.scn = yf_scene_init();
    assert(vars_.scn != NULL);

    vars_.tex = yf_texture_load("tmp/quad.png", 0, NULL);
    assert(vars_.tex != NULL);

    YF_TEST_PRINT("init", "", "quad");
    vars_.quad = yf_quad_init();
    if (vars_.quad == NULL)
        return -1;

    YF_TEST_PRINT("getnode", "quad", "");
    yf_node_t *node = yf_quad_getnode(vars_.quad);
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

    YF_TEST_PRINT("getrect", "quad", "");
    const yf_rect_t *rect = yf_quad_getrect(vars_.quad);
    yf_dim2_t dim = yf_texture_getdim(vars_.tex);
    if (rect->origin.x != 0 || rect->origin.y != 0 ||
        rect->size.width != dim.width || rect->size.height != dim.height)
        return -1;

    YF_TEST_PRINT("getcolor", "quad, CORNER_ALL", "");
    yf_color_t clr = yf_quad_getcolor(vars_.quad, YF_CORNER_ALL);
    yf_color_t white = YF_COLOR_WHITE;
    if (clr.r != white.r || clr.g != white.g || clr.b != white.b ||
        clr.a != white.a)
        return -1;

    yf_node_insert(yf_scene_getnode(vars_.scn), node);

    if (yf_view_loop(vars_.view, vars_.scn, YF_FPS, update, NULL) != 0)
        assert(0);

    YF_TEST_PRINT("deinit", "quad", "");
    yf_quad_deinit(vars_.quad);

    yf_view_deinit(vars_.view);
    yf_scene_deinit(vars_.scn);
    yf_texture_deinit(vars_.tex);
    yf_window_deinit(vars_.win);

    return 0;
}
