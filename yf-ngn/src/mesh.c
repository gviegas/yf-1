/*
 * YF
 * mesh.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "yf/com/yf-util.h"
#include "yf/com/yf-error.h"
#include "yf/core/yf-buffer.h"

#include "mesh.h"
#include "coreobj.h"
#include "data-gltf.h"

/* TODO: Thread-safe. */

/* TODO: Should be defined elsewhere. */
#undef YF_BUFLEN
#define YF_BUFLEN 1048576

struct YF_mesh_o {
    struct {
        size_t offset;
        unsigned stride;
        unsigned n;
    } v;
    struct {
        size_t offset;
        short stride;
        unsigned n;
    } i;
};

/* Type defining an unused range of memory. */
typedef struct {
    size_t offset;
    size_t size;
} T_memblk;

/* Global context. */
static YF_context ctx_ = NULL;

/* Buffer instance. */
static YF_buffer buf_ = NULL;

/* Ordered list of unused memory block ranges. */
static T_memblk *blks_ = NULL;
static size_t blk_n_ = 1;

/* Resizes the buffer instance. */
static size_t resize_buf(size_t new_len)
{
    size_t sz = new_len < SIZE_MAX ? YF_BUFLEN : new_len;
    while (sz < new_len)
        sz <<= 1;

    size_t buf_len = yf_buffer_getsize(buf_);

    if (sz != buf_len) {
        YF_buffer new_buf;
        if ((new_buf = yf_buffer_init(ctx_, sz)) == NULL) {
            if ((new_buf = yf_buffer_init(ctx_, new_len)) == NULL)
                return buf_len;
            else
                sz = new_len;
        }

        YF_cmdbuf cb = yf_cmdbuf_get(ctx_, YF_CMDBUF_XFER);
        if (cb == NULL) {
            yf_buffer_deinit(new_buf);
            return buf_len;
        }
        /* TODO: Copy used range only, not the whole buffer. */
        yf_cmdbuf_copybuf(cb, new_buf, 0, buf_, 0, YF_MIN(sz, buf_len));
        if (yf_cmdbuf_end(cb) != 0) {
            yf_buffer_deinit(new_buf);
            return buf_len;
        }

        if (yf_cmdbuf_exec(ctx_) != 0) {
            yf_cmdbuf_reset(ctx_);
            yf_buffer_deinit(new_buf);
            return buf_len;
        }
        yf_buffer_deinit(buf_);
        buf_ = new_buf;
    }

    return sz;
}

/* Copies mesh data to buffer instance and updates mesh object. */
static int copy_data(YF_mesh mesh, const YF_meshdt *data)
{
    assert(data->v.n <= UINT_MAX);
    assert(data->i.n <= UINT_MAX);

    switch (data->v.vtype) {
    case YF_VTYPE_MDL:
        mesh->v.stride = sizeof(YF_vmdl);
        break;
    case YF_VTYPE_TERR:
        mesh->v.stride = sizeof(YF_vterr);
        break;
    case YF_VTYPE_PART:
        mesh->v.stride = sizeof(YF_vpart);
        break;
    case YF_VTYPE_QUAD:
        mesh->v.stride = sizeof(YF_vquad);
        break;
    case YF_VTYPE_LABL:
        mesh->v.stride = sizeof(YF_vlabl);
        break;
    default:
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    switch (data->i.itype) {
    case YF_ITYPE_USHORT:
        mesh->i.stride = 2;
        break;
    case YF_ITYPE_UINT:
        mesh->i.stride = 4;
        break;
    default:
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    const size_t vtx_sz = data->v.n * mesh->v.stride;
    const size_t idx_sz = data->i.n * mesh->i.stride;
    const size_t sz = vtx_sz + idx_sz;
    size_t blk_i = blk_n_;

    for (size_t i = 0; i < blk_n_; i++) {
        if (blks_[i].size >= sz) {
            blk_i = i;
            break;
        }
    }

    if (blk_i == blk_n_) {
        if (blk_n_ == blk_cap_) {
            size_t new_cap = blk_cap_ + 1;
            if (resize_blks(new_cap) < new_cap)
                return -1;
        }
        size_t buf_len = yf_buffer_getsize(buf_);
        size_t new_len = buf_len + sz;
        int merge_last = 0;
        if (blk_n_ > 0) {
            T_memblk *last = blks_+blk_i-1;
            if (last->offset + last->size == buf_len) {
                new_len -= last->size;
                merge_last = 1;
            }
        }
        if (resize_buf(new_len) < new_len)
            return -1;
        if (merge_last) {
            blks_[--blk_i].size += new_len - buf_len;
        } else {
            blks_[blk_i].offset = buf_len;
            blks_[blk_i].size = new_len - buf_len;
            blk_n_++;
        }
    }

    const size_t off = blks_[blk_i].offset;
    if (yf_buffer_copy(buf_, off, data->v.data, vtx_sz) != 0)
        return -1;
    if (idx_sz > 0) {
        if (yf_buffer_copy(buf_, off + vtx_sz, data->i.data, idx_sz) != 0)
            return -1;
    }
    mesh->v.offset = off;
    mesh->v.n = data->v.n;
    mesh->i.offset = off + vtx_sz;
    mesh->i.n = data->i.n;

    if (blks_[blk_i].size > sz) {
        blks_[blk_i].offset += sz;
        blks_[blk_i].size -= sz;
    } else {
        if (blk_i + 1 != blk_n_) {
            void *dst = blks_+blk_i;
            void *src = blks_+blk_i+1;
            memmove(dst, src, (blk_n_ - blk_i - 1) * sizeof *blks_);
        }
        blk_n_--;
    }

    return 0;
}

YF_mesh yf_mesh_init(int filetype, const char *pathname, size_t index)
{
    switch (filetype) {
    case YF_FILETYPE_INTERNAL:
        /* TODO */
        yf_seterr(YF_ERR_UNSUP, __func__);
        break;

    case YF_FILETYPE_GLTF: {
        YF_datac datac;
        if (yf_loadgltf(pathname, index, YF_DATAC_MESH, &datac) == 0)
            return datac.mesh;
    } break;

    default:
        yf_seterr(YF_ERR_INVARG, __func__);
    }

    return NULL;
}

void yf_mesh_deinit(YF_mesh mesh)
{
    if (mesh == NULL)
        return;

    const size_t vtx_sz = mesh->v.n * mesh->v.stride;
    const size_t idx_sz = mesh->i.n * mesh->i.stride;
    const size_t sz = vtx_sz + idx_sz;
    const size_t off = mesh->v.offset;
    size_t blk_i = blk_n_;

    for (size_t i = 0; i < blk_n_; i++) {
        if (blks_[i].offset > off) {
            blk_i = i;
            break;
        }
    }

    if (blk_n_ == 0) {
        /* no blocks */
        blks_[0].offset = off;
        blks_[0].size = sz;
        blk_n_++;

    } else if (blk_i == blk_n_) {
        /* no next blocks */
        T_memblk *prev = blks_+blk_i-1;
        if (prev->offset + prev->size == off) {
            prev->size += sz;
        } else {
            if (blk_n_ == blk_cap_ && resize_blks(blk_i+1) < blk_i+1)
                /* TODO */
                assert(0);
            blks_[blk_i].offset = off;
            blks_[blk_i].size = sz;
            blk_n_++;
        }

    } else if (blk_i == 0) {
        /* no previous blocks */
        T_memblk *next = blks_;
        if (off + sz == next->offset) {
            next->offset = off;
            next->size += sz;
        } else {
            if (blk_n_ == blk_cap_ && resize_blks(blk_n_+1) < blk_n_+1)
                /* TODO */
                assert(0);
            memmove(next+1, next, blk_n_ * sizeof *blks_);
            blks_[0].offset = off;
            blks_[0].size = sz;
            blk_n_++;
        }

    } else {
        /* previous & next blocks */
        T_memblk *prev = blks_+blk_i-1;
        T_memblk *next = blks_+blk_i;
        int prev_merged = 0;
        int next_merged = 0;
        if (prev->offset + prev->size == off) {
            prev->size += sz;
            prev_merged = 1;
        }
        if (off + sz == next->offset) {
            if (prev_merged) {
                prev->size += next->size;
            } else {
                next->offset = off;
                next->size += sz;
            }
            next_merged = 1;
        }
        if (!prev_merged && !next_merged) {
            if (blk_n_ == blk_cap_ && resize_blks(blk_n_+1) < blk_n_+1)
                /* TODO */
                assert(0);
            memmove(next+1, next, (blk_n_ - blk_i) * sizeof *blks_);
            blks_[blk_i].offset = off;
            blks_[blk_i].size = sz;
            blk_n_++;
        } else if (prev_merged && next_merged) {
            if (blk_i + 1 < blk_n_)
                memmove(next, next+1, (blk_n_ - blk_i) * sizeof *blks_);
            blk_n_--;
        }
    }

    free(mesh);
}

YF_mesh yf_mesh_initdt(const YF_meshdt *data)
{
    assert(data != NULL);

    if (ctx_ == NULL) {
        assert(buf_ == NULL);
        assert(blks_ == NULL);
        if ((ctx_ = yf_getctx()) == NULL)
            return NULL;
        if ((buf_ = yf_buffer_init(ctx_, YF_BUFLEN)) == NULL) {
            ctx_ = NULL;
            return NULL;
        }
        if ((blks_ = malloc(YF_BLKCAP * sizeof *blks_)) == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            ctx_ = NULL;
            yf_buffer_deinit(buf_);
            buf_ = NULL;
            return NULL;
        }
        blks_[0].offset = 0;
        blks_[0].size = yf_buffer_getsize(buf_);
    }

    YF_mesh mesh = calloc(1, sizeof(struct YF_mesh_o));
    if (mesh == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }
    if (copy_data(mesh, data) != 0) {
        yf_mesh_deinit(mesh);
        mesh = NULL;
    }
    return mesh;
}

int yf_mesh_setvtx(YF_mesh mesh, YF_slice range, const void *data)
{
    assert(mesh != NULL);
    assert(range.n > 0);
    assert(data != NULL);

    if (range.i + range.n > mesh->v.n) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    size_t off = mesh->v.offset + mesh->v.stride * range.i;
    size_t sz = mesh->v.stride * range.n;
    return yf_buffer_copy(buf_, off, data, sz);
}

void yf_mesh_draw(YF_mesh mesh, YF_cmdbuf cmdb, unsigned inst_n)
{
    assert(mesh != NULL);
    assert(cmdb != NULL);

    yf_cmdbuf_setvbuf(cmdb, 0, buf_, mesh->v.offset);
    if (mesh->i.n != 0) {
        yf_cmdbuf_setibuf(cmdb, buf_, mesh->i.offset, mesh->i.stride);
        yf_cmdbuf_drawi(cmdb, 0, 0, mesh->i.n, 0, inst_n);
    } else {
        yf_cmdbuf_draw(cmdb, 0, mesh->v.n, 0, inst_n);
    }
}
