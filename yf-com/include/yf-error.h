/*
 * YF
 * yf-error.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_ERROR_H
#define YF_YF_ERROR_H

#include <stddef.h>

#include "yf-defs.h"

YF_DECLS_BEGIN

/**
 * Sets the error code variable.
 *
 * @param err: The 'YF_ERR' value to set.
 * @param info: Custom info. Can be 'NULL'.
 */
void yf_seterr(int err, const char *info);

/**
 * Gets the last error code.
 *
 * @return: The last 'YF_ERR' value that was set.
 */
int yf_geterr(void);

/**
 * Gets the last error info.
 *
 * @param dst: The destination for the info string.
 * @param n: The length of the destination buffer.
 * @return: If the length of the info string exceeds 'n', returns 'NULL'.
 *  Otherwise, returns 'dst'.
 */
char *yf_geterrinfo(char *dst, size_t n);

/**
 * Prints the last error.
 */
void yf_printerr(void);

/**
 * Error codes.
 */
#define YF_ERR_UNKNOWN 1
#define YF_ERR_NOMEM   2
#define YF_ERR_INVARG  3
#define YF_ERR_NILPTR  4
#define YF_ERR_NOFILE  5
#define YF_ERR_INVFILE 6
#define YF_ERR_INUSE   7
#define YF_ERR_BUSY    8
#define YF_ERR_INVCMD  9
#define YF_ERR_QFULL   10
#define YF_ERR_NOTFND  11
#define YF_ERR_EXIST   12
#define YF_ERR_INVWIN  13
#define YF_ERR_UNSUP   14
#define YF_ERR_OFLOW   15
#define YF_ERR_LIMIT   16
#define YF_ERR_DEVGEN  17
#define YF_ERR_OTHER   32767

YF_DECLS_END

#endif /* YF_YF_ERROR_H */
