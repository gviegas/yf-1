/*
 * YF
 * test-capab.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "context.h"

/* Context instance. */
static YF_context l_ctx = NULL;

/* Prints extension-related info. */
static void print_extensions(void) {
  puts("\n=Instance exts=\n");
  for (;;) {
    VkExtensionProperties exts[100];
    unsigned ext_n = sizeof exts / sizeof exts[0];
    VkResult res = vkEnumerateInstanceExtensionProperties(NULL, &ext_n, exts);
    assert(res == VK_SUCCESS || res == VK_INCOMPLETE);
    for (size_t i = 0; i < ext_n; ++i)
      printf("%s\n", exts[i].extensionName);
    if (res == VK_SUCCESS)
      break;
  }

  puts("\n=Device exts=\n");
  for (;;) {
    VkExtensionProperties exts[100];
    unsigned ext_n = sizeof exts / sizeof exts[0];
    VkResult res = vkEnumerateDeviceExtensionProperties(l_ctx->phy_dev, NULL,
        &ext_n, exts);
    assert(res == VK_SUCCESS || res == VK_INCOMPLETE);
    for (size_t i = 0; i < ext_n; ++i)
      printf("%s\n", exts[i].extensionName);
    if (res == VK_SUCCESS)
      break;
  }

  puts("\n=Layers=\n");
  for (;;) {
    VkLayerProperties lays[100];
    unsigned lay_n = sizeof lays / sizeof lays[0];
    VkResult res;
    res = vkEnumerateInstanceLayerProperties(&lay_n, lays);
    assert(res == VK_SUCCESS || res == VK_INCOMPLETE);
    for (size_t i = 0; i < lay_n; ++i) {
      printf("%s\n? %s\n", lays[i].layerName, lays[i].description);
      for (;;) {
        VkExtensionProperties exts[100];
        unsigned ext_n = sizeof exts / sizeof exts[0];
        VkResult res = vkEnumerateInstanceExtensionProperties(
            lays[i].layerName, &ext_n, exts);
        assert(res == VK_SUCCESS || res == VK_INCOMPLETE);
        for (size_t i = 0; i < ext_n; ++i)
          printf("\t%s\n", exts[i].extensionName);
        if (res == VK_SUCCESS)
          break;
      }
      puts("");
    }
    if (res == VK_SUCCESS)
      break;
  }
}

/* Prints device features. */
static void print_features(void) {
  VkPhysicalDeviceFeatures feat;
  vkGetPhysicalDeviceFeatures(l_ctx->phy_dev, &feat);
  const char *av[2] = {"NO", "YES"};

  puts("\n=Features=\n");
  printf("robustBufferAccess: %s\n", av[feat.robustBufferAccess]);
  printf("fullDrawIndexUint32: %s\n", av[feat.fullDrawIndexUint32]);
  printf("imageCubeArray: %s\n", av[feat.imageCubeArray]);
  printf("independentBlend: %s\n", av[feat.independentBlend]);
  printf("geometryShader: %s\n", av[feat.geometryShader]);
  printf("tessellationShader: %s\n", av[feat.tessellationShader]);
  printf("sampleRateShading: %s\n", av[feat.sampleRateShading]);
  printf("dualSrcBlend: %s\n", av[feat.dualSrcBlend]);
  printf("logicOp: %s\n", av[feat.logicOp]);
  printf("multiDrawIndirect: %s\n", av[feat.multiDrawIndirect]);
  printf("drawIndirectFirstInstance: %s\n", av[feat.drawIndirectFirstInstance]);
  printf("depthClamp: %s\n", av[feat.depthClamp]);
  printf("depthBiasClamp: %s\n", av[feat.depthBiasClamp]);
  printf("fillModeNonSolid: %s\n", av[feat.fillModeNonSolid]);
  printf("depthBounds: %s\n", av[feat.depthBounds]);
  printf("wideLines: %s\n", av[feat.wideLines]);
  printf("largePoints: %s\n", av[feat.largePoints]);
  printf("alphaToOne: %s\n", av[feat.alphaToOne]);
  printf("multiViewport: %s\n", av[feat.multiViewport]);
  printf("samplerAnisotropy: %s\n", av[feat.samplerAnisotropy]);
  printf("textureCompressionETC2: %s\n", av[feat.textureCompressionETC2]);
  printf("textureCompressionASTC_LDR: %s\n",
    av[feat.textureCompressionASTC_LDR]);
  printf("textureCompressionBC: %s\n", av[feat.textureCompressionBC]);
  printf("occlusionQueryPrecise: %s\n", av[feat.occlusionQueryPrecise]);
  printf("pipelineStatisticsQuery: %s\n", av[feat.pipelineStatisticsQuery]);
  printf("vertexPipelineStoresAndAtomics: %s\n",
    av[feat.vertexPipelineStoresAndAtomics]);
  printf("fragmentStoresAndAtomics: %s\n", av[feat.fragmentStoresAndAtomics]);
  printf("shaderTessellationAndGeometryPointSize: %s\n",
    av[feat.shaderTessellationAndGeometryPointSize]);
  printf("shaderImageGatherExtended: %s\n", av[feat.shaderImageGatherExtended]);
  printf("shaderStorageImageExtendedFormats: %s\n",
    av[feat.shaderStorageImageExtendedFormats]);
  printf("shaderStorageImageMultisample: %s\n",
    av[feat.shaderStorageImageMultisample]);
  printf("shaderStorageImageReadWithoutFormat: %s\n",
    av[feat.shaderStorageImageReadWithoutFormat]);
  printf("shaderStorageImageWriteWithoutFormat: %s\n",
    av[feat.shaderStorageImageWriteWithoutFormat]);
  printf("shaderUniformBufferArrayDynamicIndexing: %s\n",
    av[feat.shaderUniformBufferArrayDynamicIndexing]);
  printf("shaderSampledImageArrayDynamicIndexing: %s\n",
    av[feat.shaderSampledImageArrayDynamicIndexing]);
  printf("shaderStorageBufferArrayDynamicIndexing: %s\n",
    av[feat.shaderStorageBufferArrayDynamicIndexing]);
  printf("shaderStorageImageArrayDynamicIndexing: %s\n",
    av[feat.shaderStorageImageArrayDynamicIndexing]);
  printf("shaderClipDistance: %s\n", av[feat.shaderClipDistance]);
  printf("shaderCullDistance: %s\n", av[feat.shaderCullDistance]);
  printf("shaderFloat64: %s\n", av[feat.shaderFloat64]);
  printf("shaderInt64: %s\n", av[feat.shaderInt64]);
  printf("shaderInt16: %s\n", av[feat.shaderInt16]);
  printf("shaderResourceResidency: %s\n", av[feat.shaderResourceResidency]);
  printf("shaderResourceMinLod: %s\n", av[feat.shaderResourceMinLod]);
  printf("sparseBinding: %s\n", av[feat.sparseBinding]);
  printf("sparseResidencyBuffer: %s\n", av[feat.sparseResidencyBuffer]);
  printf("sparseResidencyImage2D: %s\n", av[feat.sparseResidencyImage2D]);
  printf("sparseResidencyImage3D: %s\n", av[feat.sparseResidencyImage3D]);
  printf("sparseResidency2Samples: %s\n", av[feat.sparseResidency2Samples]);
  printf("sparseResidency4Samples: %s\n", av[feat.sparseResidency4Samples]);
  printf("sparseResidency8Samples: %s\n", av[feat.sparseResidency8Samples]);
  printf("sparseResidency16Samples: %s\n", av[feat.sparseResidency16Samples]);
  printf("sparseResidencyAliased: %s\n", av[feat.sparseResidencyAliased]);
  printf("variableMultisampleRate: %s\n", av[feat.variableMultisampleRate]);
  printf("inheritedQueries: %s\n", av[feat.inheritedQueries]);
}

/* Prints device limits. */
static void print_limits() {
  VkPhysicalDeviceLimits *lim = &l_ctx->dev_prop.limits;

  puts("\n=limits=\n");
  printf("maxImageDimension1D: %u\n", lim->maxImageDimension1D);
  printf("maxImageDimension2D: %u\n", lim->maxImageDimension2D);
  printf("maxImageDimension3D: %u\n", lim->maxImageDimension3D);
  printf("maxImageDimensionCube: %u\n", lim->maxImageDimensionCube);
  printf("maxImageArrayLayers: %u\n", lim->maxImageArrayLayers);
  printf("maxTexelBufferElements: %u\n", lim->maxTexelBufferElements);
  printf("maxUniformBufferRange: %u\n", lim->maxUniformBufferRange);
  printf("maxStorageBufferRange: %u\n", lim->maxStorageBufferRange);
  printf("maxPushConstantsSize: %u\n", lim->maxPushConstantsSize);
  printf("maxMemoryAllocationCount: %u\n", lim->maxMemoryAllocationCount);
  printf("maxSamplerAllocationCount: %u\n", lim->maxSamplerAllocationCount);
  printf("bufferImageGranularity: %lu\n", lim->bufferImageGranularity);
  printf("sparseAddressSpaceSize: %lu\n", lim->sparseAddressSpaceSize);
  printf("maxBoundDescriptorSets: %u\n", lim->maxBoundDescriptorSets);
  printf("maxPerStageDescriptorSamplers: %u\n",
    lim->maxPerStageDescriptorSamplers);
  printf("maxPerStageDescriptorUniformBuffers: %u\n",
    lim->maxPerStageDescriptorUniformBuffers);
  printf("maxPerStageDescriptorStorageBuffers: %u\n",
    lim->maxPerStageDescriptorStorageBuffers);
  printf("maxPerStageDescriptorSampledImages: %u\n",
    lim->maxPerStageDescriptorSampledImages);
  printf("maxPerStageDescriptorStorageImages: %u\n",
    lim->maxPerStageDescriptorStorageImages);
  printf("maxPerStageDescriptorInputAttachments: %u\n",
    lim->maxPerStageDescriptorInputAttachments);
  printf("maxPerStageResources: %u\n", lim->maxPerStageResources);
  printf("maxDescriptorSetSamplers: %u\n", lim->maxDescriptorSetSamplers);
  printf("maxDescriptorSetUniformBuffers: %u\n",
    lim->maxDescriptorSetUniformBuffers);
  printf("maxDescriptorSetUniformBuffersDynamic: %u\n",
    lim->maxDescriptorSetUniformBuffersDynamic);
  printf("maxDescriptorSetStorageBuffers: %u\n",
    lim->maxDescriptorSetStorageBuffers);
  printf("maxDescriptorSetStorageBuffersDynamic: %u\n",
    lim->maxDescriptorSetStorageBuffersDynamic);
  printf("maxDescriptorSetSampledImages: %u\n",
    lim->maxDescriptorSetSampledImages);
  printf("maxDescriptorSetStorageImages: %u\n",
    lim->maxDescriptorSetStorageImages);
  printf("maxDescriptorSetInputAttachments: %u\n",
    lim->maxDescriptorSetInputAttachments);
  printf("maxVertexInputAttributes: %u\n", lim->maxVertexInputAttributes);
  printf("maxVertexInputBindings: %u\n", lim->maxVertexInputBindings);
  printf("maxVertexInputAttributeOffset: %u\n",
    lim->maxVertexInputAttributeOffset);
  printf("maxVertexInputBindingStride: %u\n", lim->maxVertexInputBindingStride);
  printf("maxVertexOutputComponents: %u\n", lim->maxVertexOutputComponents);
  printf("maxTessellationGenerationLevel: %u\n",
    lim->maxTessellationGenerationLevel);
  printf("maxTessellationPatchSize: %u\n", lim->maxTessellationPatchSize);
  printf("maxTessellationControlPerVertexInputComponents: %u\n",
    lim->maxTessellationControlPerVertexInputComponents);
  printf("maxTessellationControlPerVertexOutputComponents: %u\n",
    lim->maxTessellationControlPerVertexOutputComponents);
  printf("maxTessellationControlPerPatchOutputComponents: %u\n",
    lim->maxTessellationControlPerPatchOutputComponents);
  printf("maxTessellationControlTotalOutputComponents: %u\n",
    lim->maxTessellationControlTotalOutputComponents);
  printf("maxTessellationEvaluationInputComponents: %u\n",
    lim->maxTessellationEvaluationInputComponents);
  printf("maxTessellationEvaluationOutputComponents: %u\n",
    lim->maxTessellationEvaluationOutputComponents);
  printf("maxGeometryShaderInvocations: %u\n",
    lim->maxGeometryShaderInvocations);
  printf("maxGeometryInputComponents: %u\n", lim->maxGeometryInputComponents);
  printf("maxGeometryOutputComponents: %u\n", lim->maxGeometryOutputComponents);
  printf("maxGeometryOutputVertices: %u\n", lim->maxGeometryOutputVertices);
  printf("maxGeometryTotalOutputComponents: %u\n",
    lim->maxGeometryTotalOutputComponents);
  printf("maxFragmentInputComponents: %u\n", lim->maxFragmentInputComponents);
  printf("maxFragmentOutputAttachments: %u\n",
    lim->maxFragmentOutputAttachments);
  printf("maxFragmentDualSrcAttachments: %u\n",
    lim->maxFragmentDualSrcAttachments);
  printf("maxFragmentCombinedOutputResources: %u\n",
    lim->maxFragmentCombinedOutputResources);
  printf("maxComputeSharedMemorySize: %u\n", lim->maxComputeSharedMemorySize);
  printf("maxComputeWorkGroupCount: %u, %u, %u\n",
    lim->maxComputeWorkGroupCount[0],
    lim->maxComputeWorkGroupCount[1],
    lim->maxComputeWorkGroupCount[2]);
  printf("maxComputeWorkGroupInvocations: %u\n",
    lim->maxComputeWorkGroupInvocations);
  printf("maxComputeWorkGroupSize: %u, %u, %u\n",
    lim->maxComputeWorkGroupSize[0],
    lim->maxComputeWorkGroupSize[1],
    lim->maxComputeWorkGroupSize[2]);
  printf("subPixelPrecisionBits: %u\n", lim->subPixelPrecisionBits);
  printf("subTexelPrecisionBits: %u\n", lim->subTexelPrecisionBits);
  printf("mipmapPrecisionBits: %u\n", lim->mipmapPrecisionBits);
  printf("maxDrawIndexedIndexValue: %u\n", lim->maxDrawIndexedIndexValue);
  printf("maxDrawIndirectCount: %u\n", lim->maxDrawIndirectCount);
  printf("maxSamplerLodBias: %f\n", lim->maxSamplerLodBias);
  printf("maxSamplerAnisotropy: %f\n", lim->maxSamplerAnisotropy);
  printf("maxViewports: %u\n", lim->maxViewports);
  printf("maxViewportDimensions: %u, %u\n",
    lim->maxViewportDimensions[0], lim->maxViewportDimensions[1]);
  printf("viewportBoundsRange: %f, %f\n",
    lim->viewportBoundsRange[0], lim->viewportBoundsRange[1]);
  printf("viewportSubPixelBits: %u\n", lim->viewportSubPixelBits);
  printf("minMemoryMapAlignment: %lu\n", lim->minMemoryMapAlignment);
  printf("minTexelBufferOffsetAlignment: %lu\n",
    lim->minTexelBufferOffsetAlignment);
  printf("minUniformBufferOffsetAlignment: %lu\n",
    lim->minUniformBufferOffsetAlignment);
  printf("minStorageBufferOffsetAlignment: %lu\n",
    lim->minStorageBufferOffsetAlignment);
  printf("minTexelOffset: %d\n", lim->minTexelOffset);
  printf("maxTexelOffset: %u\n", lim->maxTexelOffset);
  printf("minTexelGatherOffset: %d\n", lim->minTexelGatherOffset);
  printf("maxTexelGatherOffset: %u\n", lim->maxTexelGatherOffset);
  printf("minInterpolationOffset: %f\n", lim->minInterpolationOffset);
  printf("maxInterpolationOffset: %f\n", lim->maxInterpolationOffset);
  printf("subPixelInterpolationOffsetBits: %u\n",
    lim->subPixelInterpolationOffsetBits);
  printf("maxFramebufferWidth: %u\n", lim->maxFramebufferWidth);
  printf("maxFramebufferHeight: %u\n", lim->maxFramebufferHeight);
  printf("maxFramebufferLayers: %u\n", lim->maxFramebufferLayers);
  printf("framebufferColorSampleCounts (flags): %x\n",
    lim->framebufferColorSampleCounts);
  printf("framebufferDepthSampleCounts (flags): %x\n",
    lim->framebufferDepthSampleCounts);
  printf("framebufferStencilSampleCounts (flags): %x\n",
    lim->framebufferStencilSampleCounts);
  printf("framebufferNoAttachmentsSampleCounts (flags): %x\n",
    lim->framebufferNoAttachmentsSampleCounts);
  printf("maxColorAttachments: %u\n", lim->maxColorAttachments);
  printf("sampledImageColorSampleCounts (flags): %x\n",
    lim->sampledImageColorSampleCounts);
  printf("sampledImageIntegerSampleCounts (flags): %x\n",
    lim->sampledImageIntegerSampleCounts);
  printf("sampledImageDepthSampleCounts (flags): %x\n",
    lim->sampledImageDepthSampleCounts);
  printf("sampledImageStencilSampleCounts (flags): %x\n",
    lim->sampledImageStencilSampleCounts);
  printf("storageImageSampleCounts (flags): %x\n",
    lim->storageImageSampleCounts);
  printf("maxSampleMaskWords: %u\n", lim->maxSampleMaskWords);
  printf("timestampComputeAndGraphics: %u\n", lim->timestampComputeAndGraphics);
  printf("timestampPeriod: %f\n", lim->timestampPeriod);
  printf("maxClipDistances: %u\n", lim->maxClipDistances);
  printf("maxCullDistances: %u\n", lim->maxCullDistances);
  printf("maxCombinedClipAndCullDistances: %u\n",
    lim->maxCombinedClipAndCullDistances);
  printf("discreteQueuePriorities: %u\n", lim->discreteQueuePriorities);
  printf("pointSizeRange: %f, %f\n",
    lim->pointSizeRange[0], lim->pointSizeRange[1]);
  printf("lineWidthRange: %f, %f\n",
    lim->lineWidthRange[0], lim->lineWidthRange[1]);
  printf("pointSizeGranularity: %f\n", lim->pointSizeGranularity);
  printf("lineWidthGranularity: %f\n", lim->lineWidthGranularity);
  printf("strictLines: %u\n", lim->strictLines);
  printf("standardSampleLocations: %u\n", lim->standardSampleLocations);
  printf("optimalBufferCopyOffsetAlignment: %lu\n",
    lim->optimalBufferCopyOffsetAlignment);
  printf("optimalBufferCopyRowPitchAlignment: %lu\n",
    lim->optimalBufferCopyRowPitchAlignment);
  printf("nonCoherentAtomSize: %lu\n", lim->nonCoherentAtomSize);
}

/* Called by the main test. */
int yf_test_capab(void) {
  l_ctx = yf_context_init();
  if (l_ctx == NULL)
    return -1;

  print_extensions();
  print_features();
  print_limits();

  yf_context_deinit(l_ctx);
  return 0;
}
