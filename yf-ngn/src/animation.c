/*
 * YF
 * animation.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "yf/com/yf-error.h"

#include "yf-animation.h"

struct YF_animation_o {
    YF_kfinput *inputs;
    unsigned input_n;
    YF_kfoutput *outputs;
    unsigned output_n;
    YF_kfaction *actions;
    unsigned action_n;
    /* TODO */
};

YF_animation yf_animation_init(const YF_kfinput *inputs, unsigned input_n,
                               const YF_kfoutput *outputs, unsigned output_n,
                               const YF_kfaction *actions, unsigned action_n)
{
    assert(inputs != NULL && input_n > 0);
    assert(outputs != NULL && output_n > 0);
    assert(actions != NULL && action_n > 0);

    YF_animation anim = calloc(1, sizeof(struct YF_animation_o));
    if (anim == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }

    /* inputs */
    anim->inputs = calloc(input_n, sizeof *inputs);
    if (anim->inputs == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        yf_animation_deinit(anim);
        return NULL;
    }
    anim->input_n = input_n;

    for (unsigned i = 0; i < input_n; i++) {
        assert(inputs[i].timeline != NULL && inputs[i].n > 0);

        const size_t sz = inputs[i].n * sizeof *inputs[i].timeline;
        anim->inputs[i].timeline = malloc(sz);
        if (anim->inputs[i].timeline == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            yf_animation_deinit(anim);
            return NULL;
        }
        memcpy(anim->inputs[i].timeline, inputs[i].timeline, sz);
        anim->inputs[i].n = inputs[i].n;
    }

    /* outputs */
    anim->outputs = calloc(output_n, sizeof *outputs);
    if (anim->outputs == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        yf_animation_deinit(anim);
        return NULL;
    }
    anim->output_n = output_n;

    for (unsigned i = 0; i < output_n; i++) {
        assert(outputs[i].n > 0);

        size_t sz;
        switch (outputs[i].kfprop) {
        case YF_KFPROP_T:
            sz = outputs[i].n * sizeof *outputs[i].t;
            anim->outputs[i].t = malloc(sz);
            if (anim->outputs[i].t == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                yf_animation_deinit(anim);
                return NULL;
            }
            memcpy(anim->outputs[i].t, outputs[i].t, sz);
            break;

        case YF_KFPROP_R:
            sz = outputs[i].n * sizeof *outputs[i].r;
            anim->outputs[i].r = malloc(sz);
            if (anim->outputs[i].r == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                yf_animation_deinit(anim);
                return NULL;
            }
            memcpy(anim->outputs[i].r, outputs[i].r, sz);
            break;

        case YF_KFPROP_S:
            sz = outputs[i].n * sizeof *outputs[i].s;
            anim->outputs[i].s = malloc(sz);
            if (anim->outputs[i].s == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                yf_animation_deinit(anim);
                return NULL;
            }
            memcpy(anim->outputs[i].s, outputs[i].s, sz);
            break;

        default:
            assert(0);
            yf_seterr(YF_ERR_INVARG, __func__);
            yf_animation_deinit(anim);
            return NULL;
        }

        anim->outputs[i].kfprop = outputs[i].kfprop;
        anim->outputs[i].n = outputs[i].n;
    }

    /* actions */
    anim->actions = malloc(action_n * sizeof *actions);
    if (anim->actions == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        yf_animation_deinit(anim);
        return NULL;
    }
    memcpy(anim->actions, actions, action_n * sizeof *actions);
    anim->action_n = action_n;

    return anim;
}

const YF_kfinput *yf_animation_getins(YF_animation anim, unsigned *n)
{
    assert(anim != NULL);
    assert(n != NULL);

    *n = anim->input_n;
    return anim->inputs;
}

void yf_animation_deinit(YF_animation anim)
{
    if (anim == NULL)
        return;

    if (anim->inputs != NULL) {
        for (unsigned i = 0; i < anim->input_n; i++)
            free(anim->inputs[i].timeline);
        free(anim->inputs);
    }

    if (anim->outputs != NULL) {
        for (unsigned i = 0; i < anim->output_n; i++) {
            switch (anim->outputs[i].kfprop) {
            case YF_KFPROP_T:
                free(anim->outputs[i].t);
                break;
            case YF_KFPROP_R:
                free(anim->outputs[i].r);
                break;
            case YF_KFPROP_S:
                free(anim->outputs[i].s);
                break;
            default:
                break;
            }
        }
        free(anim->outputs);
    }

    free(anim->actions);
    free(anim);
}
