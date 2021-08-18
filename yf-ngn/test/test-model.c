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
static struct T_vars l_vars = {0};

/* Model transforms. */
static const YF_mat4 l_xforms[] = {
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
#define YF_XFORMN (sizeof l_xforms / sizeof *l_xforms)

/* Handles key events. */
static void on_key(int key, int state,
                   YF_UNUSED unsigned mod_mask, YF_UNUSED void *arg)
{
    switch (key) {
    case YF_KEY_RETURN:
        l_vars.input.place = state;
        break;
    case YF_KEY_SPACE:
        l_vars.input.point = state;
        break;
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
    case YF_KEY_DOT:
        l_vars.input.next = state;
        break;
    case YF_KEY_COMMA:
        l_vars.input.prev = state;
        break;
    default:
        l_vars.input.quit |= state;
    }
}

/* Updates content. */
static void update(double elapsed_time)
{
    if (l_vars.input.quit)
        yf_view_stop(l_vars.view);

    YF_camera cam = yf_scene_getcam(l_vars.scn);
    const float md = 16.0 * elapsed_time;
    const float td = 2.0 * elapsed_time;

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

    static size_t xform_i = 0;
    if (l_vars.input.next) {
        xform_i = (xform_i + 1) % YF_XFORMN;
        yf_mat4_copy(*yf_node_getxform(yf_model_getnode(l_vars.mdl)),
                     l_xforms[xform_i]);
        l_vars.input.next = 0;
    } else if (l_vars.input.prev) {
        xform_i = xform_i > 0 ? (xform_i - 1) % YF_XFORMN : YF_XFORMN - 1;
        yf_mat4_copy(*yf_node_getxform(yf_model_getnode(l_vars.mdl)),
                     l_xforms[xform_i]);
        l_vars.input.prev = 0;
    }
}

/* Tests model. */
int yf_test_model(void)
{
    YF_evtfn evtfn = {.key_kb = on_key};
    yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);

    l_vars.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
    assert(l_vars.win != NULL);

    l_vars.view = yf_view_init(l_vars.win);
    assert(l_vars.view != NULL);

    l_vars.scn = yf_scene_init();
    assert(l_vars.scn != NULL);

    l_vars.mesh = yf_mesh_init(YF_FILETYPE_GLTF, "tmp/cube.glb", 0);
    assert(l_vars.mesh != NULL);

    YF_texture tex = yf_texture_init(YF_FILETYPE_PNG, "tmp/cube.png");
    assert(tex != NULL);
    YF_matlprop mprop = {
        .pbr = YF_PBR_METALROUGH,
        .pbrmr = {tex, {1.0f, 1.0f, 1.0f, 1.0f}, NULL, 1.0f, 0.0f},
        .normal = {0},
        .occlusion = {0},
        .emissive = {0},
        .alphamode = YF_ALPHAMODE_OPAQUE
    };
    l_vars.matl = yf_material_init(&mprop);
    assert(l_vars.matl != NULL);

    YF_TEST_PRINT("init", "", "mdl");
    l_vars.mdl = yf_model_init();
    if (l_vars.mdl == NULL)
        return -1;

    YF_TEST_PRINT("getnode", "mdl", "");
    YF_node node = yf_model_getnode(l_vars.mdl);
    if (node == NULL)
        return -1;

    YF_TEST_PRINT("getmesh", "mdl", "");
    if (yf_model_getmesh(l_vars.mdl) != NULL)
        return -1;

    YF_TEST_PRINT("setmesh", "mdl, mesh", "");
    yf_model_setmesh(l_vars.mdl, l_vars.mesh);

    YF_TEST_PRINT("getmesh", "mdl", "");
    if (yf_model_getmesh(l_vars.mdl) != l_vars.mesh)
        return -1;

    YF_skeleton skel;

    YF_TEST_PRINT("getskin", "mdl, &skel", "");
    if (yf_model_getskin(l_vars.mdl, &skel) != NULL)
        return -1;

    YF_TEST_PRINT("setskin", "mdl, NULL, NULL", "");
    yf_model_setskin(l_vars.mdl, NULL, NULL);

    YF_TEST_PRINT("getskin", "mdl, &skel", "");
    if (yf_model_getskin(l_vars.mdl, &skel) != NULL)
        return -1;

    YF_TEST_PRINT("getmatl", "mdl", "");
    if (yf_model_getmatl(l_vars.mdl) != NULL)
        return -1;

    YF_TEST_PRINT("setmatl", "mdl, matl", "");
    yf_model_setmatl(l_vars.mdl, l_vars.matl);

    YF_TEST_PRINT("getmatl", "mdl", "");
    if (yf_model_getmatl(l_vars.mdl) != l_vars.matl)
        return -1;

    yf_node_insert(yf_scene_getnode(l_vars.scn), node);
    yf_view_setscene(l_vars.view, l_vars.scn);
    if (yf_view_start(l_vars.view, YF_FPS, update) != 0)
        assert(0);

    YF_TEST_PRINT("deinit", "mdl", "");
    yf_model_deinit(l_vars.mdl);

    yf_view_deinit(l_vars.view);
    yf_scene_deinit(l_vars.scn);
    yf_mesh_deinit(l_vars.mesh);
    yf_texture_deinit(tex);
    yf_material_deinit(l_vars.matl);
    yf_window_deinit(l_vars.win);

    return 0;
}
