/*
 * YF
 * yf-stage.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_STAGE_H
#define YF_YF_STAGE_H

#include "yf/com/yf-defs.h"

#include "yf-context.h"

YF_DECLS_BEGIN

/**
 * Programmable pipeline stages.
 */
#define YF_STAGE_VERT 0x01
#define YF_STAGE_TESC 0x02
#define YF_STAGE_TESE 0x04
#define YF_STAGE_GEOM 0x08
#define YF_STAGE_FRAG 0x10
#define YF_STAGE_COMP 0x20

/**
 * Type defining the identifier of a loaded shader.
 *
 * Shaders are context-managed objects representing executable code for a
 * programmable pipeline stage.
 */
typedef unsigned long yf_shdid_t;

/**
 * Type defining a single shader stage.
 */
typedef struct yf_stage {
    int stage;
    yf_shdid_t shd;
    char entry_point[64];
} yf_stage_t;

/**
 * Loads a shader.
 *
 * @param ctx: The context.
 * @param pathname: The pathname of the shader code file.
 * @param shd: The destination for the shader identifier.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_loadshd(yf_context_t *ctx, const char *pathname, yf_shdid_t *shd);

/**
 * Unloads a shader.
 *
 * @param ctx: The context that owns the shader to unload.
 * @param shd: The identifier of the shader to unload.
 */
void yf_unldshd(yf_context_t *ctx, yf_shdid_t shd);

YF_DECLS_END

#endif /* YF_YF_STAGE_H */
