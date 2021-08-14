/*
 * YF
 * ngn-test.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "test.h"

#define YF_TEST_DEF(id) \
int yf_test_##id(void); \
static int test_##id(void) { puts("\n["#id"]\n"); return yf_test_##id(); }

#define YF_TEST_ALL "all"

/* Node test. */
#define YF_TEST_NODE "node"
YF_TEST_DEF(node)

/* Vector test. */
#define YF_TEST_VECTOR "vector"
YF_TEST_DEF(vector)

/* Matrix test. */
#define YF_TEST_MATRIX "matrix"
YF_TEST_DEF(matrix)

/* Model test. */
#define YF_TEST_MODEL "model"
YF_TEST_DEF(model)

/* Terrain test. */
#define YF_TEST_TERRAIN "terrain"
YF_TEST_DEF(terrain)

/* Particle test. */
#define YF_TEST_PARTICLE "particle"
YF_TEST_DEF(particle)

/* Quad test. */
#define YF_TEST_QUAD "quad"
YF_TEST_DEF(quad)

/* Label test. */
#define YF_TEST_LABEL "label"
YF_TEST_DEF(label)

/* Scene test. */
#define YF_TEST_SCENE "scene"
YF_TEST_DEF(scene)

/* Animation test. */
#define YF_TEST_ANIMATION "animation"
YF_TEST_DEF(animation)

/* Rendering test. */
#define YF_TEST_RENDERING "rendering"
YF_TEST_DEF(rendering)

/* Composition test. */
#define YF_TEST_COMPOSITION "composition"
YF_TEST_DEF(composition)

static const char *l_ids[] = {
    YF_TEST_NODE,
    YF_TEST_VECTOR,
    YF_TEST_MATRIX,
    YF_TEST_MODEL,
    YF_TEST_TERRAIN,
    YF_TEST_PARTICLE,
    YF_TEST_QUAD,
    YF_TEST_LABEL,
    YF_TEST_SCENE,
    YF_TEST_ANIMATION,
    YF_TEST_RENDERING,
    YF_TEST_COMPOSITION,
    YF_TEST_ALL
};

/* Test function. */
static int test(int argc, char *argv[])
{
    assert(argc > 0);

    size_t test_n;
    size_t results;

    if (strcmp(argv[0], YF_TEST_NODE) == 0) {
        test_n = 1;
        results = test_node() == 0;
    } else if (strcmp(argv[0], YF_TEST_VECTOR) == 0) {
        test_n = 1;
        results = test_vector() == 0;
    } else if (strcmp(argv[0], YF_TEST_MATRIX) == 0) {
        test_n = 1;
        results = test_matrix() == 0;
    } else if (strcmp(argv[0], YF_TEST_MODEL) == 0) {
        test_n = 1;
        results = test_model() == 0;
    } else if (strcmp(argv[0], YF_TEST_TERRAIN) == 0) {
        test_n = 1;
        results = test_terrain() == 0;
    } else if (strcmp(argv[0], YF_TEST_PARTICLE) == 0) {
        test_n = 1;
        results = test_particle() == 0;
    } else if (strcmp(argv[0], YF_TEST_QUAD) == 0) {
        test_n = 1;
        results = test_quad() == 0;
    } else if (strcmp(argv[0], YF_TEST_LABEL) == 0) {
        test_n = 1;
        results = test_label() == 0;
    } else if (strcmp(argv[0], YF_TEST_SCENE) == 0) {
        test_n = 1;
        results = test_scene() == 0;
    } else if (strcmp(argv[0], YF_TEST_ANIMATION) == 0) {
        test_n = 1;
        results = test_animation() == 0;
    } else if (strcmp(argv[0], YF_TEST_RENDERING) == 0) {
        test_n = 1;
        results = test_rendering() == 0;
    } else if (strcmp(argv[0], YF_TEST_COMPOSITION) == 0) {
        test_n = 1;
        results = test_composition() == 0;
    } else if (strcmp(argv[0], YF_TEST_ALL) == 0) {
        int (*const tests[])(void) = {
            test_node,
            test_vector,
            test_matrix,
            test_model,
            test_terrain,
            test_particle,
            test_quad,
            test_label,
            test_scene,
            test_animation,
            test_rendering,
            test_composition
        };
        test_n = sizeof tests / sizeof tests[0];
        results = 0;
        for (size_t i = 0; i < test_n; i++)
            results += tests[i]() == 0;
    } else {
        printf("! Error: unknown TEST_ID '%s'\n", argv[0]);
        printf("\nPossible values for TEST_ID:\n");
        for (size_t i = 0; i < (sizeof l_ids / sizeof l_ids[0]); i++)
            printf("* %s\n", l_ids[i]);
        printf("\n! No tests executed\n");
        return -1;
    }

    printf("\nDONE!\n\nNumber of tests executed: %zu\n", test_n);
    printf("> #%zu passed\n", results);
    printf("> #%zu failed\n", test_n - results);
    printf("\n(%.0f%% coverage)\n",(double)results / (double)test_n * 100.0);

    return 0;
}

const YF_test yf_g_test = {"ngn", test, l_ids, sizeof l_ids / sizeof l_ids[0]};
