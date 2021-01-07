/*
 * YF
 * data-sfnt.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_DATA_SFNT_H
#define YF_DATA_SFNT_H

#include "font.h"

/* Loads font resource from a SFNT file. */
int yf_loadsfnt(const char *pathname, YF_fontdt *data);

#endif /* YF_DATA_SFNT_H */
