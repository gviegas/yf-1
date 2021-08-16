/*
 * YF
 * ngn-test.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include "test.h"

int yf_test_node(void);
int yf_test_vector(void);
int yf_test_matrix(void);
int yf_test_model(void);
int yf_test_terrain(void);
int yf_test_particle(void);
int yf_test_quad(void);
int yf_test_label(void);
int yf_test_scene(void);
int yf_test_animation(void);
int yf_test_rendering(void);
int yf_test_composition(void);

static const char *l_ids[] = {
    "node",
    "vector",
    "matrix",
    "model",
    "terrain",
    "particle",
    "quad",
    "label",
    "scene",
    "animation",
    "rendering",
    "composition"
};

static int (*l_fns[])(void) = {
    yf_test_node,
    yf_test_vector,
    yf_test_matrix,
    yf_test_model,
    yf_test_terrain,
    yf_test_particle,
    yf_test_quad,
    yf_test_label,
    yf_test_scene,
    yf_test_animation,
    yf_test_rendering,
    yf_test_composition,
};

_Static_assert(sizeof l_ids / sizeof *l_ids == sizeof l_fns / sizeof *l_fns,
               "!sizeof");

const YF_test yf_g_test = {
    .name = "ngn",
    .ids = l_ids,
    .fns = l_fns,
    .n = sizeof l_ids / sizeof *l_ids
};
