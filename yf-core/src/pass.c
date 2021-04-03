/*
 * YF
 * pass.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include "yf/com/yf-error.h"

#include "pass.h"
#include "context.h"
#include "limits.h"

#define YF_TGTN 4

YF_pass yf_pass_init(YF_context ctx, const YF_colordsc *colors,
    unsigned color_n, const YF_colordsc *resolves,
    const YF_depthdsc *depth_stencil)
{
  assert(ctx != NULL);

  /* TODO: Support for imageless pass. */
  if (colors == NULL || color_n < 1) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return NULL;
  }
  if (color_n > yf_getlimits(ctx)->pass.color_max) {
    yf_seterr(YF_ERR_LIMIT, __func__);
    return NULL;
  }

  YF_pass pass = calloc(1, sizeof(YF_pass_o));
  if (pass == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  pass->ctx = ctx;
  pass->color_n = color_n;
  pass->resolve_n = resolves == NULL ? 0 : color_n;
  pass->depth_n = depth_stencil == NULL ? 0 : 1;

  pass->tgts = calloc(YF_TGTN, sizeof *pass->tgts);
  if (pass->tgts == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    free(pass);
    return NULL;
  }
  pass->tgt_cap = YF_TGTN;
  pass->tgt_n = 0;
  pass->tgt_i = 0;

  const unsigned dsc_n = pass->color_n + pass->resolve_n + pass->depth_n;
  VkAttachmentDescription *dscs;
  dscs = malloc(dsc_n * sizeof(VkAttachmentDescription));
  if (dscs == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    yf_pass_deinit(pass);
    return NULL;
  }
  VkAttachmentReference *color_refs = NULL;
  if (pass->color_n > 0) {
    color_refs = malloc(pass->color_n * sizeof(VkAttachmentReference));
    if (color_refs == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      yf_pass_deinit(pass);
      free(dscs);
      return NULL;
    }
  }
  VkAttachmentReference *resolve_refs = NULL;
  if (pass->resolve_n > 0) {
    resolve_refs = malloc(pass->resolve_n * sizeof(VkAttachmentReference));
    if (resolve_refs == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      yf_pass_deinit(pass);
      free(dscs);
      free(color_refs);
      return NULL;
    }
  }
  VkAttachmentReference *depth_ref = NULL;
  if (pass->depth_n > 0) {
    depth_ref = malloc(pass->depth_n * sizeof(VkAttachmentReference));
    if (depth_ref == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      yf_pass_deinit(pass);
      free(dscs);
      free(color_refs);
      free(resolve_refs);
      return NULL;
    }
  }

  unsigned dsc_i = 0;
  for (unsigned i = 0; i < pass->color_n; ++i) {
    dscs[dsc_i].flags = 0;
    YF_PIXFMT_FROM(colors[i].pixfmt, dscs[dsc_i].format);
    YF_SAMPLES_FROM(colors[i].samples, dscs[dsc_i].samples);
    YF_LOADOP_FROM(colors[i].loadop, dscs[dsc_i].loadOp);
    YF_STOREOP_FROM(colors[i].storeop, dscs[dsc_i].storeOp);
    dscs[dsc_i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    dscs[dsc_i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    dscs[dsc_i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    dscs[dsc_i].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_refs[i].attachment = dsc_i;
    color_refs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ++dsc_i;
  }
  for (unsigned i = 0; i < pass->resolve_n; ++i) {
    dscs[dsc_i].flags = 0;
    YF_PIXFMT_FROM(resolves[i].pixfmt, dscs[dsc_i].format);
    YF_SAMPLES_FROM(resolves[i].samples, dscs[dsc_i].samples);
    YF_LOADOP_FROM(resolves[i].loadop, dscs[dsc_i].loadOp);
    YF_STOREOP_FROM(resolves[i].storeop, dscs[dsc_i].storeOp);
    dscs[dsc_i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    dscs[dsc_i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    dscs[dsc_i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    dscs[dsc_i].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    resolve_refs[i].attachment = dsc_i;
    resolve_refs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ++dsc_i;
  }
  if (depth_stencil != NULL) {
    dscs[dsc_i].flags = 0;
    YF_PIXFMT_FROM(depth_stencil->pixfmt, dscs[dsc_i].format);
    YF_SAMPLES_FROM(depth_stencil->samples, dscs[dsc_i].samples);
    YF_LOADOP_FROM(depth_stencil->depth_loadop, dscs[dsc_i].loadOp);
    YF_STOREOP_FROM(depth_stencil->depth_storeop, dscs[dsc_i].storeOp);
    YF_LOADOP_FROM(depth_stencil->stencil_loadop, dscs[dsc_i].loadOp);
    YF_STOREOP_FROM(depth_stencil->stencil_storeop, dscs[dsc_i].storeOp);
    dscs[dsc_i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    dscs[dsc_i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_ref->attachment = dsc_i;
    depth_ref->layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  }
  for (unsigned i = 0; i < dsc_n; ++i) {
    if (dscs[i].format == VK_FORMAT_UNDEFINED || dscs[i].samples == INT_MAX ||
        dscs[i].loadOp == INT_MAX || dscs[i].storeOp == INT_MAX ||
        dscs[i].stencilLoadOp == INT_MAX || dscs[i].stencilStoreOp == INT_MAX)
    {
      yf_seterr(YF_ERR_INVARG, __func__);
      yf_pass_deinit(pass);
      free(dscs);
      free(color_refs);
      free(resolve_refs);
      free(depth_ref);
      return NULL;
    }
  }

  VkSubpassDescription sub = {
    .flags = 0,
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .inputAttachmentCount = 0,
    .pInputAttachments = NULL,
    .colorAttachmentCount = pass->color_n,
    .pColorAttachments = color_refs,
    .pResolveAttachments = resolve_refs,
    .pDepthStencilAttachment = depth_ref,
    .preserveAttachmentCount = 0,
    .pPreserveAttachments = NULL
  };
  VkRenderPassCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .attachmentCount = dsc_n,
    .pAttachments = dscs,
    .subpassCount = 1,
    .pSubpasses = &sub,
    .dependencyCount = 0,
    .pDependencies = NULL
  };
  VkResult res = vkCreateRenderPass(ctx->device, &info, NULL, &pass->ren_pass);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    yf_pass_deinit(pass);
    pass = NULL;
  }

  free(dscs);
  free(color_refs);
  free(resolve_refs);
  free(depth_ref);
  return pass;
}

YF_target yf_pass_maketarget(YF_pass pass, YF_dim2 dim, unsigned layers,
    const YF_attach *colors, unsigned color_n, const YF_attach *resolves,
    const YF_attach *depth_stencil)
{
  assert(pass != NULL);

  const YF_limits *lim = yf_getlimits(pass->ctx);
  if (layers > lim->pass.layer_max ||
      dim.width > lim->pass.dim_max.width ||
      dim.height > lim->pass.dim_max.height)
  {
    yf_seterr(YF_ERR_LIMIT, __func__);
    return NULL;
  }

  const unsigned resolve_n = resolves == NULL ? 0 : color_n;
  const unsigned depth_n = depth_stencil == NULL ? 0 : 1;
  if (layers < 1 || color_n != pass->color_n ||
      resolve_n != pass->resolve_n || depth_n != pass->depth_n)
  {
    yf_seterr(YF_ERR_INVARG, __func__);
    return NULL;
  }

  if (pass->tgt_n == pass->tgt_cap) {
    unsigned cap = pass->tgt_cap * 2;
    YF_target *tmp = realloc(pass->tgts, cap * sizeof *pass->tgts);
    if (tmp == NULL) {
      cap = pass->tgt_cap + 1;
      tmp = realloc(pass->tgts, cap * sizeof *pass->tgts);
      if (tmp == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
      }
    }
    memset(tmp+pass->tgt_n, 0, (cap - pass->tgt_cap) * sizeof *pass->tgts);
    pass->tgts = tmp;
    pass->tgt_cap = cap;
    pass->tgt_i = pass->tgt_n;
  }

  for (unsigned i = 0, j = 0; i < pass->tgt_cap; ++i) {
    j = (pass->tgt_i + i) % pass->tgt_cap;
    if (pass->tgts[j] == NULL) {
      pass->tgt_i = j;
      break;
    }
  }

  YF_target tgt = calloc(1, sizeof(YF_target_o));
  if (tgt == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  tgt->pass = pass;
  tgt->dim = dim;
  tgt->layers = layers;
  tgt->iview_n = color_n + resolve_n + depth_n;
  tgt->iviews = calloc(tgt->iview_n, sizeof *tgt->iviews);
  tgt->imgs = malloc(tgt->iview_n * sizeof *tgt->imgs);
  tgt->lays_base = malloc(tgt->iview_n * sizeof *tgt->lays_base);
  if (tgt->iviews == NULL || tgt->imgs == NULL || tgt->lays_base == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    free(tgt->iviews);
    free(tgt->imgs);
    free(tgt->lays_base);
    free(tgt);
    return NULL;
  }

  const YF_slice lvl = {0, 1};
  YF_slice lay = {0, layers};
  unsigned iview_i = 0;
  int r = -1;
  VkImageView *info_views = malloc(tgt->iview_n * sizeof(VkImageView));
  if (info_views == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    free(tgt->iviews);
    free(tgt->imgs);
    free(tgt->lays_base);
    free(tgt);
    return NULL;
  }

  for (unsigned i = 0; i < color_n; ++i) {
    lay.i = colors[i].layer_base;
    r = yf_image_getiview(colors[i].img, lay, lvl, tgt->iviews+iview_i);
    if (r != 0) {
      for (unsigned i = 0; i < iview_i; ++i)
        yf_image_ungetiview(tgt->imgs[i], tgt->iviews+i);
      free(tgt->iviews);
      free(tgt->imgs);
      free(tgt->lays_base);
      free(tgt);
      free(info_views);
      return NULL;
    }
    tgt->imgs[iview_i] = colors[i].img;
    tgt->lays_base[iview_i] = lay.i;
    info_views[iview_i] = tgt->iviews[iview_i].view;
    ++iview_i;
  }
  for (unsigned i = 0; i < resolve_n; ++i) {
    lay.i = resolves[i].layer_base;
    r = yf_image_getiview(resolves[i].img, lay, lvl, tgt->iviews+iview_i);
    if (r != 0) {
      for (unsigned i = 0; i < iview_i; ++i)
        yf_image_ungetiview(tgt->imgs[i], tgt->iviews+i);
      free(tgt->iviews);
      free(tgt->imgs);
      free(tgt->lays_base);
      free(tgt);
      free(info_views);
      return NULL;
    }
    tgt->imgs[iview_i] = resolves[i].img;
    tgt->lays_base[iview_i] = lay.i;
    info_views[iview_i] = tgt->iviews[iview_i].view;
    ++iview_i;
  }
  if (depth_stencil != NULL) {
    lay.i = depth_stencil->layer_base;
    r = yf_image_getiview(depth_stencil->img, lay, lvl, tgt->iviews+iview_i);
    if (r != 0) {
      for (unsigned i = 0; i < iview_i; ++i)
        yf_image_ungetiview(tgt->imgs[i], tgt->iviews+i);
      free(tgt->iviews);
      free(tgt->imgs);
      free(tgt->lays_base);
      free(tgt);
      free(info_views);
      return NULL;
    }
    tgt->imgs[iview_i] = depth_stencil->img;
    tgt->lays_base[iview_i] = lay.i;
    info_views[iview_i] = tgt->iviews[iview_i].view;
  }

  VkFramebufferCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .renderPass = pass->ren_pass,
    .attachmentCount = tgt->iview_n,
    .pAttachments = info_views,
    .width = dim.width,
    .height = dim.height,
    .layers = layers
  };
  VkResult res = vkCreateFramebuffer(pass->ctx->device, &info, NULL,
      &tgt->framebuf);
  free(info_views);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    for (unsigned i = 0; i < tgt->iview_n; ++i)
      yf_image_ungetiview(tgt->imgs[i], tgt->iviews+i);
    free(tgt->iviews);
    free(tgt->imgs);
    free(tgt->lays_base);
    free(tgt);
    return NULL;
  }

  pass->tgts[pass->tgt_i] = tgt;
  ++pass->tgt_n;
  pass->tgt_i = (pass->tgt_i + 1) % pass->tgt_cap;
  return tgt;
}

int yf_pass_unmktarget(YF_pass pass, YF_target tgt) {
  assert(pass != NULL);

  if (tgt == NULL)
    return 0;

  /* the somewhat redundant 'pass' parameter makes the api consistent */
  if (pass != tgt->pass) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
  }

  unsigned index = pass->tgt_cap;
  for (unsigned i = 0; i < pass->tgt_cap; ++i) {
    if (pass->tgts[i] == tgt) {
      index = i;
      break;
    }
  }
  assert(index < pass->tgt_cap);

  vkDestroyFramebuffer(pass->ctx->device, tgt->framebuf, NULL);
  for (unsigned i = 0; i < tgt->iview_n; ++i)
    yf_image_ungetiview(tgt->imgs[i], tgt->iviews+i);
  free(tgt->iviews);
  free(tgt->imgs);
  free(tgt->lays_base);
  free(tgt);

  pass->tgts[index] = NULL;
  --pass->tgt_n;
  pass->tgt_i = index;
  return 0;
}

void yf_pass_deinit(YF_pass pass) {
  if (pass != NULL) {
    vkDestroyRenderPass(pass->ctx->device, pass->ren_pass, NULL);
    for (unsigned i = 0; i < pass->tgt_cap; ++i)
      yf_pass_unmktarget(pass, pass->tgts[i]);
    free(pass->tgts);
    free(pass);
  }
}
