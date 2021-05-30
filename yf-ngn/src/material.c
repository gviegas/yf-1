/*
 * YF
 * material.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include "yf/com/yf-error.h"

#include "yf-material.h"

struct YF_material_o {
    YF_matlprop prop;
};

YF_material yf_material_init(void)
{
    YF_material matl = calloc(1, sizeof(struct YF_material_o));
    if (matl == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }

    matl->prop.pbr = YF_PBR_SPECGLOSS;
    yf_vec4_set(matl->prop.pbrsg.diffuse_fac, 1.0);
    yf_vec3_set(matl->prop.pbrsg.specular_fac, 1.0);
    matl->prop.pbrsg.glossiness_fac = 1.0;
    matl->prop.normal.scale = 1.0;
    matl->prop.occlusion.strength = 1.0;
    matl->prop.alphamode = YF_ALPHAMODE_OPAQUE;

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
