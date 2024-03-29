/*
 * YF
 * test-composition.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "yf/wsys/yf-wsys.h"

#include "yf-ngn.h"

#define YF_WINW 640
#define YF_WINH 480
#define YF_WINT "Composition"
#define YF_FPS  60
#define YF_PLACE (yf_vec3_t){20.0f, 20.0f, 20.0f}
#define YF_POINT (yf_vec3_t){0}

/* Shared variables. */
struct vars {
    yf_window_t *win;
    yf_view_t *view;
    yf_scene_t *scn;
    yf_collec_t *coll;
    yf_model_t *mdl;
    yf_node_t *labl_node;
    yf_label_t *labls[2];
    yf_light_t *light;

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
        int primary;
        int secondary;
    } input;
};
static struct vars vars_ = {0};

/* Handles key events. */
static void on_key(int key, int state,
                   YF_UNUSED unsigned mod_mask, YF_UNUSED void *arg)
{
    switch (key) {
    case YF_KEY_1:
        vars_.input.camera = 1;
        yf_label_setstr(vars_.labls[0], L"MODE: Camera");
        break;
    case YF_KEY_2:
        vars_.input.camera = 0;
        yf_label_setstr(vars_.labls[0], L"MODE: Object");
        break;
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
    case YF_KEY_T:
        vars_.input.toggle = state;
        break;
    default:
        vars_.input.quit |= state;
    }
}

/* Handles motion events. */
static void on_motion(int x, int y, YF_UNUSED void *arg)
{
    vars_.input.x[0] = vars_.input.x[1];
    vars_.input.y[0] = vars_.input.y[1];
    vars_.input.x[1] = x;
    vars_.input.y[1] = y;
}

/* Handles button events. */
static void on_button(int btn, int state, int x, int y, YF_UNUSED void *arg)
{
    switch (btn) {
    case YF_BTN_LEFT:
        vars_.input.primary = state;
        break;
    case YF_BTN_RIGHT:
        vars_.input.secondary = state;
        break;
    }

    vars_.input.x[0] = vars_.input.x[1];
    vars_.input.y[0] = vars_.input.y[1];
    vars_.input.x[1] = x;
    vars_.input.y[1] = y;
}

/* Updates content. */
static int update(double elapsed_time, YF_UNUSED void *arg)
{
    if (vars_.input.quit)
        return -1;

    yf_camera_t *cam = yf_scene_getcam(vars_.scn);

    if (vars_.input.primary) {
        const float ld = 0.25 * elapsed_time;
        const int x0 = vars_.input.x[0];
        const int x1 = vars_.input.x[1];
        const int y0 = vars_.input.y[0];
        const int y1 = vars_.input.y[1];

        if (x1 < x0) {
            yf_camera_turnl(cam, (x0 - x1) * ld);
            vars_.input.x[0] = x1;
        } else if (x1 > x0) {
            yf_camera_turnr(cam, (x1 - x0) * ld);
            vars_.input.x[0] = x1;
        }
        if (y1 < y0) {
            yf_camera_turnu(cam, (y0 - y1) * ld);
            vars_.input.y[0] = y1;
        } else if (y1 > y0) {
            yf_camera_turnd(cam, (y1 - y0) * ld);
            vars_.input.y[0] = y1;
        }
    }

    if (vars_.input.camera) {
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

    } else {
        const float md = 10.0 * elapsed_time;
        const float td = 6.0 * elapsed_time;

        yf_vec3_t t = {0};
        yf_vec4_t r = {0.0f, 0.0f, 0.0f, 1.0f};

        if (vars_.input.place) {
            yf_mat4_t *xform = yf_node_getxform(yf_model_getnode(vars_.mdl));
            (*xform)[12] = (*xform)[13] = (*xform)[14] = 0.0f;
            vars_.input.place = 0;
        }

        if (vars_.input.move[0])
            t[2] += md;
        if (vars_.input.move[1])
            t[2] -= md;
        if (vars_.input.move[2])
            t[0] += md;
        if (vars_.input.move[3])
            t[0] -= md;
        if (vars_.input.move[4])
            t[1] += md;
        if (vars_.input.move[5])
            t[1] -= md;

        if (vars_.input.turn[0]) {
            yf_vec4_t q;
            yf_vec4_rotqx(q, td);
            yf_vec4_mulqi(r, q);
        }
        if (vars_.input.turn[1]) {
            yf_vec4_t q;
            yf_vec4_rotqx(q, -td);
            yf_vec4_mulqi(r, q);
        }
        if (vars_.input.turn[2]) {
            yf_vec4_t q;
            yf_vec4_rotqy(q, td);
            yf_vec4_mulqi(r, q);
        }
        if (vars_.input.turn[3]) {
            yf_vec4_t q;
            yf_vec4_rotqy(q, -td);
            yf_vec4_mulqi(r, q);
        }

        yf_mat4_t mt, mr;
        yf_mat4_xlate(mt, t[0], t[1], t[2]);
        yf_mat4_rotq(mr, r);

        yf_mat4_t m, tr;
        yf_mat4_copy(m, *yf_node_getxform(yf_model_getnode(vars_.mdl)));
        yf_mat4_mul(tr, mt, mr);
        yf_mat4_mul(*yf_node_getxform(yf_model_getnode(vars_.mdl)), m, tr);
    }

    if (vars_.input.toggle) {
        if (yf_node_descends(vars_.labl_node, yf_scene_getnode(vars_.scn)))
            yf_node_drop(vars_.labl_node);
        else
            yf_node_insert(yf_scene_getnode(vars_.scn), vars_.labl_node);
        vars_.input.toggle = 0;
    }

    return 0;
}

/* Tests object composition. */
int yf_test_composition(void)
{
    srand(time(NULL));

    yf_evtfn_t evtfn = {.key_kb = on_key};
    yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);

    evtfn.motion_pt = on_motion;
    yf_setevtfn(YF_EVT_MOTIONPT, evtfn, NULL);

    evtfn.button_pt = on_button;
    yf_setevtfn(YF_EVT_BUTTONPT, evtfn, NULL);

    vars_.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
    assert(vars_.win != NULL);

    vars_.view = yf_view_init(vars_.win);
    assert(vars_.view != NULL);

    vars_.scn = yf_scene_init();
    assert(vars_.scn != NULL);

    vars_.labl_node = yf_node_init();
    assert(vars_.labl_node != NULL);

    vars_.coll = yf_collec_init(NULL);
    assert(vars_.coll != NULL);

    if (yf_collec_manage(vars_.coll, YF_CITEM_SCENE, "scn",
                         vars_.scn) != 0 ||
        yf_collec_manage(vars_.coll, YF_CITEM_NODE, "labl",
                         vars_.labl_node) != 0)
        assert(0);

    yf_mesh_t *mesh1 = yf_collec_loaditem(vars_.coll, YF_CITEM_MESH,
                                          "tmp/cube.glb", 0);
    yf_mesh_t *mesh2 = yf_collec_loaditem(vars_.coll, YF_CITEM_MESH,
                                          "tmp/cube2.glb", 0);
    assert(mesh1 != NULL);
    assert(mesh2 != NULL);

    yf_texture_t *tex1 = yf_collec_loaditem(vars_.coll, YF_CITEM_TEXTURE,
                                            "tmp/cube.png", 0);
    yf_texture_t *tex2 = yf_collec_loaditem(vars_.coll, YF_CITEM_TEXTURE,
                                            "tmp/cube2.png", 0);
    assert(tex1 != NULL);
    assert(tex2 != NULL);

    yf_font_t *font1 = yf_collec_loaditem(vars_.coll, YF_CITEM_FONT,
                                          "tmp/serif.ttf", 0);
    yf_font_t *font2 = yf_collec_loaditem(vars_.coll, YF_CITEM_FONT,
                                          "tmp/sans.ttf", 0);
    assert(font1 != NULL);
    assert(font2 != NULL);

    yf_material_t *matl1 = yf_material_init(NULL);
    yf_material_t *matl2 = yf_material_init(NULL);
    assert(matl1 != NULL);
    assert(matl2 != NULL);
    yf_matlprop_t *mprop;
    mprop = yf_material_getprop(matl1);
    mprop->pbr = YF_PBR_METALROUGH;
    mprop->pbrmr.color_tex.tex = tex1;
    mprop = yf_material_getprop(matl2);
    mprop->pbr = YF_PBR_METALROUGH;
    mprop->pbrmr.color_tex.tex = tex2;

    if (yf_collec_manage(vars_.coll, YF_CITEM_MATERIAL, "m1", matl1) != 0 ||
        yf_collec_manage(vars_.coll, YF_CITEM_MATERIAL, "m2", matl2) != 0)
        assert(0);

    vars_.mdl = yf_model_init();
    assert(vars_.mdl != NULL);

    const int mdl_num = rand()&1 ? 1 : 2;
    yf_model_setmesh(vars_.mdl, mdl_num == 1 ? mesh1 : mesh2);
    yf_mesh_setmatl(yf_model_getmesh(vars_.mdl), 0, mdl_num == 1 ? matl1 :
                                                                   matl2);

    yf_node_insert(yf_scene_getnode(vars_.scn), yf_model_getnode(vars_.mdl));

    const size_t labl_n = sizeof vars_.labls / sizeof vars_.labls[0];
    for (size_t i = 0; i < labl_n; i++) {
        vars_.labls[i] = yf_label_init();
        assert(vars_.labls[i] != NULL);

        yf_label_setfont(vars_.labls[i], i ? font2 : font1);
        yf_mat4_t *m = yf_node_getxform(yf_label_getnode(vars_.labls[i]));

        switch (i) {
        case 0:
            yf_label_setstr(vars_.labls[i], L"MODE: Object");
            yf_label_setpt(vars_.labls[i], 24);
            (*m)[12] = -0.7;
            (*m)[13] = -0.9;
            break;

        case 1:
            yf_label_setstr(vars_.labls[i], L"test-composition");
            yf_label_setpt(vars_.labls[i], 18);
            yf_label_setcolor(vars_.labls[i], YF_CORNER_ALL, YF_COLOR_BLACK);
            (*m)[12] = 0.75;
            (*m)[13] = 0.9;
            break;

        default:
            yf_label_setstr(vars_.labls[i], L"label");
            yf_label_setpt(vars_.labls[i], 24+i*12);
            (*m)[12] = i*-0.15;
            (*m)[13] = i*-0.15;
        }

        if (yf_collec_getitem(vars_.coll, YF_CITEM_NODE, "labl")
            != vars_.labl_node)
            assert(0);

        yf_node_insert(vars_.labl_node, yf_label_getnode(vars_.labls[i]));
    }

    if (yf_collec_getitem(vars_.coll, YF_CITEM_SCENE, "scn") != vars_.scn)
        assert(0);

    yf_node_insert(yf_scene_getnode(vars_.scn), vars_.labl_node);

    const yf_vec3_t light_clr = {1.0f, 1.0f, 1.0f};
    vars_.light = yf_light_init(YF_LIGHTT_SPOT, light_clr, 1000.0f, 100.0f,
                                0.0f, 0.785398f);
    assert(vars_.light != NULL);

    yf_mat4_t t;
    yf_mat4_xlate(t, 10.0f, 10.0f, 10.0f);

    yf_vec4_t qx, qy;
    yf_vec4_rotqx(qx, 3.141593f * -0.25f);
    yf_vec4_rotqy(qy, 3.141593f * 0.25f);
    yf_vec4_mulqi(qx, qy);

    yf_mat4_t r;
    yf_mat4_rotq(r, qx);

    yf_mat4_mul(*yf_node_getxform(yf_light_getnode(vars_.light)), t, r);

    if (yf_collec_manage(vars_.coll, YF_CITEM_NODE, "light",
                         yf_light_getnode(vars_.light)) != 0)
        assert(0);

    yf_node_insert(yf_scene_getnode(vars_.scn), yf_light_getnode(vars_.light));

    yf_camera_t *cam = yf_scene_getcam(vars_.scn);
    const yf_vec3_t pos = {-4.0f, 6.0f, 15.0f};
    const yf_vec3_t tgt = {0};
    yf_camera_place(cam, pos);
    yf_camera_point(cam, tgt);

    if (yf_view_loop(vars_.view, vars_.scn, YF_FPS, update, NULL) != 0)
        assert(0);

    yf_view_deinit(vars_.view);
    yf_window_deinit(vars_.win);

    yf_model_deinit(vars_.mdl);
    for (size_t i = 0; i < labl_n; i++)
        yf_label_deinit(vars_.labls[i]);

    yf_collec_deinit(vars_.coll);

    return 0;
}
