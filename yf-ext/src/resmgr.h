/*
 * YF
 * resmgr.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_RESMGR_H
#define YF_RESMGR_H

#include <yf/core/yf-gstate.h>

/* Resource requirements for node objects. */
#define YF_RESRQ_MDL   0
#define YF_RESRQ_MDL4  1
#define YF_RESRQ_MDL16 2
#define YF_RESRQ_MDL64 3
#define YF_RESRQ_TERR  4
#define YF_RESRQ_PART  5

#define YF_RESRQ_N 6

/* Obtains a resource that satisfies the given requirements. */
YF_gstate yf_resmgr_obtain(int resrq, unsigned *inst_alloc);

/* Yields a previously obtained resource. */
void yf_resmgr_yield(int resrq, unsigned inst_alloc);

/* Gets the global dtable. */
YF_dtable yf_resmgr_getglob(void);

/* Gets the number of instance allocations for a given 'resrq' value. */
unsigned yf_resmgr_getallocn(int resrq);

/* Sets the number of instance allocations for a given 'resrq' value. */
int yf_resmgr_setallocn(int resrq, unsigned n);

/* Pre-allocates resources for a given 'resrq' value. */
int yf_resmgr_prealloc(int resrq);

/* Deallocates resources for a given 'resrq' value. */
void yf_resmgr_dealloc(int resrq);

/* Deallocates all resources. */
void yf_resmgr_clear(void);

/*
 * Conventions
 */

/* Index numbers used for dtable resources. */
#define YF_RESIDX_GLOB 0
#define YF_RESIDX_INST 1

/* Binding numbers used for dtable resources. */
#define YF_RESBIND_GLOB 0
#define YF_RESBIND_INST 0
#define YF_RESBIND_TEX  1
#define YF_RESBIND_HMAP 3

/* Location numbers used for vinput resources. */
#define YF_RESLOC_POS  0
#define YF_RESLOC_TC   1
#define YF_RESLOC_NORM 2
#define YF_RESLOC_CLR  3

#endif /* YF_RESMGR_H */
