/*
 * YF
 * particle.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <yf/com/yf-error.h>

#include "particle.h"
#include "node.h"
#include "mesh.h"
#include "vertex.h"

#undef YF_NRND
#define YF_NRND ((YF_float)rand() / (YF_float)RAND_MAX)

#undef YF_LERP
#define YF_LERP(a, b, t) (((YF_float)1-t)*a + t*b)

/* Type defining the state of a single particle. */
typedef struct {
#define YF_PSTATE_UNSET    0
#define YF_PSTATE_SPAWNING 1
#define YF_PSTATE_SPAWNED  2
#define YF_PSTATE_DYING    3
#define YF_PSTATE_DEAD     4
  int pstate;
  YF_float tm;
  YF_float dur;
  YF_float spawn;
  YF_float death;
  YF_float alpha;
  YF_vec3 vel;
} L_pstate;

struct YF_particle_o {
  YF_node node;
  unsigned count;
  YF_mat4 xform;
  YF_psys sys;
  YF_vpart *pts;
  L_pstate *sts;
  YF_mesh mesh;
  YF_texture tex;
  /* TODO: Other particle system properties. */
  YF_mat4 mvp;
};

/* Initializes vertices, states and mesh object. */
static int init_points(YF_particle part);

YF_particle yf_particle_init(unsigned count) {
  if (count == 0) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return NULL;
  }

  YF_particle part = calloc(1, sizeof(struct YF_particle_o));
  if (part == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  if ((part->node = yf_node_init()) == NULL) {
    free(part);
    return NULL;
  }
  yf_node_setobj(part->node, YF_NODEOBJ_PARTICLE, part);
  part->count = count;
  yf_mat4_iden(part->xform);
  yf_mat4_iden(part->mvp);

  part->sys.emitter.norm[0] = 0.0;
  part->sys.emitter.norm[1] = -1.0;
  part->sys.emitter.norm[2] = 0.0;
  part->sys.emitter.size = 1.0;
  part->sys.lifetime.duration_min = 0.1;
  part->sys.lifetime.duration_max = 1.0;
  part->sys.lifetime.spawn_min = 0.01;
  part->sys.lifetime.spawn_max = 0.1;
  part->sys.lifetime.death_min = 0.01;
  part->sys.lifetime.death_max = 0.1;
  part->sys.lifetime.once = 0;
  part->sys.color.min[0] = 0.0;
  part->sys.color.min[1] = 0.0;
  part->sys.color.min[2] = 0.0;
  part->sys.color.min[3] = 0.1;
  part->sys.color.max[0] = 1.0;
  part->sys.color.max[1] = 1.0;
  part->sys.color.max[2] = 1.0;
  part->sys.color.max[3] = 1.0;
  part->sys.velocity.min[0] = 0.0;
  part->sys.velocity.min[1] = -0.1;
  part->sys.velocity.min[2] = 0.0;
  part->sys.velocity.max[0] = 0.0;
  part->sys.velocity.max[1] = -0.01;
  part->sys.velocity.max[2] = 0.0;

  if (init_points(part) != 0) {
    yf_particle_deinit(part);
    return NULL;
  }
  return part;
}

YF_node yf_particle_getnode(YF_particle part) {
  assert(part != NULL);
  return part->node;
}

YF_mat4 *yf_particle_getxform(YF_particle part) {
  assert(part != NULL);
  return &part->xform;
}

YF_psys *yf_particle_getsys(YF_particle part) {
  assert(part != NULL);
  return &part->sys;
}

YF_mesh yf_particle_getmesh(YF_particle part) {
  assert(part != NULL);
  return part->mesh;
}

YF_texture yf_particle_gettex(YF_particle part) {
  assert(part != NULL);
  return part->tex;
}

void yf_particle_settex(YF_particle part, YF_texture tex) {
  assert(part != NULL);
  part->tex = tex;
}

void yf_particle_simulate(YF_particle part, double tm) {
  assert(tm >= 0.0);

  const YF_float dt = tm;
  const YF_psys *sys = &part->sys;
  YF_vpart *pt;
  L_pstate *st;

  for (unsigned i = 0; i < part->count; ++i) {
    pt = part->pts+i;
    st = part->sts+i;

    switch (st->pstate) {
      case YF_PSTATE_UNSET:
        pt->pos[0] = sys->emitter.size * (2.0 * YF_NRND - 1.0);
        pt->pos[1] = sys->emitter.size * (2.0 * YF_NRND - 1.0);
        pt->pos[2] = sys->emitter.size * (2.0 * YF_NRND - 1.0);
        pt->col[0] = YF_LERP(sys->color.min[0], sys->color.max[0], YF_NRND);
        pt->col[1] = YF_LERP(sys->color.min[1], sys->color.max[1], YF_NRND);
        pt->col[2] = YF_LERP(sys->color.min[2], sys->color.max[2], YF_NRND);
        pt->col[3] = 0.0;
        st->tm = 0.0;
        st->dur = YF_LERP(
          sys->lifetime.duration_min,
          sys->lifetime.duration_max,
          YF_NRND);
        st->spawn = YF_LERP(
          sys->lifetime.spawn_min,
          sys->lifetime.spawn_max,
          YF_NRND);
        st->death = YF_LERP(
          sys->lifetime.death_min,
          sys->lifetime.death_max,
          YF_NRND);
        st->alpha = YF_LERP(sys->color.min[3], sys->color.max[3], YF_NRND);
        st->vel[0] = YF_LERP(
          sys->velocity.min[0],
          sys->velocity.max[0],
          YF_NRND);
        st->vel[1] = YF_LERP(
          sys->velocity.min[1],
          sys->velocity.max[1],
          YF_NRND);
        st->vel[2] = YF_LERP(
          sys->velocity.min[2],
          sys->velocity.max[2],
          YF_NRND);
        st->pstate = YF_PSTATE_SPAWNING;
        break;

      case YF_PSTATE_SPAWNING:
        if (st->tm >= st->spawn) {
          pt->col[3] = st->alpha;
          st->tm -= st->spawn;
          st->pstate = YF_PSTATE_SPAWNED;
        } else {
          pt->pos[0] += st->vel[0];
          pt->pos[1] += st->vel[1];
          pt->pos[2] += st->vel[2];
          pt->col[3] = st->tm / st->spawn * st->alpha;
          st->tm += dt;
        }
        break;

      case YF_PSTATE_SPAWNED:
        if (st->tm >= st->dur) {
          st->tm -= st->dur;
          st->pstate = YF_PSTATE_DYING;
        } else {
          pt->pos[0] += st->vel[0];
          pt->pos[1] += st->vel[1];
          pt->pos[2] += st->vel[2];
          st->tm += dt;
        }
        break;

      case YF_PSTATE_DYING:
        if (st->tm >= st->death) {
          pt->col[3] = 0.0;
          st->tm -= st->death;
          st->pstate = YF_PSTATE_DEAD;
        } else {
          pt->pos[0] += st->vel[0];
          pt->pos[1] += st->vel[1];
          pt->pos[2] += st->vel[2];
          pt->col[3] = st->alpha - st->tm / st->death * st->alpha;
          st->tm += dt;
        }
        break;

      case YF_PSTATE_DEAD:
        if (!sys->lifetime.once) {
          st->tm = 0.0;
          st->pstate = YF_PSTATE_UNSET;
        }
        break;
    }
  }

  YF_slice range = {0, part->count};
#ifdef YF_DEBUG
  if (yf_mesh_setvtx(part->mesh, range, part->pts) != 0)
    assert(0);
#else
  yf_mesh_setvtx(part->mesh, range, part->pts);
#endif
}

void yf_particle_deinit(YF_particle part) {
  if (part != NULL) {
    yf_node_deinit(part->node);
    yf_mesh_deinit(part->mesh);
    free(part->pts);
    free(part->sts);
    free(part);
  }
}

YF_mat4 *yf_particle_getmvp(YF_particle part) {
  assert(part != NULL);
  return &part->mvp;
}

static int init_points(YF_particle part) {
  assert(part != NULL);

  part->pts = malloc(sizeof(YF_vpart) * part->count);
  part->sts = malloc(sizeof(L_pstate) * part->count);
  if (part->pts == NULL || part->sts == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }

  const YF_vpart pt = {.pos = {0.0, 0.0, 0.5}, .col = {1.0, 1.0, 1.0, 1.0}};
  const L_pstate st = {YF_PSTATE_UNSET, 0.0, 0.0, 0.0, 0.0, 0.0, {0}};
  for (unsigned i = 0; i < part->count; ++i) {
    memcpy(part->pts+i, &pt, sizeof pt);
    memcpy(part->sts+i, &st, sizeof st);
  }

  const YF_meshdt dt = {
    .v = {YF_VTYPE_PART, part->pts, part->count},
    .i = {NULL, 0, 0}
  };
  if ((part->mesh = yf_mesh_initdt(&dt)) == NULL)
    return -1;

  return 0;
}
