/*
 * YF
 * filetype-bmp.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_FILETYPE_BMP_H
#define YF_FILETYPE_BMP_H

#include "texture.h"

/* Loads texture data from a BMP file. */
int yf_filetype_bmp_load(const char *pathname, YF_texdt *data);

#endif /* YF_FILETYPE_BMP_H */
