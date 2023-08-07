/*
 * YF
 * vk.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_VK_H
#define YF_VK_H

#if defined(__linux__)
# define VK_USE_PLATFORM_WAYLAND_KHR
# define VK_USE_PLATFORM_XCB_KHR
#elif defined(__APPLE__)
# define VK_USE_PLATFORM_METAL_EXT
#elif defined(_WIN32)
# define VK_USE_PLATFORM_WIN32_KHR
#else
# error "Invalid platform"
#endif /* defined(__linux__) */
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include "yf/com/yf-defs.h"

/* Loads vk lib. */
int yf_loadvk(void);

/* Unloads vk lib. */
void yf_unldvk(void);

/* Sets instance-level function pointers. */
int yf_setiprocvk(VkInstance instance);

/* Sets device-level function pointers. */
int yf_setdprocvk(VkDevice device);

#define YF_DECLVK(name) extern YF_HIDDEN PFN_##name name
#define YF_DEFVK(name)  PFN_##name name

#define YF_IPROCVK(inst, name) \
    name = (PFN_##name)vkGetInstanceProcAddr(inst, #name)
#define YF_DPROCVK(dev, name) \
    name = (PFN_##name)vkGetDeviceProcAddr(dev, #name)

/*
 * Instance-level proc. (null instance)
 */
YF_DECLVK(vkGetInstanceProcAddr);
YF_DECLVK(vkEnumerateInstanceVersion); /* 1.1 */
YF_DECLVK(vkEnumerateInstanceExtensionProperties);
YF_DECLVK(vkEnumerateInstanceLayerProperties);
YF_DECLVK(vkCreateInstance);

/*
 * Instance-level proc. (non-null instance)
 */
YF_DECLVK(vkDestroyInstance);
YF_DECLVK(vkEnumeratePhysicalDevices);
YF_DECLVK(vkGetPhysicalDeviceProperties);
YF_DECLVK(vkGetPhysicalDeviceQueueFamilyProperties);
YF_DECLVK(vkGetPhysicalDeviceMemoryProperties);
YF_DECLVK(vkGetPhysicalDeviceFormatProperties);
YF_DECLVK(vkGetPhysicalDeviceImageFormatProperties);
YF_DECLVK(vkGetPhysicalDeviceFeatures);
YF_DECLVK(vkGetPhysicalDeviceProperties2); /* 1.1 */
YF_DECLVK(vkGetPhysicalDeviceQueueFamilyProperties2); /* 1.1 */
YF_DECLVK(vkGetPhysicalDeviceMemoryProperties2); /* 1.1 */
YF_DECLVK(vkGetPhysicalDeviceFormatProperties2); /* 1.1 */
YF_DECLVK(vkGetPhysicalDeviceFeatures2); /* 1.1 */
YF_DECLVK(vkEnumerateDeviceExtensionProperties);
YF_DECLVK(vkCreateDevice);
YF_DECLVK(vkDestroySurfaceKHR); /* VK_KHR_surface */
YF_DECLVK(vkGetPhysicalDeviceSurfaceSupportKHR); /* VK_KHR_surface */
YF_DECLVK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR); /* VK_KHR_surface */
YF_DECLVK(vkGetPhysicalDeviceSurfaceFormatsKHR); /* VK_KHR_surface */
YF_DECLVK(vkGetPhysicalDeviceSurfacePresentModesKHR); /* VK_KHR_surface */
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
YF_DECLVK(vkCreateWaylandSurfaceKHR); /* VK_KHR_wayland_surface */
YF_DECLVK(vkGetPhysicalDeviceWaylandPresentationSupportKHR); /* VK_KHR_wayland_surface */
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
YF_DECLVK(vkCreateWin32SurfaceKHR); /* VK_KHR_win32_surface */
YF_DECLVK(vkGetPhysicalDeviceWin32PresentationSupportKHR); /* VK_KHR_win32_surface */
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
YF_DECLVK(vkCreateXcbSurfaceKHR); /* VK_KHR_xcb_surface */
YF_DECLVK(vkGetPhysicalDeviceXcbPresentationSupportKHR); /* VK_KHR_xcb_surface */
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
YF_DECLVK(vkCreateMetalSurfaceEXT); /* VK_EXT_metal_surface */
#endif

/*
 * Device-level proc.
 */
YF_DECLVK(vkGetDeviceProcAddr);
YF_DECLVK(vkDestroyDevice);
YF_DECLVK(vkGetDeviceQueue);
YF_DECLVK(vkCreateCommandPool);
YF_DECLVK(vkTrimCommandPool); /* 1.1 */
YF_DECLVK(vkResetCommandPool);
YF_DECLVK(vkDestroyCommandPool);
YF_DECLVK(vkAllocateCommandBuffers);
YF_DECLVK(vkResetCommandBuffer);
YF_DECLVK(vkFreeCommandBuffers);
YF_DECLVK(vkBeginCommandBuffer);
YF_DECLVK(vkEndCommandBuffer);
YF_DECLVK(vkQueueSubmit);
YF_DECLVK(vkCmdExecuteCommands);
YF_DECLVK(vkCreateFence);
YF_DECLVK(vkDestroyFence);
YF_DECLVK(vkGetFenceStatus);
YF_DECLVK(vkResetFences);
YF_DECLVK(vkWaitForFences);
YF_DECLVK(vkCreateSemaphore);
YF_DECLVK(vkDestroySemaphore);
YF_DECLVK(vkGetSemaphoreCounterValue); /* 1.2 */
YF_DECLVK(vkWaitSemaphores); /* 1.2 */
YF_DECLVK(vkSignalSemaphore); /* 1.2 */
YF_DECLVK(vkCmdPipelineBarrier);
YF_DECLVK(vkQueueWaitIdle);
YF_DECLVK(vkDeviceWaitIdle);
YF_DECLVK(vkCreateRenderPass);
YF_DECLVK(vkDestroyRenderPass);
YF_DECLVK(vkCreateFramebuffer);
YF_DECLVK(vkDestroyFramebuffer);
YF_DECLVK(vkCmdBeginRenderPass);
YF_DECLVK(vkCmdEndRenderPass);
YF_DECLVK(vkCmdNextSubpass);
YF_DECLVK(vkCreateShaderModule);
YF_DECLVK(vkDestroyShaderModule);
YF_DECLVK(vkCreateGraphicsPipelines);
YF_DECLVK(vkCreateComputePipelines);
YF_DECLVK(vkDestroyPipeline);
YF_DECLVK(vkCmdBindPipeline);
YF_DECLVK(vkCreatePipelineCache);
YF_DECLVK(vkMergePipelineCaches);
YF_DECLVK(vkGetPipelineCacheData);
YF_DECLVK(vkDestroyPipelineCache);
YF_DECLVK(vkAllocateMemory);
YF_DECLVK(vkFreeMemory);
YF_DECLVK(vkMapMemory);
YF_DECLVK(vkUnmapMemory);
YF_DECLVK(vkCreateBuffer);
YF_DECLVK(vkDestroyBuffer);
YF_DECLVK(vkCreateBufferView);
YF_DECLVK(vkDestroyBufferView);
YF_DECLVK(vkCreateImage);
YF_DECLVK(vkDestroyImage);
YF_DECLVK(vkGetImageSubresourceLayout);
YF_DECLVK(vkCreateImageView);
YF_DECLVK(vkDestroyImageView);
YF_DECLVK(vkGetBufferMemoryRequirements);
YF_DECLVK(vkGetImageMemoryRequirements);
YF_DECLVK(vkBindBufferMemory);
YF_DECLVK(vkBindImageMemory);
YF_DECLVK(vkCreateSampler);
YF_DECLVK(vkDestroySampler);
YF_DECLVK(vkCreateDescriptorSetLayout);
YF_DECLVK(vkDestroyDescriptorSetLayout);
YF_DECLVK(vkGetDescriptorSetLayoutSupport); /* 1.1 */
YF_DECLVK(vkCreatePipelineLayout);
YF_DECLVK(vkDestroyPipelineLayout);
YF_DECLVK(vkCreateDescriptorPool);
YF_DECLVK(vkDestroyDescriptorPool);
YF_DECLVK(vkResetDescriptorPool);
YF_DECLVK(vkAllocateDescriptorSets);
YF_DECLVK(vkFreeDescriptorSets);
YF_DECLVK(vkUpdateDescriptorSets);
YF_DECLVK(vkCmdBindDescriptorSets);
YF_DECLVK(vkCmdPushConstants);
YF_DECLVK(vkCmdClearColorImage);
YF_DECLVK(vkCmdClearDepthStencilImage);
YF_DECLVK(vkCmdClearAttachments);
YF_DECLVK(vkCmdFillBuffer);
YF_DECLVK(vkCmdUpdateBuffer);
YF_DECLVK(vkCmdCopyBuffer);
YF_DECLVK(vkCmdCopyImage);
YF_DECLVK(vkCmdCopyBufferToImage);
YF_DECLVK(vkCmdCopyImageToBuffer);
YF_DECLVK(vkCmdBlitImage);
YF_DECLVK(vkCmdResolveImage);
YF_DECLVK(vkCmdDraw);
YF_DECLVK(vkCmdDrawIndexed);
YF_DECLVK(vkCmdDrawIndirect);
YF_DECLVK(vkCmdDrawIndexedIndirect);
YF_DECLVK(vkCmdBindVertexBuffers);
YF_DECLVK(vkCmdBindIndexBuffer);
YF_DECLVK(vkCmdSetViewport);
YF_DECLVK(vkCmdSetLineWidth);
YF_DECLVK(vkCmdSetDepthBias);
YF_DECLVK(vkCmdSetScissor);
YF_DECLVK(vkCmdSetDepthBounds);
YF_DECLVK(vkCmdSetStencilCompareMask);
YF_DECLVK(vkCmdSetStencilWriteMask);
YF_DECLVK(vkCmdSetStencilReference);
YF_DECLVK(vkCmdSetBlendConstants);
YF_DECLVK(vkCmdDispatch);
YF_DECLVK(vkCmdDispatchIndirect);
YF_DECLVK(vkCreateSwapchainKHR); /* VK_KHR_swapchain */
YF_DECLVK(vkDestroySwapchainKHR); /* VK_KHR_swapchain */
YF_DECLVK(vkGetSwapchainImagesKHR); /* VK_KHR_swapchain */
YF_DECLVK(vkAcquireNextImageKHR); /* VK_KHR_swapchain */
YF_DECLVK(vkQueuePresentKHR); /* VK_KHR_swapchain */

#endif /* YF_VK_H */
