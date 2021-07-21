/*
 * YF
 * animation.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
    /* TODO */
    return NULL;
}

void yf_animation_deinit(YF_animation anim)
{
    /* TODO */
}
