/*
 * YF
 * test-camera.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "test.h"
#include "print.h"
#include "yf-camera.h"

/* Tests camera. */
int yf_test_camera(void)
{
    yf_vec3_t orig = {2.0f, 10.0f, -5.0f};
    yf_vec3_t tgt = {0};
    float asp = 1024.0f / 768.0f;

    char s[256] = {0};

    snprintf(s, sizeof s, "[%.1f, %.1f, %.1f], [%.1f, %.1f, %.1f], %.3f",
             orig[0], orig[1], orig[2], tgt[0], tgt[1], tgt[2], asp);
    YF_TEST_PRINT("init", s, "cam");
    yf_camera_t *cam = yf_camera_init(orig, tgt, asp);
    if (cam == NULL)
        return -1;

    yf_print_cam(cam);

    orig[0] = orig[1] = 0.0f;
    snprintf(s, sizeof s, "cam, [%.1f, %.1f, %.1f]", orig[0], orig[1], orig[2]);
    YF_TEST_PRINT("place", s, "");
    yf_camera_place(cam, orig);

    yf_print_cam(cam);

    tgt[0] = 5.0f;
    snprintf(s, sizeof s, "cam, [%.1f, %.1f, %.1f]", tgt[0], tgt[1], tgt[2]);
    YF_TEST_PRINT("point", s, "");
    yf_camera_point(cam, tgt);

    yf_print_cam(cam);

    YF_TEST_PRINT("movef", "cam, 10.0", "");
    yf_camera_movef(cam, 10.0f);

    yf_print_cam(cam);

    YF_TEST_PRINT("turnr", "cam, 3.141593 * 0.25", "");
    yf_camera_turnr(cam, 3.141593f * 0.25f);

    yf_print_cam(cam);

    YF_TEST_PRINT("zoomi", "cam, 0.1", "");
    yf_camera_zoomi(cam, 0.1f);

    yf_print_cam(cam);

    YF_TEST_PRINT("adjust", "cam, 1920 / 1080", "");
    yf_camera_adjust(cam, 1920.0f / 1080.0f);

    yf_print_cam(cam);

    YF_TEST_PRINT("getview", "cam", "");
    if (yf_camera_getview(cam) == NULL)
        return -1;

    YF_TEST_PRINT("getproj", "cam", "");
    if (yf_camera_getproj(cam) == NULL)
        return -1;

    YF_TEST_PRINT("getxform", "cam", "");
    if (yf_camera_getxform(cam) == NULL)
        return -1;

    YF_TEST_PRINT("deinit", "cam", "");
    yf_camera_deinit(cam);

    return 0;
}
