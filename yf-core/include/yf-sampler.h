/*
 * YF
 * yf-sampler.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_SAMPLER_H
#define YF_YF_SAMPLER_H

/**
 * Wrap modes.
 */
#define YF_WRAPMODE_CLAMP  0
#define YF_WRAPMODE_MIRROR 1
#define YF_WRAPMODE_REPEAT 2

/**
 * Filters.
 */
#define YF_FILTER_NEAREST 0
#define YF_FILTER_LINEAR  1

/**
 * Type defining an image sampler.
 */
typedef struct {
    struct {
        int u, v, w;
    } wrapmode;
    struct {
        int mag, min, mipmap;
    } filter;
} YF_sampler;

#endif /* YF_YF_SAMPLER_H */
