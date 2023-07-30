/*
 * YF
 * yf-defs.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_DEFS_H
#define YF_DEFS_H

/**
 * Linkage macros.
 */
#ifdef __cplusplus
# define YF_DECLS_BEGIN extern "C" {
# define YF_DECLS_END }
#else
# define YF_DECLS_BEGIN
# define YF_DECLS_END
#endif

/**
 * Compilation attributes.
 */
#ifndef YF_UNUSED
# ifdef __GNUC__
#  define YF_UNUSED __attribute__ ((unused))
# else
#  define YF_UNUSED
# endif
#endif

/**
 * Versioning utilities.
 */
#define YF_VERSION_MAKE(major, minor, patch) \
    ( (((major) << 20) & 0xfff00000) | \
      (((minor) << 10) & 0x000ffc00) | \
      ((patch) & 0x000003ff) )

#define YF_MAJOR_GET(version) (((version) >> 20) & 0x00000fff)
#define YF_MINOR_GET(version) (((version) >> 10) & 0x000003ff)
#define YF_PATCH_GET(version) ((version) & 0x000003ff)

#endif /* YF_DEFS_H */
