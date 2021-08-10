/*
 * YF
 * yf-stage.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
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
typedef unsigned long YF_shdid;

/**
 * Type defining a single shader stage.
 */
typedef struct {
    int stage;
    YF_shdid shd;
    const char entry_point[128];
} YF_stage;

/**
 * Loads a shader module.
 *
 * @param ctx: The context.
 * @param pathname: The pathname of the shader code file.
 * @param mod: The destination for the module identifier.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_loadmod(YF_context ctx, const char *pathname, YF_modid *mod);

/**
 * Unloads a shader module.
 *
 * @param ctx: The context that owns the module to unload.
 * @param mod: The identifier of the module to unload.
 */
void yf_unldmod(YF_context ctx, YF_modid mod);

YF_DECLS_END

#endif /* YF_YF_STAGE_H */
