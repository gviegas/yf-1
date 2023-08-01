/*
 * YF
 * cmd.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_CMD_H
#define YF_CMD_H

/* The parameters of a 'set gstate' command. */
typedef struct yf_cmd_gst {
    yf_gstate_t *gst;
} yf_cmd_gst_t;

/* The parameters of a 'set cstate' command. */
typedef struct yf_cmd_cst {
    yf_cstate_t *cst;
} yf_cmd_cst_t;

/* The parameters of a 'set target' command. */
typedef struct yf_cmd_tgt {
    yf_target_t *tgt;
} yf_cmd_tgt_t;

/* The parameters of a 'set viewport' command. */
typedef struct yf_cmd_vport {
    unsigned index;
    yf_viewport_t vport;
} yf_cmd_vport_t;

/* The parameters of a 'set scissor' command. */
typedef struct yf_cmd_sciss {
    unsigned index;
    yf_rect_t rect;
} yf_cmd_sciss_t;

/* The parameters of a 'set dtable' command. */
typedef struct yf_cmd_dtb {
    unsigned index;
    unsigned alloc_i;
} yf_cmd_dtb_t;

/* The parameters of a 'set vertex buffer' command. */
typedef struct yf_cmd_vbuf {
    unsigned index;
    yf_buffer_t *buf;
    size_t offset;
} yf_cmd_vbuf_t;

/* The parameters of a 'set index buffer' command. */
typedef struct yf_cmd_ibuf {
    yf_buffer_t *buf;
    size_t offset;
    int itype;
} yf_cmd_ibuf_t;

/* The parameters of a 'clear color' command. */
typedef struct yf_cmd_clrcol {
    unsigned index;
    yf_color_t value;
} yf_cmd_clrcol_t;

/* The parameters of a 'clear depth' command. */
typedef struct yf_cmd_clrdep {
    float value;
} yf_cmd_clrdep_t;

/* The parameters of a 'clear stencil' command. */
typedef struct yf_cmd_clrsten {
    unsigned value;
} yf_cmd_clrsten_t;

/* The parameters of a 'draw' command. */
typedef struct yf_cmd_draw {
    unsigned vert_id;
    unsigned vert_n;
    unsigned inst_id;
    unsigned inst_n;
} yf_cmd_draw_t;

/* The parameters of a 'draw indexed' command. */
typedef struct yf_cmd_drawi {
    unsigned index_base;
    int vert_off;
    unsigned vert_n;
    unsigned inst_id;
    unsigned inst_n;
} yf_cmd_drawi_t;

/* The parameters of a 'dispatch' command. */
typedef struct yf_cmd_disp {
    yf_dim3_t dim;
} yf_cmd_disp_t;

/* The parameters of a 'copy buffer' command. */
typedef struct yf_cmd_cpybuf {
    yf_buffer_t *dst;
    size_t dst_off;
    yf_buffer_t *src;
    size_t src_off;
    size_t size;
} yf_cmd_cpybuf_t;

/* The parameters of a 'copy image' command. */
typedef struct yf_cmd_cpyimg {
    yf_image_t *dst;
    yf_off3_t dst_off;
    unsigned dst_layer;
    unsigned dst_level;
    yf_image_t *src;
    yf_off3_t src_off;
    unsigned src_layer;
    unsigned src_level;
    yf_dim3_t dim;
    unsigned layer_n;
} yf_cmd_cpyimg_t;

/* Command types. */
#define YF_CMD_GST     0
#define YF_CMD_CST     1
#define YF_CMD_TGT     2
#define YF_CMD_VPORT   3
#define YF_CMD_SCISS   4
#define YF_CMD_DTB     5
#define YF_CMD_VBUF    6
#define YF_CMD_IBUF    7
#define YF_CMD_CLRCOL  8
#define YF_CMD_CLRDEP  9
#define YF_CMD_CLRSTEN 10
#define YF_CMD_DRAW    11
#define YF_CMD_DRAWI   12
#define YF_CMD_DISP    13
#define YF_CMD_CPYBUF  14
#define YF_CMD_CPYIMG  15
#define YF_CMD_SYNC    16

/* Command of a given type. */
typedef struct yf_cmd {
    int cmd;
    union {
        yf_cmd_gst_t gst;
        yf_cmd_cst_t cst;
        yf_cmd_tgt_t tgt;
        yf_cmd_vport_t vport;
        yf_cmd_sciss_t sciss;
        yf_cmd_dtb_t dtb;
        yf_cmd_vbuf_t vbuf;
        yf_cmd_ibuf_t ibuf;
        yf_cmd_clrcol_t clrcol;
        yf_cmd_clrdep_t clrdep;
        yf_cmd_clrsten_t clrsten;
        yf_cmd_draw_t draw;
        yf_cmd_drawi_t drawi;
        yf_cmd_disp_t disp;
        yf_cmd_cpybuf_t cpybuf;
        yf_cmd_cpyimg_t cpyimg;
    };
} yf_cmd_t;

#endif /* YF_CMD_H */
