/*
 * YF
 * resmgr.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_RESMGR_H
#define YF_RESMGR_H

#include "yf/core/yf-gstate.h"

/* Resource requirements for node objects. */
#define YF_RESRQ_MDL   0
#define YF_RESRQ_MDL2  1
#define YF_RESRQ_MDL4  2
#define YF_RESRQ_MDL8  3
#define YF_RESRQ_MDL16 4
#define YF_RESRQ_MDL32 5
#define YF_RESRQ_MDL64 6
#define YF_RESRQ_TERR  7
#define YF_RESRQ_PART  8
#define YF_RESRQ_QUAD  9
#define YF_RESRQ_LABL  10

#define YF_RESRQ_N 11

/* Obtains a resource that satisfies the given requirements. */
YF_gstate yf_resmgr_obtain(int resrq, unsigned *inst_alloc);

/* Yields a previously obtained resource. */
void yf_resmgr_yield(int resrq, unsigned inst_alloc);

/* Gets the global descriptor table. */
YF_dtable yf_resmgr_getglobl(void);

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

/* Descriptor table indices. */
#define YF_RESIDX_GLOBL 0
#define YF_RESIDX_INST  1

/* Descriptor table bindings. */
#define YF_RESBIND_GLOBL 0
#define YF_RESBIND_INST  0
#define YF_RESBIND_LIGHT 1
#define YF_RESBIND_MATL  1
#define YF_RESBIND_TEX   1
#define YF_RESBIND_HMAP  2
#define YF_RESBIND_CLR   2
#define YF_RESBIND_PBR   3
#define YF_RESBIND_NORM  4
#define YF_RESBIND_OCC   5
#define YF_RESBIND_EMIS  6

/* Vertex input locations. */
#define YF_RESLOC_POS   0
#define YF_RESLOC_TC    1
#define YF_RESLOC_NORM  2
#define YF_RESLOC_TGNT  3
#define YF_RESLOC_CLR   4
#define YF_RESLOC_JNTS  5
#define YF_RESLOC_WGTS  6
#define YF_RESLOC_TC1   7
#define YF_RESLOC_TC2   8
#define YF_RESLOC_CLR1  9
#define YF_RESLOC_CLR2  10
#define YF_RESLOC_JNTS1 11
#define YF_RESLOC_WGTS1 12

#endif /* YF_RESMGR_H */
