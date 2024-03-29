/*
 * YF
 * particle.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "yf/com/yf-error.h"

#include "yf-particle.h"
#include "node.h"
#include "mesh.h"

#undef YF_NRND
#define YF_NRND ((float)rand() / (float)RAND_MAX)

#undef YF_LERP
#define YF_LERP(a, b, t) ((1.0f - (t)) * (a) + (t) * (b))

/* State of a single particle. */
typedef struct {
#define YF_PSTATE_UNSET    0
#define YF_PSTATE_SPAWNING 1
#define YF_PSTATE_SPAWNED  2
#define YF_PSTATE_DYING    3
#define YF_PSTATE_DEAD     4
    int pstate;
    float tm;
    float dur;
    float spawn;
    float death;
    float alpha;
    yf_vec3_t vel;
} pstate_t;

struct yf_particle {
    yf_node_t *node;
    unsigned count;
    yf_psys_t sys;
    void *pts;
    pstate_t *sts;
    yf_mesh_t *mesh;
    yf_texture_t *tex;
    /* TODO: Other particle system properties. */
};

/* Initializes vertex data, states and mesh object. */
/* TODO: Use quads instead of points. */
static int init_points(yf_particle_t *part)
{
    assert(part != NULL);

    const size_t pos_sz = sizeof(float[3]) * part->count;
    const size_t clr_sz = sizeof(float[4]) * part->count;

    part->pts = malloc(pos_sz + clr_sz);
    part->sts = malloc(sizeof(pstate_t) * part->count);
    if (part->pts == NULL || part->sts == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }

    const float pos[3] = {0.0f, 0.0f, 0.5f};
    const float clr[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    const pstate_t st = {YF_PSTATE_UNSET, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, {0}};

    unsigned char *pos_dt = part->pts;
    unsigned char *clr_dt = pos_dt + pos_sz;
    for (unsigned i = 0; i < part->count; i++) {
        memcpy(pos_dt, pos, sizeof pos);
        pos_dt += sizeof pos;
        memcpy(clr_dt, clr, sizeof clr);
        clr_dt += sizeof clr;
        memcpy(part->sts+i, &st, sizeof st);
    }

    const yf_meshdt_t data = {
        .prims = &(yf_primdt_t){
            .topology = YF_TOPOLOGY_POINT,
            .vert_n = part->count,
            .indx_n = 0,
            .data_off = 0,
            .vsemt_mask = YF_VSEMT_POS | YF_VSEMT_CLR,
            .attrs = (yf_attrdt_t[]){
                [0] = {YF_VSEMT_POS, YF_VFMT_FLOAT3, 0},
                [1] = {YF_VSEMT_CLR, YF_VFMT_FLOAT4, pos_sz}
            },
            .attr_n = 2,
            .itype = 0,
            .indx_data_off = 0,
            .matl = NULL
        },
        .prim_n = 1,
        .data = part->pts,
        .data_sz = pos_sz + clr_sz
    };

    part->mesh = yf_mesh_init(&data);
    return part->mesh == NULL ? -1 : 0;
}

/* Particle system deinitialization callback. */
static void deinit_part(void *obj)
{
    yf_particle_t *part = obj;

    yf_mesh_deinit(part->mesh);
    free(part->pts);
    free(part->sts);
    free(part);
}

yf_particle_t *yf_particle_init(unsigned count)
{
    if (count == 0) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return NULL;
    }

    yf_particle_t *part = calloc(1, sizeof(yf_particle_t));
    if (part == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }
    if ((part->node = yf_node_init()) == NULL) {
        free(part);
        return NULL;
    }
    yf_node_setobj(part->node, YF_NODEOBJ_PARTICLE, part, deinit_part);
    part->count = count;

    part->sys.emitter.norm[0] = 0.0f;
    part->sys.emitter.norm[1] = 1.0f;
    part->sys.emitter.norm[2] = 0.0f;
    part->sys.emitter.size = 1.0f;
    part->sys.lifetime.duration_min = 0.1f;
    part->sys.lifetime.duration_max = 1.0f;
    part->sys.lifetime.spawn_min = 0.01f;
    part->sys.lifetime.spawn_max = 0.1f;
    part->sys.lifetime.death_min = 0.01f;
    part->sys.lifetime.death_max = 0.1f;
    part->sys.lifetime.once = 0;
    part->sys.color.min[0] = 0.0f;
    part->sys.color.min[1] = 0.0f;
    part->sys.color.min[2] = 0.0f;
    part->sys.color.min[3] = 0.1f;
    part->sys.color.max[0] = 1.0f;
    part->sys.color.max[1] = 1.0f;
    part->sys.color.max[2] = 1.0f;
    part->sys.color.max[3] = 1.0f;
    part->sys.velocity.min[0] = -0.1f;
    part->sys.velocity.min[1] = -0.1f;
    part->sys.velocity.min[2] = -0.1f;
    part->sys.velocity.max[0] = 0.1f;
    part->sys.velocity.max[1] = 0.1f;
    part->sys.velocity.max[2] = 0.1f;

    if (init_points(part) != 0) {
        yf_particle_deinit(part);
        return NULL;
    }

    return part;
}

yf_node_t *yf_particle_getnode(yf_particle_t *part)
{
    assert(part != NULL);
    return part->node;
}

yf_psys_t *yf_particle_getsys(yf_particle_t *part)
{
    assert(part != NULL);
    return &part->sys;
}

yf_mesh_t *yf_particle_getmesh(yf_particle_t *part)
{
    assert(part != NULL);
    return part->mesh;
}

yf_texture_t *yf_particle_gettex(yf_particle_t *part)
{
    assert(part != NULL);
    return part->tex;
}

void yf_particle_settex(yf_particle_t *part, yf_texture_t *tex)
{
    assert(part != NULL);
    part->tex = tex;
}

/* TODO: Simulate on GPU. */
void yf_particle_simulate(yf_particle_t *part, float tm)
{
    assert(tm >= 0.0f);

    const yf_psys_t *sys = &part->sys;
    float *pos = part->pts;
    float *clr = pos + 3 * part->count;
    pstate_t *st = part->sts;

    for (unsigned i = 0; i < part->count; i++) {
        switch (st->pstate) {
        case YF_PSTATE_UNSET:
            pos[0] = sys->emitter.size * (2.0f * YF_NRND - 1.0f);
            pos[1] = sys->emitter.size * (2.0f * YF_NRND - 1.0f);
            pos[2] = sys->emitter.size * (2.0f * YF_NRND - 1.0f);
            clr[0] = YF_LERP(sys->color.min[0], sys->color.max[0], YF_NRND);
            clr[1] = YF_LERP(sys->color.min[1], sys->color.max[1], YF_NRND);
            clr[2] = YF_LERP(sys->color.min[2], sys->color.max[2], YF_NRND);
            clr[3] = 0.0f;

            st->tm = 0.0f;
            st->alpha = YF_LERP(sys->color.min[3], sys->color.max[3], YF_NRND);

            st->dur = YF_LERP(sys->lifetime.duration_min,
                              sys->lifetime.duration_max, YF_NRND);
            st->spawn = YF_LERP(sys->lifetime.spawn_min,
                                sys->lifetime.spawn_max, YF_NRND);
            st->death = YF_LERP(sys->lifetime.death_min,
                                sys->lifetime.death_max, YF_NRND);

            st->vel[0] = YF_LERP(sys->velocity.min[0], sys->velocity.max[0],
                                 YF_NRND);
            st->vel[1] = YF_LERP(sys->velocity.min[1], sys->velocity.max[1],
                                 YF_NRND);
            st->vel[2] = YF_LERP(sys->velocity.min[2], sys->velocity.max[2],
                                 YF_NRND);

            st->pstate = YF_PSTATE_SPAWNING;
            break;

        case YF_PSTATE_SPAWNING:
            if (st->tm >= st->spawn) {
                clr[3] = st->alpha;
                st->tm -= st->spawn;
                st->pstate = YF_PSTATE_SPAWNED;
            } else {
                pos[0] += st->vel[0];
                pos[1] += st->vel[1];
                pos[2] += st->vel[2];
                clr[3] = st->tm / st->spawn * st->alpha;
                st->tm += tm;
            }
            break;

        case YF_PSTATE_SPAWNED:
            if (st->tm >= st->dur) {
                st->tm -= st->dur;
                st->pstate = YF_PSTATE_DYING;
            } else {
                pos[0] += st->vel[0];
                pos[1] += st->vel[1];
                pos[2] += st->vel[2];
                st->tm += tm;
            }
            break;

        case YF_PSTATE_DYING:
            if (st->tm >= st->death) {
                clr[3] = 0.0f;
                st->tm -= st->death;
                st->pstate = YF_PSTATE_DEAD;
            } else {
                pos[0] += st->vel[0];
                pos[1] += st->vel[1];
                pos[2] += st->vel[2];
                clr[3] = st->alpha - st->tm / st->death * st->alpha;
                st->tm += tm;
            }
            break;

        case YF_PSTATE_DEAD:
            if (!sys->lifetime.once) {
                st->tm = 0.0f;
                st->pstate = YF_PSTATE_UNSET;
            }
            break;
        }

        pos += 3;
        clr += 4;
        st++;
    }

#ifdef YF_DEVEL
    if (yf_mesh_setdata(part->mesh, 0, part->pts,
                        sizeof(float[3 + 4]) * part->count) != 0)
        assert(0);
#else
    yf_mesh_setdata(part->mesh, 0, part->pts,
                    sizeof(float[3 + 4]) * part->count);
#endif
}

void yf_particle_deinit(yf_particle_t *part)
{
    if (part != NULL)
        yf_node_deinit(part->node);
}
