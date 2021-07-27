/*
 * YF
 * animation.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <assert.h>

#include "yf/com/yf-util.h"
#include "yf/com/yf-error.h"

#include "yf-animation.h"

struct YF_animation_o {
    YF_kfinput *inputs;
    unsigned input_n;
    YF_kfoutput *outputs;
    unsigned output_n;
    YF_kfaction *actions;
    unsigned action_n;

    /* targets can be set (or unset) at any time */
    YF_node *targets;
};

/* Gets a pair of timeline indices defining keyframes for interpolation. */
static void get_keyframes(const YF_kfinput *input, float frame_tm,
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
static void lerp3(YF_vec3 dst, const YF_vec3 a, const YF_vec3 b, YF_float t)
{
    dst[0] = ((YF_float)1 - t) * a[0] + t * b[0];
    dst[1] = ((YF_float)1 - t) * a[1] + t * b[1];
    dst[2] = ((YF_float)1 - t) * a[2] + t * b[2];
}

/* Spherical linear interpolation on quaternions. */
static void slerpq(YF_vec4 dst, const YF_vec4 a, const YF_vec4 b, YF_float t)
{
    YF_float d = yf_vec3_dot(a, b);

#ifdef YF_USE_FLOAT64
    const YF_float e = 1.0 - DBL_EPSILON;
#else
    const YF_float e = 1.0f - FLT_EPSILON;
#endif

    if (d > e) {
        lerp3(dst, a, b, t);
        dst[3] = ((YF_float)1 - t) * a[3] + t * b[3];
        return;
    }

    YF_float k = (YF_float)1;
    if (d < (YF_float)0) {
        k = -k;
        d = -d;
    }

#ifdef YF_USE_FLOAT64
    const YF_float ang = acos(d);
    const YF_float s = sin(ang);
    const YF_float s1 = sin((1.0 - t) * ang);
    const YF_float s2 = sin(t * ang);
#else
    const YF_float ang = acosf(d);
    const YF_float s = sinf(ang);
    const YF_float s1 = sinf((1.0f - t) * ang);
    const YF_float s2 = sinf(t * ang);
#endif

    dst[0] = (a[0] * s1 + b[0] * s2 * k) / s;
    dst[1] = (a[1] * s1 + b[1] * s2 * k) / s;
    dst[2] = (a[2] * s1 + b[2] * s2 * k) / s;
    dst[3] = (a[3] * s1 + b[3] * s2 * k) / s;
}

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
    anim->targets = calloc(action_n, sizeof(YF_node));
    if (anim->targets == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        free(anim);
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

YF_node yf_animation_gettarget(YF_animation anim, unsigned action)
{
    assert(anim != NULL);

    if (action < anim->action_n)
        return anim->targets[action];
    yf_seterr(YF_ERR_INVARG, __func__);
    return NULL;
}

int yf_animation_settarget(YF_animation anim, unsigned action, YF_node target)
{
    assert(anim != NULL);

    if (action < anim->action_n) {
        anim->targets[action] = target;
        return 0;
    }
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
}

const YF_kfinput *yf_animation_getins(YF_animation anim, unsigned *n)
{
    assert(anim != NULL);
    assert(n != NULL);

    *n = anim->input_n;
    return anim->inputs;
}

const YF_kfoutput *yf_animation_getouts(YF_animation anim, unsigned *n)
{
    assert(anim != NULL);
    assert(n != NULL);

    *n = anim->output_n;
    return anim->outputs;
}

const YF_kfaction *yf_animation_getacts(YF_animation anim, unsigned *n)
{
    assert(anim != NULL);
    assert(n != NULL);

    *n = anim->action_n;
    return anim->actions;
}

float yf_animation_apply(YF_animation anim, float frame_tm)
{
    assert(anim != NULL);

    /* TODO: Compute this once. */
    float tm_min = 0.0f;
    float tm_max = 0.0f;
    for (unsigned i = 0; i < anim->input_n; i++) {
        tm_min = YF_MIN(tm_min, anim->inputs[i].timeline[0]);
        tm_max = YF_MAX(tm_max,
                        anim->inputs[i].timeline[anim->inputs[i].n - 1]);
    }
    assert(tm_min <= tm_max);
    const float dur = tm_max - tm_min;

    for (unsigned i = 0; i < anim->action_n; i++) {
        YF_node node = anim->targets[i];
        if (node == NULL)
            continue;

        const YF_kfaction *act = &anim->actions[i];
        const YF_kfinput *in = &anim->inputs[act->in_i];
        const YF_kfoutput *out = &anim->outputs[act->out_i];

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

    /* TODO */

    return dur - frame_tm;
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
    free(anim->targets);
    free(anim);
}
