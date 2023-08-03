/*
 * YF
 * data-sfnt.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_DATA_SFNT_H
#define YF_DATA_SFNT_H

#include <stdio.h>

#include "font.h"

/* Loads font resource from a SFNT file. */
int yf_loadsfnt(const char *pathname, yf_fontdt_t *data);

/* Loads font resource from a file containing SFNT data.
   Decoding begins at the file's current position. */
int yf_loadsfnt2(FILE *file, yf_fontdt_t *data);

#endif /* YF_DATA_SFNT_H */
