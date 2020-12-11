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

typedef struct YF_wsi_o {
  YF_context ctx;
  YF_window win;

  VkSurfaceKHR surface;
  VkSwapchainKHR swapchain;
  VkSwapchainCreateInfoKHR sc_info;
  unsigned min_img_n;
  unsigned acq_limit;

  YF_image *imgs;
  int *imgs_acq;
  VkSemaphore *imgs_sem;
  unsigned img_n;
} YF_wsi_o;

/* Checks whether a given physical device supports presentation. */
int yf_canpresent(VkPhysicalDevice phy_dev, int queue_i);

#endif /* YF_WSI_H */
