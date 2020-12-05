/*
 * YF
 * memory.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_MEMORY_H
#define YF_MEMORY_H

#include "yf-buffer.h"
#include "yf-image.h"

/* Allocates memory for a buffer. */
int yf_buffer_alloc(YF_buffer buf);

/* Allocates memory for an image. */
int yf_image_alloc(YF_image img);

/* Deallocates memory held by a buffer. */
void yf_buffer_free(YF_buffer buf);

/* Deallocates memory held by an image. */
void yf_image_free(YF_image img);

#endif /* YF_MEMORY_H */
