/*
 * YF
 * texture.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "yf/com/yf-dict.h"
#include "yf/com/yf-error.h"
#include "yf/core/yf-image.h"

#include "texture.h"
#include "coreobj.h"
#include "data-png.h"

/* TODO: Thread-safe. */

/* XXX */
#define YF_LAYCAP 64

/* Type defining an image object and associated layer state. */
/* XXX: This assumes that a single image object will suffice. */
typedef struct {
    YF_image img;
    char *lay_used;
    unsigned lay_n;
    unsigned lay_i;
} T_imge;

/* Type defining a key for the dictionary of 'T_imge' values. */
/* TODO: Add levels and samples as key. */
typedef struct {
    int pixfmt;
    YF_dim2 dim;
} T_key;

/* Type defining key/value pair for the dictionary of 'T_imge' values. */
typedef struct {
    T_key key;
    T_imge val;
} T_kv;

struct YF_texture_o {
    T_imge *imge;
    unsigned layer;
};

/* Global context. */
static YF_context l_ctx = NULL;

/* Dictionary containing all created images. */
static YF_dict l_imges = NULL;

/* Copies texture data to image and updates texture object. */
static int copy_data(YF_texture tex, const YF_texdt *data)
{
    const T_key key = {data->pixfmt, data->dim};
    T_kv *kv = yf_dict_search(l_imges, &key);

    YF_dim3 dim;
    unsigned layers;
    T_imge *val;

    if (kv == NULL) {
        /* new image */
        if ((kv = malloc(sizeof *kv)) == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
        }

        val = &kv->val;
        dim = (YF_dim3){data->dim.width, data->dim.height, 1};

        val->img = yf_image_init(l_ctx, data->pixfmt, dim, YF_LAYCAP, 1, 1);
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

        if (yf_dict_insert(l_imges, &kv->key, kv) != 0) {
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
            YF_image new_img = yf_image_init(l_ctx, pixfmt, dim, new_lay_cap,
                                             levels, samples);

            if (new_img == NULL)
                return -1;

            YF_cmdbuf cb;

            if ((cb = yf_cmdbuf_get(l_ctx, YF_CMDBUF_GRAPH)) == NULL &&
                (cb = yf_cmdbuf_get(l_ctx, YF_CMDBUF_COMP)) == NULL) {
                yf_image_deinit(new_img);
                return -1;
            }

            const YF_off3 off = {0};
            yf_cmdbuf_copyimg(cb, new_img, off, 0, 0, val->img, off, 0, 0,
                              dim, layers);

            if (yf_cmdbuf_end(cb) != 0 || yf_cmdbuf_exec(l_ctx) != 0) {
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

    const YF_off3 off = {0};
    if (yf_image_copy(val->img, off, dim, layer, 0, data->data) != 0) {
        if (val->lay_n == 0) {
            yf_dict_remove(l_imges, &key);
            yf_image_deinit(val->img);
            free(val->lay_used);
            free(kv);
        }
        return -1;
    }

    val->lay_used[layer] = 1;
    val->lay_n++;
    val->lay_i = (layer+1) % layers;
    tex->imge = val;
    tex->layer = layer;

    return 0;
}

/* Hashes a 'T_key'. */
static size_t hash_key(const void *x)
{
    static_assert(sizeof(T_key) == 3*sizeof(int), "!sizeof");
    return yf_hashv(x, sizeof(T_key), NULL);
}

/* Compares a 'T_key' to another. */
static int cmp_key(const void *a, const void *b)
{
    const T_key *k1 = a;
    const T_key *k2 = b;

    return k1->pixfmt != k2->pixfmt ||
           k1->dim.width != k2->dim.width ||
           k1->dim.height != k2->dim.height;
}

YF_texture yf_texture_init(int filetype, const char *pathname)
{
    YF_texdt data = {0};

    switch (filetype) {
    case YF_FILETYPE_INTERNAL:
        /* TODO */
        assert(0);
        return NULL;
    case YF_FILETYPE_PNG:
        if (yf_loadpng(pathname, &data) != 0)
            return NULL;
        break;
    default:
        yf_seterr(YF_ERR_INVARG, __func__);
        return NULL;
    }

    YF_texture tex = yf_texture_initdt(&data);
    free(data.data);

    return tex;
}

YF_dim2 yf_texture_getdim(YF_texture tex)
{
    assert(tex != NULL);

    YF_dim3 dim;
    yf_image_getval(tex->imge->img, NULL, &dim, NULL, NULL, NULL);
    return (YF_dim2){dim.width, dim.height};
}

void yf_texture_deinit(YF_texture tex)
{
    if (tex == NULL)
        return;

    YF_dim3 dim;
    int pixfmt;
    yf_image_getval(tex->imge->img, &pixfmt, &dim, NULL, NULL, NULL);

    const T_key key = {pixfmt, {dim.width, dim.height}};
    T_kv *kv = yf_dict_search(l_imges, &key);

    assert(kv != NULL);

    if (kv->val.lay_n > 1) {
        kv->val.lay_used[tex->layer] = 0;
        kv->val.lay_n--;
    } else {
        yf_dict_remove(l_imges, &key);
        yf_image_deinit(kv->val.img);
        free(kv->val.lay_used);
        free(kv);
    }

    free(tex);
}

YF_texture yf_texture_initdt(const YF_texdt *data)
{
    assert(data != NULL);

    if (l_ctx == NULL && (l_ctx = yf_getctx()) == NULL)
        return NULL;
    if (l_imges == NULL && (l_imges = yf_dict_init(hash_key, cmp_key)) == NULL)
        return NULL;

    YF_texture tex = calloc(1, sizeof(struct YF_texture_o));
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

int yf_texture_setdata(YF_texture tex, YF_off2 off, YF_dim2 dim,
                       const void *data)
{
    assert(tex != NULL);
    assert(data != NULL);

    const YF_off3 off3 = {off.x, off.y, 0};
    const YF_dim3 dim3 = {dim.width, dim.height, 1};
    /* TODO: Mip level. */
    return yf_image_copy(tex->imge->img, off3, dim3, tex->layer, 0, data);
}

int yf_texture_copyres(YF_texture tex, YF_dtable dtb, unsigned alloc_i,
                       unsigned binding, unsigned element)
{
    assert(tex != NULL);
    assert(dtb != NULL);

    YF_slice elem = {element, 1};
    return yf_dtable_copyimg(dtb, alloc_i, binding, elem,
                             &tex->imge->img, &tex->layer);
}
