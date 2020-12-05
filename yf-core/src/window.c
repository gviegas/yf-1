/*
 * YF
 * window.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "window.h"
#include "context.h"
#include "image.h"
#include "error.h"

#ifndef YF_MIN
# define YF_MIN(a, b) (a < b ? a : b)
#endif
#undef YF_STR_MAXLEN
#define YF_STR_MAXLEN 60
#undef YF_NBUFFERED
#define YF_NBUFFERED 3

/* Initializes the swapchain object. */
static int init_swapchain(YF_window win);

/* Initializes the images/attachments. */
static int init_atts(YF_window win);

YF_window yf_window_init(YF_context ctx, YF_dim2 dim, const char *title) {
  assert(ctx != NULL);
  YF_window win = calloc(1, sizeof(YF_window_o));
  if (win == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  win->ctx = ctx;
  win->dim = dim;
  if (title != NULL) {
    size_t len = 1 + strnlen(title, YF_STR_MAXLEN-1);
    if ((win->title = malloc(len)) == NULL) {
      free(win);
      yf_seterr(YF_ERR_NOMEM, __func__);
      return NULL;
    }
    strncpy(win->title, title, len);
    win->title[len-1] = '\0';
  }
  if ((win->wsi = yf_wsi_init(win)) == NULL) {
    yf_window_deinit(win);
    return NULL;
  }
  if (init_swapchain(win) != 0) {
    yf_window_deinit(win);
    return NULL;
  }
  if (init_atts(win) != 0) {
    yf_window_deinit(win);
    return NULL;
  }
  return win;
}

YF_dim2 yf_window_getdim(YF_window win) {
  assert(win != NULL);
  return win->dim;
}

const YF_colordsc *yf_window_getdsc(YF_window win) {
  assert(win != NULL);
  return &win->colordsc;
}

const YF_attach *yf_window_getatts(YF_window win, unsigned *n) {
  assert(win != NULL);
  assert(n != NULL);
  *n = win->att_n;
  return win->atts;
}

int yf_window_next(YF_window win) {
  assert(win != NULL);
  if (win->att_i < 0) {
    unsigned idx;
    VkResult res = vkAcquireNextImageKHR(
      win->ctx->device,
      win->swapchain,
      UINT64_MAX,
      win->semaphore,
      NULL,
      &idx);
    switch (res) {
      case VK_SUCCESS:
        win->att_i = idx;
        break;
      case VK_SUBOPTIMAL_KHR:
      case VK_ERROR_OUT_OF_DATE_KHR:
        yf_seterr(YF_ERR_INVWIN, __func__);
        break;
      default:
        yf_seterr(YF_ERR_DEVGEN, __func__);
    }
  }
  return win->att_i;
}

int yf_window_present(YF_window win) {
  assert(win != NULL);
  if (win->att_i < 0) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
  }
  VkPresentInfoKHR info = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .pNext = NULL,
    .waitSemaphoreCount = 0,
    .swapchainCount = 1,
    .pSwapchains = &win->swapchain,
    .pImageIndices = (const unsigned *)&win->att_i,
    .pResults = NULL
  };
  VkResult res = vkQueuePresentKHR(win->pres_queue, &info);
  win->att_i = -1;
  switch (res) {
    case VK_SUCCESS:
      return 0;
    case VK_SUBOPTIMAL_KHR:
    case VK_ERROR_OUT_OF_DATE_KHR:
      yf_seterr(YF_ERR_INVWIN, __func__);
      break;
    default:
      yf_seterr(YF_ERR_DEVGEN, __func__);
  }
  return -1;
}

int yf_window_resize(YF_window win, YF_dim2 dim) {
  assert(win != NULL);
  if (dim.width == win->dim.width && dim.height == win->dim.height)
    return 0;
  win->dim = dim;
  return yf_wsi_resize(win->wsi);
}

int yf_window_close(YF_window win) {
  assert(win != NULL);
  return yf_wsi_close(win->wsi);
}

int yf_window_recreate(YF_window win) {
  assert(win != NULL);
  vkDestroySemaphore(win->ctx->device, win->semaphore, NULL);
  win->semaphore = NULL;
  for (unsigned i = 0; i < win->att_n; ++i)
    yf_image_deinit(win->atts[i].img);
  free(win->atts);
  win->atts = NULL;
  /* TODO: Consider informing the wsi that window recreation is taking place. */
  if (init_swapchain(win) != 0 || init_atts(win) != 0)
    return -1;
  return 0;
}

void yf_window_deinit(YF_window win) {
  if (win != NULL) {
    vkDestroySemaphore(win->ctx->device, win->semaphore, NULL);
    for (unsigned i = 0; i < win->att_n; ++i)
      yf_image_deinit(win->atts[i].img);
    free(win->atts);
    vkDestroySwapchainKHR(win->ctx->device, win->swapchain, NULL);
    vkDestroySurfaceKHR(win->ctx->instance, win->surface, NULL);
    yf_wsi_deinit(win->wsi);
    free(win->title);
    free(win);
  }
}

static int init_swapchain(YF_window win) {
  win->pres_queue_i = -1;
  VkResult res;

  VkSharingMode sharing;
  unsigned indices[2];
  unsigned index_n;
  VkBool32 supported;
  res = vkGetPhysicalDeviceSurfaceSupportKHR(
    win->ctx->phy_dev,
    win->ctx->graph_queue_i,
    win->surface,
    &supported);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  if (supported) {
    win->pres_queue = win->ctx->graph_queue;
    win->pres_queue_i = win->ctx->graph_queue_i;
    sharing = VK_SHARING_MODE_EXCLUSIVE;
    index_n = 0;
  } else {
    unsigned n;
    vkGetPhysicalDeviceQueueFamilyProperties(win->ctx->phy_dev, &n, NULL);
    for (unsigned i = 0; i < n; ++i) {
      if ((int)i == win->ctx->graph_queue_i)
        continue;
      res = vkGetPhysicalDeviceSurfaceSupportKHR(
        win->ctx->phy_dev,
        i,
        win->surface,
        &supported);
      if (res != VK_SUCCESS) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        return -1;
      }
      if (supported) {
        vkGetDeviceQueue(win->ctx->device, i, 0, &win->pres_queue);
        win->pres_queue_i = i;
        sharing = VK_SHARING_MODE_CONCURRENT;
        indices[0] = win->ctx->graph_queue_i;
        indices[1] = i;
        index_n = 2;
        break;
      }
    }
    if (win->pres_queue_i == -1) {
      yf_seterr(YF_ERR_DEVGEN, __func__);
      return -1;
    }
  }

  VkSurfaceCapabilitiesKHR capab;
  res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    win->ctx->phy_dev,
    win->surface,
    &capab);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  if (capab.minImageCount > 1)
    win->att_n = capab.minImageCount;
  else if (capab.maxImageCount == 0)
    win->att_n = YF_NBUFFERED;
  else
    win->att_n = YF_MIN(capab.maxImageCount, YF_NBUFFERED);
  if (capab.currentExtent.width == 0xffffffff) {
    if (capab.minImageExtent.width > win->dim.width)
      win->dim.width = capab.minImageExtent.width;
    else if (capab.maxImageExtent.width < win->dim.width)
      win->dim.width = capab.maxImageExtent.width;
    if (capab.minImageExtent.height > win->dim.height)
      win->dim.height = capab.minImageExtent.height;
    else if (capab.maxImageExtent.height < win->dim.height)
      win->dim.height = capab.maxImageExtent.height;
  }
  VkCompositeAlphaFlagBitsKHR cp_alpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
  if ((capab.supportedCompositeAlpha & cp_alpha) == 0) {
    cp_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if ((capab.supportedCompositeAlpha & cp_alpha) == 0) {
      yf_seterr(YF_ERR_UNSUP, __func__);
      return -1;
    }
  }

  VkColorSpaceKHR cl_space;
  VkSurfaceFormatKHR *fmts;
  unsigned fmt_n;
  unsigned fmt_i = 0;
  VkFormat pref_fmts[2] = {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM};
  res = vkGetPhysicalDeviceSurfaceFormatsKHR(
    win->ctx->phy_dev,
    win->surface,
    &fmt_n,
    NULL);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  fmts = malloc(fmt_n * sizeof *fmts);
  if (fmts == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  res = vkGetPhysicalDeviceSurfaceFormatsKHR(
    win->ctx->phy_dev,
    win->surface,
    &fmt_n,
    fmts);
  if (res != VK_SUCCESS) {
    free(fmts);
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  for (unsigned i = 0; i < fmt_n; ++i) {
    for (unsigned j = 0; j < sizeof pref_fmts / sizeof pref_fmts[0]; ++j) {
      if (fmts[i].format == pref_fmts[j]) {
        fmt_i = i;
        i = fmt_n;
        break;
      }
    }
  }
  win->format = fmts[fmt_i].format;
  cl_space = fmts[fmt_i].colorSpace;
  free(fmts);

  VkSwapchainKHR old_sc = win->swapchain;
  VkSwapchainCreateInfoKHR sc_info = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .pNext = NULL,
    .flags = 0,
    .surface = win->surface,
    .minImageCount = win->att_n,
    .imageFormat = win->format,
    .imageColorSpace = cl_space,
    .imageExtent = {win->dim.width, win->dim.height},
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .imageSharingMode = sharing,
    .queueFamilyIndexCount = index_n,
    .pQueueFamilyIndices = index_n == 0 ? NULL : indices,
    .preTransform = capab.currentTransform,
    .compositeAlpha = cp_alpha,
    .presentMode = VK_PRESENT_MODE_FIFO_KHR,
    .clipped = VK_TRUE,
    .oldSwapchain = old_sc
  };
  res = vkCreateSwapchainKHR(win->ctx->device, &sc_info, NULL, &win->swapchain);
  if (old_sc != NULL)
    vkDestroySwapchainKHR(win->ctx->device, old_sc, NULL);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  return 0;
}

static int init_atts(YF_window win) {
  const YF_dim3 dim = {win->dim.width, win->dim.height, 1};
  VkImage *imgs = NULL;
  unsigned n;
  VkResult res;

  res = vkGetSwapchainImagesKHR(win->ctx->device, win->swapchain, &n, NULL);
  if (res != VK_SUCCESS || n < win->att_n) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  imgs = malloc(n * sizeof *imgs);
  if (imgs == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  res = vkGetSwapchainImagesKHR(win->ctx->device, win->swapchain, &n, imgs);
  if (res != VK_SUCCESS) {
    free(imgs);
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  win->att_n = n;

  win->atts = calloc(n, sizeof *win->atts);
  if (win->atts == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  for (unsigned i = 0; i < n; ++i) {
    win->atts[i].img = yf_image_wrap(
      win->ctx,
      imgs[i],
      win->format,
      VK_IMAGE_TYPE_2D,
      dim,
      1,
      1,
      VK_SAMPLE_COUNT_1_BIT,
      VK_IMAGE_LAYOUT_UNDEFINED);
    if (win->atts[i].img == NULL) {
      free(imgs);
      return -1;
    }
    win->atts[i].layer_base = 0;
  }
  free(imgs);

  YF_PIXFMT_TO(win->format, win->colordsc.pixfmt);
  if (win->colordsc.pixfmt == YF_PIXFMT_UNDEF) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  win->colordsc.samples = 1;
  win->colordsc.loadop = YF_LOADOP_UNDEF;
  win->colordsc.storeop = YF_STOREOP_UNDEF;

  VkSemaphoreCreateInfo sem_info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0
  };
  res = vkCreateSemaphore(win->ctx->device, &sem_info, NULL, &win->semaphore);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  win->att_i = -1;
  return 0;
}
