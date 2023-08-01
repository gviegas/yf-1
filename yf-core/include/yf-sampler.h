/*
 * YF
 * yf-sampler.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_SAMPLER_H
#define YF_YF_SAMPLER_H

/**
 * Wrap modes.
 */
#define YF_WRAPMODE_REPEAT 0
#define YF_WRAPMODE_MIRROR 1
#define YF_WRAPMODE_CLAMP  2

/**
 * Filters.
 */
#define YF_FILTER_NEAREST 0
#define YF_FILTER_LINEAR  1

/**
 * Type defining an image sampler.
 */
typedef struct yf_sampler {
    struct {
        int u;
        int v;
        int w;
    } wrapmode;
    struct {
        int mag;
        int min;
        int mipmap;
    } filter;
} yf_sampler_t;

#endif /* YF_YF_SAMPLER_H */
