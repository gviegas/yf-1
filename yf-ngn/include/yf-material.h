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
 * Opaque type defining a material resource.
 */
typedef struct YF_material_o *YF_material;

/**
 * PBR methodologies.
 */
#define YF_PBR_SPECGLOSS  0
#define YF_PBR_METALROUGH 1

/**
 * Alpha modes.
 */
#define YF_ALPHAMODE_OPAQUE 0
#define YF_ALPHAMODE_BLEND  1

/**
 * Type defining the material properties of an object.
 */
typedef struct {
    int pbr;
    union {
        /* PBR specular-glossiness. */
        struct {
            YF_texture diffuse_tex;
            YF_vec4 diffuse_fac;
            YF_texture spec_gloss_tex;
            YF_vec3 specular_fac;
            YF_float glossiness_fac;
        } pbrsg;
        /* PBR metallic-roughness. */
        struct {
            YF_texture color_tex;
            YF_vec4 color_fac;
            YF_texture metal_rough_tex;
            YF_float metallic_fac;
            YF_float roughness_fac;
        } pbrmr;
    };
    struct {
        YF_texture tex;
        YF_float scale;
    } normal;
    struct {
        YF_texture tex;
        YF_float strength;
    } occlusion;
    struct {
        YF_texture tex;
        YF_vec3 factor;
    } emissive;
    int alphamode;
} YF_matlprop;

/**
 * Initializes a new material.
 *
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
