/*
 * YF
 * test-rendering.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "yf/wsys/yf-event.h"
#include "yf/wsys/yf-keyboard.h"

#include "yf-ngn.h"

#define YF_WINW 640
#define YF_WINH 480
#define YF_WINT "Rendering"
#define YF_FPS  60
#define YF_MDLN_1 10
#define YF_MDLN_2 5
#define YF_PLACE (YF_vec3){20.0f, 20.0f, 20.0f}
#define YF_POINT (YF_vec3){0}

/* Shared variables. */
struct T_vars {
    YF_window win;
    YF_view view;
    YF_scene scn1;
    YF_scene scn2;
    YF_mesh mesh1;
    YF_mesh mesh2;
    YF_texture tex1;
    YF_texture tex2;
    YF_material matl1;
    YF_material matl2;
    YF_model mdls1[YF_MDLN_1];
    YF_model mdls2[YF_MDLN_2];
    YF_light light1;
    YF_light light2;

    struct {
        int quit;
        int swap;
        int place;
        int point;
        int move[6];
        int turn[4];
        int drop;
        int insert;
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
    case YF_KEY_TAB:
        vars_.input.swap = state;
        break;
    case YF_KEY_X:
        vars_.input.drop = state;
        break;
    case YF_KEY_Z:
        vars_.input.insert = state;
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

    static unsigned scn_i = 1;
    if (vars_.input.swap) {
        vars_.input.swap = 0;
        if (scn_i != 1) {
            yf_view_setscene(vars_.view, vars_.scn1);
            scn_i = 1;
        } else {
            yf_view_setscene(vars_.view, vars_.scn2);
            scn_i = 2;
        }
        return;
    }

    YF_camera cam = yf_scene_getcam(scn_i == 1 ? vars_.scn1 : vars_.scn2);
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

    if (scn_i == 1)
        return;

    if (vars_.input.drop && YF_MDLN_2 > 1) {
        vars_.input.drop = 0;
        static size_t prev = YF_MDLN_2 >> 1;
        size_t next = (prev + 1) % YF_MDLN_2;
        yf_node_drop(yf_model_getnode(vars_.mdls2[next]));
        prev = next;
    }

    if (vars_.input.insert && YF_MDLN_2 > 0) {
        vars_.input.insert = 0;
        yf_node_insert(yf_scene_getnode(vars_.scn2),
                       yf_model_getnode(vars_.mdls2[0]));
        for (size_t i = 1; i < YF_MDLN_2; i++)
            yf_node_insert(yf_model_getnode(vars_.mdls2[i-1]),
                           yf_model_getnode(vars_.mdls2[i]));
    }
}

/* Tests rendering. */
int yf_test_rendering(void)
{
    YF_evtfn evtfn = {.key_kb = on_key};
    yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);

    vars_.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
    assert(vars_.win != NULL);

    vars_.view = yf_view_init(vars_.win);
    assert(vars_.view != NULL);

    vars_.scn1 = yf_scene_init();
    vars_.scn2 = yf_scene_init();
    assert(vars_.scn1 != NULL);
    assert(vars_.scn2 != NULL);

    vars_.mesh1 = yf_mesh_init("tmp/cube.glb", 0);
    vars_.mesh2 = yf_mesh_init("tmp/cube2.glb", 0);
    assert(vars_.mesh1 != NULL);
    assert(vars_.mesh2 != NULL);

    vars_.tex1 = yf_texture_init("tmp/cube.png");
    vars_.tex2 = yf_texture_init("tmp/cube2.png");
    assert(vars_.tex1 != NULL);
    assert(vars_.tex2 != NULL);

    vars_.matl1 = yf_material_init(NULL);
    vars_.matl2 = yf_material_init(NULL);
    assert(vars_.matl1 != NULL);
    assert(vars_.matl2 != NULL);
    YF_matlprop *mprop;
    mprop = yf_material_getprop(vars_.matl1);
    mprop->pbr = YF_PBR_METALROUGH;
    mprop->pbrmr.color_tex = vars_.tex1;
    mprop = yf_material_getprop(vars_.matl2);
    mprop->pbr = YF_PBR_METALROUGH;
    mprop->pbrmr.color_tex = vars_.tex2;

    YF_node scn1_nd = yf_scene_getnode(vars_.scn1);
    float tf = -YF_MDLN_1;
    for (size_t i = 0; i < YF_MDLN_1; i++) {
        vars_.mdls1[i] = yf_model_init();
        assert(vars_.mdls1[i] != NULL);

        yf_model_setmesh(vars_.mdls1[i], vars_.mesh1);
        yf_model_setmatl(vars_.mdls1[i], vars_.matl1);

        YF_node nd = yf_model_getnode(vars_.mdls1[i]);
        YF_mat4 *m = yf_node_getxform(nd);
        yf_mat4_xlate(*m, tf, tf*0.5f, 0.0f);
        tf += 2.0f;

        yf_node_insert(scn1_nd, nd);
    }

    YF_node scn2_nd = yf_scene_getnode(vars_.scn2);
    for (size_t i = 0; i < YF_MDLN_2; i++) {
        vars_.mdls2[i] = yf_model_init();
        assert(vars_.mdls2[i] != NULL);

        yf_model_setmesh(vars_.mdls2[i], vars_.mesh2);
        yf_model_setmatl(vars_.mdls2[i], vars_.matl2);

        YF_node nd = yf_model_getnode(vars_.mdls2[i]);
        YF_mat4 *m = yf_node_getxform(nd);
        YF_mat4 t, r, s, tr;
        YF_vec4 q;
        yf_mat4_xlate(t, 0.0f, 0.0f, -2.0f);
        yf_vec4_rotqz(q, M_PI_4);
        yf_mat4_rotq(r, q);
        yf_mat4_scale(s, 1.2f, 1.2f, 1.0f);
        yf_mat4_mul(tr, t, r);
        yf_mat4_mul(*m, tr, s);

        if (i == 0)
            yf_node_insert(scn2_nd, nd);
        else
            yf_node_insert(yf_model_getnode(vars_.mdls2[i-1]), nd);
    }

    const YF_vec3 light_clr = {1.0f, 1.0f, 1.0f};
    vars_.light1 = yf_light_init(YF_LIGHTT_DIRECT, light_clr, 3.0f,
                                 0.0f, 0.0f, 0.0f);
    vars_.light2 = yf_light_init(YF_LIGHTT_DIRECT, light_clr, 3.0f,
                                 0.0f, 0.0f, 0.0f);
    assert(vars_.light1 != NULL);
    assert(vars_.light2 != NULL);

    YF_vec4 qx, qy;
    yf_vec4_rotqx(qx, 3.141593f * -0.25f);
    yf_vec4_rotqy(qy, 3.141593f * 0.25f);
    yf_vec4_mulqi(qx, qy);

    YF_mat4 r;
    yf_mat4_rotq(r, qx);
    yf_mat4_copy(*yf_node_getxform(yf_light_getnode(vars_.light1)), r);
    yf_mat4_copy(*yf_node_getxform(yf_light_getnode(vars_.light2)), r);

    yf_node_insert(yf_scene_getnode(vars_.scn1),
                   yf_light_getnode(vars_.light1));
    yf_node_insert(yf_scene_getnode(vars_.scn2),
                   yf_light_getnode(vars_.light2));

    yf_scene_setcolor(vars_.scn1, YF_COLOR_YELLOW);
    yf_scene_setcolor(vars_.scn2, YF_COLOR_BLUE);

    yf_view_setscene(vars_.view, vars_.scn1);
    yf_view_start(vars_.view, YF_FPS, update, NULL);

    for (size_t i = 0; i < YF_MDLN_1; i++)
        yf_model_deinit(vars_.mdls1[i]);
    for (size_t i = 0; i < YF_MDLN_2; i++)
        yf_model_deinit(vars_.mdls2[i]);

    yf_light_deinit(vars_.light1);
    yf_light_deinit(vars_.light2);
    yf_material_deinit(vars_.matl1);
    yf_material_deinit(vars_.matl2);
    yf_texture_deinit(vars_.tex1);
    yf_texture_deinit(vars_.tex2);
    yf_mesh_deinit(vars_.mesh1);
    yf_mesh_deinit(vars_.mesh2);
    yf_scene_deinit(vars_.scn1);
    yf_scene_deinit(vars_.scn2);
    yf_view_deinit(vars_.view);
    yf_window_deinit(vars_.win);

    return 0;
}
