/*
 * YF
 * test-terrain.c
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
#define YF_WINT "Terrain"
#define YF_FPS  60
#define YF_PLACE (YF_vec3){20.0f, 20.0f, 20.0f}
#define YF_POINT (YF_vec3){0}

/* Shared variables. */
struct T_vars {
    YF_window win;
    YF_view view;
    YF_scene scn;
    YF_terrain terr;
    YF_texture hmap;
    YF_texture tex;

    struct {
        int quit;
        int place;
        int point;
        int move[6];
        int turn[4];
    } input;
};
static struct T_vars vars_ = {0};

/* Handles key events. */
static void on_key(int key, int state,
                   YF_UNUSED unsigned mod_mask, YF_UNUSED void *arg)
{
    switch (key) {
    case YF_KEY_RETURN:
        vars_.input.place = state;
        break;
    case YF_KEY_SPACE:
        vars_.input.point = state;
        break;
    case YF_KEY_W:
        vars_.input.move[0] = state;
        break;
    case YF_KEY_S:
        vars_.input.move[1] = state;
        break;
    case YF_KEY_A:
        vars_.input.move[2] = state;
        break;
    case YF_KEY_D:
        vars_.input.move[3] = state;
        break;
    case YF_KEY_R:
        vars_.input.move[4] = state;
        break;
    case YF_KEY_F:
        vars_.input.move[5] = state;
        break;
    case YF_KEY_UP:
        vars_.input.turn[0] = state;
        break;
    case YF_KEY_DOWN:
        vars_.input.turn[1] = state;
        break;
    case YF_KEY_LEFT:
        vars_.input.turn[2] = state;
        break;
    case YF_KEY_RIGHT:
        vars_.input.turn[3] = state;
        break;
    default:
        vars_.input.quit |= state;
    }
}

/* Updates content. */
static void update(double elapsed_time)
{
    if (vars_.input.quit)
        yf_view_stop(vars_.view);

    YF_camera cam = yf_scene_getcam(vars_.scn);
    const float md = 16.0 * elapsed_time;
    const float td = 2.0 * elapsed_time;

    if (vars_.input.place)
        yf_camera_place(cam, YF_PLACE);
    if (vars_.input.point)
        yf_camera_point(cam, YF_POINT);
    if (vars_.input.move[0])
        yf_camera_movef(cam, md);
    if (vars_.input.move[1])
        yf_camera_moveb(cam, md);
    if (vars_.input.move[2])
        yf_camera_movel(cam, md);
    if (vars_.input.move[3])
        yf_camera_mover(cam, md);
    if (vars_.input.move[4])
        yf_camera_moveu(cam, md);
    if (vars_.input.move[5])
        yf_camera_moved(cam, md);
    if (vars_.input.turn[0])
        yf_camera_turnu(cam, td);
    if (vars_.input.turn[1])
        yf_camera_turnd(cam, td);
    if (vars_.input.turn[2])
        yf_camera_turnl(cam, td);
    if (vars_.input.turn[3])
        yf_camera_turnr(cam, td);
}

/* Tests terrain. */
int yf_test_terrain(void)
{
    YF_evtfn evtfn = {.key_kb = on_key};
    yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);

    vars_.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
    assert(vars_.win != NULL);

    vars_.view = yf_view_init(vars_.win);
    assert(vars_.view != NULL);

    vars_.scn = yf_scene_init();
    assert(vars_.scn != NULL);

    vars_.hmap = yf_texture_init(YF_FILETYPE_PNG, "tmp/hmap.png");
    assert(vars_.hmap != NULL);

    vars_.tex = yf_texture_init(YF_FILETYPE_PNG, "tmp/terrain.png");
    assert(vars_.tex != NULL);

    YF_TEST_PRINT("init", "160, 160", "terr");
    vars_.terr = yf_terrain_init(160, 160);
    if (vars_.terr == NULL)
        return -1;

    YF_TEST_PRINT("getnode", "terr", "");
    YF_node node = yf_terrain_getnode(vars_.terr);
    if (node == NULL)
        return -1;

    yf_mat4_scale(*yf_node_getxform(node), 3.0f, 3.0f, 3.0f);

    YF_TEST_PRINT("getmesh", "terr", "");
    if (yf_terrain_getmesh(vars_.terr) == NULL)
        return -1;

    YF_TEST_PRINT("gethmap", "terr", "");
    if (yf_terrain_gethmap(vars_.terr) != NULL)
        return -1;

    YF_TEST_PRINT("sethmap", "terr, hmap", "");
    yf_terrain_sethmap(vars_.terr, vars_.hmap);

    YF_TEST_PRINT("gethmap", "terr", "");
    if (yf_terrain_gethmap(vars_.terr) != vars_.hmap)
        return -1;

    YF_TEST_PRINT("gettex", "terr", "");
    if (yf_terrain_gettex(vars_.terr) != NULL)
        return -1;

    YF_TEST_PRINT("settex", "terr, tex", "");
    yf_terrain_settex(vars_.terr, vars_.tex);

    YF_TEST_PRINT("gettex", "terr", "");
    if (yf_terrain_gettex(vars_.terr) != vars_.tex)
        return -1;

    yf_node_insert(yf_scene_getnode(vars_.scn), node);

    YF_camera cam = yf_scene_getcam(vars_.scn);
    const YF_vec3 pos = {-2.0f, 20.0f, 10.0f};
    const YF_vec3 tgt = {0};
    yf_camera_place(cam, pos);
    yf_camera_point(cam, tgt);

    yf_view_setscene(vars_.view, vars_.scn);

    if (yf_view_start(vars_.view, YF_FPS, update) != 0)
        assert(0);

    YF_TEST_PRINT("deinit", "terr", "");
    yf_terrain_deinit(vars_.terr);

    yf_view_deinit(vars_.view);
    yf_scene_deinit(vars_.scn);
    yf_texture_deinit(vars_.hmap);
    yf_texture_deinit(vars_.tex);
    yf_window_deinit(vars_.win);

    return 0;
}
