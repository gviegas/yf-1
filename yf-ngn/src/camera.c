/*
 * YF
 * camera.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#ifdef YF_DEVEL
# include <stdio.h>
#endif

#include "yf/com/yf-util.h"
#include "yf/com/yf-error.h"

#include "yf-camera.h"

#define YF_FOV_MIN 0.07957747154594767280
#define YF_FOV_MAX M_PI_4

#define YF_TURNX_MIN 0.0001
#define YF_TURNX_MAX (M_PI-0.0001)

struct yf_camera {
    yf_vec3_t pos;
    yf_vec3_t dir;
    float turn_x;
    float zoom;
    float aspect;
    yf_mat4_t view;
    yf_mat4_t proj;
    yf_mat4_t view_proj;
#define YF_PEND_NONE 0
#define YF_PEND_V    0x01 /* 'view' not up to date */
#define YF_PEND_P    0x02 /* 'proj' not up to date */
#define YF_PEND_VP   0x04 /* 'view_proj' not up to date */
    unsigned pend_mask;
};

/* World's 'up' vector. */
static const yf_vec3_t wld_up_ = {0.0f, 1.0f, 0.0f};

/* Updates the camera's view matrix. */
static void update_view(yf_camera_t *cam)
{
    yf_vec3_t center;
    yf_vec3_add(center, cam->pos, cam->dir);
    yf_mat4_lookat(cam->view, center, cam->pos, wld_up_);

    cam->pend_mask &= ~YF_PEND_V;
    cam->pend_mask |= YF_PEND_VP;
}

/* Updates the camera's projection matrix. */
static void update_proj(yf_camera_t *cam)
{
    cam->zoom = YF_CLAMP(cam->zoom, YF_FOV_MIN, YF_FOV_MAX);
    yf_mat4_persp(cam->proj, cam->zoom, cam->aspect, 0.01f, 100.0f);

    cam->pend_mask &= ~YF_PEND_P;
    cam->pend_mask |= YF_PEND_VP;
}

/* Update the camera's view-projection matrix. */
static void update_vp(yf_camera_t *cam)
{
    yf_mat4_mul(cam->view_proj, cam->proj, cam->view);
    cam->pend_mask &= ~YF_PEND_VP;
}

yf_camera_t *yf_camera_init(const yf_vec3_t origin, const yf_vec3_t target,
                            float aspect)
{
    assert(!yf_vec3_iseq(origin, target));
    assert(aspect > 0.0f);

    yf_camera_t *cam = malloc(sizeof(yf_camera_t));
    if (cam == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }
    yf_vec3_copy(cam->pos, origin);
    yf_vec3_sub(cam->dir, target, origin);
    yf_vec3_normi(cam->dir);

    cam->turn_x = acosf(yf_vec3_dot(cam->dir, wld_up_));
    if (cam->turn_x < YF_TURNX_MIN || cam->turn_x > YF_TURNX_MAX) {
        yf_seterr(YF_ERR_INVARG, __func__);
        free(cam);
        return NULL;
    }
    cam->zoom = YF_FOV_MAX;
    cam->aspect = aspect;

    update_view(cam);
    update_proj(cam);
    update_vp(cam);
    return cam;
}

void yf_camera_place(yf_camera_t *cam, const yf_vec3_t pos)
{
    assert(cam != NULL);

    yf_vec3_copy(cam->pos, pos);
    cam->pend_mask |= YF_PEND_V;
}

void yf_camera_point(yf_camera_t *cam, const yf_vec3_t pos)
{
    assert(cam != NULL);

#ifdef YF_DEVEL
    assert(!yf_vec3_iseq(pos, cam->pos));
#else
    if (yf_vec3_iseq(pos, cam->pos)) return;
#endif

    yf_vec3_t dir;
    yf_vec3_sub(dir, pos, cam->pos);
    yf_vec3_normi(dir);
    float ang = acosf(yf_vec3_dot(dir, wld_up_));

    if (ang >= YF_TURNX_MIN && ang <= YF_TURNX_MAX) {
        yf_vec3_copy(cam->dir, dir);
        cam->turn_x = ang;
    } else {
        ang = (ang < YF_TURNX_MIN ? YF_TURNX_MIN : YF_TURNX_MAX) - cam->turn_x;
        yf_vec3_t side, front;
        yf_vec4_t q;
        yf_mat3_t rot;
        yf_vec3_cross(side, cam->dir, wld_up_);
        yf_vec4_rotq(q, ang, side);
        yf_mat3_rotq(rot, q);
        yf_mat3_mulv(front, rot, cam->dir);
        yf_vec3_norm(cam->dir, front);
        cam->turn_x += ang;
    }
    cam->pend_mask |= YF_PEND_V;
}

void yf_camera_movef(yf_camera_t *cam, float d)
{
    assert(cam != NULL);

    yf_vec3_t off;
    yf_vec3_muls(off, cam->dir, d);
    yf_vec3_addi(cam->pos, off);

    cam->pend_mask |= YF_PEND_V;
}

void yf_camera_moveb(yf_camera_t *cam, float d)
{
    yf_camera_movef(cam, -d);
}

void yf_camera_moveu(yf_camera_t *cam, float d)
{
    assert(cam != NULL);

    yf_vec3_t off;
    yf_vec3_muls(off, wld_up_, d);
    yf_vec3_addi(cam->pos, off);

    cam->pend_mask |= YF_PEND_V;
}

void yf_camera_moved(yf_camera_t *cam, float d)
{
    yf_camera_moveu(cam, -d);
}

void yf_camera_movel(yf_camera_t *cam, float d)
{
    assert(cam != NULL);

    yf_vec3_t side, off;
    yf_vec3_cross(side, cam->dir, wld_up_);
    yf_vec3_muls(off, side, d);
    yf_vec3_subi(cam->pos, off);

    cam->pend_mask |= YF_PEND_V;
}

void yf_camera_mover(yf_camera_t *cam, float d)
{
    yf_camera_movel(cam, -d);
}

void yf_camera_turnu(yf_camera_t *cam, float d)
{
    assert(cam != NULL);

    float ang;
    if (cam->turn_x - d < YF_TURNX_MIN)
        ang = YF_TURNX_MIN - cam->turn_x;
    else if (cam->turn_x - d > YF_TURNX_MAX)
        ang = YF_TURNX_MAX - cam->turn_x;
    else
        ang = -d;

    yf_vec3_t side, front;
    yf_vec4_t q;
    yf_mat3_t rot;
    yf_vec3_cross(side, cam->dir, wld_up_);
    yf_vec4_rotq(q, -ang, side);
    yf_mat3_rotq(rot, q);
    yf_mat3_mulv(front, rot, cam->dir);
    yf_vec3_norm(cam->dir, front);
    cam->turn_x += ang;

    cam->pend_mask |= YF_PEND_V;
}

void yf_camera_turnd(yf_camera_t *cam, float d)
{
    yf_camera_turnu(cam, -d);
}

void yf_camera_turnl(yf_camera_t *cam, float d)
{
    assert(cam != NULL);

    yf_vec3_t front;
    yf_vec4_t q;
    yf_mat3_t rot;
    yf_vec4_rotq(q, d, wld_up_);
    yf_mat3_rotq(rot, q);
    yf_mat3_mulv(front, rot, cam->dir);
    yf_vec3_norm(cam->dir, front);

    cam->pend_mask |= YF_PEND_V;
}

void yf_camera_turnr(yf_camera_t *cam, float d)
{
    yf_camera_turnl(cam, -d);
}

void yf_camera_zoomi(yf_camera_t *cam, float d)
{
    assert(cam != NULL);

    cam->zoom -= d;
    cam->pend_mask |= YF_PEND_P;
}

void yf_camera_zoomo(yf_camera_t *cam, float d)
{
    assert(cam != NULL);

    cam->zoom += d;
    cam->pend_mask |= YF_PEND_P;
}

const yf_mat4_t *yf_camera_getxform(yf_camera_t *cam)
{
    assert(cam != NULL);

    if (cam->pend_mask & YF_PEND_V)
        update_view(cam);
    if (cam->pend_mask & YF_PEND_P)
        update_proj(cam);
    if (cam->pend_mask & YF_PEND_VP)
        update_vp(cam);

    return (const yf_mat4_t *)&cam->view_proj;
}

const yf_mat4_t *yf_camera_getview(yf_camera_t *cam)
{
    assert(cam != NULL);

    if (cam->pend_mask & YF_PEND_V)
        update_view(cam);

    return (const yf_mat4_t *)&cam->view;
}

const yf_mat4_t *yf_camera_getproj(yf_camera_t *cam)
{
    assert(cam != NULL);

    if (cam->pend_mask & YF_PEND_P)
        update_proj(cam);

    return (const yf_mat4_t *)&cam->proj;
}

void yf_camera_adjust(yf_camera_t *cam, float aspect)
{
    assert(cam != NULL);
    assert(aspect > 0.0f);

    cam->aspect = aspect;
    cam->pend_mask |= YF_PEND_P;
}

void yf_camera_deinit(yf_camera_t *cam)
{
    if (cam != NULL)
        free(cam);
}

/*
 * DEVEL
 */

#ifdef YF_DEVEL

void yf_print_cam(yf_camera_t *cam)
{
    assert(cam != NULL);

    printf("\n[YF] OUTPUT (%s):\n"
           " camera:\n"
           "  position: [%.4f, %.4f, %.4f]\n"
           "  direction: [%.4f, %.4f, %.4f]\n"
           "  turn (x-axis): %.4f\n"
           "  zoom: %.4f\n"
           "  aspect: %.4f\n\n",
           __func__, cam->pos[0], cam->pos[1], cam->pos[2], cam->dir[0],
           cam->dir[1], cam->dir[2], cam->turn_x, cam->zoom, cam->aspect);
}

#endif
