/*
 * YF
 * yf-particle.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_PARTICLE_H
#define YF_YF_PARTICLE_H

#include "yf-common.h"
#include "yf-node.h"
#include "yf-matrix.h"
#include "yf-mesh.h"
#include "yf-texture.h"

YF_DECLS_BEGIN

/* Opaque type defining a particle system. */
typedef struct YF_particle_o *YF_particle;

/* Type defining particle system parameters. */
typedef struct {
  struct {
    YF_vec3 norm;
    YF_float size;
  } emitter;
  struct {
    YF_float duration_min;
    YF_float duration_max;
    YF_float spawn_min;
    YF_float spawn_max;
    YF_float death_min;
    YF_float death_max;
    int once;
  } lifetime;
  struct {
    YF_vec4 min;
    YF_vec4 max;
  } color;
  struct {
    YF_vec3 min;
    YF_vec3 max;
  } velocity;
} YF_psys;

/* Initializes a new particle system. */
YF_particle yf_particle_init(unsigned count);

/* Gets the node of a particle system. */
YF_node yf_particle_getnode(YF_particle part);

/* Gets the transformation matrix of a particle system. */
YF_mat4 *yf_particle_getxform(YF_particle part);

/* Gets the parameters of a particle system. */
YF_psys *yf_particle_getsys(YF_particle part);

/* Gets the mesh of a particle system. */
YF_mesh yf_particle_getmesh(YF_particle part);

/* Gets the texture of a particle system. */
YF_texture yf_particle_gettex(YF_particle part);

/* Sets the texture for a particle system. */
void yf_particle_settex(YF_particle part, YF_texture tex);

/* Simulates a particle system. */
void yf_particle_simulate(YF_particle part, double tm);

/* Deinitializes a particle system. */
void yf_particle_deinit(YF_particle part);

YF_DECLS_END

#endif /* YF_YF_PARTICLE_H */
