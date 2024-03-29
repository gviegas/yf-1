/*
 * YF
 * context.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef __STDC_NO_ATOMICS__
# include <stdatomic.h>
#else
# error "C11 atomics required"
#endif

#include "yf/com/yf-util.h"
#include "yf/com/yf-error.h"

#include "context.h"
#include "cmdpool.h"
#include "cmdexec.h"
#include "wsi.h"

#undef YF
#define YF "YF"

#ifndef YF_CORE_VERSION
# define YF_CORE_VERSION 0
#endif

#ifndef YF_APP
# define YF_APP ""
#endif

#ifndef YF_APP_VERSION
# define YF_APP_VERSION 0
#endif

/* TODO: Should be defined elsewhere. */
#define YF_CMDPCAP 16
#define YF_CMDECAP YF_CMDPCAP

/* Flag to disallow creation of multiple contexts.
   Multiple contexts are not allowed currently because many VK symbols,
   which are loaded as globals, refer to a specific instance handle. */
static atomic_flag flag_ = ATOMIC_FLAG_INIT;

/* Sets layers. */
#if defined(YF_DEVEL) && !defined(YF_NO_VALIDATION)
static int set_layers(yf_context_t *ctx)
{
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
    for (size_t i = 0; i < opt_n; i++) {
        for (size_t j = 0; j < prop_n; j++) {
            if (strcmp(opt_lays[i], props[j].layerName) == 0) {
                ctx->layers[ctx->layer_n] = malloc(strlen(opt_lays[i])+1);
                if (ctx->layers[ctx->layer_n] == NULL) {
                    yf_seterr(YF_ERR_NOMEM, __func__);
                    free(props);
                    return -1;
                }
                strcpy(ctx->layers[ctx->layer_n], opt_lays[i]);
                ctx->layer_n++;
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

/* Sets instance extensions. */
static int set_inst_exts(yf_context_t *ctx)
{
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

    VkExtensionProperties *props =
        malloc(sizeof(VkExtensionProperties) * prop_n);
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
    for (size_t i = 0; i < req_n; i++) {
        for (size_t j = 0; j < prop_n; j++) {
            if (strcmp(req_exts[i], props[j].extensionName) == 0) {
                ctx->inst_exts[i] = malloc(strlen(req_exts[i])+1);
                if (ctx->inst_exts[i] == NULL) {
                    yf_seterr(YF_ERR_NOMEM, __func__);
                    free(props);
                    return -1;
                }
                strcpy(ctx->inst_exts[i], req_exts[i]);
                ctx->inst_ext_n++;
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

/* Initializes instance handle and instance-related properties. */
static int init_instance(yf_context_t *ctx)
{
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
        .ppEnabledLayerNames = (const char *const *)ctx->layers,
        .enabledExtensionCount = ctx->inst_ext_n,
        .ppEnabledExtensionNames = (const char *const *)ctx->inst_exts
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

/* Sets device extensions. */
static int set_dev_exts(yf_context_t *ctx)
{
    const char *req_exts[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    const size_t req_n = sizeof req_exts / sizeof req_exts[0];

    /* TODO: Optional extensions. */
    const size_t opt_n = 0;

    VkResult res;
    unsigned prop_n;

    res = vkEnumerateDeviceExtensionProperties(ctx->phy_dev, NULL, &prop_n,
                                               NULL);
    if (res != VK_SUCCESS) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        return -1;
    }
    if (prop_n < req_n) {
        yf_seterr(YF_ERR_UNSUP, __func__);
        return -1;
    }

    VkExtensionProperties *props =
        malloc(sizeof(VkExtensionProperties) * prop_n);
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
    for (size_t i = 0; i < req_n; i++) {
        for (size_t j = 0; j < prop_n; j++) {
            if (strcmp(req_exts[i], props[j].extensionName) == 0) {
                ctx->dev_exts[i] = malloc(strlen(req_exts[i])+1);
                if (ctx->dev_exts[i] == NULL) {
                    yf_seterr(YF_ERR_NOMEM, __func__);
                    free(props);
                    return -1;
                }
                strcpy(ctx->dev_exts[i], req_exts[i]);
                ctx->dev_ext_n++;
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

/* Sets features. */
static int set_features(yf_context_t *ctx)
{
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
        feat.shaderSampledImageArrayDynamicIndexing;
    ctx->features.shaderStorageBufferArrayDynamicIndexing =
        feat.shaderStorageBufferArrayDynamicIndexing;
    ctx->features.shaderStorageImageArrayDynamicIndexing =
        feat.shaderStorageImageArrayDynamicIndexing;
    ctx->features.shaderClipDistance = feat.shaderClipDistance;
    ctx->features.shaderCullDistance = feat.shaderCullDistance;

    /* required features */
    /* TODO: Refine. */
    if (ctx->features.geometryShader == VK_FALSE ||
        ctx->features.tessellationShader == VK_FALSE ||
        ctx->features.fillModeNonSolid == VK_FALSE ||
        ctx->features.wideLines == VK_FALSE ||
        ctx->features.largePoints == VK_FALSE) {
        yf_seterr(YF_ERR_UNSUP, __func__);
        return -1;
    }

    /* TODO: Query newer features. */

    return 0;
}

/* Initializes device handle and device-related properties. */
static int init_device(yf_context_t *ctx)
{
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
        free(phy_devs);
        return -1;
    }

    /* TODO: Consider sorting devices. */
    VkPhysicalDeviceProperties prop;
    for (size_t i = 0; i < phy_n; i++) {
        vkGetPhysicalDeviceProperties(phy_devs[i], &prop);
        ctx->phy_dev = phy_devs[i];

        unsigned qf_n;
        vkGetPhysicalDeviceQueueFamilyProperties(ctx->phy_dev, &qf_n, NULL);
        VkQueueFamilyProperties *qf_props;
        qf_props = malloc(sizeof(VkQueueFamilyProperties) * qf_n);
        if (qf_props == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            free(phy_devs);
            return -1;
        }
        vkGetPhysicalDeviceQueueFamilyProperties(ctx->phy_dev, &qf_n, qf_props);

        ctx->queue_i = ctx->pres_queue_i = -1;
        ctx->queue_mask = 0;
        const unsigned graph_comp = YF_QUEUE_GRAPH | YF_QUEUE_COMP;

        for (unsigned i = 0; i < qf_n; i++) {
            /* there must be a queue that supports both graphics and compute,
               otherwise graphics operations are not supported at all */
            if (ctx->queue_mask != graph_comp) {
                unsigned mask = 0;
                if (qf_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    mask |= YF_QUEUE_GRAPH;
                if (qf_props[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                    mask |= YF_QUEUE_COMP;
                if (mask != 0) {
                    ctx->queue_mask = mask;
                    ctx->queue_i = i;
                }
            }

            if (yf_canpresent(ctx->phy_dev, i))
                ctx->pres_queue_i = i;

            if (ctx->queue_mask == graph_comp && ctx->pres_queue_i != -1)
                break;
        }

        free(qf_props);

        if (ctx->queue_mask == graph_comp && ctx->pres_queue_i != -1) {
            ctx->dev_prop = prop;
            break;
        }
    }

    free(phy_devs);

    if (ctx->queue_i == -1) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        return -1;
    }

    const float priority[1] = {0.0f};
    VkDeviceQueueCreateInfo queue_infos[2];
    queue_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_infos[0].pNext = NULL;
    queue_infos[0].flags = 0;
    queue_infos[0].queueFamilyIndex = ctx->queue_i;
    queue_infos[0].queueCount = 1;
    queue_infos[0].pQueuePriorities = priority;
    unsigned queue_info_n = 1;

    if (ctx->pres_queue_i != -1 && ctx->pres_queue_i != ctx->queue_i) {
        queue_infos[queue_info_n] = queue_infos[0];
        queue_infos[queue_info_n].queueFamilyIndex = ctx->pres_queue_i;
        queue_info_n++;
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
        .ppEnabledExtensionNames = (const char *const *)ctx->dev_exts,
        .pEnabledFeatures = &ctx->features
    };

    res = vkCreateDevice(ctx->phy_dev, &dev_info, NULL, &ctx->device);
    if (res != VK_SUCCESS) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        return -1;
    }

    if (yf_setdprocvk(ctx->device) != 0)
        return -1;

    vkGetDeviceQueue(ctx->device, ctx->queue_i, 0, &ctx->queue);
    if (ctx->pres_queue_i != -1)
        vkGetDeviceQueue(ctx->device, ctx->pres_queue_i, 0, &ctx->pres_queue);

    vkGetPhysicalDeviceMemoryProperties(ctx->phy_dev, &ctx->mem_prop);
    return 0;
}

/* Initializes pipeline cache. */
static int init_cache(yf_context_t *ctx)
{
    VkPipelineCacheCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .initialDataSize = 0,
        .pInitialData = NULL
    };

    VkResult res = vkCreatePipelineCache(ctx->device, &info, NULL,
                                         &ctx->pl_cache);
    if (res != VK_SUCCESS) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        return -1;
    }
    return 0;
}

yf_context_t *yf_context_init(void)
{
    if (atomic_flag_test_and_set(&flag_)) {
        yf_seterr(YF_ERR_EXIST, __func__);
        return NULL;
    }

    if (yf_loadvk() != 0) {
        atomic_flag_clear(&flag_);
        return NULL;
    }

    yf_context_t *ctx = calloc(1, sizeof(yf_context_t));
    if (ctx == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        yf_unldvk();
        atomic_flag_clear(&flag_);
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

void yf_context_deinit(yf_context_t *ctx)
{
    if (ctx == NULL)
        return;

    vkDeviceWaitIdle(ctx->device);

    if (ctx->splr.deinit_callb != NULL)
        ctx->splr.deinit_callb(ctx);
    if (ctx->stg.deinit_callb != NULL)
        ctx->stg.deinit_callb(ctx);
    if (ctx->lim.deinit_callb != NULL)
        ctx->lim.deinit_callb(ctx);
    if (ctx->cmde.deinit_callb != NULL)
        ctx->cmde.deinit_callb(ctx);
    if (ctx->cmdp.deinit_callb != NULL)
        ctx->cmdp.deinit_callb(ctx);

    for (unsigned i = 0; i < ctx->layer_n; i++)
        free(ctx->layers[i]);
    free(ctx->layers);
    for (unsigned i = 0; i < ctx->inst_ext_n; i++)
        free(ctx->inst_exts[i]);
    free(ctx->inst_exts);
    for (unsigned i = 0; i < ctx->dev_ext_n; i++)
        free(ctx->dev_exts[i]);
    free(ctx->dev_exts);

    vkDestroyPipelineCache(ctx->device, ctx->pl_cache, NULL);
    vkDestroyDevice(ctx->device, NULL);
    vkDestroyInstance(ctx->instance, NULL);
    free(ctx);

    yf_unldvk();

    atomic_flag_clear(&flag_);
}

/*
 * DEVEL
 */

#ifdef YF_DEVEL

void yf_print_ctx(yf_context_t *ctx)
{
    assert(ctx != NULL);

    printf("\n[YF] OUTPUT (%s):\n", __func__);

    printf(" context (vk):\n"
           "  queues:\n"
           "   index (subm): %d\n"
           "   index (pres): %d\n"
           "   mask:         %x\n"
           "  api version: %u.%u\n",
           ctx->queue_i, ctx->pres_queue_i, ctx->queue_mask,
           VK_VERSION_MAJOR(ctx->inst_version),
           VK_VERSION_MINOR(ctx->inst_version));

    printf("  layers (%u):\n", ctx->layer_n);
    for (unsigned i = 0; i < ctx->layer_n; i++)
        printf("   * %s\n", ctx->layers[i]);

    puts("  extensions:");
    printf("   instance (%u):\n", ctx->inst_ext_n);
    for (unsigned i = 0; i < ctx->inst_ext_n; i++)
        printf("    * %s\n", ctx->inst_exts[i]);
    printf("   device (%u):\n", ctx->dev_ext_n);
    for (unsigned i = 0; i < ctx->dev_ext_n; i++)
        printf("    * %s\n", ctx->dev_exts[i]);

    puts("");
}

#endif
