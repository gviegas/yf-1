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
typedef struct YF_material_o *YF_material;

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
typedef struct {
    int pbr;
    union {
        /* PBR specular-glossiness ('PBR_SPECGLOSS'). */
        struct {
            YF_texref diffuse_tex;
            YF_vec4 diffuse_fac;
            YF_texref spec_gloss_tex;
            YF_vec3 specular_fac;
            float glossiness_fac;
        } pbrsg;

        /* PBR metallic-roughness ('PBR_METALROUGH'). */
        struct {
            YF_texref color_tex;
            YF_vec4 color_fac;
            YF_texref metal_rough_tex;
            float metallic_fac;
            float roughness_fac;
        } pbrmr;

        /* Unlit ('PBR_NONE'). */
        struct {
            YF_texref color_tex;
            YF_vec4 color_fac;
        } nopbr;
    };

    /* Additional maps.
       These are ignored when 'pbr' is 'PBR_NONE'. */
    struct {
        YF_texref tex;
        float scale;
    } normal;
    struct {
        YF_texref tex;
        float strength;
    } occlusion;
    struct {
        YF_texref tex;
        YF_vec3 factor;
    } emissive;

    /* For alpha modes other than 'MASK', cutoff value is ignored. */
    int alphamode;
    float alpha_cutoff;

    /* Disables back-face culling. */
    int double_sided;
} YF_matlprop;

/**
 * Initializes a new material.
 *
 * @param prop: The 'YF_matlprop' to set on creation. Can be 'NULL'.
 * @return: On success, returns a new material. Otherwise, 'NULL' is returned
 *  and the global error is set to indicate the cause.
 */
YF_material yf_material_init(const YF_matlprop *prop);

/**
 * Gets the properties of a material.
 *
 * @param matl: The material.
 * @return: The material's properties.
 */
YF_matlprop *yf_material_getprop(YF_material matl);

/**
 * Deinitializes a material.
 *
 * @param matl: The material to deinitialize. Can be 'NULL'.
 */
void yf_material_deinit(YF_material matl);

YF_DECLS_END

#endif /* YF_YF_MATERIAL_H */
