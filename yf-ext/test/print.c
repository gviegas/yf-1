/*
 * YF
 * print.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "print.h"
#include "model.h"
#include "vertex.h"

#undef YF_PTITLE
#define YF_PTITLE printf("\n[YF] OUTPUT (%s):", __func__);

void yf_print_nodeobj(YF_node node) {
  YF_PTITLE;

  void *obj;
  switch (yf_node_getobj(node, &obj)) {
    case YF_NODEOBJ_NONE:
      printf("\nno object for this node");
      break;
    case YF_NODEOBJ_MODEL:
      printf("\nnodeobj is a model (%p)", obj);
      printf("\n mesh: %p", (void *)yf_model_getmesh((YF_model)obj));
      printf("\n tex: %p", (void *)yf_model_gettex((YF_model)node));
      break;
    case YF_NODEOBJ_TERRAIN:
      /* TODO */
      assert(0);
      break;
    case YF_NODEOBJ_PARTICLE:
      /* TODO */
      assert(0);
      break;
    case YF_NODEOBJ_QUAD:
      /* TODO */
      assert(0);
      break;
    case YF_NODEOBJ_LABEL:
      /* TODO */
      assert(0);
      break;
    case YF_NODEOBJ_LIGHT:
      /* TODO */
      assert(0);
      break;
    case YF_NODEOBJ_EFFECT:
      /* TODO */
      assert(0);
      break;
    default:
      assert(0);
  }

  printf("\n\n");
}

void yf_print_meshdt(const YF_meshdt *data) {
  YF_PTITLE;

  printf("\nv.n: %lu", data->v.n);
  switch (data->v.vtype) {
    case YF_VTYPE_MDL:
      for (size_t i = 0; i < data->v.n; ++i)
        YF_VMDL_PRINT(((YF_vmdl *)data->v.data)[i]);
      break;
    case YF_VTYPE_TERR:
      for (size_t i = 0; i < data->v.n; ++i)
        YF_VTERR_PRINT(((YF_vterr *)data->v.data)[i]);
      break;
    case YF_VTYPE_PART:
      for (size_t i = 0; i < data->v.n; ++i)
        YF_VPART_PRINT(((YF_vpart *)data->v.data)[i]);
      break;
    case YF_VTYPE_QUAD:
      for (size_t i = 0; i < data->v.n; ++i)
        YF_VQUAD_PRINT(((YF_vquad *)data->v.data)[i]);
      break;
    case YF_VTYPE_LABL:
      for (size_t i = 0; i < data->v.n; ++i)
        YF_VLABL_PRINT(((YF_vlabl *)data->v.data)[i]);
      break;
    default:
      assert(0);
    }

  printf("\ni.stride: %d", data->i.stride);
  printf("\ni.n: %lu", data->i.n);
  for (size_t i = 0; i < data->i.n; ++i) {
    if (i % 3 == 0)
      printf("\n");
    if ((size_t)data->i.stride < sizeof(unsigned))
      printf(" %d", ((unsigned short *)data->i.data)[i]);
    else
      printf(" %u", ((unsigned *)data->i.data)[i]);
  }

  printf("\n\n");
}
