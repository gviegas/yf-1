/*
 * YF
 * yf-vinput.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
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
#define YF_VFMT_BYTE   16
#define YF_VFMT_UBYTE  17
#define YF_VFMT_SHORT  18
#define YF_VFMT_USHORT 19
#define YF_VFMT_INT    20
#define YF_VFMT_UINT   21
#define YF_VFMT_FLOAT  22
#define YF_VFMT_DOUBLE 23

/**
 * 2-component formats.
 */
#define YF_VFMT_BYTE2   32
#define YF_VFMT_UBYTE2  33
#define YF_VFMT_SHORT2  34
#define YF_VFMT_USHORT2 35
#define YF_VFMT_INT2    36
#define YF_VFMT_UINT2   37
#define YF_VFMT_FLOAT2  38
#define YF_VFMT_DOUBLE2 39

/**
 * 3-component formats.
 */
#define YF_VFMT_BYTE3   48
#define YF_VFMT_UBYTE3  49
#define YF_VFMT_SHORT3  50
#define YF_VFMT_USHORT3 51
#define YF_VFMT_INT3    52
#define YF_VFMT_UINT3   53
#define YF_VFMT_FLOAT3  54
#define YF_VFMT_DOUBLE3 55

/**
 * 4-component formats.
 */
#define YF_VFMT_BYTE4   64
#define YF_VFMT_UBYTE4  65
#define YF_VFMT_SHORT4  66
#define YF_VFMT_USHORT4 67
#define YF_VFMT_INT4    68
#define YF_VFMT_UINT4   69
#define YF_VFMT_FLOAT4  70
#define YF_VFMT_DOUBLE4 71

/**
 * Type defining a single vertex attribute.
 */
typedef struct yf_vattr {
    unsigned location;
    int vfmt;
    size_t offset;
} yf_vattr_t;

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
typedef struct yf_vinput {
    const yf_vattr_t *attrs;
    unsigned attr_n;
    size_t stride;
    int vrate;
} yf_vinput_t;

#endif /* YF_YF_VINPUT_H */
