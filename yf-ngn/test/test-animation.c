/*
 * YF
 * test-animation.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "yf/wsys/yf-wsys.h"

#include "yf-ngn.h"

#define YF_WINW 960
#define YF_WINH 600
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
    case YF_KEY_TAB:
        if (state == YF_KEYSTATE_PRESSED)
            l_vars.input.play = !l_vars.input.play;
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
    const float md = 10.0 * elapsed_time;
    const float td = 1.5 * elapsed_time;

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

    if (l_vars.input.play) {
        static float dt = 0.0f;
        if (dt > 1.0f) {
            float rem = yf_animation_apply(l_vars.anim, dt);
            if (rem < 0.0f)
                dt = 0.0f;
        }
        dt += elapsed_time;
    }
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
int yf_test_animation(void)
{
    YF_evtfn evtfn = {.key_kb = on_key};
    yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);

    l_vars.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
    assert(l_vars.win != NULL);

    l_vars.view = yf_view_init(l_vars.win);
    assert(l_vars.view != NULL);

    l_vars.coll = yf_collection_init("tmp/animation.glb");
    assert(l_vars.coll != NULL);

    yf_collection_each(l_vars.coll, YF_CITEM_ANIMATION, each_anim, NULL);

    l_vars.anim = yf_collection_getitem(l_vars.coll, YF_CITEM_ANIMATION,
                                        "Animation");
    assert(l_vars.anim != NULL);

    l_vars.scn = yf_collection_getitem(l_vars.coll, YF_CITEM_SCENE, "Scene");
    assert(l_vars.scn != NULL);

    yf_node_traverse(yf_scene_getnode(l_vars.scn), traverse, NULL);

    yf_scene_setcolor(l_vars.scn, YF_COLOR_DARKGREY);
    yf_view_setscene(l_vars.view, l_vars.scn);
    yf_view_start(l_vars.view, YF_FPS, update);

    /* managed... */
    /*yf_animation_deinit(l_vars.anim);*/
    /*yf_scene_deinit(l_vars.scn);*/
    yf_collection_deinit(l_vars.coll);
    yf_view_deinit(l_vars.view);
    yf_window_deinit(l_vars.win);
    return 0;
}
