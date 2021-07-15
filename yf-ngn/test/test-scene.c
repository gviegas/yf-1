/*
 * YF
 * test-scene.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "yf/wsys/yf-wsys.h"

#include "yf-ngn.h"

#define YF_WINW 960
#define YF_WINH 600
#define YF_WINT "Scene"
#define YF_FPS  60
#define YF_PLACE (YF_vec3){20.0, 20.0, 20.0}
#define YF_POINT (YF_vec3){0}

/* Shared variables. */
struct T_vars {
    YF_window win;
    YF_view view;
    YF_collection coll;
    YF_scene scn;

    struct {
        int quit;
        int place;
        int point;
        int move[6];
        int turn[4];
    } input;
};
static struct T_vars l_vars = {0};

/* Handles key events. */
static void on_key(int key, int state,
                   YF_UNUSED unsigned mod_mask, YF_UNUSED void *arg)
{
    switch (key) {
    case YF_KEY_W:
        l_vars.input.move[0] = state;
        break;
    case YF_KEY_S:
        l_vars.input.move[1] = state;
        break;
    case YF_KEY_A:
        l_vars.input.move[2] = state;
        break;
    case YF_KEY_D:
        l_vars.input.move[3] = state;
        break;
    case YF_KEY_R:
        l_vars.input.move[4] = state;
        break;
    case YF_KEY_F:
        l_vars.input.move[5] = state;
        break;
    case YF_KEY_UP:
        l_vars.input.turn[0] = state;
        break;
    case YF_KEY_DOWN:
        l_vars.input.turn[1] = state;
        break;
    case YF_KEY_LEFT:
        l_vars.input.turn[2] = state;
        break;
    case YF_KEY_RIGHT:
        l_vars.input.turn[3] = state;
        break;
    case YF_KEY_RETURN:
        l_vars.input.place = state;
        break;
    case YF_KEY_SPACE:
        l_vars.input.point = state;
        break;
    case YF_KEY_ESC:
        l_vars.input.quit |= state;
        break;
    default:
        break;
    }
}

/* Updates content. */
static void update(double elapsed_time)
{
    printf("update (%.4f)\n", elapsed_time);

    if (l_vars.input.quit) {
        puts("quit");
        yf_view_stop(l_vars.view);
    }


    YF_camera cam = yf_scene_getcam(l_vars.scn);
    const YF_float md = 10.0 * elapsed_time;
    const YF_float td = 1.5 * elapsed_time;

    if (l_vars.input.place)
        yf_camera_place(cam, YF_PLACE);
    if (l_vars.input.point)
        yf_camera_point(cam, YF_POINT);
    if (l_vars.input.move[0])
        yf_camera_movef(cam, md);
    if (l_vars.input.move[1])
        yf_camera_moveb(cam, md);
    if (l_vars.input.move[2])
        yf_camera_movel(cam, md);
    if (l_vars.input.move[3])
        yf_camera_mover(cam, md);
    if (l_vars.input.move[4])
        yf_camera_moveu(cam, md);
    if (l_vars.input.move[5])
        yf_camera_moved(cam, md);
    if (l_vars.input.turn[0])
        yf_camera_turnu(cam, td);
    if (l_vars.input.turn[1])
        yf_camera_turnd(cam, td);
    if (l_vars.input.turn[2])
        yf_camera_turnl(cam, td);
    if (l_vars.input.turn[3])
        yf_camera_turnr(cam, td);
}

static int traverse(YF_node node, YF_UNUSED void *arg)
{
    char name[2][256];
    size_t n[2] = {256, 256};
    printf("> node '%s' is child of '%s'\n",
           yf_node_getname(node, name[0], n),
           yf_node_getname(yf_node_getparent(node), name[1], n+1));

    return 0;
}

/* Tests scene loading and rendering. */
int yf_test_scene(void)
{
    YF_evtfn evtfn = {.key_kb = on_key};
    yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);

    l_vars.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
    assert(l_vars.win != NULL);

    l_vars.view = yf_view_init(l_vars.win);
    assert(l_vars.view != NULL);

    l_vars.coll = yf_collection_init("tmp/scene.glb");
    assert(l_vars.coll != NULL);

    l_vars.scn = yf_collection_getitem(l_vars.coll, YF_CITEM_SCENE, "Scene");
    assert(l_vars.scn != NULL);

    yf_node_traverse(yf_scene_getnode(l_vars.scn), traverse, NULL);

    yf_scene_setcolor(l_vars.scn, YF_COLOR_LIGHTGREY);
    yf_view_setscene(l_vars.view, l_vars.scn);
    yf_view_start(l_vars.view, YF_FPS, update);

    /* managed... */
    /*yf_scene_deinit(l_vars.scn);*/
    yf_collection_deinit(l_vars.coll);
    yf_view_deinit(l_vars.view);
    yf_window_deinit(l_vars.win);
    return 0;
}
