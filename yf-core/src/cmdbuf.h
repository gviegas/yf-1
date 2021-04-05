/*
 * YF
 * cmdbuf.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_CMDBUF_H
#define YF_CMDBUF_H

#include "yf-cmdbuf.h"
#include "cmd.h"

typedef struct YF_cmdbuf_o {
  YF_context ctx;
  int cmdbuf;
  YF_cmd *cmds;
  unsigned cmd_n;
  unsigned cmd_cap;
  int invalid;
} YF_cmdbuf_o;

/* Decodes a command buffer and enqueues the resulting object for execution.
   Unlike encoding, decoding is platform-dependent and defined elsewhere. */
int yf_cmdbuf_decode(YF_cmdbuf cmdb);

#endif /* YF_CMDBUF_H */
