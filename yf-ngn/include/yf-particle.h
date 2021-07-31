/*
 * YF
 * yf-particle.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_PARTICLE_H
#define YF_YF_PARTICLE_H

#include "yf/com/yf-defs.h"

#include "yf-node.h"
#include "yf-mesh.h"
#include "yf-texture.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a particle system.
 */
typedef struct YF_particle_o *YF_particle;

/**
 * Type defining particle system parameters.
 */
typedef struct {
    struct {
        YF_vec3 norm;
        float size;
    } emitter;
    struct {
        float duration_min;
        float duration_max;
        float spawn_min;
        float spawn_max;
        float death_min;
        float death_max;
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

/**
 * Initializes a new particle system.
 *
 * @param count: The particle count.
 * @return: On success, returns a new particle system. Otherwise, 'NULL' is
 *  returned and the global error is set to indicate the cause.
 */
YF_particle yf_particle_init(unsigned count);

/**
 * Gets the node of a particle system.
 *
 * @param part: The particle system.
 * @return: The particle system's node.
 */
YF_node yf_particle_getnode(YF_particle part);

/**
 * Gets the parameters of a particle system.
 *
 * @param part: The particle system.
 * @return: The particle system's parameters.
 */
YF_psys *yf_particle_getsys(YF_particle part);

/**
 * Gets the mesh of a particle system.
 *
 * @param part: The particle system.
 * @return: The mesh used by the particle system.
 */
YF_mesh yf_particle_getmesh(YF_particle part);

/**
 * Gets the texture of a particle system.
 *
 * @param part: The particle system.
 * @return: The texture used by the particle system, or 'NULL' if none is set.
 */
YF_texture yf_particle_gettex(YF_particle part);

/**
 * Sets the texture for a particle system.
 *
 * @param part: The particle system.
 * @param tex: The texture to set. Can be 'NULL'.
 */
void yf_particle_settex(YF_particle part, YF_texture tex);

/**
 * Simulates a particle system.
 *
 * @param part: The particle system.
 * @param tm: The time to advance the simulation.
 */
void yf_particle_simulate(YF_particle part, float tm);

/**
 * Deinitializes a particle system.
 *
 * @param part: The particle system to deinitialize. Can be 'NULL'.
 */
void yf_particle_deinit(YF_particle part);

YF_DECLS_END

#endif /* YF_YF_PARTICLE_H */
