/*
 * YF
 * resmgr.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "yf/com/yf-error.h"

#include "resmgr.h"
#include "coreobj.h"
#include "node.h"
#include "mesh.h"

#ifndef YF_SHD_DIR
# define YF_SHD_DIR "bin/"
#endif
#ifndef YF_SHD_PREFIX
# define YF_SHD_PREFIX "shd."
#endif
#ifndef YF_SHD_SUFFIX
# define YF_SHD_SUFFIX ".bin"
#endif

/* Resource entry. */
typedef struct {
    yf_gstate_t *gst;
    char *obtained;
    unsigned n;
    unsigned i;
} entry_t;

/* List of resources, indexed by 'resrq' values. */
static entry_t entries_[YF_RESRQ_N] = {0};

/* Global descriptor table. */
static yf_dtable_t *globl_ = NULL;

/* Sizes used for instance allocations, indexed by 'resrq' values. */
static unsigned allocn_[YF_RESRQ_N] = {0};

/* Vertex inputs. */
/* TODO: This should be shared with mesh. */
static const yf_vinput_t vins_[] = {
    [0] = {
        .attrs = &(yf_vattr_t){YF_RESLOC_POS, YF_VFMT_FLOAT3, 0},
        .attr_n = 1,
        .stride = sizeof(float[3]),
        .vrate = YF_VRATE_VERT
    },
    [1] = {
        .attrs = &(yf_vattr_t){YF_RESLOC_NORM, YF_VFMT_FLOAT3, 0},
        .attr_n = 1,
        .stride = sizeof(float[3]),
        .vrate = YF_VRATE_VERT
    },
    [2] = {
        .attrs = &(yf_vattr_t){YF_RESLOC_TGNT, YF_VFMT_FLOAT4, 0},
        .attr_n = 1,
        .stride = sizeof(float[4]),
        .vrate = YF_VRATE_VERT
    },
    [3] = {
        .attrs = &(yf_vattr_t){YF_RESLOC_TC, YF_VFMT_FLOAT2, 0},
        .attr_n = 1,
        .stride = sizeof(float[2]),
        .vrate = YF_VRATE_VERT
    },
    [4] = {
        .attrs = &(yf_vattr_t){YF_RESLOC_TC1, YF_VFMT_FLOAT2, 0},
        .attr_n = 1,
        .stride = sizeof(float[2]),
        .vrate = YF_VRATE_VERT
    },
    [5] = {
        .attrs = &(yf_vattr_t){YF_RESLOC_CLR, YF_VFMT_FLOAT4, 0},
        .attr_n = 1,
        .stride = sizeof(float[4]),
        .vrate = YF_VRATE_VERT
    },
    [6] = {
        .attrs = &(yf_vattr_t){YF_RESLOC_JNTS, YF_VFMT_UBYTE4, 0},
        .attr_n = 1,
        .stride = 4,
        .vrate = YF_VRATE_VERT
    },
    [7] = {
        .attrs = &(yf_vattr_t){YF_RESLOC_WGTS, YF_VFMT_FLOAT4, 0},
        .attr_n = 1,
        .stride = sizeof(float[4]),
        .vrate = YF_VRATE_VERT
    }
};

/* Makes a string to use as the pathname of a shader module.
   The caller is responsible for deallocating the returned string. */
static char *make_shdpath(int nodeobj, int stage, unsigned elements)
{
    char *obj_name = NULL;
    switch (nodeobj) {
    case YF_NODEOBJ_MODEL:
        obj_name = "model";
        break;
    case YF_NODEOBJ_TERRAIN:
        obj_name = "terrain";
        break;
    case YF_NODEOBJ_PARTICLE:
        obj_name = "particle";
        break;
    case YF_NODEOBJ_QUAD:
        obj_name = "quad";
        break;
    case YF_NODEOBJ_LABEL:
        obj_name = "label";
        break;
    default:
        yf_seterr(YF_ERR_INVARG, __func__);
        return NULL;
    }

    char *stg_name = NULL;
    switch (stage) {
    case YF_STAGE_VERT:
        stg_name = "vert";
        break;
    case YF_STAGE_TESC:
        stg_name = "tesc";
        break;
    case YF_STAGE_TESE:
        stg_name = "tese";
        break;
    case YF_STAGE_GEOM:
        stg_name = "geom";
        break;
    case YF_STAGE_FRAG:
        stg_name = "frag";
        break;
    case YF_STAGE_COMP:
        stg_name = "comp";
        break;
    default:
        yf_seterr(YF_ERR_INVARG, __func__);
        return NULL;
    }

    char elem_str[8] = {'\0'};
    if (elements > 1) {
        int sz = sizeof elem_str;
        int r = snprintf(elem_str, sz, "%u", elements);
        if (r >= sz || r < 0) {
            yf_seterr(YF_ERR_OTHER, __func__);
            return NULL;
        }
    }

    const char *fmt = YF_SHD_DIR YF_SHD_PREFIX "%s%s.%s" YF_SHD_SUFFIX;
    char *str = NULL;
    int len = snprintf(str, 0, fmt, obj_name, elem_str, stg_name);
    if (len < 0) {
        yf_seterr(YF_ERR_OTHER, __func__);
        return NULL;
    }
    if ((str = malloc(++len)) == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }
    if (snprintf(str, len, fmt, obj_name, elem_str, stg_name) < 0) {
        yf_seterr(YF_ERR_OTHER, __func__);
        free(str);
        return NULL;
    }
    return str;
}

/* Initializes the entry of a model resource. */
static int init_mdl(entry_t *entry, unsigned elements)
{
    yf_context_t *ctx = yf_getctx();
    yf_pass_t *pass = yf_getpass();

    assert(ctx != NULL && pass != NULL);

    /* shader stage */
    char *vert_path = make_shdpath(YF_NODEOBJ_MODEL, YF_STAGE_VERT, elements);
    char *frag_path = make_shdpath(YF_NODEOBJ_MODEL, YF_STAGE_FRAG, 1);
    if (vert_path == NULL || frag_path == NULL) {
        free(vert_path);
        free(frag_path);
        return -1;
    }

    yf_shdid_t vert_shd;
    if (yf_loadshd(ctx, vert_path, &vert_shd) != 0) {
        free(vert_path);
        free(frag_path);
        return -1;
    }
    free(vert_path);

    yf_shdid_t frag_shd;
    if (yf_loadshd(ctx, frag_path, &frag_shd) != 0) {
        yf_unldshd(ctx, vert_shd);
        free(frag_path);
        return -1;
    }
    free(frag_path);

    const yf_stage_t stgs[] = {
        {YF_STAGE_VERT, vert_shd, "main"},
        {YF_STAGE_FRAG, frag_shd, "main"}
    };
    const unsigned stg_n = sizeof stgs / sizeof *stgs;

    /* descriptor table */
    const yf_dentry_t inst_ents[] = {
        {YF_RESBIND_INST, YF_DTYPE_UNIFORM, 1, NULL},
        {YF_RESBIND_MATL, YF_DTYPE_UNIFORM, 1, NULL},
        {YF_RESBIND_CLR, YF_DTYPE_ISAMPLER, 1, NULL},
        {YF_RESBIND_PBR, YF_DTYPE_ISAMPLER, 1, NULL},
        {YF_RESBIND_NORM, YF_DTYPE_ISAMPLER, 1, NULL},
        {YF_RESBIND_OCC, YF_DTYPE_ISAMPLER, 1, NULL},
        {YF_RESBIND_EMIS, YF_DTYPE_ISAMPLER, 1, NULL}
    };

    yf_dtable_t *inst_dtb =
        yf_dtable_init(ctx, inst_ents, sizeof inst_ents / sizeof *inst_ents);

    if (inst_dtb == NULL || yf_dtable_alloc(inst_dtb, entry->n) != 0) {
        yf_unldshd(ctx, vert_shd);
        yf_unldshd(ctx, frag_shd);
        yf_dtable_deinit(inst_dtb);
        return -1;
    }

    yf_dtable_t *const dtbs[] = {globl_, inst_dtb};
    const unsigned dtb_n = sizeof dtbs / sizeof *dtbs;

    /* vertex input */
    const yf_vinput_t *vins = vins_;
    const unsigned vin_n = sizeof vins_ / sizeof *vins_;

    /* graphics state */
    const yf_gconf_t conf = {
        pass,
        stgs,
        stg_n,
        dtbs,
        dtb_n,
        vins,
        vin_n,
        YF_TOPOLOGY_TRIANGLE,
        YF_POLYMODE_FILL,
        YF_CULLMODE_BACK,
        YF_WINDING_CCW
    };

    entry->gst = yf_gstate_init(ctx, &conf);
    if (entry->gst == NULL) {
        yf_unldshd(ctx, vert_shd);
        yf_unldshd(ctx, frag_shd);
        yf_dtable_deinit(inst_dtb);
        return -1;
    }

    return 0;
}

/* Initializes the entry of a terrain resource. */
static int init_terr(entry_t *entry)
{
    yf_context_t *ctx = yf_getctx();
    yf_pass_t *pass = yf_getpass();

    assert(ctx != NULL && pass != NULL);

    /* shader stage */
    char *vert_path = make_shdpath(YF_NODEOBJ_TERRAIN, YF_STAGE_VERT, 1);
    char *frag_path = make_shdpath(YF_NODEOBJ_TERRAIN, YF_STAGE_FRAG, 1);
    if (vert_path == NULL || frag_path == NULL) {
        free(vert_path);
        free(frag_path);
        return -1;
    }

    yf_shdid_t vert_shd;
    if (yf_loadshd(ctx, vert_path, &vert_shd) != 0) {
        free(vert_path);
        free(frag_path);
        return -1;
    }
    free(vert_path);

    yf_shdid_t frag_shd;
    if (yf_loadshd(ctx, frag_path, &frag_shd) != 0) {
        yf_unldshd(ctx, vert_shd);
        free(frag_path);
        return -1;
    }
    free(frag_path);

    const yf_stage_t stgs[] = {
        {YF_STAGE_VERT, vert_shd, "main"},
        {YF_STAGE_FRAG, frag_shd, "main"}
    };
    const unsigned stg_n = sizeof stgs / sizeof *stgs;

    /* descriptor table */
    const yf_dentry_t inst_ents[] = {
        {YF_RESBIND_INST, YF_DTYPE_UNIFORM, 1, NULL},
        {YF_RESBIND_TEX, YF_DTYPE_ISAMPLER, 1, NULL},
        {YF_RESBIND_HMAP, YF_DTYPE_ISAMPLER, 1, NULL}
    };

    yf_dtable_t *inst_dtb =
        yf_dtable_init(ctx, inst_ents, sizeof inst_ents / sizeof *inst_ents);

    if (inst_dtb == NULL || yf_dtable_alloc(inst_dtb, entry->n) != 0) {
        yf_unldshd(ctx, vert_shd);
        yf_unldshd(ctx, frag_shd);
        yf_dtable_deinit(inst_dtb);
        return -1;
    }

    yf_dtable_t *const dtbs[] = {globl_, inst_dtb};
    const unsigned dtb_n = sizeof dtbs / sizeof *dtbs;

    /* vertex input */
    const yf_vinput_t *vins = vins_;
    const unsigned vin_n = sizeof vins_ / sizeof *vins_;

    /* graphics state */
    const yf_gconf_t conf = {
        pass,
        stgs,
        stg_n,
        dtbs,
        dtb_n,
        vins,
        vin_n,
        YF_TOPOLOGY_TRIANGLE,
        YF_POLYMODE_FILL,
        YF_CULLMODE_BACK,
        YF_WINDING_CCW
    };

    entry->gst = yf_gstate_init(ctx, &conf);
    if (entry->gst == NULL) {
        yf_unldshd(ctx, vert_shd);
        yf_unldshd(ctx, frag_shd);
        yf_dtable_deinit(inst_dtb);
        return -1;
    }

    return 0;
}

/* Initializes the entry of a particle system resource. */
static int init_part(entry_t *entry)
{
    yf_context_t *ctx = yf_getctx();
    yf_pass_t *pass = yf_getpass();

    assert(ctx != NULL && pass != NULL);

    /* shader stage */
    char *vert_path = make_shdpath(YF_NODEOBJ_PARTICLE, YF_STAGE_VERT, 1);
    char *frag_path = make_shdpath(YF_NODEOBJ_PARTICLE, YF_STAGE_FRAG, 1);
    if (vert_path == NULL || frag_path == NULL) {
        free(vert_path);
        free(frag_path);
        return -1;
    }

    yf_shdid_t vert_shd;
    if (yf_loadshd(ctx, vert_path, &vert_shd) != 0) {
        free(vert_path);
        free(frag_path);
        return -1;
    }
    free(vert_path);

    yf_shdid_t frag_shd;
    if (yf_loadshd(ctx, frag_path, &frag_shd) != 0) {
        yf_unldshd(ctx, vert_shd);
        free(frag_path);
        return -1;
    }
    free(frag_path);

    const yf_stage_t stgs[] = {
        {YF_STAGE_VERT, vert_shd, "main"},
        {YF_STAGE_FRAG, frag_shd, "main"}
    };
    const unsigned stg_n = sizeof stgs / sizeof *stgs;

    /* descriptor table */
    const yf_dentry_t inst_ents[] = {
        {YF_RESBIND_INST, YF_DTYPE_UNIFORM, 1, NULL},
        {YF_RESBIND_TEX, YF_DTYPE_ISAMPLER, 1, NULL}
    };

    yf_dtable_t *inst_dtb =
        yf_dtable_init(ctx, inst_ents, sizeof inst_ents / sizeof *inst_ents);

    if (inst_dtb == NULL || yf_dtable_alloc(inst_dtb, entry->n) != 0) {
        yf_unldshd(ctx, vert_shd);
        yf_unldshd(ctx, frag_shd);
        yf_dtable_deinit(inst_dtb);
        return -1;
    }

    yf_dtable_t *const dtbs[] = {globl_, inst_dtb};
    const unsigned dtb_n = sizeof dtbs / sizeof *dtbs;

    /* vertex input */
    const yf_vinput_t *vins = vins_;
    const unsigned vin_n = sizeof vins_ / sizeof *vins_;

    /* graphics state */
    const yf_gconf_t conf = {
        pass,
        stgs,
        stg_n,
        dtbs,
        dtb_n,
        vins,
        vin_n,
        YF_TOPOLOGY_POINT,
        YF_POLYMODE_FILL,
        YF_CULLMODE_BACK,
        YF_WINDING_CCW
    };

    entry->gst = yf_gstate_init(ctx, &conf);
    if (entry->gst == NULL) {
        yf_unldshd(ctx, vert_shd);
        yf_unldshd(ctx, frag_shd);
        yf_dtable_deinit(inst_dtb);
        return -1;
    }

    return 0;
}

/* Initializes the entry of a quad resource. */
static int init_quad(entry_t *entry)
{
    yf_context_t *ctx = yf_getctx();
    yf_pass_t *pass = yf_getpass();

    assert(ctx != NULL && pass != NULL);

    /* shader stage */
    char *vert_path = make_shdpath(YF_NODEOBJ_QUAD, YF_STAGE_VERT, 1);
    char *frag_path = make_shdpath(YF_NODEOBJ_QUAD, YF_STAGE_FRAG, 1);
    if (vert_path == NULL || frag_path == NULL) {
        free(vert_path);
        free(frag_path);
        return -1;
    }

    yf_shdid_t vert_shd;
    if (yf_loadshd(ctx, vert_path, &vert_shd) != 0) {
        free(vert_path);
        free(frag_path);
        return -1;
    }
    free(vert_path);

    yf_shdid_t frag_shd;
    if (yf_loadshd(ctx, frag_path, &frag_shd) != 0) {
        yf_unldshd(ctx, vert_shd);
        free(frag_path);
        return -1;
    }
    free(frag_path);

    const yf_stage_t stgs[] = {
        {YF_STAGE_VERT, vert_shd, "main"},
        {YF_STAGE_FRAG, frag_shd, "main"}
    };
    const unsigned stg_n = sizeof stgs / sizeof *stgs;

    /* descriptor table */
    const yf_dentry_t inst_ents[] = {
        {YF_RESBIND_INST, YF_DTYPE_UNIFORM, 1, NULL},
        {YF_RESBIND_TEX, YF_DTYPE_ISAMPLER, 1, NULL}
    };

    yf_dtable_t *inst_dtb =
        yf_dtable_init(ctx, inst_ents, sizeof inst_ents / sizeof *inst_ents);

    if (inst_dtb == NULL || yf_dtable_alloc(inst_dtb, entry->n) != 0) {
        yf_unldshd(ctx, vert_shd);
        yf_unldshd(ctx, frag_shd);
        yf_dtable_deinit(inst_dtb);
        return -1;
    }

    yf_dtable_t *const dtbs[] = {globl_, inst_dtb};
    const unsigned dtb_n = sizeof dtbs / sizeof *dtbs;

    /* vertex input */
    const yf_vinput_t *vins = vins_;
    const unsigned vin_n = sizeof vins_ / sizeof *vins_;

    /* graphics state */
    const yf_gconf_t conf = {
        pass,
        stgs,
        stg_n,
        dtbs,
        dtb_n,
        vins,
        vin_n,
        YF_TOPOLOGY_TRIANGLE,
        YF_POLYMODE_FILL,
        YF_CULLMODE_BACK,
        YF_WINDING_CCW
    };

    entry->gst = yf_gstate_init(ctx, &conf);
    if (entry->gst == NULL) {
        yf_unldshd(ctx, vert_shd);
        yf_unldshd(ctx, frag_shd);
        yf_dtable_deinit(inst_dtb);
        return -1;
    }

    return 0;
}

/* Initializes the entry of a label resource. */
static int init_labl(entry_t *entry)
{
    yf_context_t *ctx = yf_getctx();
    yf_pass_t *pass = yf_getpass();

    assert(ctx != NULL && pass != NULL);

    /* shader stage */
    char *vert_path = make_shdpath(YF_NODEOBJ_LABEL, YF_STAGE_VERT, 1);
    char *frag_path = make_shdpath(YF_NODEOBJ_LABEL, YF_STAGE_FRAG, 1);
    if (vert_path == NULL || frag_path == NULL) {
        free(vert_path);
        free(frag_path);
        return -1;
    }

    yf_shdid_t vert_shd;
    if (yf_loadshd(ctx, vert_path, &vert_shd) != 0) {
        free(vert_path);
        free(frag_path);
        return -1;
    }
    free(vert_path);

    yf_shdid_t frag_shd;
    if (yf_loadshd(ctx, frag_path, &frag_shd) != 0) {
        yf_unldshd(ctx, vert_shd);
        free(frag_path);
        return -1;
    }
    free(frag_path);

    const yf_stage_t stgs[] = {
        {YF_STAGE_VERT, vert_shd, "main"},
        {YF_STAGE_FRAG, frag_shd, "main"}
    };
    const unsigned stg_n = sizeof stgs / sizeof *stgs;

    /* descriptor table */
    const yf_dentry_t inst_ents[] = {
        {YF_RESBIND_INST, YF_DTYPE_UNIFORM, 1, NULL},
        {YF_RESBIND_TEX, YF_DTYPE_ISAMPLER, 1, NULL}
    };

    yf_dtable_t *inst_dtb =
        yf_dtable_init(ctx, inst_ents, sizeof inst_ents / sizeof *inst_ents);

    if (inst_dtb == NULL || yf_dtable_alloc(inst_dtb, entry->n) != 0) {
        yf_unldshd(ctx, vert_shd);
        yf_unldshd(ctx, frag_shd);
        yf_dtable_deinit(inst_dtb);
        return -1;
    }

    yf_dtable_t *const dtbs[] = {globl_, inst_dtb};
    const unsigned dtb_n = sizeof dtbs / sizeof *dtbs;

    /* vertex input */
    const yf_vinput_t *vins = vins_;
    const unsigned vin_n = sizeof vins_ / sizeof *vins_;

    /* graphics state */
    const yf_gconf_t conf = {
        pass,
        stgs,
        stg_n,
        dtbs,
        dtb_n,
        vins,
        vin_n,
        YF_TOPOLOGY_TRIANGLE,
        YF_POLYMODE_FILL,
        YF_CULLMODE_BACK,
        YF_WINDING_CCW
    };

    entry->gst = yf_gstate_init(ctx, &conf);
    if (entry->gst == NULL) {
        yf_unldshd(ctx, vert_shd);
        yf_unldshd(ctx, frag_shd);
        yf_dtable_deinit(inst_dtb);
        return -1;
    }

    return 0;
}

/* Initializes the entry of a given 'resrq' value. */
static int init_entry(int resrq)
{
    assert(resrq >= 0 && resrq < YF_RESRQ_N);
    assert(allocn_[resrq] > 0);
    assert(entries_[resrq].obtained == NULL);

    if (yf_resmgr_getglobl() == NULL)
        return -1;

    const unsigned n = allocn_[resrq];
    entries_[resrq].obtained = calloc(n, sizeof *entries_[resrq].obtained);
    if (entries_[resrq].obtained == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }
    entries_[resrq].n = n;
    entries_[resrq].i = 0;

    int r = -1;

    /* TODO: Consider using spec. constants to set the number of elements. */
    switch (resrq) {
    case YF_RESRQ_MDL:
        r = init_mdl(entries_+resrq, 1);
        break;
    case YF_RESRQ_MDL2:
        r = init_mdl(entries_+resrq, 2);
        break;
    case YF_RESRQ_MDL4:
        r = init_mdl(entries_+resrq, 4);
        break;
    case YF_RESRQ_MDL8:
        r = init_mdl(entries_+resrq, 8);
        break;
    case YF_RESRQ_MDL16:
        r = init_mdl(entries_+resrq, 16);
        break;
    case YF_RESRQ_MDL32:
        r = init_mdl(entries_+resrq, 32);
        break;
    case YF_RESRQ_MDL64:
        r = init_mdl(entries_+resrq, 64);
        break;
    case YF_RESRQ_TERR:
        r = init_terr(entries_+resrq);
        break;
    case YF_RESRQ_PART:
        r = init_part(entries_+resrq);
        break;
    case YF_RESRQ_QUAD:
        r = init_quad(entries_+resrq);
        break;
    case YF_RESRQ_LABL:
        r = init_labl(entries_+resrq);
        break;
    }

    if (r != 0) {
        /* TODO: Move other deinitialization code from 'init_{obj}()' to
           this function.*/
        free(entries_[resrq].obtained);
        memset(entries_+resrq, 0, sizeof *entries_);
    }

    return r;
}

/* Deinitializes the entry of a given 'resrq' value. */
static void deinit_entry(int resrq)
{
    assert(resrq >= 0 && resrq < YF_RESRQ_N);
    assert(entries_[resrq].gst != NULL);
    assert(yf_getctx() != NULL);

    const int stages[] = {
        YF_STAGE_VERT,
        YF_STAGE_TESC,
        YF_STAGE_TESE,
        YF_STAGE_GEOM,
        YF_STAGE_FRAG,
        YF_STAGE_COMP
    };

    for (size_t i = 0; i < (sizeof stages / sizeof *stages); i++) {
        const yf_stage_t *stg = yf_gstate_getstg(entries_[resrq].gst,
                                                 stages[i]);
        if (stg != NULL)
            yf_unldshd(yf_getctx(), stg->shd);
    }

    yf_dtable_deinit(yf_gstate_getdtb(entries_[resrq].gst, YF_RESIDX_INST));
    yf_gstate_deinit(entries_[resrq].gst);
    free(entries_[resrq].obtained);
    memset(entries_+resrq, 0, sizeof(entry_t));
}

yf_gstate_t *yf_resmgr_obtain(int resrq, unsigned *inst_alloc)
{
    assert(resrq >= 0 && resrq < YF_RESRQ_N);
    assert(inst_alloc != NULL);

    if (allocn_[resrq] == 0) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return NULL;
    }

    yf_gstate_t *gst = NULL;
    if (entries_[resrq].gst == NULL) {
        if (init_entry(resrq) == 0) {
            gst = entries_[resrq].gst;
            *inst_alloc = 0;
            entries_[resrq].obtained[0] = 1;
            entries_[resrq].i = 1 % entries_[resrq].n;
        }
    } else {
        unsigned j;
        for (unsigned i = 0; i < entries_[resrq].n; i++) {
            j = (entries_[resrq].i + i) % entries_[resrq].n;
            if (!entries_[resrq].obtained[j]) {
                gst = entries_[resrq].gst;
                *inst_alloc = j;
                entries_[resrq].obtained[j] = 1;
                entries_[resrq].i = (j + 1) % entries_[resrq].n;
                break;
            }
        }
        if (gst == NULL)
            yf_seterr(YF_ERR_INUSE, __func__);
    }
    return gst;
}

void yf_resmgr_yield(int resrq, unsigned inst_alloc)
{
    assert(resrq >= 0 && resrq < YF_RESRQ_N);
    assert(inst_alloc < entries_[resrq].n);

    entries_[resrq].obtained[inst_alloc] = 0;
    entries_[resrq].i = inst_alloc;
}

yf_dtable_t *yf_resmgr_getglobl(void)
{
    if (globl_ != NULL)
        return globl_;

    yf_context_t *ctx = yf_getctx();
    assert(ctx != NULL);

    const yf_dentry_t ents[] = {
        {YF_RESBIND_GLOBL, YF_DTYPE_UNIFORM, 1, NULL},
        {YF_RESBIND_LIGHT, YF_DTYPE_UNIFORM, 1, NULL}
    };

    globl_ = yf_dtable_init(ctx, ents, sizeof ents / sizeof *ents);

    if (globl_ == NULL || yf_dtable_alloc(globl_, 1) != 0) {
        yf_dtable_deinit(globl_);
        globl_ = NULL;
    }

    return globl_;
}

unsigned yf_resmgr_getallocn(int resrq)
{
    assert(resrq >= 0 && resrq < YF_RESRQ_N);
    return entries_[resrq].n;
}

int yf_resmgr_setallocn(int resrq, unsigned n)
{
    assert(resrq >= 0 && resrq < YF_RESRQ_N);

    if (n == allocn_[resrq])
        return 0;

    if (entries_[resrq].gst != NULL) {
        if (n > 0) {
            yf_dtable_t *dtb = yf_gstate_getdtb(entries_[resrq].gst,
                                                YF_RESIDX_INST);
            if (yf_dtable_alloc(dtb, n) != 0) {
                deinit_entry(resrq);
                return -1;
            }
            char *tmp = realloc(entries_[resrq].obtained, n);
            if (tmp == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                deinit_entry(resrq);
                return -1;
            }
            memset(tmp, 0, n);
            entries_[resrq].obtained = tmp;
            entries_[resrq].n = n;
            entries_[resrq].i = 0;
        } else {
            deinit_entry(resrq);
        }
    }

    allocn_[resrq] = n;
    return 0;
}

int yf_resmgr_prealloc(int resrq)
{
    assert(resrq >= 0 && resrq < YF_RESRQ_N);

    if (allocn_[resrq] == 0)
        return 0;

    if (entries_[resrq].gst != NULL) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    return init_entry(resrq);
}

void yf_resmgr_dealloc(int resrq)
{
    assert(resrq >= 0 && resrq < YF_RESRQ_N);

    if (entries_[resrq].gst != NULL)
        deinit_entry(resrq);
}

void yf_resmgr_clear(void)
{
    for (unsigned i = 0; i < YF_RESRQ_N; i++) {
        if (entries_[i].gst != NULL)
            deinit_entry(i);
    }
    yf_dtable_deinit(globl_);
    globl_ = NULL;
}
