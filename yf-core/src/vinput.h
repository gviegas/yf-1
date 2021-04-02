/*
 * YF
 * vinput.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_VINPUT_H
#define YF_VINPUT_H

#include "yf-vinput.h"

/* Converts from a 'YF_TYPEFMT' value. */
#define YF_TYPEFMT_FROM(tf, to) do { \
  switch (tf) { \
  case YF_TYPEFMT_INT: \
    to = VK_FORMAT_R32_SINT; \
    break; \
  case YF_TYPEFMT_UINT: \
    to = VK_FORMAT_R32_UINT; \
    break; \
  case YF_TYPEFMT_FLOAT: \
    to = VK_FORMAT_R32_SFLOAT; \
    break; \
  case YF_TYPEFMT_DOUBLE: \
    to = VK_FORMAT_R64_SFLOAT; \
    break; \
  case YF_TYPEFMT_INT2: \
    to = VK_FORMAT_R32G32_SINT; \
    break; \
  case YF_TYPEFMT_UINT2: \
    to = VK_FORMAT_R32G32_UINT; \
    break; \
  case YF_TYPEFMT_FLOAT2: \
    to = VK_FORMAT_R32G32_SFLOAT; \
    break; \
  case YF_TYPEFMT_DOUBLE2: \
    to = VK_FORMAT_R64G64_SFLOAT; \
    break; \
  case YF_TYPEFMT_INT3: \
    to = VK_FORMAT_R32G32B32_SINT; \
    break; \
  case YF_TYPEFMT_UINT3: \
    to = VK_FORMAT_R32G32B32_UINT; \
    break; \
  case YF_TYPEFMT_FLOAT3: \
    to = VK_FORMAT_R32G32B32_SFLOAT; \
    break; \
  case YF_TYPEFMT_DOUBLE3: \
    to = VK_FORMAT_R64G64B64_SFLOAT; \
    break; \
  case YF_TYPEFMT_INT4: \
    to = VK_FORMAT_R32G32B32A32_SINT; \
    break; \
  case YF_TYPEFMT_UINT4: \
    to = VK_FORMAT_R32G32B32A32_UINT; \
    break; \
  case YF_TYPEFMT_FLOAT4: \
    to = VK_FORMAT_R32G32B32A32_SFLOAT; \
    break; \
  case YF_TYPEFMT_DOUBLE4: \
    to = VK_FORMAT_R64G64B64A64_SFLOAT; \
    break; \
  default: \
    to = VK_FORMAT_UNDEFINED; \
  } } while (0)

/* Converts from a 'YF_VRATE' value. */
#define YF_VRATE_FROM(vr, to) do { \
  switch (vr) { \
  case YF_VRATE_VERT: \
    to = VK_VERTEX_INPUT_RATE_VERTEX; \
    break; \
  case YF_VRATE_INST: \
    to = VK_VERTEX_INPUT_RATE_INSTANCE; \
    break; \
  default: \
    to = INT_MAX; \
  } } while (0)

#endif /* YF_VINPUT_H */
