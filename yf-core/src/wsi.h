/*
 * YF
 * wsi.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_WSI_H
#define YF_WSI_H

#include "yf-wsi.h"
#include "vk.h"

struct yf_wsi {
    yf_context_t *ctx;
    yf_window_t *win;

    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkSwapchainCreateInfoKHR sc_info;
    unsigned min_img_n;
    unsigned acq_limit;

    yf_image_t **imgs;
    int *imgs_acq;
    VkSemaphore *imgs_sem;
    unsigned img_n;
};

/* Checks whether a given physical device supports presentation. */
int yf_canpresent(VkPhysicalDevice phy_dev, int queue_i);

#endif /* YF_WSI_H */
