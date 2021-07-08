/*
 * YF
 * data-png.h
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#ifndef YF_DATA_PNG_H
#define YF_DATA_PNG_H

#include <stdio.h>

#include "texture.h"

/* Loads texture data from a PNG file. */
int yf_loadpng(const char *pathname, YF_texdt *data);

/* Loads texture data from a file containing PNG data.
   Decoding begins at the file's current position. */
int yf_loadpng2(FILE *file, YF_texdt *data);

#endif /* YF_DATA_PNG_H */
