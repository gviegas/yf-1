/*
 * YF
 * sampler.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_SAMPLER_H
#define YF_SAMPLER_H

#include "yf-context.h"
#include "yf-sampler.h"
#include "vk.h"

/* Makes a new sampler from a given 'YF_SAMPLER' value.
   The caller is responsible for the object's destruction. */
VkSampler yf_sampler_make(YF_context ctx, int sampler);

#endif /* YF_SAMPLER_H */
