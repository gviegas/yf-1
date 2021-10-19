/*
 * YF
 * cmddec.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef __STDC_NO_THREADS__
# error "C11 threads required"
#endif

#include "yf/com/yf-util.h"
#include "yf/com/yf-error.h"

#include "cmdbuf.h"
#include "cmdpool.h"
#include "cmdexec.h"
#include "gstate.h"
#include "cstate.h"
#include "context.h"
#include "buffer.h"
#include "image.h"
#include "pass.h"
#include "dtable.h"
#include "vk.h"
#include "yf-limits.h"

/* Graphics decoding state. */
typedef struct {
    YF_context ctx;
    const YF_cmdres *cmdr;
#define YF_GDEC_GST   0x01
#define YF_GDEC_TGT   0x02
#define YF_GDEC_VPORT 0x04
#define YF_GDEC_SCISS 0x08
#define YF_GDEC_VBUF  0x10
#define YF_GDEC_IBUF  0x20
#define YF_GDEC_PASS  0x03 /* requires gstate and target */
#define YF_GDEC_DRAW  0x1f /* requires everything but index buffer */
#define YF_GDEC_DRAWI 0x3f /* requires everything */
    int gdec;
    YF_pass pass;
    YF_target tgt;
    YF_gstate gst;
    struct {
        int pending;
        unsigned *allocs;
        int *used;
        unsigned n;
    } dtb;
    int clr_pending;
    struct {
        int pending;
        YF_color *vals;
        int *used;
        unsigned n;
    } clrcol;
    struct {
        int pending;
        float val;
    } clrdep;
    struct {
        int pending;
        unsigned val;
    } clrsten;
} T_gdec;

/* Compute decoding state. */
typedef struct {
    YF_context ctx;
    const YF_cmdres *cmdr;
#define YF_CDEC_CST  0x01
#define YF_CDEC_DISP 0x01 /* dispatch only requires cstate */
    int cdec;
    YF_cstate cst;
    struct {
        int pending;
        unsigned *allocs;
        int *used;
        unsigned n;
    } dtb;
} T_cdec;

/* Transfer decoding state. */
typedef struct {
    YF_context ctx;
    const YF_cmdres *cmdr;
} T_xdec;

/* The current decoding states for graph/comp/xfer. */
static _Thread_local T_gdec *gdec_ = NULL;
static _Thread_local T_cdec *cdec_ = NULL;
static _Thread_local T_xdec *xdec_ = NULL;

/* Decodes a 'set gstate' command. */
static int decode_gst(const YF_cmd *cmd)
{
    if (cmd->gst.gst != gdec_->gst) {
        gdec_->gdec |= YF_GDEC_GST;
        gdec_->gst = cmd->gst.gst;

        /* TODO: Check if passes are compatible instead. */
        if (gdec_->pass != NULL && gdec_->pass != gdec_->gst->pass) {
            vkCmdEndRenderPass(gdec_->cmdr->pool_res);
            gdec_->pass = NULL;
        }

        vkCmdBindPipeline(gdec_->cmdr->pool_res,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          gdec_->gst->pipeline);
    }
    return 0;
}

/* Decodes a 'set cstate' command. */
static int decode_cst(const YF_cmd *cmd)
{
    if (cmd->cst.cst != cdec_->cst) {
        cdec_->cdec |= YF_CDEC_CST;
        cdec_->cst = cmd->cst.cst;

        vkCmdBindPipeline(cdec_->cmdr->pool_res,
                          VK_PIPELINE_BIND_POINT_COMPUTE,
                          cdec_->cst->pipeline);
    }
    return 0;
}

/* Decodes a 'set target' command. */
static int decode_tgt(const YF_cmd *cmd)
{
    if (cmd->tgt.tgt != gdec_->tgt) {
        gdec_->gdec |= YF_GDEC_TGT;
        gdec_->tgt = cmd->tgt.tgt;

        if (gdec_->pass != NULL) {
            vkCmdEndRenderPass(gdec_->cmdr->pool_res);
            gdec_->pass = NULL;
        }
    }
    return 0;
}

/* Decodes a 'set viewport' command. */
static int decode_vport(const YF_cmd *cmd)
{
    const YF_limits *lim = yf_getlimits(gdec_->ctx);

    if (cmd->vport.index >= lim->viewport.max ||
        cmd->vport.vport.width <= 0.0f ||
        cmd->vport.vport.width > lim->viewport.dim_max.width ||
        cmd->vport.vport.height <= 0.0f ||
        cmd->vport.vport.height > lim->viewport.dim_max.height ||
        cmd->vport.vport.x < lim->viewport.bounds_min ||
        cmd->vport.vport.x+cmd->vport.vport.width > lim->viewport.bounds_max ||
        cmd->vport.vport.y < lim->viewport.bounds_min ||
        cmd->vport.vport.y+cmd->vport.vport.height > lim->viewport.bounds_max ||
        cmd->vport.vport.min_depth < 0.0f ||
        cmd->vport.vport.min_depth > 1.0f ||
        cmd->vport.vport.max_depth < 0.0f ||
        cmd->vport.vport.max_depth > 1.0f) {

        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    gdec_->gdec |= YF_GDEC_VPORT;

    VkViewport viewport = {
        .x = cmd->vport.vport.x,
        .y = cmd->vport.vport.y,
        .width = cmd->vport.vport.width,
        .height = cmd->vport.vport.height,
        .minDepth = cmd->vport.vport.min_depth,
        .maxDepth = cmd->vport.vport.max_depth
    };

    vkCmdSetViewport(gdec_->cmdr->pool_res, cmd->vport.index, 1, &viewport);
    return 0;
}

/* Decodes a 'set scissor' command. */
static int decode_sciss(const YF_cmd *cmd)
{
    gdec_->gdec |= YF_GDEC_SCISS;

    VkRect2D scissor = {
        .offset = {cmd->sciss.rect.origin.x, cmd->sciss.rect.origin.y},
        .extent = {cmd->sciss.rect.size.width, cmd->sciss.rect.size.height}
    };

    vkCmdSetScissor(gdec_->cmdr->pool_res, cmd->sciss.index, 1, &scissor);
    return 0;
}

/* Decodes a 'set dtable' command. */
static int decode_dtb(int cmdbuf, const YF_cmd *cmd)
{
    switch (cmdbuf) {
    case YF_CMDBUF_GRAPH:
        if (cmd->dtb.index >= yf_getlimits(gdec_->ctx)->state.dtable_max) {
            yf_seterr(YF_ERR_INVARG, __func__);
            return -1;
        }
        gdec_->dtb.pending = 1;
        gdec_->dtb.allocs[cmd->dtb.index] = cmd->dtb.alloc_i;
        if (!gdec_->dtb.used[cmd->dtb.index]) {
            gdec_->dtb.used[cmd->dtb.index] = 1;
            gdec_->dtb.n++;
        }
        break;

    case YF_CMDBUF_COMP:
        if (cmd->dtb.index >= yf_getlimits(cdec_->ctx)->state.dtable_max) {
            yf_seterr(YF_ERR_INVARG, __func__);
            return -1;
        }
        cdec_->dtb.pending = 1;
        cdec_->dtb.allocs[cmd->dtb.index] = cmd->dtb.alloc_i;
        if (!cdec_->dtb.used[cmd->dtb.index]) {
            cdec_->dtb.used[cmd->dtb.index] = 1;
            cdec_->dtb.n++;
        }
        break;

    default:
        assert(0);
        abort();
    }
    return 0;
}

/* Decodes a 'set vertex buffer' command. */
static int decode_vbuf(const YF_cmd *cmd)
{
    if (cmd->vbuf.index >= yf_getlimits(gdec_->ctx)->state.vinput_max) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }
    gdec_->gdec |= YF_GDEC_VBUF;

    vkCmdBindVertexBuffers(gdec_->cmdr->pool_res, cmd->vbuf.index, 1,
                           &cmd->vbuf.buf->buffer, &cmd->vbuf.offset);

    return 0;
}

/* Decodes a 'set index buffer' command. */
static int decode_ibuf(const YF_cmd *cmd)
{
    VkIndexType indx_type;
    switch (cmd->ibuf.itype) {
    case YF_ITYPE_USHORT:
        indx_type = VK_INDEX_TYPE_UINT16;
        break;
    case YF_ITYPE_UINT:
        indx_type = VK_INDEX_TYPE_UINT32;
        break;
    default:
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }
    gdec_->gdec |= YF_GDEC_IBUF;

    vkCmdBindIndexBuffer(gdec_->cmdr->pool_res, cmd->ibuf.buf->buffer,
                         cmd->ibuf.offset, indx_type);

    return 0;
}

/* Decodes a 'clear color' command. */
static int decode_clrcol(const YF_cmd *cmd)
{
    if (cmd->clrcol.index >= yf_getlimits(gdec_->ctx)->pass.color_max) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }
    gdec_->clr_pending = gdec_->clrcol.pending = 1;
    gdec_->clrcol.vals[cmd->clrcol.index] = cmd->clrcol.value;

    if (!gdec_->clrcol.used[cmd->clrcol.index]) {
        gdec_->clrcol.used[cmd->clrcol.index] = 1;
        gdec_->clrcol.n++;
    }
    return 0;
}

/* Decodes a 'clear depth' command. */
static int decode_clrdep(const YF_cmd *cmd)
{
    gdec_->clr_pending = gdec_->clrdep.pending = 1;
    gdec_->clrdep.val = cmd->clrdep.value;
    return 0;
}

/* Decodes a 'clear stencil' command. */
static int decode_clrsten(const YF_cmd *cmd)
{
    gdec_->clr_pending = gdec_->clrsten.pending = 1;
    gdec_->clrsten.val = cmd->clrsten.value;
    return 0;
}

/* Decodes a 'draw' or 'drawi' command. */
static int decode_draw(const YF_cmd *cmd)
{
    if ((gdec_->gdec & YF_GDEC_PASS) != YF_GDEC_PASS) {
        yf_seterr(YF_ERR_INVCMD, __func__);
        return -1;
    }
    if (gdec_->tgt->pass != gdec_->gst->pass) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    /* render pass */
    if (gdec_->pass != gdec_->gst->pass) {
        if (gdec_->pass != NULL)
            vkCmdEndRenderPass(gdec_->cmdr->pool_res);
        gdec_->pass = gdec_->gst->pass;

        VkRenderPassBeginInfo info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = NULL,
            .renderPass = gdec_->pass->ren_pass,
            .framebuffer = gdec_->tgt->framebuf,
            .renderArea = {
                {0, 0},
                {gdec_->tgt->dim.width, gdec_->tgt->dim.height}
            },
            .clearValueCount = 0,
            .pClearValues = NULL
        };

        vkCmdBeginRenderPass(gdec_->cmdr->pool_res, &info,
                             VK_SUBPASS_CONTENTS_INLINE);
    }

    /* dtables */
    if (gdec_->dtb.pending) {
        for (unsigned i = 0, n = 0; ; i++) {
            if (!gdec_->dtb.used[i])
                continue;

            if (i >= gdec_->gst->dtb_n ||
                gdec_->dtb.allocs[i] >= gdec_->gst->dtbs[i]->set_n) {
                yf_seterr(YF_ERR_INVARG, __func__);
                return -1;
            }

            VkDescriptorSet *ds =
                &gdec_->gst->dtbs[i]->sets[gdec_->dtb.allocs[i]];

            vkCmdBindDescriptorSets(gdec_->cmdr->pool_res,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    gdec_->gst->layout, i, 1, ds, 0, NULL);

            if (++n == gdec_->dtb.n)
                break;
        }
        gdec_->dtb.pending = 0;
    }

    /* clear requests */
    if (gdec_->clr_pending) {
        VkClearRect clr_rect = {
            .rect = {{0, 0}, {gdec_->tgt->dim.width, gdec_->tgt->dim.height}},
            .baseArrayLayer = 0,
            .layerCount = gdec_->tgt->layers
        };
        VkClearAttachment *clr_atts = NULL;
        unsigned clr_i = 0;

        if (gdec_->clrcol.pending) {
            clr_atts = malloc(sizeof *clr_atts * (gdec_->clrcol.n+1));
            if (clr_atts == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                return -1;
            }

            for (unsigned i = 0; ; i++) {
                if (gdec_->clrcol.used[i]) {
                    clr_atts[clr_i].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    clr_atts[clr_i].colorAttachment = i;
                    YF_color *vals = gdec_->clrcol.vals;
                    clr_atts[clr_i].clearValue.color.float32[0] = vals[i].r;
                    clr_atts[clr_i].clearValue.color.float32[1] = vals[i].g;
                    clr_atts[clr_i].clearValue.color.float32[2] = vals[i].b;
                    clr_atts[clr_i].clearValue.color.float32[3] = vals[i].a;
                    gdec_->clrcol.used[i] = 0;

                    if (++clr_i == gdec_->clrcol.n)
                        break;
                }
            }

            gdec_->clrcol.n = 0;
            gdec_->clrcol.pending = 0;

        } else {
            clr_atts = malloc(sizeof *clr_atts);
            if (clr_atts == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                return -1;
            }
        }

        clr_atts[clr_i].aspectMask = 0;
        if (gdec_->clrdep.pending) {
            clr_atts[clr_i].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            clr_atts[clr_i].clearValue.depthStencil.depth = gdec_->clrdep.val;
            gdec_->clrdep.pending = 0;
            /* XXX */
            assert(gdec_->clrdep.val >= 0.0f && gdec_->clrdep.val <= 1.0f);
        }
        if (gdec_->clrsten.pending) {
            clr_atts[clr_i].aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            clr_atts[clr_i].clearValue.depthStencil.stencil =
                gdec_->clrsten.val;
            gdec_->clrsten.pending = 0;
        }

        vkCmdClearAttachments(gdec_->cmdr->pool_res,
                              clr_i + (clr_atts[clr_i].aspectMask != 0),
                              clr_atts, 1, &clr_rect);

        free(clr_atts);
        gdec_->clr_pending = 0;
    }

    /* draw */
    int r;
    if (cmd->cmd == YF_CMD_DRAWI) {
        if ((gdec_->gdec & YF_GDEC_DRAWI) == YF_GDEC_DRAWI) {
            vkCmdDrawIndexed(gdec_->cmdr->pool_res, cmd->drawi.vert_n,
                             cmd->drawi.inst_n, cmd->drawi.index_base,
                             cmd->drawi.vert_off, cmd->drawi.inst_id);
            r = 0;
        } else {
            yf_seterr(YF_ERR_INVCMD, __func__);
            r = -1;
        }
    } else {
        if ((gdec_->gdec & YF_GDEC_DRAW) == YF_GDEC_DRAW) {
            vkCmdDraw(gdec_->cmdr->pool_res, cmd->draw.vert_n,
                      cmd->draw.inst_n, cmd->draw.vert_id, cmd->draw.inst_id);
            r = 0;
        } else {
            yf_seterr(YF_ERR_INVCMD, __func__);
            r = -1;
        }
    }
    return r;
}

/* Decodes a 'dispatch' command. */
static int decode_disp(const YF_cmd *cmd)
{
    assert(cmd->disp.dim.width > 0);
    assert(cmd->disp.dim.height > 0);
    assert(cmd->disp.dim.depth > 0);

    const YF_limits *lim = yf_getlimits(cdec_->ctx);
    if (cmd->disp.dim.width > lim->cmdbuf.disp_dim_max.width ||
        cmd->disp.dim.height > lim->cmdbuf.disp_dim_max.height ||
        cmd->disp.dim.depth > lim->cmdbuf.disp_dim_max.depth) {
        yf_seterr(YF_ERR_LIMIT, __func__);
        return -1;
    }

    /* dtables */
    if (cdec_->dtb.pending) {
        if (cdec_->cst == NULL) {
            yf_seterr(YF_ERR_INVCMD, __func__);
            return -1;
        }

        for (unsigned i = 0, n = 0; ; i++) {
            if (!cdec_->dtb.used[i])
                continue;

            if (i >= cdec_->cst->dtb_n ||
                cdec_->dtb.allocs[i] >= cdec_->cst->dtbs[i]->set_n) {
                yf_seterr(YF_ERR_INVARG, __func__);
                return -1;
            }

            VkDescriptorSet *ds =
                &cdec_->cst->dtbs[i]->sets[cdec_->dtb.allocs[i]];

            vkCmdBindDescriptorSets(cdec_->cmdr->pool_res,
                                    VK_PIPELINE_BIND_POINT_COMPUTE,
                                    cdec_->cst->layout, i, 1, ds, 0, NULL);

            if (++n == cdec_->dtb.n)
                break;
        }
        cdec_->dtb.pending = 0;
    }

    /* dispatch */
    int r;
    if ((cdec_->cdec & YF_CDEC_DISP) == YF_CDEC_DISP) {
        vkCmdDispatch(cdec_->cmdr->pool_res, cmd->disp.dim.width,
                      cmd->disp.dim.height, cmd->disp.dim.depth);
        r = 0;
    } else {
        yf_seterr(YF_ERR_INVCMD, __func__);
        r = -1;
    }
    return r;
}

/* Decodes a 'copy buffer' command. */
static int decode_cpybuf(const YF_cmd *cmd)
{
    assert(cmd->cpybuf.dst->size >= cmd->cpybuf.dst_off + cmd->cpybuf.size);
    assert(cmd->cpybuf.src->size >= cmd->cpybuf.src_off + cmd->cpybuf.size);
    assert(cmd->cpybuf.size > 0);

    VkBufferCopy region = {
        .srcOffset = cmd->cpybuf.src_off,
        .dstOffset = cmd->cpybuf.dst_off,
        .size = cmd->cpybuf.size
    };

    vkCmdCopyBuffer(xdec_->cmdr->pool_res, cmd->cpybuf.src->buffer,
                    cmd->cpybuf.dst->buffer, 1, &region);

    return 0;
}

/* Decodes a 'copy image' command. */
static int decode_cpyimg(const YF_cmd *cmd)
{
    assert(cmd->cpyimg.dst->dim.width >=
           cmd->cpyimg.dst_off.x + cmd->cpyimg.dim.width);
    assert(cmd->cpyimg.dst->dim.height >=
           cmd->cpyimg.dst_off.y + cmd->cpyimg.dim.height);
    assert(cmd->cpyimg.dst->dim.depth >=
           cmd->cpyimg.dst_off.z + cmd->cpyimg.dim.depth);
    assert(cmd->cpyimg.dst->layers >=
           cmd->cpyimg.dst_layer + cmd->cpyimg.layer_n);
    assert(cmd->cpyimg.dst->levels > cmd->cpyimg.dst_level);

    assert(cmd->cpyimg.src->dim.width >=
           cmd->cpyimg.src_off.x + cmd->cpyimg.dim.width);
    assert(cmd->cpyimg.src->dim.height >=
           cmd->cpyimg.src_off.y + cmd->cpyimg.dim.height);
    assert(cmd->cpyimg.src->dim.depth >=
           cmd->cpyimg.src_off.z + cmd->cpyimg.dim.depth);
    assert(cmd->cpyimg.src->layers >=
           cmd->cpyimg.src_layer + cmd->cpyimg.layer_n);
    assert(cmd->cpyimg.src->levels > cmd->cpyimg.src_level);

    assert(cmd->cpyimg.dim.width > 0);
    assert(cmd->cpyimg.dim.height > 0);
    assert(cmd->cpyimg.dim.depth > 0);
    assert(cmd->cpyimg.layer_n > 0);

    /* FIXME: These layout changes happen in the priority command buffer. */
    if (cmd->cpyimg.dst->next_layout != VK_IMAGE_LAYOUT_GENERAL &&
        yf_image_chglayout(cmd->cpyimg.dst, VK_IMAGE_LAYOUT_GENERAL) != 0)
        return -1;
    if (cmd->cpyimg.src->next_layout != VK_IMAGE_LAYOUT_GENERAL &&
        yf_image_chglayout(cmd->cpyimg.src, VK_IMAGE_LAYOUT_GENERAL) != 0)
        return -1;

    VkImageCopy region = {
        .srcSubresource = {
            .aspectMask = cmd->cpyimg.src->aspect,
            .mipLevel = cmd->cpyimg.src_level,
            .baseArrayLayer = cmd->cpyimg.src_layer,
            .layerCount = cmd->cpyimg.layer_n
        },
        .srcOffset = {
            .x = cmd->cpyimg.src_off.x,
            .y = cmd->cpyimg.src_off.y,
            .z = cmd->cpyimg.src_off.z
        },
        .dstSubresource = {
            .aspectMask = cmd->cpyimg.dst->aspect,
            .mipLevel = cmd->cpyimg.dst_level,
            .baseArrayLayer = cmd->cpyimg.dst_layer,
            .layerCount = cmd->cpyimg.layer_n
        },
        .dstOffset = {
            .x = cmd->cpyimg.dst_off.x,
            .y = cmd->cpyimg.dst_off.y,
            .z = cmd->cpyimg.dst_off.z
        },
        .extent = {
            .width = cmd->cpyimg.dim.width,
            .height = cmd->cpyimg.dim.height,
            .depth = cmd->cpyimg.dim.depth
        }
    };

    vkCmdCopyImage(xdec_->cmdr->pool_res,
                   cmd->cpyimg.src->image, VK_IMAGE_LAYOUT_GENERAL,
                   cmd->cpyimg.dst->image, VK_IMAGE_LAYOUT_GENERAL,
                   1, &region);

    return 0;
}

/* Decodes a 'synchronize' command. */
static int decode_sync(int cmdbuf)
{
    /* TODO: Provide sync. parameters to avoid such dramatic solution. */

    const YF_cmdres *cmdr;
    switch (cmdbuf) {
    case YF_CMDBUF_GRAPH:
        if (gdec_->pass != NULL) {
            vkCmdEndRenderPass(gdec_->cmdr->pool_res);
            gdec_->pass = NULL;
        }
        cmdr = gdec_->cmdr;
        break;
    case YF_CMDBUF_COMP:
        cmdr = cdec_->cmdr;
        break;
    case YF_CMDBUF_XFER:
        cmdr = xdec_->cmdr;
        break;
    default:
        assert(0);
        abort();
    }

    VkMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT
    };

    vkCmdPipelineBarrier(cmdr->pool_res, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_DEPENDENCY_BY_REGION_BIT, 1, &barrier,
                         0, NULL, 0, NULL);

    return 0;
}

/* Decodes a graphics command buffer. */
static int decode_graph(YF_cmdbuf cmdb, const YF_cmdres *cmdr)
{
    gdec_ = calloc(1, sizeof *gdec_);
    if (gdec_ == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }
    gdec_->ctx = cmdb->ctx;
    gdec_->cmdr = cmdr;

    const unsigned dtb_max = yf_getlimits(cmdb->ctx)->state.dtable_max;
    gdec_->dtb.allocs = calloc(dtb_max, sizeof *gdec_->dtb.allocs);
    gdec_->dtb.used = calloc(dtb_max, sizeof *gdec_->dtb.used);

    const unsigned col_max = yf_getlimits(cmdb->ctx)->pass.color_max;
    gdec_->clrcol.vals = calloc(col_max, sizeof *gdec_->clrcol.vals);
    gdec_->clrcol.used = calloc(col_max, sizeof *gdec_->clrcol.used);

    if (gdec_->dtb.allocs == NULL || gdec_->dtb.used == NULL ||
        gdec_->clrcol.vals == NULL || gdec_->clrcol.used == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        free(gdec_->dtb.allocs);
        free(gdec_->dtb.used);
        free(gdec_->clrcol.vals);
        free(gdec_->clrcol.used);
        free(gdec_);
        gdec_ = NULL;
        return -1;
    }

    int r = 0;
    for (unsigned i = 0; i < cmdb->cmd_n; i++) {
        YF_cmd *cmd = &cmdb->cmds[i];

        switch (cmd->cmd) {
        case YF_CMD_GST:
            r = decode_gst(cmd);
            break;
        case YF_CMD_TGT:
            r = decode_tgt(cmd);
            break;
        case YF_CMD_VPORT:
            r = decode_vport(cmd);
            break;
        case YF_CMD_SCISS:
            r = decode_sciss(cmd);
            break;
        case YF_CMD_DTB:
            r = decode_dtb(YF_CMDBUF_GRAPH, cmd);
            break;
        case YF_CMD_VBUF:
            r = decode_vbuf(cmd);
            break;
        case YF_CMD_IBUF:
            r = decode_ibuf(cmd);
            break;
        case YF_CMD_CLRCOL:
            r = decode_clrcol(cmd);
            break;
        case YF_CMD_CLRDEP:
            r = decode_clrdep(cmd);
            break;
        case YF_CMD_CLRSTEN:
            r = decode_clrsten(cmd);
            break;
        case YF_CMD_DRAW:
        case YF_CMD_DRAWI:
            r = decode_draw(cmd);
            break;
        case YF_CMD_SYNC:
            r = decode_sync(YF_CMDBUF_GRAPH);
            break;
        default:
            assert(0);
            abort();
        }

        if (r != 0)
            break;
    }

    if (gdec_->pass != NULL)
        vkCmdEndRenderPass(cmdr->pool_res);

    /* XXX: Clear commands are deferred until a draw is issued. When a clear
       request comes last, it is handled here. */
    if (r == 0 && gdec_->clr_pending) {
        if (gdec_->tgt == NULL) {
            yf_seterr(YF_ERR_INVCMD, __func__);
            r = -1;

        } else {
            VkImageSubresourceRange range = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = gdec_->tgt->layers
            };

            if (gdec_->clrcol.pending) {
                VkClearColorValue col;
                for (unsigned i = 0, n = 0; ; i++) {
                    if (gdec_->clrcol.used[i]) {
                        assert(gdec_->tgt->pass->color_n > i);

                        col.float32[0] = gdec_->clrcol.vals[i].r;
                        col.float32[1] = gdec_->clrcol.vals[i].g;
                        col.float32[2] = gdec_->clrcol.vals[i].b;
                        col.float32[3] = gdec_->clrcol.vals[i].a;
                        range.baseArrayLayer = gdec_->tgt->lays_base[i];

                        vkCmdClearColorImage(cmdr->pool_res,
                                             gdec_->tgt->imgs[i]->image,
                                             VK_IMAGE_LAYOUT_GENERAL, &col,
                                             1, &range);

                        if (++n == gdec_->clrcol.n)
                            break;
                    }
                }
            }

            if (gdec_->clrdep.pending || gdec_->clrsten.pending) {
                assert(gdec_->tgt->pass->depth_n != 0);

                unsigned index = gdec_->tgt->iview_n-1;
                VkClearDepthStencilValue ds;
                range.baseArrayLayer = gdec_->tgt->lays_base[index];
                range.layerCount = gdec_->tgt->layers;
                range.aspectMask = 0;

                if (gdec_->clrdep.pending) {
                    range.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
                    ds.depth = gdec_->clrdep.val;
                }

                if (gdec_->clrsten.pending) {
                    range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                    ds.stencil = gdec_->clrsten.val;
                }

                vkCmdClearDepthStencilImage(cmdr->pool_res,
                                            gdec_->tgt->imgs[index]->image,
                                            VK_IMAGE_LAYOUT_GENERAL, &ds,
                                            1, &range);
            }
        }
    }

    free(gdec_->dtb.allocs);
    free(gdec_->dtb.used);
    free(gdec_->clrcol.vals);
    free(gdec_->clrcol.used);
    free(gdec_);
    gdec_ = NULL;
    return r;
}

/* Decodes a compute command buffer. */
static int decode_comp(YF_cmdbuf cmdb, const YF_cmdres *cmdr)
{
    cdec_ = calloc(1, sizeof *cdec_);
    if (cdec_ == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }
    cdec_->ctx = cmdb->ctx;
    cdec_->cmdr = cmdr;

    const unsigned dtb_max = yf_getlimits(cmdb->ctx)->state.dtable_max;
    cdec_->dtb.allocs = calloc(dtb_max, sizeof *cdec_->dtb.allocs);
    cdec_->dtb.used = calloc(dtb_max, sizeof *cdec_->dtb.used);

    if (cdec_->dtb.allocs == NULL || cdec_->dtb.used == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        free(cdec_->dtb.allocs);
        free(cdec_->dtb.used);
        free(cdec_);
        cdec_ = NULL;
        return -1;
    }

    int r = 0;
    for (unsigned i = 0; i < cmdb->cmd_n; i++) {
        YF_cmd *cmd = &cmdb->cmds[i];

        switch (cmd->cmd) {
        case YF_CMD_CST:
            r = decode_cst(cmd);
            break;
        case YF_CMD_DTB:
            r = decode_dtb(YF_CMDBUF_COMP, cmd);
            break;
        case YF_CMD_DISP:
            r = decode_disp(cmd);
            break;
        case YF_CMD_SYNC:
            r = decode_sync(YF_CMDBUF_COMP);
            break;
        default:
            assert(0);
            abort();
        }

        if (r != 0)
            break;
    }

    free(cdec_->dtb.allocs);
    free(cdec_->dtb.used);
    free(cdec_);
    cdec_ = NULL;
    return r;
}

/* Decodes a transfer command buffer. */
static int decode_xfer(YF_cmdbuf cmdb, const YF_cmdres *cmdr)
{
    xdec_ = calloc(1, sizeof *xdec_);
    if (xdec_ == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }
    xdec_->ctx = cmdb->ctx;
    xdec_->cmdr = cmdr;

    int r = 0;
    for (unsigned i = 0; i < cmdb->cmd_n; i++) {
        YF_cmd *cmd = &cmdb->cmds[i];

        switch (cmd->cmd) {
        case YF_CMD_CPYBUF:
            r = decode_cpybuf(cmd);
            break;
        case YF_CMD_CPYIMG:
            r = decode_cpyimg(cmd);
            break;
        case YF_CMD_SYNC:
            r = decode_sync(YF_CMDBUF_XFER);
            break;
        default:
            assert(0);
            abort();
        }

        if (r != 0)
            break;
    }

    free(xdec_);
    xdec_ = NULL;
    return r;
}

int yf_cmdbuf_decode(YF_cmdbuf cmdb)
{
    assert(cmdb != NULL);

    if (cmdb->cmd_n == 0)
        /* nothing to decode */
        return 0;

    YF_cmdres cmdr;
    if (yf_cmdpool_obtain(cmdb->ctx, &cmdr) != 0)
        return -1;

    VkCommandBufferBeginInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = NULL
    };

    if (vkBeginCommandBuffer(cmdr.pool_res, &info) != VK_SUCCESS) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        yf_cmdpool_yield(cmdb->ctx, &cmdr);
        return -1;
    }

    int r = 0;
    switch (cmdb->cmdbuf) {
    case YF_CMDBUF_GRAPH:
        if (gdec_ != NULL) {
            yf_seterr(YF_ERR_INUSE, __func__);
            r = -1;
            break;
        }
        r = decode_graph(cmdb, &cmdr);
        break;
    case YF_CMDBUF_COMP:
        if (cdec_ != NULL) {
            yf_seterr(YF_ERR_INUSE, __func__);
            r = -1;
            break;
        }
        r = decode_comp(cmdb, &cmdr);
        break;
    case YF_CMDBUF_XFER:
        if (xdec_ != NULL) {
            yf_seterr(YF_ERR_INUSE, __func__);
            r = -1;
            break;
        }
        r = decode_xfer(cmdb, &cmdr);
        break;
    default:
        assert(0);
        abort();
    }

    if (vkEndCommandBuffer(cmdr.pool_res) != VK_SUCCESS && r == 0) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        r = -1;
    }
    if (r == 0)
        r = yf_cmdexec_enqueue(cmdb->ctx, &cmdr, NULL, NULL);
    if (r != 0)
        yf_cmdpool_yield(cmdb->ctx, &cmdr);

    return r;
}
