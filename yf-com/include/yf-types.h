/*
 * YF
 * yf-types.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_TYPES_H
#define YF_YF_TYPES_H

/**
 * Type defining a range of size 'n', starting at index 'i'.
 */
typedef struct {
    unsigned i;
    unsigned n;
} YF_slice;

/**
 * Type defining a 2D offset.
 */
typedef struct {
    int x;
    int y;
} YF_off2;

/**
 * Type defining a 3D offset.
 */
typedef struct {
    int x;
    int y;
    int z;
} YF_off3;

/**
 * Type defining a 2D size.
 */
typedef struct {
    unsigned width;
    unsigned height;
} YF_dim2;

/**
 * Type defining a 3D size.
 */
typedef struct {
    unsigned width;
    unsigned height;
    unsigned depth;
} YF_dim3;

/**
 * Type defining a rectangle.
 */
typedef struct {
    YF_off2 origin;
    YF_dim2 size;
} YF_rect;

/**
 * Corners of a rectangle.
 */
#define YF_CORNER_TOPL    0x01
#define YF_CORNER_TOPR    0x02
#define YF_CORNER_BOTTOML 0x04
#define YF_CORNER_BOTTOMR 0x08
#define YF_CORNER_TOP     0x03
#define YF_CORNER_BOTTOM  0x0c
#define YF_CORNER_LEFT    0x05
#define YF_CORNER_RIGHT   0x0a
#define YF_CORNER_ALL     0x0f

/**
 * Type defining a normalized RGBA color.
 */
typedef struct {
    float r;
    float g;
    float b;
    float a;
} YF_color;

/**
 * Predefined colors.
 */
#define YF_COLOR_RED         (YF_color){1.0f, 0.0f, 0.0f, 1.0f}
#define YF_COLOR_GREEN       (YF_color){0.0f, 1.0f, 0.0f, 1.0f}
#define YF_COLOR_BLUE        (YF_color){0.0f, 0.0f, 1.0f, 1.0f}
#define YF_COLOR_CYAN        (YF_color){0.0f, 1.0f, 1.0f, 1.0f}
#define YF_COLOR_MAGENTA     (YF_color){1.0f, 0.0f, 1.0f, 1.0f}
#define YF_COLOR_YELLOW      (YF_color){1.0f, 1.0f, 0.0f, 1.0f}
#define YF_COLOR_WHITE       (YF_color){1.0f, 1.0f, 1.0f, 1.0f}
#define YF_COLOR_BLACK       (YF_color){0.0f, 0.0f, 0.0f, 1.0f}
#define YF_COLOR_GREY        (YF_color){0.5f, 0.5f, 0.5f, 1.0f}
#define YF_COLOR_LIGHTGREY   (YF_color){0.9f, 0.9f, 0.9f, 1.0f}
#define YF_COLOR_DARKGREY    (YF_color){0.1f, 0.1f, 0.1f, 1.0f}
#define YF_COLOR_TRANSPARENT (YF_color){1.0f, 1.0f, 1.0f, 0.0f}

/**
 * Type defining a viewport.
 */
typedef struct {
    float x;
    float y;
    float width;
    float height;
    float min_depth;
    float max_depth;
} YF_viewport;

/**
 * Makes a viewport from a given 2D size.
 *
 * @param dim: The 'YF_dim2' from which to produce a viewport.
 * @param vport: The 'YF_viewport' to set.
 */
#define YF_VIEWPORT_FROMDIM2(dim, vport) do { \
    (vport).x = 0.0f; \
    (vport).y = 0.0f; \
    (vport).width = (dim).width; \
    (vport).height = (dim).height; \
    (vport).min_depth = 0.0f; \
    (vport).max_depth = 1.0f; } while (0)

/**
 * Makes a scissor from a given viewport.
 *
 * @param vport: The 'YF_viewport' from which to produce a scissor.
 * @param sciss_rect: The 'YF_rect' to set.
 */
#define YF_VIEWPORT_SCISSOR(vport, sciss_rect) do { \
    (sciss_rect).origin.x = (vport).x; \
    (sciss_rect).origin.y = (vport).y; \
    (sciss_rect).size.width = (vport).width; \
    (sciss_rect).size.height = (vport).height; } while (0)

#endif /* YF_YF_TYPES_H */
