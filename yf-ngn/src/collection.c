/*
 * YF
 * collection.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "yf/com/yf-dict.h"
#include "yf/com/yf-error.h"

#include "yf-collection.h"
#include "mesh.h"
#include "texture.h"
#include "font.h"

struct YF_collection_o {
    YF_dict res[YF_COLLRES_N];
    size_t n;
};

/* Deinitializes a collection resource. */
static int deinit_res(void *key, void *val, void *arg)
{
    switch ((size_t)arg) {
    case YF_COLLRES_MESH:
        yf_mesh_deinit(val);
        break;
    case YF_COLLRES_TEXTURE:
        yf_texture_deinit(val);
        break;
    case YF_COLLRES_FONT:
        yf_font_deinit(val);
        break;
    default:
        assert(0);
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

    for (size_t i = 0; i < YF_COLLRES_N; i++) {
        coll->res[i] = yf_dict_init(yf_hashstr, yf_cmpstr);

        if (coll->res[i] == NULL) {
            yf_collection_deinit(coll);
            return NULL;
        }
    }

    if (pathname != NULL) {
        /* TODO */
        assert(0);
    }

    return coll;
}

void *yf_collection_getres(YF_collection coll, int collres, const char *name)
{
    assert(coll != NULL);
    assert(collres >= 0 && collres < YF_COLLRES_N);
    assert(name != NULL);

    return yf_dict_search(coll->res[collres], name);
}

int yf_collection_manage(YF_collection coll, int collres, const char *name,
                         void *res)
{
    assert(coll != NULL);
    assert(collres >= 0 && collres < YF_COLLRES_N);
    assert(name != NULL);
    assert(res != NULL);

    char *key = malloc(1+strlen(name));

    if (key == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }

    if (yf_dict_insert(coll->res[collres], strcpy(key, name), res) != 0) {
        free(key);
        return -1;
    }

    coll->n++;
    return 0;
}

int yf_collection_contains(YF_collection coll, int collres, const char *name)
{
    assert(coll != NULL);
    assert(collres >= 0 && collres < YF_COLLRES_N);
    assert(name != NULL);

    return yf_dict_contains(coll->res[collres], name);
}

void yf_collection_deinit(YF_collection coll)
{
    if (coll == NULL)
        return;

    for (size_t i = 0; i < YF_COLLRES_N; i++) {
        if (coll->res[i] == NULL)
            continue;

        yf_dict_each(coll->res[i], deinit_res, (void *)i);
        yf_dict_deinit(coll->res[i]);
    }

    free(coll);
}
