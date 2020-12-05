/*
 * YF
 * sampler.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include "sampler.h"
#include "context.h"
#include "error.h"

VkSampler yf_sampler_make(YF_context ctx, int sampler) {
  VkSamplerCreateInfo info = {0};
  info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  info.pNext = NULL;
  info.flags = 0;
  switch (sampler) {
    case YF_SAMPLER_BASIC:
      info.magFilter = VK_FILTER_NEAREST;
      info.minFilter = VK_FILTER_NEAREST;
      info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
      info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
      info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
      info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
      info.mipLodBias = 0.0f;
      info.anisotropyEnable = VK_FALSE;
      info.maxAnisotropy = 0.0f;
      info.compareEnable = VK_FALSE;
      info.compareOp = VK_COMPARE_OP_NEVER;
      info.minLod = 0.0f;
      info.maxLod = 0.0f;
      info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
      info.unnormalizedCoordinates = VK_FALSE;
      break;
    default:
      yf_seterr(YF_ERR_INVARG, __func__);
      return NULL;
  }
  VkSampler samp;
  VkResult res = vkCreateSampler(ctx->device, &info, NULL, &samp);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return NULL;
  }
  return samp;
}
