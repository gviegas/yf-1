/*
 * YF
 * dtable.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "yf/com/yf-pubsub.h"
#include "yf/com/yf-error.h"

#include "dtable.h"
#include "context.h"
#include "sampler.h"
#include "buffer.h"
#include "image.h"
#include "yf-limits.h"

/* Key/value for the 'iss' dictionary. */
typedef struct {
    struct {
        unsigned alloc_i;
        unsigned entry_i;
    } key;
    /* one value for each element */
    struct {
        yf_iview_t iview;
        yf_image_t *img;
        const yf_splrh_t *splrh;
    } *val;
} kv_t;

/* Invalidates all iviews acquired from a given image. */
static void inval_iview(void *img, int pubsub, void *dtb)
{
    assert(img != NULL);
    assert(dtb != NULL);
    assert(pubsub == YF_PUBSUB_DEINIT);

    /* XXX: This may end up being too slow if images are destroyed often. */

    const yf_dentry_t *entries = ((yf_dtable_t *)dtb)->entries;
    yf_dict_t *iss = ((yf_dtable_t *)dtb)->iss;
    yf_iter_t it = YF_NILIT;
    kv_t *kv;

    while ((kv = yf_dict_next(iss, &it, NULL)) != NULL) {
        const unsigned n = entries[kv->key.entry_i].elements;
        for (unsigned i = 0; i < n; i++) {
            if (kv->val[i].img == img)
                kv->val[i].img = NULL;
        }
    }
}

/* Hashes a 'kv_t'. */
static size_t hash_kv(const void *x)
{
    const kv_t *kv = x;
    return yf_hashv(&kv->key, sizeof kv->key, NULL);

    static_assert(sizeof kv->key == 2*sizeof(unsigned), "!sizeof");
}

/* Compares a 'kv_t' to another. */
static int cmp_kv(const void *a, const void *b)
{
    const kv_t *kv1 = a;
    const kv_t *kv2 = b;

    return kv1->key.alloc_i != kv2->key.alloc_i ||
           kv1->key.entry_i != kv2->key.entry_i;
}

/* Initializes the descriptor set layout. */
static int init_layout(yf_dtable_t *dtb)
{
    VkDescriptorSetLayoutBinding *bindings = malloc(dtb->entry_n *
                                                    sizeof *bindings);
    if (bindings == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }

    for (unsigned i = 0; i < dtb->entry_n; i++) {
        bindings[i].binding = dtb->entries[i].binding;
        bindings[i].descriptorCount = dtb->entries[i].elements;
        bindings[i].stageFlags = VK_SHADER_STAGE_ALL;
        bindings[i].pImmutableSamplers = NULL;

        switch (dtb->entries[i].dtype) {
        case YF_DTYPE_UNIFORM:
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            dtb->count.unif += dtb->entries[i].elements;
            break;
        case YF_DTYPE_MUTABLE:
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            dtb->count.mut += dtb->entries[i].elements;
            break;
        case YF_DTYPE_IMAGE:
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            dtb->count.img += dtb->entries[i].elements;
            break;
        case YF_DTYPE_SAMPLED:
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            dtb->count.spld += dtb->entries[i].elements;
            break;
        case YF_DTYPE_SAMPLER:
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            dtb->count.splr += dtb->entries[i].elements;
            break;
        case YF_DTYPE_ISAMPLER:
            bindings[i].descriptorType =
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            dtb->count.ispl += dtb->entries[i].elements;
            break;
        default:
            yf_seterr(YF_ERR_INVARG, __func__);
            free(bindings);
            return -1;
        }
    }

    const yf_limits_t *lim = yf_getlimits(dtb->ctx);

    if (dtb->count.unif > lim->dtable.unif_max ||
        dtb->count.mut > lim->dtable.mut_max ||
        dtb->count.img > lim->dtable.img_max ||
        dtb->count.spld > lim->dtable.spld_max ||
        dtb->count.splr > lim->dtable.splr_max ||
        dtb->count.ispl > lim->dtable.ispl_max ||
        (dtb->count.unif + dtb->count.mut + dtb->count.img + dtb->count.spld +
         dtb->count.splr + dtb->count.ispl) > lim->dtable.stg_res_max) {

        yf_seterr(YF_ERR_LIMIT, __func__);
        free(bindings);
        return -1;
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
        return -1;
    }

    free(bindings);
    return 0;
}

yf_dtable_t *yf_dtable_init(yf_context_t *ctx, const yf_dentry_t *entries,
                            unsigned entry_n)
{
    assert(ctx != NULL);
    assert(entries != NULL);
    assert(entry_n > 0);

    yf_dtable_t *dtb = calloc(1, sizeof(yf_dtable_t));
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

int yf_dtable_alloc(yf_dtable_t *dtb, unsigned n)
{
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
        sz_i++;
    }
    if (dtb->count.mut > 0) {
        sizes[sz_i].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        sizes[sz_i].descriptorCount = dtb->count.mut * n;
        sz_i++;
    }
    if (dtb->count.img > 0) {
        sizes[sz_i].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        sizes[sz_i].descriptorCount = dtb->count.img * n;
        sz_i++;
    }
    if (dtb->count.spld > 0) {
        sizes[sz_i].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        sizes[sz_i].descriptorCount = dtb->count.spld * n;
        sz_i++;
    }
    if (dtb->count.splr > 0) {
        sizes[sz_i].type = VK_DESCRIPTOR_TYPE_SAMPLER;
        sizes[sz_i].descriptorCount = dtb->count.splr * n;
        sz_i++;
    }
    if (dtb->count.ispl > 0) {
        sizes[sz_i].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sizes[sz_i].descriptorCount = dtb->count.ispl * n;
        sz_i++;
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

    res = vkCreateDescriptorPool(dtb->ctx->device, &pool_info, NULL,
                                 &dtb->pool);
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

    for (unsigned i = 0; i < n; i++)
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

    dtb->iss = yf_dict_init(hash_kv, cmp_kv);
    if (dtb->iss == NULL) {
        yf_dtable_dealloc(dtb);
        return -1;
    }

    for (unsigned i = 0; i < dtb->entry_n; i++) {
        switch (dtb->entries[i].dtype) {
        case YF_DTYPE_IMAGE:
        case YF_DTYPE_SAMPLED:
        case YF_DTYPE_SAMPLER:
        case YF_DTYPE_ISAMPLER:
            for (unsigned j = 0; j < n; j++) {
                kv_t *kv = calloc(1, sizeof(kv_t));
                if (kv == NULL) {
                    yf_dtable_dealloc(dtb);
                    return -1;
                }

                kv->key.alloc_i = j;
                kv->key.entry_i = i;

                kv->val = calloc(dtb->entries[i].elements, sizeof *kv->val);
                if (kv->val == NULL || yf_dict_insert(dtb->iss, kv, kv) != 0) {
                    yf_dtable_dealloc(dtb);
                    free(kv->val);
                    free(kv);
                    return -1;
                }
            }
            break;

        default:
            break;
        }
    }

    return 0;
}

void yf_dtable_dealloc(yf_dtable_t *dtb)
{
    assert(dtb != NULL);

    if (dtb->sets == NULL)
        return;

    yf_iter_t it = YF_NILIT;
    kv_t *kv;

    while ((kv = yf_dict_next(dtb->iss, &it, NULL)) != NULL) {
        for (unsigned i = 0; i < dtb->entries[kv->key.entry_i].elements; i++) {
            if (kv->val[i].img != NULL) {
                yf_subscribe(kv->val[i].img, dtb, YF_PUBSUB_NONE, NULL, NULL);
                yf_image_ungetiview(kv->val[i].img, &kv->val[i].iview);
            }
            if (kv->val[i].splrh != NULL)
                yf_sampler_unget(dtb->ctx, kv->val[i].splrh);
        }
        free(kv->val);
        free(kv);
    }

    yf_dict_deinit(dtb->iss);
    dtb->iss = NULL;

    vkDestroyDescriptorPool(dtb->ctx->device, dtb->pool, NULL);
    dtb->pool = VK_NULL_HANDLE;
    free(dtb->sets);
    dtb->sets = NULL;
    dtb->set_n = 0;
}

int yf_dtable_copybuf(yf_dtable_t *dtb, unsigned alloc_i, unsigned binding,
                      yf_slice_t elements, yf_buffer_t *const *bufs,
                      const size_t *offsets, const size_t *sizes)
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

    yf_dentry_t *entry = NULL;

    for (unsigned i = 0; i < dtb->entry_n; i++) {
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

    VkDescriptorBufferInfo *buf_infos = malloc(elements.n * sizeof *buf_infos);
    if (buf_infos == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }

    for (unsigned i = 0; i < elements.n; i++) {
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

    const yf_limits_t *lim = yf_getlimits(dtb->ctx);

    switch (entry->dtype) {
    case YF_DTYPE_UNIFORM:
        for (unsigned i = 0; i < elements.n; i++) {
            if (offsets[i] % lim->dtable.cpy_unif_align_min != 0 ||
                sizes[i] > lim->dtable.cpy_unif_sz_max) {
                yf_seterr(YF_ERR_LIMIT, __func__);
                free(buf_infos);
                return -1;
            }
        }
        ds_wr.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        break;

    case YF_DTYPE_MUTABLE:
        for (unsigned i = 0; i < elements.n; i++) {
            if (offsets[i] % lim->dtable.cpy_mut_align_min != 0 ||
                sizes[i] > lim->dtable.cpy_mut_sz_max) {
                yf_seterr(YF_ERR_LIMIT, __func__);
                free(buf_infos);
                return -1;
            }
        }
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

int yf_dtable_copyimg(yf_dtable_t *dtb, unsigned alloc_i, unsigned binding,
                      yf_slice_t elements, yf_image_t *const *imgs,
                      const unsigned *layers, const yf_sampler_t *splrs)
{
    assert(dtb != NULL);
    assert(elements.n > 0);
    assert(imgs != NULL);
    assert(layers != NULL);

    if (alloc_i >= dtb->set_n) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    yf_dentry_t *entry = NULL;
    unsigned entry_i = 0;

    for (; entry_i < dtb->entry_n; entry_i++) {
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

    VkDescriptorImageInfo *img_infos = malloc(elements.n * sizeof *img_infos);
    if (img_infos == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }

    const kv_t k = {{alloc_i, entry_i}, NULL};
    kv_t *kv = yf_dict_search(dtb->iss, &k);
    const yf_slice_t lvl = {0, 1};
    yf_slice_t lay = {0, 1};
    yf_iview_t iview;
    unsigned elem_i;

    assert(kv != NULL);

    for (unsigned i = 0; i < elements.n; i++) {
        /* TODO: Check if region is within bounds. */
        lay.i = layers[i];
        elem_i = elements.i + i;

        if (yf_image_getiview(imgs[i], lay, lvl, &iview) != 0) {
            free(img_infos);
            return -1;
        }

        yf_subscribe(imgs[i], dtb, YF_PUBSUB_DEINIT, inval_iview, dtb);

        if (kv->val[elem_i].img != NULL) {
            if (kv->val[elem_i].img != imgs[i])
                yf_subscribe(kv->val[elem_i].img, dtb, YF_PUBSUB_NONE,
                             NULL, NULL);

            yf_image_ungetiview(kv->val[elem_i].img, &kv->val[elem_i].iview);
        }

        kv->val[elem_i].iview = iview;
        kv->val[elem_i].img = imgs[i];

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
        for (unsigned i = 0; i < elements.n; i++) {
            elem_i = elements.i + i;
            const yf_sampler_t *splr = splrs != NULL ? splrs+i : NULL;
            const yf_splrh_t *subs = kv->val[elem_i].splrh;

            kv->val[elem_i].splrh = yf_sampler_get(dtb->ctx, splr, subs);
            if (kv->val[elem_i].splrh == NULL) {
                free(img_infos);
                return -1;
            }

            img_infos[i].sampler = kv->val[elem_i].splrh->handle;
        }
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

void yf_dtable_deinit(yf_dtable_t *dtb)
{
    if (dtb == NULL)
        return;

    yf_dtable_dealloc(dtb);
    vkDestroyDescriptorSetLayout(dtb->ctx->device, dtb->layout, NULL);
    free(dtb->entries);
    free(dtb);
}
