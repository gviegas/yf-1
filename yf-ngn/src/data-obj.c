/*
 * YF
 * data-obj.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "yf/com/yf-dict.h"
#include "yf/com/yf-error.h"

#include "data-obj.h"
#include "vertex.h"

#undef YF_INITCAP
#define YF_INITCAP 1024

#if !defined(_POSIX_C_SOURCE) || (_POSIX_C_SOURCE < 200809L)
/* TODO: Provide an implementation of 'getline()'. */
# error "Invalid platform"
#endif

/* Type defining key/value pair for the vertex map. */
typedef struct {
    unsigned key[3];
    unsigned val;
} T_kv;

/* Hashes a 'T_kv'. */
static size_t hash_kv(const void *x)
{
    const T_kv *kv = x;
    return yf_hashv(kv->key, sizeof kv->key, NULL);
}

/* Compares a 'T_kv' to another. */
static int cmp_kv(const void *a, const void *b)
{
    const T_kv *kv1 = a;
    const T_kv *kv2 = b;

    return memcmp(kv1->key, kv2->key, sizeof kv1->key);
}

/* Deallocates a 'T_kv'. */
static int dealloc_kv(YF_UNUSED void *key, void *val, YF_UNUSED void *arg)
{
    free(val);
    return 0;
}

int yf_loadobj(const char *pathname, YF_meshdt *data)
{
    assert(data != NULL);

    if (pathname == NULL) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    YF_vec3 *poss = NULL;
    YF_vec2 *texs = NULL;
    YF_vec3 *norms = NULL;
    YF_vmdl *verts = NULL;
    unsigned *inds = NULL;
    size_t pos_n = 0;
    size_t tex_n = 0;
    size_t norm_n = 0;
    size_t vtx_n = 0;
    size_t idx_n = 0;
    size_t pos_cap = 0;
    size_t tex_cap = 0;
    size_t norm_cap = 0;
    size_t vtx_cap = 0;
    size_t idx_cap = 0;

    const char *fmt_v = "v %f %f %f";
    const char *fmt_vt = "vt %f %f";
    const char *fmt_vn = "vn %f %f %f";
    const char *fmt_f_ptn = "f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d";
    const char *fmt_f_pt = "f %d/%d %d/%d %d/%d %d/%d";
    const char *fmt_f_pn = "f %d//%d %d//%d %d//%d %d//%d";
    const char *fmt_f_p = "f %d %d %d %d";
    float v[3];
    float vt[2];
    float vn[3];
    int f[12];
#define YF_FACEFMT_PTN 1
#define YF_FACEFMT_PT  2
#define YF_FACEFMT_PN  3
#define YF_FACEFMT_P   4

    YF_dict map = yf_dict_init(hash_kv, cmp_kv);

    if (map == NULL)
        return -1;

    FILE *file = fopen(pathname, "r");

    if (file == NULL) {
        yf_seterr(YF_ERR_NOFILE, __func__);
        yf_dict_deinit(map);
        return -1;
    }

    char *line = NULL;
    size_t line_sz = 0;
    int r = -1;

    while (getline(&line, &line_sz, file) != -1) {
        if (sscanf(line, fmt_v, v, v+1, v+2) == 3) {
            /* position */
            if (pos_n == pos_cap) {
                pos_cap = pos_n == 0 ? YF_INITCAP : pos_cap << 1;
                YF_vec3 *tmp = realloc(poss, pos_cap * sizeof *poss);

                if (tmp == NULL) {
                    yf_seterr(YF_ERR_NOMEM, __func__);
                    goto dealloc;
                }

                poss = tmp;
            }

            memcpy(poss+pos_n, v, sizeof v);
            pos_n++;

        } else if (sscanf(line, fmt_vt, vt, vt+1) == 2) {
            /* tex. coord. */
            if (tex_n == tex_cap) {
                tex_cap = tex_n == 0 ? YF_INITCAP : tex_cap << 1;
                YF_vec2 *tmp = realloc(texs, tex_cap * sizeof *texs);

                if (tmp == NULL) {
                    yf_seterr(YF_ERR_NOMEM, __func__);
                    goto dealloc;
                }

                texs = tmp;
            }

            memcpy(texs+tex_n, vt, sizeof vt);
            tex_n++;

        } else if (sscanf(line, fmt_vn, vn, vn+1, vn+2) == 3) {
            /* normal */
            if (norm_n == norm_cap) {
                norm_cap = norm_n == 0 ? YF_INITCAP : norm_cap << 1;
                YF_vec3 *tmp = realloc(norms, norm_cap * sizeof *norms);

                if (tmp == NULL) {
                    yf_seterr(YF_ERR_NOMEM, __func__);
                    goto dealloc;
                }

                norms = tmp;
            }

            memcpy(norms+norm_n, vn, sizeof vn);
            norm_n++;

        } else {
            /* face or ignored */
            int n;
            int fmt;
            n = sscanf(line, fmt_f_ptn,
                       f, f+1, f+2, f+3, f+4, f+5, f+6, f+7, f+8, f+9,
                       f+10, f+11);

            if (n == 9 || n == 12) {
                fmt = YF_FACEFMT_PTN;
            } else {
                n = sscanf(line, fmt_f_pt,
                           f, f+1, f+2, f+3, f+4, f+5, f+6, f+7);

                if (n == 6 || n == 8) {
                    fmt = YF_FACEFMT_PT;
                } else {
                    n = sscanf(line, fmt_f_pn,
                               f, f+1, f+2, f+3, f+4, f+5, f+6, f+7);

                    if (n == 6 || n == 8) {
                        fmt = YF_FACEFMT_PN;
                    } else {
                        n = sscanf(line, fmt_f_p, f, f+1, f+2, f+3);

                        if (n == 3 || n == 4)
                            fmt = YF_FACEFMT_P;
                        else
                            /* something else */
                            continue;
                    }
                }
            }

            /* quads will need two triangles and thus, six indices */
            if (idx_n + 6 > idx_cap) {
                idx_cap = idx_n == 0 ? YF_INITCAP : idx_cap << 1;
                unsigned *tmp = realloc(inds, idx_cap * sizeof *inds);

                if (tmp == NULL) {
                    yf_seterr(YF_ERR_NOMEM, __func__);
                    goto dealloc;
                }

                inds = tmp;
            }

            T_kv key = {0};
            T_kv *kv;
            unsigned face[4];

            switch (fmt) {
            case YF_FACEFMT_PTN:
                /* pos/texc/norm triangle or quad */
                for (int i = 0; i < n/3; i++) {
                    key.key[0] = f[3*i] - 1;
                    key.key[1] = f[3*i+1] - 1;
                    key.key[2] = f[3*i+2] - 1;

                    if ((kv = yf_dict_search(map, &key)) != NULL) {
                        face[i] = kv->val;

                    } else {
                        if (vtx_n == vtx_cap) {
                            vtx_cap = vtx_n == 0 ? YF_INITCAP : vtx_cap << 1;
                            YF_vmdl *tmp = realloc(verts,
                                                   vtx_cap * sizeof *verts);
                            if (tmp == NULL) {
                                yf_seterr(YF_ERR_NOMEM, __func__);
                                goto dealloc;
                            }

                            verts = tmp;
                        }

                        yf_vec3_copy(verts[vtx_n].pos, poss[key.key[0]]);
                        yf_vec2_copy(verts[vtx_n].tc, texs[key.key[1]]);
                        yf_vec3_copy(verts[vtx_n].norm, norms[key.key[2]]);
                        kv = malloc(sizeof *kv);

                        if (kv == NULL) {
                            yf_seterr(YF_ERR_NOMEM, __func__);
                            goto dealloc;
                        }

                        *kv = key;
                        kv->val = vtx_n++;

                        if (yf_dict_insert(map, kv, kv) != 0) {
                            free(kv);
                            goto dealloc;
                        }

                        face[i] = kv->val;
                    }
                }

                inds[idx_n++] = face[0];
                inds[idx_n++] = face[1];
                inds[idx_n++] = face[2];

                if (n == 12) {
                    inds[idx_n++] = face[0];
                    inds[idx_n++] = face[2];
                    inds[idx_n++] = face[3];
                }

                break;

            case YF_FACEFMT_PT:
                /* pos/texc triangle or quad */
                for (int i = 0; i < (n>>1); i++) {
                    key.key[0] = f[2*i] - 1;
                    key.key[1] = f[2*i+1] - 1;

                    if ((kv = yf_dict_search(map, &key)) != NULL) {
                        face[i] = kv->val;

                    } else {
                        if (vtx_n == vtx_cap) {
                            vtx_cap = vtx_n == 0 ? YF_INITCAP : vtx_cap << 1;
                            YF_vmdl *tmp = realloc(verts,
                                                   vtx_cap * sizeof *verts);
                            if (tmp == NULL) {
                                yf_seterr(YF_ERR_NOMEM, __func__);
                                goto dealloc;
                            }

                            verts = tmp;
                        }

                        yf_vec3_copy(verts[vtx_n].pos, poss[key.key[0]]);
                        yf_vec2_copy(verts[vtx_n].tc, texs[key.key[1]]);
                        yf_vec3_set(verts[vtx_n].norm, 0.0);
                        kv = malloc(sizeof *kv);

                        if (kv == NULL) {
                            yf_seterr(YF_ERR_NOMEM, __func__);
                            goto dealloc;
                        }

                        *kv = key;
                        kv->val = vtx_n++;

                        if (yf_dict_insert(map, kv, kv) != 0) {
                            free(kv);
                            goto dealloc;
                        }

                        face[i] = kv->val;
                    }
                }

                inds[idx_n++] = face[0];
                inds[idx_n++] = face[1];
                inds[idx_n++] = face[2];

                if (n == 8) {
                    inds[idx_n++] = face[0];
                    inds[idx_n++] = face[2];
                    inds[idx_n++] = face[3];
                }

                break;

            case YF_FACEFMT_PN:
                /* pos/norm triangle or quad */
                for (int i = 0; i < (n>>1); i++) {
                    key.key[0] = f[2*i] - 1;
                    key.key[2] = f[2*i+1] - 1;

                    if ((kv = yf_dict_search(map, &key)) != NULL) {
                        face[i] = kv->val;

                    } else {
                        if (vtx_n == vtx_cap) {
                            vtx_cap = vtx_n == 0 ? YF_INITCAP : vtx_cap << 1;
                            YF_vmdl *tmp = realloc(verts,
                                                   vtx_cap * sizeof *verts);
                            if (tmp == NULL) {
                                yf_seterr(YF_ERR_NOMEM, __func__);
                                goto dealloc;
                            }

                            verts = tmp;
                        }

                        yf_vec3_copy(verts[vtx_n].pos, poss[key.key[0]]);
                        yf_vec2_set(verts[vtx_n].tc, 0.0);
                        yf_vec3_copy(verts[vtx_n].norm, norms[key.key[2]]);
                        kv = malloc(sizeof *kv);

                        if (kv == NULL) {
                            yf_seterr(YF_ERR_NOMEM, __func__);
                            goto dealloc;
                        }

                        *kv = key;
                        kv->val = vtx_n++;

                        if (yf_dict_insert(map, kv, kv) != 0) {
                            free(kv);
                            goto dealloc;
                        }

                        face[i] = kv->val;
                    }
                }

                inds[idx_n++] = face[0];
                inds[idx_n++] = face[1];
                inds[idx_n++] = face[2];

                if (n == 8) {
                    inds[idx_n++] = face[0];
                    inds[idx_n++] = face[2];
                    inds[idx_n++] = face[3];
                }

                break;

            case YF_FACEFMT_P:
                /* position only triangle or quad */
                for (int i = 0; i < n; i++) {
                    key.key[0] = f[i] - 1;

                    if ((kv = yf_dict_search(map, &key)) != NULL) {
                        face[i] = kv->val;

                    } else {
                        if (vtx_n == vtx_cap) {
                            vtx_cap = vtx_n == 0 ? YF_INITCAP : vtx_cap << 1;
                            YF_vmdl *tmp = realloc(verts,
                                                   vtx_cap * sizeof *verts);
                            if (tmp == NULL) {
                                yf_seterr(YF_ERR_NOMEM, __func__);
                                goto dealloc;
                            }

                            verts = tmp;
                        }

                        yf_vec3_copy(verts[vtx_n].pos, poss[key.key[0]]);
                        yf_vec2_set(verts[vtx_n].tc, 0.0);
                        yf_vec3_set(verts[vtx_n].norm, 0.0);
                        kv = malloc(sizeof *kv);

                        if (kv == NULL) {
                            yf_seterr(YF_ERR_NOMEM, __func__);
                            goto dealloc;
                        }

                        *kv = key;
                        kv->val = vtx_n++;

                        if (yf_dict_insert(map, kv, kv) != 0) {
                            free(kv);
                            goto dealloc;
                        }

                        face[i] = kv->val;
                    }
                }

                inds[idx_n++] = face[0];
                inds[idx_n++] = face[1];
                inds[idx_n++] = face[2];

                if (n == 4) {
                    inds[idx_n++] = face[0];
                    inds[idx_n++] = face[2];
                    inds[idx_n++] = face[3];
                }

                break;
            }
        }
    }

    /* vertex data */
    if (vtx_n < vtx_cap) {
        YF_vmdl *tmp = realloc(verts, vtx_n * sizeof *verts);
        if (tmp != NULL)
            verts = tmp;
    }

    data->v.vtype = YF_VTYPE_MDL;
    data->v.data = verts;
    data->v.n = vtx_n;
    verts = NULL;

    /* index data */
    if (vtx_n > 65535) {
        if (idx_n < idx_cap) {
            unsigned *tmp = realloc(inds, idx_n * sizeof *inds);
            if (tmp != NULL)
                inds = tmp;
        }

        data->i.data = inds;
        data->i.stride = sizeof *inds;

    } else {
        unsigned short *new_inds = malloc(idx_n * sizeof(unsigned short));
        if (new_inds != NULL) {
            for (unsigned i = 0; i < idx_n; i++)
                new_inds[i] = inds[i];

            free(inds);
            data->i.data = new_inds;
            data->i.stride = sizeof *new_inds;

        } else {
            data->i.data = inds;
            data->i.stride = sizeof *inds;
        }
    }

    data->i.n = idx_n;
    inds = NULL;

    r = 0;

dealloc:
    free(poss);
    free(texs);
    free(norms);
    free(verts);
    free(inds);
    yf_dict_each(map, dealloc_kv, NULL);
    yf_dict_deinit(map);
    fclose(file);
    free(line);

    return r;
}
