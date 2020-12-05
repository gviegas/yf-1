/*
 * YF
 * particle.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_PARTICLE_H
#define YF_PARTICLE_H

#include "yf-particle.h"

/* Gets the model-view-projection matrix of a particle system. */
YF_mat4 *yf_particle_getmvp(YF_particle part);

#endif /* YF_PARTICLE_H */
