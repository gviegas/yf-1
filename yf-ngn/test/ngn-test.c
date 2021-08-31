/*
 * YF
 * ngn-test.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include "test.h"

int yf_test_node(void);
int yf_test_view(void);
int yf_test_vector(void);
int yf_test_matrix(void);
int yf_test_mesh(void);
int yf_test_texture(void);
int yf_test_model(void);
int yf_test_terrain(void);
int yf_test_particle(void);
int yf_test_quad(void);
int yf_test_label(void);
int yf_test_scene(void);
int yf_test_camera(void);
int yf_test_animation(void);
int yf_test_collection(void);
int yf_test_rendering(void);
int yf_test_composition(void);

static const char *ids_[] = {
    "node",
    "view",
    "vector",
    "matrix",
    "mesh",
    "texture",
    "model",
    "terrain",
    "particle",
    "quad",
    "label",
    "scene",
    "camera",
    "animation",
    "collection",
    "rendering",
    "composition"
};

static int (*fns_[])(void) = {
    yf_test_node,
    yf_test_view,
    yf_test_vector,
    yf_test_matrix,
    yf_test_mesh,
    yf_test_texture,
    yf_test_model,
    yf_test_terrain,
    yf_test_particle,
    yf_test_quad,
    yf_test_label,
    yf_test_scene,
    yf_test_camera,
    yf_test_animation,
    yf_test_collection,
    yf_test_rendering,
    yf_test_composition,
};

_Static_assert(sizeof ids_ / sizeof *ids_ == sizeof fns_ / sizeof *fns_,
               "!sizeof");

const YF_test yf_g_test = {
    .name = "ngn",
    .ids = ids_,
    .fns = fns_,
    .n = sizeof ids_ / sizeof *ids_
};
