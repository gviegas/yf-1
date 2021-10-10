/*
 * YF
 * test-model.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "yf/wsys/yf-event.h"
#include "yf/wsys/yf-keyboard.h"

#include "test.h"
#include "yf-ngn.h"

#define YF_WINW 640
#define YF_WINH 480
#define YF_WINT "Model"
#define YF_FPS 60
#define YF_PLACE (YF_vec3){20.0f, 20.0f, 20.0f}
#define YF_POINT (YF_vec3){0}

/* Shared variables. */
struct T_vars {
    YF_window win;
    YF_view view;
    YF_scene scn;
    YF_mesh mesh;
    YF_material matl;
    YF_model mdl;

    struct {
        int quit;
        int place;
        int point;
        int move[6];
        int turn[4];
        int next;
        int prev;
    } input;
};
static struct T_vars vars_ = {0};

/* Model transforms. */
static const YF_mat4 xforms_[] = {
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    },
    {
        1.5f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.5f, 0.0f,
        3.0f, -3.0f, 2.0f, 1.0f
    },
    {
        2.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 2.0f, 0.0f,
        -6.0f, -2.0f, 4.0f, 1.0f
    },
    {
        0.7f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.7f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.7f, 0.0f,
        4.0f, -2.0f, -1.0f, 1.0f
    },
    {
        0.3f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.3f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.3f, 0.0f,
        -2.5f, -2.5f, 0.0f, 1.0f
    },
    {
        0.1f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.1f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.1f, 0.0f,
        2.5f, 0.0f, -1.0f, 1.0f
    }
};
#define YF_XFORMN (sizeof xforms_ / sizeof *xforms_)

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
    case YF_KEY_DOT:
        vars_.input.next = state;
        break;
    case YF_KEY_COMMA:
        vars_.input.prev = state;
        break;
    default:
        vars_.input.quit |= state;
    }
}

/* Updates content. */
static int update(double elapsed_time, YF_UNUSED void *arg)
{
    if (vars_.input.quit)
        return -1;

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

    static size_t xform_i = 0;
    if (vars_.input.next) {
        xform_i = (xform_i + 1) % YF_XFORMN;
        yf_mat4_copy(*yf_node_getxform(yf_model_getnode(vars_.mdl)),
                     xforms_[xform_i]);
        vars_.input.next = 0;
    } else if (vars_.input.prev) {
        xform_i = xform_i > 0 ? (xform_i - 1) % YF_XFORMN : YF_XFORMN - 1;
        yf_mat4_copy(*yf_node_getxform(yf_model_getnode(vars_.mdl)),
                     xforms_[xform_i]);
        vars_.input.prev = 0;
    }

    return 0;
}

/* Tests model. */
int yf_test_model(void)
{
    YF_evtfn evtfn = {.key_kb = on_key};
    yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);

    vars_.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
    assert(vars_.win != NULL);

    vars_.view = yf_view_init(vars_.win);
    assert(vars_.view != NULL);

    vars_.scn = yf_scene_init();
    assert(vars_.scn != NULL);

    vars_.mesh = yf_mesh_init("tmp/cube.glb", 0);
    assert(vars_.mesh != NULL);

    YF_texture tex = yf_texture_init("tmp/cube.png", 0, NULL, YF_UVSET_0);
    assert(tex != NULL);
    YF_matlprop mprop = {
        .pbr = YF_PBR_NONE,
        .nopbr = {{tex}, {1.0f, 1.0f, 1.0f, 1.0f}},
        .alphamode = YF_ALPHAMODE_OPAQUE
    };
    vars_.matl = yf_material_init(&mprop);
    assert(vars_.matl != NULL);

    yf_mesh_setmatl(vars_.mesh, 0, vars_.matl);

    YF_TEST_PRINT("init", "", "mdl");
    vars_.mdl = yf_model_init();
    if (vars_.mdl == NULL)
        return -1;

    YF_TEST_PRINT("getnode", "mdl", "");
    YF_node node = yf_model_getnode(vars_.mdl);
    if (node == NULL)
        return -1;

    YF_TEST_PRINT("getmesh", "mdl", "");
    if (yf_model_getmesh(vars_.mdl) != NULL)
        return -1;

    YF_TEST_PRINT("setmesh", "mdl, mesh", "");
    yf_model_setmesh(vars_.mdl, vars_.mesh);

    YF_TEST_PRINT("getmesh", "mdl", "");
    if (yf_model_getmesh(vars_.mdl) != vars_.mesh)
        return -1;

    YF_skeleton skel;

    YF_TEST_PRINT("getskin", "mdl, &skel", "");
    if (yf_model_getskin(vars_.mdl, &skel) != NULL)
        return -1;

    YF_TEST_PRINT("setskin", "mdl, NULL, NULL", "");
    yf_model_setskin(vars_.mdl, NULL, NULL);

    YF_TEST_PRINT("getskin", "mdl, &skel", "");
    if (yf_model_getskin(vars_.mdl, &skel) != NULL)
        return -1;

    yf_node_insert(yf_scene_getnode(vars_.scn), node);

    if (yf_view_loop(vars_.view, vars_.scn, YF_FPS, update, NULL) != 0)
        assert(0);

    YF_TEST_PRINT("deinit", "mdl", "");
    yf_model_deinit(vars_.mdl);

    yf_view_deinit(vars_.view);
    yf_scene_deinit(vars_.scn);
    yf_mesh_deinit(vars_.mesh);
    yf_texture_deinit(tex);
    yf_material_deinit(vars_.matl);
    yf_window_deinit(vars_.win);

    return 0;
}
