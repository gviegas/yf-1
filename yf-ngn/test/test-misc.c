/*
 * YF
 * test-misc.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "yf/wsys/yf-wsys.h"

#include "yf-ngn.h"

#define YF_WINW 960
#define YF_WINH 600
#define YF_WINT "Misc"
#define YF_FPS  60
#define YF_PLACE (YF_vec3){20.0, 20.0, 20.0}
#define YF_POINT (YF_vec3){0}

/* Shared variables. */
struct T_vars {
    YF_window win;
    YF_view view;
    YF_scene scn;
    YF_collection coll;
    YF_model mdl;
    YF_node labl_node;
    YF_label labls[2];

    struct {
        int camera;
        int place;
        int point;
        int move[6];
        int turn[4];
        int toggle;
        int quit;
        int x[2];
        int y[2];
    } input;
};
static struct T_vars l_vars = {0};

/* Handles key events. */
static void on_key(int key, int state,
                   YF_UNUSED unsigned mod_mask, YF_UNUSED void *arg)
{
    switch (key) {
    case YF_KEY_1:
        l_vars.input.camera = 1;
        yf_label_setstr(l_vars.labls[0], L"MODE: Camera");
        break;
    case YF_KEY_2:
        l_vars.input.camera = 0;
        yf_label_setstr(l_vars.labls[0], L"MODE: Object");
        break;
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
    case YF_KEY_T:
        l_vars.input.toggle = state;
        break;
    default:
        l_vars.input.quit |= state;
    }
}

/* Handles motion events. */
static void on_motion(int x, int y, YF_UNUSED void *arg)
{
    l_vars.input.x[0] = l_vars.input.x[1];
    l_vars.input.y[0] = l_vars.input.y[1];
    l_vars.input.x[1] = x;
    l_vars.input.y[1] = y;
}

/* Updates content. */
static void update(double elapsed_time)
{
    printf("update (%.4f)\n", elapsed_time);

    if (l_vars.input.quit) {
        puts("quit");
        yf_view_stop(l_vars.view);
    }

    if (l_vars.input.camera) {
        YF_camera cam = yf_scene_getcam(l_vars.scn);
        const YF_float md = 20.0 * elapsed_time;
        const YF_float td = 2.0 * elapsed_time;

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

        const YF_float ld = 0.5 * elapsed_time;
        const int x0 = l_vars.input.x[0];
        const int x1 = l_vars.input.x[1];
        const int y0 = l_vars.input.y[0];
        const int y1 = l_vars.input.y[1];

        if (x1 < x0) {
            yf_camera_turnl(cam, ld);
            l_vars.input.x[0] = x1;
        } else if (x1 > x0) {
            yf_camera_turnr(cam, ld);
            l_vars.input.x[0] = x1;
        }
        if (y1 < y0) {
            yf_camera_turnu(cam, ld);
            l_vars.input.y[0] = y1;
        } else if (y1 > y0) {
            yf_camera_turnd(cam, ld);
            l_vars.input.y[0] = y1;
        }

    } else {
        const YF_float d = 6.0 * elapsed_time;
        const YF_float a = 3.14159265358979 * elapsed_time;

        YF_vec3 t = {0};
        YF_vec4 r = {0.0, 0.0, 0.0, 1.0};

        if (l_vars.input.place) {
            YF_mat4 *xform = yf_node_getxform(yf_model_getnode(l_vars.mdl));
            (*xform)[12] = (*xform)[13] = (*xform)[14] = 0.0;
            l_vars.input.place = 0;
        }

        if (l_vars.input.move[0])
            t[2] += d;
        if (l_vars.input.move[1])
            t[2] -= d;
        if (l_vars.input.move[2])
            t[0] += d;
        if (l_vars.input.move[3])
            t[0] -= d;
        if (l_vars.input.move[4])
            t[1] += d;
        if (l_vars.input.move[5])
            t[1] -= d;

        if (l_vars.input.turn[0]) {
            YF_vec4 q;
            yf_vec4_rotqx(q, a);
            yf_vec4_mulqi(r, q);
        }
        if (l_vars.input.turn[1]) {
            YF_vec4 q;
            yf_vec4_rotqx(q, -a);
            yf_vec4_mulqi(r, q);
        }
        if (l_vars.input.turn[2]) {
            YF_vec4 q;
            yf_vec4_rotqy(q, a);
            yf_vec4_mulqi(r, q);
        }
        if (l_vars.input.turn[3]) {
            YF_vec4 q;
            yf_vec4_rotqy(q, -a);
            yf_vec4_mulqi(r, q);
        }

        YF_mat4 mt, mr;
        yf_mat4_xlate(mt, t[0], t[1], t[2]);
        yf_mat4_rotq(mr, r);

        YF_mat4 m, tr;
        yf_mat4_copy(m, *yf_node_getxform(yf_model_getnode(l_vars.mdl)));
        yf_mat4_mul(tr, mt, mr);
        yf_mat4_mul(*yf_node_getxform(yf_model_getnode(l_vars.mdl)), m, tr);
    }

    if (l_vars.input.toggle) {
        if (yf_node_descends(l_vars.labl_node, yf_scene_getnode(l_vars.scn)))
            yf_node_drop(l_vars.labl_node);
        else
            yf_node_insert(yf_scene_getnode(l_vars.scn), l_vars.labl_node);
        l_vars.input.toggle = 0;
    }
}

/* Tests miscellany. */
int yf_test_misc(void)
{
    srand(time(NULL));

    YF_evtfn evtfn = {.key_kb = on_key};
    yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);

    evtfn.motion_pt = on_motion;
    yf_setevtfn(YF_EVT_MOTIONPT, evtfn, NULL);

    l_vars.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
    assert(l_vars.win != NULL);

    l_vars.view = yf_view_init(l_vars.win);
    assert(l_vars.view != NULL);

    l_vars.scn = yf_scene_init();
    assert(l_vars.scn != NULL);

    l_vars.labl_node = yf_node_init();
    assert(l_vars.labl_node != NULL);

    YF_mesh mesh1 = yf_mesh_init(YF_FILETYPE_GLTF, "tmp/model1.gltf", 0);
    assert(mesh1 != NULL);
    YF_mesh mesh2 = yf_mesh_init(YF_FILETYPE_GLTF, "tmp/model2.gltf", 0);
    assert(mesh2 != NULL);

    YF_texture tex1 = yf_texture_init(YF_FILETYPE_PNG, "tmp/model1.png");
    assert(tex1 != NULL);
    YF_texture tex2 = yf_texture_init(YF_FILETYPE_PNG, "tmp/model2.png");
    assert(tex2 != NULL);

    YF_font font1 = yf_font_init(YF_FILETYPE_TTF, "tmp/serif.ttf");
    assert(font1 != NULL);
    YF_font font2 = yf_font_init(YF_FILETYPE_TTF, "tmp/sans.ttf");
    assert(font2 != NULL);

    l_vars.coll = yf_collection_init(NULL);
    assert(l_vars.coll != NULL);

    if (yf_collection_manage(l_vars.coll, YF_CITEM_SCENE, "scn",
                             l_vars.scn) != 0 ||
        yf_collection_manage(l_vars.coll, YF_CITEM_NODE, "labl",
                             l_vars.labl_node) != 0 ||
        yf_collection_manage(l_vars.coll, YF_CITEM_MESH, "m1",
                             mesh1) != 0 ||
        yf_collection_manage(l_vars.coll, YF_CITEM_MESH, "m2",
                             mesh2) != 0 ||
        yf_collection_manage(l_vars.coll, YF_CITEM_TEXTURE, "t1",
                             tex1) != 0 ||
        yf_collection_manage(l_vars.coll, YF_CITEM_TEXTURE, "t2",
                             tex2) != 0 ||
        yf_collection_manage(l_vars.coll, YF_CITEM_FONT, "f1",
                             font1) != 0 ||
        yf_collection_manage(l_vars.coll, YF_CITEM_FONT, "f2",
                             font2) != 0)
        assert(0);

    YF_material matl1 = yf_material_init(NULL);
    assert(matl1 != NULL);
    YF_material matl2 = yf_material_init(NULL);
    assert(matl2 != NULL);
    YF_matlprop *mprop;
    mprop = yf_material_getprop(matl1);
    mprop->pbr = YF_PBR_METALROUGH;
    mprop->pbrmr.color_tex = yf_collection_getres(l_vars.coll,
                                                  YF_CITEM_TEXTURE, "t1");
    mprop = yf_material_getprop(matl2);
    mprop->pbr = YF_PBR_METALROUGH;
    mprop->pbrmr.color_tex = yf_collection_getres(l_vars.coll,
                                                  YF_CITEM_TEXTURE, "t2");

    if (yf_collection_manage(l_vars.coll, YF_CITEM_MATERIAL, "m1",
                             matl1) != 0 ||
        yf_collection_manage(l_vars.coll, YF_CITEM_MATERIAL, "m2",
                             matl2) != 0)
        assert(0);

    l_vars.mdl = yf_model_init();
    assert(l_vars.mdl != NULL);

    const int mdl_num = rand()&1 ? 1 : 2;
    yf_model_setmesh(l_vars.mdl,
                     yf_collection_getres(l_vars.coll, YF_CITEM_MESH,
                                          mdl_num == 1 ? "m1" : "m2"));
    yf_model_setmatl(l_vars.mdl,
                     yf_collection_getres(l_vars.coll, YF_CITEM_MATERIAL,
                                          mdl_num == 1 ? "m1" : "m2"));

    yf_node_insert(yf_scene_getnode(l_vars.scn), yf_model_getnode(l_vars.mdl));

    const size_t labl_n = sizeof l_vars.labls / sizeof l_vars.labls[0];
    for (size_t i = 0; i < labl_n; i++) {
        l_vars.labls[i] = yf_label_init();
        assert(l_vars.labls[i] != NULL);

        yf_label_setfont(l_vars.labls[i],
                         yf_collection_getres(l_vars.coll, YF_CITEM_FONT,
                                              i == 0 ? "f1" : "f2"));
        YF_mat4 *m = yf_node_getxform(yf_label_getnode(l_vars.labls[i]));

        switch (i) {
        case 0:
            yf_label_setstr(l_vars.labls[i], L"MODE: Object");
            yf_label_setpt(l_vars.labls[i], 24);
            (*m)[12] = -0.75;
            (*m)[13] = -0.9;
            break;

        case 1:
            yf_label_setstr(l_vars.labls[i], L"test-misc");
            yf_label_setpt(l_vars.labls[i], 18);
            yf_label_setcolor(l_vars.labls[i], YF_CORNER_ALL, YF_COLOR_BLACK);
            (*m)[12] = 0.85;
            (*m)[13] = 0.9;
            break;

        default:
            yf_label_setstr(l_vars.labls[i], L"label");
            yf_label_setpt(l_vars.labls[i], 24+i*12);
            (*m)[12] = i*-0.15;
            (*m)[13] = i*-0.15;
        }

        if (yf_collection_getres(l_vars.coll, YF_CITEM_NODE,
                                 "labl") != l_vars.labl_node)
            assert(0);

        yf_node_insert(l_vars.labl_node, yf_label_getnode(l_vars.labls[i]));
    }

    if (yf_collection_getres(l_vars.coll, YF_CITEM_SCENE,
                             "scn") != l_vars.scn)
        assert(0);

    yf_node_insert(yf_scene_getnode(l_vars.scn), l_vars.labl_node);

    YF_camera cam = yf_scene_getcam(l_vars.scn);
    const YF_vec3 pos = {-4.0, 6.0, 15.0};
    const YF_vec3 tgt = {0};
    yf_camera_place(cam, pos);
    yf_camera_point(cam, tgt);

    yf_scene_setcolor(l_vars.scn, YF_COLOR_DARKGREY);
    yf_view_setscene(l_vars.view, l_vars.scn);

    if (yf_view_start(l_vars.view, YF_FPS, update) != 0)
        assert(0);

    yf_view_deinit(l_vars.view);
    /* managed... */
    /*yf_scene_deinit(l_vars.scn);*/
    yf_window_deinit(l_vars.win);
    /* managed... */
    /*yf_node_deinit(l_vars.labl_node);*/

    yf_model_deinit(l_vars.mdl);
    for (size_t i = 0; i < labl_n; i++)
        yf_label_deinit(l_vars.labls[i]);

    yf_collection_deinit(l_vars.coll);

    return 0;
}
