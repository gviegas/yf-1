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

#define YF_TEST_ALL "all"
#define YF_TEST_SUBL "................................"
#define YF_TEST_SUBT \
    printf("%s\n%.*s\n", __func__, (int)strlen(__func__), YF_TEST_SUBL)

/* Node test. */
#define YF_TEST_NODE "node"

int yf_test_node(void);

static int test_node(void)
{
    YF_TEST_SUBT;
    puts("");
    int r = yf_test_node();
    puts("");
    return r;
}

/* Vector/Matrix test. */
#define YF_TEST_VECMAT "vecmat"

int yf_test_vecmat(void);

static int test_vecmat(void)
{
    YF_TEST_SUBT;
    puts("");
    int r = yf_test_vecmat();
    puts("");
    return r;
}

/* Model test. */
#define YF_TEST_MODEL "model"

int yf_test_model(void);

static int test_model(void)
{
    YF_TEST_SUBT;
    puts("");
    int r = yf_test_model();
    puts("");
    return r;
}

/* Terrain test. */
#define YF_TEST_TERRAIN "terrain"

int yf_test_terrain(void);

static int test_terrain(void)
{
    YF_TEST_SUBT;
    puts("");
    int r = yf_test_terrain();
    puts("");
    return r;
}

/* Particle test. */
#define YF_TEST_PARTICLE "particle"

int yf_test_particle(void);

static int test_particle(void)
{
    YF_TEST_SUBT;
    puts("");
    int r = yf_test_particle();
    puts("");
    return r;
}

/* Quad test. */
#define YF_TEST_QUAD "quad"

int yf_test_quad(void);

static int test_quad(void)
{
    YF_TEST_SUBT;
    puts("");
    int r = yf_test_quad();
    puts("");
    return r;
}

/* Label test. */
#define YF_TEST_LABEL "label"

int yf_test_label(void);

static int test_label(void)
{
    YF_TEST_SUBT;
    puts("");
    int r = yf_test_label();
    puts("");
    return r;
}

/* Scene test. */
#define YF_TEST_SCENE "scene"

int yf_test_scene(void);

static int test_scene(void)
{
    YF_TEST_SUBT;
    puts("");
    int r = yf_test_scene();
    puts("");
    return r;
}

/* Animation test. */
#define YF_TEST_ANIMATION "animation"

int yf_test_animation(void);

static int test_animation(void)
{
    YF_TEST_SUBT;
    puts("");
    int r = yf_test_animation();
    puts("");
    return r;
}

/* Render test. */
#define YF_TEST_RENDER "render"

int yf_test_render(void);

static int test_render(void)
{
    YF_TEST_SUBT;
    puts("");
    int r = yf_test_render();
    puts("");
    return r;
}

/* Miscellaneous test. */
#define YF_TEST_MISC "misc"

int yf_test_misc(void);

static int test_misc(void)
{
    YF_TEST_SUBT;
    puts("");
    int r = yf_test_misc();
    puts("");
    return r;
}

static const char *l_ids[] = {
    YF_TEST_NODE,
    YF_TEST_VECMAT,
    YF_TEST_MODEL,
    YF_TEST_TERRAIN,
    YF_TEST_PARTICLE,
    YF_TEST_QUAD,
    YF_TEST_LABEL,
    YF_TEST_SCENE,
    YF_TEST_ANIMATION,
    YF_TEST_RENDER,
    YF_TEST_MISC,
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
    } else if (strcmp(argv[0], YF_TEST_VECMAT) == 0) {
        test_n = 1;
        results = test_vecmat() == 0;
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
    } else if (strcmp(argv[0], YF_TEST_RENDER) == 0) {
        test_n = 1;
        results = test_render() == 0;
    } else if (strcmp(argv[0], YF_TEST_MISC) == 0) {
        test_n = 1;
        results = test_misc() == 0;
    } else if (strcmp(argv[0], YF_TEST_ALL) == 0) {
        int (*const tests[])(void) = {
            test_node,
            test_vecmat,
            test_model,
            test_terrain,
            test_particle,
            test_quad,
            test_label,
            test_scene,
            test_animation,
            test_render,
            test_misc
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
