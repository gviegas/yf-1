/*
 * YF
 * vertex.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_VERTEX_H
#define YF_VERTEX_H

#include "yf-vector.h"

/* Type defining the mesh vertex for model objects. */
typedef struct {
  YF_vec3 pos;
  YF_vec2 tc;
  YF_vec3 norm;
  /* TODO */
} YF_vmdl;
#define YF_VTYPE_MDL 0

/* Type defining the mesh vertex for terrain objects. */
typedef struct {
  YF_vec3 pos;
  YF_vec2 tc;
  YF_vec3 norm;
  /* TODO */
} YF_vterr;
#define YF_VTYPE_TERR 1

/* Type defining the mesh vertex for particle system objects. */
typedef struct {
  YF_vec3 pos;
  YF_vec4 col;
  /* TODO */
} YF_vpart;
#define YF_VTYPE_PART 2

/* Type defining the mesh vertex for quad objects. */
typedef struct {
  YF_vec3 pos;
  YF_vec2 tc;
  YF_vec4 col;
  /* TODO */
} YF_vquad;
#define YF_VTYPE_QUAD 3

#ifdef YF_DEBUG
# define YF_VMDL_PRINT(vtx) do { \
   printf("\n-- Vertex (debug) --"); \
   printf("\npos:  [%.3f %.3f %.3f]", \
    (vtx).pos[0], (vtx).pos[1], (vtx).pos[2]); \
   printf("\ntc:   [%.3f %.3f]", (vtx).tc[0], (vtx).tc[1]); \
   printf("\nnorm: [%.3f %.3f %.3f]", \
    (vtx).norm[0], (vtx).norm[1], (vtx).norm[2]); \
   printf("\n--\n"); } while (0)

# define YF_VTERR_PRINT(vtx) do { \
   printf("\n-- Vertex (debug) --"); \
   printf("\npos:  [%.3f %.3f %.3f]", \
    (vtx).pos[0], (vtx).pos[1], (vtx).pos[2]); \
   printf("\ntc:   [%.3f %.3f]", (vtx).tc[0], (vtx).tc[1]); \
   printf("\nnorm: [%.3f %.3f %.3f]", \
    (vtx).norm[0], (vtx).norm[1], (vtx).norm[2]); \
   printf("\n--\n"); } while (0)

# define YF_VPART_PRINT(vtx) do { \
   printf("\n-- Vertex (debug) --"); \
   printf("\npos:  [%.3f %.3f %.3f]", \
    (vtx).pos[0], (vtx).pos[1], (vtx).pos[2]); \
   printf("\ncol: [%.3f %.3f %.3f %.3f]", \
    (vtx).col[0], (vtx).col[1], (vtx).col[2], (vtx).col[3]); \
   printf("\n--\n"); } while (0)

# define YF_VQUAD_PRINT(vtx) do { \
   printf("\n-- Vertex (debug) --"); \
   printf("\npos: [%.3f %.3f %.3f]", \
    (vtx).pos[0], (vtx).pos[1], (vtx).pos[2]); \
   printf("\ntc: [%.3f %.3f]", (vtx).tc[0], (vtx).tc[1]); \
   printf("\ncol: [%.3f %.3f %.3f %.3f]", \
    (vtx).col[0], (vtx).col[1], (vtx).col[2], (vtx).col[3]); \
   printf("\n--\n"); } while (0)
#endif

#endif /* YF_VERTEX_H */
