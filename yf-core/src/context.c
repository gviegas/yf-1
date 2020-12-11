/*
 * YF
 * context.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

/* TODO: Disallow creation of multiple contexts. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <yf/com/yf-error.h>

#include "context.h"
#include "cmdpool.h"
#include "cmdexec.h"

#ifndef YF_MIN
# define YF_MIN(a, b) (a < b ? a : b)
#endif

#undef YF
#define YF "YF"

#ifndef YF_APP
# define YF_APP ""
#endif

#ifndef YF_APP_VERSION
# define YF_APP_VERSION 0
#endif

#define YF_CMDPCAP 16
#define YF_CMDECAP YF_CMDPCAP

/* Initializes instance handle and instance-related properties. */
static int init_instance(YF_context ctx);

/* Initializes device handle and device-related properties. */
static int init_device(YF_context ctx);

/* Initializes pipeline cache. */
static int init_cache(YF_context ctx);

/* Sets layers. */
#if defined(YF_DEBUG) && !defined(YF_NO_VALIDATION)
static int set_layers(YF_context ctx);
#endif

/* Sets instance extensions. */
static int set_inst_exts(YF_context ctx);

/* Sets device extensions. */
static int set_dev_exts(YF_context ctx);

YF_context yf_context_init(void) {
  if (yf_loadvk() != 0)
    return NULL;

  YF_context ctx = calloc(1, sizeof(YF_context_o));
  if (ctx == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
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

#if defined(YF_DEBUG) && !defined(YF_NO_VALIDATION)
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
  unsigned n;
  VkResult res;

  res = vkEnumeratePhysicalDevices(ctx->instance, &n, NULL);
  if (res != VK_SUCCESS || n == 0) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  VkPhysicalDevice *phy_devs = malloc(sizeof(VkPhysicalDevice) * n);
  if (phy_devs == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  res = vkEnumeratePhysicalDevices(ctx->instance, &n, phy_devs);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }

  VkPhysicalDeviceProperties prop;
  for (unsigned i = 0; i < n; ++i) {
    vkGetPhysicalDeviceProperties(phy_devs[i], &prop);
    if (prop.apiVersion > ctx->dev_prop.apiVersion) {
      ctx->phy_dev = phy_devs[i];
      ctx->dev_prop = prop;
    }
  }
  free(phy_devs);

  if (set_dev_exts(ctx) != 0)
    return -1;

  vkGetPhysicalDeviceQueueFamilyProperties(ctx->phy_dev, &n, NULL);
  VkQueueFamilyProperties *qf_props;
  qf_props = malloc(sizeof(VkQueueFamilyProperties) * n);
  if (qf_props == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  vkGetPhysicalDeviceQueueFamilyProperties(ctx->phy_dev, &n, qf_props);

  ctx->graph_queue_i = ctx->comp_queue_i = -1;
  for (unsigned i = 0; i < n; ++i) {
    int found[2] = {0, 0};
    if (qf_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      ctx->graph_queue_i = i;
      found[0] = 1;
    }
    if (qf_props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
      ctx->comp_queue_i = i;
      found[1] = 1;
    }
    if (found[0] && found[1])
      break;
  }
  free(qf_props);
  if (ctx->graph_queue_i == -1 && ctx->comp_queue_i == -1) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }

  const int have_graph = ctx->graph_queue_i != -1;
  const int have_comp = ctx->comp_queue_i != -1;
  const int same_queue = ctx->graph_queue_i == ctx->comp_queue_i;
  const float priority[1] = {0.0f};
  VkDeviceQueueCreateInfo queue_infos[2] = {0};
  queue_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_infos[0].pNext = NULL;
  queue_infos[0].flags = 0;
  queue_infos[0].queueFamilyIndex = 0;
  queue_infos[0].queueCount = 1;
  queue_infos[0].pQueuePriorities = priority;
  unsigned queue_info_n = 1;

  if (have_graph && have_comp) {
    queue_infos[0].queueFamilyIndex = ctx->graph_queue_i;
    if (!same_queue) {
      memcpy(queue_infos + 1, queue_infos, sizeof queue_infos[0]);
      queue_infos[1].queueFamilyIndex = ctx->comp_queue_i;
      queue_info_n = 2;
    }
  } else {
    if (have_graph)
      queue_infos[0].queueFamilyIndex = ctx->graph_queue_i;
    else
      queue_infos[0].queueFamilyIndex = ctx->comp_queue_i;
  }

  /* TODO: Enable features of interest only. */
  VkPhysicalDeviceFeatures feat;
  vkGetPhysicalDeviceFeatures(ctx->phy_dev, &feat);

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
    .pEnabledFeatures = &feat
  };
  res = vkCreateDevice(ctx->phy_dev, &dev_info, NULL, &ctx->device);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }

  if (yf_setdprocvk(ctx->device) != 0)
    return -1;

  if (same_queue) {
    vkGetDeviceQueue(ctx->device, ctx->graph_queue_i, 0, &ctx->graph_queue);
    ctx->comp_queue = ctx->graph_queue;
  } else {
    if (have_graph)
      vkGetDeviceQueue(ctx->device, ctx->graph_queue_i, 0, &ctx->graph_queue);
    if (have_comp)
      vkGetDeviceQueue(ctx->device, ctx->comp_queue_i, 0, &ctx->comp_queue);
  }

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

#if defined(YF_DEBUG) && !defined(YF_NO_VALIDATION)
static int set_layers(YF_context ctx) {
  unsigned n;
  VkResult res;

  res = vkEnumerateInstanceLayerProperties(&n, NULL);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  if (n == 0)
    return 0;

  VkLayerProperties *props = malloc(sizeof(VkLayerProperties) * n);
  if (props == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  res = vkEnumerateInstanceLayerProperties(&n, props);
  if (res != VK_SUCCESS) {
    free(props);
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }

  ctx->layers = calloc(n, sizeof(char *));
  if (ctx->layers == NULL) {
    free(props);
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  const size_t len = sizeof props[0].layerName;
  for (unsigned i = 0; i < n; ++i) {
    ctx->layers[i] = malloc(len);
    if (ctx->layers[i] == NULL) {
      free(props);
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    strncpy(ctx->layers[i], props[i].layerName, len);
  }

  ctx->layer_n = n;
  free(props);
  return 0;
}
#endif

static int set_inst_exts(YF_context ctx) {
  unsigned n;
  VkResult res;

  res = vkEnumerateInstanceExtensionProperties(NULL, &n, NULL);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  if (n == 0)
    return 0;

  VkExtensionProperties *props = malloc(sizeof(VkExtensionProperties) * n);
  if (props == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  res = vkEnumerateInstanceExtensionProperties(NULL, &n, props);
  if (res != VK_SUCCESS) {
    free(props);
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }

  ctx->inst_exts = calloc(n, sizeof(char *));
  if (ctx->inst_exts == NULL) {
    free(props);
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  const size_t len = sizeof props[0].extensionName;
  for (unsigned i = 0; i < n; ++i) {
    ctx->inst_exts[i] = malloc(len);
    if (ctx->inst_exts[i] == NULL) {
      free(props);
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    strncpy(ctx->inst_exts[i], props[i].extensionName, len);
  }

  ctx->inst_ext_n = n;
  free(props);
  return 0;
}

static int set_dev_exts(YF_context ctx) {
  unsigned n;
  VkResult res;

  res = vkEnumerateDeviceExtensionProperties(ctx->phy_dev, NULL, &n, NULL);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  if (n == 0)
    return 0;

  VkExtensionProperties *props = malloc(sizeof(VkExtensionProperties) * n);
  if (props == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  res = vkEnumerateDeviceExtensionProperties(ctx->phy_dev, NULL, &n, props);
  if (res != VK_SUCCESS) {
    free(props);
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }

  ctx->dev_exts = calloc(n, sizeof(char *));
  if (ctx->dev_exts == NULL) {
    free(props);
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  const size_t len = sizeof props[0].extensionName;
  for (unsigned i = 0; i < n; ++i) {
    ctx->dev_exts[i] = malloc(len);
    if (ctx->dev_exts[i] == NULL) {
      free(props);
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    strncpy(ctx->dev_exts[i], props[i].extensionName, len);
  }

  ctx->dev_ext_n = n;
  free(props);
  return 0;
}
