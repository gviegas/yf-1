/*
 * YF
 * texture.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef YF_DEVEL
# include <stdio.h>
#endif

#include "yf/com/yf-dict.h"
#include "yf/com/yf-error.h"
#include "yf/core/yf-image.h"

#include "texture.h"
#include "coreobj.h"
#include "data-png.h"

/* TODO: Thread-safe. */

/* TODO: Should be defined elsewhere. */
#define YF_LAYCAP 16

/* Image object and associated layer state. */
/* XXX: This assumes that a single image object will suffice. */
typedef struct {
    yf_image_t *img;
    char *lay_used;
    unsigned lay_n;
    unsigned lay_i;
} img_t;

/* Key for the dictionary of 'img_t' values. */
/* TODO: Add levels and samples as key. */
typedef struct {
    int pixfmt;
    yf_dim2_t dim;
} k_t;

/* Key/value pair for the dictionary of 'img_t' values. */
typedef struct {
    k_t key;
    img_t val;
} kv_t;

struct yf_texture {
    img_t *img;
    unsigned layer;
    yf_sampler_t splr;
    int uvset;
};

/* Global context. */
static yf_context_t *ctx_ = NULL;

/* Dictionary containing all created images. */
static yf_dict_t *imgs_ = NULL;

/* Copies texture data to image and updates texture object. */
static int copy_data(yf_texture_t *tex, const yf_texdt_t *data)
{
    const k_t key = {data->pixfmt, data->dim};
    kv_t *kv = yf_dict_search(imgs_, &key);

    yf_dim3_t dim;
    unsigned layers;
    img_t *val;

    if (kv == NULL) {
        /* new image */
        if ((kv = malloc(sizeof *kv)) == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
        }

        val = &kv->val;
        dim = (yf_dim3_t){data->dim.width, data->dim.height, 1};

        val->img = yf_image_init(ctx_, data->pixfmt, dim, YF_LAYCAP, 1, 1);
        if (val->img == NULL) {
            free(kv);
            return -1;
        }

        val->lay_used = calloc(YF_LAYCAP, sizeof *val->lay_used);
        if (val->lay_used == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            yf_image_deinit(val->img);
            free(kv);
            return -1;
        }

        val->lay_n = 0;
        val->lay_i = 0;
        kv->key = key;

        if (yf_dict_insert(imgs_, &kv->key, kv) != 0) {
            yf_image_deinit(val->img);
            free(val->lay_used);
            free(kv);
            return -1;
        }

        layers = YF_LAYCAP;

    } else {
        /* check layer cap. */
        val = &kv->val;

        int pixfmt;
        unsigned levels, samples;
        yf_image_getval(val->img, &pixfmt, &dim, &layers, &levels, &samples);

        if (val->lay_n == layers) {
            const unsigned new_lay_cap = layers << 1;
            yf_image_t *new_img = yf_image_init(ctx_, pixfmt, dim, new_lay_cap,
                                                levels, samples);
            if (new_img == NULL)
                return -1;

            yf_cmdbuf_t *cb = yf_cmdbuf_get(ctx_, YF_CMDBUF_XFER);
            if (cb == NULL) {
                yf_image_deinit(new_img);
                return -1;
            }

            const yf_off3_t off = {0};
            yf_cmdbuf_copyimg(cb, new_img, off, 0, 0, val->img, off, 0, 0,
                              dim, layers);

            if (yf_cmdbuf_end(cb) != 0 || yf_cmdbuf_exec(ctx_) != 0) {
                yf_image_deinit(new_img);
                return -1;
            }

            char *new_lay_used = realloc(val->lay_used, new_lay_cap);
            if (new_lay_used == NULL) {
                yf_image_deinit(new_img);
                return -1;
            }

            /* XXX: Will crash if old image is in use. */
            yf_image_deinit(val->img);
            val->img = new_img;

            memset(new_lay_used+layers, 0, layers);
            val->lay_used = new_lay_used;
            val->lay_i = layers;

            layers = new_lay_cap;
        }
    }

    unsigned layer = val->lay_i;
    for (; val->lay_used[layer]; layer = (layer+1) % layers)
        ;

    const yf_off3_t off = {0};
    if (yf_image_copy(val->img, off, dim, layer, 0, data->data) != 0) {
        if (val->lay_n == 0) {
            yf_dict_remove(imgs_, &key);
            yf_image_deinit(val->img);
            free(val->lay_used);
            free(kv);
        }
        return -1;
    }

    val->lay_used[layer] = 1;
    val->lay_n++;
    val->lay_i = (layer+1) % layers;
    tex->img = val;
    tex->layer = layer;
    tex->splr = data->splr;
    tex->uvset = data->uvset;

    return 0;
}

/* Hashes a 'k_t'. */
static size_t hash_key(const void *x)
{
    static_assert(sizeof(k_t) == 3*sizeof(int), "!sizeof");
    return yf_hashv(x, sizeof(k_t), NULL);
}

/* Compares a 'k_t' to another. */
static int cmp_key(const void *a, const void *b)
{
    const k_t *k1 = a;
    const k_t *k2 = b;

    return k1->pixfmt != k2->pixfmt ||
           k1->dim.width != k2->dim.width ||
           k1->dim.height != k2->dim.height;
}

yf_texture_t *yf_texture_load(const char *pathname, size_t index,
                              yf_collec_t *coll)
{
    /* TODO: Consider checking the type of the file. */
    if (coll == NULL)
        coll = yf_collec_get();
    return yf_collec_loaditem(coll, YF_CITEM_TEXTURE, pathname, index);
}

yf_texture_t *yf_texture_init(const yf_texdt_t *data)
{
    assert(data != NULL);

    if (ctx_ == NULL && (ctx_ = yf_getctx()) == NULL)
        return NULL;
    if (imgs_ == NULL && (imgs_ = yf_dict_init(hash_key, cmp_key)) == NULL)
        return NULL;

    yf_texture_t *tex = calloc(1, sizeof(yf_texture_t));
    if (tex == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }

    if (copy_data(tex, data) != 0) {
        yf_texture_deinit(tex);
        return NULL;
    }

    return tex;
}

yf_texref_t *yf_texture_getref(yf_texture_t *tex, yf_texref_t *ref)
{
    assert(tex != NULL);
    assert(ref != NULL);

    ref->tex = tex;
    ref->splr = tex->splr;
    ref->uvset = tex->uvset;
    return ref;
}

yf_dim2_t yf_texture_getdim(yf_texture_t *tex)
{
    assert(tex != NULL);

    yf_dim3_t dim;
    yf_image_getval(tex->img->img, NULL, &dim, NULL, NULL, NULL);
    return (yf_dim2_t){dim.width, dim.height};
}

yf_sampler_t *yf_texture_getsplr(yf_texture_t *tex)
{
    assert(tex != NULL);
    return &tex->splr;
}

int yf_texture_getuv(yf_texture_t *tex)
{
    assert(tex != NULL);
    return tex->uvset;
}

void yf_texture_setuv(yf_texture_t *tex, int uvset)
{
    assert(tex != NULL);
    assert(uvset == YF_UVSET_0 || uvset == YF_UVSET_1);

    tex->uvset = uvset;
}

void yf_texture_deinit(yf_texture_t *tex)
{
    if (tex == NULL)
        return;

    yf_dim3_t dim;
    int pixfmt;
    yf_image_getval(tex->img->img, &pixfmt, &dim, NULL, NULL, NULL);

    const k_t key = {pixfmt, {dim.width, dim.height}};
    kv_t *kv = yf_dict_search(imgs_, &key);

    assert(kv != NULL);

    if (kv->val.lay_n > 1) {
        kv->val.lay_used[tex->layer] = 0;
        kv->val.lay_n--;
    } else {
        yf_dict_remove(imgs_, &key);
        yf_image_deinit(kv->val.img);
        free(kv->val.lay_used);
        free(kv);
    }

    free(tex);
}

int yf_texture_setdata(yf_texture_t *tex, yf_off2_t off, yf_dim2_t dim,
                       const void *data)
{
    assert(tex != NULL);
    assert(data != NULL);

    const yf_off3_t off3 = {off.x, off.y, 0};
    const yf_dim3_t dim3 = {dim.width, dim.height, 1};
    /* TODO: Mip level. */
    return yf_image_copy(tex->img->img, off3, dim3, tex->layer, 0, data);
}

int yf_texture_copyres(yf_texture_t *tex, yf_dtable_t *dtb, unsigned alloc_i,
                       unsigned binding, unsigned element)
{
    assert(tex != NULL);
    assert(dtb != NULL);

    yf_slice_t elem = {element, 1};
    return yf_dtable_copyimg(dtb, alloc_i, binding, elem,
                             &tex->img->img, &tex->layer, &tex->splr);
}

int yf_texture_copyres2(const yf_texref_t *ref, yf_dtable_t *dtb,
                        unsigned alloc_i, unsigned binding, unsigned element)
{
    assert(ref != NULL);
    assert(dtb != NULL);
    assert(ref->tex != NULL);

    yf_slice_t elem = {element, 1};
    yf_texture_t *tex = ref->tex;
    return yf_dtable_copyimg(dtb, alloc_i, binding, elem,
                             &tex->img->img, &tex->layer, &ref->splr);
}

/* Called by 'coreobj' on exit. */
void yf_unsettex(void)
{
    if (imgs_ != NULL) {
        yf_iter_t it = YF_NILIT;
        kv_t *kv;
        while ((kv = yf_dict_next(imgs_, &it, NULL)) != NULL) {
            yf_image_deinit(kv->val.img);
            free(kv->val.lay_used);
            free(kv);
        }
        yf_dict_deinit(imgs_);
        imgs_ = NULL;
    }
    ctx_ = NULL;
}

/*
 * DEVEL
 */

#ifdef YF_DEVEL

void yf_print_tex(yf_texture_t *tex)
{
    assert(ctx_ != NULL);

    printf("\n[YF] OUTPUT (%s):\n", __func__);

    if (tex == NULL) {
        size_t n = yf_dict_getlen(imgs_);
        printf(" imgs (%zu):\n", n);

        yf_iter_t it = YF_NILIT;
        kv_t *kv;
        while ((kv = yf_dict_next(imgs_, &it, NULL)) != NULL) {
            int pixfmt;
            yf_dim3_t dim;
            unsigned lays, lvls, spls;
            yf_image_getval(kv->val.img, &pixfmt, &dim, &lays, &lvls, &spls);

            printf("  img <%p>:\n"
                   "   image obj.:\n"
                   "    pixfmt: %d\n"
                   "    dim: %ux%ux%u\n"
                   "    layers: %u\n"
                   "    levels: %u\n"
                   "    samples: %u\n"
                   "   layer count: %u\n"
                   "   layer index: %u\n"
                   "   layer state:\n",
                   (void *)&kv->val, pixfmt, dim.width, dim.height, dim.depth,
                   lays, lvls, spls, kv->val.lay_n, kv->val.lay_i);

            for (size_t i = 0; i < lays; i++)
                printf("    [%.3zu] %s\n", i, kv->val.lay_used[i] ? "X" : "");
        }

    } else {
        printf(" texture <%p>:\n"
               "  img: %p\n"
               "  layer: %u\n"
               "  sampler:\n"
               "   wrapmode:\n"
               "    u: %d\n"
               "    v: %d\n"
               "    w: %d\n"
               "   filter:\n"
               "    mag: %d\n"
               "    min: %d\n"
               "    mipmap: %d\n"
               "  uvset: %d\n",
               (void *)tex, (void *)(tex->img), tex->layer,
               tex->splr.wrapmode.u, tex->splr.wrapmode.v,
               tex->splr.wrapmode.w, tex->splr.filter.mag,
               tex->splr.filter.min, tex->splr.filter.mipmap, tex->uvset);
    }

    puts("");
}

#endif
