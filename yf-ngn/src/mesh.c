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

#ifdef YF_DEVEL
# include <stdio.h>
#endif

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

#undef YF_BLKMAX
#define YF_BLKMAX 256

struct YF_mesh_o {
    YF_primdt *prims;
    unsigned prim_n;
    size_t offset;
    size_t size;

    /* mesh whose buffer location precedes this one's */
    YF_mesh prev;
    /* mesh whose buffer location succeeds this one's */
    YF_mesh next;

    /* indicates that deinitialization failed */
    int invalid;
};

/* Type defining an unused range of memory. */
typedef struct {
    size_t offset;
    size_t size;

    /* mesh whose buffer location precedes this block */
    YF_mesh prev_mesh;
} T_memblk;

/* Global context. */
static YF_context ctx_ = NULL;

/* Buffer instance. */
static YF_buffer buf_ = NULL;

/* Ordered list of unused memory block ranges. */
static T_memblk blks_[YF_BLKMAX] = {0};
static size_t blk_n_ = 1;

/* First mesh relative to locations in the buffer. */
static YF_mesh head_ = NULL;

/* Last mesh relative to locations in the buffer. */
static YF_mesh tail_ = NULL;

/* Number of invalid meshes. */
static size_t inval_n_ = 0;

/* Resizes the buffer instance. */
static size_t resize_buf(size_t new_len)
{
    assert(ctx_ != NULL);
    assert(buf_ != NULL);

    size_t sz = new_len < SIZE_MAX ? YF_BUFLEN : new_len;
    while (sz < new_len)
        sz <<= 1;

    const size_t buf_len = yf_buffer_getsize(buf_);

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

/* Trims buffer instance/memory blocks. */
static int trim_mem(void)
{
    assert(ctx_ != NULL);
    assert(buf_ != NULL);

    if (blk_n_ <= 1)
        return 0;

    /* trimmed data will be copied into a new buffer */
    const size_t buf_sz = yf_buffer_getsize(buf_);
    YF_buffer new_buf = yf_buffer_init(ctx_, buf_sz);
    if (new_buf == NULL)
        return -1;

    YF_cmdbuf cb = yf_cmdbuf_get(ctx_, YF_CMDBUF_XFER);
    if (cb == NULL) {
        yf_buffer_deinit(new_buf);
        return -1;
    }

    size_t dst_off, src_off;

    if (blks_[0].prev_mesh != NULL) {
        /* copy data that precedes the first block */
        yf_cmdbuf_copybuf(cb, new_buf, 0, buf_, 0, blks_[0].offset);
        dst_off = blks_[0].offset;
        src_off = blks_[0].offset + blks_[0].size;
    } else {
        dst_off = 0;
        src_off = blks_[0].size;
    }

    /* copy data that is located between blocks */
    for (size_t i = 1; i < blk_n_; i++) {
        size_t sz = blks_[i].offset - src_off;
        yf_cmdbuf_copybuf(cb, new_buf, dst_off, buf_, src_off, sz);
        dst_off += sz;
        src_off = blks_[i].offset + blks_[i].size;
    }

    if (blks_[blk_n_-1].prev_mesh != tail_) {
        /* copy data that succeeds the last block */
        size_t sz = buf_sz - src_off;
        yf_cmdbuf_copybuf(cb, new_buf, dst_off, buf_, src_off, sz);
        dst_off += sz;
    }

    if (yf_cmdbuf_end(cb) != 0) {
        yf_buffer_deinit(new_buf);
        return -1;
    }
    if (yf_cmdbuf_exec(ctx_) != 0) {
        yf_cmdbuf_reset(ctx_);
        yf_buffer_deinit(new_buf);
        return -1;
    }

    yf_buffer_deinit(buf_);
    buf_ = new_buf;

    /* mesh offsets need to match new buffer locations */
    size_t diff = 0;

    for (size_t i = 1; i < blk_n_; i++) {
        YF_mesh prev = blks_[i-1].prev_mesh;
        YF_mesh mesh = blks_[i].prev_mesh;
        diff += blks_[i-1].size;
        do
            mesh->offset -= diff;
        while ((mesh = mesh->prev) != prev);
    }

    if (blks_[blk_n_-1].prev_mesh != tail_) {
        YF_mesh prev = blks_[blk_n_-1].prev_mesh;
        YF_mesh mesh = tail_;
        diff += blks_[blk_n_-1].size;
        do
            mesh->offset -= diff;
        while ((mesh = mesh->prev) != prev);
    }

    /* unused memory is now contiguous */
    blks_[0].offset = dst_off;
    blks_[0].size = buf_sz - dst_off;
    blks_[0].prev_mesh = tail_;
    blk_n_ = 1;

    return 0;
}

/* Tries to deinitialize invalid meshes. */
static void try_release(void)
{
    YF_mesh mesh = head_;
    size_t n = inval_n_;

    /* TODO: Store first/last invalid meshes to speed this up. */
    while (n-- > 0) {
        while (!mesh->invalid)
            mesh = mesh->next;
        YF_mesh next = mesh->next;
        yf_mesh_deinit(mesh);
        mesh = next;
    }
}

/* Copies mesh data to buffer instance and updates mesh object. */
static int copy_data(YF_mesh mesh, const YF_meshdt *data)
{
    assert(mesh != NULL);
    assert(data != NULL);
    assert(data->prims != NULL && data->prim_n > 0);
    assert(data->data != NULL && data->data_sz > 0);

#ifdef YF_DEVEL
    for (unsigned i = 0; i < data->prim_n; i++) {
        assert(data->prims[i].vert_n > 0 && data->prims[i].vert_n <= UINT_MAX);
        assert(data->prims[i].indx_n <= UINT_MAX);
        assert(data->prims[i].attrs != NULL && data->prims[i].attr_n > 0);
        /* TODO... */
    }
#endif

    const size_t sz = data->data_sz;
    size_t blk_i = blk_n_;

    for (size_t i = 0; i < blk_n_; i++) {
        if (blks_[i].size >= sz) {
            blk_i = i;
            break;
        }
    }

    if (blk_i == blk_n_) {
        if (blk_n_ == YF_BLKMAX) {
            if (trim_mem() != 0)
                return -1;
            /* if enough memory is made available after trimming,
               buffer resizing can be omitted */
            if (blks_[0].size >= sz) {
                blk_i = 0;
                goto no_resz;
            }
            blk_i = 1;
        }

        const size_t buf_len = yf_buffer_getsize(buf_);
        size_t new_len = buf_len + sz;
        int merge_last = 0;

        if (blk_n_ > 0) {
            T_memblk *last = blks_+blk_n_-1;
            if (last->offset + last->size == buf_len) {
                new_len -= last->size;
                merge_last = 1;
            }
        }

        new_len = resize_buf(new_len);
        if (new_len == buf_len)
            return -1;

        if (merge_last) {
            blks_[--blk_i].size += new_len - buf_len;
        } else {
            blks_[blk_i].offset = buf_len;
            blks_[blk_i].size = new_len - buf_len;
            blks_[blk_i].prev_mesh = tail_;
            blk_n_++;
        }

no_resz:
        ;
    }

    const size_t off = blks_[blk_i].offset;
    if (yf_buffer_copy(buf_, off, data->data, sz) != 0)
        return -1;

    mesh->offset = off;
    mesh->size = sz;

    mesh->prev = blks_[blk_i].prev_mesh;
    if (mesh->prev != NULL) {
        mesh->next = mesh->prev->next;
        mesh->prev->next = mesh;
        if (mesh->next != NULL)
            mesh->next->prev = mesh;
        else
            tail_ = mesh;
    } else {
        if (head_ != NULL) {
            mesh->next = head_;
            head_->prev = mesh;
        } else {
            tail_ = mesh;
        }
        head_ = mesh;
    }

    mesh->invalid = 0;

    if (blks_[blk_i].size > sz) {
        blks_[blk_i].offset += sz;
        blks_[blk_i].size -= sz;
        blks_[blk_i].prev_mesh = mesh;
    } else {
        if (blk_i + 1 != blk_n_) {
            void *dst = blks_+blk_i;
            void *src = blks_+blk_i+1;
            memmove(dst, src, (blk_n_ - blk_i - 1) * sizeof *blks_);
        }
        blk_n_--;
        if (inval_n_ > 0)
            try_release();
    }

    return 0;
}

/* Invalidates a mesh. */
static void invalidate(YF_mesh mesh)
{
    if (!mesh->invalid) {
        mesh->invalid = 1;
        inval_n_++;
    }
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

    size_t blk_i = blk_n_;
    for (size_t i = 0; i < blk_n_; i++) {
        if (blks_[i].offset > mesh->offset) {
            blk_i = i;
            break;
        }
    }

    if (blk_n_ == 0) {
        /* no blocks */
        blks_[0].offset = mesh->offset;
        blks_[0].size = mesh->size;
        blks_[0].prev_mesh = mesh->prev;
        blk_n_++;

    } else if (blk_i == blk_n_) {
        /* no next blocks */
        T_memblk *prev = blks_+blk_i-1;
        if (prev->offset + prev->size == mesh->offset) {
            prev->size += mesh->size;
        } else if (blk_n_ < YF_BLKMAX) {
            blks_[blk_i].offset = mesh->offset;
            blks_[blk_i].size = mesh->size;
            blks_[blk_i].prev_mesh = mesh->prev;
            blk_n_++;
        } else {
            if (trim_mem() == 0)
                yf_mesh_deinit(mesh);
            else
                invalidate(mesh);
            return;
        }

    } else if (blk_i == 0) {
        /* no previous blocks */
        T_memblk *next = blks_;
        if (mesh->offset + mesh->size == next->offset) {
            next->offset = mesh->offset;
            next->size += mesh->size;
            next->prev_mesh = mesh->prev;
        } else if (blk_n_ < YF_BLKMAX) {
            memmove(next+1, next, blk_n_ * sizeof *blks_);
            blks_[0].offset = mesh->offset;
            blks_[0].size = mesh->size;
            blks_[0].prev_mesh = mesh->prev;
            blk_n_++;
        } else {
            if (trim_mem() == 0)
                yf_mesh_deinit(mesh);
            else
                invalidate(mesh);
            return;
        }

    } else {
        /* previous & next blocks */
        T_memblk *prev = blks_+blk_i-1;
        T_memblk *next = blks_+blk_i;
        int prev_merged = 0;
        int next_merged = 0;
        if (prev->offset + prev->size == mesh->offset) {
            prev->size += mesh->size;
            prev_merged = 1;
        }
        if (mesh->offset + mesh->size == next->offset) {
            if (prev_merged) {
                prev->size += next->size;
            } else {
                next->offset = mesh->offset;
                next->size += mesh->size;
                next->prev_mesh = mesh->prev;
            }
            next_merged = 1;
        }
        if (!prev_merged && !next_merged) {
            if (blk_n_ < YF_BLKMAX) {
                memmove(next+1, next, (blk_n_ - blk_i) * sizeof *blks_);
                blks_[blk_i].offset = mesh->offset;
                blks_[blk_i].size = mesh->size;
                blks_[blk_i].prev_mesh = mesh->prev;
                blk_n_++;
            } else {
                if (trim_mem() == 0)
                    yf_mesh_deinit(mesh);
                else
                    invalidate(mesh);
                return;
            }
        } else if (prev_merged && next_merged) {
            if (blk_i + 1 < blk_n_)
                memmove(next, next+1, (blk_n_ - blk_i) * sizeof *blks_);
            blk_n_--;
        }
    }

    if (mesh->prev != NULL) {
        mesh->prev->next = mesh->next;
        if (mesh->next != NULL)
            mesh->next->prev = mesh->prev;
        else
            tail_ = mesh->prev;
    } else {
        if (mesh->next != NULL)
            mesh->next->prev = NULL;
        head_ = mesh->next;
    }

    if (mesh->invalid) {
        inval_n_--;
    } else {
        if (inval_n_ > 0 && (blk_n_ < YF_BLKMAX ||
                             (mesh->prev != NULL && mesh->prev->invalid) ||
                             (mesh->next != NULL && mesh->next->invalid)))
            try_release();
    }

    /* TODO: Consider resizing the buffer down if too much mem. goes unused. */

    free(mesh);
}

YF_mesh yf_mesh_initdt(const YF_meshdt *data)
{
    assert(data != NULL);
    assert(data->prims != NULL && data->prim_n > 0);
    assert(data->data != NULL && data->data_sz > 0);

    if (ctx_ == NULL) {
        if ((ctx_ = yf_getctx()) == NULL ||
            (buf_ = yf_buffer_init(ctx_, YF_BUFLEN)) == NULL)
            return (ctx_ = NULL, NULL);

        blks_[0].offset = 0;
        blks_[0].size = yf_buffer_getsize(buf_);
        blks_[0].prev_mesh = NULL;
        blk_n_ = 1;

        head_ = NULL;
        tail_ = NULL;

        inval_n_ = 0;
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

int yf_mesh_setdata(YF_mesh mesh, size_t offset, const void *data, size_t size)
{
    assert(mesh != NULL);
    assert(data != NULL);
    assert(size > 0);

    if (offset + size > mesh->size) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    return yf_buffer_copy(buf_, mesh->offset + offset, data, size);
}

void yf_mesh_draw(YF_mesh mesh, YF_cmdbuf cmdb, unsigned inst_n)
{
    assert(mesh != NULL);
    assert(cmdb != NULL);
    assert(inst_n > 0);

    /* TODO: Consider managing and binding the graphics states internally. */

    /* FIXME: Multiple primitives may require different states. */
    for (unsigned i = 0; i < mesh->prim_n; i++) {
        const YF_primdt *prim = mesh->prims+i;
        const size_t off = mesh->offset + prim->data_off;

        for (unsigned j = 0; j < prim->attr_n; j++)
            /* FIXME: This assumes an exactly match with state's 'vins'. */
            yf_cmdbuf_setvbuf(cmdb, j, buf_, off + prim->attrs[j].data_off);

        if (mesh->prims[i].indx_n > 0) {
            /* indexed draw */
            unsigned isz;
            switch (mesh->prims[i].itype) {
            case YF_ITYPE_UINT:
                isz = 4;
                break;
            case YF_ITYPE_USHORT:
                isz = 2;
                break;
            default:
                assert(0);
                abort();
            }
            yf_cmdbuf_setibuf(cmdb, buf_, off + prim->indx_data_off, isz);
            yf_cmdbuf_drawi(cmdb, 0, 0, mesh->prims[i].indx_n, 0, inst_n);

        } else {
            /* non-indexed draw */
            yf_cmdbuf_draw(cmdb, 0, mesh->prims[i].vert_n, 0, inst_n);
        }
    }
}

/* Called by 'coreobj' on exit. */
void yf_unsetmesh(void)
{
    inval_n_ = 0;
    tail_ = NULL;
    while (head_ != NULL) {
        YF_mesh tmp = head_;
        head_ = head_->next;
        yf_mesh_deinit(tmp);
    }
    blk_n_ = 1;
    blks_[0] = (T_memblk){0};
    yf_buffer_deinit(buf_);
    buf_ = NULL;
    ctx_ = NULL;
}

/*
 * DEVEL
 */

#ifdef YF_DEVEL

#define YF_SPANOFMESH(mesh, beg_p, end_p) do { \
    *(beg_p) = (mesh)->v.offset; \
    *(end_p) = (mesh)->v.stride * (mesh)->v.n; \
    if ((mesh)->i.n > 0) \
        *(end_p) += (mesh)->i.stride * (mesh)->i.n; \
    *(end_p) += *(beg_p); } while (0)

void yf_print_mesh(YF_mesh mesh)
{
    assert(ctx_ != NULL);

    printf("\n[YF] OUTPUT (%s):\n", __func__);

    if (mesh == NULL) {
        size_t beg, end;

        for (size_t i = 0; i < blk_n_; i++) {
            printf(" mem. block #%zu:\n"
                   "  offset: \t%zu\n"
                   "  size:   \t%zu\n",
                   i, blks_[i].offset, blks_[i].size);

            if (blks_[i].prev_mesh != NULL) {
                YF_SPANOFMESH(blks_[i].prev_mesh, &beg, &end);
                printf("  prev. mesh: \t[%zu, %zu)\n", beg, end);
            } else {
                printf("  (no prev. mesh)\n");
            }

        }

        puts("\n meshes:");
        YF_mesh next = head_;
        while (next != NULL) {
            YF_SPANOFMESH(next, &beg, &end);
            printf("  [%zu, %zu)%s\n", beg, end,
                   next->invalid ? "\t<inval.>" : "");
            next = next->next;
        }

        if (head_) {
            YF_SPANOFMESH(head_, &beg, &end);
            printf("\n head: [%zu, %zu)", beg, end);
            YF_SPANOFMESH(tail_, &beg, &end);
            printf("\n tail: [%zu, %zu)\n", beg, end);
        } else {
            printf("\n (no meshes)\n");
        }

        printf("\n inval. count: %zu\n", inval_n_);

    } else {
        printf(" mesh <%p>:\n"
               "  vertex:\n"
               "   offset: %zu\n"
               "   stride: %u\n"
               "   count:  %u\n"
               "  index:\n"
               "   offset: %zu\n"
               "   stride: %hd\n"
               "   count:  %u\n",
               (void *)mesh,
               mesh->v.offset, mesh->v.stride, mesh->v.n,
               mesh->i.offset, mesh->i.stride, mesh->i.n);
    }

    puts("");
}

#endif
