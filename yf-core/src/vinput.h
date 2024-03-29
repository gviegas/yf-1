/*
 * YF
 * vinput.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_VINPUT_H
#define YF_VINPUT_H

#include "yf-vinput.h"

/* Converts from a 'YF_VFMT' value. */
#define YF_VFMT_FROM(vf, to) do { \
    switch (vf) { \
    case YF_VFMT_BYTE: \
        to = VK_FORMAT_R8_SINT; \
        break; \
    case YF_VFMT_UBYTE: \
        to = VK_FORMAT_R8_UINT; \
        break; \
    case YF_VFMT_SHORT: \
        to = VK_FORMAT_R16_SINT; \
        break; \
    case YF_VFMT_USHORT: \
        to = VK_FORMAT_R16_UINT; \
        break; \
    case YF_VFMT_INT: \
        to = VK_FORMAT_R32_SINT; \
        break; \
    case YF_VFMT_UINT: \
        to = VK_FORMAT_R32_UINT; \
        break; \
    case YF_VFMT_FLOAT: \
        to = VK_FORMAT_R32_SFLOAT; \
        break; \
    case YF_VFMT_DOUBLE: \
        to = VK_FORMAT_R64_SFLOAT; \
        break; \
    case YF_VFMT_BYTE2: \
        to = VK_FORMAT_R8G8_SINT; \
        break; \
    case YF_VFMT_UBYTE2: \
        to = VK_FORMAT_R8G8_UINT; \
        break; \
    case YF_VFMT_SHORT2: \
        to = VK_FORMAT_R16G16_SINT; \
        break; \
    case YF_VFMT_USHORT2: \
        to = VK_FORMAT_R16G16_UINT; \
        break; \
    case YF_VFMT_INT2: \
        to = VK_FORMAT_R32G32_SINT; \
        break; \
    case YF_VFMT_UINT2: \
        to = VK_FORMAT_R32G32_UINT; \
        break; \
    case YF_VFMT_FLOAT2: \
        to = VK_FORMAT_R32G32_SFLOAT; \
        break; \
    case YF_VFMT_DOUBLE2: \
        to = VK_FORMAT_R64G64_SFLOAT; \
        break; \
    case YF_VFMT_BYTE3: \
        to = VK_FORMAT_R8G8B8_SINT; \
        break; \
    case YF_VFMT_UBYTE3: \
        to = VK_FORMAT_R8G8B8_UINT; \
        break; \
    case YF_VFMT_SHORT3: \
        to = VK_FORMAT_R16G16B16_SINT; \
        break; \
    case YF_VFMT_USHORT3: \
        to = VK_FORMAT_R16G16B16_UINT; \
        break; \
    case YF_VFMT_INT3: \
        to = VK_FORMAT_R32G32B32_SINT; \
        break; \
    case YF_VFMT_UINT3: \
        to = VK_FORMAT_R32G32B32_UINT; \
        break; \
    case YF_VFMT_FLOAT3: \
        to = VK_FORMAT_R32G32B32_SFLOAT; \
        break; \
    case YF_VFMT_DOUBLE3: \
        to = VK_FORMAT_R64G64B64_SFLOAT; \
        break; \
    case YF_VFMT_BYTE4: \
        to = VK_FORMAT_R8G8B8A8_SINT; \
        break; \
    case YF_VFMT_UBYTE4: \
        to = VK_FORMAT_R8G8B8A8_UINT; \
        break; \
    case YF_VFMT_SHORT4: \
        to = VK_FORMAT_R16G16B16A16_SINT; \
        break; \
    case YF_VFMT_USHORT4: \
        to = VK_FORMAT_R16G16B16A16_UINT; \
        break; \
    case YF_VFMT_INT4: \
        to = VK_FORMAT_R32G32B32A32_SINT; \
        break; \
    case YF_VFMT_UINT4: \
        to = VK_FORMAT_R32G32B32A32_UINT; \
        break; \
    case YF_VFMT_FLOAT4: \
        to = VK_FORMAT_R32G32B32A32_SFLOAT; \
        break; \
    case YF_VFMT_DOUBLE4: \
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
