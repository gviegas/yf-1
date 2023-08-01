/*
 * YF
 * vk.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include "yf/com/yf-error.h"

#include "vk.h"

#if defined(__linux__)
# include <dlfcn.h>
# define YF_LIBVK "libvulkan.so.1"
#elif defined(__APPLE__)
# include <dlfcn.h>
# define YF_LIBVK "libvulkan.dylib"
#elif defined(_WIN32)
# include <windows.h>
# define YF_LIBVK "vulkan-1.dll"
#else
# error "Invalid platform"
#endif /* defined(__linux__) */

/* Shared object handle. */
static void *handle_ = NULL;

#if defined(__linux__) || defined(__APPLE__)
int yf_loadvk(void)
{
    if (handle_ != NULL)
        return 0;

    handle_ = dlopen(YF_LIBVK, RTLD_LAZY);
    if (handle_ == NULL) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        return -1;
    }

    *(void **)(&vkGetInstanceProcAddr) = dlsym(handle_,
                                               "vkGetInstanceProcAddr");
    if (vkGetInstanceProcAddr == NULL) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        yf_unldvk();
        return -1;
    }

    return 0;
}

void yf_unldvk(void)
{
    if (handle_ != NULL) {
        dlclose(handle_);
        handle_ = NULL;
    }
}
#elif defined(_WIN32)
# error "Unimplemented"
#else
# error "Invalid Platform"
#endif /* defined(__linux__) || defined(__APPLE__) */

int yf_setiprocvk(VkInstance instance)
{
    if (handle_ == NULL && yf_loadvk() != 0)
        return -1;

    if (instance == NULL) {
        YF_IPROCVK(NULL, vkGetInstanceProcAddr);
        YF_IPROCVK(NULL, vkEnumerateInstanceVersion);
        YF_IPROCVK(NULL, vkEnumerateInstanceExtensionProperties);
        YF_IPROCVK(NULL, vkEnumerateInstanceLayerProperties);
        YF_IPROCVK(NULL, vkCreateInstance);
    } else {
        YF_IPROCVK(instance, vkDestroyInstance);
        YF_IPROCVK(instance, vkEnumeratePhysicalDevices);
        YF_IPROCVK(instance, vkGetPhysicalDeviceProperties);
        YF_IPROCVK(instance, vkGetPhysicalDeviceQueueFamilyProperties);
        YF_IPROCVK(instance, vkGetPhysicalDeviceMemoryProperties);
        YF_IPROCVK(instance, vkGetPhysicalDeviceFormatProperties);
        YF_IPROCVK(instance, vkGetPhysicalDeviceImageFormatProperties);
        YF_IPROCVK(instance, vkGetPhysicalDeviceFeatures);
        YF_IPROCVK(instance, vkGetPhysicalDeviceProperties2);
        YF_IPROCVK(instance, vkGetPhysicalDeviceQueueFamilyProperties2);
        YF_IPROCVK(instance, vkGetPhysicalDeviceMemoryProperties2);
        YF_IPROCVK(instance, vkGetPhysicalDeviceFormatProperties2);
        YF_IPROCVK(instance, vkGetPhysicalDeviceFeatures2);
        YF_IPROCVK(instance, vkEnumerateDeviceExtensionProperties);
        YF_IPROCVK(instance, vkCreateDevice);
        YF_IPROCVK(instance, vkDestroySurfaceKHR);
        YF_IPROCVK(instance, vkGetPhysicalDeviceSurfaceSupportKHR);
        YF_IPROCVK(instance, vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
        YF_IPROCVK(instance, vkGetPhysicalDeviceSurfaceFormatsKHR);
        YF_IPROCVK(instance, vkGetPhysicalDeviceSurfacePresentModesKHR);
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        YF_IPROCVK(instance, vkCreateWaylandSurfaceKHR); /* VK_KHR_wayland_surface */
        YF_IPROCVK(instance, vkGetPhysicalDeviceWaylandPresentationSupportKHR); /* VK_KHR_wayland_surface */
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
        YF_IPROCVK(instance, vkCreateWin32SurfaceKHR); /* VK_KHR_win32_surface */
        YF_IPROCVK(instance, vkGetPhysicalDeviceWin32PresentationSupportKHR); /* VK_KHR_win32_surface */
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
        YF_IPROCVK(instance, vkCreateXcbSurfaceKHR); /* VK_KHR_xcb_surface */
        YF_IPROCVK(instance, vkGetPhysicalDeviceXcbPresentationSupportKHR); /* VK_KHR_xcb_surface */
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
        YF_IPROCVK(instance, vkCreateMetalSurfaceEXT); /* VK_EXT_metal_surface */
#endif
        YF_IPROCVK(instance, vkGetDeviceProcAddr);
    }

    return 0;
}

int yf_setdprocvk(VkDevice device)
{
    if (handle_ == NULL || device == NULL) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    YF_DPROCVK(device, vkGetDeviceProcAddr);
    YF_DPROCVK(device, vkDestroyDevice);
    YF_DPROCVK(device, vkGetDeviceQueue);
    YF_DPROCVK(device, vkCreateCommandPool);
    YF_DPROCVK(device, vkTrimCommandPool);
    YF_DPROCVK(device, vkResetCommandPool);
    YF_DPROCVK(device, vkDestroyCommandPool);
    YF_DPROCVK(device, vkAllocateCommandBuffers);
    YF_DPROCVK(device, vkResetCommandBuffer);
    YF_DPROCVK(device, vkFreeCommandBuffers);
    YF_DPROCVK(device, vkBeginCommandBuffer);
    YF_DPROCVK(device, vkEndCommandBuffer);
    YF_DPROCVK(device, vkQueueSubmit);
    YF_DPROCVK(device, vkCmdExecuteCommands);
    YF_DPROCVK(device, vkCreateFence);
    YF_DPROCVK(device, vkDestroyFence);
    YF_DPROCVK(device, vkGetFenceStatus);
    YF_DPROCVK(device, vkResetFences);
    YF_DPROCVK(device, vkWaitForFences);
    YF_DPROCVK(device, vkCreateSemaphore);
    YF_DPROCVK(device, vkDestroySemaphore);
    YF_DPROCVK(device, vkGetSemaphoreCounterValue);
    YF_DPROCVK(device, vkWaitSemaphores);
    YF_DPROCVK(device, vkSignalSemaphore);
    YF_DPROCVK(device, vkCmdPipelineBarrier);
    YF_DPROCVK(device, vkQueueWaitIdle);
    YF_DPROCVK(device, vkDeviceWaitIdle);
    YF_DPROCVK(device, vkCreateRenderPass);
    YF_DPROCVK(device, vkDestroyRenderPass);
    YF_DPROCVK(device, vkCreateFramebuffer);
    YF_DPROCVK(device, vkDestroyFramebuffer);
    YF_DPROCVK(device, vkCmdBeginRenderPass);
    YF_DPROCVK(device, vkCmdEndRenderPass);
    YF_DPROCVK(device, vkCmdNextSubpass);
    YF_DPROCVK(device, vkCreateShaderModule);
    YF_DPROCVK(device, vkDestroyShaderModule);
    YF_DPROCVK(device, vkCreateGraphicsPipelines);
    YF_DPROCVK(device, vkCreateComputePipelines);
    YF_DPROCVK(device, vkDestroyPipeline);
    YF_DPROCVK(device, vkCmdBindPipeline);
    YF_DPROCVK(device, vkCreatePipelineCache);
    YF_DPROCVK(device, vkMergePipelineCaches);
    YF_DPROCVK(device, vkGetPipelineCacheData);
    YF_DPROCVK(device, vkDestroyPipelineCache);
    YF_DPROCVK(device, vkAllocateMemory);
    YF_DPROCVK(device, vkFreeMemory);
    YF_DPROCVK(device, vkMapMemory);
    YF_DPROCVK(device, vkUnmapMemory);
    YF_DPROCVK(device, vkCreateBuffer);
    YF_DPROCVK(device, vkDestroyBuffer);
    YF_DPROCVK(device, vkCreateBufferView);
    YF_DPROCVK(device, vkDestroyBufferView);
    YF_DPROCVK(device, vkCreateImage);
    YF_DPROCVK(device, vkDestroyImage);
    YF_DPROCVK(device, vkGetImageSubresourceLayout);
    YF_DPROCVK(device, vkCreateImageView);
    YF_DPROCVK(device, vkDestroyImageView);
    YF_DPROCVK(device, vkGetBufferMemoryRequirements);
    YF_DPROCVK(device, vkGetImageMemoryRequirements);
    YF_DPROCVK(device, vkBindBufferMemory);
    YF_DPROCVK(device, vkBindImageMemory);
    YF_DPROCVK(device, vkCreateSampler);
    YF_DPROCVK(device, vkDestroySampler);
    YF_DPROCVK(device, vkCreateDescriptorSetLayout);
    YF_DPROCVK(device, vkDestroyDescriptorSetLayout);
    YF_DPROCVK(device, vkGetDescriptorSetLayoutSupport);
    YF_DPROCVK(device, vkCreatePipelineLayout);
    YF_DPROCVK(device, vkDestroyPipelineLayout);
    YF_DPROCVK(device, vkCreateDescriptorPool);
    YF_DPROCVK(device, vkDestroyDescriptorPool);
    YF_DPROCVK(device, vkResetDescriptorPool);
    YF_DPROCVK(device, vkAllocateDescriptorSets);
    YF_DPROCVK(device, vkFreeDescriptorSets);
    YF_DPROCVK(device, vkUpdateDescriptorSets);
    YF_DPROCVK(device, vkCmdBindDescriptorSets);
    YF_DPROCVK(device, vkCmdPushConstants);
    YF_DPROCVK(device, vkCmdClearColorImage);
    YF_DPROCVK(device, vkCmdClearDepthStencilImage);
    YF_DPROCVK(device, vkCmdClearAttachments);
    YF_DPROCVK(device, vkCmdFillBuffer);
    YF_DPROCVK(device, vkCmdUpdateBuffer);
    YF_DPROCVK(device, vkCmdCopyBuffer);
    YF_DPROCVK(device, vkCmdCopyImage);
    YF_DPROCVK(device, vkCmdCopyBufferToImage);
    YF_DPROCVK(device, vkCmdCopyImageToBuffer);
    YF_DPROCVK(device, vkCmdBlitImage);
    YF_DPROCVK(device, vkCmdResolveImage);
    YF_DPROCVK(device, vkCmdDraw);
    YF_DPROCVK(device, vkCmdDrawIndexed);
    YF_DPROCVK(device, vkCmdDrawIndirect);
    YF_DPROCVK(device, vkCmdDrawIndexedIndirect);
    YF_DPROCVK(device, vkCmdBindVertexBuffers);
    YF_DPROCVK(device, vkCmdBindIndexBuffer);
    YF_DPROCVK(device, vkCmdSetViewport);
    YF_DPROCVK(device, vkCmdSetLineWidth);
    YF_DPROCVK(device, vkCmdSetDepthBias);
    YF_DPROCVK(device, vkCmdSetScissor);
    YF_DPROCVK(device, vkCmdSetDepthBounds);
    YF_DPROCVK(device, vkCmdSetStencilCompareMask);
    YF_DPROCVK(device, vkCmdSetStencilWriteMask);
    YF_DPROCVK(device, vkCmdSetStencilReference);
    YF_DPROCVK(device, vkCmdSetBlendConstants);
    YF_DPROCVK(device, vkCmdDispatch);
    YF_DPROCVK(device, vkCmdDispatchIndirect);
    YF_DPROCVK(device, vkCreateSwapchainKHR);
    YF_DPROCVK(device, vkDestroySwapchainKHR);
    YF_DPROCVK(device, vkGetSwapchainImagesKHR);
    YF_DPROCVK(device, vkAcquireNextImageKHR);
    YF_DPROCVK(device, vkQueuePresentKHR);

    return 0;
}

/*
 * Instance-level proc. (null instance)
 */
YF_DEFVK(vkGetInstanceProcAddr);
YF_DEFVK(vkEnumerateInstanceVersion); /* 1.1 */
YF_DEFVK(vkEnumerateInstanceExtensionProperties);
YF_DEFVK(vkEnumerateInstanceLayerProperties);
YF_DEFVK(vkCreateInstance);

/*
 * Instance-level proc. (non-null instance)
 */
YF_DEFVK(vkDestroyInstance);
YF_DEFVK(vkEnumeratePhysicalDevices);
YF_DEFVK(vkGetPhysicalDeviceProperties);
YF_DEFVK(vkGetPhysicalDeviceQueueFamilyProperties);
YF_DEFVK(vkGetPhysicalDeviceMemoryProperties);
YF_DEFVK(vkGetPhysicalDeviceFormatProperties);
YF_DEFVK(vkGetPhysicalDeviceImageFormatProperties);
YF_DEFVK(vkGetPhysicalDeviceFeatures);
YF_DEFVK(vkGetPhysicalDeviceProperties2); /* 1.1 */
YF_DEFVK(vkGetPhysicalDeviceQueueFamilyProperties2); /* 1.1 */
YF_DEFVK(vkGetPhysicalDeviceMemoryProperties2); /* 1.1 */
YF_DEFVK(vkGetPhysicalDeviceFormatProperties2); /* 1.1 */
YF_DEFVK(vkGetPhysicalDeviceFeatures2); /* 1.1 */
YF_DEFVK(vkEnumerateDeviceExtensionProperties);
YF_DEFVK(vkCreateDevice);
YF_DEFVK(vkDestroySurfaceKHR); /* VK_KHR_surface */
YF_DEFVK(vkGetPhysicalDeviceSurfaceSupportKHR); /* VK_KHR_surface */
YF_DEFVK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR); /* VK_KHR_surface */
YF_DEFVK(vkGetPhysicalDeviceSurfaceFormatsKHR); /* VK_KHR_surface */
YF_DEFVK(vkGetPhysicalDeviceSurfacePresentModesKHR); /* VK_KHR_surface */
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
YF_DEFVK(vkCreateWaylandSurfaceKHR); /* VK_KHR_wayland_surface */
YF_DEFVK(vkGetPhysicalDeviceWaylandPresentationSupportKHR); /* VK_KHR_wayland_surface */
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
YF_DEFVK(vkCreateWin32SurfaceKHR); /* VK_KHR_win32_surface */
YF_DEFVK(vkGetPhysicalDeviceWin32PresentationSupportKHR); /* VK_KHR_win32_surface */
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
YF_DEFVK(vkCreateXcbSurfaceKHR); /* VK_KHR_xcb_surface */
YF_DEFVK(vkGetPhysicalDeviceXcbPresentationSupportKHR); /* VK_KHR_xcb_surface */
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
YF_DEFVK(vkCreateMetalSurfaceEXT); /* VK_EXT_metal_surface */
#endif

/*
 * Device-level proc.
 */
YF_DEFVK(vkGetDeviceProcAddr);
YF_DEFVK(vkDestroyDevice);
YF_DEFVK(vkGetDeviceQueue);
YF_DEFVK(vkCreateCommandPool);
YF_DEFVK(vkTrimCommandPool); /* 1.1 */
YF_DEFVK(vkResetCommandPool);
YF_DEFVK(vkDestroyCommandPool);
YF_DEFVK(vkAllocateCommandBuffers);
YF_DEFVK(vkResetCommandBuffer);
YF_DEFVK(vkFreeCommandBuffers);
YF_DEFVK(vkBeginCommandBuffer);
YF_DEFVK(vkEndCommandBuffer);
YF_DEFVK(vkQueueSubmit);
YF_DEFVK(vkCmdExecuteCommands);
YF_DEFVK(vkCreateFence);
YF_DEFVK(vkDestroyFence);
YF_DEFVK(vkGetFenceStatus);
YF_DEFVK(vkResetFences);
YF_DEFVK(vkWaitForFences);
YF_DEFVK(vkCreateSemaphore);
YF_DEFVK(vkDestroySemaphore);
YF_DEFVK(vkGetSemaphoreCounterValue); /* 1.2 */
YF_DEFVK(vkWaitSemaphores); /* 1.2 */
YF_DEFVK(vkSignalSemaphore); /* 1.2 */
YF_DEFVK(vkCmdPipelineBarrier);
YF_DEFVK(vkQueueWaitIdle);
YF_DEFVK(vkDeviceWaitIdle);
YF_DEFVK(vkCreateRenderPass);
YF_DEFVK(vkDestroyRenderPass);
YF_DEFVK(vkCreateFramebuffer);
YF_DEFVK(vkDestroyFramebuffer);
YF_DEFVK(vkCmdBeginRenderPass);
YF_DEFVK(vkCmdEndRenderPass);
YF_DEFVK(vkCmdNextSubpass);
YF_DEFVK(vkCreateShaderModule);
YF_DEFVK(vkDestroyShaderModule);
YF_DEFVK(vkCreateGraphicsPipelines);
YF_DEFVK(vkCreateComputePipelines);
YF_DEFVK(vkDestroyPipeline);
YF_DEFVK(vkCmdBindPipeline);
YF_DEFVK(vkCreatePipelineCache);
YF_DEFVK(vkMergePipelineCaches);
YF_DEFVK(vkGetPipelineCacheData);
YF_DEFVK(vkDestroyPipelineCache);
YF_DEFVK(vkAllocateMemory);
YF_DEFVK(vkFreeMemory);
YF_DEFVK(vkMapMemory);
YF_DEFVK(vkUnmapMemory);
YF_DEFVK(vkCreateBuffer);
YF_DEFVK(vkDestroyBuffer);
YF_DEFVK(vkCreateBufferView);
YF_DEFVK(vkDestroyBufferView);
YF_DEFVK(vkCreateImage);
YF_DEFVK(vkDestroyImage);
YF_DEFVK(vkGetImageSubresourceLayout);
YF_DEFVK(vkCreateImageView);
YF_DEFVK(vkDestroyImageView);
YF_DEFVK(vkGetBufferMemoryRequirements);
YF_DEFVK(vkGetImageMemoryRequirements);
YF_DEFVK(vkBindBufferMemory);
YF_DEFVK(vkBindImageMemory);
YF_DEFVK(vkCreateSampler);
YF_DEFVK(vkDestroySampler);
YF_DEFVK(vkCreateDescriptorSetLayout);
YF_DEFVK(vkDestroyDescriptorSetLayout);
YF_DEFVK(vkGetDescriptorSetLayoutSupport); /* 1.1 */
YF_DEFVK(vkCreatePipelineLayout);
YF_DEFVK(vkDestroyPipelineLayout);
YF_DEFVK(vkCreateDescriptorPool);
YF_DEFVK(vkDestroyDescriptorPool);
YF_DEFVK(vkResetDescriptorPool);
YF_DEFVK(vkAllocateDescriptorSets);
YF_DEFVK(vkFreeDescriptorSets);
YF_DEFVK(vkUpdateDescriptorSets);
YF_DEFVK(vkCmdBindDescriptorSets);
YF_DEFVK(vkCmdPushConstants);
YF_DEFVK(vkCmdClearColorImage);
YF_DEFVK(vkCmdClearDepthStencilImage);
YF_DEFVK(vkCmdClearAttachments);
YF_DEFVK(vkCmdFillBuffer);
YF_DEFVK(vkCmdUpdateBuffer);
YF_DEFVK(vkCmdCopyBuffer);
YF_DEFVK(vkCmdCopyImage);
YF_DEFVK(vkCmdCopyBufferToImage);
YF_DEFVK(vkCmdCopyImageToBuffer);
YF_DEFVK(vkCmdBlitImage);
YF_DEFVK(vkCmdResolveImage);
YF_DEFVK(vkCmdDraw);
YF_DEFVK(vkCmdDrawIndexed);
YF_DEFVK(vkCmdDrawIndirect);
YF_DEFVK(vkCmdDrawIndexedIndirect);
YF_DEFVK(vkCmdBindVertexBuffers);
YF_DEFVK(vkCmdBindIndexBuffer);
YF_DEFVK(vkCmdSetViewport);
YF_DEFVK(vkCmdSetLineWidth);
YF_DEFVK(vkCmdSetDepthBias);
YF_DEFVK(vkCmdSetScissor);
YF_DEFVK(vkCmdSetDepthBounds);
YF_DEFVK(vkCmdSetStencilCompareMask);
YF_DEFVK(vkCmdSetStencilWriteMask);
YF_DEFVK(vkCmdSetStencilReference);
YF_DEFVK(vkCmdSetBlendConstants);
YF_DEFVK(vkCmdDispatch);
YF_DEFVK(vkCmdDispatchIndirect);
YF_DEFVK(vkCreateSwapchainKHR); /* VK_KHR_swapchain */
YF_DEFVK(vkDestroySwapchainKHR); /* VK_KHR_swapchain */
YF_DEFVK(vkGetSwapchainImagesKHR); /* VK_KHR_swapchain */
YF_DEFVK(vkAcquireNextImageKHR); /* VK_KHR_swapchain */
YF_DEFVK(vkQueuePresentKHR); /* VK_KHR_swapchain */
