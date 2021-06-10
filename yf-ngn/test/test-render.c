/*
 * YF
 * test-render.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "yf/wsys/yf-event.h"
#include "yf/wsys/yf-keyboard.h"

#include "yf-ngn.h"

#define YF_WINW 600
#define YF_WINH 480
#define YF_WINT "test-render"
#define YF_FPS  60
#define YF_MDLN 10
#define YF_PLACE (YF_vec3){0.0, 0.0, 20.0}
#define YF_POINT (YF_vec3){0}

/* Local variables. */
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
    YF_model mdls[YF_MDLN];

    struct {
        int quit;
        int swap;
        int place;
        int point;
        int move[4];
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
    case YF_KEY_TAB:
        l_vars.input.swap = state;
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

    static unsigned scn_i = 1;
    if (l_vars.input.swap) {
        l_vars.input.swap = 0;
        if (scn_i != 1) {
            yf_view_setscene(l_vars.view, l_vars.scn1);
            scn_i = 1;
        } else {
            yf_view_setscene(l_vars.view, l_vars.scn2);
            scn_i = 2;
        }
        return;
    }

    YF_camera cam = yf_scene_getcam(scn_i == 1 ? l_vars.scn1 : l_vars.scn2);
    const YF_float md = 9.0 * elapsed_time;
    const YF_float td = 1.0 * elapsed_time;

    if (l_vars.input.move[0])
        yf_camera_movef(cam, md);
    if (l_vars.input.move[1])
        yf_camera_moveb(cam, md);
    if (l_vars.input.move[2])
        yf_camera_movel(cam, md);
    if (l_vars.input.move[3])
        yf_camera_mover(cam, md);
    if (l_vars.input.turn[0])
        yf_camera_turnu(cam, td);
    if (l_vars.input.turn[1])
        yf_camera_turnd(cam, td);
    if (l_vars.input.turn[2])
        yf_camera_turnl(cam, td);
    if (l_vars.input.turn[3])
        yf_camera_turnr(cam, td);

    if (l_vars.input.place) {
        yf_camera_place(cam, YF_PLACE);
        l_vars.input.place = 0;
    }
    if (l_vars.input.point) {
        yf_camera_point(cam, YF_POINT);
        l_vars.input.point = 0;
    }
}

/* Tests rendering. */
int yf_test_render(void)
{
    YF_evtfn evtfn = {.key_kb = on_key};
    yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);

    l_vars.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
    assert(l_vars.win != NULL);

    l_vars.view = yf_view_init(l_vars.win);
    assert(l_vars.view != NULL);

    l_vars.scn1 = yf_scene_init();
    l_vars.scn2 = yf_scene_init();
    assert(l_vars.scn1 != NULL);
    assert(l_vars.scn2 != NULL);

    l_vars.mesh1 = yf_mesh_init(YF_FILETYPE_GLTF, "tmp/cube.gltf");
    l_vars.mesh2 = yf_mesh_init(YF_FILETYPE_GLTF, "tmp/cube.gltf");
    assert(l_vars.mesh1 != NULL);
    assert(l_vars.mesh2 != NULL);

    l_vars.tex1 = yf_texture_init(YF_FILETYPE_PNG, "tmp/cube.png");
    l_vars.tex2 = yf_texture_init(YF_FILETYPE_PNG, "tmp/cube.png");
    assert(l_vars.tex1 != NULL);
    assert(l_vars.tex2 != NULL);

    l_vars.matl1 = yf_material_init();
    l_vars.matl2 = yf_material_init();
    assert(l_vars.matl1 != NULL);
    assert(l_vars.matl2 != NULL);
    YF_matlprop *mprop;
    mprop = yf_material_getprop(l_vars.matl1);
    mprop->pbr = YF_PBR_METALROUGH;
    mprop->pbrmr.color_tex = l_vars.tex1;
    mprop = yf_material_getprop(l_vars.matl2);
    mprop->pbr = YF_PBR_METALROUGH;
    mprop->pbrmr.color_tex = l_vars.tex1;

    YF_node scn1_nd = yf_scene_getnode(l_vars.scn1);
    YF_float tf = -YF_MDLN;
    for (size_t i = 0; i < YF_MDLN; i++) {
        l_vars.mdls[i] = yf_model_init();
        assert(l_vars.mdls[i] != NULL);

        yf_model_setmesh(l_vars.mdls[i], l_vars.mesh1);
        yf_model_setmatl(l_vars.mdls[i], l_vars.matl1);

        YF_node nd = yf_model_getnode(l_vars.mdls[i]);
        YF_mat4 *m = yf_node_getxform(nd);
        yf_mat4_xlate(*m, tf, tf*0.5, 0.0);
        tf += 2.0;

        yf_node_insert(scn1_nd, nd);
    }

    yf_scene_setcolor(l_vars.scn1, YF_COLOR_YELLOW);
    yf_scene_setcolor(l_vars.scn2, YF_COLOR_BLUE);

    yf_view_setscene(l_vars.view, l_vars.scn1);
    yf_view_start(l_vars.view, YF_FPS, update);

    for (size_t i = 0; i < YF_MDLN; i++)
        yf_model_deinit(l_vars.mdls[i]);
    yf_material_deinit(l_vars.matl1);
    yf_material_deinit(l_vars.matl2);
    yf_texture_deinit(l_vars.tex1);
    yf_texture_deinit(l_vars.tex2);
    yf_mesh_deinit(l_vars.mesh1);
    yf_mesh_deinit(l_vars.mesh2);
    yf_scene_deinit(l_vars.scn1);
    yf_scene_deinit(l_vars.scn2);
    yf_view_deinit(l_vars.view);
    yf_window_deinit(l_vars.win);

    return 0;
}
