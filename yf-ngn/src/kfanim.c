/*
 * YF
 * kfanim.c
 *
 * Copyright © 2023 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <assert.h>

#ifdef YF_DEVEL
# include <stdio.h>
#endif

#include "yf/com/yf-util.h"
#include "yf/com/yf-error.h"

#include "yf-kfanim.h"

struct yf_kfanim {
    yf_kfin_t *inputs;
    unsigned input_n;
    yf_kfout_t *outputs;
    unsigned output_n;
    yf_kfact_t *acts;
    unsigned act_n;

    /* targets can be set (or unset) at any time */
    yf_node_t **targets;

    /* considering every timeline of every input */
    float duration;
};

/* Gets a pair of timeline indices defining keyframes for interpolation. */
static void get_keyframes(const yf_kfin_t *input, float frame_tm,
                          unsigned *i1, unsigned *i2)
{
    const float *timeline = input->timeline;
    const unsigned n = input->n;
    assert(n > 0);

    if (timeline[0] > frame_tm) {
        *i1 = *i2 = 0;
        return;
    }

    if (timeline[n-1] < frame_tm) {
        *i1 = *i2 = n - 1;
        return;
    }

    unsigned beg = 0;
    unsigned end = n - 1;
    unsigned cur = (beg + end) >> 1;

    while (beg < end) {
        if (timeline[cur] < frame_tm)
            beg = cur + 1;
        else if (timeline[cur] > frame_tm)
            end = cur - 1;
        else
            break;

        cur = (beg + end) >> 1;
    }

    if (timeline[cur] > frame_tm) {
        *i1 = cur - 1;
        *i2 = cur;
    } else {
        *i1 = cur;
        *i2 = cur + 1;
    }
}

/* Linear interpolation on 3-component vectors. */
static void lerp3(yf_vec3_t dst, const yf_vec3_t a, const yf_vec3_t b, float t)
{
    dst[0] = (1.0f - t) * a[0] + t * b[0];
    dst[1] = (1.0f - t) * a[1] + t * b[1];
    dst[2] = (1.0f - t) * a[2] + t * b[2];
}

/* Spherical linear interpolation on quaternions. */
static void slerpq(yf_vec4_t dst, const yf_vec4_t a, const yf_vec4_t b, float t)
{
    float d = yf_vec3_dot(a, b);
    if (d > 1.0f - FLT_EPSILON) {
        lerp3(dst, a, b, t);
        dst[3] = (1.0f - t) * a[3] + t * b[3];
        return;
    }

    float k = 1.0f;
    if (d < 0.0f) {
        k = -k;
        d = -d;
    }

    const float ang = acosf(d);
    const float s = sinf(ang);
    const float s1 = sinf((1.0f - t) * ang);
    const float s2 = sinf(t * ang);

    dst[0] = (a[0] * s1 + b[0] * s2 * k) / s;
    dst[1] = (a[1] * s1 + b[1] * s2 * k) / s;
    dst[2] = (a[2] * s1 + b[2] * s2 * k) / s;
    dst[3] = (a[3] * s1 + b[3] * s2 * k) / s;
}

yf_kfanim_t *yf_kfanim_init(const yf_kfin_t *inputs, unsigned input_n,
                            const yf_kfout_t *outputs, unsigned output_n,
                            const yf_kfact_t *acts, unsigned act_n)
{
    assert(inputs != NULL && input_n > 0);
    assert(outputs != NULL && output_n > 0);
    assert(acts != NULL && act_n > 0);

    /* TODO: Ensure that all 'inputs'/'outputs' are referenced by 'acts'. */

    yf_kfanim_t *anim = calloc(1, sizeof(yf_kfanim_t));
    if (anim == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }
    anim->targets = calloc(act_n, sizeof(yf_node_t *));
    if (anim->targets == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        free(anim);
        return NULL;
    }

    /* inputs */
    anim->inputs = calloc(input_n, sizeof *inputs);
    if (anim->inputs == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        yf_kfanim_deinit(anim);
        return NULL;
    }
    anim->input_n = input_n;

    float tm_min = 0.0f;
    float tm_max = 0.0f;

    for (unsigned i = 0; i < input_n; i++) {
        assert(inputs[i].timeline != NULL && inputs[i].n > 0);

        const size_t sz = inputs[i].n * sizeof *inputs[i].timeline;
        anim->inputs[i].timeline = malloc(sz);
        if (anim->inputs[i].timeline == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            yf_kfanim_deinit(anim);
            return NULL;
        }
        memcpy(anim->inputs[i].timeline, inputs[i].timeline, sz);
        anim->inputs[i].n = inputs[i].n;

        tm_min = YF_MIN(tm_min, inputs[i].timeline[0]);
        tm_max = YF_MAX(tm_max, inputs[i].timeline[inputs[i].n - 1]);
    }

    if (tm_min < 0.0f || tm_min > tm_max) {
        yf_seterr(YF_ERR_INVARG, __func__);
        yf_kfanim_deinit(anim);
        return NULL;
    }
    anim->duration = tm_max - tm_min;

    /* outputs */
    anim->outputs = calloc(output_n, sizeof *outputs);
    if (anim->outputs == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        yf_kfanim_deinit(anim);
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
                yf_kfanim_deinit(anim);
                return NULL;
            }
            memcpy(anim->outputs[i].t, outputs[i].t, sz);
            break;

        case YF_KFPROP_R:
            sz = outputs[i].n * sizeof *outputs[i].r;
            anim->outputs[i].r = malloc(sz);
            if (anim->outputs[i].r == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                yf_kfanim_deinit(anim);
                return NULL;
            }
            memcpy(anim->outputs[i].r, outputs[i].r, sz);
            break;

        case YF_KFPROP_S:
            sz = outputs[i].n * sizeof *outputs[i].s;
            anim->outputs[i].s = malloc(sz);
            if (anim->outputs[i].s == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                yf_kfanim_deinit(anim);
                return NULL;
            }
            memcpy(anim->outputs[i].s, outputs[i].s, sz);
            break;

        default:
            assert(0);
            yf_seterr(YF_ERR_INVARG, __func__);
            yf_kfanim_deinit(anim);
            return NULL;
        }

        anim->outputs[i].kfprop = outputs[i].kfprop;
        anim->outputs[i].n = outputs[i].n;
    }

    /* acts */
    anim->acts = malloc(act_n * sizeof *acts);
    if (anim->acts == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        yf_kfanim_deinit(anim);
        return NULL;
    }
    memcpy(anim->acts, acts, act_n * sizeof *acts);
    anim->act_n = act_n;

    return anim;
}

yf_node_t *yf_kfanim_gettarget(yf_kfanim_t *anim, unsigned act)
{
    assert(anim != NULL);

    if (act < anim->act_n)
        return anim->targets[act];
    yf_seterr(YF_ERR_INVARG, __func__);
    return NULL;
}

int yf_kfanim_settarget(yf_kfanim_t *anim, unsigned act, yf_node_t *target)
{
    assert(anim != NULL);

    if (act < anim->act_n) {
        anim->targets[act] = target;
        return 0;
    }
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
}

const yf_kfin_t *yf_kfanim_getins(yf_kfanim_t *anim, unsigned *n)
{
    assert(anim != NULL);
    assert(n != NULL);

    *n = anim->input_n;
    return anim->inputs;
}

const yf_kfout_t *yf_kfanim_getouts(yf_kfanim_t *anim, unsigned *n)
{
    assert(anim != NULL);
    assert(n != NULL);

    *n = anim->output_n;
    return anim->outputs;
}

const yf_kfact_t *yf_kfanim_getacts(yf_kfanim_t *anim, unsigned *n)
{
    assert(anim != NULL);
    assert(n != NULL);

    *n = anim->act_n;
    return anim->acts;
}

float yf_kfanim_apply(yf_kfanim_t *anim, float frame_tm)
{
    assert(anim != NULL);

    for (unsigned i = 0; i < anim->act_n; i++) {
        yf_node_t *node = anim->targets[i];
        if (node == NULL)
            continue;

        const yf_kfact_t *act = &anim->acts[i];
        const yf_kfin_t *in = &anim->inputs[act->in_i];
        const yf_kfout_t *out = &anim->outputs[act->out_i];

        unsigned i1, i2;
        get_keyframes(in, frame_tm, &i1, &i2);

        switch (out->kfprop) {
        case YF_KFPROP_T:
            switch (act->kferp) {
            case YF_KFERP_STEP:
                if (frame_tm - in->timeline[i1] < in->timeline[i2] - frame_tm)
                    yf_vec3_copy(*yf_node_gett(node), out->t[i1]);
                else
                    yf_vec3_copy(*yf_node_gett(node), out->t[i2]);
                break;
            case YF_KFERP_LINEAR:
                if (i1 != i2)
                    lerp3(*yf_node_gett(node), out->t[i1], out->t[i2],
                          (frame_tm - in->timeline[i1]) /
                          (in->timeline[i2] - in->timeline[i1]));
                else
                    yf_vec3_copy(*yf_node_gett(node), out->t[i1]);
                break;
            default:
                assert(0);
                abort();
            }
            break;

        case YF_KFPROP_R:
            switch (act->kferp) {
            case YF_KFERP_STEP:
                if (frame_tm - in->timeline[i1] < in->timeline[i2] - frame_tm)
                    yf_vec4_copy(*yf_node_getr(node), out->r[i1]);
                else
                    yf_vec4_copy(*yf_node_getr(node), out->r[i2]);
                break;
            case YF_KFERP_LINEAR:
                if (i1 != i2)
                    slerpq(*yf_node_getr(node), out->r[i1], out->r[i2],
                           (frame_tm - in->timeline[i1]) /
                           (in->timeline[i2] - in->timeline[i1]));
                else
                    yf_vec4_copy(*yf_node_getr(node), out->r[i1]);
                break;
            default:
                assert(0);
                abort();
            }
            break;

        case YF_KFPROP_S:
            switch (act->kferp) {
            case YF_KFERP_STEP:
                if (frame_tm - in->timeline[i1] < in->timeline[i2] - frame_tm)
                    yf_vec3_copy(*yf_node_gets(node), out->s[i1]);
                else
                    yf_vec3_copy(*yf_node_gets(node), out->s[i2]);
                break;
            case YF_KFERP_LINEAR:
                if (i1 != i2)
                    lerp3(*yf_node_gets(node), out->s[i1], out->s[i2],
                          (frame_tm - in->timeline[i1]) /
                          (in->timeline[i2] - in->timeline[i1]));
                else
                    yf_vec3_copy(*yf_node_gets(node), out->s[i1]);
                break;
            default:
                assert(0);
                abort();
            }
            break;

        default:
            assert(0);
            abort();
        }
    }

    return anim->duration - frame_tm;
}

void yf_kfanim_deinit(yf_kfanim_t *anim)
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

    free(anim->acts);
    free(anim->targets);
    free(anim);
}

/*
 * DEVEL
 */

#ifdef YF_DEVEL

void yf_print_anim(yf_kfanim_t *anim)
{
    assert(anim != NULL);

    printf("\n[YF] OUTPUT (%s):\n", __func__);

    const yf_kfin_t *ins = anim->inputs;
    const yf_kfout_t *outs = anim->outputs;
    const yf_kfact_t *acts = anim->acts;

    printf(" Keyframe animation:\n"
           "  duration: %.6f\n"
           "  inputs (%u):\n",
           anim->duration, anim->input_n);
    for (unsigned i = 0; i < anim->input_n; i++) {
        printf("   input [%u]:\n"
               "    number of samples: %u\n"
               "    timeline:\n",
               i, ins[i].n);

        for (unsigned j = 0; j < ins[i].n; j++)
            printf("     [%.5u]  %.6f\n", j, ins[i].timeline[j]);
    }

    printf("  outputs (%u):\n", anim->output_n);
    for (unsigned i = 0; i < anim->output_n; i++) {
        printf("   output [%u]:\n"
               "    number of samples: %u\n"
               "    animated property: ",
               i, outs[i].n);

        switch (outs[i].kfprop) {
        case YF_KFPROP_T:
            puts("KFPROP_T\n    values:");
            for (unsigned j = 0; j < outs[i].n; j++)
                printf("     [%.5u]  %.4f, %.4f, %.4f\n", j, outs[i].t[j][0],
                       outs[i].t[j][1], outs[i].t[j][2]);
            break;
        case YF_KFPROP_R:
            puts("KFPROP_R\n    values:");
            for (unsigned j = 0; j < outs[i].n; j++)
                printf("     [%.5u]  %.4f, %.4f, %.4f, %.4f\n", j,
                       outs[i].r[j][0], outs[i].r[j][1], outs[i].r[j][2],
                       outs[i].r[j][3]);
            break;
        case YF_KFPROP_S:
            puts("KFPROP_S\n    values:");
            for (unsigned j = 0; j < outs[i].n; j++)
                printf("     [%.5u]  %.4f, %.4f, %.4f\n", j, outs[i].s[j][0],
                       outs[i].s[j][1], outs[i].s[j][2]);
            break;
        default:
            assert(0);
        }
    }

    printf("  acts (%u):\n", anim->act_n);
    for (unsigned i = 0; i < anim->act_n; i++) {
        char *erp = "";
        switch (acts[i].kferp) {
        case YF_KFERP_STEP:
            erp = "KFERP_STEP";
            break;
        case YF_KFERP_LINEAR:
            erp = "KFERP_LINEAR";
            break;
        default:
            assert(0);
        }

        printf("   act [%u]:\n"
               "    interpolation method: %s\n"
               "    input index: %u\n"
               "    output index: %u\n",
               i, erp, acts[i].in_i, acts[i].out_i);

        yf_node_t *node = anim->targets[i];
        if (node != NULL) {
            size_t len;
            yf_node_getname(node, NULL, &len);
            char name[len];
            yf_node_getname(node, name, &len);
            printf("    target node: '%s' <%p>\n", name, (void *)node);
        } else {
            printf("    (no target set for this act)\n");
        }
    }

    puts("");
}

#endif
