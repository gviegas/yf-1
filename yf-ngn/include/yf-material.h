/*
 * YF
 * yf-material.h
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_MATERIAL_H
#define YF_YF_MATERIAL_H

#include "yf/com/yf-defs.h"

#include "yf-texture.h"
#include "yf-vector.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a material.
 */
typedef struct yf_material yf_material_t;

/**
 * PBR methodologies.
 */
#define YF_PBR_SPECGLOSS  0
#define YF_PBR_METALROUGH 1
#define YF_PBR_NONE       2

/**
 * Alpha modes.
 */
#define YF_ALPHAMODE_OPAQUE 0
#define YF_ALPHAMODE_BLEND  1
#define YF_ALPHAMODE_MASK   2

/**
 * Type defining the material properties of an object.
 */
typedef struct yf_matlprop {
    int pbr;
    union {
        /* PBR specular-glossiness ('PBR_SPECGLOSS'). */
        struct {
            yf_texref_t diffuse_tex;
            yf_vec4_t diffuse_fac;
            yf_texref_t spec_gloss_tex;
            yf_vec3_t specular_fac;
            float glossiness_fac;
        } pbrsg;

        /* PBR metallic-roughness ('PBR_METALROUGH'). */
        struct {
            yf_texref_t color_tex;
            yf_vec4_t color_fac;
            yf_texref_t metal_rough_tex;
            float metallic_fac;
            float roughness_fac;
        } pbrmr;

        /* Unlit ('PBR_NONE'). */
        struct {
            yf_texref_t color_tex;
            yf_vec4_t color_fac;
        } nopbr;
    };

    /* Additional maps.
       These are ignored when 'pbr' is 'PBR_NONE'. */
    struct {
        yf_texref_t tex;
        float scale;
    } normal;
    struct {
        yf_texref_t tex;
        float strength;
    } occlusion;
    struct {
        yf_texref_t tex;
        yf_vec3_t factor;
    } emissive;

    /* For alpha modes other than 'MASK', cutoff value is ignored. */
    int alphamode;
    float alpha_cutoff;

    /* Disables back-face culling. */
    int double_sided;
} yf_matlprop_t;

/**
 * Initializes a new material.
 *
 * @param prop: The 'yf_matlprop_t' to set on creation. Can be 'NULL'.
 * @return: On success, returns a new material. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
yf_material_t *yf_material_init(const yf_matlprop_t *prop);

/**
 * Gets the properties of a material.
 *
 * @param matl: The material.
 * @return: The material's properties.
 */
yf_matlprop_t *yf_material_getprop(yf_material_t *matl);

/**
 * Deinitializes a material.
 *
 * @param matl: The material to deinitialize. Can be 'NULL'.
 */
void yf_material_deinit(yf_material_t *matl);

YF_DECLS_END

#endif /* YF_YF_MATERIAL_H */
