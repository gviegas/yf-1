/*
 * YF
 * yf-common.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_COMMON_H
#define YF_YF_COMMON_H

/* Version utilities. */
#define YF_MAJOR_GET(v) (v >> 16)
#define YF_MINOR_GET(v) (v & 0x0000ffff)
#define YF_VERSION_MAKE(maj, min) ((maj << 16) | min)

/* Version definitions. */
#define YF_CORE_MAJOR 0
#define YF_CORE_MINOR 2
#define YF_CORE_PATCH 0
#define YF_CORE_VERSION YF_VERSION_MAKE(YF_CORE_MAJOR, YF_CORE_MINOR)
#define YF_CORE_VERSION_0_2 YF_VERSION_MAKE(0, 2)

/* Linkage macros. */
#ifdef __cplusplus
# define YF_DECLS_BEGIN extern "C" {
# define YF_DECLS_END }
#else
# define YF_DECLS_BEGIN
# define YF_DECLS_END
#endif

/* Configurable float type. */
#ifdef YF_USE_FLOAT64
typedef double YF_float;
#else
typedef float YF_float;
#endif

/* Type defining a 2D coordinate. */
typedef struct {
  YF_float x;
  YF_float y;
} YF_coord2;

/* Type defining a 3D coordinate. */
typedef struct {
  YF_float x;
  YF_float y;
  YF_float z;
} YF_coord3;

/* Type defining a 2D size. */
typedef struct {
  unsigned width;
  unsigned height;
} YF_dim2;

/* Type defining a 3D size. */
typedef struct {
  unsigned width;
  unsigned height;
  unsigned depth;
} YF_dim3;

/* Type defining a 2D offset. */
typedef struct {
  int x;
  int y;
} YF_offs2;

/* Type defining a 3D offset. */
typedef struct {
  int x;
  int y;
  int z;
} YF_offs3;

/* Type defining a rectangle. */
typedef struct {
  YF_offs2 origin;
  YF_dim2 size;
} YF_rect;

/* Type defining a range of size 'n', starting at index 'i'. */
typedef struct {
  unsigned i;
  unsigned n;
} YF_slice;

/* Type defining a normalized RGBA color. */
typedef struct {
  float r;
  float g;
  float b;
  float a;
} YF_color;

/* Predefined colors. */
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
#define YF_COLOR_TRANSPARENT (YF_color){0.0f}

/* Type defining a viewport. */
typedef struct {
  float x;
  float y;
  float width;
  float height;
  float min_depth;
  float max_depth;
} YF_viewport;

/* Makes a viewport from a given 2D size. */
#define YF_VIEWPORT_FROMDIM2(dim, vport) do { \
  (vport).x = 0.0f; \
  (vport).y = 0.0f; \
  (vport).width = (dim).width; \
  (vport).height = (dim).height; \
  (vport).min_depth = 0.0f; \
  (vport).max_depth = 1.0f; } while (0)

/* Makes a scissor from a given viewport. */
#define YF_VIEWPORT_SCISSOR(vport, sciss_rect) do { \
  (sciss_rect).origin.x = (vport).x; \
  (sciss_rect).origin.y = (vport).y; \
  (sciss_rect).size.width = (vport).width; \
  (sciss_rect).size.height = (vport).height; } while (0)

#endif /* YF_YF_COMMON_H */
