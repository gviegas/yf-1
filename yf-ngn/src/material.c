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

struct YF_material_o {
    YF_matlprop prop;
};

YF_material yf_material_init(const YF_matlprop *prop)
{
    YF_material matl = calloc(1, sizeof(struct YF_material_o));
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

YF_matlprop *yf_material_getprop(YF_material matl)
{
    assert(matl != NULL);
    return &matl->prop;
}

void yf_material_deinit(YF_material matl)
{
    if (matl != NULL)
        /* XXX: Textures not deinitialized. */
        free(matl);
}

/*
 * DEVEL
 */

#ifdef YF_DEVEL

void yf_print_matl(YF_material matl)
{
    assert(matl != NULL);

    printf("\n[YF] OUTPUT (%s):\n"
           " material:\n", __func__);

    switch (matl->prop.pbr) {
    case YF_PBR_SPECGLOSS:
        printf("  pbr: PBR_SPECGLOSS\n"
               "   diffuse texture: %p\n"
               "   diffuse factor: [%.4f, %.4f, %.4f, %.4f]\n"
               "   spec-gloss texture: %p\n"
               "   specular factor: [%.4f, %.4f, %.4f]\n"
               "   glossiness factor: %.4f\n",
               (void *)matl->prop.pbrsg.diffuse_tex.tex,
               matl->prop.pbrsg.diffuse_fac[0],
               matl->prop.pbrsg.diffuse_fac[1],
               matl->prop.pbrsg.diffuse_fac[2],
               matl->prop.pbrsg.diffuse_fac[3],
               (void *)matl->prop.pbrsg.spec_gloss_tex.tex,
               matl->prop.pbrsg.specular_fac[0],
               matl->prop.pbrsg.specular_fac[1],
               matl->prop.pbrsg.specular_fac[2],
               matl->prop.pbrsg.glossiness_fac);
        break;
    case YF_PBR_METALROUGH:
        printf("  pbr: PBR_METALROUGH\n"
               "   color texture: %p\n"
               "   color factor: [%.4f, %.4f, %.4f, %.4f]\n"
               "   metal-rough texture: %p\n"
               "   metallic factor: %.4f\n"
               "   roughness factor: %.4f\n",
               (void *)matl->prop.pbrmr.color_tex.tex,
               matl->prop.pbrmr.color_fac[0], matl->prop.pbrmr.color_fac[1],
               matl->prop.pbrmr.color_fac[2], matl->prop.pbrmr.color_fac[3],
               (void *)matl->prop.pbrmr.metal_rough_tex.tex,
               matl->prop.pbrmr.metallic_fac, matl->prop.pbrmr.roughness_fac);
        break;
    case YF_PBR_NONE:
        printf("  pbr: PBR_NONE\n"
               "   color texture: %p\n"
               "   color factor: [%.4f, %.4f, %.4f, %.4f]\n",
               (void *)matl->prop.nopbr.color_tex.tex,
               matl->prop.nopbr.color_fac[0], matl->prop.nopbr.color_fac[1],
               matl->prop.nopbr.color_fac[2], matl->prop.nopbr.color_fac[3]);
        break;
    default:
        assert(0);
    }

    printf("  normal:\n"
           "   texture: %p\n"
           "   scale: %.4f\n"
           "  occlusion:\n"
           "   texture: %p\n"
           "   strength: %.4f\n"
           "  emissive:\n"
           "   texture: %p\n"
           "   factor: [%.4f, %.4f, %.4f]\n"
           "  alpha mode: ALPHAMODE_",
           (void*)matl->prop.normal.tex.tex, matl->prop.normal.scale,
           (void*)matl->prop.occlusion.tex.tex, matl->prop.occlusion.strength,
           (void*)matl->prop.emissive.tex.tex, matl->prop.emissive.factor[0],
           matl->prop.emissive.factor[1], matl->prop.emissive.factor[2]);

    switch (matl->prop.alphamode) {
    case YF_ALPHAMODE_OPAQUE:
        puts("OPAQUE");
        break;
    case YF_ALPHAMODE_BLEND:
        puts("BLEND");
        break;
    default:
        assert(0);
    }

    puts("");
}

#endif
