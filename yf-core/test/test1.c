/*
 * YF
 * test1.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "context.h"
#include "list.h"
#include "hashset.h"
#include "test.h"

/* Context instance. */
static YF_context l_ctx = NULL;

/* Prints device limits. */
static int print_limits(void);

/* Prints available features. */
static int print_features(void);

/* Iteration callback. */
static int do_each(void *val, void *arg);

/* Tests linked list functionality. */
static int test_list(void);

/* Tests hashset functionality. */
static int test_hset(void);

int yf_test_1(void) {
  int r = -1;
  l_ctx = yf_context_init();
  if (l_ctx != NULL)  {
    YF_CTX_PRINT(l_ctx);
    if ((r = print_limits()) != 0) {}
    else if ((r = print_features()) != 0) {}
    else if ((r = test_list()) != 0) {}
    else if ((r = test_hset()) != 0) {}
    yf_context_deinit(l_ctx);
  }
  return r;
}

static int print_limits() {
  VkPhysicalDeviceLimits *lim = &l_ctx->dev_prop.limits;
  puts("=limits=");
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
  puts("----");
  return 0;
}

static int print_features(void) {
  VkPhysicalDeviceFeatures feat;
  vkGetPhysicalDeviceFeatures(l_ctx->phy_dev, &feat);
  const char *av[2] = {"NO", "YES"};
  puts("=Features=");
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
  puts("----");
  return 0;
}

static int do_each(void *val, void *arg) {
  printf("%s %s\n", (char *)arg, (char *)val);
  if (((char *)val)[0] == '!')
    return -1;
  return 0;
}

static int test_list(void) {
  char *s1 = "one two three";
  char *s2 = "!abc";
  char *s3 = "xyz";
  YF_list list = yf_list_init(NULL);
  puts("=list=");
  printf(
    "insert: %d, %d, %d\n",
    yf_list_insert(list, s1),
    yf_list_insert(list, s2),
    yf_list_insert(list, s3));
  printf("getlen: %lu\n", yf_list_getlen(list));
  printf("contains %d\n", yf_list_contains(list, s2));
  printf("remove: %d\n", yf_list_remove(list, s2));
  printf("getlen: %lu\n", yf_list_getlen(list));
  printf("contains: %d\n", yf_list_contains(list, s2));
  YF_iter it = YF_NILIT;
  do {
    char *s = yf_list_next(list, &it);
    if (YF_IT_ISNIL(it))
      break;
    printf("(it) %s\n", s);
  } while (1);
  yf_list_each(list, do_each, "<each>");
  printf("getlen: %lu\n", yf_list_getlen(list));
  printf("clear\n");
  yf_list_clear(list);
  printf("getlen: %lu\n", yf_list_getlen(list));
  yf_list_deinit(list);
  puts("----");
  return 0;
}

static int test_hset(void) {
  char *s1 = "??";
  char *s2 = "Q";
  char *s3 = "!the end!";
  char *s4 = "~~~";
  YF_hashset set = yf_hashset_init(NULL, NULL);
  assert(set);
  puts("=hset=");
  printf(
    "insert: %d, %d, %d, %d\n",
    yf_hashset_insert(set, s1),
    yf_hashset_insert(set, s2),
    yf_hashset_insert(set, s3),
    yf_hashset_insert(set, s4));
  printf("getlen: %lu\n", yf_hashset_getlen(set));
  printf("contains %d\n", yf_hashset_contains(set, s1));
  printf("remove: %d\n", yf_hashset_remove(set, s1));
  printf("getlen: %lu\n", yf_hashset_getlen(set));
  printf("contains: %d\n", yf_hashset_contains(set, s1));
  YF_iter it = YF_NILIT;
  do {
    char *s = yf_hashset_next(set, &it);
    if (YF_IT_ISNIL(it))
      break;
    printf("(it) %s\n", s);
  } while (1);
  yf_hashset_each(set, do_each, "<each>");
  printf("getlen: %lu\n", yf_hashset_getlen(set));
  printf("clear\n");
  yf_hashset_clear(set);
  printf("getlen: %lu\n", yf_hashset_getlen(set));
  yf_hashset_deinit(set);

  set = yf_hashset_init(NULL, NULL);
  assert(set != NULL);
  size_t vals[53];
  for (size_t i = 0; i < sizeof vals / sizeof vals[0]; ++i) {
    vals[i] = i;
    yf_hashset_insert(set, vals+i);
  }
  //for (size_t i = sizeof vals / sizeof vals[0]; i > 1; --i)
    //yf_hashset_remove(set, vals+i-1);
  it = YF_NILIT;
  do {
    size_t *val = yf_hashset_next(set, &it);
    if (YF_IT_ISNIL(it))
      break;
    printf("(it): %lu\n", *val);
  } while (1);
  yf_hashset_deinit(set);

  set = yf_hashset_init(NULL, NULL);
  assert(set != NULL);
  for (int v = -5; v < 5; ++v)
    yf_hashset_insert(set, (void *)v);
  printf("getlen: %lu\n", yf_hashset_getlen(set));
  it = YF_NILIT;
  do {
    void *v = NULL;
    if ((v = yf_hashset_next(set, &it)) == (void *)2) {
      printf("extract next: %d\n", (int)yf_hashset_extract(set, &it));
      break;
    }
    printf("skip next: %d\n", (int)v);
  } while (1);
  printf("getlen: %lu\n", yf_hashset_getlen(set));
  while (yf_hashset_getlen(set) > 0) {
    void *v = yf_hashset_extract(set, /*NULL*/&it);
    printf("extract: %d\n", (int)v);
    printf("getlen: %lu\n", yf_hashset_getlen(set));
  }

  puts("----");
  return 0;
}
