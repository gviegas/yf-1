/*
 * YF
 * test-scene.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "yf/wsys/yf-event.h"
#include "yf/wsys/yf-keyboard.h"

#include "test.h"
#include "yf-ngn.h"

#define YF_WINW 640
#define YF_WINH 480
#define YF_WINT "Scene"
#define YF_FPS  60
#define YF_PLACE (YF_vec3){20.0f, 20.0f, 20.0f}
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
static struct T_vars vars_ = {0};

/* Handles key events. */
static void on_key(int key, int state,
                   YF_UNUSED unsigned mod_mask, YF_UNUSED void *arg)
{
    switch (key) {
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
    case YF_KEY_RETURN:
        vars_.input.place = state;
        break;
    case YF_KEY_SPACE:
        vars_.input.point = state;
        break;
    case YF_KEY_ESC:
        vars_.input.quit |= state;
        break;
    default:
        break;
    }
}

/* Updates content. */
static void update(double elapsed_time, YF_UNUSED void *arg)
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

static int traverse(YF_node node, YF_UNUSED void *arg)
{
    char name[2][256];
    size_t n[2] = {256, 256};
    printf(" node '%s' is child of '%s'\n",
           yf_node_getname(node, name[0], n),
           yf_node_getname(yf_node_getparent(node), name[1], n+1));

    return 0;
}

/* Tests scene. */
int yf_test_scene(void)
{
    YF_evtfn evtfn = {.key_kb = on_key};
    yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);

    vars_.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
    assert(vars_.win != NULL);

    vars_.view = yf_view_init(vars_.win);
    assert(vars_.view != NULL);

    YF_TEST_PRINT("init", "", "scn");
    YF_scene scn = yf_scene_init();
    if (scn == NULL)
        return -1;

    YF_TEST_PRINT("getnode", "scn", "");
    if (yf_scene_getnode(scn) == NULL)
        return -1;

    YF_TEST_PRINT("getcam", "scn", "");
    if (yf_scene_getcam(scn) == NULL)
        return -1;

    YF_TEST_PRINT("getcolor", "scn", "");
    YF_color color = yf_scene_getcolor(scn);
    if (memcmp(&color, &YF_COLOR_DARKGREY, sizeof color) != 0)
        return -1;

    YF_TEST_PRINT("setcolor", "scn, COLOR_YELLOW", "");
    yf_scene_setcolor(scn, YF_COLOR_YELLOW);

    YF_TEST_PRINT("getcolor", "scn", "");
    color = yf_scene_getcolor(scn);
    if (memcmp(&color, &YF_COLOR_YELLOW, sizeof color) != 0)
        return -1;

    YF_TEST_PRINT("deinit", "scn", "");
    yf_scene_deinit(scn);

    puts("\n- collection scene loading -\n");

    vars_.coll = yf_collection_init("tmp/scene.glb");
    assert(vars_.coll != NULL);

    vars_.scn = yf_collection_getitem(vars_.coll, YF_CITEM_SCENE, "Scene");
    assert(vars_.scn != NULL);

    YF_TEST_PRINT("getnode", "scn", "node");
    YF_node node = yf_scene_getnode(vars_.scn);
    if (node == NULL)
        return -1;

    const YF_vec3 light_clr = {1.0f, 1.0f, 1.0f};
    YF_light light = yf_light_init(YF_LIGHTT_POINT, light_clr, 1000.0f, 100.0f,
                                   0.0f, 0.0f);
    assert(light != NULL);
    YF_node light_node = yf_light_getnode(light);
    yf_mat4_xlate(*yf_node_getxform(light_node), 10.0f, 10.0f, 10.0f);
    yf_node_insert(node, light_node);

    YF_TEST_PRINT("traverse", "node, traverse, NULL", "");
    yf_node_traverse(node, traverse, NULL);

    yf_view_setscene(vars_.view, vars_.scn);
    yf_view_start(vars_.view, YF_FPS, update, NULL);

    puts("\n- no explicit 'deinit()' call for managed scene -\n");

    yf_collection_deinit(vars_.coll);
    yf_light_deinit(light);
    yf_view_deinit(vars_.view);
    yf_window_deinit(vars_.win);

    return 0;
}
