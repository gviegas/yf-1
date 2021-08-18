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
static struct T_vars l_vars = {0};

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
    case YF_KEY_X:
        l_vars.input.once = state;
        break;
    case YF_KEY_1:
        if (state)
            l_vars.input.rgb[0] = ~l_vars.input.rgb[0];
        break;
    case YF_KEY_2:
        if (state)
            l_vars.input.rgb[1] = ~l_vars.input.rgb[1];
        break;
    case YF_KEY_3:
        if (state)
            l_vars.input.rgb[2] = ~l_vars.input.rgb[2];
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
    const float md = 16.0 * elapsed_time;;
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

    YF_psys *sys = yf_particle_getsys(l_vars.part);
    sys->lifetime.once = l_vars.input.once;
    sys->color.max[0] = l_vars.input.rgb[0] ? 0.0f : 1.0f;
    sys->color.max[1] = l_vars.input.rgb[1] ? 0.0f : 1.0f;
    sys->color.max[2] = l_vars.input.rgb[2] ? 0.0f : 1.0f;

    char s[64];
    snprintf(s, sizeof s, "part, %.6f", elapsed_time);
    YF_TEST_PRINT("simulate", s, "");
    yf_particle_simulate(l_vars.part, elapsed_time);
}

/* Tests particle. */
int yf_test_particle(void)
{
    srand(time(NULL));

    YF_evtfn evtfn = {.key_kb = on_key};
    yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);

    l_vars.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
    assert(l_vars.win != NULL);

    l_vars.view = yf_view_init(l_vars.win);
    assert(l_vars.view != NULL);

    l_vars.scn = yf_scene_init();
    assert(l_vars.scn != NULL);

    l_vars.tex = yf_texture_init(YF_FILETYPE_PNG, "tmp/sprite.png");
    assert(l_vars.tex != NULL);

    YF_TEST_PRINT("init", "1000", "part");
    l_vars.part = yf_particle_init(1000);
    if (l_vars.part == NULL)
        return -1;

    YF_TEST_PRINT("getsys", "part", "");
    YF_psys *sys = yf_particle_getsys(l_vars.part);
    if (sys == NULL)
        return -1;
    sys->velocity.min[1] = -0.001f;
    sys->velocity.max[1] = 0.25f;
    sys->lifetime.spawn_min = 0.1f;
    sys->lifetime.spawn_max = 0.5f;
    sys->lifetime.duration_min = 0.75f;
    sys->lifetime.duration_max = 2.5f;

    YF_TEST_PRINT("getnode", "part", "");
    YF_node node = yf_particle_getnode(l_vars.part);
    if (node == NULL)
        return -1;

    yf_mat4_scale(*yf_node_getxform(node), 0.5f, 1.0f, 0.25f);

    YF_TEST_PRINT("getmesh", "part", "");
    if (yf_particle_getmesh(l_vars.part) == NULL)
        return -1;

    YF_TEST_PRINT("gettex", "part", "");
    if (yf_particle_gettex(l_vars.part) != NULL)
        return -1;

    YF_TEST_PRINT("settex", "part, tex", "");
    yf_particle_settex(l_vars.part, l_vars.tex);

    YF_TEST_PRINT("gettex", "part", "");
    if (yf_particle_gettex(l_vars.part) != l_vars.tex)
        return -1;

    yf_node_insert(yf_scene_getnode(l_vars.scn), node);

    YF_camera cam = yf_scene_getcam(l_vars.scn);
    const YF_vec3 pos = {0.0f, 0.0f, 20.0f};
    const YF_vec3 tgt = {0.0f, 6.0f, 0.0f};
    yf_camera_place(cam, pos);
    yf_camera_point(cam, tgt);

    yf_view_setscene(l_vars.view, l_vars.scn);

    if (yf_view_start(l_vars.view, YF_FPS, update) != 0)
        assert(0);

    YF_TEST_PRINT("deinit", "part", "");
    yf_particle_deinit(l_vars.part);

    yf_view_deinit(l_vars.view);
    yf_scene_deinit(l_vars.scn);
    yf_texture_deinit(l_vars.tex);
    yf_window_deinit(l_vars.win);

    return 0;
}
