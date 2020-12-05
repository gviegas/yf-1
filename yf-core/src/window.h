/*
 * YF
 * window.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_WINDOW_H
#define YF_WINDOW_H

#include "yf-window.h"
#include "wsi.h"
#include "vk.h"

typedef struct YF_window_o {
  YF_context ctx;
  YF_dim2 dim;
  char *title;
  YF_colordsc colordsc;
  YF_attach *atts;
  unsigned att_n;
  int att_i;
  YF_wsi wsi;

  VkQueue pres_queue;
  int pres_queue_i;
  VkSurfaceKHR surface;
  VkFormat format;
  VkSwapchainKHR swapchain;
  VkSemaphore semaphore;
} YF_window_o;

#endif /* YF_WINDOW_H */
