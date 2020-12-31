/*
 * YF
 * mesh.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <assert.h>

#include <yf/com/yf-error.h>
#include <yf/core/yf-buffer.h>

#include "mesh.h"
#include "vertex.h"
#include "coreobj.h"
#include "filetype.h"
#include "data-obj.h"

#ifdef YF_DEBUG
# include <stdio.h>
# define YF_MESH_PRINT(mesh, info, buf_size) do { \
   printf("\n-- Mesh (debug) --"); \
   printf("\n%s", info); \
   printf("\nv: o=%lu, n=%u", mesh->v.offset, mesh->v.n); \
   printf("\ni: o=%lu, n=%u, strd=%d", \
    mesh->i.offset, mesh->i.n, mesh->i.stride); \
   printf("\nbuf: sz=%lu", buf_size); \
   printf("\nblks: n=%lu, cap=%lu", l_blk_n, l_blk_cap); \
   for (size_t i = 0; i < l_blk_n; ++i) \
    printf("\nblk(%lu): o=%lu, sz=%lu", i, l_blks[i].offset, l_blks[i].size); \
   printf("\n--\n"); } while (0)
#endif

#ifndef YF_MIN
# define YF_MIN(a, b) (a < b ? a : b)
#endif

#undef YF_BUFLEN
#define YF_BUFLEN 1048576

#undef YF_BLKCAP
#define YF_BLKCAP 32

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

/* Type representing a memory block available for use. */
typedef struct {
  size_t offset;
  size_t size;
} L_memblk;

/* Global context. */
static YF_context l_ctx = NULL;

/* Buffer instance. */
static YF_buffer l_buf = NULL;

/* Ordered list of memory blocks available for use. */
static L_memblk *l_blks = NULL;
static size_t l_blk_n = 1;
static size_t l_blk_cap = YF_BLKCAP;

/* Copies mesh data to buffer instance and updates mesh object. */
static int copy_data(YF_mesh mesh, const YF_meshdt *data);

/* Resizes the buffer instance. */
static size_t resize_buf(size_t new_len);

/* Resizes the memory block list. */
static size_t resize_blks(size_t new_cap);

YF_mesh yf_mesh_init(int filetype, const char *pathname) {
  YF_meshdt data = {0};
  switch (filetype) {
    case YF_FILETYPE_INTERNAL:
      /* TODO */
      assert(0);
    case YF_FILETYPE_COLLADA:
      /* TODO */
      assert(0);
    case YF_FILETYPE_OBJ:
      if (yf_loadobj(pathname, &data) != 0)
        return NULL;
      break;
    default:
      yf_seterr(YF_ERR_INVARG, __func__);
      return NULL;
  }
  YF_mesh mesh = yf_mesh_initdt(&data);
  free(data.v.data);
  free(data.i.data);
  return mesh;
}

void yf_mesh_deinit(YF_mesh mesh) {
  if (mesh == NULL)
    return;

  const size_t vtx_sz = mesh->v.n * mesh->v.stride;
  const size_t idx_sz = mesh->i.n * mesh->i.stride;
  const size_t sz = vtx_sz + idx_sz;
  const size_t offs = mesh->v.offset;
  size_t blk_i = l_blk_n;

  for (size_t i = 0; i < l_blk_n; ++i) {
    if (l_blks[i].offset > offs) {
      blk_i = i;
      break;
    }
  }
  if (l_blk_n == 0) {
    /* no blocks */
    l_blks[0].offset = offs;
    l_blks[0].size = sz;
    ++l_blk_n;
  } else if (blk_i == l_blk_n) {
    /* no next blocks */
    L_memblk *prev = l_blks+blk_i-1;
    if (prev->offset + prev->size == offs) {
      prev->size += sz;
    } else {
      if (l_blk_n == l_blk_cap && resize_blks(blk_i+1) < blk_i+1)
        /* TODO */
        assert(0);
      l_blks[blk_i].offset = offs;
      l_blks[blk_i].size = sz;
      ++l_blk_n;
    }
  } else if (blk_i == 0) {
    /* no previous blocks */
    L_memblk *next = l_blks;
    if (offs + sz == next->offset) {
      next->offset = offs;
      next->size += sz;
    } else {
      if (l_blk_n == l_blk_cap && resize_blks(l_blk_n+1) < l_blk_n+1)
        /* TODO */
        assert(0);
      memmove(next+1, next, l_blk_n * sizeof *l_blks);
      l_blks[0].offset = offs;
      l_blks[0].size = sz;
      ++l_blk_n;
    }
  } else {
    /* previous & next blocks */
    L_memblk *prev = l_blks+blk_i-1;
    L_memblk *next = l_blks+blk_i;
    int prev_merged = 0;
    int next_merged = 0;
    if (prev->offset + prev->size == offs) {
      prev->size += sz;
      prev_merged = 1;
    }
    if (offs + sz == next->offset) {
      if (prev_merged) {
        prev->size += next->size;
      } else {
        next->offset = offs;
        next->size += sz;
      }
      next_merged = 1;
    }
    if (!prev_merged && !next_merged) {
      if (l_blk_n == l_blk_cap && resize_blks(l_blk_n+1) < l_blk_n+1)
        /* TODO */
        assert(0);
      memmove(next+1, next, (l_blk_n - blk_i) * sizeof *l_blks);
      l_blks[blk_i].offset = offs;
      l_blks[blk_i].size = sz;
      ++l_blk_n;
    } else if (prev_merged && next_merged) {
      if (blk_i + 1 < l_blk_n)
        memmove(next, next+1, (l_blk_n - blk_i) * sizeof *l_blks);
      --l_blk_n;
    }
  }

#ifdef YF_DEBUG
  size_t buf_len;
  yf_buffer_getval(l_buf, &buf_len);
  YF_MESH_PRINT(mesh, __func__, buf_len);
#endif
  free(mesh);
}

YF_mesh yf_mesh_initdt(const YF_meshdt *data) {
  assert(data != NULL);
#ifdef YF_DEBUG
  YF_MESHDT_PRINT(data);
#endif

  if (l_ctx == NULL) {
    assert(l_buf == NULL);
    assert(l_blks == NULL);
    if ((l_ctx = yf_getctx()) == NULL)
      return NULL;
    if ((l_buf = yf_buffer_init(l_ctx, YF_BUFLEN)) == NULL) {
      l_ctx = NULL;
      return NULL;
    }
    if ((l_blks = malloc(YF_BLKCAP * sizeof *l_blks)) == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      l_ctx = NULL;
      yf_buffer_deinit(l_buf);
      l_buf = NULL;
      return NULL;
    }
    l_blks[0].offset = 0;
    yf_buffer_getval(l_buf, &l_blks[0].size);
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
#ifdef YF_DEBUG
  if (mesh != NULL) {
    size_t buf_len;
    yf_buffer_getval(l_buf, &buf_len);
    YF_MESH_PRINT(mesh, __func__, buf_len);
  }
#endif
  return mesh;
}

int yf_mesh_setvtx(YF_mesh mesh, YF_slice range, const void *data) {
  assert(mesh != NULL);
  assert(range.n > 0);
  assert(data != NULL);

  if (range.i + range.n > mesh->v.n) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
  }

  size_t offs = mesh->v.offset + mesh->v.stride * range.i;
  size_t sz = mesh->v.stride * range.n;
  return yf_buffer_copy(l_buf, offs, data, sz);
}

void yf_mesh_draw(YF_mesh mesh, YF_cmdbuf cmdb, unsigned inst_n, int inst_id) {
  assert(mesh != NULL);
  assert(cmdb != NULL);

  yf_cmdbuf_setvbuf(cmdb, 0, l_buf, mesh->v.offset);
  if (mesh->i.n != 0) {
    yf_cmdbuf_setibuf(cmdb, l_buf, mesh->i.offset, mesh->i.stride);
    yf_cmdbuf_draw(cmdb, 1, 0, mesh->i.n, inst_n, 0, inst_id);
  } else {
    yf_cmdbuf_draw(cmdb, 0, 0, mesh->v.n, inst_n, 0, inst_id);
  }
}

static int copy_data(YF_mesh mesh, const YF_meshdt *data) {
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
    default:
      assert(0);
      yf_seterr(YF_ERR_OTHER, __func__);
      return -1;
  }

  const size_t vtx_sz = data->v.n * mesh->v.stride;
  const size_t idx_sz = data->i.n * data->i.stride;
  const size_t sz = vtx_sz + idx_sz;
  size_t blk_i = l_blk_n;

  for (size_t i = 0; i < l_blk_n; ++i) {
    if (l_blks[i].size >= sz) {
      blk_i = i;
      break;
    }
  }
  if (blk_i == l_blk_n) {
    if (l_blk_n == l_blk_cap) {
      size_t new_cap = l_blk_cap + 1;
      if (resize_blks(new_cap) < new_cap)
        return -1;
    }
    size_t buf_len;
    yf_buffer_getval(l_buf, &buf_len);
    size_t new_len = buf_len + sz;
    int merge_last = 0;
    if (l_blk_n > 0) {
      L_memblk *last = l_blks+blk_i-1;
      if (last->offset + last->size == buf_len) {
        new_len -= last->size;
        merge_last = 1;
      }
    }
    if (resize_buf(new_len) < new_len)
      return -1;
    if (merge_last) {
      l_blks[--blk_i].size += new_len - buf_len;
    } else {
      l_blks[blk_i].offset = buf_len;
      l_blks[blk_i].size = new_len - buf_len;
      ++l_blk_n;
    }
  }

  const size_t offs = l_blks[blk_i].offset;
  if (yf_buffer_copy(l_buf, offs, data->v.data, vtx_sz) != 0)
    return -1;
  if (idx_sz > 0) {
    if (yf_buffer_copy(l_buf, offs + vtx_sz, data->i.data, idx_sz) != 0)
      return -1;
  }
  mesh->v.offset = offs;
  mesh->v.n = data->v.n;
  mesh->i.offset = offs + vtx_sz;
  mesh->i.stride = data->i.stride;
  mesh->i.n = data->i.n;

  if (l_blks[blk_i].size > sz) {
    l_blks[blk_i].offset += sz;
    l_blks[blk_i].size -= sz;
  } else {
    if (blk_i + 1 != l_blk_n) {
      void *dst = l_blks+blk_i;
      void *src = l_blks+blk_i+1;
      memmove(dst, src, (l_blk_n - blk_i - 1) * sizeof *l_blks);
    }
    --l_blk_n;
  }
  return 0;
}

static size_t resize_buf(size_t new_len) {
  size_t sz = new_len < SIZE_MAX ? YF_BUFLEN : new_len;
  while (sz < new_len)
    sz *= 2;
  size_t buf_len;
  yf_buffer_getval(l_buf, &buf_len);

  if (sz != buf_len) {
    YF_buffer new_buf;
    if ((new_buf = yf_buffer_init(l_ctx, sz)) == NULL) {
      if ((new_buf = yf_buffer_init(l_ctx, new_len)) == NULL)
        return buf_len;
      else
        sz = new_len;
    }
    YF_cmdbuf cb = yf_cmdbuf_get(l_ctx, YF_CMDBUF_GRAPH);
    if (cb == NULL && (cb = yf_cmdbuf_get(l_ctx, YF_CMDBUF_COMP)) == NULL) {
      yf_buffer_deinit(new_buf);
      return buf_len;
    }
    /* TODO: Copy used range only, not the whole buffer. */
    yf_cmdbuf_copybuf(cb, new_buf, 0, l_buf, 0, YF_MIN(sz, buf_len));
    if (yf_cmdbuf_end(cb) != 0) {
      yf_buffer_deinit(new_buf);
      return buf_len;
    }
    if (yf_cmdbuf_exec(l_ctx) != 0) {
      yf_cmdbuf_reset(l_ctx);
      yf_buffer_deinit(new_buf);
      return buf_len;
    }
    yf_buffer_deinit(l_buf);
    l_buf = new_buf;
  }

  return sz;
}

static size_t resize_blks(size_t new_cap) {
  if (new_cap > SIZE_MAX / sizeof(L_memblk)) {
    yf_seterr(YF_ERR_OFLOW, __func__);
    return l_blk_cap;
  }
  size_t n = new_cap * sizeof(L_memblk) < SIZE_MAX ? YF_BLKCAP : new_cap;
  while (n < new_cap)
    n *= 2;
  if (n != l_blk_cap) {
    L_memblk *tmp = realloc(l_blks, n * sizeof(L_memblk));
    if (tmp == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      n = l_blk_cap;
    } else {
      l_blks = tmp;
      l_blk_cap = n;
    }
  }
  return n;
}
