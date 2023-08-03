/*
 * YF
 * test-animation.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "yf/wsys/yf-event.h"
#include "yf/wsys/yf-keyboard.h"

#include "test.h"
#include "print.h"
#include "yf-ngn.h"

#define YF_WINW 640
#define YF_WINH 480
#define YF_WINT "Animation"
#define YF_FPS  60
#define YF_PLACE (yf_vec3_t){20.0f, 20.0f, 20.0f}
#define YF_POINT (yf_vec3_t){0}

/* Shared variables. */
struct vars {
    yf_window_t *win;
    yf_view_t *view;
    yf_collec_t *coll;
    yf_kfanim_t *anim;
    yf_scene_t *scn;

    struct {
        int quit;
        int place;
        int point;
        int move[6];
        int turn[4];
        int play;
    } input;
};
static struct vars vars_ = {0};

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
        if (state == YF_KEYSTATE_PRESSED)
            vars_.input.play = !vars_.input.play;
        break;
    case YF_KEY_ESC:
        vars_.input.quit |= state;
        break;
    default:
        break;
    }
}

/* Updates content. */
static int update(double elapsed_time, YF_UNUSED void *arg)
{
    if (vars_.input.quit)
        return -1;

    yf_camera_t *cam = yf_scene_getcam(vars_.scn);
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

    if (vars_.input.play) {
        static float dt = 0.0f;
        char s[128];
        float rem = yf_kfanim_apply(vars_.anim, dt);

        snprintf(s, sizeof s >> 1, "anim, %.6f", dt);
        snprintf(s+(sizeof s >> 1), sizeof s >> 1, "%.6f", rem);
        YF_TEST_PRINT("apply", s, s+64);

        if (rem < -1.0f)
            dt = 0.0f;
        else
            dt += elapsed_time;
    }

    return 0;
}

static int traverse(yf_node_t *node, YF_UNUSED void *arg)
{
    char name[2][256];
    size_t n[2] = {256, 256};
    printf(" node '%s' is child of '%s'\n",
           yf_node_getname(node, name[0], n),
           yf_node_getname(yf_node_getparent(node), name[1], n+1));

    return 0;
}

static int each_anim(YF_UNUSED const char *name,
                     void *anim,
                     YF_UNUSED void *arg)
{
    yf_print_anim(anim);

    unsigned out_n, act_n;
    const yf_kfout_t *outs = yf_kfanim_getouts(anim, &out_n);
    const yf_kfact_t *acts = yf_kfanim_getacts(anim, &act_n);

    for (unsigned i = 0; i < act_n; i++) {
        yf_node_t *node = yf_kfanim_gettarget(anim, i);
        if (node == NULL)
            continue;

        const unsigned out_i = acts[i].out_i;
        const unsigned val_i = outs[out_i].n - 1;

        switch (outs[out_i].kfprop) {
        case YF_KFPROP_T:
            yf_vec3_copy(*yf_node_gett(node), outs[out_i].t[val_i]);
            break;
        case YF_KFPROP_R:
            yf_vec4_copy(*yf_node_getr(node), outs[out_i].r[val_i]);
            break;
        case YF_KFPROP_S:
            yf_vec3_copy(*yf_node_gets(node), outs[out_i].s[val_i]);
            break;
        default:
            assert(0);
        }
    }

    return 0;
}

/* Tests animation. */
int yf_test_animation(void)
{
    yf_evtfn_t evtfn = {.key_kb = on_key};
    yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);

    vars_.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
    assert(vars_.win != NULL);

    vars_.view = yf_view_init(vars_.win);
    assert(vars_.view != NULL);

    yf_node_t *tgt = yf_node_init();
    assert(tgt != NULL);

    float tmln[] = {0.0f, 0.0333f, 0.0667f};

    yf_vec3_t tvals[] = {
        {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}
    };

    yf_vec4_t rvals[3] = {0};
    yf_vec4_rotqx(rvals[0], 3.141593f * 0.5f);
    yf_vec4_rotqy(rvals[1], 3.141593f * 0.5f);
    yf_vec4_rotqz(rvals[2], 3.141593f * 0.5f);

    yf_vec3_t svals[] = {
        {0.5f, 1.0f, 1.0f}, {1.0f, 0.5f, 1.0f}, {1.0f, 1.0f, 0.5f}
    };

    yf_kfin_t ins[] = {
        {.timeline = tmln, .n = 1},
        {.timeline = tmln, .n = 3}
    };

    yf_kfout_t outs[] = {
        {.kfprop = YF_KFPROP_T, .t = tvals, .n = 1},
        {.kfprop = YF_KFPROP_R, .r = rvals, .n = 1},
        {.kfprop = YF_KFPROP_S, .s = svals, .n = 1},
        {.kfprop = YF_KFPROP_T, .t = tvals, .n = 3},
        {.kfprop = YF_KFPROP_R, .r = rvals, .n = 3},
        {.kfprop = YF_KFPROP_S, .s = svals, .n = 3}
    };

    yf_kfact_t acts[] = {
        {.kferp = YF_KFERP_STEP, .in_i = 0, .out_i = 0},
        {.kferp = YF_KFERP_STEP, .in_i = 0, .out_i = 1},
        {.kferp = YF_KFERP_STEP, .in_i = 0, .out_i = 2},
        {.kferp = YF_KFERP_LINEAR, .in_i = 1, .out_i = 3},
        {.kferp = YF_KFERP_LINEAR, .in_i = 1, .out_i = 4},
        {.kferp = YF_KFERP_LINEAR, .in_i = 1, .out_i = 5}
    };

    unsigned in_n, out_n, act_n;

    YF_TEST_PRINT("init", "ins, 1, outs, 2, acts, 2", "anim");
    yf_kfanim_t *anim = yf_kfanim_init(ins, 1, outs, 2, acts, 2);
    if (anim == NULL)
        return -1;

    YF_TEST_PRINT("gettarget", "anim, 1", "");
    if (yf_kfanim_gettarget(anim, 1) != NULL)
        return -1;

    YF_TEST_PRINT("gettarget", "anim, 0", "");
    if (yf_kfanim_gettarget(anim, 0) != NULL)
        return -1;

    YF_TEST_PRINT("settarget", "anim, 0, tgt", "");
    yf_kfanim_settarget(anim, 0, tgt);

    YF_TEST_PRINT("settarget", "anim, 1, tgt", "");
    yf_kfanim_settarget(anim, 1, tgt);

    YF_TEST_PRINT("gettarget", "anim, 1", "");
    if (yf_kfanim_gettarget(anim, 1) != tgt)
        return -1;

    YF_TEST_PRINT("gettarget", "anim, 0", "");
    if (yf_kfanim_gettarget(anim, 0) != tgt)
        return -1;

    YF_TEST_PRINT("getins", "anim, &in_n", "");
    if (yf_kfanim_getins(anim, &in_n) == NULL || in_n != 1)
        return -1;

    YF_TEST_PRINT("getouts", "anim, &out_n", "");
    if (yf_kfanim_getouts(anim, &out_n) == NULL || out_n != 2)
        return -1;

    YF_TEST_PRINT("getacts", "anim, &act_n", "");
    if (yf_kfanim_getacts(anim, &act_n) == NULL || act_n != 2)
        return -1;

    yf_print_anim(anim);

    YF_TEST_PRINT("init", "ins, 2, outs, 6, acts, 6", "anim2");
    yf_kfanim_t *anim2 = yf_kfanim_init(ins, 2, outs, 6, acts, 6);
    if (anim2 == NULL)
        return -1;

    YF_TEST_PRINT("gettarget", "anim2, 0", "");
    if (yf_kfanim_gettarget(anim2, 0) != NULL)
        return -1;

    YF_TEST_PRINT("gettarget", "anim2, 2", "");
    if (yf_kfanim_gettarget(anim2, 2) != NULL)
        return -1;

    YF_TEST_PRINT("gettarget", "anim2, 5", "");
    if (yf_kfanim_gettarget(anim2, 5) != NULL)
        return -1;

    YF_TEST_PRINT("settarget", "anim2, 0, tgt", "");
    yf_kfanim_settarget(anim2, 0, tgt);

    YF_TEST_PRINT("settarget", "anim2, 2, tgt", "");
    yf_kfanim_settarget(anim2, 2, tgt);

    YF_TEST_PRINT("settarget", "anim2, 5, tgt", "");
    yf_kfanim_settarget(anim2, 5, tgt);

    YF_TEST_PRINT("settarget", "anim2, 2, NULL", "");
    yf_kfanim_settarget(anim2, 2, NULL);

    YF_TEST_PRINT("gettarget", "anim2, 0", "");
    if (yf_kfanim_gettarget(anim2, 0) != tgt)
        return -1;

    YF_TEST_PRINT("gettarget", "anim2, 2", "");
    if (yf_kfanim_gettarget(anim2, 2) != NULL)
        return -1;

    YF_TEST_PRINT("gettarget", "anim2, 5", "");
    if (yf_kfanim_gettarget(anim2, 5) != tgt)
        return -1;

    YF_TEST_PRINT("getins", "anim2, &in_n", "");
    if (yf_kfanim_getins(anim2, &in_n) == NULL || in_n != 2)
        return -1;

    YF_TEST_PRINT("getouts", "anim2, &out_n", "");
    if (yf_kfanim_getouts(anim2, &out_n) == NULL || out_n != 6)
        return -1;

    YF_TEST_PRINT("getacts", "anim2, &act_n", "");
    if (yf_kfanim_getacts(anim2, &act_n) == NULL || act_n != 6)
        return -1;

    yf_print_anim(anim2);

    YF_TEST_PRINT("deinit", "anim", "");
    yf_kfanim_deinit(anim);

    YF_TEST_PRINT("deinit", "anim2", "");
    yf_kfanim_deinit(anim2);

    yf_node_deinit(tgt);

    puts("\n- collection animation loading -\n");

    vars_.coll = yf_collec_init("tmp/animation.glb");
    assert(vars_.coll != NULL);

    yf_collec_each(vars_.coll, YF_CITEM_KFANIM, each_anim, NULL);

    vars_.anim = yf_collec_getitem(vars_.coll, YF_CITEM_KFANIM, "Animation");
    assert(vars_.anim != NULL);

    vars_.scn = yf_collec_getitem(vars_.coll, YF_CITEM_SCENE, "Scene");
    assert(vars_.scn != NULL);

    const yf_vec3_t light_clr = {1.0f, 1.0f, 1.0f};
    yf_light_t *light = yf_light_init(YF_LIGHTT_POINT, light_clr, 1000.0f,
                                      100.0f, 0.0f, 0.0f);
    assert(light != NULL);
    yf_mat4_xlate(*yf_node_getxform(yf_light_getnode(light)),
                  10.0f, 10.0f, 10.0f);
    yf_node_insert(yf_scene_getnode(vars_.scn), yf_light_getnode(light));

    YF_TEST_PRINT("traverse", "getnode(scn), traverse, NULL", "");
    yf_node_traverse(yf_scene_getnode(vars_.scn), traverse, NULL);

    if (yf_view_loop(vars_.view, vars_.scn, YF_FPS, update, NULL) != 0)
        assert(0);

    puts("\n- no explicit 'deinit()' call for managed animation -\n");

    yf_collec_deinit(vars_.coll);
    yf_light_deinit(light);
    yf_view_deinit(vars_.view);
    yf_window_deinit(vars_.win);

    return 0;
}
