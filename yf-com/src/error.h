/*
 * YF
 * error.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_ERROR_H
#define YF_ERROR_H

#include "yf-error.h"

/* Sets the global error code variable. */
void yf_seterr(int err, const char *info);

#endif /* YF_ERROR_H */
