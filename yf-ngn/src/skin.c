/*
 * YF
 * skin.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "yf/com/yf-error.h"

#include "yf-skin.h"

struct YF_skin_o {
    YF_joint *jnts;
    unsigned jnt_n;
};

YF_skin yf_skin_init(const YF_joint *jnts, unsigned jnt_n)
{
    if (jnts == NULL || jnt_n == 0) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return NULL;
    }

    YF_skin skin = malloc(sizeof(struct YF_skin_o));
    if (skin == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }

    skin->jnts = malloc(jnt_n * sizeof *jnts);
    if (skin->jnts == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        free(skin);
        return NULL;
    }
    memcpy(skin->jnts, jnts, jnt_n * sizeof *jnts);
    skin->jnt_n = jnt_n;

    return skin;
}
