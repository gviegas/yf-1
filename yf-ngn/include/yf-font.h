/*
 * YF
 * yf-font.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_FONT_H
#define YF_YF_FONT_H

#include <stddef.h>

#include "yf/com/yf-defs.h"

#include "yf-collection.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a font resource.
 */
typedef struct YF_font_o *YF_font;

/**
 * Loads a new font from file.
 *
 * @param pathname: The pathname of the font file.
 * @param index: The index of the font to load.
 * @param coll: The collection for the font.
 * @return: On success, returns a new font. Otherwise, 'NULL' is returned and
 *  the global error is set to indicate the cause.
 */
YF_font yf_font_load(const char *pathname, size_t index, YF_collection coll);

/**
 * Deinitializes a font.
 *
 * @param font: The font to deinitialize. Can be 'NULL'.
 */
void yf_font_deinit(YF_font font);

YF_DECLS_END

#endif /* YF_YF_FONT_H */
