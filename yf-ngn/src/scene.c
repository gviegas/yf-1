/*
 * YF
 * scene.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

/* XXX */
#define YF_SCN_DYNAMIC

#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "yf/com/yf-util.h"
#include "yf/com/yf-list.h"
#include "yf/com/yf-dict.h"
#include "yf/com/yf-error.h"
#include "yf/core/yf-cmdbuf.h"
#include "yf/core/yf-limits.h"

#include "scene.h"
#include "coreobj.h"
#include "resmgr.h"
#include "node.h"
#include "mesh.h"
#include "texture.h"
#include "yf-model.h"
#include "yf-terrain.h"
#include "yf-particle.h"
#include "yf-quad.h"
#include "yf-label.h"
#include "yf-light.h"

#if defined(YF_DEVEL) && defined(YF_PRINT)
# include <stdio.h>
# include "../test/print.h"
#endif

#ifndef YF_SCN_DYNAMIC
# ifndef YF_SCN_MDLN
#  define YF_SCN_MDLN 64
# endif
# ifndef YF_SCN_MDL2N
#  define YF_SCN_MDL2N 16
# endif
# ifndef YF_SCN_MDL4N
#  define YF_SCN_MDL4N 16
# endif
# ifndef YF_SCN_MDL8N
#  define YF_SCN_MDL8N 16
# endif
# ifndef YF_SCN_MDL16N
#  define YF_SCN_MDL16N 8
# endif
# ifndef YF_SCN_MDL32N
#  define YF_SCN_MDL32N 8
# endif
# ifndef YF_SCN_MDL64N
#  define YF_SCN_MDL64N 4
# endif
# ifndef YF_SCN_TERRN
#  define YF_SCN_TERRN 4
# endif
# ifndef YF_SCN_PARTN
#  define YF_SCN_PARTN 64
# endif
# ifndef YF_SCN_QUADN
#  define YF_SCN_QUADN 32
# endif
# ifndef YF_SCN_LABLN
#  define YF_SCN_LABLN 64
# endif
#endif /* !YF_SCN_DYNAMIC */

#define YF_CAMORIG (yf_vec3_t){-20.0f, 20.0f, 20.0f}
#define YF_CAMTGT  (yf_vec3_t){0.0f, 0.0f, 0.0f}
#define YF_CAMASP  1.0f

/* TODO: These should be defined elsewhere. */
#define YF_LIGHTN 16
#define YF_JOINTN 64

#define YF_GLOBLSZ     ((sizeof(yf_mat4_t) << 2) + 32)
#define YF_LIGHTSZ     ((sizeof(yf_vec4_t) << 2) * YF_LIGHTN)
#define YF_INSTSZ_MDL  ((sizeof(yf_mat4_t) * 3) + \
                        (sizeof(yf_mat4_t) * (YF_JOINTN << 1)))
#define YF_INSTSZ_TERR (sizeof(yf_mat4_t) << 1)
#define YF_INSTSZ_PART (sizeof(yf_mat4_t) << 1)
#define YF_INSTSZ_QUAD ((sizeof(yf_mat4_t) << 1) + 16)
#define YF_INSTSZ_LABL ((sizeof(yf_mat4_t) << 1) + 16)
#define YF_MATLSZ      (sizeof(yf_vec4_t) << 2)

#define YF_PEND_NONE 0
#define YF_PEND_MDL  0x01
#define YF_PEND_TERR 0x02
#define YF_PEND_PART 0x04
#define YF_PEND_QUAD 0x08
#define YF_PEND_LABL 0x10

#define YF_INSTCAP 16
static_assert(YF_INSTCAP > 1);

struct yf_scene {
    yf_node_t *node;
    yf_camera_t *cam;
    yf_color_t color;
    yf_viewport_t vport;
    yf_rect_t sciss;
};

/* Shared variables available to all scenes. */
typedef struct {
    yf_context_t *ctx;
    yf_buffer_t *buf;
    size_t buf_off;
    unsigned globlpd;
    unsigned lightpd;
    unsigned instpd_mdl;
    unsigned instpd_terr;
    unsigned instpd_part;
    unsigned instpd_quad;
    unsigned instpd_labl;
    unsigned matlpd;
    unsigned insts[YF_RESRQ_N];
    yf_list_t *res_obtd;
    yf_cmdbuf_t *cb;
    yf_dict_t *mdls;
    yf_list_t *terrs;
    yf_list_t *parts;
    yf_list_t *quads;
    yf_list_t *labls;
    yf_light_t *lights[YF_LIGHTN];
    unsigned light_n;
} vars_t;

/* Entry in the list of obtained resources. */
typedef struct {
    int resrq;
    unsigned inst_alloc;
} reso_t;

/* Key/value pair for the model dictionary. */
typedef struct {
    struct {
        yf_mesh_t *mesh;
    } key;
    union {
        yf_model_t *mdl;
        yf_model_t **mdls;
    };
    unsigned mdl_n;
    unsigned mdl_cap;
} kv_mdl_t;

/* Variables' instance. */
static vars_t vars_ = {0};

/* Traverses a scene graph to process its objects. */
static int traverse_scn(yf_node_t *node, void *arg)
{
    void *obj = NULL;
    const int nodeobj = yf_node_getobj(node, &obj);

    switch (nodeobj) {
    case YF_NODEOBJ_MODEL: {
        yf_model_t *mdl = obj;
        yf_mesh_t *mesh = yf_model_getmesh(mdl);

        kv_mdl_t key = {{mesh}, {NULL}, 0, 0};
        kv_mdl_t *val = yf_dict_search(vars_.mdls, &key);

        if (val == NULL) {
            /* new unique model */
            val = malloc(sizeof *val);
            if (val == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                *(int *)arg = -1;
                return -1;
            }

            val->key = key.key;
            if (yf_dict_insert(vars_.mdls, val, val) != 0) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                free(val);
                *(int *)arg = -1;
                return -1;
            }

            val->mdl = mdl;
            val->mdl_n = 1;
            val->mdl_cap = 1;

        } else {
            /* model with shared resources */
            if (val->mdl_n == val->mdl_cap) {
                if (val->mdl_cap == 1) {
                    yf_model_t **mdls = malloc(YF_INSTCAP * sizeof mdl);
                    if (mdls == NULL) {
                        yf_seterr(YF_ERR_NOMEM, __func__);
                        *(int *)arg = -1;
                        return -1;
                    }

                    mdls[0] = val->mdl;
                    val->mdls = mdls;
                    val->mdl_cap = YF_INSTCAP;

                } else {
                    yf_model_t **mdls =
                        realloc(val->mdls, (val->mdl_cap * sizeof mdl) << 1);
                    if (mdls == NULL) {
                        yf_seterr(YF_ERR_NOMEM, __func__);
                        *(int *)arg = -1;
                        return -1;
                    }

                    val->mdls = mdls;
                    val->mdl_cap <<= 1;
                }
            }

            val->mdls[val->mdl_n++] = mdl;
        }
    } break;

    case YF_NODEOBJ_TERRAIN:
        if (yf_list_insert(vars_.terrs, obj) != 0) {
            *(int *)arg = -1;
            return -1;
        }
        break;

    case YF_NODEOBJ_PARTICLE:
        if (yf_list_insert(vars_.parts, obj) != 0) {
            *(int *)arg = -1;
            return -1;
        }
        break;

    case YF_NODEOBJ_QUAD:
        if (yf_list_insert(vars_.quads, obj) != 0) {
            *(int *)arg = -1;
            return -1;
        }
        break;

    case YF_NODEOBJ_LABEL:
        if (yf_list_insert(vars_.labls, obj) != 0) {
            *(int *)arg = -1;
            return -1;
        }
        break;

    case YF_NODEOBJ_LIGHT:
        if (vars_.light_n >= YF_LIGHTN) {
            yf_seterr(YF_ERR_LIMIT, __func__);
            *(int *)arg = -1;
            return -1;
        }
        vars_.lights[vars_.light_n++] = obj;
        break;

    case YF_NODEOBJ_EFFECT:
        /* TODO */
        assert(0);

    default:
        /* not a drawable object */
        break;
    }

    /* transforms */
    yf_mat4_t *wld = yf_node_getwldxform(node);
    yf_mat4_t *inv = yf_node_getwldinv(node);
    yf_mat4_t *norm = yf_node_getwldnorm(node);
    yf_mat4_t *pnt = yf_node_getwldxform(yf_node_getparent(node));
    yf_mat4_t *loc = yf_node_getxform(node);
    yf_mat4_mul(*wld, *pnt, *loc);
    yf_mat4_inv(*inv, *wld);
    yf_mat4_xpose(*norm, *inv);

#if defined(YF_DEVEL) && defined(YF_PRINT)
    yf_print_nodeobj(node);
#endif

    return 0;
}

/* Creates uniform buffer and sets required resources. */
static int prepare_res(void)
{
    assert(vars_.ctx != NULL);

    /* TODO: Check limits. */

#ifdef YF_SCN_DYNAMIC
    /* dynamically allocate resources based on processed objects */
    vars_.insts[YF_RESRQ_MDL]   = 0;
    vars_.insts[YF_RESRQ_MDL2]  = 0;
    vars_.insts[YF_RESRQ_MDL4]  = 0;
    vars_.insts[YF_RESRQ_MDL8]  = 0;
    vars_.insts[YF_RESRQ_MDL16] = 0;
    vars_.insts[YF_RESRQ_MDL32] = 0;
    vars_.insts[YF_RESRQ_MDL64] = 0;
    vars_.insts[YF_RESRQ_TERR]  = yf_list_getlen(vars_.terrs);
    vars_.insts[YF_RESRQ_PART]  = yf_list_getlen(vars_.parts);
    vars_.insts[YF_RESRQ_QUAD]  = yf_list_getlen(vars_.quads);
    vars_.insts[YF_RESRQ_LABL]  = yf_list_getlen(vars_.labls);

    yf_iter_t it = YF_NILIT;
    kv_mdl_t *kv_mdl;

    while ((kv_mdl = yf_dict_next(vars_.mdls, &it, NULL)) != NULL) {
        unsigned n = kv_mdl->mdl_n;
        while (n >= 64) {
            vars_.insts[YF_RESRQ_MDL64]++;
            n -= 64;
        }
        if (n >= 32) {
            vars_.insts[YF_RESRQ_MDL32]++;
            n -= 32;
        }
        if (n >= 16) {
            vars_.insts[YF_RESRQ_MDL16]++;
            n -= 16;
        }
        if (n >= 8) {
            vars_.insts[YF_RESRQ_MDL8]++;
            n -= 8;
        }
        if (n >= 4) {
            vars_.insts[YF_RESRQ_MDL4]++;
            n -= 4;
        }
        if (n >= 2) {
            vars_.insts[YF_RESRQ_MDL2]++;
            n -= 2;
        }
        if (n == 1) {
            vars_.insts[YF_RESRQ_MDL]++;
            n--;
        }
        assert(n == 0);
    }
#else
    /* use predefined number of resource allocations */
    vars_.insts[YF_RESRQ_MDL]   = YF_SCN_MDLN;
    vars_.insts[YF_RESRQ_MDL2]  = YF_SCN_MDL2N;
    vars_.insts[YF_RESRQ_MDL4]  = YF_SCN_MDL4N;
    vars_.insts[YF_RESRQ_MDL8]  = YF_SCN_MDL8N;
    vars_.insts[YF_RESRQ_MDL16] = YF_SCN_MDL16N;
    vars_.insts[YF_RESRQ_MDL32] = YF_SCN_MDL32N;
    vars_.insts[YF_RESRQ_MDL64] = YF_SCN_MDL64N;
    vars_.insts[YF_RESRQ_TERR]  = YF_SCN_TERRN;
    vars_.insts[YF_RESRQ_PART]  = YF_SCN_PARTN;
    vars_.insts[YF_RESRQ_QUAD]  = YF_SCN_QUADN;
    vars_.insts[YF_RESRQ_LABL]  = YF_SCN_LABLN;
#endif /* YF_SCN_DYNAMIC */

    size_t inst_min = 0;
    size_t inst_sum = 0;
    for (unsigned i = 0; i < YF_RESRQ_N; i++) {
        inst_min += vars_.insts[i] != 0;
        inst_sum += vars_.insts[i];
    }

    size_t buf_sz;

    while (1) {
        int failed = 0;
        buf_sz = YF_GLOBLSZ + vars_.globlpd + YF_LIGHTSZ + vars_.lightpd;

        for (unsigned i = 0; i < YF_RESRQ_N; i++) {
            if (yf_resmgr_setallocn(i, vars_.insts[i]) != 0) {
                yf_resmgr_clear();
                failed = 1;
                break;
            }

            if (vars_.insts[i] == 0)
                continue;

            size_t inst_sz;
            size_t matl_sz;

            switch (i) {
            case YF_RESRQ_MDL:
                inst_sz = YF_INSTSZ_MDL + vars_.instpd_mdl;
                matl_sz = YF_MATLSZ + vars_.matlpd;
                break;
            case YF_RESRQ_MDL2:
                inst_sz = (YF_INSTSZ_MDL + vars_.instpd_mdl) << 1;
                matl_sz = YF_MATLSZ + vars_.matlpd;
                break;
            case YF_RESRQ_MDL4:
                inst_sz = (YF_INSTSZ_MDL + vars_.instpd_mdl) << 2;
                matl_sz = YF_MATLSZ + vars_.matlpd;
                break;
            case YF_RESRQ_MDL8:
                inst_sz = (YF_INSTSZ_MDL + vars_.instpd_mdl) << 3;
                matl_sz = YF_MATLSZ + vars_.matlpd;
                break;
            case YF_RESRQ_MDL16:
                inst_sz = (YF_INSTSZ_MDL + vars_.instpd_mdl) << 4;
                matl_sz = YF_MATLSZ + vars_.matlpd;
                break;
            case YF_RESRQ_MDL32:
                inst_sz = (YF_INSTSZ_MDL + vars_.instpd_mdl) << 5;
                matl_sz = YF_MATLSZ + vars_.matlpd;
                break;
            case YF_RESRQ_MDL64:
                inst_sz = (YF_INSTSZ_MDL + vars_.instpd_mdl) << 6;
                matl_sz = YF_MATLSZ + vars_.matlpd;
                break;
            case YF_RESRQ_TERR:
                inst_sz = YF_INSTSZ_TERR + vars_.instpd_terr;
                matl_sz = 0;
                break;
            case YF_RESRQ_PART:
                inst_sz = YF_INSTSZ_PART + vars_.instpd_part;
                matl_sz = 0;
                break;
            case YF_RESRQ_QUAD:
                inst_sz = YF_INSTSZ_QUAD + vars_.instpd_quad;
                matl_sz = 0;
                break;
            case YF_RESRQ_LABL:
                inst_sz = YF_INSTSZ_LABL + vars_.instpd_labl;
                matl_sz = 0;
                break;
            default:
                assert(0);
                abort();
            }

            buf_sz += vars_.insts[i] * (inst_sz + matl_sz);
        }

        /* proceed if all allocations succeed */
        if (!failed)
            break;

        /* give up if cannot allocate the minimum */
        if (inst_sum <= inst_min)
            return -1;

        /* try again with reduced number of instances */
        inst_sum = 0;
        for (unsigned i = 0; i < YF_RESRQ_N; i++) {
            if (vars_.insts[i] > 0) {
                vars_.insts[i] = YF_MAX(1, vars_.insts[i] >> 1);
                inst_sum += vars_.insts[i];
            }
        }
    }

    if (vars_.buf != NULL) {
        size_t cur_sz = yf_buffer_getsize(vars_.buf);
        if (cur_sz < buf_sz || (cur_sz >> 1) > buf_sz) {
            yf_buffer_deinit(vars_.buf);
            vars_.buf = yf_buffer_init(vars_.ctx, buf_sz);
        }
    } else {
        vars_.buf = yf_buffer_init(vars_.ctx, buf_sz);
    }

    if (vars_.buf == NULL)
        return -1;

    return 0;
}

/* Obtains a resource for rendering. */
static yf_gstate_t *obtain_res(int resrq, unsigned *inst_alloc)
{
    yf_gstate_t *gst = yf_resmgr_obtain(resrq, inst_alloc);
    if (gst == NULL)
        return NULL;

    reso_t *reso = malloc(sizeof *reso);
    if (reso == NULL || yf_list_insert(vars_.res_obtd, reso) != 0) {
        if (reso == NULL)
            yf_seterr(YF_ERR_NOMEM, __func__);
        else
            free(reso);
        yf_resmgr_yield(resrq, *inst_alloc);
        return NULL;
    }
    reso->resrq = resrq;
    reso->inst_alloc = *inst_alloc;

    return gst;
}

/* Copies global uniform to buffer and updates dtable. */
static int copy_globl(yf_scene_t *scn)
{
    yf_dtable_t *dtb = yf_resmgr_getglobl();
    if (dtb == NULL)
        return -1;

    const yf_slice_t elems = {0, 1};
    const size_t sz = YF_GLOBLSZ;
    size_t off = vars_.buf_off;

    /* view matrix */
    if (yf_buffer_copy(vars_.buf, vars_.buf_off,
                       *yf_camera_getview(scn->cam), sizeof(yf_mat4_t)) != 0)
        return -1;
    vars_.buf_off += sizeof(yf_mat4_t);

    /* projection matrix (persp.) */
    if (yf_buffer_copy(vars_.buf, vars_.buf_off,
                       *yf_camera_getproj(scn->cam), sizeof(yf_mat4_t)) != 0)
        return -1;
    vars_.buf_off += sizeof(yf_mat4_t);

    /* projection matrix (ortho.) */
    /* TODO: This matrix should be taken from the camera. */
    yf_mat4_t ortho;
    yf_mat4_ortho(ortho, 1.0f, 1.0f, 0.0f, -1.0f);
    if (yf_buffer_copy(vars_.buf, vars_.buf_off, ortho, sizeof ortho) != 0)
        return -1;
    vars_.buf_off += sizeof(yf_mat4_t);

    /* view-projection matrix */
    if (yf_buffer_copy(vars_.buf, vars_.buf_off,
                       *yf_camera_getxform(scn->cam), sizeof(yf_mat4_t)) != 0)
        return -1;
    vars_.buf_off += sizeof(yf_mat4_t);

    /* viewport #0 */
    if (yf_buffer_copy(vars_.buf, vars_.buf_off,
                       &scn->vport, sizeof scn->vport) != 0)
        return -1;
    vars_.buf_off += 32;

    /* copy */
    vars_.buf_off += vars_.globlpd;
    if (yf_dtable_copybuf(dtb, 0, YF_RESBIND_GLOBL, elems,
                          &vars_.buf, &off, &sz) != 0)
        return -1;

    return 0;
}

/* Copies light uniform to buffer and updates dtable. */
static int copy_light(void)
{
#define YF_TPOINT  0
#define YF_TSPOT   1
#define YF_TDIRECT 2

    yf_dtable_t *dtb = yf_resmgr_getglobl();
    if (dtb == NULL)
        return -1;

    struct {
        /* 0 */
        int unused, type; float inten, range;
        /* 16 */
        yf_vec3_t clr; float ang_scale;
        /* 32 */
        yf_vec3_t pos; float ang_off;
        /* 48 */
        yf_vec3_t dir; float pad;
    } unif[YF_LIGHTN];

    static_assert(sizeof unif == YF_LIGHTSZ, "!sizeof");

    unif[YF_MIN(YF_LIGHTN-1, vars_.light_n)].unused = 1;

    for (unsigned i = 0; i < vars_.light_n; i++) {
        unif[i].unused = 0;

        int lightt;
        float inner_angle, outer_angle;
        yf_light_getval(vars_.lights[i], &lightt, unif[i].clr, &unif[i].inten,
                        &unif[i].range, &inner_angle, &outer_angle);

        /* light type */
        switch (lightt) {
        case YF_LIGHTT_POINT:
            unif[i].type = YF_TPOINT;
            break;
        case YF_LIGHTT_SPOT:
            unif[i].type = YF_TSPOT;
            break;
        case YF_LIGHTT_DIRECT:
            unif[i].type = YF_TDIRECT;
            break;
        default:
            assert(0);
            yf_seterr(YF_ERR_INVARG, __func__);
            return -1;
        }

        /* angular attenuation data */
        if (lightt == YF_LIGHTT_SPOT) {
            const float inner_cos = cosf(inner_angle);
            const float outer_cos = cosf(outer_angle);
            const float cos_diff = inner_cos - outer_cos;
            if (cos_diff < 1.0e-6f)
                unif[i].ang_scale = 1.0e6f;
            else
                unif[i].ang_scale = 1.0f / cos_diff;
            unif[i].ang_off = unif[i].ang_scale * -outer_cos;
        }

        yf_mat4_t *m = yf_node_getwldxform(yf_light_getnode(vars_.lights[i]));

        /* position */
        if (lightt != YF_LIGHTT_DIRECT)
            yf_vec3_copy(unif[i].pos, &(*m)[12]);

        /* direction */
        if (lightt != YF_LIGHTT_POINT) {
            yf_mat3_t rs = {
                (*m)[0], (*m)[1], (*m)[2],
                (*m)[4], (*m)[5], (*m)[6],
                (*m)[8], (*m)[9], (*m)[10]
            };
            yf_vec3_t dir = {0.0f, 0.0f, -1.0f};
            yf_mat3_mulv(unif[i].dir, rs, dir);
            yf_vec3_normi(unif[i].dir);
        }
    }

    const size_t sz = YF_LIGHTSZ / YF_LIGHTN *
                      YF_MIN(vars_.light_n + 1, YF_LIGHTN);
    const yf_slice_t elems = {0, 1};

    /* copy */
    if (yf_buffer_copy(vars_.buf, vars_.buf_off, unif, sz) != 0 ||
        yf_dtable_copybuf(dtb, 0, YF_RESBIND_LIGHT, elems,
                          &vars_.buf, &vars_.buf_off, &sz) != 0)
        return -1;
    vars_.buf_off += YF_LIGHTSZ + vars_.lightpd;

    return 0;

#undef YF_TPOINT
#undef YF_TSPOT
#undef YF_TDIRECT
}

/* Copies model's instance uniform to buffer and updates dtable. */
static int copy_inst_mdl(yf_scene_t *scn, yf_model_t **mdls, unsigned mdl_n,
                         yf_dtable_t *inst_dtb, unsigned inst_alloc)
{
    const yf_mat4_t *v = yf_camera_getview(scn->cam);
    yf_node_t *node;
    yf_mat4_t mv, *m, *norm;

    const size_t off = vars_.buf_off;

    for (unsigned i = 0; i < mdl_n; i++) {
        node = yf_model_getnode(mdls[i]);
        m = yf_node_getwldxform(node);
        norm = yf_node_getwldnorm(node);
        yf_mat4_mul(mv, *v, *m);

        /* model matrix */
        if (yf_buffer_copy(vars_.buf, vars_.buf_off, *m, sizeof *m) != 0)
            return -1;
        vars_.buf_off += sizeof(yf_mat4_t);

        /* normal matrix */
        if (yf_buffer_copy(vars_.buf, vars_.buf_off, *norm,
                           sizeof *norm) != 0)
            return -1;
        vars_.buf_off += sizeof(yf_mat4_t);

        /* model-view matrix */
        if (yf_buffer_copy(vars_.buf, vars_.buf_off, mv, sizeof mv) != 0)
            return -1;
        vars_.buf_off += sizeof(yf_mat4_t);

        /* skinning matrices */
        yf_skeleton_t *skel;
        yf_skin_t *skin = yf_model_getskin(mdls[i], &skel);
        unsigned jnt_n = 0;
        yf_mat4_t jm[YF_JOINTN << 1];
        if (skin != NULL) {
            const yf_joint_t *jnts = yf_skin_getjnts(skin, &jnt_n);
            assert(jnt_n <= YF_JOINTN);
            for (unsigned j = 0; j < jnt_n; j++) {
                yf_node_t *jnt = yf_skin_getjntnode(skin, skel, j);
                assert(jnt != NULL);
                /* joint matrix */
                yf_mat4_mul(jm[j], *yf_node_getwldxform(jnt), jnts[j].ibm);
                /* joint normal matrix */
                yf_mat4_t inv;
                yf_mat4_inv(inv, jm[j]);
                yf_mat4_xpose(jm[YF_JOINTN + j], inv);
            }
        }
        /* FIXME: Setting the first set of joint and normal matrices when
           skin is not provided should suffice. */
        for (unsigned j = jnt_n; j < YF_JOINTN; j++) {
            yf_mat4_iden(jm[j]);
            yf_mat4_iden(jm[YF_JOINTN + j]);
        }
        if (yf_buffer_copy(vars_.buf, vars_.buf_off, jm, sizeof jm) != 0)
            return -1;
        vars_.buf_off += sizeof jm;
    }

    const yf_slice_t elems = {0, 1};
    const size_t sz = mdl_n * YF_INSTSZ_MDL;

    /* copy */
    vars_.buf_off += vars_.instpd_mdl * mdl_n;
    if (yf_dtable_copybuf(inst_dtb, inst_alloc, YF_RESBIND_INST, elems,
                          &vars_.buf, &off, &sz) != 0)
        return -1;

    return 0;
}

/* Copies terrain's instance uniform to buffer and updates dtable. */
static int copy_inst_terr(yf_scene_t *scn, yf_terrain_t **terrs,
                          unsigned terr_n, yf_dtable_t *inst_dtb,
                          unsigned inst_alloc)
{
    const yf_mat4_t *v = yf_camera_getview(scn->cam);
    yf_node_t *node;
    yf_mat4_t mv, *m;

    const size_t off = vars_.buf_off;

    for (unsigned i = 0; i < terr_n; i++) {
        node = yf_terrain_getnode(terrs[i]);
        m = yf_node_getwldxform(node);
        yf_mat4_mul(mv, *v, *m);

        /* model matrix */
        if (yf_buffer_copy(vars_.buf, vars_.buf_off, *m, sizeof *m) != 0)
            return -1;
        vars_.buf_off += sizeof(yf_mat4_t);

        /* model-view matrix */
        if (yf_buffer_copy(vars_.buf, vars_.buf_off, mv, sizeof mv) != 0)
            return -1;
        vars_.buf_off += sizeof(yf_mat4_t);
    }

    const yf_slice_t elems = {0, 1};
    const size_t sz = terr_n * YF_INSTSZ_TERR;

    /* copy */
    vars_.buf_off += vars_.instpd_terr * terr_n;
    if (yf_dtable_copybuf(inst_dtb, inst_alloc, YF_RESBIND_INST, elems,
                          &vars_.buf, &off, &sz) != 0)
        return -1;

    return 0;
}

/* Copies particle's instance uniform to buffer and updates dtable. */
static int copy_inst_part(yf_scene_t *scn, yf_particle_t **parts,
                          unsigned part_n, yf_dtable_t *inst_dtb,
                          unsigned inst_alloc)
{
    const yf_mat4_t *v = yf_camera_getview(scn->cam);
    yf_node_t *node;
    yf_mat4_t mv, *m;

    const size_t off = vars_.buf_off;

    for (unsigned i = 0; i < part_n; i++) {
        node = yf_particle_getnode(parts[i]);
        m = yf_node_getwldxform(node);
        yf_mat4_mul(mv, *v, *m);

        /* model matrix */
        if (yf_buffer_copy(vars_.buf, vars_.buf_off, *m, sizeof *m) != 0)
            return -1;
        vars_.buf_off += sizeof(yf_mat4_t);

        /* model-view matrix */
        if (yf_buffer_copy(vars_.buf, vars_.buf_off, mv, sizeof mv) != 0)
            return -1;
        vars_.buf_off += sizeof(yf_mat4_t);
    }

    const yf_slice_t elems = {0, 1};
    const size_t sz = part_n * YF_INSTSZ_PART;

    /* copy */
    vars_.buf_off += vars_.instpd_part * part_n;
    if (yf_dtable_copybuf(inst_dtb, inst_alloc, YF_RESBIND_INST, elems,
                          &vars_.buf, &off, &sz) != 0)
        return -1;

    return 0;
}

/* Copies quad's instance uniform to buffer and updates dtable. */
static int copy_inst_quad(yf_scene_t *scn, yf_quad_t **quads, unsigned quad_n,
                          yf_dtable_t *inst_dtb, unsigned inst_alloc)
{
    const yf_mat4_t *v = yf_camera_getview(scn->cam);
    yf_node_t *node;
    yf_mat4_t mv, *m;
    const yf_rect_t *rect;
    float dim[2];

    const size_t off = vars_.buf_off;

    for (unsigned i = 0; i < quad_n; i++) {
        node = yf_quad_getnode(quads[i]);
        m = yf_node_getwldxform(node);
        yf_mat4_mul(mv, *v, *m);
        rect = yf_quad_getrect(quads[i]);
        dim[0] = rect->size.width;
        dim[1] = rect->size.height;

        /* model matrix */
        if (yf_buffer_copy(vars_.buf, vars_.buf_off, *m, sizeof *m) != 0)
            return -1;
        vars_.buf_off += sizeof(yf_mat4_t);

        /* model-view matrix */
        if (yf_buffer_copy(vars_.buf, vars_.buf_off, mv, sizeof mv) != 0)
            return -1;
        vars_.buf_off += sizeof(yf_mat4_t);

        /* dimensions */
        if (yf_buffer_copy(vars_.buf, vars_.buf_off, dim, sizeof dim) != 0)
            return -1;
        vars_.buf_off += 16;
    }

    const yf_slice_t elems = {0, 1};
    const size_t sz = quad_n * YF_INSTSZ_QUAD;

    /* copy */
    vars_.buf_off += vars_.instpd_quad * quad_n;
    if (yf_dtable_copybuf(inst_dtb, inst_alloc, YF_RESBIND_INST, elems,
                          &vars_.buf, &off, &sz) != 0)
        return -1;

    return 0;
}

/* Copies label's instance uniform to buffer and updates dtable. */
static int copy_inst_labl(yf_scene_t *scn, yf_label_t **labls, unsigned labl_n,
                          yf_dtable_t *inst_dtb, unsigned inst_alloc)
{
    const yf_mat4_t *v = yf_camera_getview(scn->cam);
    yf_node_t *node;
    yf_mat4_t mv, *m;
    yf_dim2_t udim;
    float dim[2];

    const size_t off = vars_.buf_off;

    for (unsigned i = 0; i < labl_n; i++) {
        node = yf_label_getnode(labls[i]);
        m = yf_node_getwldxform(node);
        yf_mat4_mul(mv, *v, *m);
        udim = yf_label_getdim(labls[i]);
        dim[0] = udim.width;
        dim[1] = udim.height;

        /* model matrix */
        if (yf_buffer_copy(vars_.buf, vars_.buf_off, *m, sizeof *m) != 0)
            return -1;
        vars_.buf_off += sizeof(yf_mat4_t);

        /* model-view matrix */
        if (yf_buffer_copy(vars_.buf, vars_.buf_off, mv, sizeof mv) != 0)
            return -1;
        vars_.buf_off += sizeof(yf_mat4_t);

        /* dimensions */
        if (yf_buffer_copy(vars_.buf, vars_.buf_off, dim, sizeof dim) != 0)
            return -1;
        vars_.buf_off += 16;
    }

    const yf_slice_t elems = {0, 1};
    const size_t sz = labl_n * YF_INSTSZ_LABL;

    /* copy */
    vars_.buf_off += vars_.instpd_labl * labl_n;
    if (yf_dtable_copybuf(inst_dtb, inst_alloc, YF_RESBIND_INST, elems,
                          &vars_.buf, &off, &sz) != 0)
        return -1;

    return 0;
}

/* Copies material uniform to buffer and updates dtable. */
static int copy_matl(yf_material_t *matl, yf_dtable_t *inst_dtb,
                     unsigned inst_alloc)
{
#define YF_MPBRSG 0
#define YF_MPBRMR 1
#define YF_MUNLIT 2

#define YF_BOPAQUE 0
#define YF_BBLEND  1

#define YF_TCLR  0x01
#define YF_TPBR  0x02
#define YF_TNORM 0x04
#define YF_TOCC  0x08
#define YF_TEMIS 0x10

    struct {
        /* 0 */
        int method, blend; float norm_fac, occ_fac;
        /* 16 */
        yf_vec4_t clr_fac;
        /* 32 */
        yf_vec4_t pbr_fac;
        /* 48 */
        yf_vec3_t emis_fac; unsigned tex_mask;
    } unif;

    static_assert(sizeof unif == YF_MATLSZ, "!sizeof");

    const yf_matlprop_t *prop = yf_material_getprop(matl);
    unif.tex_mask = 0;

    /* normal map */
    if (prop->normal.tex.tex != NULL) {
        if (yf_texture_copyres2(&prop->normal.tex, inst_dtb, inst_alloc,
                                YF_RESBIND_NORM, 0) != 0)
            return -1;
        unif.tex_mask |= YF_TNORM;
    }
    unif.norm_fac = prop->normal.scale;

    /* occlusion map */
    if (prop->occlusion.tex.tex != NULL) {
        if (yf_texture_copyres2(&prop->occlusion.tex, inst_dtb, inst_alloc,
                                YF_RESBIND_OCC, 0) != 0)
            return -1;
        unif.tex_mask |= YF_TOCC;
    }
    unif.occ_fac = prop->occlusion.strength;

    /* emissive map */
    if (prop->emissive.tex.tex != NULL) {
        if (yf_texture_copyres2(&prop->emissive.tex, inst_dtb, inst_alloc,
                                YF_RESBIND_EMIS, 0) != 0)
            return -1;
        unif.tex_mask |= YF_TEMIS;
    }
    yf_vec3_copy(unif.emis_fac, prop->emissive.factor);

    /* pbr */
    switch (prop->pbr) {
    case YF_PBR_SPECGLOSS:
        if (prop->pbrsg.diffuse_tex.tex != NULL) {
            if (yf_texture_copyres2(&prop->pbrsg.diffuse_tex, inst_dtb,
                                    inst_alloc, YF_RESBIND_CLR, 0) != 0)
                return -1;
            unif.tex_mask |= YF_TCLR;
        }

        if (prop->pbrsg.spec_gloss_tex.tex != NULL) {
            if (yf_texture_copyres2(&prop->pbrsg.spec_gloss_tex, inst_dtb,
                                    inst_alloc, YF_RESBIND_PBR, 0) != 0)
                return -1;
            unif.tex_mask |= YF_TPBR;
        }

        yf_vec4_copy(unif.clr_fac, prop->pbrsg.diffuse_fac);
        yf_vec3_copy(unif.pbr_fac, prop->pbrsg.specular_fac);
        unif.pbr_fac[3] = prop->pbrsg.glossiness_fac;

        unif.method = YF_MPBRSG;
        break;

    case YF_PBR_METALROUGH:
        if (prop->pbrmr.color_tex.tex != NULL) {
            if (yf_texture_copyres2(&prop->pbrmr.color_tex, inst_dtb,
                                    inst_alloc, YF_RESBIND_CLR, 0) != 0)
                return -1;
            unif.tex_mask |= YF_TCLR;
        }

        if (prop->pbrmr.metal_rough_tex.tex != NULL) {
            if (yf_texture_copyres2(&prop->pbrmr.metal_rough_tex, inst_dtb,
                                    inst_alloc, YF_RESBIND_PBR, 0) != 0)
                return -1;
            unif.tex_mask |= YF_TPBR;
        }

        yf_vec4_copy(unif.clr_fac, prop->pbrmr.color_fac);
        unif.pbr_fac[0] = prop->pbrmr.metallic_fac;
        unif.pbr_fac[1] = prop->pbrmr.roughness_fac;
        unif.pbr_fac[2] = unif.pbr_fac[3] = 1.0f;

        unif.method = YF_MPBRMR;
        break;

    case YF_PBR_NONE:
        if (prop->nopbr.color_tex.tex != NULL) {
            if (yf_texture_copyres2(&prop->nopbr.color_tex, inst_dtb,
                                    inst_alloc, YF_RESBIND_CLR, 0) != 0)
                return -1;
            unif.tex_mask |= YF_TCLR;
        }

        yf_vec4_copy(unif.clr_fac, prop->nopbr.color_fac);

        unif.method = YF_MUNLIT;
        break;

    default:
        assert(0);
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    /* blend */
    switch (prop->alphamode) {
    case YF_ALPHAMODE_OPAQUE:
        unif.blend = YF_BOPAQUE;
        break;
    case YF_ALPHAMODE_BLEND:
        unif.blend = YF_BBLEND;
        break;
    default:
        assert(0);
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    const size_t sz = YF_MATLSZ;
    const yf_slice_t elems = {0, 1};

    /* copy */
    if (yf_buffer_copy(vars_.buf, vars_.buf_off, &unif, sz) != 0 ||
        yf_dtable_copybuf(inst_dtb, inst_alloc, YF_RESBIND_MATL, elems,
                          &vars_.buf, &vars_.buf_off, &sz) != 0)
        return -1;
    vars_.buf_off += sz + vars_.matlpd;

    return 0;

#undef YF_MPBRSG
#undef YF_MPBRMR
#undef YF_MUNLIT

#undef YF_BOPAQUE
#undef YF_BBLEND

#undef YF_TCLR
#undef YF_TPBR
#undef YF_TNORM
#undef YF_TOCC
#undef YF_TEMIS
}

/* Renders model objects. */
static int render_mdl(yf_scene_t *scn)
{
    assert(YF_RESRQ_MDL == 0 && YF_RESRQ_MDL2 == 1 && YF_RESRQ_MDL4 == 2 &&
           YF_RESRQ_MDL8 == 3 && YF_RESRQ_MDL16 == 4 && YF_RESRQ_MDL32 == 5 &&
           YF_RESRQ_MDL64 == 6);

    int resrq[YF_RESRQ_MDL64 + 1];
    unsigned insts[YF_RESRQ_N];
    unsigned rq_n = 0;
    for (int i = 0; i < YF_RESRQ_MDL64 + 1; i++) {
        if (vars_.insts[i] != 0) {
            resrq[rq_n] = i;
            insts[rq_n] = 1 << i;
            rq_n++;
        }
    }

    yf_list_t *done = yf_list_init(NULL);
    if (done == NULL)
        return -1;

    yf_iter_t it = YF_NILIT;
    kv_mdl_t *val;

    while (1) {
        val = yf_dict_next(vars_.mdls, &it, NULL);
        if (YF_IT_ISNIL(it))
            break;

        yf_model_t **mdls;
        if (val->mdl_cap == 1)
            mdls = &val->mdl;
        else
            mdls = val->mdls;

        unsigned rem = val->mdl_n;
        unsigned n = 0;

        while (1) {
            unsigned rq_i = 0;
            for (; rq_i + 1 < rq_n; rq_i++) {
                if (insts[rq_i] >= rem)
                    break;
            }

            yf_gstate_t *gst = NULL;
            unsigned inst_alloc;
            for (int i = rq_i; i >= 0; i--) {
                gst = obtain_res(resrq[i], &inst_alloc);
                if (gst != NULL) {
                    n = YF_MIN(insts[i], rem);
                    rem -= n;
                    rq_i = i;
                    break;
                }
            }

            if (gst == NULL) {
                if (yf_geterr() == YF_ERR_INUSE) {
                    /* out of resources, the remaining instances for this entry
                       will be rendered in future passes */
                    val->mdl_n = rem;
                    break;
                } else {
                    yf_list_deinit(done);
                    return -1;
                }
            }

            yf_dtable_t *inst_dtb = yf_gstate_getdtb(gst, YF_RESIDX_INST);
            if (copy_inst_mdl(scn, mdls+rem, n, inst_dtb, inst_alloc) != 0) {
                yf_list_deinit(done);
                return -1;
            }

            yf_cmdbuf_setgstate(vars_.cb, gst);
            yf_cmdbuf_setdtable(vars_.cb, YF_RESIDX_INST, inst_alloc);

            yf_mesh_t *mesh = yf_model_getmesh(mdls[rem]);
            if (mesh != NULL) {
                /* TODO: Multiple materials. */
                yf_material_t *matl = yf_mesh_getmatl(mesh, 0);
                if (matl != NULL) {
                    if (copy_matl(matl, inst_dtb, inst_alloc) != 0) {
                        yf_list_deinit(done);
                        return -1;
                    }
                } else {
                    /* TODO */
                    assert(0);
                    abort();
                }
                yf_mesh_draw(mesh, vars_.cb, n);
            } else {
                /* TODO */
                assert(0);
                abort();
            }

            if (rem == 0) {
                /* cannot invalidate the dictionary iterator */
                if (yf_list_insert(done, val) == 0)
                    break;
                yf_list_deinit(done);
                return -1;
            }
        }
    }

    while ((val = yf_list_removeat(done, NULL)) != NULL) {
        yf_dict_remove(vars_.mdls, val);
        if (val->mdl_cap > 1)
            free(val->mdls);
        free(val);
    }

    yf_list_deinit(done);

    return 0;
}

/* Renders terrain objects. */
static int render_terr(yf_scene_t *scn)
{
    yf_iter_t it = YF_NILIT;

    while (1) {
        yf_terrain_t *terr = yf_list_next(vars_.terrs, &it);
        if (YF_IT_ISNIL(it))
            break;

        unsigned inst_alloc;
        yf_gstate_t *gst = obtain_res(YF_RESRQ_TERR, &inst_alloc);
        if (gst == NULL) {
            switch (yf_geterr()) {
            case YF_ERR_INUSE:
                /* out of resources, need to execute pending work */
                return 0;
            default:
                return -1;
            }
        }

        yf_dtable_t *inst_dtb = yf_gstate_getdtb(gst, YF_RESIDX_INST);
        if (copy_inst_terr(scn, &terr, 1, inst_dtb, inst_alloc) != 0)
            return -1;

        yf_texture_t *hmap = yf_terrain_gethmap(terr);
        if (hmap != NULL) {
            yf_texture_copyres(hmap, inst_dtb, inst_alloc, YF_RESBIND_HMAP, 0);
        } else {
            /* TODO */
            assert(0);
            abort();
        }

        yf_texture_t *tex = yf_terrain_gettex(terr);
        if (tex != NULL) {
            yf_texture_copyres(tex, inst_dtb, inst_alloc, YF_RESBIND_TEX, 0);
        } else {
            /* TODO */
            assert(0);
            abort();
        }

        yf_cmdbuf_setgstate(vars_.cb, gst);
        yf_cmdbuf_setdtable(vars_.cb, YF_RESIDX_INST, inst_alloc);

        yf_mesh_t *mesh = yf_terrain_getmesh(terr);
        yf_mesh_draw(mesh, vars_.cb, 1);

        yf_list_removeat(vars_.terrs, &it);
        it = YF_NILIT;
    }

    return 0;
}

/* Renders particle system objects. */
static int render_part(yf_scene_t *scn)
{
    yf_iter_t it = YF_NILIT;

    while (1) {
        yf_particle_t *part = yf_list_next(vars_.parts, &it);
        if (YF_IT_ISNIL(it))
            break;

        unsigned inst_alloc;
        yf_gstate_t *gst = obtain_res(YF_RESRQ_PART, &inst_alloc);
        if (gst == NULL) {
            switch (yf_geterr()) {
            case YF_ERR_INUSE:
                /* out of resources, need to execute pending work */
                return 0;
            default:
                return -1;
            }
        }

        yf_dtable_t *inst_dtb = yf_gstate_getdtb(gst, YF_RESIDX_INST);
        if (copy_inst_part(scn, &part, 1, inst_dtb, inst_alloc) != 0)
            return -1;

        yf_texture_t *tex = yf_particle_gettex(part);
        if (tex != NULL) {
            yf_texture_copyres(tex, inst_dtb, inst_alloc, YF_RESBIND_TEX, 0);
        } else {
            /* TODO */
            assert(0);
            abort();
        }

        yf_cmdbuf_setgstate(vars_.cb, gst);
        yf_cmdbuf_setdtable(vars_.cb, YF_RESIDX_INST, inst_alloc);

        yf_mesh_t *mesh = yf_particle_getmesh(part);
        yf_mesh_draw(mesh, vars_.cb, 1);

        yf_list_removeat(vars_.parts, &it);
        it = YF_NILIT;
    }

    return 0;
}

/* Renders quad objects. */
static int render_quad(yf_scene_t *scn)
{
    yf_iter_t it = YF_NILIT;

    while (1) {
        yf_quad_t *quad = yf_list_next(vars_.quads, &it);
        if (YF_IT_ISNIL(it))
            break;

        unsigned inst_alloc;
        yf_gstate_t *gst = obtain_res(YF_RESRQ_QUAD, &inst_alloc);
        if (gst == NULL) {
            switch (yf_geterr()) {
            case YF_ERR_INUSE:
                /* out of resources, need to execute pending work */
                return 0;
            default:
                return -1;
            }
        }

        yf_dtable_t *inst_dtb = yf_gstate_getdtb(gst, YF_RESIDX_INST);
        if (copy_inst_quad(scn, &quad, 1, inst_dtb, inst_alloc) != 0)
            return -1;

        yf_texture_t *tex = yf_quad_gettex(quad);
        if (tex != NULL) {
            yf_texture_copyres(tex, inst_dtb, inst_alloc, YF_RESBIND_TEX, 0);
        } else {
            /* TODO */
            assert(0);
            abort();
        }

        yf_cmdbuf_setgstate(vars_.cb, gst);
        yf_cmdbuf_setdtable(vars_.cb, YF_RESIDX_INST, inst_alloc);

        yf_mesh_t *mesh = yf_quad_getmesh(quad);
        yf_mesh_draw(mesh, vars_.cb, 1);

        yf_list_removeat(vars_.quads, &it);
        it = YF_NILIT;
    }

    return 0;
}

/* Renders label objects. */
static int render_labl(yf_scene_t *scn)
{
    yf_iter_t it = YF_NILIT;

    while (1) {
        yf_label_t *labl = yf_list_next(vars_.labls, &it);
        if (YF_IT_ISNIL(it))
            break;

        unsigned inst_alloc;
        yf_gstate_t *gst = obtain_res(YF_RESRQ_LABL, &inst_alloc);
        if (gst == NULL) {
            switch (yf_geterr()) {
            case YF_ERR_INUSE:
                /* out of resources, need to execute pending work */
                return 0;
            default:
                return -1;
            }
        }

        yf_dtable_t *inst_dtb = yf_gstate_getdtb(gst, YF_RESIDX_INST);
        if (copy_inst_labl(scn, &labl, 1, inst_dtb, inst_alloc) != 0)
            return -1;

        /* FIXME: Texture may be invalid. */
        yf_texture_t *tex = yf_label_gettex(labl);
        yf_texture_copyres(tex, inst_dtb, inst_alloc, YF_RESBIND_TEX, 0);

        yf_cmdbuf_setgstate(vars_.cb, gst);
        yf_cmdbuf_setdtable(vars_.cb, YF_RESIDX_INST, inst_alloc);

        yf_mesh_t *mesh = yf_label_getmesh(labl);
        yf_mesh_draw(mesh, vars_.cb, 1);

        yf_list_removeat(vars_.labls, &it);
        it = YF_NILIT;
    }

    return 0;
}

/* Hashes a 'kv_mdl_t'. */
static size_t hash_mdl(const void *x)
{
    const kv_mdl_t *kv = x;
    return yf_hashv(&kv->key, sizeof kv->key, NULL);

    static_assert(sizeof kv->key == sizeof(void *), "!sizeof");
}

/* Compares a 'kv_mdl_t' to another. */
static int cmp_mdl(const void *a, const void *b)
{
    const kv_mdl_t *kv1 = a;
    const kv_mdl_t *kv2 = b;

    return kv1->key.mesh != kv2->key.mesh;
}

/* Deallocates a 'kv_mdl_t'. */
static int dealloc_mdl(YF_UNUSED void *key, void *val, YF_UNUSED void *arg)
{
    kv_mdl_t *kv = val;

    if (kv->mdl_cap > 1)
        free(kv->mdls);

    free(val);
    return 0;
}

/* Yields all previously obtained resources. */
static void yield_res(void)
{
    reso_t *val;
    while ((val = yf_list_removeat(vars_.res_obtd, NULL)) != NULL) {
        yf_resmgr_yield(val->resrq, val->inst_alloc);
        free(val);
    }
}

/* Clears data structures of all objects. */
static void clear_obj(void)
{
    if (yf_dict_getlen(vars_.mdls) != 0) {
        yf_dict_each(vars_.mdls, dealloc_mdl, NULL);
        yf_dict_clear(vars_.mdls);
    }
    yf_list_clear(vars_.terrs);
    yf_list_clear(vars_.parts);
    yf_list_clear(vars_.quads);
    yf_list_clear(vars_.labls);
    vars_.light_n = 0;
}

/* Deinitializes shared variables and releases resources. */
static void deinit_vars(void)
{
    if (vars_.ctx == NULL)
        return;

    yf_resmgr_clear();
    yf_buffer_deinit(vars_.buf);
    yf_list_deinit(vars_.res_obtd);

    if (vars_.mdls != NULL) {
        yf_dict_each(vars_.mdls, dealloc_mdl, NULL);
        yf_dict_deinit(vars_.mdls);
    }
    yf_list_deinit(vars_.terrs);
    yf_list_deinit(vars_.parts);
    yf_list_deinit(vars_.quads);
    yf_list_deinit(vars_.labls);

    memset(&vars_, 0, sizeof vars_);
}

/* Initializes shared variables and prepares static resources. */
static int init_vars(void)
{
    assert(vars_.ctx == NULL);

    if ((vars_.ctx = yf_getctx()) == NULL)
        return -1;

    if ((vars_.res_obtd = yf_list_init(NULL)) == NULL ||
        (vars_.mdls = yf_dict_init(hash_mdl, cmp_mdl)) == NULL ||
        (vars_.terrs = yf_list_init(NULL)) == NULL ||
        (vars_.parts = yf_list_init(NULL)) == NULL ||
        (vars_.quads = yf_list_init(NULL)) == NULL ||
        (vars_.labls = yf_list_init(NULL)) == NULL) {

        deinit_vars();
        return -1;
    }

    const unsigned align = yf_getlimits(vars_.ctx)->dtable.cpy_unif_align_min;
    unsigned mod;

    if ((mod = YF_GLOBLSZ % align) != 0)
        vars_.globlpd = align - mod;
    if ((mod = YF_LIGHTSZ % align) != 0)
        vars_.lightpd = align - mod;
    if ((mod = YF_INSTSZ_MDL % align) != 0)
        vars_.instpd_mdl = align - mod;
    if ((mod = YF_INSTSZ_TERR % align) != 0)
        vars_.instpd_terr = align - mod;
    if ((mod = YF_INSTSZ_PART % align) != 0)
        vars_.instpd_part = align - mod;
    if ((mod = YF_INSTSZ_QUAD % align) != 0)
        vars_.instpd_quad = align - mod;
    if ((mod = YF_INSTSZ_LABL % align) != 0)
        vars_.instpd_labl = align - mod;
    if ((mod = YF_MATLSZ % align) != 0)
        vars_.matlpd = align - mod;

#ifndef YF_SCN_DYNAMIC
    if (prepare_res() != 0) {
        deinit_vars();
        return -1;
    }
#endif

    return 0;
}

yf_scene_t *yf_scene_init(void)
{
    yf_scene_t *scn = calloc(1, sizeof(yf_scene_t));
    if (scn == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }
    if ((scn->node = yf_node_init()) == NULL) {
        yf_scene_deinit(scn);
        return NULL;
    }
    if ((scn->cam = yf_camera_init(YF_CAMORIG, YF_CAMTGT, YF_CAMASP)) == NULL) {
        yf_scene_deinit(scn);
        return NULL;
    }
    scn->color = YF_COLOR_DARKGREY;

    return scn;
}

yf_node_t *yf_scene_getnode(yf_scene_t *scn)
{
    assert(scn != NULL);
    return scn->node;
}

yf_camera_t *yf_scene_getcam(yf_scene_t *scn)
{
    assert(scn != NULL);
    return scn->cam;
}

yf_color_t yf_scene_getcolor(yf_scene_t *scn)
{
    assert(scn != NULL);
    return scn->color;
}

void yf_scene_setcolor(yf_scene_t *scn, yf_color_t color)
{
    assert(scn != NULL);
    scn->color = color;
}

void yf_scene_deinit(yf_scene_t *scn)
{
    if (scn == NULL)
        return;

    yf_camera_deinit(scn->cam);
    yf_node_deinit(scn->node);
    free(scn);
}

int yf_scene_render(yf_scene_t *scn, yf_pass_t *pass, yf_target_t *tgt,
                    yf_dim2_t dim)
{
    assert(scn != NULL);
    assert(pass != NULL);
    assert(tgt != NULL);

    /* XXX: This is done here because 'resmgr' needs a valid 'pass'. */
    if (vars_.ctx == NULL && init_vars() != 0)
        return -1;

    yf_camera_adjust(scn->cam, (float)dim.width / (float)dim.height);
    YF_VIEWPORT_FROMDIM2(dim, scn->vport);
    YF_VIEWPORT_SCISSOR(scn->vport, scn->sciss);

    int r = 0;
    yf_node_traverse(scn->node, traverse_scn, &r);
    if (r != 0) {
        clear_obj();
        return -1;
    }

#ifdef YF_SCN_DYNAMIC
    if (prepare_res() != 0) {
        clear_obj();
        return -1;
    }
#endif

    unsigned pend = YF_PEND_NONE;
    if (yf_dict_getlen(vars_.mdls) != 0)
        pend |= YF_PEND_MDL;
    if (yf_list_getlen(vars_.terrs) != 0)
        pend |= YF_PEND_TERR;
    if (yf_list_getlen(vars_.parts) != 0)
        pend |= YF_PEND_PART;
    if (yf_list_getlen(vars_.quads) != 0)
        pend |= YF_PEND_QUAD;
    if (yf_list_getlen(vars_.labls) != 0)
        pend |= YF_PEND_LABL;

    if ((vars_.cb = yf_cmdbuf_get(vars_.ctx, YF_CMDBUF_GRAPH)) == NULL) {
        clear_obj();
        return -1;
    }

    yf_cmdbuf_clearcolor(vars_.cb, 0, scn->color);
    yf_cmdbuf_cleardepth(vars_.cb, 1.0f);

    vars_.buf_off = 0;
    if (copy_globl(scn) != 0 || copy_light() != 0) {
        clear_obj();
        return -1;
    }
    const size_t globl_off = vars_.buf_off;
    yf_cmdbuf_setdtable(vars_.cb, YF_RESIDX_GLOBL, 0);

#if defined(YF_DEVEL) && defined(YF_PRINT)
    printf("\n[YF] OUTPUT (%s):\n"
           " scene resources:\n"
           "  RESRQ_MDL:   %u\n"
           "  RESRQ_MDL2:  %u\n"
           "  RESRQ_MDL4:  %u\n"
           "  RESRQ_MDL8:  %u\n"
           "  RESRQ_MDL16: %u\n"
           "  RESRQ_MDL32: %u\n"
           "  RESRQ_MDL64: %u\n"
           "  RESRQ_TERR:  %u\n"
           "  RESRQ_PART:  %u\n"
           "  RESRQ_QUAD:  %u\n"
           "  RESRQ_LABL:  %u\n\n",
           __func__,
           vars_.insts[YF_RESRQ_MDL], vars_.insts[YF_RESRQ_MDL2],
           vars_.insts[YF_RESRQ_MDL4], vars_.insts[YF_RESRQ_MDL8],
           vars_.insts[YF_RESRQ_MDL16], vars_.insts[YF_RESRQ_MDL32],
           vars_.insts[YF_RESRQ_MDL64], vars_.insts[YF_RESRQ_TERR],
           vars_.insts[YF_RESRQ_PART], vars_.insts[YF_RESRQ_QUAD],
           vars_.insts[YF_RESRQ_LABL]);

    unsigned exec_n = 0;
#endif

    while (1) {
        yf_cmdbuf_settarget(vars_.cb, tgt);
        yf_cmdbuf_setvport(vars_.cb, 0, &scn->vport);
        yf_cmdbuf_setsciss(vars_.cb, 0, scn->sciss);

        if (pend & YF_PEND_MDL) {
            if (render_mdl(scn) != 0) {
                yf_cmdbuf_end(vars_.cb);
                yf_cmdbuf_reset(vars_.ctx);
                yield_res();
                clear_obj();
                return -1;
            }
            if (yf_dict_getlen(vars_.mdls) == 0)
                pend &= ~YF_PEND_MDL;
        }

        if (pend & YF_PEND_TERR) {
            if (render_terr(scn) != 0) {
                yf_cmdbuf_end(vars_.cb);
                yf_cmdbuf_reset(vars_.ctx);
                yield_res();
                clear_obj();
                return -1;
            }
            if (yf_list_getlen(vars_.terrs) == 0)
                pend &= ~YF_PEND_TERR;
        }

        if (pend & YF_PEND_PART) {
            if (render_part(scn) != 0) {
                yf_cmdbuf_end(vars_.cb);
                yf_cmdbuf_reset(vars_.ctx);
                yield_res();
                clear_obj();
                return -1;
            }
            if (yf_list_getlen(vars_.parts) == 0)
                pend &= ~YF_PEND_PART;
        }

        if (pend & YF_PEND_QUAD) {
            if (render_quad(scn) != 0) {
                yf_cmdbuf_end(vars_.cb);
                yf_cmdbuf_reset(vars_.ctx);
                yield_res();
                clear_obj();
                return -1;
            }
            if (yf_list_getlen(vars_.quads) == 0)
                pend &= ~YF_PEND_QUAD;
        }

        if (pend & YF_PEND_LABL) {
            if (render_labl(scn) != 0) {
                yf_cmdbuf_end(vars_.cb);
                yf_cmdbuf_reset(vars_.ctx);
                yield_res();
                clear_obj();
                return -1;
            }
            if (yf_list_getlen(vars_.labls) == 0)
                pend &= ~YF_PEND_LABL;
        }

        if (yf_cmdbuf_end(vars_.cb) == 0) {
            if (yf_cmdbuf_exec(vars_.ctx) != 0) {
                yield_res();
                clear_obj();
                return -1;
            }
        } else {
            yf_cmdbuf_reset(vars_.ctx);
            yield_res();
            clear_obj();
            return -1;
        }

#if defined(YF_DEVEL) && defined(YF_PRINT)
        exec_n++;
#endif

        yield_res();

        if (pend != YF_PEND_NONE) {
            vars_.cb = yf_cmdbuf_get(vars_.ctx, YF_CMDBUF_GRAPH);
            if (vars_.cb == NULL) {
                clear_obj();
                return -1;
            }
            vars_.buf_off = globl_off;
            yf_cmdbuf_setdtable(vars_.cb, YF_RESIDX_GLOBL, 0);
        } else {
            break;
        }
    }

#if defined(YF_DEVEL) && defined(YF_PRINT)
    printf("\n[YF] OUTPUT (%s):\n 'cmdbuf_exec()' called %u time%s\n\n",
           __func__, exec_n, (exec_n > 1 ? "s" : ""));
#endif

    clear_obj();
    return 0;
}

/* Called by 'coreobj' on exit. */
void yf_unsetscn(void)
{
    deinit_vars();
}
