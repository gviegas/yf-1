/*
 * YF
 * yf-error.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_ERROR_H
#define YF_YF_ERROR_H

#include "yf-common.h"

YF_DECLS_BEGIN

/* Retrieves the last error code. */
int yf_geterr(void);

/* Prints the last error code. */
void yf_printerr(void);

/* Error codes. */
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
