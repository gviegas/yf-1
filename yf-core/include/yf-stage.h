/*
 * YF
 * yf-stage.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_STAGE_H
#define YF_YF_STAGE_H

#include <yf/com/yf-defs.h>

#include "yf-context.h"

YF_DECLS_BEGIN

/* Programmable pipeline stages. */
#define YF_STAGE_VERT 0x01
#define YF_STAGE_TESC 0x02
#define YF_STAGE_TESE 0x04
#define YF_STAGE_GEOM 0x08
#define YF_STAGE_FRAG 0x10
#define YF_STAGE_COMP 0x20

/* Type defining the identifier of a loaded module. */
typedef unsigned YF_modid;

/* Type defining a single shader stage. */
typedef struct {
  int stage;
  YF_modid mod;
  const char entry_point[48];
} YF_stage;

/* Loads a shader module. */
int yf_loadmod(YF_context ctx, const char *pathname, YF_modid *mod);

/* Unloads a shader module. */
void yf_unldmod(YF_context ctx, YF_modid mod);

YF_DECLS_END

#endif /* YF_YF_STAGE_H */
