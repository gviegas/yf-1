/*
 * YF
 * mesh.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_MESH_H
#define YF_MESH_H

#include <yf/core/yf-cmdbuf.h>

#include "yf-mesh.h"

/* Type defining the mesh data. */
typedef struct {
  struct {
    int vtype;
    void *data;
    size_t n;
  } v;
  struct {
    void *data;
    short stride;
    size_t n;
  } i;
} YF_meshdt;

/* Initializes a new mesh object from mesh data directly. */
YF_mesh yf_mesh_initdt(const YF_meshdt *data);

/* Replaces the contents of a mesh object's vertices. */
int yf_mesh_setvtx(YF_mesh mesh, YF_slice range, const void *data);

/* Encodes vertex/index state and draws a given mesh. */
void yf_mesh_draw(YF_mesh mesh, YF_cmdbuf cmdb, unsigned inst_n, int inst_id);

#ifdef YF_DEBUG
# define YF_MESHDT_PRINT(dt_p) do { \
   printf("\n-- Meshdt (debug) --"); \
   printf("\nv.n: %lu", (dt_p)->v.n); \
   if ((dt_p)->v.n < 192) { \
    switch ((dt_p)->v.vtype) { \
     case YF_VTYPE_MDL: \
      for (size_t i = 0; i < (dt_p)->v.n; ++i) \
       YF_VMDL_PRINT(((YF_vmdl *)(dt_p)->v.data)[i]); \
      break; \
     case YF_VTYPE_TERR: \
      for (size_t i = 0; i < (dt_p)->v.n; ++i) \
       YF_VTERR_PRINT(((YF_vterr *)(dt_p)->v.data)[i]); \
      break; \
     case YF_VTYPE_PART: \
      for (size_t i = 0; i < (dt_p)->v.n; ++i) \
       YF_VPART_PRINT(((YF_vpart *)(dt_p)->v.data)[i]); \
      break; \
     case YF_VTYPE_QUAD: \
      for (size_t i = 0; i < (dt_p)->v.n; ++i) \
       YF_VQUAD_PRINT(((YF_vquad *)(dt_p)->v.data)[i]); \
      break; \
     case YF_VTYPE_LABL: \
      for (size_t i = 0; i < (dt_p)->v.n; ++i) \
       YF_VLABL_PRINT(((YF_vlabl *)(dt_p)->v.data)[i]); \
      break; \
     default: assert(0); \
    } \
   } \
   printf("\ni.stride: %d", (dt_p)->i.stride); \
   printf("\ni.n: %lu", (dt_p)->i.n); \
   if ((dt_p)->i.n < 256) { \
    for (size_t i = 0; i < (dt_p)->i.n; ++i) { \
     if (i % 3 == 0) \
      printf("\n"); \
     if ((size_t)(dt_p)->i.stride < sizeof(unsigned)) \
      printf(" %d", ((unsigned short *)(dt_p)->i.data)[i]); \
     else \
      printf(" %u", ((unsigned *)(dt_p)->i.data)[i]); \
    } \
   } \
   printf("\n--\n"); } while (0)
#endif

#endif /* YF_MESH_H */
