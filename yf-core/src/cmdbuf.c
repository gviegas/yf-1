/*
 * YF
 * cmdbuf.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <limits.h>
#include <assert.h>

#include "yf/com/yf-util.h"
#include "yf/com/yf-error.h"

#include "cmdbuf.h"
#include "context.h"
#include "cmdexec.h"

#define YF_CMDCAP 128

/* Grows the command list. */
static int grow_cmds(YF_cmdbuf cmdb);

YF_cmdbuf yf_cmdbuf_get(YF_context ctx, int cmdbuf)
{
  assert(ctx != NULL);

  YF_cmdbuf cmdb = calloc(1, sizeof(YF_cmdbuf_o));
  if (cmdb == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  cmdb->ctx = ctx;
  cmdb->cmdbuf = cmdbuf;
  cmdb->cmds = calloc(YF_CMDCAP, sizeof(YF_cmd));
  if (cmdb->cmds == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    free(cmdb);
    return NULL;
  }
  cmdb->cmd_n = 0;
  cmdb->cmd_cap = YF_CMDCAP;
  cmdb->invalid = 0;

  return cmdb;
}

int yf_cmdbuf_end(YF_cmdbuf cmdb)
{
  assert(cmdb != NULL);

  int r = -1;
  if (!cmdb->invalid)
    r = yf_cmdbuf_decode(cmdb);

  free(cmdb->cmds);
  free(cmdb);
  return r;
}

int yf_cmdbuf_exec(YF_context ctx)
{
  assert(ctx != NULL);
  return yf_cmdexec_exec(ctx);
}

void yf_cmdbuf_reset(YF_context ctx)
{
  assert(ctx != NULL);
  yf_cmdexec_reset(ctx);
}

void yf_cmdbuf_setgstate(YF_cmdbuf cmdb, YF_gstate gst)
{
  assert(cmdb != NULL);
  assert(gst != NULL);

  if (cmdb->invalid)
    return;

  unsigned i;
  switch (cmdb->cmdbuf) {
  case YF_CMDBUF_GRAPH:
    i = cmdb->cmd_n++;
    if (i == cmdb->cmd_cap && grow_cmds(cmdb) != 0) {
      cmdb->invalid = 1;
      return;
    }
    cmdb->cmds[i].cmd = YF_CMD_GST;
    cmdb->cmds[i].gst.gst = gst;
    break;
  default:
    yf_seterr(YF_ERR_INVARG, __func__);
    cmdb->invalid = 1;
  }
}

void yf_cmdbuf_setcstate(YF_cmdbuf cmdb, YF_cstate cst)
{
  assert(cmdb != NULL);
  assert(cst != NULL);

  if (cmdb->invalid)
    return;

  unsigned i;
  switch (cmdb->cmdbuf) {
  case YF_CMDBUF_COMP:
    i = cmdb->cmd_n++;
    if (i == cmdb->cmd_cap && grow_cmds(cmdb) != 0) {
      cmdb->invalid = 1;
      return;
    }
    cmdb->cmds[i].cmd = YF_CMD_CST;
    cmdb->cmds[i].cst.cst = cst;
    break;
  default:
    yf_seterr(YF_ERR_INVARG, __func__);
    cmdb->invalid = 1;
  }
}

void yf_cmdbuf_settarget(YF_cmdbuf cmdb, YF_target tgt)
{
  assert(cmdb != NULL);

  if (cmdb->invalid)
    return;

  unsigned i;
  switch (cmdb->cmdbuf) {
  case YF_CMDBUF_GRAPH:
    i = cmdb->cmd_n++;
    if (i == cmdb->cmd_cap && grow_cmds(cmdb) != 0) {
      cmdb->invalid = 1;
      return;
    }
    cmdb->cmds[i].cmd = YF_CMD_TGT;
    cmdb->cmds[i].tgt.tgt = tgt;
    break;
  default:
    yf_seterr(YF_ERR_INVARG, __func__);
    cmdb->invalid = 1;
  }
}

void yf_cmdbuf_setvport(YF_cmdbuf cmdb, unsigned index,
    const YF_viewport *vport)
{
  assert(cmdb != NULL);
  assert(vport != NULL);

  if (cmdb->invalid)
    return;

  unsigned i;
  switch (cmdb->cmdbuf) {
  case YF_CMDBUF_GRAPH:
    i = cmdb->cmd_n++;
    if (i == cmdb->cmd_cap && grow_cmds(cmdb) != 0) {
      cmdb->invalid = 1;
      return;
    }
    cmdb->cmds[i].cmd = YF_CMD_VPORT;
    cmdb->cmds[i].vport.index = index;
    cmdb->cmds[i].vport.vport = *vport;
    break;
  default:
    yf_seterr(YF_ERR_INVARG, __func__);
    cmdb->invalid = 1;
  }
}

void yf_cmdbuf_setsciss(YF_cmdbuf cmdb, unsigned index, YF_rect rect)
{
  assert(cmdb != NULL);

  if (cmdb->invalid)
    return;

  unsigned i;
  switch (cmdb->cmdbuf) {
  case YF_CMDBUF_GRAPH:
    i = cmdb->cmd_n++;
    if (i == cmdb->cmd_cap && grow_cmds(cmdb) != 0) {
      cmdb->invalid = 1;
      return;
    }
    cmdb->cmds[i].cmd = YF_CMD_SCISS;
    cmdb->cmds[i].sciss.index = index;
    cmdb->cmds[i].sciss.rect = rect;
    break;
  default:
    yf_seterr(YF_ERR_INVARG, __func__);
    cmdb->invalid = 1;
  }
}

void yf_cmdbuf_setdtable(YF_cmdbuf cmdb, unsigned index, unsigned alloc_i)
{
  assert(cmdb != NULL);

  if (cmdb->invalid)
    return;

  unsigned i = cmdb->cmd_n++;
  if (i == cmdb->cmd_cap && grow_cmds(cmdb) != 0) {
    cmdb->invalid = 1;
    return;
  }
  cmdb->cmds[i].cmd = YF_CMD_DTB;
  cmdb->cmds[i].dtb.index = index;
  cmdb->cmds[i].dtb.alloc_i = alloc_i;
}

void yf_cmdbuf_setvbuf(YF_cmdbuf cmdb, unsigned index, YF_buffer buf,
    size_t offset)
{
  assert(cmdb != NULL);
  assert(buf != NULL);

  if (cmdb->invalid)
    return;

  unsigned i;
  switch (cmdb->cmdbuf) {
  case YF_CMDBUF_GRAPH:
    i = cmdb->cmd_n++;
    if (i == cmdb->cmd_cap && grow_cmds(cmdb) != 0) {
      cmdb->invalid = 1;
      return;
    }
    cmdb->cmds[i].cmd = YF_CMD_VBUF;
    cmdb->cmds[i].vbuf.index = index;
    cmdb->cmds[i].vbuf.buf = buf;
    cmdb->cmds[i].vbuf.offset = offset;
    break;
  default:
    yf_seterr(YF_ERR_INVARG, __func__);
    cmdb->invalid = 1;
  }
}

void yf_cmdbuf_setibuf(YF_cmdbuf cmdb, YF_buffer buf, size_t offset,
    unsigned stride)
{
  assert(cmdb != NULL);
  assert(buf != NULL);

  if (cmdb->invalid)
    return;

  unsigned i;
  switch (cmdb->cmdbuf) {
  case YF_CMDBUF_GRAPH:
    i = cmdb->cmd_n++;
    if (i == cmdb->cmd_cap && grow_cmds(cmdb) != 0) {
      cmdb->invalid = 1;
      return;
    }
    cmdb->cmds[i].cmd = YF_CMD_IBUF;
    cmdb->cmds[i].ibuf.buf = buf;
    cmdb->cmds[i].ibuf.offset = offset;
    cmdb->cmds[i].ibuf.stride = stride;
    break;
  default:
    yf_seterr(YF_ERR_INVARG, __func__);
    cmdb->invalid = 1;
  }
}

void yf_cmdbuf_clearcolor(YF_cmdbuf cmdb, unsigned index, YF_color value)
{
  assert(cmdb != NULL);

  if (cmdb->invalid)
    return;

  unsigned i;
  switch (cmdb->cmdbuf) {
  case YF_CMDBUF_GRAPH:
    i = cmdb->cmd_n++;
    if (i == cmdb->cmd_cap && grow_cmds(cmdb) != 0) {
      cmdb->invalid = 1;
      return;
    }
    cmdb->cmds[i].cmd = YF_CMD_CLRCOL;
    cmdb->cmds[i].clrcol.index = index;
    cmdb->cmds[i].clrcol.value = value;
    break;
  default:
    yf_seterr(YF_ERR_INVARG, __func__);
    cmdb->invalid = 1;
  }
}

void yf_cmdbuf_cleardepth(YF_cmdbuf cmdb, float value)
{
  assert(cmdb != NULL);

  if (cmdb->invalid)
    return;

  unsigned i;
  switch (cmdb->cmdbuf) {
  case YF_CMDBUF_GRAPH:
    i = cmdb->cmd_n++;
    if (i == cmdb->cmd_cap && grow_cmds(cmdb) != 0) {
      cmdb->invalid = 1;
      return;
    }
    cmdb->cmds[i].cmd = YF_CMD_CLRDEP;
    cmdb->cmds[i].clrdep.value = value;
    break;
  default:
    yf_seterr(YF_ERR_INVARG, __func__);
    cmdb->invalid = 1;
  }
}

void yf_cmdbuf_clearsten(YF_cmdbuf cmdb, unsigned value)
{
  assert(cmdb != NULL);

  if (cmdb->invalid)
    return;

  unsigned i;
  switch (cmdb->cmdbuf) {
  case YF_CMDBUF_GRAPH:
    i = cmdb->cmd_n++;
    if (i == cmdb->cmd_cap && grow_cmds(cmdb) != 0) {
      cmdb->invalid = 1;
      return;
    }
    cmdb->cmds[i].cmd = YF_CMD_CLRSTEN;
    cmdb->cmds[i].clrsten.value = value;
    break;
  default:
    yf_seterr(YF_ERR_INVARG, __func__);
    cmdb->invalid = 1;
  }
}

void yf_cmdbuf_draw(YF_cmdbuf cmdb, int indexed, unsigned index_base,
    unsigned vert_n, unsigned inst_n, int vert_id, int inst_id)
{
  assert(cmdb != NULL);

  if (cmdb->invalid)
    return;

  unsigned i;
  switch (cmdb->cmdbuf) {
  case YF_CMDBUF_GRAPH:
    i = cmdb->cmd_n++;
    if (i == cmdb->cmd_cap && grow_cmds(cmdb) != 0) {
      cmdb->invalid = 1;
      return;
    }
    cmdb->cmds[i].cmd = YF_CMD_DRAW;
    cmdb->cmds[i].draw.indexed = indexed;
    cmdb->cmds[i].draw.index_base = index_base;
    cmdb->cmds[i].draw.vert_n = vert_n;
    cmdb->cmds[i].draw.inst_n = inst_n;
    cmdb->cmds[i].draw.vert_id = vert_id;
    cmdb->cmds[i].draw.inst_id = inst_id;
    break;
  default:
    yf_seterr(YF_ERR_INVARG, __func__);
    cmdb->invalid = 1;
  }
}

void yf_cmdbuf_dispatch(YF_cmdbuf cmdb, YF_dim3 dim)
{
  assert(cmdb != NULL);

  if (cmdb->invalid)
    return;

  unsigned i;
  switch (cmdb->cmdbuf) {
  case YF_CMDBUF_COMP:
    i = cmdb->cmd_n++;
    if (i == cmdb->cmd_cap && grow_cmds(cmdb) != 0) {
      cmdb->invalid = 1;
      return;
    }
    cmdb->cmds[i].cmd = YF_CMD_DISP;
    cmdb->cmds[i].disp.dim = dim;
    break;
  default:
    yf_seterr(YF_ERR_INVARG, __func__);
    cmdb->invalid = 1;
  }
}

void yf_cmdbuf_copybuf(YF_cmdbuf cmdb, YF_buffer dst, size_t dst_offs,
    YF_buffer src, size_t src_offs, size_t size)
{
  assert(cmdb != NULL);
  assert(dst != NULL);
  assert(src != NULL);

  if (cmdb->invalid)
    return;

  unsigned i = cmdb->cmd_n++;
  if (i == cmdb->cmd_cap && grow_cmds(cmdb) != 0) {
    cmdb->invalid = 1;
    return;
  }
  cmdb->cmds[i].cmd = YF_CMD_CPYBUF;
  cmdb->cmds[i].cpybuf.dst = dst;
  cmdb->cmds[i].cpybuf.dst_offs = dst_offs;
  cmdb->cmds[i].cpybuf.src = src;
  cmdb->cmds[i].cpybuf.src_offs = src_offs;
  cmdb->cmds[i].cpybuf.size = size;
}

void yf_cmdbuf_copyimg(YF_cmdbuf cmdb, YF_image dst, YF_off3 dst_off,
    unsigned dst_layer, unsigned dst_level, YF_image src, YF_off3 src_off,
    unsigned src_layer, unsigned src_level, YF_dim3 dim, unsigned layer_n)
{
  assert(cmdb != NULL);
  assert(dst != NULL);
  assert(src != NULL);

  if (cmdb->invalid)
    return;

  unsigned i = cmdb->cmd_n++;
  if (i == cmdb->cmd_cap && grow_cmds(cmdb) != 0) {
    cmdb->invalid = 1;
    return;
  }
  cmdb->cmds[i].cmd = YF_CMD_CPYIMG;
  cmdb->cmds[i].cpyimg.dst = dst;
  cmdb->cmds[i].cpyimg.dst_off = dst_off;
  cmdb->cmds[i].cpyimg.dst_layer = dst_layer;
  cmdb->cmds[i].cpyimg.dst_level = dst_level;
  cmdb->cmds[i].cpyimg.src = src;
  cmdb->cmds[i].cpyimg.src_off = src_off;
  cmdb->cmds[i].cpyimg.src_layer = src_layer;
  cmdb->cmds[i].cpyimg.src_level = src_level;
  cmdb->cmds[i].cpyimg.dim = dim;
  cmdb->cmds[i].cpyimg.layer_n = layer_n;
}

void yf_cmdbuf_sync(YF_cmdbuf cmdb)
{
  assert(cmdb != NULL);

  if (cmdb->invalid)
    return;

  unsigned i = cmdb->cmd_n++;
  if (i == cmdb->cmd_cap && grow_cmds(cmdb) != 0) {
    cmdb->invalid = 1;
    return;
  }
  cmdb->cmds[i].cmd = YF_CMD_SYNC;
}

static int grow_cmds(YF_cmdbuf cmdb)
{
  assert(cmdb != NULL);

  if (cmdb->cmd_cap == UINT_MAX) {
    yf_seterr(YF_ERR_LIMIT, __func__);
    return -1;
  }

  unsigned cap = cmdb->cmd_cap << 1;
  cap = YF_MAX(cap, cmdb->cmd_cap + 1);

  void *tmp = realloc(cmdb->cmds, cap * sizeof *cmdb->cmds);
  if (tmp == NULL) {
    cap = cmdb->cmd_cap + 1;
    if ((tmp = realloc(cmdb->cmds, cap * sizeof *cmdb->cmds)) == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
  }
  cmdb->cmds = tmp;
  cmdb->cmd_cap = cap;
  return 0;
}
