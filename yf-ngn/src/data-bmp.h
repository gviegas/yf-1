/*
 * YF
 * data-bmp.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_DATA_BMP_H
#define YF_DATA_BMP_H

#include "texture.h"

/* Loads texture data from a BMP file. */
int yf_loadbmp(const char *pathname, YF_texdt *data);

#endif /* YF_DATA_BMP_H */
