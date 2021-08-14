/*
 * YF
 * test-window.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "yf/com/yf-clock.h"

#include "yf-window.h"

/* Tests window. */
int yf_test_window(void)
{
    puts("(init 400,240,\"A\",0)\n -> wins[0]\n");
    puts("(init 240,400,\"B\",0)\n -> wins[1]\n");
    puts("(init 480,480,\"C\",YF_WINCREAT_HIDDEN)\n -> wins[2]\n");
    YF_window wins[] = {
        yf_window_init(400, 240, "A", 0),
        yf_window_init(240, 400, "B", 0),
        yf_window_init(480, 480, "C", YF_WINCREAT_HIDDEN)
    };
    const size_t n = sizeof wins / sizeof wins[0];

    for (size_t i = 0; i < n; i++) {
        if (wins[i] == NULL)
            return -1;
    }

    yf_sleep(1.0);
    puts("(open wins[2])\n");
    yf_window_open(wins[2]);
    yf_sleep(1.0);
    puts("(resize wins[0],640,360)\n");
    yf_window_resize(wins[0], 640, 360);
    yf_sleep(1.0);
    puts("(settitle wins[1],\"WIN2\")\n");
    yf_window_settitle(wins[1], "WIN2");
    yf_sleep(1.0);
    puts("(close wins[1])\n");
    yf_window_close(wins[1]);
    puts("(close wins[2])\n");
    yf_window_close(wins[2]);
    yf_sleep(1.0);
    puts("(settitle wins[2],\"WIN3\")\n");
    yf_window_settitle(wins[2], "WIN3");
    puts("(resize wins[2],800,600)\n");
    yf_window_resize(wins[2], 800, 600);
    puts("(open wins[2])\n");
    yf_window_open(wins[2]);
    yf_sleep(1.0);

    unsigned w, h;

    puts("(getsize wins[0])\n");
    yf_window_getsize(wins[0], &w, &h);
    if (w != 640 || h != 360)
        return -1;
    puts("(getsize wins[1])\n");
    yf_window_getsize(wins[1], &w, &h);
    if (w != 240 || h != 400)
        return -1;
    puts("(getsize wins[2])\n");
    yf_window_getsize(wins[2], &w, &h);
    if (w != 800 || h != 600)
        return -1;

    puts("(deinit wins[0])\n");
    puts("(deinit wins[1])\n");
    puts("(deinit wins[2])");
    for (size_t i = 0; i < n; i++)
        yf_window_deinit(wins[i]);

    return 0;
}
