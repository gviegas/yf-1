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

#include <yf/com/yf-error.h>

#include "camera.h"

#ifdef YF_DEBUG
# include <stdio.h>
# define YF_CAM_PRINT(cam) do { \
   const char *pos = "\npos: %.5f, %.5f, %.5f"; \
   const char *dir = "\ndir: %.5f, %.5f, %.5f"; \
   const char *turnx = "\nturnx: %.5f"; \
   const char *zoom = "\nzoom: %.5f"; \
   const char *asp = "\nasp: %.5f"; \
   printf("\n-- Camera (debug) --"); \
   printf(pos, cam->pos[0], cam->pos[1], cam->pos[2]); \
   printf(dir, cam->dir[0], cam->dir[1], cam->dir[2]); \
   printf(turnx, cam->turn_x); \
   printf(zoom, cam->zoom); \
   printf(asp, cam->aspect); \
   if (yf_vec3_iszero(cam->pos)) \
    printf("\n** At origin **"); \
   printf("\n--\n"); } while (0)
#endif

#define YF_FOVMIN (YF_float)0.07957747154594767280
#define YF_FOVMAX (YF_float)M_PI_4
#define YF_TURNXMIN (YF_float)0.0001
#define YF_TURNXMAX (YF_float)3.14149265358979323846

struct YF_camera_o {
  YF_vec3 pos;
  YF_vec3 dir;
  YF_float turn_x;
  YF_float zoom;
  YF_float aspect;
  YF_mat4 view;
  YF_mat4 proj;
  YF_mat4 view_proj;
  int view_upd;
  int proj_upd;
  int vp_upd;
};

/* World's `up` vector. */
static const YF_vec3 l_wld_u = {0.0, -1.0, 0.0};

/* Updates the camera's view matrix. */
static void update_view(YF_camera cam);

/* Updates the camera's projection matrix. */
static void update_proj(YF_camera cam);

YF_camera yf_camera_init(const YF_vec3 origin, const YF_vec3 target,
    YF_float aspect)
{
  assert(!yf_vec3_iseq(origin, target));
  assert(aspect > 0.0);

  YF_camera cam = malloc(sizeof(struct YF_camera_o));
  if (cam == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }

  yf_vec3_copy(cam->pos, origin);
  yf_vec3_sub(cam->dir, target, origin);
  yf_vec3_normi(cam->dir);

#ifdef YF_USE_FLOAT64
  cam->turn_x = acos(yf_vec3_dot(cam->dir, l_wld_u));
#else
  cam->turn_x = acosf(yf_vec3_dot(cam->dir, l_wld_u));
#endif

  if (cam->turn_x < YF_TURNXMIN || cam->turn_x > YF_TURNXMAX) {
    yf_seterr(YF_ERR_INVARG, __func__);
    free(cam);
    return NULL;
  }

  cam->zoom = YF_FOVMAX;
  cam->aspect = aspect;

  update_view(cam);
  update_proj(cam);

  yf_mat4_mul(cam->view_proj, cam->proj, cam->view);

  cam->view_upd = 1;
  cam->proj_upd = 1;
  cam->vp_upd = 1;

  return cam;
}

void yf_camera_place(YF_camera cam, const YF_vec3 pos) {
  assert(cam != NULL);

  yf_vec3_copy(cam->pos, pos);
  cam->view_upd = 0;
}

void yf_camera_point(YF_camera cam, const YF_vec3 pos) {
  assert(cam != NULL);

#ifdef YF_DEBUG
  assert(!yf_vec3_iseq(pos, cam->pos));
#else
  if (yf_vec3_iseq(pos, cam->pos)) return;
#endif

  YF_float ang;
  YF_vec3 dir;
  yf_vec3_sub(dir, pos, cam->pos);
  yf_vec3_normi(dir);

#ifdef YF_USE_FLOAT64
  ang = acos(yf_vec3_dot(dir, l_wld_u));
#else
  ang = acosf(yf_vec3_dot(dir, l_wld_u));
#endif

  if (ang >= YF_TURNXMIN && ang <= YF_TURNXMAX) {
    yf_vec3_copy(cam->dir, dir);
    cam->turn_x = ang;
  } else {
    ang = (ang < YF_TURNXMIN ? YF_TURNXMIN : YF_TURNXMAX) - cam->turn_x;
    YF_vec3 side, front;
    YF_mat3 rot;
    yf_vec3_cross(side, l_wld_u, cam->dir);
    yf_mat3_rot(rot, ang, side);
    yf_mat3_mulv(front, rot, cam->dir);
    yf_vec3_norm(cam->dir, front);
    cam->turn_x += ang;
  }

  cam->view_upd = 0;
}

void yf_camera_movef(YF_camera cam, YF_float d) {
  assert(cam != NULL);

  YF_vec3 offs;
  yf_vec3_muls(offs, cam->dir, d);
  yf_vec3_addi(cam->pos, offs);
  cam->view_upd = 0;
}

void yf_camera_moveb(YF_camera cam, YF_float d) {
  yf_camera_movef(cam, -d);
}

void yf_camera_moveu(YF_camera cam, YF_float d) {
  assert(cam != NULL);

  YF_vec3 offs;
  yf_vec3_muls(offs, l_wld_u, d);
  yf_vec3_addi(cam->pos, offs);
  cam->view_upd = 0;
}

void yf_camera_moved(YF_camera cam, YF_float d) {
  yf_camera_moveu(cam, -d);
}

void yf_camera_movel(YF_camera cam, YF_float d) {
  assert(cam != NULL);

  YF_vec3 side, offs;
  yf_vec3_cross(side, cam->dir, l_wld_u);
  yf_vec3_muls(offs, side, d);
  yf_vec3_subi(cam->pos, offs);
  cam->view_upd = 0;
}

void yf_camera_mover(YF_camera cam, YF_float d) {
  yf_camera_movel(cam, -d);
}

void yf_camera_turnu(YF_camera cam, YF_float d) {
  assert(cam != NULL);

  YF_float ang;
  if (cam->turn_x - d < YF_TURNXMIN)
    ang = YF_TURNXMIN - cam->turn_x;
  else if (cam->turn_x - d > YF_TURNXMAX)
    ang = YF_TURNXMAX - cam->turn_x;
  else
    ang = -d;

  YF_vec3 side, front;
  YF_mat3 rot;
  yf_vec3_cross(side, l_wld_u, cam->dir);
  yf_mat3_rot(rot, ang, side);
  yf_mat3_mulv(front, rot, cam->dir);
  yf_vec3_norm(cam->dir, front);
  cam->turn_x += ang;
  cam->view_upd = 0;
}

void yf_camera_turnd(YF_camera cam, YF_float d) {
  yf_camera_turnu(cam, -d);
}

void yf_camera_turnl(YF_camera cam, YF_float d) {
  assert(cam != NULL);

  YF_vec3 front;
  YF_mat3 rot;
  yf_mat3_rot(rot, d, l_wld_u);
  yf_mat3_mulv(front, rot, cam->dir);
  yf_vec3_norm(cam->dir, front);
  cam->view_upd = 0;
}

void yf_camera_turnr(YF_camera cam, YF_float d) {
  yf_camera_turnl(cam, -d);
}

void yf_camera_zoomi(YF_camera cam, YF_float d) {
  assert(cam != NULL);

  cam->zoom -= d;
  cam->proj_upd = 0;
}

void yf_camera_zoomo(YF_camera cam, YF_float d) {
  assert(cam != NULL);

  cam->zoom += d;
  cam->proj_upd = 0;
}

const YF_mat4 *yf_camera_getxform(YF_camera cam) {
  assert(cam != NULL);

  if (!cam->view_upd)
    update_view(cam);
  if (!cam->proj_upd)
    update_proj(cam);
  if (!cam->vp_upd) {
    yf_mat4_mul(cam->view_proj, cam->proj, cam->view);
    cam->vp_upd = 1;
  }

#ifdef YF_DEBUG
  YF_CAM_PRINT(cam);
#endif

  return (const YF_mat4 *)&cam->view_proj;
}

const YF_mat4 *yf_camera_getview(YF_camera cam) {
  assert(cam != NULL);

  if (!cam->view_upd)
    update_view(cam);
  return (const YF_mat4 *)&cam->view;
}

const YF_mat4 *yf_camera_getproj(YF_camera cam) {
  assert(cam != NULL);

  if (!cam->proj_upd)
    update_proj(cam);
  return (const YF_mat4 *)&cam->proj;
}

void yf_camera_adjust(YF_camera cam, YF_float aspect) {
  assert(cam != NULL);
  assert(aspect > 0.0);

  cam->aspect = aspect;
  cam->proj_upd = 0;
}

void yf_camera_deinit(YF_camera cam) {
  if (cam != NULL)
    free(cam);
}

static void update_view(YF_camera cam) {
  YF_vec3 center;
  yf_vec3_add(center, cam->pos, cam->dir);
  yf_mat4_lookat(cam->view, cam->pos, center, l_wld_u);
  cam->view_upd = 1;
  cam->vp_upd = 0;
}

static void update_proj(YF_camera cam) {
  if (cam->zoom < YF_FOVMIN)
    cam->zoom = YF_FOVMIN;
  else if (cam->zoom > YF_FOVMAX)
    cam->zoom = YF_FOVMAX;
  yf_mat4_persp(cam->proj, cam->zoom, cam->aspect, 0.1, 100.0);
  cam->proj_upd = 1;
  cam->vp_upd = 0;
}
