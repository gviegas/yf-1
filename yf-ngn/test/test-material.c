/*
 * YF
 * test-material.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "test.h"
#include "print.h"
#include "yf-material.h"

#define YF_CMPTEXREF(a, b) do { \
    if ((a).tex != (b).tex || \
        (a).splr.wrapmode.u != (b).splr.wrapmode.u || \
        (a).splr.wrapmode.v != (b).splr.wrapmode.v || \
        (a).splr.wrapmode.w != (b).splr.wrapmode.w || \
        (a).splr.filter.mag != (b).splr.filter.mag || \
        (a).splr.filter.min != (b).splr.filter.min || \
        (a).splr.filter.mipmap != (b).splr.filter.mipmap || \
        (a).uvset != (b).uvset) \
        return -1; } while (0)

#define YF_CMPMATL(a, b) do { \
    if ((a)->pbr != (b)->pbr) \
        return -1; \
    if ((a)->pbr == YF_PBR_SPECGLOSS) { \
        YF_CMPTEXREF((a)->pbrsg.diffuse_tex, (b)->pbrsg.diffuse_tex); \
        YF_CMPTEXREF((a)->pbrsg.spec_gloss_tex, (b)->pbrsg.spec_gloss_tex); \
        if (!yf_vec4_iseq((a)->pbrsg.diffuse_fac, (b)->pbrsg.diffuse_fac) || \
            !yf_vec3_iseq((a)->pbrsg.specular_fac, (b)->pbrsg.specular_fac) || \
            (a)->pbrsg.glossiness_fac != (b)->pbrsg.glossiness_fac) \
            return -1; \
    } else if ((a)->pbr == YF_PBR_METALROUGH) { \
        YF_CMPTEXREF((a)->pbrmr.color_tex, (b)->pbrmr.color_tex); \
        YF_CMPTEXREF((a)->pbrmr.metal_rough_tex, (b)->pbrmr.metal_rough_tex); \
        if (!yf_vec4_iseq((a)->pbrmr.color_fac, (b)->pbrmr.color_fac) || \
            (a)->pbrmr.metallic_fac != (b)->pbrmr.metallic_fac || \
            (a)->pbrmr.roughness_fac != (b)->pbrmr.roughness_fac) \
            return -1; \
    } else if ((a)->pbr == YF_PBR_NONE) { \
        YF_CMPTEXREF((a)->nopbr.color_tex, (b)->nopbr.color_tex); \
        if (!yf_vec4_iseq((a)->nopbr.color_fac, (b)->nopbr.color_fac)) \
            return -1; \
    } else \
        return -1; \
    YF_CMPTEXREF((a)->normal.tex, (b)->normal.tex); \
    if ((a)->normal.scale != (b)->normal.scale) \
        return -1; \
    YF_CMPTEXREF((a)->occlusion.tex, (b)->occlusion.tex); \
    if ((a)->occlusion.strength != (b)->occlusion.strength) \
        return -1; \
    YF_CMPTEXREF((a)->emissive.tex, (b)->emissive.tex); \
    if (!yf_vec3_iseq((a)->emissive.factor, (b)->emissive.factor)) \
        return -1; \
    if ((a)->alphamode != (b)->alphamode) \
        return -1; } while (0)

/* Tests material. */
int yf_test_material(void)
{
    YF_TEST_PRINT("init", "NULL", "matl");
    yf_material_t *matl = yf_material_init(NULL);
    if (matl == NULL)
        return -1;

    yf_print_matl(matl);

    yf_matlprop_t prop_sg = {
        .pbr = YF_PBR_SPECGLOSS,
        .pbrsg = {
            .diffuse_tex = {0},
            .diffuse_fac = {1.0f, 0.5f, 0.25f, 0.125f},
            .spec_gloss_tex = {0},
            .specular_fac = {0.667f, 0.333f, 0.0f},
            .glossiness_fac = 0.999f
        },
        .normal = {
            .tex = {0},
            .scale = 0.25f
        },
        .occlusion = {
            .tex = {0},
            .strength = 0.45f
        },
        .emissive = {
            .tex = {0},
            .factor = {0.1f, 0.01f, 0.001f}
        },
        .alphamode = YF_ALPHAMODE_BLEND
    };

    YF_TEST_PRINT("init", "&prop_sg", "matl2");
    yf_material_t *matl2 = yf_material_init(&prop_sg);
    if (matl2 == NULL)
        return -1;

    yf_print_matl(matl2);

    yf_matlprop_t prop_mr = {
        .pbr = YF_PBR_METALROUGH,
        .pbrmr = {
            .color_tex = {0},
            .color_fac = {0.2f, 0.4f, 0.8f, 1.0f},
            .metal_rough_tex = {0},
            .metallic_fac = 0.5f,
            .roughness_fac = 0.2f
        },
        .normal = {
            .tex = {0},
            .scale = 0.625f
        },
        .occlusion = {
            .tex = {0},
            .strength = 0.175f
        },
        .emissive = {
            .tex = {0},
            .factor = {0.3f, 0.2f, 0.1f}
        },
        .alphamode = YF_ALPHAMODE_OPAQUE
    };

    YF_TEST_PRINT("init", "&prop_mr", "matl3");
    yf_material_t *matl3 = yf_material_init(&prop_mr);
    if (matl3 == NULL)
        return -1;

    yf_print_matl(matl3);

    yf_matlprop_t prop_ul = {
        .pbr = YF_PBR_NONE,
        .nopbr = {
            .color_tex = {0},
            .color_fac = {0.01f, 0.02f, 0.04f, 1.0f}
        },
        .normal = {{0}},
        .occlusion = {{0}},
        .emissive = {{0}},
        .alphamode = YF_ALPHAMODE_OPAQUE
    };

    YF_TEST_PRINT("init", "&prop_ul", "matl4");
    yf_material_t *matl4 = yf_material_init(&prop_ul);
    if (matl4 == NULL)
        return -1;

    yf_print_matl(matl4);

    const yf_matlprop_t *prop;

    YF_TEST_PRINT("getprop", "matl", "");
    prop = yf_material_getprop(matl);
    if (prop == NULL)
        return -1;

    YF_TEST_PRINT("getprop", "matl2", "");
    prop = yf_material_getprop(matl2);
    if (prop == NULL)
        return -1;
    YF_CMPMATL(prop, &prop_sg);

    YF_TEST_PRINT("getprop", "matl3", "");
    prop = yf_material_getprop(matl3);
    if (prop == NULL)
        return -1;
    YF_CMPMATL(prop, &prop_mr);

    YF_TEST_PRINT("getprop", "matl4", "");
    prop = yf_material_getprop(matl4);
    if (prop == NULL)
        return -1;
    YF_CMPMATL(prop, &prop_ul);

    YF_TEST_PRINT("deinit", "matl", "");
    yf_material_deinit(matl);

    YF_TEST_PRINT("deinit", "matl2", "");
    yf_material_deinit(matl2);

    YF_TEST_PRINT("deinit", "matl3", "");
    yf_material_deinit(matl3);

    YF_TEST_PRINT("deinit", "matl4", "");
    yf_material_deinit(matl4);

    return 0;
}
