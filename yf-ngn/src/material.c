/*
 * YF
 * material.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#ifdef YF_DEVEL
# include <stdio.h>
#endif

#include "yf/com/yf-error.h"

#include "yf-material.h"

struct yf_material {
    yf_matlprop_t prop;
};

yf_material_t *yf_material_init(const yf_matlprop_t *prop)
{
    yf_material_t *matl = calloc(1, sizeof(yf_material_t));
    if (matl == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }

    if (prop == NULL) {
        matl->prop.pbr = YF_PBR_SPECGLOSS;
        yf_vec4_set(matl->prop.pbrsg.diffuse_fac, 1.0f);
        yf_vec3_set(matl->prop.pbrsg.specular_fac, 1.0f);
        matl->prop.pbrsg.glossiness_fac = 1.0f;
        matl->prop.normal.scale = 1.0f;
        matl->prop.occlusion.strength = 1.0f;
        matl->prop.alphamode = YF_ALPHAMODE_OPAQUE;
    } else {
        matl->prop = *prop;
    }

    return matl;
}

yf_matlprop_t *yf_material_getprop(yf_material_t *matl)
{
    assert(matl != NULL);
    return &matl->prop;
}

void yf_material_deinit(yf_material_t *matl)
{
    if (matl != NULL)
        /* XXX: Textures not deinitialized. */
        free(matl);
}

/*
 * DEVEL
 */

#ifdef YF_DEVEL

void yf_print_matl(yf_material_t *matl)
{
    assert(matl != NULL);

    printf("\n[YF] OUTPUT (%s):\n"
           " material:\n", __func__);

    switch (matl->prop.pbr) {
    case YF_PBR_SPECGLOSS:
        printf("  pbr: PBR_SPECGLOSS\n"
               "   diffuse:\n"
               "    texture: <%p>\n"
               "    sampler:\n"
               "     wrap mode (u/v/w): %d/%d/%d\n"
               "     filter (mag/min/mipmap): %d/%d/%d\n"
               "    coordinate set: %d\n"
               "    factor: [%.4f, %.4f, %.4f, %.4f]\n",
               (void *)matl->prop.pbrsg.diffuse_tex.tex,
               matl->prop.pbrsg.diffuse_tex.splr.wrapmode.u,
               matl->prop.pbrsg.diffuse_tex.splr.wrapmode.v,
               matl->prop.pbrsg.diffuse_tex.splr.wrapmode.w,
               matl->prop.pbrsg.diffuse_tex.splr.filter.mag,
               matl->prop.pbrsg.diffuse_tex.splr.filter.min,
               matl->prop.pbrsg.diffuse_tex.splr.filter.mipmap,
               matl->prop.pbrsg.diffuse_tex.uvset,
               matl->prop.pbrsg.diffuse_fac[0],
               matl->prop.pbrsg.diffuse_fac[1],
               matl->prop.pbrsg.diffuse_fac[2],
               matl->prop.pbrsg.diffuse_fac[3]);
        printf("   spec-gloss:\n"
               "    texture: <%p>\n"
               "    sampler:\n"
               "     wrap mode (u/v/w): %d/%d/%d\n"
               "     filter (mag/min/mipmap): %d/%d/%d\n"
               "    coordinate set: %d\n"
               "    specular factor: [%.4f, %.4f, %.4f]\n"
               "    glossiness factor: %.4f\n",
               (void *)matl->prop.pbrsg.spec_gloss_tex.tex,
               matl->prop.pbrsg.spec_gloss_tex.splr.wrapmode.u,
               matl->prop.pbrsg.spec_gloss_tex.splr.wrapmode.v,
               matl->prop.pbrsg.spec_gloss_tex.splr.wrapmode.w,
               matl->prop.pbrsg.spec_gloss_tex.splr.filter.mag,
               matl->prop.pbrsg.spec_gloss_tex.splr.filter.min,
               matl->prop.pbrsg.spec_gloss_tex.splr.filter.mipmap,
               matl->prop.pbrsg.spec_gloss_tex.uvset,
               matl->prop.pbrsg.specular_fac[0],
               matl->prop.pbrsg.specular_fac[1],
               matl->prop.pbrsg.specular_fac[2],
               matl->prop.pbrsg.glossiness_fac);
        break;

    case YF_PBR_METALROUGH:
        printf("  pbr: PBR_METALROUGH\n"
               "   color:\n"
               "    texture: <%p>\n"
               "    sampler:\n"
               "     wrap mode (u/v/w): %d/%d/%d\n"
               "     filter (mag/min/mipmap): %d/%d/%d\n"
               "    coordinate set: %d\n"
               "    factor: [%.4f, %.4f, %.4f, %.4f]\n",
               (void *)matl->prop.pbrmr.color_tex.tex,
               matl->prop.pbrmr.color_tex.splr.wrapmode.u,
               matl->prop.pbrmr.color_tex.splr.wrapmode.v,
               matl->prop.pbrmr.color_tex.splr.wrapmode.w,
               matl->prop.pbrmr.color_tex.splr.filter.mag,
               matl->prop.pbrmr.color_tex.splr.filter.min,
               matl->prop.pbrmr.color_tex.splr.filter.mipmap,
               matl->prop.pbrmr.color_tex.uvset,
               matl->prop.pbrmr.color_fac[0], matl->prop.pbrmr.color_fac[1],
               matl->prop.pbrmr.color_fac[2], matl->prop.pbrmr.color_fac[3]);
        printf("   metal-rough:\n"
               "    texture: <%p>\n"
               "    sampler:\n"
               "     wrap mode (u/v/w): %d/%d/%d\n"
               "     filter (mag/min/mipmap): %d/%d/%d\n"
               "    coordinate set: %d\n"
               "    metallic factor: %.4f\n"
               "    roughness factor: %.4f\n",
               (void *)matl->prop.pbrmr.metal_rough_tex.tex,
               matl->prop.pbrmr.metal_rough_tex.splr.wrapmode.u,
               matl->prop.pbrmr.metal_rough_tex.splr.wrapmode.v,
               matl->prop.pbrmr.metal_rough_tex.splr.wrapmode.w,
               matl->prop.pbrmr.metal_rough_tex.splr.filter.mag,
               matl->prop.pbrmr.metal_rough_tex.splr.filter.min,
               matl->prop.pbrmr.metal_rough_tex.splr.filter.mipmap,
               matl->prop.pbrmr.metal_rough_tex.uvset,
               matl->prop.pbrmr.metallic_fac, matl->prop.pbrmr.roughness_fac);
        break;

    case YF_PBR_NONE:
        printf("  pbr: PBR_NONE\n"
               "   color:\n"
               "    texture: <%p>\n"
               "    sampler:\n"
               "     wrap mode (u/v/w): %d/%d/%d\n"
               "     filter (mag/min/mipmap): %d/%d/%d\n"
               "    coordinate set: %d\n"
               "    factor: [%.4f, %.4f, %.4f, %.4f]\n",
               (void *)matl->prop.nopbr.color_tex.tex,
               matl->prop.nopbr.color_tex.splr.wrapmode.u,
               matl->prop.nopbr.color_tex.splr.wrapmode.v,
               matl->prop.nopbr.color_tex.splr.wrapmode.w,
               matl->prop.nopbr.color_tex.splr.filter.mag,
               matl->prop.nopbr.color_tex.splr.filter.min,
               matl->prop.nopbr.color_tex.splr.filter.mipmap,
               matl->prop.nopbr.color_tex.uvset,
               matl->prop.nopbr.color_fac[0], matl->prop.nopbr.color_fac[1],
               matl->prop.nopbr.color_fac[2], matl->prop.nopbr.color_fac[3]);
        break;

    default:
        assert(0);
    }

    printf("  normal:\n"
           "   texture: <%p>\n"
           "   sampler:\n"
           "    wrap mode (u/v/w): %d/%d/%d\n"
           "    filter (mag/min/mipmap): %d/%d/%d\n"
           "   coordinate set: %d\n"
           "   scale: %.4f\n",
           (void*)matl->prop.normal.tex.tex,
           matl->prop.normal.tex.splr.wrapmode.u,
           matl->prop.normal.tex.splr.wrapmode.v,
           matl->prop.normal.tex.splr.wrapmode.w,
           matl->prop.normal.tex.splr.filter.mag,
           matl->prop.normal.tex.splr.filter.min,
           matl->prop.normal.tex.splr.filter.mipmap,
           matl->prop.normal.tex.uvset, matl->prop.normal.scale);

    printf("  occlusion:\n"
           "   texture: <%p>\n"
           "   sampler:\n"
           "    wrap mode (u/v/w): %d/%d/%d\n"
           "    filter (mag/min/mipmap): %d/%d/%d\n"
           "   coordinate set: %d\n"
           "   strength: %.4f\n",
           (void*)matl->prop.occlusion.tex.tex,
           matl->prop.occlusion.tex.splr.wrapmode.u,
           matl->prop.occlusion.tex.splr.wrapmode.v,
           matl->prop.occlusion.tex.splr.wrapmode.w,
           matl->prop.occlusion.tex.splr.filter.mag,
           matl->prop.occlusion.tex.splr.filter.min,
           matl->prop.occlusion.tex.splr.filter.mipmap,
           matl->prop.occlusion.tex.uvset, matl->prop.occlusion.strength);

    printf("  emissive:\n"
           "   texture: <%p>\n"
           "   sampler:\n"
           "    wrap mode (u/v/w): %d/%d/%d\n"
           "    filter (mag/min/mipmap): %d/%d/%d\n"
           "   coordinate set: %d\n"
           "   factor: [%.4f, %.4f, %.4f]\n",
           (void*)matl->prop.emissive.tex.tex,
           matl->prop.emissive.tex.splr.wrapmode.u,
           matl->prop.emissive.tex.splr.wrapmode.v,
           matl->prop.emissive.tex.splr.wrapmode.w,
           matl->prop.emissive.tex.splr.filter.mag,
           matl->prop.emissive.tex.splr.filter.min,
           matl->prop.emissive.tex.splr.filter.mipmap,
           matl->prop.emissive.tex.uvset, matl->prop.emissive.factor[0],
           matl->prop.emissive.factor[1], matl->prop.emissive.factor[2]);

    switch (matl->prop.alphamode) {
    case YF_ALPHAMODE_OPAQUE:
        puts("  alpha mode: ALPHAMODE_OPAQUE");
        break;
    case YF_ALPHAMODE_BLEND:
        puts("  alpha mode: ALPHAMODE_BLEND");
        break;
    default:
        assert(0);
    }

    puts("");
}

#endif
