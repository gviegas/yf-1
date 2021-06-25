/*
 * YF
 * yf-vinput.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_VINPUT_H
#define YF_YF_VINPUT_H

/**
 * Undefined vertex format.
 */
#define YF_VFMT_UNDEF 0

/**
 * 1-component formats.
 */
#define YF_VFMT_INT    16
#define YF_VFMT_UINT   17
#define YF_VFMT_FLOAT  18
#define YF_VFMT_DOUBLE 19

/**
 * 2-component formats.
 */
#define YF_VFMT_INT2    32
#define YF_VFMT_UINT2   33
#define YF_VFMT_FLOAT2  34
#define YF_VFMT_DOUBLE2 35

/**
 * 3-component formats.
 */
#define YF_VFMT_INT3    48
#define YF_VFMT_UINT3   49
#define YF_VFMT_FLOAT3  50
#define YF_VFMT_DOUBLE3 51

/**
 * 4-component formats.
 */
#define YF_VFMT_INT4    64
#define YF_VFMT_UINT4   65
#define YF_VFMT_FLOAT4  66
#define YF_VFMT_DOUBLE4 67

/**
 * Type defining a single vertex attribute.
 */
typedef struct {
    unsigned location;
    int vfmt;
    size_t offset;
} YF_vattr;

/**
 * Vertex input rate.
 */
#define YF_VRATE_VERT 0
#define YF_VRATE_INST 1

/**
 * Type defining a vertex input.
 *
 * The vertex input defines a set of attributes residing in the same vertex
 * buffer. A buffer object bound as vertex buffer during command encoding must
 * adhere to the input layout.
 *
 * Non-interleaved data requires one 'vinput' per attribute. Interleaved data
 * uses the 'offset' member of each attribute to indicate their locations in
 * the vertex buffer.
 */
typedef struct {
    const YF_vattr *attrs;
    unsigned attr_n;
    size_t stride;
    int vrate;
} YF_vinput;

#endif /* YF_YF_VINPUT_H */
