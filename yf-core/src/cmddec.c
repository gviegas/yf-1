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

/* Type defining graphics decoding state. */
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

/* Type defining compute decoding state. */
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

/* Type defining transfer decoding state. */
typedef struct {
    YF_context ctx;
    const YF_cmdres *cmdr;
} T_xdec;

/* The current decoding states for graph/comp/xfer. */
static _Thread_local T_gdec *l_gdec = NULL;
static _Thread_local T_cdec *l_cdec = NULL;
static _Thread_local T_xdec *l_xdec = NULL;

/* Decodes a 'set gstate' command. */
static int decode_gst(const YF_cmd *cmd)
{
    if (cmd->gst.gst != l_gdec->gst) {
        l_gdec->gdec |= YF_GDEC_GST;
        l_gdec->gst = cmd->gst.gst;

        /* TODO: Check if passes are compatible instead. */
        if (l_gdec->pass != NULL && l_gdec->pass != l_gdec->gst->pass) {
            vkCmdEndRenderPass(l_gdec->cmdr->pool_res);
            l_gdec->pass = NULL;
        }

        vkCmdBindPipeline(l_gdec->cmdr->pool_res,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          l_gdec->gst->pipeline);
    }
    return 0;
}

/* Decodes a 'set cstate' command. */
static int decode_cst(const YF_cmd *cmd)
{
    if (cmd->cst.cst != l_cdec->cst) {
        l_cdec->cdec |= YF_CDEC_CST;
        l_cdec->cst = cmd->cst.cst;

        vkCmdBindPipeline(l_cdec->cmdr->pool_res,
                          VK_PIPELINE_BIND_POINT_COMPUTE,
                          l_cdec->cst->pipeline);
    }
    return 0;
}

/* Decodes a 'set target' command. */
static int decode_tgt(const YF_cmd *cmd)
{
    if (cmd->tgt.tgt != l_gdec->tgt) {
        l_gdec->gdec |= YF_GDEC_TGT;
        l_gdec->tgt = cmd->tgt.tgt;

        if (l_gdec->pass != NULL) {
            vkCmdEndRenderPass(l_gdec->cmdr->pool_res);
            l_gdec->pass = NULL;
        }
    }
    return 0;
}

/* Decodes a 'set viewport' command. */
static int decode_vport(const YF_cmd *cmd)
{
    const YF_limits *lim = yf_getlimits(l_gdec->ctx);

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

    l_gdec->gdec |= YF_GDEC_VPORT;

    VkViewport viewport = {
        .x = cmd->vport.vport.x,
        .y = cmd->vport.vport.y,
        .width = cmd->vport.vport.width,
        .height = cmd->vport.vport.height,
        .minDepth = cmd->vport.vport.min_depth,
        .maxDepth = cmd->vport.vport.max_depth
    };

    vkCmdSetViewport(l_gdec->cmdr->pool_res, cmd->vport.index, 1, &viewport);
    return 0;
}

/* Decodes a 'set scissor' command. */
static int decode_sciss(const YF_cmd *cmd)
{
    l_gdec->gdec |= YF_GDEC_SCISS;

    VkRect2D scissor = {
        .offset = {cmd->sciss.rect.origin.x, cmd->sciss.rect.origin.y},
        .extent = {cmd->sciss.rect.size.width, cmd->sciss.rect.size.height}
    };

    vkCmdSetScissor(l_gdec->cmdr->pool_res, cmd->sciss.index, 1, &scissor);
    return 0;
}

/* Decodes a 'set dtable' command. */
static int decode_dtb(int cmdbuf, const YF_cmd *cmd)
{
    switch (cmdbuf) {
    case YF_CMDBUF_GRAPH:
        if (cmd->dtb.index >= yf_getlimits(l_gdec->ctx)->state.dtable_max) {
            yf_seterr(YF_ERR_INVARG, __func__);
            return -1;
        }
        l_gdec->dtb.pending = 1;
        l_gdec->dtb.allocs[cmd->dtb.index] = cmd->dtb.alloc_i;
        if (!l_gdec->dtb.used[cmd->dtb.index]) {
            l_gdec->dtb.used[cmd->dtb.index] = 1;
            l_gdec->dtb.n++;
        }
        break;

    case YF_CMDBUF_COMP:
        if (cmd->dtb.index >= yf_getlimits(l_cdec->ctx)->state.dtable_max) {
            yf_seterr(YF_ERR_INVARG, __func__);
            return -1;
        }
        l_cdec->dtb.pending = 1;
        l_cdec->dtb.allocs[cmd->dtb.index] = cmd->dtb.alloc_i;
        if (!l_cdec->dtb.used[cmd->dtb.index]) {
            l_cdec->dtb.used[cmd->dtb.index] = 1;
            l_cdec->dtb.n++;
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
    if (cmd->vbuf.index >= yf_getlimits(l_gdec->ctx)->state.vinput_max) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }
    l_gdec->gdec |= YF_GDEC_VBUF;

    vkCmdBindVertexBuffers(l_gdec->cmdr->pool_res, cmd->vbuf.index, 1,
                           &cmd->vbuf.buf->buffer, &cmd->vbuf.offset);

    return 0;
}

/* Decodes a 'set index buffer' command. */
static int decode_ibuf(const YF_cmd *cmd)
{
    VkIndexType idx_type;
    switch (cmd->ibuf.stride) {
    case sizeof(unsigned):
        idx_type = VK_INDEX_TYPE_UINT32;
        break;
    case sizeof(unsigned short):
        idx_type = VK_INDEX_TYPE_UINT16;
        break;
    default:
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }
    l_gdec->gdec |= YF_GDEC_IBUF;

    vkCmdBindIndexBuffer(l_gdec->cmdr->pool_res, cmd->ibuf.buf->buffer,
                         cmd->ibuf.offset, idx_type);

    return 0;
}

/* Decodes a 'clear color' command. */
static int decode_clrcol(const YF_cmd *cmd)
{
    if (cmd->clrcol.index >= yf_getlimits(l_gdec->ctx)->pass.color_max) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }
    l_gdec->clr_pending = l_gdec->clrcol.pending = 1;
    l_gdec->clrcol.vals[cmd->clrcol.index] = cmd->clrcol.value;

    if (!l_gdec->clrcol.used[cmd->clrcol.index]) {
        l_gdec->clrcol.used[cmd->clrcol.index] = 1;
        l_gdec->clrcol.n++;
    }
    return 0;
}

/* Decodes a 'clear depth' command. */
static int decode_clrdep(const YF_cmd *cmd)
{
    l_gdec->clr_pending = l_gdec->clrdep.pending = 1;
    l_gdec->clrdep.val = cmd->clrdep.value;
    return 0;
}

/* Decodes a 'clear stencil' command. */
static int decode_clrsten(const YF_cmd *cmd)
{
    l_gdec->clr_pending = l_gdec->clrsten.pending = 1;
    l_gdec->clrsten.val = cmd->clrsten.value;
    return 0;
}

/* Decodes a 'draw' or 'drawi' command. */
static int decode_draw(const YF_cmd *cmd)
{
    if ((l_gdec->gdec & YF_GDEC_PASS) != YF_GDEC_PASS) {
        yf_seterr(YF_ERR_INVCMD, __func__);
        return -1;
    }
    if (l_gdec->tgt->pass != l_gdec->gst->pass) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    /* render pass */
    if (l_gdec->pass != l_gdec->gst->pass) {
        if (l_gdec->pass != NULL)
            vkCmdEndRenderPass(l_gdec->cmdr->pool_res);
        l_gdec->pass = l_gdec->gst->pass;

        VkRenderPassBeginInfo info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = NULL,
            .renderPass = l_gdec->pass->ren_pass,
            .framebuffer = l_gdec->tgt->framebuf,
            .renderArea = {
                {0, 0},
                {l_gdec->tgt->dim.width, l_gdec->tgt->dim.height}
            },
            .clearValueCount = 0,
            .pClearValues = NULL
        };

        vkCmdBeginRenderPass(l_gdec->cmdr->pool_res, &info,
                             VK_SUBPASS_CONTENTS_INLINE);
    }

    /* dtables */
    if (l_gdec->dtb.pending) {
        for (unsigned i = 0, n = 0; ; i++) {
            if (!l_gdec->dtb.used[i])
                continue;

            if (i >= l_gdec->gst->dtb_n ||
                l_gdec->dtb.allocs[i] >= l_gdec->gst->dtbs[i]->set_n) {
                yf_seterr(YF_ERR_INVARG, __func__);
                return -1;
            }

            VkDescriptorSet *ds =
                &l_gdec->gst->dtbs[i]->sets[l_gdec->dtb.allocs[i]];

            vkCmdBindDescriptorSets(l_gdec->cmdr->pool_res,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    l_gdec->gst->layout, i, 1, ds, 0, NULL);

            if (++n == l_gdec->dtb.n)
                break;
        }
        l_gdec->dtb.pending = 0;
    }

    /* clear requests */
    if (l_gdec->clr_pending) {
        VkClearRect clr_rect = {
            .rect = {{0, 0}, {l_gdec->tgt->dim.width, l_gdec->tgt->dim.height}},
            .baseArrayLayer = 0,
            .layerCount = l_gdec->tgt->layers
        };
        VkClearAttachment *clr_atts = NULL;
        unsigned clr_i = 0;

        if (l_gdec->clrcol.pending) {
            clr_atts = malloc(sizeof *clr_atts * (l_gdec->clrcol.n+1));
            if (clr_atts == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                return -1;
            }

            for (unsigned i = 0; ; i++) {
                if (l_gdec->clrcol.used[i]) {
                    clr_atts[clr_i].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    clr_atts[clr_i].colorAttachment = i;
                    YF_color *vals = l_gdec->clrcol.vals;
                    clr_atts[clr_i].clearValue.color.float32[0] = vals[i].r;
                    clr_atts[clr_i].clearValue.color.float32[1] = vals[i].g;
                    clr_atts[clr_i].clearValue.color.float32[2] = vals[i].b;
                    clr_atts[clr_i].clearValue.color.float32[3] = vals[i].a;
                    l_gdec->clrcol.used[i] = 0;

                    if (++clr_i == l_gdec->clrcol.n)
                        break;
                }
            }

            l_gdec->clrcol.n = 0;
            l_gdec->clrcol.pending = 0;

        } else {
            clr_atts = malloc(sizeof *clr_atts);
            if (clr_atts == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                return -1;
            }
        }

        clr_atts[clr_i].aspectMask = 0;
        if (l_gdec->clrdep.pending) {
            clr_atts[clr_i].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            clr_atts[clr_i].clearValue.depthStencil.depth = l_gdec->clrdep.val;
            l_gdec->clrdep.pending = 0;
            /* XXX */
            assert(l_gdec->clrdep.val >= 0.0f && l_gdec->clrdep.val <= 1.0f);
        }
        if (l_gdec->clrsten.pending) {
            clr_atts[clr_i].aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            clr_atts[clr_i].clearValue.depthStencil.stencil =
                l_gdec->clrsten.val;
            l_gdec->clrsten.pending = 0;
        }

        vkCmdClearAttachments(l_gdec->cmdr->pool_res,
                              clr_i + (clr_atts[clr_i].aspectMask != 0),
                              clr_atts, 1, &clr_rect);

        free(clr_atts);
        l_gdec->clr_pending = 0;
    }

    /* draw */
    int r;
    if (cmd->cmd == YF_CMD_DRAWI) {
        if ((l_gdec->gdec & YF_GDEC_DRAWI) == YF_GDEC_DRAWI) {
            vkCmdDrawIndexed(l_gdec->cmdr->pool_res, cmd->drawi.vert_n,
                             cmd->drawi.inst_n, cmd->drawi.index_base,
                             cmd->drawi.vert_off, cmd->drawi.inst_id);
            r = 0;
        } else {
            yf_seterr(YF_ERR_INVCMD, __func__);
            r = -1;
        }
    } else {
        if ((l_gdec->gdec & YF_GDEC_DRAW) == YF_GDEC_DRAW) {
            vkCmdDraw(l_gdec->cmdr->pool_res, cmd->draw.vert_n,
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

    const YF_limits *lim = yf_getlimits(l_cdec->ctx);
    if (cmd->disp.dim.width > lim->cmdbuf.disp_dim_max.width ||
        cmd->disp.dim.height > lim->cmdbuf.disp_dim_max.height ||
        cmd->disp.dim.depth > lim->cmdbuf.disp_dim_max.depth) {
        yf_seterr(YF_ERR_LIMIT, __func__);
        return -1;
    }

    /* dtables */
    if (l_cdec->dtb.pending) {
        if (l_cdec->cst == NULL) {
            yf_seterr(YF_ERR_INVCMD, __func__);
            return -1;
        }

        for (unsigned i = 0, n = 0; ; i++) {
            if (!l_cdec->dtb.used[i])
                continue;

            if (i >= l_cdec->cst->dtb_n ||
                l_cdec->dtb.allocs[i] >= l_cdec->cst->dtbs[i]->set_n) {
                yf_seterr(YF_ERR_INVARG, __func__);
                return -1;
            }

            VkDescriptorSet *ds =
                &l_cdec->cst->dtbs[i]->sets[l_cdec->dtb.allocs[i]];

            vkCmdBindDescriptorSets(l_cdec->cmdr->pool_res,
                                    VK_PIPELINE_BIND_POINT_COMPUTE,
                                    l_cdec->cst->layout, i, 1, ds, 0, NULL);

            if (++n == l_cdec->dtb.n)
                break;
        }
        l_cdec->dtb.pending = 0;
    }

    /* dispatch */
    int r;
    if ((l_cdec->cdec & YF_CDEC_DISP) == YF_CDEC_DISP) {
        vkCmdDispatch(l_cdec->cmdr->pool_res, cmd->disp.dim.width,
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
    assert(cmd->cpybuf.dst->size >= cmd->cpybuf.dst_offs + cmd->cpybuf.size);
    assert(cmd->cpybuf.src->size >= cmd->cpybuf.src_offs + cmd->cpybuf.size);
    assert(cmd->cpybuf.size > 0);

    VkBufferCopy region = {
        .srcOffset = cmd->cpybuf.src_offs,
        .dstOffset = cmd->cpybuf.dst_offs,
        .size = cmd->cpybuf.size
    };

    vkCmdCopyBuffer(l_xdec->cmdr->pool_res, cmd->cpybuf.src->buffer,
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

    vkCmdCopyImage(l_xdec->cmdr->pool_res,
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
        if (l_gdec->pass != NULL) {
            vkCmdEndRenderPass(l_gdec->cmdr->pool_res);
            l_gdec->pass = NULL;
        }
        cmdr = l_gdec->cmdr;
        break;
    case YF_CMDBUF_COMP:
        cmdr = l_cdec->cmdr;
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
    l_gdec = calloc(1, sizeof *l_gdec);
    if (l_gdec == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }
    l_gdec->ctx = cmdb->ctx;
    l_gdec->cmdr = cmdr;

    const unsigned dtb_max = yf_getlimits(cmdb->ctx)->state.dtable_max;
    l_gdec->dtb.allocs = calloc(dtb_max, sizeof *l_gdec->dtb.allocs);
    l_gdec->dtb.used = calloc(dtb_max, sizeof *l_gdec->dtb.used);

    const unsigned col_max = yf_getlimits(cmdb->ctx)->pass.color_max;
    l_gdec->clrcol.vals = calloc(col_max, sizeof *l_gdec->clrcol.vals);
    l_gdec->clrcol.used = calloc(col_max, sizeof *l_gdec->clrcol.used);

    if (l_gdec->dtb.allocs == NULL || l_gdec->dtb.used == NULL ||
        l_gdec->clrcol.vals == NULL || l_gdec->clrcol.used == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        free(l_gdec->dtb.allocs);
        free(l_gdec->dtb.used);
        free(l_gdec->clrcol.vals);
        free(l_gdec->clrcol.used);
        free(l_gdec);
        l_gdec = NULL;
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

    if (l_gdec->pass != NULL)
        vkCmdEndRenderPass(cmdr->pool_res);

    /* XXX: Clear commands are deferred until a draw is issued. When a clear
       request comes last, it is handled here. */
    if (r == 0 && l_gdec->clr_pending) {
        if (l_gdec->tgt == NULL) {
            yf_seterr(YF_ERR_INVCMD, __func__);
            r = -1;

        } else {
            VkImageSubresourceRange range = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = l_gdec->tgt->layers
            };

            if (l_gdec->clrcol.pending) {
                VkClearColorValue col;
                for (unsigned i = 0, n = 0; ; i++) {
                    if (l_gdec->clrcol.used[i]) {
                        assert(l_gdec->tgt->pass->color_n > i);

                        col.float32[0] = l_gdec->clrcol.vals[i].r;
                        col.float32[1] = l_gdec->clrcol.vals[i].g;
                        col.float32[2] = l_gdec->clrcol.vals[i].b;
                        col.float32[3] = l_gdec->clrcol.vals[i].a;
                        range.baseArrayLayer = l_gdec->tgt->lays_base[i];

                        vkCmdClearColorImage(cmdr->pool_res,
                                             l_gdec->tgt->imgs[i]->image,
                                             VK_IMAGE_LAYOUT_GENERAL, &col,
                                             1, &range);

                        if (++n == l_gdec->clrcol.n)
                            break;
                    }
                }
            }

            if (l_gdec->clrdep.pending || l_gdec->clrsten.pending) {
                assert(l_gdec->tgt->pass->depth_n != 0);

                unsigned index = l_gdec->tgt->iview_n-1;
                VkClearDepthStencilValue ds;
                range.baseArrayLayer = l_gdec->tgt->lays_base[index];
                range.layerCount = l_gdec->tgt->layers;
                range.aspectMask = 0;

                if (l_gdec->clrdep.pending) {
                    range.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
                    ds.depth = l_gdec->clrdep.val;
                }

                if (l_gdec->clrsten.pending) {
                    range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                    ds.stencil = l_gdec->clrsten.val;
                }

                vkCmdClearDepthStencilImage(cmdr->pool_res,
                                            l_gdec->tgt->imgs[index]->image,
                                            VK_IMAGE_LAYOUT_GENERAL, &ds,
                                            1, &range);
            }
        }
    }

    free(l_gdec->dtb.allocs);
    free(l_gdec->dtb.used);
    free(l_gdec->clrcol.vals);
    free(l_gdec->clrcol.used);
    free(l_gdec);
    l_gdec = NULL;
    return r;
}

/* Decodes a compute command buffer. */
static int decode_comp(YF_cmdbuf cmdb, const YF_cmdres *cmdr)
{
    l_cdec = calloc(1, sizeof *l_cdec);
    if (l_cdec == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }
    l_cdec->ctx = cmdb->ctx;
    l_cdec->cmdr = cmdr;

    const unsigned dtb_max = yf_getlimits(cmdb->ctx)->state.dtable_max;
    l_cdec->dtb.allocs = calloc(dtb_max, sizeof *l_cdec->dtb.allocs);
    l_cdec->dtb.used = calloc(dtb_max, sizeof *l_cdec->dtb.used);

    if (l_cdec->dtb.allocs == NULL || l_cdec->dtb.used == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        free(l_cdec->dtb.allocs);
        free(l_cdec->dtb.used);
        free(l_cdec);
        l_cdec = NULL;
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

    free(l_cdec->dtb.allocs);
    free(l_cdec->dtb.used);
    free(l_cdec);
    l_cdec = NULL;
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
        if (l_gdec != NULL) {
            yf_seterr(YF_ERR_INUSE, __func__);
            r = -1;
            break;
        }
        r = decode_graph(cmdb, &cmdr);
        break;
    case YF_CMDBUF_COMP:
        if (l_cdec != NULL) {
            yf_seterr(YF_ERR_INUSE, __func__);
            r = -1;
            break;
        }
        r = decode_comp(cmdb, &cmdr);
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
