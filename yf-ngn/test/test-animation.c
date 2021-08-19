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
#include "yf-ngn.h"

#define YF_WINW 640
#define YF_WINH 480
#define YF_WINT "Animation"
#define YF_FPS  60
#define YF_PLACE (YF_vec3){20.0f, 20.0f, 20.0f}
#define YF_POINT (YF_vec3){0}

/* Shared variables. */
struct T_vars {
    YF_window win;
    YF_view view;
    YF_collection coll;
    YF_animation anim;
    YF_scene scn;

    struct {
        int quit;
        int place;
        int point;
        int move[6];
        int turn[4];
        int play;
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

    if (vars_.input.play) {
        static float dt = 0.0f;
        char s[128];
        float rem = yf_animation_apply(vars_.anim, dt);

        snprintf(s, sizeof s >> 1, "anim, %.6f", dt);
        snprintf(s+(sizeof s >> 1), sizeof s >> 1, "%.6f", rem);
        YF_TEST_PRINT("apply", s, s+64);

        if (rem < -1.0f)
            dt = 0.0f;
        else
            dt += elapsed_time;
    }
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

static int each_anim(const char *name, void *anim, YF_UNUSED void *arg)
{
    printf("\nanimation '%s':\n", name);

    unsigned in_n, out_n, act_n;
    const YF_kfinput *ins = yf_animation_getins(anim, &in_n);
    const YF_kfoutput *outs = yf_animation_getouts(anim, &out_n);
    const YF_kfaction *acts = yf_animation_getacts(anim, &act_n);

    for (unsigned i = 0; i < in_n; i++) {
        printf(" input #%u (%u sample(s)):\n", i, ins[i].n);
        for (unsigned j = 0; j < ins[i].n; j++)
            printf("  %.4f\n", ins[i].timeline[j]);
    }

    for (unsigned i = 0; i < out_n; i++) {
        printf(" output #%u (%u sample(s)):\n", i, outs[i].n);
        switch (outs[i].kfprop) {
        case YF_KFPROP_T:
            printf("  (T)\n");
            for (unsigned j = 0; j < outs[i].n; j++)
                printf("  [%.4f, %.4f, %.4f]\n",
                       outs[i].t[j][0], outs[i].t[j][1], outs[i].t[j][2]);
            break;
        case YF_KFPROP_R:
            printf("  (R)\n");
            for (unsigned j = 0; j < outs[i].n; j++)
                printf("  [%.4f, %.4f, %.4f, %.4f]\n",
                       outs[i].r[j][0], outs[i].r[j][1], outs[i].r[j][2],
                       outs[i].r[j][3]);
            break;
        case YF_KFPROP_S:
            printf("  (S)\n");
            for (unsigned j = 0; j < outs[i].n; j++)
                printf("  [%.4f, %.4f, %.4f]\n",
                       outs[i].s[j][0], outs[i].s[j][1], outs[i].s[j][2]);
            break;
        default:
            assert(0);
        }
    }

    for (unsigned i = 0; i < act_n; i++) {
        printf(" action #%u:\n", i);
        printf("  kferp: %s\n",
               acts[i].kferp == YF_KFERP_STEP ? "step" :
               (acts[i].kferp == YF_KFERP_LINEAR ? "linear" : "ERR"));
        printf("  in_i: %u\n  out_i: %u\n", acts[i].in_i, acts[i].out_i);
        YF_node node = yf_animation_gettarget(anim, i);
        if (node == NULL)
            printf("  (no target set for this action)\n");
        else {
            char name[256];
            size_t n = 256;
            printf("  target node is '%s'\n", yf_node_getname(node, name, &n));
            if (yf_node_isleaf(node))
                printf("  (is a leaf node)\n");
            else
                yf_node_traverse(node, traverse, NULL);
        }

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
/* TODO: More tests. */
int yf_test_animation(void)
{
    YF_evtfn evtfn = {.key_kb = on_key};
    yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);

    vars_.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
    assert(vars_.win != NULL);

    vars_.view = yf_view_init(vars_.win);
    assert(vars_.view != NULL);

    puts("\n- collection animation loading -\n");

    vars_.coll = yf_collection_init("tmp/animation.glb");
    assert(vars_.coll != NULL);

    yf_collection_each(vars_.coll, YF_CITEM_ANIMATION, each_anim, NULL);

#if 1
    vars_.anim = yf_collection_getitem(vars_.coll, YF_CITEM_ANIMATION,
                                       "Animation");
    assert(vars_.anim != NULL);

    vars_.scn = yf_collection_getitem(vars_.coll, YF_CITEM_SCENE, "Scene");
    assert(vars_.scn != NULL);

    yf_node_traverse(yf_scene_getnode(vars_.scn), traverse, NULL);

    yf_scene_setcolor(vars_.scn, YF_COLOR_DARKGREY);
    yf_view_setscene(vars_.view, vars_.scn);
    yf_view_start(vars_.view, YF_FPS, update);
#endif

    puts("\n- no explicit 'deinit()' call for managed animation -\n");

    /* managed... */
    /*yf_animation_deinit(vars_.anim);*/
    /*yf_scene_deinit(vars_.scn);*/
    yf_collection_deinit(vars_.coll);
    yf_view_deinit(vars_.view);
    yf_window_deinit(vars_.win);
    return 0;
}
