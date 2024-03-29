/*
 * YF
 * mesh.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
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
/* TODO: Use a bit map for buffer management. */
/* TODO: Fix alignment within buffer. */

/* TODO: Should be defined elsewhere. */
#undef YF_BUFLEN
#define YF_BUFLEN 1048576

#undef YF_BLKMAX
#define YF_BLKMAX 256

struct yf_mesh {
    yf_primdt_t *prims;
    unsigned prim_n;
    size_t offset;
    size_t size;

    /* mesh whose buffer location precedes this one's */
    yf_mesh_t *prev;
    /* mesh whose buffer location succeeds this one's */
    yf_mesh_t *next;

    /* indicates that deinitialization failed */
    int invalid;
};

/* Unused range of memory. */
typedef struct {
    size_t offset;
    size_t size;

    /* mesh whose buffer location precedes this block */
    yf_mesh_t *prev_mesh;
} memblk_t;

/* Global context. */
static yf_context_t *ctx_ = NULL;

/* Buffer instance. */
static yf_buffer_t *buf_ = NULL;

/* Ordered list of unused memory block ranges. */
static memblk_t blks_[YF_BLKMAX] = {0};
static size_t blk_n_ = 1;

/* First mesh relative to locations in the buffer. */
static yf_mesh_t *head_ = NULL;

/* Last mesh relative to locations in the buffer. */
static yf_mesh_t *tail_ = NULL;

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
        yf_buffer_t *new_buf;
        if ((new_buf = yf_buffer_init(ctx_, sz)) == NULL) {
            if ((new_buf = yf_buffer_init(ctx_, new_len)) == NULL)
                return buf_len;
            else
                sz = new_len;
        }

        yf_cmdbuf_t *cb = yf_cmdbuf_get(ctx_, YF_CMDBUF_XFER);
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
    yf_buffer_t *new_buf = yf_buffer_init(ctx_, buf_sz);
    if (new_buf == NULL)
        return -1;

    yf_cmdbuf_t *cb = yf_cmdbuf_get(ctx_, YF_CMDBUF_XFER);
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
        yf_mesh_t *prev = blks_[i-1].prev_mesh;
        yf_mesh_t *mesh = blks_[i].prev_mesh;
        diff += blks_[i-1].size;
        do
            mesh->offset -= diff;
        while ((mesh = mesh->prev) != prev);
    }

    if (blks_[blk_n_-1].prev_mesh != tail_) {
        yf_mesh_t *prev = blks_[blk_n_-1].prev_mesh;
        yf_mesh_t *mesh = tail_;
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
    yf_mesh_t *mesh = head_;
    size_t n = inval_n_;

    /* TODO: Store first/last invalid meshes to speed this up. */
    while (n-- > 0) {
        while (!mesh->invalid)
            mesh = mesh->next;
        yf_mesh_t *next = mesh->next;
        yf_mesh_deinit(mesh);
        mesh = next;
    }
}

/* Copies mesh data to buffer instance and updates mesh object. */
static int copy_data(yf_mesh_t *mesh, const void *data, size_t size)
{
    assert(mesh != NULL);
    assert(data != NULL);
    assert(size > 0);
    assert(mesh->prims != NULL);
    assert(mesh->prim_n > 0);

    size_t blk_i = blk_n_;

    for (size_t i = 0; i < blk_n_; i++) {
        if (blks_[i].size >= size) {
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
            if (blks_[0].size >= size) {
                blk_i = 0;
                goto no_resz;
            }
            blk_i = 1;
        }

        const size_t buf_len = yf_buffer_getsize(buf_);
        size_t new_len = buf_len + size;
        int merge_last = 0;

        if (blk_n_ > 0) {
            memblk_t *last = blks_+blk_n_-1;
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
    if (yf_buffer_copy(buf_, off, data, size) != 0)
        return -1;

    mesh->offset = off;
    mesh->size = size;

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

    if (blks_[blk_i].size > size) {
        blks_[blk_i].offset += size;
        blks_[blk_i].size -= size;
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
static void invalidate(yf_mesh_t *mesh)
{
    if (!mesh->invalid) {
        mesh->invalid = 1;
        inval_n_++;
    }
}

yf_mesh_t *yf_mesh_load(const char *pathname, size_t index, yf_collec_t *coll)
{
    /* TODO: Consider checking the type of the file. */
    if (coll == NULL)
        coll = yf_collec_get();
    return yf_collec_loaditem(coll, YF_CITEM_MESH, pathname, index);
}

yf_mesh_t *yf_mesh_init(const yf_meshdt_t *data)
{
    assert(data != NULL);
    assert(data->prims != NULL);
    assert(data->prim_n > 0);
    assert(data->data != NULL);
    assert(data->data_sz > 0);

#ifdef YF_DEVEL
    for (unsigned i = 0; i < data->prim_n; i++) {
        assert(data->prims[i].vert_n > 0 && data->prims[i].vert_n <= UINT_MAX);
        assert(data->prims[i].indx_n <= UINT_MAX);
        assert(data->prims[i].attrs != NULL);
        assert(data->prims[i].attr_n > 0);
        /* TODO: More... */
    }
#endif

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

    yf_mesh_t *mesh = calloc(1, sizeof(yf_mesh_t));
    if (mesh == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return NULL;
    }

    mesh->prims = malloc(data->prim_n * sizeof *data->prims);
    if (mesh->prims == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        free(mesh);
        return NULL;
    }

    for (unsigned i = 0; i < data->prim_n; i++) {
        mesh->prims[i] = data->prims[i];
        mesh->prims[i].attrs = malloc(data->prims[i].attr_n *
                                      sizeof *data->prims->attrs);

        if (mesh->prims[i].attrs == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            for (unsigned j = 0; j < i; j++)
                free(mesh->prims[j].attrs);
            free(mesh->prims);
            free(mesh);
            return NULL;
        }

        for (unsigned j = 0; j < data->prims[i].attr_n; j++)
            mesh->prims[i].attrs[j] = data->prims[i].attrs[j];
    }

    mesh->prim_n = data->prim_n;

    if (copy_data(mesh, data->data, data->data_sz) != 0) {
        yf_mesh_deinit(mesh);
        mesh = NULL;
    }

    return mesh;
}

unsigned yf_mesh_getprimn(yf_mesh_t *mesh)
{
    assert(mesh != NULL);
    return mesh->prim_n;
}

yf_material_t *yf_mesh_getmatl(yf_mesh_t *mesh, unsigned prim)
{
    assert(mesh != NULL);
    assert(prim < mesh->prim_n);

    return mesh->prims[prim].matl;
}

yf_material_t *yf_mesh_setmatl(yf_mesh_t *mesh, unsigned prim,
                               yf_material_t *matl)
{
    assert(mesh != NULL);
    assert(prim < mesh->prim_n);

    yf_material_t *prev = mesh->prims[prim].matl;
    mesh->prims[prim].matl = matl;
    return prev;
}

void yf_mesh_deinit(yf_mesh_t *mesh)
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
        memblk_t *prev = blks_+blk_i-1;
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
        memblk_t *next = blks_;
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
        memblk_t *prev = blks_+blk_i-1;
        memblk_t *next = blks_+blk_i;
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

    for (unsigned i = 0; i < mesh->prim_n; i++)
        free(mesh->prims[i].attrs);
    free(mesh->prims);
    free(mesh);
}

int yf_mesh_setdata(yf_mesh_t *mesh, size_t offset, const void *data,
                    size_t size)
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

void yf_mesh_draw(yf_mesh_t *mesh, yf_cmdbuf_t *cmdb, unsigned inst_n)
{
    assert(mesh != NULL);
    assert(cmdb != NULL);
    assert(inst_n > 0);

    /* TODO: Consider managing and binding the graphics states internally. */

    /* FIXME: Multiple primitives may require different states. */
    for (unsigned i = 0; i < mesh->prim_n; i++) {
        const yf_primdt_t *prim = mesh->prims+i;
        const size_t off = mesh->offset + prim->data_off;

        for (unsigned j = 0; j < prim->attr_n; j++) {
            /* FIXME: This assumes an exactly match with state's 'vins'. */
            int vsemt = prim->attrs[j].vsemt;
            unsigned vin = 0;
            while ((vsemt >>= 1) != 0)
                vin++;
            yf_cmdbuf_setvbuf(cmdb, vin, buf_, off + prim->attrs[j].data_off);
        }

        if (mesh->prims[i].indx_n > 0) {
            /* indexed draw */
            yf_cmdbuf_setibuf(cmdb, buf_, off + prim->indx_data_off,
                              prim->itype);
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
        yf_mesh_t *tmp = head_;
        head_ = head_->next;
        yf_mesh_deinit(tmp);
    }
    blk_n_ = 1;
    blks_[0] = (memblk_t){0};
    yf_buffer_deinit(buf_);
    buf_ = NULL;
    ctx_ = NULL;
}

/*
 * DEVEL
 */

#ifdef YF_DEVEL

#define YF_SPANOFMESH(mesh, beg, end) do { \
    beg = (mesh)->offset; \
    end = (mesh)->size; \
    end += beg; } while (0)

void yf_print_mesh(yf_mesh_t *mesh)
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
                YF_SPANOFMESH(blks_[i].prev_mesh, beg, end);
                printf("  prev. mesh: \t[%zu, %zu)\n", beg, end);
            } else {
                printf("  (no prev. mesh)\n");
            }

        }

        puts("\n meshes:");
        yf_mesh_t *next = head_;
        while (next != NULL) {
            YF_SPANOFMESH(next, beg, end);
            printf("  [%zu, %zu)%s\n", beg, end,
                   next->invalid ? "\t<inval.>" : "");
            next = next->next;
        }

        if (head_) {
            YF_SPANOFMESH(head_, beg, end);
            printf("\n head: [%zu, %zu)", beg, end);
            YF_SPANOFMESH(tail_, beg, end);
            printf("\n tail: [%zu, %zu)\n", beg, end);
        } else {
            printf("\n (no meshes)\n");
        }

        printf("\n inval. count: %zu\n", inval_n_);

    } else {
        printf(" mesh <%p>:\n"
               "  offset: %zu\n"
               "  size: %zu\n"
               "  primitives (%u):\n",
               (void *)mesh, mesh->offset, mesh->size, mesh->prim_n);

        for (unsigned i = 0; i < mesh->prim_n; i++) {
            printf("   primitive [%u]:\n"
                   "    topology: %d\n"
                   "    vertex count: %u\n"
                   "    index count: %u\n"
                   "    data offset: %zu\n"
                   "    vertex sematic mask: %xh\n"
                   "    attributes (%u):\n",
                   i, mesh->prims[i].topology, mesh->prims[i].vert_n,
                   mesh->prims[i].indx_n, mesh->prims[i].data_off,
                   mesh->prims[i].vsemt_mask, mesh->prims[i].attr_n);

            for (unsigned j = 0; j < mesh->prims[i].attr_n; j++)
                printf("     attribute [%u]:\n"
                       "      vertex semantic: %xh\n"
                       "      vertex format: %d\n"
                       "      vertex data offset: %zu\n",
                       j, mesh->prims[i].attrs[j].vsemt,
                       mesh->prims[i].attrs[j].vfmt,
                       mesh->prims[i].attrs[j].data_off);

            if (mesh->prims[i].indx_n > 0)
                printf("    index type: %d\n"
                       "    index data offset: %zu\n",
                       mesh->prims[i].itype, mesh->prims[i].indx_data_off);

            if (mesh->prims[i].matl != NULL)
                printf("    material: <%p>\n", (void *)mesh->prims[i].matl);
            else
                puts("    (no material)");
        }
    }

    puts("");
}

#endif
