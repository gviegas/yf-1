/*
 * YF
 * yf-particle.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
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
typedef struct yf_particle yf_particle_t;

/**
 * Type defining particle system parameters.
 */
typedef struct yf_psys {
    /* TODO: Improve this. */
    struct {
        yf_vec3_t norm;
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
        yf_vec4_t min;
        yf_vec4_t max;
    } color;
    struct {
        yf_vec3_t min;
        yf_vec3_t max;
    } velocity;
} yf_psys_t;

/**
 * Initializes a new particle system.
 *
 * @param count: The particle count.
 * @return: On success, returns a new particle system. Otherwise, 'NULL' is
 *  returned and the global error is set to indicate the cause.
 */
yf_particle_t *yf_particle_init(unsigned count);

/**
 * Gets the node of a particle system.
 *
 * @param part: The particle system.
 * @return: The particle system's node.
 */
yf_node_t *yf_particle_getnode(yf_particle_t *part);

/**
 * Gets the parameters of a particle system.
 *
 * @param part: The particle system.
 * @return: The particle system's parameters.
 */
yf_psys_t *yf_particle_getsys(yf_particle_t *part);

/**
 * Gets the mesh of a particle system.
 *
 * @param part: The particle system.
 * @return: The mesh used by the particle system.
 */
yf_mesh_t *yf_particle_getmesh(yf_particle_t *part);

/**
 * Gets the texture of a particle system.
 *
 * @param part: The particle system.
 * @return: The texture used by the particle system, or 'NULL' if none is set.
 */
yf_texture_t *yf_particle_gettex(yf_particle_t *part);

/**
 * Sets the texture for a particle system.
 *
 * @param part: The particle system.
 * @param tex: The texture to set. Can be 'NULL'.
 */
void yf_particle_settex(yf_particle_t *part, yf_texture_t *tex);

/**
 * Simulates a particle system.
 *
 * @param part: The particle system.
 * @param tm: The time to advance the simulation.
 */
/* TODO: This should be done on GPU. */
void yf_particle_simulate(yf_particle_t *part, float tm);

/**
 * Deinitializes a particle system.
 *
 * @param part: The particle system to deinitialize. Can be 'NULL'.
 */
void yf_particle_deinit(yf_particle_t *part);

YF_DECLS_END

#endif /* YF_YF_PARTICLE_H */
