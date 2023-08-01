/*
 * YF
 * cmdbuf.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_CMDBUF_H
#define YF_CMDBUF_H

#include "yf-cmdbuf.h"
#include "cmd.h"

struct yf_cmdbuf {
    yf_context_t *ctx;
    int cmdbuf;
    yf_cmd_t *cmds;
    unsigned cmd_n;
    unsigned cmd_cap;
    int invalid;
};

/* Decodes a command buffer and enqueues the resulting object for execution.
   Unlike encoding, decoding is platform-dependent and defined elsewhere. */
int yf_cmdbuf_decode(yf_cmdbuf_t *cmdb);

#endif /* YF_CMDBUF_H */
