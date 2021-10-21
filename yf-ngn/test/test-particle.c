/*
 * YF
 * test-particle.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "yf/wsys/yf-event.h"
#include "yf/wsys/yf-keyboard.h"

#include "test.h"
#include "print.h"
#include "yf-ngn.h"

#define YF_WINW 640
#define YF_WINH 480
#define YF_WINT "Particle"
#define YF_FPS  60
#define YF_PLACE (YF_vec3){20.0f, 20.0f, 20.0f}
#define YF_POINT (YF_vec3){0}

/* Shared variables. */
struct T_vars {
    YF_window win;
    YF_view view;
    YF_scene scn;
    YF_particle part;
    YF_texture tex;

    struct {
        int quit;
        int place;
        int point;
        int move[6];
        int turn[4];
        int once;
        int rgb[3];
    } input;
};
static struct T_vars vars_ = {0};

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
    case YF_KEY_X:
        vars_.input.once = state;
        break;
    case YF_KEY_1:
        if (state)
            vars_.input.rgb[0] = ~vars_.input.rgb[0];
        break;
    case YF_KEY_2:
        if (state)
            vars_.input.rgb[1] = ~vars_.input.rgb[1];
        break;
    case YF_KEY_3:
        if (state)
            vars_.input.rgb[2] = ~vars_.input.rgb[2];
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
    const float md = 16.0 * elapsed_time;;
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

    YF_psys *sys = yf_particle_getsys(vars_.part);
    sys->lifetime.once = vars_.input.once;
    sys->color.max[0] = vars_.input.rgb[0] ? 0.0f : 1.0f;
    sys->color.max[1] = vars_.input.rgb[1] ? 0.0f : 1.0f;
    sys->color.max[2] = vars_.input.rgb[2] ? 0.0f : 1.0f;

    char s[64];
    snprintf(s, sizeof s, "part, %.6f", elapsed_time);
    YF_TEST_PRINT("simulate", s, "");
    yf_particle_simulate(vars_.part, elapsed_time);

    return 0;
}

/* Tests particle. */
int yf_test_particle(void)
{
    srand(time(NULL));

    YF_evtfn evtfn = {.key_kb = on_key};
    yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);

    vars_.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
    assert(vars_.win != NULL);

    vars_.view = yf_view_init(vars_.win);
    assert(vars_.view != NULL);

    vars_.scn = yf_scene_init();
    assert(vars_.scn != NULL);

    vars_.tex = yf_texture_load("tmp/sprite.png", 0, NULL);
    assert(vars_.tex != NULL);

    YF_TEST_PRINT("init", "1000", "part");
    vars_.part = yf_particle_init(1000);
    if (vars_.part == NULL)
        return -1;

    YF_TEST_PRINT("getnode", "part", "");
    YF_node node = yf_particle_getnode(vars_.part);
    if (node == NULL)
        return -1;

    yf_mat4_scale(*yf_node_getxform(node), 0.5f, 1.0f, 0.25f);

    yf_print_nodeobj(node);

    YF_TEST_PRINT("getsys", "part", "");
    YF_psys *sys = yf_particle_getsys(vars_.part);
    if (sys == NULL)
        return -1;
    /* TODO: Setting 'emitter.norm' has no effect currently. */
    sys->emitter.norm[0] = 0.0f;
    sys->emitter.norm[1] = 1.0f;
    sys->emitter.norm[2] = 0.0f;
    sys->emitter.size = 2.0f;
    sys->lifetime.spawn_min = 0.15f;
    sys->lifetime.spawn_max = 0.95f;
    sys->lifetime.duration_min = 1.0f;
    sys->lifetime.duration_max = 2.0f;
    sys->lifetime.death_min = 1.0f;
    sys->lifetime.death_max = 1.75f;
    sys->color.min[0] = 0.0f;
    sys->color.max[0] = 1.0f;
    sys->color.min[1] = 0.0f;
    sys->color.max[1] = 0.0f;
    sys->color.min[2] = 0.0f;
    sys->color.max[2] = 0.0f;
    sys->velocity.min[0] = -0.01f;
    sys->velocity.max[0] = 0.01f;
    sys->velocity.min[1] = -0.001f;
    sys->velocity.max[1] = 0.05f;
    sys->velocity.min[2] = -0.01f;
    sys->velocity.max[2] = 0.01f;

    yf_print_nodeobj(node);

    vars_.input.rgb[1] = vars_.input.rgb[2] = ~0;

    YF_TEST_PRINT("getmesh", "part", "");
    if (yf_particle_getmesh(vars_.part) == NULL)
        return -1;

    YF_TEST_PRINT("gettex", "part", "");
    if (yf_particle_gettex(vars_.part) != NULL)
        return -1;

    YF_TEST_PRINT("settex", "part, tex", "");
    yf_particle_settex(vars_.part, vars_.tex);

    YF_TEST_PRINT("gettex", "part", "");
    if (yf_particle_gettex(vars_.part) != vars_.tex)
        return -1;

    yf_node_insert(yf_scene_getnode(vars_.scn), node);

    if (yf_view_loop(vars_.view, vars_.scn, YF_FPS, update, NULL) != 0)
        assert(0);

    yf_print_nodeobj(node);

    YF_TEST_PRINT("deinit", "part", "");
    yf_particle_deinit(vars_.part);

    yf_view_deinit(vars_.view);
    yf_scene_deinit(vars_.scn);
    yf_texture_deinit(vars_.tex);
    yf_window_deinit(vars_.win);

    return 0;
}
