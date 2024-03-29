/*
 * YF
 * test-light.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "yf/com/yf-error.h"

#include "test.h"
#include "print.h"
#include "yf-light.h"

#define YF_LIGHTVAL(light, type) do { \
    int tp; \
    yf_vec3_t clr; \
    float intens, rng, inner, outer; \
    YF_TEST_PRINT("getval", #light", <non-nil>", ""); \
    yf_light_getval(light, &tp, clr, &intens, &rng, &inner, &outer); \
    if (tp != (type) || !yf_vec3_iseq(clr, color) || intens != intensity) \
        return -1; \
    if (tp != YF_LIGHTT_DIRECT && rng != range) \
        return -1; \
    if (tp == YF_LIGHTT_SPOT && (inner != inner_angle || \
                                 outer != outer_angle)) \
        return -1; \
    tp = ~type; \
    YF_TEST_PRINT("getval", #light", <nil>", ""); \
    yf_light_getval(light, &tp, NULL, NULL, NULL, NULL, NULL); \
    if (tp != (type)) \
        return -1; } while (0)

/* Tests light. */
int yf_test_light(void)
{
    yf_node_t *node = NULL;
    void *obj = NULL;
    char s[256] = {0};

    yf_vec3_t color = {1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    float range = 1.0f;
    float inner_angle = 0.0f;
    float outer_angle = 0.0f;

    snprintf(s, sizeof s,
             "LIGHTT_POINT, [%.2f, %.2f, %.2f], %.2f, %.2f, %.4f, %.4f",
             color[0], color[1], color[2], intensity,
             range, inner_angle, outer_angle);

    YF_TEST_PRINT("init", s, "light");
    yf_light_t *light = yf_light_init(YF_LIGHTT_POINT, color, intensity,
                                      range, inner_angle, outer_angle);
    if (light == NULL)
        return -1;

    YF_TEST_PRINT("getnode", "light", "");
    node = yf_light_getnode(light);
    if (node == NULL || yf_node_getobj(node, &obj) != YF_NODEOBJ_LIGHT ||
        obj != light)
        return -1;

    YF_LIGHTVAL(light, YF_LIGHTT_POINT);

    yf_print_nodeobj(node);

    intensity = -0.1f;

    snprintf(s, sizeof s,
             "LIGHTT_POINT, [%.2f, %.2f, %.2f], %.2f, %.2f, %.4f, %.4f",
             color[0], color[1], color[2], intensity,
             range, inner_angle, outer_angle);

    YF_TEST_PRINT("init", s, "(nil)");
    yf_seterr(YF_ERR_UNKNOWN, NULL);
    if (yf_light_init(YF_LIGHTT_POINT, color, intensity, range, inner_angle,
                      outer_angle) != NULL || yf_geterr() != YF_ERR_INVARG)
        return -1;
    yf_printerr();

    intensity = 10.0f;
    outer_angle = -1.57f;

    snprintf(s, sizeof s,
             "LIGHTT_SPOT, [%.2f, %.2f, %.2f], %.2f, %.2f, %.4f, %.4f",
             color[0], color[1], color[2], intensity,
             range, inner_angle, outer_angle);

    YF_TEST_PRINT("init", s, "(nil)");
    yf_seterr(YF_ERR_UNKNOWN, NULL);
    if (yf_light_init(YF_LIGHTT_SPOT, color, intensity, range, inner_angle,
                      outer_angle) != NULL || yf_geterr() != YF_ERR_INVARG)
        return -1;
    yf_printerr();

    outer_angle = 0.79f;

    snprintf(s, sizeof s,
             "LIGHTT_SPOT, [%.2f, %.2f, %.2f], %.2f, %.2f, %.4f, %.4f",
             color[0], color[1], color[2], intensity,
             range, inner_angle, outer_angle);

    YF_TEST_PRINT("init", s, "light2");
    yf_light_t *light2 = yf_light_init(YF_LIGHTT_SPOT, color, intensity,
                                       range, inner_angle, outer_angle);
    if (light2 == NULL)
        return -1;

    YF_TEST_PRINT("getnode", "light2", "");
    node = yf_light_getnode(light2);
    if (node == NULL || yf_node_getobj(node, &obj) != YF_NODEOBJ_LIGHT ||
        obj != light2)
        return -1;

    YF_LIGHTVAL(light2, YF_LIGHTT_SPOT);

    yf_print_nodeobj(node);

    color[0] = color[1] = 0.2f;
    range = 0.0f;
    inner_angle = -0.79f;

    snprintf(s, sizeof s,
             "LIGHTT_DIRECT, [%.2f, %.2f, %.2f], %.2f, %.2f, %.4f, %.4f",
             color[0], color[1], color[2], intensity,
             range, inner_angle, outer_angle);

    YF_TEST_PRINT("init", s, "light3");
    yf_light_t *light3 = yf_light_init(YF_LIGHTT_DIRECT, color, intensity,
                                       range, inner_angle, outer_angle);
    if (light3 == NULL)
        return -1;

    YF_TEST_PRINT("getnode", "light3", "");
    node = yf_light_getnode(light3);
    if (node == NULL || yf_node_getobj(node, &obj) != YF_NODEOBJ_LIGHT ||
        obj != light3)
        return -1;

    YF_LIGHTVAL(light3, YF_LIGHTT_DIRECT);

    yf_print_nodeobj(node);

    color[0] = 0.0f;
    color[1] = color[2] = 0.9f;
    intensity = 1000.0f;
    range = 3.0f;

    snprintf(s, sizeof s, "light2, [%.2f, %.2f, %.2f], %.2f, %.2f",
             color[0], color[1], color[2], intensity, range);

    YF_TEST_PRINT("setpoint", s, "");
    yf_light_setpoint(light2, color, intensity, range);

    YF_LIGHTVAL(light2, YF_LIGHTT_POINT);

    yf_print_nodeobj(yf_light_getnode(light2));

    color[0] = 0.6f;
    color[2] = 0.1f;
    intensity = 500.0f;
    range = 6.0f;
    inner_angle = 0.39f;
    outer_angle = 1.05f;

    snprintf(s, sizeof s, "light3, [%.2f, %.2f, %.2f], %.2f, %.2f, %.4f, %.4f",
             color[0], color[1], color[2], intensity,
             range, inner_angle, outer_angle);

    YF_TEST_PRINT("setspot", s, "");
    yf_light_setspot(light3, color, intensity, range, inner_angle, outer_angle);

    YF_LIGHTVAL(light3, YF_LIGHTT_SPOT);

    yf_print_nodeobj(yf_light_getnode(light3));

    color[0] = 1.0f;
    color[1] = 0.5f;
    color[2] = 0.2f;
    intensity = 4.5f;

    snprintf(s, sizeof s, "light, [%.2f, %.2f, %.2f], %.2f",
             color[0], color[1], color[2], intensity);

    YF_TEST_PRINT("setdirect", s, "");
    yf_light_setdirect(light, color, intensity);

    YF_LIGHTVAL(light, YF_LIGHTT_DIRECT);

    yf_print_nodeobj(yf_light_getnode(light));

    YF_TEST_PRINT("deinit", "light", "");
    yf_light_deinit(light);

    YF_TEST_PRINT("deinit", "light2", "");
    yf_light_deinit(light2);

    YF_TEST_PRINT("deinit", "light3", "");
    yf_light_deinit(light3);

    return 0;
}
