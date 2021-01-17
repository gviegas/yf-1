/*
 * YF
 * context.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef __STDC_NO_ATOMICS__
# include <stdatomic.h>
#else
/* TODO */
# error "Missing C11 atomics"
#endif

#include <yf/com/yf-util.h>
#include <yf/com/yf-error.h>

#include "context.h"
#include "cmdpool.h"
#include "cmdexec.h"
#include "wsi.h"

#undef YF
#define YF "YF"

/* TODO */
#undef YF_CORE_VERSION
#define YF_CORE_VERSION YF_VERSION_MAKE(0, 2, 0)

#ifndef YF_APP
# define YF_APP ""
#endif

#ifndef YF_APP_VERSION
# define YF_APP_VERSION 0
#endif

#define YF_CMDPCAP 16
#define YF_CMDECAP YF_CMDPCAP

/* Flag to disallow creation of multiple contexts.
   Multiple contexts are not allowed currently because many VK symbols,
   which are loaded as globals, refer to a specific instance handle. */
static atomic_flag l_flag = ATOMIC_FLAG_INIT;

/* Initializes instance handle and instance-related properties. */
static int init_instance(YF_context ctx);

/* Initializes device handle and device-related properties. */
static int init_device(YF_context ctx);

/* Initializes pipeline cache. */
static int init_cache(YF_context ctx);

/* Sets layers. */
#if defined(YF_DEVEL) && !defined(YF_NO_VALIDATION)
static int set_layers(YF_context ctx);
#endif

/* Sets instance extensions. */
static int set_inst_exts(YF_context ctx);

/* Sets device extensions. */
static int set_dev_exts(YF_context ctx);

/* Sets features. */
static int set_features(YF_context ctx);

YF_context yf_context_init(void) {
  if (atomic_flag_test_and_set(&l_flag)) {
    yf_seterr(YF_ERR_EXIST, __func__);
    return NULL;
  }

  if (yf_loadvk() != 0) {
    yf_context_deinit(NULL);
    return NULL;
  }

  YF_context ctx = calloc(1, sizeof(YF_context_o));
  if (ctx == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    yf_context_deinit(NULL);
    return NULL;
  }
  if (init_instance(ctx) != 0) {
    yf_context_deinit(ctx);
    return NULL;
  }
  if (init_device(ctx) != 0) {
    yf_context_deinit(ctx);
    return NULL;
  }
  if (init_cache(ctx) != 0) {
    yf_context_deinit(ctx);
    return NULL;
  }
  if (yf_cmdpool_create(ctx, YF_CMDPCAP) != 0) {
    yf_context_deinit(ctx);
    return NULL;
  }
  if (yf_cmdexec_create(ctx, YF_CMDECAP) != 0) {
    yf_context_deinit(ctx);
    return NULL;
  }

  return ctx;
}

void yf_context_deinit(YF_context ctx) {
  atomic_flag_clear(&l_flag);

  if (ctx == NULL)
    return;

  if (ctx->stg.deinit_callb != NULL)
    ctx->stg.deinit_callb(ctx);
  if (ctx->lim.deinit_callb != NULL)
    ctx->lim.deinit_callb(ctx);
  if (ctx->cmde.deinit_callb != NULL)
    ctx->cmde.deinit_callb(ctx);
  if (ctx->cmdp.deinit_callb != NULL)
    ctx->cmdp.deinit_callb(ctx);

  for (unsigned i = 0; i < ctx->layer_n; ++i)
    free(ctx->layers[i]);
  free(ctx->layers);
  for (unsigned i = 0; i < ctx->inst_ext_n; ++i)
    free(ctx->inst_exts[i]);
  free(ctx->inst_exts);
  for (unsigned i = 0; i < ctx->dev_ext_n; ++i)
    free(ctx->dev_exts[i]);
  free(ctx->dev_exts);

  vkDestroyDevice(ctx->device, NULL);
  vkDestroyInstance(ctx->instance, NULL);
  free(ctx);

  yf_unldvk();
}

static int init_instance(YF_context ctx) {
  assert(ctx->instance == NULL);

  if (yf_setiprocvk(NULL) != 0)
    return -1;

  if (vkEnumerateInstanceVersion != NULL)
    vkEnumerateInstanceVersion(&ctx->inst_version);
  else
    ctx->inst_version = VK_API_VERSION_1_0;

#if defined(YF_DEVEL) && !defined(YF_NO_VALIDATION)
  if (set_layers(ctx) != 0)
    return -1;
#endif

  if (set_inst_exts(ctx) != 0)
    return -1;

  VkApplicationInfo app_info = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext = NULL,
    .pApplicationName = YF_APP,
    .applicationVersion = YF_APP_VERSION,
    .pEngineName = YF,
    .engineVersion = YF_CORE_VERSION,
    .apiVersion = ctx->inst_version
  };
  VkInstanceCreateInfo inst_info = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .pApplicationInfo = &app_info,
    .enabledLayerCount = ctx->layer_n,
    .ppEnabledLayerNames = (const char * const *)ctx->layers,
    .enabledExtensionCount = ctx->inst_ext_n,
    .ppEnabledExtensionNames = (const char * const *)ctx->inst_exts
  };
  VkResult res = vkCreateInstance(&inst_info, NULL, &ctx->instance);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }

  if (yf_setiprocvk(ctx->instance) != 0)
    return -1;

  return 0;
}

static int init_device(YF_context ctx) {
  assert(ctx->instance != NULL);
  assert(ctx->phy_dev == NULL);
  assert(ctx->device == NULL);

  VkResult res;
  unsigned phy_n;

  res = vkEnumeratePhysicalDevices(ctx->instance, &phy_n, NULL);
  if (res != VK_SUCCESS || phy_n == 0) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  VkPhysicalDevice *phy_devs = malloc(sizeof(VkPhysicalDevice) * phy_n);
  if (phy_devs == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  res = vkEnumeratePhysicalDevices(ctx->instance, &phy_n, phy_devs);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }

  /* TODO: Consider sorting devices. */
  VkPhysicalDeviceProperties prop;
  for (size_t i = 0; i < phy_n; ++i) {
    vkGetPhysicalDeviceProperties(phy_devs[i], &prop);
    ctx->phy_dev = phy_devs[i];

    unsigned qf_n;
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->phy_dev, &qf_n, NULL);
    VkQueueFamilyProperties *qf_props;
    qf_props = malloc(sizeof(VkQueueFamilyProperties) * qf_n);
    if (qf_props == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->phy_dev, &qf_n, qf_props);

    ctx->graph_queue_i = ctx->comp_queue_i = ctx->pres_queue_i = -1;
    for (unsigned i = 0; i < qf_n; ++i) {
      int found[3] = {0, 0, 0};

      if (qf_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        ctx->graph_queue_i = i;
        found[0] = 1;
      }
      if (qf_props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
        ctx->comp_queue_i = i;
        found[1] = 1;
      }
      if (yf_canpresent(ctx->phy_dev, i)) {
        ctx->pres_queue_i = i;
        found[2] = 1;
      }

      if (found[0] && found[1] && found[2])
        break;
    }
    free(qf_props);

    if (ctx->graph_queue_i != -1 && ctx->comp_queue_i != -1 &&
        ctx->pres_queue_i != -1)
    {
      ctx->dev_prop = prop;
      break;
    }
  }

  if (ctx->graph_queue_i == -1 && ctx->comp_queue_i == -1) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }

  const int have_graph = ctx->graph_queue_i != -1;
  const int have_comp = ctx->comp_queue_i != -1;
  const int have_pres = ctx->pres_queue_i != -1;

  const float priority[1] = {0.0f};
  VkDeviceQueueCreateInfo queue_infos[3];
  queue_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_infos[0].pNext = NULL;
  queue_infos[0].flags = 0;
  queue_infos[0].queueFamilyIndex = 0;
  queue_infos[0].queueCount = 1;
  queue_infos[0].pQueuePriorities = priority;
  unsigned queue_info_n = 1;

  if (have_graph) {
    queue_infos[0].queueFamilyIndex = ctx->graph_queue_i;
    if (have_comp) {
      if (ctx->graph_queue_i != ctx->comp_queue_i) {
        queue_infos[queue_info_n] = queue_infos[0];
        queue_infos[queue_info_n].queueFamilyIndex = ctx->comp_queue_i;
        ++queue_info_n;
      }
    }
    if (have_pres) {
      if (ctx->graph_queue_i != ctx->pres_queue_i &&
          ctx->pres_queue_i != ctx->comp_queue_i)
      {
        queue_infos[queue_info_n] = queue_infos[0];
        queue_infos[queue_info_n].queueFamilyIndex = ctx->pres_queue_i;
        ++queue_info_n;
      }
    }
  } else {
    /* ignore pres. queue */
    ctx->pres_queue_i = -1;
    queue_infos[0].queueFamilyIndex = ctx->comp_queue_i;
  }

  if (set_dev_exts(ctx) != 0 || set_features(ctx) != 0)
    return -1;

  VkDeviceCreateInfo dev_info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .queueCreateInfoCount = queue_info_n,
    .pQueueCreateInfos = queue_infos,
    .enabledLayerCount = 0,
    .ppEnabledLayerNames = NULL,
    .enabledExtensionCount = ctx->dev_ext_n,
    .ppEnabledExtensionNames = (const char * const *)ctx->dev_exts,
    .pEnabledFeatures = &ctx->features
  };
  res = vkCreateDevice(ctx->phy_dev, &dev_info, NULL, &ctx->device);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }

  if (yf_setdprocvk(ctx->device) != 0)
    return -1;

  if (have_graph)
    vkGetDeviceQueue(ctx->device, ctx->graph_queue_i, 0, &ctx->graph_queue);
  if (have_comp)
    vkGetDeviceQueue(ctx->device, ctx->comp_queue_i, 0, &ctx->comp_queue);
  if (have_pres)
    vkGetDeviceQueue(ctx->device, ctx->pres_queue_i, 0, &ctx->pres_queue);

  vkGetPhysicalDeviceMemoryProperties(ctx->phy_dev, &ctx->mem_prop);
  return 0;
}

static int init_cache(YF_context ctx) {
  VkPipelineCacheCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .initialDataSize = 0,
    .pInitialData = NULL
  };
  VkResult res;
  res = vkCreatePipelineCache(ctx->device, &info, NULL, &ctx->pl_cache);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  return 0;
}

#if defined(YF_DEVEL) && !defined(YF_NO_VALIDATION)
static int set_layers(YF_context ctx) {
  const char *opt_lays[4];
  size_t opt_n = 0;
#if defined(__linux__)
  opt_lays[opt_n++] = "VK_LAYER_MESA_overlay";
  opt_lays[opt_n++] = "VK_LAYER_MESA_device_select";
#endif
  /* TODO: Other platforms. */

  if (opt_n == 0)
    return 0;

  VkResult res;
  unsigned prop_n;

  res = vkEnumerateInstanceLayerProperties(&prop_n, NULL);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  if (prop_n == 0)
    return 0;

  VkLayerProperties *props = malloc(sizeof(VkLayerProperties) * prop_n);
  if (props == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  res = vkEnumerateInstanceLayerProperties(&prop_n, props);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    free(props);
    return -1;
  }

  ctx->layers = calloc(opt_n, sizeof(char *));
  if (ctx->layers == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    free(props);
    return -1;
  }
  ctx->layer_n = 0;

  /* optional layers */
  for (size_t i = 0; i < opt_n; ++i) {
    for (size_t j = 0; j < prop_n; ++j) {
      if (strcmp(opt_lays[i], props[j].layerName) == 0) {
        ctx->layers[ctx->layer_n] = malloc(strlen(opt_lays[i]+1));
        if (ctx->layers[ctx->layer_n] == NULL) {
          yf_seterr(YF_ERR_NOMEM, __func__);
          free(props);
          return -1;
        }
        strcpy(ctx->layers[ctx->layer_n], opt_lays[i]);
        ++ctx->layer_n;
        break;
      }
    }
  }

  if (ctx->layer_n == 0) {
    free(ctx->layers);
    ctx->layers = NULL;
  } else if (ctx->layer_n < opt_n) {
    void *tmp = realloc(ctx->layers, ctx->layer_n * sizeof(char *));
    if (tmp != NULL)
      ctx->layers = tmp;
  }

  free(props);
  return 0;
}
#endif /* defined(YF_DEVEL) && !defined(YF_NO_VALIDATION) */

static int set_inst_exts(YF_context ctx) {
  const char *req_exts[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
    VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#endif
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#if defined(VK_USE_PLATFORM_XCB_KHR)
    VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
#if defined(VK_USE_PLATFORM_METAL_EXT)
    VK_EXT_METAL_SURFACE_EXTENSION_NAME,
#endif
  };
  const size_t req_n = sizeof req_exts / sizeof req_exts[0];

  /* TODO: Optional extensions. */
  const size_t opt_n = 0;

  VkResult res;
  unsigned prop_n;

  res = vkEnumerateInstanceExtensionProperties(NULL, &prop_n, NULL);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  if (prop_n < req_n) {
    yf_seterr(YF_ERR_UNSUP, __func__);
    return -1;
  }

  VkExtensionProperties *props = malloc(sizeof(VkExtensionProperties) * prop_n);
  if (props == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  res = vkEnumerateInstanceExtensionProperties(NULL, &prop_n, props);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    free(props);
    return -1;
  }

  ctx->inst_exts = calloc(req_n + opt_n, sizeof(char *));
  if (ctx->inst_exts == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    free(props);
    return -1;
  }
  ctx->inst_ext_n = 0;

  /* required extensions */
  for (size_t i = 0; i < req_n; ++i) {
    for (size_t j = 0; j < prop_n; ++j) {
      if (strcmp(req_exts[i], props[j].extensionName) == 0) {
        ctx->inst_exts[i] = malloc(strlen(req_exts[i]+1));
        if (ctx->inst_exts[i] == NULL) {
          yf_seterr(YF_ERR_NOMEM, __func__);
          free(props);
          return -1;
        }
        strcpy(ctx->inst_exts[i], req_exts[i]);
        ++ctx->inst_ext_n;
        break;
      }
    }
    if (ctx->inst_ext_n == i) {
      /* not found */
      yf_seterr(YF_ERR_UNSUP, __func__);
      free(props);
      return -1;
    }
  }

  /* TODO: Check optional extensions & realloc if any not found. */

  free(props);
  return 0;
}

static int set_dev_exts(YF_context ctx) {
  const char *req_exts[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };
  const size_t req_n = sizeof req_exts / sizeof req_exts[0];

  /* TODO: Optional extensions. */
  const size_t opt_n = 0;

  VkResult res;
  unsigned prop_n;

  res = vkEnumerateDeviceExtensionProperties(ctx->phy_dev, NULL, &prop_n, NULL);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  if (prop_n < req_n) {
    yf_seterr(YF_ERR_UNSUP, __func__);
    return -1;
  }

  VkExtensionProperties *props = malloc(sizeof(VkExtensionProperties) * prop_n);
  if (props == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  res = vkEnumerateDeviceExtensionProperties(ctx->phy_dev, NULL, &prop_n,
      props);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    free(props);
    return -1;
  }

  ctx->dev_exts = calloc(req_n + opt_n, sizeof(char *));
  if (ctx->dev_exts == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    free(props);
    return -1;
  }
  ctx->dev_ext_n = 0;

  /* required extensions */
  for (size_t i = 0; i < req_n; ++i) {
    for (size_t j = 0; j < prop_n; ++j) {
      if (strcmp(req_exts[i], props[j].extensionName) == 0) {
        ctx->dev_exts[i] = malloc(strlen(req_exts[i]+1));
        if (ctx->dev_exts[i] == NULL) {
          yf_seterr(YF_ERR_NOMEM, __func__);
          free(props);
          return -1;
        }
        strcpy(ctx->dev_exts[i], req_exts[i]);
        ++ctx->dev_ext_n;
        break;
      }
    }
    if (ctx->dev_ext_n == i) {
      /* not found */
      yf_seterr(YF_ERR_UNSUP, __func__);
      free(props);
      return -1;
    }
  }

  /* TODO: Check optional extensions & realloc if any not found. */

  free(props);
  return 0;
}

static int set_features(YF_context ctx) {
  VkPhysicalDeviceFeatures feat;
  vkGetPhysicalDeviceFeatures(ctx->phy_dev, &feat);

  /* TODO: Code should be updated to check these settings and ensure that
     only available features are being used. */

  ctx->features.fullDrawIndexUint32 = feat.fullDrawIndexUint32;
  ctx->features.imageCubeArray = feat.imageCubeArray;
  ctx->features.independentBlend = feat.independentBlend;
  ctx->features.geometryShader = feat.geometryShader;
  ctx->features.tessellationShader = feat.tessellationShader;
  ctx->features.sampleRateShading = feat.sampleRateShading;
  ctx->features.dualSrcBlend = feat.dualSrcBlend;
  ctx->features.depthClamp = feat.depthClamp;
  ctx->features.depthBiasClamp = feat.depthBiasClamp;
  ctx->features.fillModeNonSolid = feat.fillModeNonSolid;
  ctx->features.wideLines = feat.wideLines;
  ctx->features.largePoints = feat.largePoints;
  ctx->features.alphaToOne = feat.alphaToOne;
  ctx->features.multiViewport = feat.multiViewport;
  ctx->features.samplerAnisotropy = feat.samplerAnisotropy;
  ctx->features.fragmentStoresAndAtomics = feat.fragmentStoresAndAtomics;
  ctx->features.shaderImageGatherExtended = feat.shaderImageGatherExtended;
  ctx->features.shaderUniformBufferArrayDynamicIndexing =
    feat.shaderUniformBufferArrayDynamicIndexing;
  ctx->features.shaderSampledImageArrayDynamicIndexing =
    feat.shaderSampledImageArrayDynamicIndexing =
  ctx->features.shaderStorageBufferArrayDynamicIndexing =
    feat.shaderStorageBufferArrayDynamicIndexing =
  ctx->features.shaderStorageImageArrayDynamicIndexing =
    feat.shaderStorageImageArrayDynamicIndexing =
  ctx->features.shaderClipDistance = feat.shaderClipDistance;
  ctx->features.shaderCullDistance = feat.shaderCullDistance;

  /* required features */
  /* TODO: Refine. */
  if (ctx->features.geometryShader == VK_FALSE ||
      ctx->features.tessellationShader == VK_FALSE ||
      ctx->features.fillModeNonSolid == VK_FALSE ||
      ctx->features.wideLines == VK_FALSE ||
      ctx->features.largePoints == VK_FALSE)
  {
    yf_seterr(YF_ERR_UNSUP, __func__);
    return -1;
  }

  /* TODO: Query newer features. */

  return 0;
}
