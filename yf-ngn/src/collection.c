/*
 * YF
 * collection.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "yf/com/yf-dict.h"
#include "yf/com/yf-error.h"

#include "yf-collection.h"
#include "yf-scene.h"
#include "yf-node.h"
#include "yf-mesh.h"
#include "yf-skin.h"
#include "yf-material.h"
#include "yf-texture.h"
#include "yf-animation.h"
#include "yf-font.h"
#include "data-gltf.h"
#include "data-png.h"
#include "data-sfnt.h"

struct YF_collection_o {
    YF_dict items[YF_CITEM_N];
    size_t n;
    unsigned ids[YF_CITEM_N];
};

/* Deinitializes a collection's item. */
static int deinit_item(void *key, void *val, void *arg)
{
    switch ((size_t)arg) {
    case YF_CITEM_SCENE:
        yf_scene_deinit(val);
        break;
    case YF_CITEM_NODE:
        yf_node_deinit(val);
        break;
    case YF_CITEM_MESH:
        yf_mesh_deinit(val);
        break;
    case YF_CITEM_SKIN:
        yf_skin_deinit(val);
        break;
    case YF_CITEM_MATERIAL:
        yf_material_deinit(val);
        break;
    case YF_CITEM_TEXTURE:
        yf_texture_deinit(val);
        break;
    case YF_CITEM_ANIMATION:
        yf_animation_deinit(val);
        break;
    case YF_CITEM_FONT:
        yf_font_deinit(val);
        break;
    default:
        assert(0);
        abort();
    }

    free(key);
    return 0;
}

YF_collection yf_collection_init(const char *pathname)
{
    YF_collection coll = calloc(1, sizeof(struct YF_collection_o));
    if (coll == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }

    for (size_t i = 0; i < YF_CITEM_N; i++) {
        coll->items[i] = yf_dict_init(yf_hashstr, yf_cmpstr);
        if (coll->items[i] == NULL) {
            yf_collection_deinit(coll);
            return NULL;
        }
    }

    if (pathname != NULL) {
        YF_datac datac = {.datac = YF_DATAC_COLL, .coll = coll};
        if (yf_loadgltf(pathname, 0, &datac) != 0) {
            yf_collection_deinit(coll);
            return NULL;
        }
    }

    return coll;
}

void *yf_collection_loaditem(YF_collection coll, int citem,
                             const char *pathname, size_t index)
{
    assert(coll != NULL);
    assert(citem >= 0 && citem < YF_CITEM_N);

    if (citem == YF_CITEM_TEXTURE) {
        YF_texdt data = {0};
        if (yf_loadpng(pathname, &data) == 0) {
            YF_texture tex = yf_texture_init(&data);
            free(data.data);
            if (tex == NULL)
                return NULL;
            if (yf_collection_manage(coll, YF_CITEM_TEXTURE, NULL, tex) != 0) {
                yf_texture_deinit(tex);
                return NULL;
            }
            return tex;
        }
        /* try 'loadgltf()' otherwise */

    } else if (citem == YF_CITEM_FONT) {
        YF_fontdt data = {0};
        if (yf_loadsfnt(pathname, &data) == 0) {
            YF_font font = yf_font_init(&data);
            if (font == NULL) {
                if (data.deinit != NULL)
                    data.deinit(data.font);
                return NULL;
            }
            if (yf_collection_manage(coll, YF_CITEM_FONT, NULL, font) != 0) {
                if (data.deinit != NULL)
                    data.deinit(data.font);
                yf_font_deinit(font);
                return NULL;
            }
            return font;
        }
        /* TODO: 'loadgltf()' font loading. */
        return NULL;
    }

    YF_datac datac;
    datac.coll = coll;

    switch (citem) {
    case YF_CITEM_SCENE:
        datac.datac = YF_DATAC_SCN;
        if (yf_loadgltf(pathname, index, &datac) == 0)
            return datac.scn;
        break;
    case YF_CITEM_NODE:
        datac.datac = YF_DATAC_NODE;
        if (yf_loadgltf(pathname, index, &datac) == 0)
            return datac.node;
        break;
    case YF_CITEM_MESH:
        datac.datac = YF_DATAC_MESH;
        if (yf_loadgltf(pathname, index, &datac) == 0)
            return datac.mesh;
        break;
    case YF_CITEM_SKIN:
        datac.datac = YF_DATAC_SKIN;
        if (yf_loadgltf(pathname, index, &datac) == 0)
            return datac.skin;
        break;
    case YF_CITEM_MATERIAL:
        datac.datac = YF_DATAC_MATL;
        if (yf_loadgltf(pathname, index, &datac) == 0)
            return datac.matl;
        break;
    case YF_CITEM_TEXTURE:
        datac.datac = YF_DATAC_TEX;
        if (yf_loadgltf(pathname, index, &datac) == 0)
            return datac.tex;
        break;
    case YF_CITEM_ANIMATION:
        datac.datac = YF_DATAC_ANIM;
        if (yf_loadgltf(pathname, index, &datac) == 0)
            return datac.anim;
        break;
#if 0
    case YF_CITEM_FONT:
        /* TODO */
        break;
#endif
    default:
        abort();
    }

    return NULL;
}

void *yf_collection_getitem(YF_collection coll, int citem, const char *name)
{
    assert(coll != NULL);
    assert(citem >= 0 && citem < YF_CITEM_N);
    assert(name != NULL);

    return yf_dict_search(coll->items[citem], name);
}

int yf_collection_manage(YF_collection coll, int citem, const char *name,
                         void *item)
{
    assert(coll != NULL);
    assert(citem >= 0 && citem < YF_CITEM_N);
    assert(item != NULL);

    char *key;
    if (name == NULL) {
        /* generate name to use as key */
        key = malloc(14);
        if (key == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
        }
        snprintf(key, 14, "unnamed-%05x", ++coll->ids[citem] & 1048575);
        key[13] = '\0';
    } else {
        /* use provided name as key */
        key = malloc(1+strlen(name));
        if (key == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
        }
        strcpy(key, name);
    }

    if (yf_dict_insert(coll->items[citem], key, item) != 0) {
        free(key);
        return -1;
    }

    coll->n++;
    return 0;
}

void *yf_collection_release(YF_collection coll, int citem, const char *name)
{
    assert(coll != NULL);
    assert(citem >= 0 && citem < YF_CITEM_N);
    assert(name != NULL);

    void *key = (void *)name;
    void *val = yf_dict_delete(coll->items[citem], &key);

    if (val != NULL) {
        assert(key != name);

        free(key);
        coll->n--;
    }

    return val;
}

int yf_collection_contains(YF_collection coll, int citem, const char *name)
{
    assert(coll != NULL);
    assert(citem >= 0 && citem < YF_CITEM_N);
    assert(name != NULL);

    return yf_dict_contains(coll->items[citem], name);
}

void yf_collection_each(YF_collection coll, int citem,
                        int (*callb)(const char *name, void *item, void *arg),
                        void *arg)
{
    assert(coll != NULL);
    assert(citem >= 0 && citem < YF_CITEM_N);
    assert(callb != NULL);

    YF_iter it = YF_NILIT;
    void *name, *item;
    /* XXX: Insertion of 'NULL' item is guarded by an assertion. */
    while ((item = yf_dict_next(coll->items[citem], &it, &name)) != NULL &&
           callb(name, item, arg) == 0)
        ;
}

void yf_collection_deinit(YF_collection coll)
{
    if (coll == NULL)
        return;

    for (size_t i = 0; i < YF_CITEM_N; i++) {
        if (coll->items[i] == NULL)
            continue;

        yf_dict_each(coll->items[i], deinit_item, (void *)i);
        yf_dict_deinit(coll->items[i]);
    }

    free(coll);
}

/*
 * DEVEL
 */

#ifdef YF_DEVEL

void yf_print_coll(YF_collection coll)
{
    assert(coll != NULL);

    printf("\n[YF] OUTPUT (%s):\n"
           " collection:\n"
           "  number of items: %zu\n"
           "  items:\n",
           __func__, coll->n);

    const char *cnames[YF_CITEM_N] = {
        [YF_CITEM_SCENE]     = "CITEM_SCENE",
        [YF_CITEM_NODE]      = "CITEM_NODE",
        [YF_CITEM_MESH]      = "CITEM_MESH",
        [YF_CITEM_SKIN]      = "CITEM_SKIN",
        [YF_CITEM_MATERIAL]  = "CITEM_MATERIAL",
        [YF_CITEM_TEXTURE]   = "CITEM_TEXTURE",
        [YF_CITEM_ANIMATION] = "CITEM_ANIMATION",
        [YF_CITEM_FONT]      = "CITEM_FONT"
    };

    for (size_t i = 0; i < YF_CITEM_N; i++) {
        printf("   %s (%zu):\n", cnames[i], yf_dict_getlen(coll->items[i]));

        YF_iter it = YF_NILIT;
        void *val, *key;
        for (;;) {
            val = yf_dict_next(coll->items[i], &it, &key);
            if (YF_IT_ISNIL(it))
                break;
            printf("    '%s', %p\n", (char *)key, val);
        }
    }

    puts("");
}

#endif
