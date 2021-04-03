/*
 * YF
 * sampler.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <limits.h>
#include <assert.h>

#include "yf/com/yf-error.h"

#include "sampler.h"
#include "context.h"

VkSampler yf_sampler_make(YF_context ctx, const YF_sampler *samp) {
  assert(ctx != NULL);

  VkSamplerCreateInfo info = {0};
  info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  info.pNext = NULL;
  info.flags = 0;
  /* TODO: Additional members for setting some of these parameters. */
  info.mipLodBias = 0.0f;
  info.anisotropyEnable = VK_FALSE;
  info.maxAnisotropy = 0.0f;
  info.compareEnable = VK_FALSE;
  info.compareOp = VK_COMPARE_OP_NEVER;
  info.minLod = 0.0f;
  info.maxLod = 0.0f;
  info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
  info.unnormalizedCoordinates = VK_FALSE;

  if (samp != NULL) {
    YF_WRAPMODE_FROM(samp->wrapmode.u, info.addressModeU);
    YF_WRAPMODE_FROM(samp->wrapmode.v, info.addressModeV);
    YF_WRAPMODE_FROM(samp->wrapmode.w, info.addressModeW);
    YF_FILTER_FROM(samp->filter.mag, info.magFilter);
    YF_FILTER_FROM(samp->filter.min, info.minFilter);
    YF_FILTER_MIP_FROM(samp->filter.mipmap, info.mipmapMode);

    assert(info.addressModeU != INT_MAX);
    assert(info.addressModeV != INT_MAX);
    assert(info.addressModeW != INT_MAX);
    assert(info.magFilter != INT_MAX);
    assert(info.minFilter != INT_MAX);
    assert(info.mipmapMode != INT_MAX);

  } else {
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.magFilter = VK_FILTER_NEAREST;
    info.minFilter = VK_FILTER_NEAREST;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  }

  VkSampler sampler;
  VkResult res = vkCreateSampler(ctx->device, &info, NULL, &sampler);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return NULL;
  }
  return sampler;
}
