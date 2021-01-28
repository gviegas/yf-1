/*
 * YF
 * dtable.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <yf/com/yf-error.h>

#include "dtable.h"
#include "context.h"
#include "sampler.h"
#include "buffer.h"
#include "image.h"

/* Type defining a key/value for the iview's hashset. */
typedef struct {
  struct {
    unsigned alloc_i;
    unsigned entry_i;
  } key;
  YF_iview *iviews;
  YF_image *imgs;
} L_kv;

/* Initializes the descriptor set layout. */
static int init_layout(YF_dtable dtb);

/* Hashes a 'L_kv'. */
static size_t hash_kv(const void *x);

/* Compares a 'L_kv' to another. */
static int cmp_kv(const void *a, const void *b);

YF_dtable yf_dtable_init(YF_context ctx, const YF_dentry *entries,
    unsigned entry_n)
{
  assert(ctx != NULL);
  assert(entries != NULL);
  assert(entry_n > 0);

  YF_dtable dtb = calloc(1, sizeof(YF_dtable_o));
  if (dtb == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  dtb->ctx = ctx;

  const size_t sz = entry_n * sizeof *entries;
  dtb->entries = malloc(sz);
  if (dtb->entries == NULL) {
    free(dtb);
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  memcpy(dtb->entries, entries, sz);
  dtb->entry_n = entry_n;

  if (init_layout(dtb) != 0) {
    yf_dtable_deinit(dtb);
    return NULL;
  }

  return dtb;
}

int yf_dtable_alloc(YF_dtable dtb, unsigned n) {
  assert(dtb != NULL);

  if (n == dtb->set_n)
    return 0;
  yf_dtable_dealloc(dtb);
  if (n == 0)
    return 0;

  VkDescriptorPoolSize sizes[6];
  unsigned sz_i = 0;
  if (dtb->count.unif > 0) {
    sizes[sz_i].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    sizes[sz_i].descriptorCount = dtb->count.unif * n;
    ++sz_i;
  }
  if (dtb->count.mut > 0) {
    sizes[sz_i].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    sizes[sz_i].descriptorCount = dtb->count.mut * n;
    ++sz_i;
  }
  if (dtb->count.img > 0) {
    sizes[sz_i].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    sizes[sz_i].descriptorCount = dtb->count.img * n;
    ++sz_i;
  }
  if (dtb->count.sampd > 0) {
    sizes[sz_i].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    sizes[sz_i].descriptorCount = dtb->count.sampd * n;
    ++sz_i;
  }
  if (dtb->count.sampr > 0) {
    sizes[sz_i].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    sizes[sz_i].descriptorCount = dtb->count.sampr * n;
    ++sz_i;
  }
  if (dtb->count.isamp > 0) {
    sizes[sz_i].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sizes[sz_i].descriptorCount = dtb->count.isamp * n;
    ++sz_i;
  }

  VkResult res;
  VkDescriptorPoolCreateInfo pool_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .pNext = NULL,
    .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
    .maxSets = n,
    .poolSizeCount = sz_i,
    .pPoolSizes = sizes
  };
  res = vkCreateDescriptorPool(dtb->ctx->device, &pool_info, NULL, &dtb->pool);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }

  dtb->sets = malloc(n * sizeof *dtb->sets);
  if (dtb->sets == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    vkDestroyDescriptorPool(dtb->ctx->device, dtb->pool, NULL);
    dtb->pool = VK_NULL_HANDLE;
    return -1;
  }
  dtb->set_n = n;

  VkDescriptorSetLayout *layouts = malloc(n * sizeof dtb->layout);
  if (layouts == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    yf_dtable_dealloc(dtb);
    return -1;
  }
  for (unsigned i = 0; i < n; ++i)
    layouts[i] = dtb->layout;

  VkDescriptorSetAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .pNext = NULL,
    .descriptorPool = dtb->pool,
    .descriptorSetCount = n,
    .pSetLayouts = layouts
  };
  res = vkAllocateDescriptorSets(dtb->ctx->device, &alloc_info, dtb->sets);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    yf_dtable_dealloc(dtb);
    free(layouts);
    return -1;
  }
  free(layouts);

  dtb->iviews = yf_hashset_init(hash_kv, cmp_kv);
  if (dtb->iviews == NULL) {
    yf_dtable_dealloc(dtb);
    return -1;
  }
  for (unsigned i = 0; i < dtb->entry_n; ++i) {
    switch (dtb->entries[i].dtype) {
      case YF_DTYPE_IMAGE:
      case YF_DTYPE_SAMPLED:
      case YF_DTYPE_ISAMPLER:
        for (unsigned j = 0; j < n; ++j) {
          L_kv *kv = calloc(1, sizeof(L_kv));
          if (kv == NULL) {
            yf_dtable_dealloc(dtb);
            return -1;
          }
          kv->iviews = calloc(dtb->entries[i].elements, sizeof *kv->iviews);
          kv->imgs = calloc(dtb->entries[i].elements, sizeof *kv->imgs);
          if (kv->iviews == NULL || kv->imgs == NULL) {
            yf_dtable_dealloc(dtb);
            free(kv->iviews);
            free(kv->imgs);
            free(kv);
            return -1;
          }
          kv->key.alloc_i = j;
          kv->key.entry_i = i;
          yf_hashset_insert(dtb->iviews, kv);
        }
        break;
      default:
        break;
    }
  }

  return 0;
}

void yf_dtable_dealloc(YF_dtable dtb) {
  assert(dtb != NULL);

  if (dtb->sets == NULL)
    return;

  YF_iter it = YF_NILIT;
  L_kv *kv;
  do {
    kv = yf_hashset_next(dtb->iviews, &it);
    if (YF_IT_ISNIL(it))
      break;
    for (unsigned i = 0; i < dtb->entries[kv->key.entry_i].elements; ++i) {
      if (kv->imgs[i] != NULL)
        /* FIXME: Crashes if any image used for copy is deinitialized before
           the dtable is deallocated. */
        yf_image_ungetiview(kv->imgs[i], kv->iviews+i);
    }
    free(kv->iviews);
    free(kv->imgs);
    free(kv);
  } while (1);
  yf_hashset_deinit(dtb->iviews);
  dtb->iviews = NULL;

  vkDestroyDescriptorPool(dtb->ctx->device, dtb->pool, NULL);
  dtb->pool = VK_NULL_HANDLE;
  free(dtb->sets);
  dtb->sets = NULL;
  dtb->set_n = 0;
}

int yf_dtable_copybuf(YF_dtable dtb, unsigned alloc_i, unsigned binding,
    YF_slice elements, const YF_buffer *bufs, const size_t *offsets,
    const size_t *sizes)
{
  assert(dtb != NULL);
  assert(elements.n > 0);
  assert(bufs != NULL);
  assert(offsets != NULL);
  assert(sizes != NULL);

  if (alloc_i >= dtb->set_n) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
  }

  YF_dentry *entry = NULL;
  for (unsigned i = 0; i < dtb->entry_n; ++i) {
    if (dtb->entries[i].binding == binding) {
      entry = dtb->entries+i;
      break;
    }
  }
  if (entry == NULL) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
  }

  if (elements.i + elements.n > entry->elements) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
  }

  VkDescriptorBufferInfo *buf_infos;
  buf_infos = malloc(elements.n * sizeof *buf_infos);
  if (buf_infos == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  for (unsigned i = 0; i < elements.n; ++i) {
    /* TODO: Check if region is within bounds. */
    buf_infos[i].buffer = bufs[i]->buffer;
    buf_infos[i].offset = offsets[i];
    buf_infos[i].range = sizes[i];
  }

  VkWriteDescriptorSet ds_wr = {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .pNext = NULL,
    .dstSet = dtb->sets[alloc_i],
    .dstBinding = binding,
    .dstArrayElement = elements.i,
    .descriptorCount = elements.n,
    .descriptorType = UINT32_MAX,
    .pImageInfo = NULL,
    .pBufferInfo = buf_infos,
    .pTexelBufferView = NULL
  };
  switch (entry->dtype) {
    case YF_DTYPE_UNIFORM:
      ds_wr.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      break;
    case YF_DTYPE_MUTABLE:
      ds_wr.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      break;
    default:
      yf_seterr(YF_ERR_INVARG, __func__);
      free(buf_infos);
      return -1;
  }
  vkUpdateDescriptorSets(dtb->ctx->device, 1, &ds_wr, 0, NULL);

  free(buf_infos);
  return 0;
}

int yf_dtable_copyimg(YF_dtable dtb, unsigned alloc_i, unsigned binding,
    YF_slice elements, const YF_image *imgs, const unsigned *layers)
{
  assert(dtb != NULL);
  assert(elements.n > 0);
  assert(imgs != NULL);
  assert(layers != NULL);

  if (alloc_i >= dtb->set_n) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
  }

  YF_dentry *entry = NULL;
  unsigned entry_i = 0;
  for (; entry_i < dtb->entry_n; ++entry_i) {
    if (dtb->entries[entry_i].binding == binding) {
      entry = dtb->entries+entry_i;
      break;
    }
  }
  if (entry == NULL) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
  }

  if (elements.i + elements.n > entry->elements) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
  }

  VkDescriptorImageInfo *img_infos;
  img_infos = malloc(elements.n * sizeof *img_infos);
  if (img_infos == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }

  const L_kv k = {{alloc_i, entry_i}, NULL, NULL};
  L_kv *kv = yf_hashset_search(dtb->iviews, &k);
  assert(kv != NULL);
  const YF_slice lvl = {0, 1};
  YF_slice lay = {0, 1};
  YF_iview iview;

  for (unsigned i = 0; i < elements.n; ++i) {
    /* TODO: Check if region is within bounds. */
    lay.i = layers[i];
    if (yf_image_getiview(imgs[i], lay, lvl, &iview) != 0) {
      free(img_infos);
      return -1;
    }
    if (kv->imgs[i] != NULL)
      /* FIXME: Crashes here too if 'kv->imgs[i]' is no more. */
      yf_image_ungetiview(kv->imgs[i], kv->iviews+i);
    kv->iviews[i] = iview;
    kv->imgs[i] = imgs[i];

    img_infos[i].sampler = VK_NULL_HANDLE;
    img_infos[i].imageView = iview.view;
    img_infos[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  }

  VkWriteDescriptorSet ds_wr = {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .pNext = NULL,
    .dstSet = dtb->sets[alloc_i],
    .dstBinding = binding,
    .dstArrayElement = elements.i,
    .descriptorCount = elements.n,
    .descriptorType = UINT32_MAX,
    .pImageInfo = img_infos,
    .pBufferInfo = NULL,
    .pTexelBufferView = NULL
  };
  switch (entry->dtype) {
    case YF_DTYPE_IMAGE:
      ds_wr.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      break;
    case YF_DTYPE_SAMPLED:
      ds_wr.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
      break;
    case YF_DTYPE_ISAMPLER:
      ds_wr.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      break;
    default:
      yf_seterr(YF_ERR_INVARG, __func__);
      free(img_infos);
      return -1;
  }
  vkUpdateDescriptorSets(dtb->ctx->device, 1, &ds_wr, 0, NULL);

  free(img_infos);
  return 0;
}

void yf_dtable_deinit(YF_dtable dtb) {
  if (dtb == NULL)
    return;

  yf_dtable_dealloc(dtb);
  vkDestroyDescriptorSetLayout(dtb->ctx->device, dtb->layout, NULL);

  YF_iter it = YF_NILIT;
  VkSampler sampler;
  do {
    sampler = yf_hashset_next(dtb->samplers, &it);
    if (YF_IT_ISNIL(it))
      break;
    vkDestroySampler(dtb->ctx->device, sampler, NULL);
  } while (1);
  yf_hashset_deinit(dtb->samplers);

  free(dtb->entries);
  free(dtb);
}

static int init_layout(YF_dtable dtb) {
  dtb->samplers = yf_hashset_init(NULL, NULL);
  if (dtb->samplers == NULL)
    return -1;

  VkDescriptorSetLayoutBinding *bindings;
  bindings = malloc(dtb->entry_n * sizeof *bindings);
  if (bindings == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  VkSampler *samplers = calloc(dtb->entry_n, sizeof(VkSampler));
  if (samplers == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    free(bindings);
    return -1;
  }
  unsigned samp_n = dtb->entry_n;
  unsigned samp_i = 0;

  for (unsigned i = 0; i < dtb->entry_n; ++i) {
    bindings[i].binding = dtb->entries[i].binding;
    bindings[i].descriptorCount = dtb->entries[i].elements;
    bindings[i].stageFlags = VK_SHADER_STAGE_ALL;
    switch (dtb->entries[i].dtype) {
      case YF_DTYPE_UNIFORM:
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[i].pImmutableSamplers = NULL;
        dtb->count.unif += dtb->entries[i].elements;
        continue;
      case YF_DTYPE_MUTABLE:
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].pImmutableSamplers = NULL;
        dtb->count.mut += dtb->entries[i].elements;
        continue;
      case YF_DTYPE_IMAGE:
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[i].pImmutableSamplers = NULL;
        dtb->count.img += dtb->entries[i].elements;
        continue;
      case YF_DTYPE_SAMPLED:
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        bindings[i].pImmutableSamplers = NULL;
        dtb->count.sampd += dtb->entries[i].elements;
        continue;
      case YF_DTYPE_SAMPLER:
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        dtb->count.sampr += dtb->entries[i].elements;
        break;
      case YF_DTYPE_ISAMPLER:
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        dtb->count.isamp += dtb->entries[i].elements;
        break;
      default:
        yf_seterr(YF_ERR_INVARG, __func__);
        free(bindings);
        free(samplers);
        return -1;
    }
    /* YF_DTYPE_SAMPLER or YF_DTYPE_ISAMPLER */
    if (samp_i + dtb->entries[i].elements > samp_n) {
      samp_n = samp_i + dtb->entries[i].elements;
      VkSampler *tmp = realloc(samplers, samp_n * sizeof *samplers);
      if (tmp == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        free(bindings);
        free(samplers);
        return -1;
      }
      samplers = tmp;
    }
    for (unsigned j = samp_i; j < samp_i + dtb->entries[i].elements; ++j) {
      /* TODO: Improve the sampler type parameter passing. */
      samplers[j] = yf_sampler_make(dtb->ctx, (size_t)dtb->entries[i].info);
      if (samplers[j] == NULL) {
        free(bindings);
        free(samplers);
        return -1;
      }
      yf_hashset_insert(dtb->samplers, samplers[j]);
      static_assert(sizeof(void *) >= sizeof *samplers, "bad size");
    }
    bindings[i].pImmutableSamplers = samplers+samp_i;
    samp_i += dtb->entries[i].elements;
  }

  VkDescriptorSetLayoutCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .bindingCount = dtb->entry_n,
    .pBindings = bindings
  };
  VkResult res = vkCreateDescriptorSetLayout(dtb->ctx->device, &info, NULL,
      &dtb->layout);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    free(bindings);
    free(samplers);
    return -1;
  }

  free(bindings);
  free(samplers);
  return 0;
}

static size_t hash_kv(const void *x) {
  const L_kv *kv = x;
  const size_t h1 = kv->key.alloc_i << 10;
  const size_t h2 = kv->key.entry_i & 0x3ff;
  return (h1 | h2) ^ 0xc5749e25;
}

static int cmp_kv(const void *a, const void *b) {
  const L_kv *kv1 = a;
  const L_kv *kv2 = b;
  return (kv1->key.alloc_i != kv2->key.alloc_i) ||
    (kv1->key.entry_i != kv2->key.entry_i);
}
