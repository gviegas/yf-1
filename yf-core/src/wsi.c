/*
 * YF
 * wsi.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include "yf/com/yf-util.h"
#include "yf/com/yf-error.h"
#include "yf/wsys/yf-platform.h"

#include "wsi.h"
#include "context.h"
#include "image.h"
#include "cmdpool.h"
#include "cmdexec.h"
#include "cmdbuf.h"

/* Initializes surface. */
static int init_surface(YF_wsi wsi);

/* Queries surface. */
static int query_surface(YF_wsi wsi);

/* Creates swapchain. */
static int create_swapchain(YF_wsi wsi);

YF_wsi yf_wsi_init(YF_context ctx, YF_window win)
{
  assert(ctx != NULL);
  assert(win != NULL);

  if (ctx->pres_queue_i < 0) {
    yf_seterr(YF_ERR_UNSUP, __func__);
    return NULL;
  }

  YF_wsi wsi = calloc(1, sizeof(YF_wsi_o));
  if (wsi == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }

  wsi->ctx = ctx;
  wsi->win = win;

  if (init_surface(wsi) != 0 ||
      query_surface(wsi) != 0 ||
      create_swapchain(wsi) != 0)
  {
    yf_wsi_deinit(wsi);
    return NULL;
  }

  return wsi;
}

const YF_image *yf_wsi_getimages(YF_wsi wsi, unsigned *n)
{
  assert(wsi != NULL);
  assert(n != NULL);

  *n = wsi->img_n;
  return wsi->imgs;
}

unsigned yf_wsi_getlimit(YF_wsi wsi)
{
  assert(wsi != NULL);
  return wsi->acq_limit;
}

int yf_wsi_next(YF_wsi wsi, int nonblocking)
{
  assert(wsi != NULL);

  const uint64_t timeout = nonblocking ? 0 : UINT64_MAX;
/*
  VkSemaphore sem;
  unsigned sem_i = 0;
  for (;; ++sem_i) {
    if (!wsi->imgs_acq[sem_i]) {
      sem = wsi->imgs_sem[sem_i];
      break;
    }
  }
  unsigned img_i;
  VkResult res = vkAcquireNextImageKHR(wsi->ctx->device, wsi->swapchain,
      timeout, sem, VK_NULL_HANDLE, &img_i);
*/
  VkFence fence;
  unsigned fence_i = 0;
  for (;; ++fence_i) {
    if (!wsi->imgs_acq[fence_i]) {
      fence = wsi->imgs_fence[fence_i];
      break;
    }
  }
  unsigned img_i;
  VkResult res = vkAcquireNextImageKHR(wsi->ctx->device, wsi->swapchain,
      timeout, VK_NULL_HANDLE, fence, &img_i);

  switch (res) {
  case VK_SUCCESS:
/*
    if (sem_i != img_i) {
      VkSemaphore tmp = wsi->imgs_sem[sem_i];
      wsi->imgs_sem[sem_i] = wsi->imgs_sem[img_i];
      wsi->imgs_sem[img_i] = tmp;
    }
*/
    if (fence_i != img_i) {
      VkFence tmp = wsi->imgs_fence[fence_i];
      wsi->imgs_fence[fence_i] = wsi->imgs_fence[img_i];
      wsi->imgs_fence[img_i] = tmp;
    }
    yf_cmdexec_waitfor(wsi->ctx, wsi->imgs_fence[img_i]);
    wsi->imgs_acq[img_i] = 1;
    break;

  case VK_TIMEOUT:
  case VK_NOT_READY:
    yf_seterr(YF_ERR_INUSE, __func__);
    return -1;

  case VK_SUBOPTIMAL_KHR:
  case VK_ERROR_OUT_OF_DATE_KHR:
    /* TODO: Notify & recreate swapchain. */
    yf_seterr(YF_ERR_INVWIN, __func__);
    return -1;

  case VK_ERROR_SURFACE_LOST_KHR:
    /* TODO: Notify & (try to) recreate surface and swapchain. */
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;

  default:
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }

  return img_i;
}

int yf_wsi_present(YF_wsi wsi, unsigned index)
{
  assert(wsi != NULL);

  if (index >= wsi->img_n || !wsi->imgs_acq[index]) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
  }

  const YF_cmdres *cmdr;
  cmdr = yf_cmdpool_getprio(wsi->ctx, YF_CMDBUF_GRAPH, NULL, NULL);
  if (cmdr == NULL)
    /* TODO: May need to release the image somehow. */
    return -1;

  VkImageMemoryBarrier barrier = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
    .pNext = NULL,
    .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
    .dstAccessMask = 0,
    /* TODO: Make sure that the image is keeping the layout up to date. */
    .oldLayout = wsi->imgs[index]->layout,
    .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = wsi->imgs[index]->image,
    .subresourceRange = {
      .aspectMask = wsi->imgs[index]->aspect,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1
    }
  };

  vkCmdPipelineBarrier(cmdr->pool_res, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

  VkPresentInfoKHR info = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .pNext = NULL,
    .waitSemaphoreCount = 0,
    .pWaitSemaphores = NULL,
    .swapchainCount = 1,
    .pSwapchains = &wsi->swapchain,
    .pImageIndices = &index,
    .pResults = NULL
  };

  int exec = yf_cmdexec_execprio(wsi->ctx);
  VkResult res = vkQueuePresentKHR(wsi->ctx->pres_queue, &info);

  wsi->imgs[index]->layout = barrier.newLayout;
  wsi->imgs_acq[index] = 0;

  if (exec != 0)
    return -1;

  switch (res) {
  case VK_SUCCESS:
    break;

  case VK_SUBOPTIMAL_KHR:
  case VK_ERROR_OUT_OF_DATE_KHR:
    /* TODO: Notify & recreate swapchain. */
    yf_seterr(YF_ERR_INVWIN, __func__);
    return -1;

  case VK_ERROR_SURFACE_LOST_KHR:
    /* TODO: Notify & (try to) recreate surface and swapchain. */
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;

  default:
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }

  return 0;
}

void yf_wsi_deinit(YF_wsi wsi)
{
  if (wsi != NULL) {
    /* TODO: If any image was acquired: submit, present & wait completion. */
    vkDestroySwapchainKHR(wsi->ctx->device, wsi->swapchain, NULL);
    vkDestroySurfaceKHR(wsi->ctx->instance, wsi->surface, NULL);
    for (size_t i = 0; i < wsi->img_n; ++i) {
      yf_image_deinit(wsi->imgs[i]);
      /*vkDestroySemaphore(wsi->ctx->device, wsi->imgs_sem[i], NULL);*/
      vkDestroyFence(wsi->ctx->device, wsi->imgs_fence[i], NULL);
    }
    free(wsi->imgs);
    free(wsi->imgs_acq);
    /*free(wsi->imgs_sem);*/
    free(wsi->imgs_fence);
    free(wsi);
  }
}

int yf_canpresent(VkPhysicalDevice phy_dev, int queue_i)
{
  assert(phy_dev != NULL);
  assert(queue_i > -1);

  int plat = yf_getplatform();
  VkBool32 r;

#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
# if defined(VK_USE_PLATFORM_XCB_KHR)
  /* either wayland or xcb */
  switch (plat) {
  case YF_PLATFORM_WAYLAND:
    /* TODO */
    assert(0);
  case YF_PLATFORM_XCB:
    r = vkGetPhysicalDeviceXcbPresentationSupportKHR(phy_dev, queue_i,
        yf_getconnxcb(), yf_getvisualxcb());
    break;
  default:
    yf_seterr(YF_ERR_OTHER, __func__);
    r = VK_FALSE;
  }
# else
  /* wayland */
  if (plat == YF_PLATFORM_WAYLAND) {
    /* TODO */
    assert(0);
  } else {
    yf_seterr(YF_ERR_OTHER, __func__);
    r = VK_FALSE;
  }
# endif /* defined(VK_USE_PLATFORM_XCB_KHR) */
#elif defined(VK_USE_PLATFORM_XCB)
  /* xcb */
  if (plat == YF_PLATFORM_XCB) {
    r = vkGetPhysicalDeviceXcbPresentaionSupportKHR(phy_dev, queue_i,
        yf_getconnxcb(), yf_getvisualxcb());
  } else {
    yf_seterr(YF_ERR_OTHER, __func__);
    r = VK_FALSE;
  }
#else
  /* TODO: Check other platforms. */
  r = VK_FALSE;
#endif /* defined(VK_USE_PLATFORM_WAYLAND_KHR) */

  return r == VK_TRUE;
}

static int init_surface(YF_wsi wsi)
{
  assert(wsi != NULL);
  assert(wsi->surface == VK_NULL_HANDLE);

  int plat = yf_getplatform();
  VkResult res;

#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
# if defined(VK_USE_PLATFORM_XCB_KHR)
  /* either wayland or xcb */
  switch (plat) {
  case YF_PLATFORM_WAYLAND:
    /* TODO */
    assert(0);
  case YF_PLATFORM_XCB: {
    VkXcbSurfaceCreateInfoKHR info = {
      .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
      .pNext = NULL,
      .flags = 0,
      .connection = yf_getconnxcb(),
      .window = yf_getwindowxcb(wsi->win),
    };
    res = vkCreateXcbSurfaceKHR(wsi->ctx->instance, &info, NULL,
        &wsi->surface);
    if (res != VK_SUCCESS) {
      yf_seterr(YF_ERR_DEVGEN, __func__);
      return -1;
    }
  } break;
  default:
    yf_seterr(YF_ERR_OTHER, __func__);
    return -1;
  }
# else
  /* wayland */
  if (plat == YF_PLATFORM_WAYLAND) {
    /* TODO */
    assert(0);
  } else {
    yf_seterr(YF_ERR_OTHER, __func__);
    return -1;
  }
# endif /* defined(VK_USE_PLATFORM_XCB_KHR) */
#elif defined(VK_USE_PLATFORM_XCB)
  /* xcb */
  if (plat == YF_PLATFORM_XCB) {
    VkXcbSurfaceCreateInfoKHR info = {
      .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
      .pNext = NULL,
      .flags = 0,
      .connection = yf_getconnxcb(),
      .window = yf_getwindowxcb(wsi->win),
    };
    res = vkCreateXcbSurfaceKHR(inst, &info, NULL, &wsi->surface);
    if (res != VK_SUCCESS) {
      yf_seterr(YF_ERR_DEVGEN, __func__);
      return -1;
    }
  } else {
    yf_seterr(YF_ERR_OTHER, __func__);
    return -1;
  }
#else
  /* TODO: Other platforms. */
  yf_seterr(YF_ERR_OTHER, __func__);
  return -1;
#endif /* defined(VK_USE_PLATFORM_WAYLAND_KHR) */

  VkBool32 supported;

  res = vkGetPhysicalDeviceSurfaceSupportKHR(wsi->ctx->phy_dev,
      wsi->ctx->pres_queue_i, wsi->surface, &supported);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }

  return supported == VK_TRUE ? 0 : -1;
}

static int query_surface(YF_wsi wsi)
{
  assert(wsi != NULL);
  assert(wsi->surface != VK_NULL_HANDLE);

  VkResult res;

  VkSurfaceCapabilitiesKHR capab;
  res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(wsi->ctx->phy_dev,
      wsi->surface, &capab);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }

  /* TODO: Make this configurable. */
  if (capab.maxImageCount == 0)
    wsi->img_n = YF_MAX(capab.minImageCount, 3);
  else
    wsi->img_n = capab.minImageCount;

  wsi->min_img_n = capab.minImageCount;

  VkCompositeAlphaFlagBitsKHR comp_alpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
  if (!(comp_alpha & capab.supportedCompositeAlpha)) {
    comp_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (!(comp_alpha & capab.supportedCompositeAlpha)) {
      yf_seterr(YF_ERR_UNSUP, __func__);
      return -1;
    }
  }

  unsigned width, height;
  yf_window_getsize(wsi->win, &width, &height);
  if (capab.currentExtent.width == 0xffffffff) {
    width = YF_CLAMP(width, capab.minImageExtent.width,
        capab.maxImageExtent.width);
    height = YF_CLAMP(height, capab.minImageExtent.height,
        capab.maxImageExtent.height);
  }

  VkSurfaceFormatKHR *fmts;
  unsigned fmt_n;
  res = vkGetPhysicalDeviceSurfaceFormatsKHR(wsi->ctx->phy_dev, wsi->surface,
      &fmt_n, NULL);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  fmts = malloc(fmt_n * sizeof *fmts);
  if (fmts == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  res = vkGetPhysicalDeviceSurfaceFormatsKHR(wsi->ctx->phy_dev, wsi->surface,
      &fmt_n, fmts);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    free(fmts);
    return -1;
  }

  const VkFormat pref_fmts[] = {
    VK_FORMAT_B8G8R8A8_UNORM,
    VK_FORMAT_B8G8R8A8_SRGB
  };
  const size_t pref_fmt_n = sizeof pref_fmts / sizeof pref_fmts[0];
  int fmt_i = -1;
  for (size_t i = 0; i < pref_fmt_n && fmt_i == -1; ++i) {
    for (size_t j = 0; j < fmt_n; ++j) {
      if (pref_fmts[i] == fmts[j].format &&
          fmts[j].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
        fmt_i = j;
        break;
      }
    }
  }
  if (fmt_i == -1) {
    for (size_t i = 0; i < fmt_n; ++i) {
      int pixfmt;
      YF_PIXFMT_TO(fmts[i].format, pixfmt);
      if (pixfmt != YF_PIXFMT_UNDEF) {
        fmt_i = i;
        break;
      }
    }
  }
  if (fmt_i == -1) {
    yf_seterr(YF_ERR_UNSUP, __func__);
    free(fmts);
    return -1;
  }

  VkPresentModeKHR pres_mode = VK_PRESENT_MODE_FIFO_KHR;

  unsigned queue_is[2];
  unsigned queue_i_n = 0;
  VkSharingMode shar_mode = VK_SHARING_MODE_EXCLUSIVE;
  if (wsi->ctx->graph_queue_i != wsi->ctx->pres_queue_i) {
    queue_is[0] = wsi->ctx->graph_queue_i;
    queue_is[1] = wsi->ctx->pres_queue_i;
    queue_i_n = 2;
    shar_mode = VK_SHARING_MODE_CONCURRENT;
  }

  wsi->sc_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  wsi->sc_info.pNext = NULL;
  wsi->sc_info.flags = 0;
  wsi->sc_info.surface = wsi->surface;
  wsi->sc_info.minImageCount = wsi->img_n;
  wsi->sc_info.imageFormat = fmts[fmt_i].format;
  wsi->sc_info.imageColorSpace = fmts[fmt_i].colorSpace;
  wsi->sc_info.imageExtent = (VkExtent2D){width, height};
  wsi->sc_info.imageArrayLayers = 1;
  wsi->sc_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  wsi->sc_info.imageSharingMode = shar_mode;
  wsi->sc_info.queueFamilyIndexCount = queue_i_n;
  wsi->sc_info.pQueueFamilyIndices = queue_is;
  wsi->sc_info.preTransform = capab.currentTransform;
  wsi->sc_info.compositeAlpha = comp_alpha;
  wsi->sc_info.presentMode = pres_mode;
  wsi->sc_info.clipped = VK_TRUE;
  wsi->sc_info.oldSwapchain = VK_NULL_HANDLE;

  free(fmts);
  return 0;
}

static int create_swapchain(YF_wsi wsi)
{
  assert(wsi != NULL);
  assert(wsi->surface != VK_NULL_HANDLE);

  VkResult res;

  if (wsi->swapchain != VK_NULL_HANDLE) {
    wsi->sc_info.oldSwapchain = wsi->swapchain;
    wsi->swapchain = VK_NULL_HANDLE;
    res = vkCreateSwapchainKHR(wsi->ctx->device, &wsi->sc_info, NULL,
        &wsi->swapchain);
    vkDestroySwapchainKHR(wsi->ctx->device, wsi->sc_info.oldSwapchain, NULL);
  } else {
    res = vkCreateSwapchainKHR(wsi->ctx->device, &wsi->sc_info, NULL,
        &wsi->swapchain);
  }
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }

  VkImage *imgs;
  unsigned img_n;
  res = vkGetSwapchainImagesKHR(wsi->ctx->device, wsi->swapchain, &img_n, NULL);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  imgs = malloc(img_n * sizeof *imgs);
  if (imgs == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  res = vkGetSwapchainImagesKHR(wsi->ctx->device, wsi->swapchain, &img_n, imgs);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    free(imgs);
    return -1;
  }

  if (wsi->imgs != NULL) {
    for (size_t i = 0; i < wsi->img_n; ++i) {
      yf_image_deinit(wsi->imgs[i]);
      wsi->imgs[i] = NULL;
/*
      vkDestroySemaphore(wsi->ctx->device, wsi->imgs_sem[i], NULL);
      wsi->imgs_sem[i] = VK_NULL_HANDLE;
*/
      vkDestroyFence(wsi->ctx->device, wsi->imgs_fence[i], NULL);
      wsi->imgs_fence[i] = VK_NULL_HANDLE;
    }
  }

  void *tmp_img = realloc(wsi->imgs, img_n * sizeof *wsi->imgs);
  void *tmp_acq = realloc(wsi->imgs_acq, img_n * sizeof *wsi->imgs_acq);
  /*void *tmp_sem = realloc(wsi->imgs_sem, img_n * sizeof *wsi->imgs_sem);*/
  void *tmp_fence = realloc(wsi->imgs_fence, img_n * sizeof *wsi->imgs_fence);

  if (tmp_img == NULL || tmp_acq == NULL || tmp_fence == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    free(tmp_img);
    free(tmp_acq);
    free(tmp_fence);
    free(imgs);
    return -1;
  }

  wsi->imgs = tmp_img;
  wsi->imgs_acq = tmp_acq;
  /*wsi->imgs_sem = tmp_sem;*/
  wsi->imgs_fence = tmp_fence;
  wsi->img_n = img_n;

  YF_dim3 dim = {
    wsi->sc_info.imageExtent.width,
    wsi->sc_info.imageExtent.height,
    1
  };
  for (size_t i = 0; i < img_n; ++i) {
    wsi->imgs[i] = yf_image_wrap(wsi->ctx, imgs[i], wsi->sc_info.imageFormat,
        VK_IMAGE_TYPE_2D, dim, 1, 1, VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED);
    if (wsi->imgs[i] == NULL) {
      memset(wsi->imgs+i, 0, (img_n - i) * sizeof *wsi->imgs);
      /*memset(wsi->imgs_sem, VK_NULL_HANDLE, img_n * sizeof *wsi->imgs_sem);*/
      memset(wsi->imgs_fence, VK_NULL_HANDLE, img_n * sizeof *wsi->imgs_fence);
      free(imgs);
      return -1;
    }
  }

  free(imgs);

/*
  VkSemaphoreCreateInfo sem_info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0
  };
  for (size_t i = 0; i < img_n; ++i) {
    res = vkCreateSemaphore(wsi->ctx->device, &sem_info, NULL, wsi->imgs_sem+i);
    if (res != VK_SUCCESS) {
      size_t sz = (img_n - 1) * sizeof *wsi->imgs_sem;
      memset(wsi->imgs_sem+i, VK_NULL_HANDLE, sz);
      return -1;
    }
  }
*/
  VkFenceCreateInfo fence_info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0
  };
  for (size_t i = 0; i < img_n; ++i) {
    res = vkCreateFence(wsi->ctx->device, &fence_info, NULL, wsi->imgs_fence+i);
    if (res != VK_SUCCESS) {
      size_t sz = (img_n - 1) * sizeof *wsi->imgs_fence;
      memset(wsi->imgs_fence+i, VK_NULL_HANDLE, sz);
      return -1;
    }
  }

  memset(wsi->imgs_acq, 0, img_n * sizeof *wsi->imgs_acq);
  wsi->acq_limit = 1 + img_n - wsi->min_img_n;

  return 0;
}
