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

#endif /* YF_DEFS_H */
