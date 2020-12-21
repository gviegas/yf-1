/*
 * YF
 * cmd.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_CMD_H
#define YF_CMD_H

/* The parameters of a 'set gstate' command. */
typedef struct {
  YF_gstate gst;
} YF_cmd_gst;

/* The parameters of a 'set cstate' command. */
typedef struct {
  YF_cstate cst;
} YF_cmd_cst;

/* The parameters of a 'set target' command. */
typedef struct {
  YF_target tgt;
} YF_cmd_tgt;

/* The parameters of a 'set viewport' command. */
typedef struct {
  unsigned index;
  YF_viewport vport;
} YF_cmd_vport;

/* The parameters of a 'set scissor' command. */
typedef struct {
  unsigned index;
  YF_rect rect;
} YF_cmd_sciss;

/* The parameters of a 'set dtable' command. */
typedef struct {
  unsigned index;
  unsigned alloc_i;
} YF_cmd_dtb;

/* The parameters of a 'set vertex buffer' command. */
typedef struct {
  unsigned index;
  YF_buffer buf;
  size_t offset;
} YF_cmd_vbuf;

/* The parameters of a 'set index buffer' command. */
typedef struct {
  YF_buffer buf;
  size_t offset;
  unsigned stride;
} YF_cmd_ibuf;

/* The parameters of a 'clear color' command. */
typedef struct {
  unsigned index;
  YF_color value;
} YF_cmd_clrcol;

/* The parameters of a 'clear depth' command. */
typedef struct {
  float value;
} YF_cmd_clrdep;

/* The parameters of a 'clear stencil' command. */
typedef struct {
  unsigned value;
} YF_cmd_clrsten;

/* The parameters of a 'draw' command. */
typedef struct {
  int indexed;
  unsigned index_base;
  unsigned vert_n;
  unsigned inst_n;
  int vert_id;
  int inst_id;
} YF_cmd_draw;

/* The parameters of a 'dispatch' command. */
typedef struct {
  YF_dim3 dim;
} YF_cmd_disp;

/* The parameters of a 'copy buffer' command. */
typedef struct {
  YF_buffer dst;
  size_t dst_offs;
  YF_buffer src;
  size_t src_offs;
  size_t size;
} YF_cmd_cpybuf;

/* The parameters of a 'copy image' command. */
typedef struct {
  YF_image dst;
  unsigned dst_layer;
  YF_image src;
  unsigned src_layer;
  unsigned layer_n;
} YF_cmd_cpyimg;

/* Type defining a single command. */
typedef struct {
  int cmd;
  union {
    YF_cmd_gst gst;
    YF_cmd_cst cst;
    YF_cmd_tgt tgt;
    YF_cmd_vport vport;
    YF_cmd_sciss sciss;
    YF_cmd_dtb dtb;
    YF_cmd_vbuf vbuf;
    YF_cmd_ibuf ibuf;
    YF_cmd_clrcol clrcol;
    YF_cmd_clrdep clrdep;
    YF_cmd_clrsten clrsten;
    YF_cmd_draw draw;
    YF_cmd_disp disp;
    YF_cmd_cpybuf cpybuf;
    YF_cmd_cpyimg cpyimg;
  };
} YF_cmd;

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
#define YF_CMD_DISP    12
#define YF_CMD_CPYBUF  13
#define YF_CMD_CPYIMG  14
#define YF_CMD_SYNC    15

#endif /* YF_CMD_H */
