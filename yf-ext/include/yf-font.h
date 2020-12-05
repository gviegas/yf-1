/*
 * YF
 * yf-font.h.
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_FONT_H
#define YF_YF_FONT_H

#include "yf-common.h"

YF_DECLS_BEGIN

/* Opaque type defining a font resource. */
typedef struct YF_font_o *YF_font;

/* Initializes a new font. */
YF_font yf_font_init(int filetype, const char *pathname);

/* Deinitializes a font. */
void yf_font_deinit(YF_font font);

YF_DECLS_END

#endif /* YF_YF_FONT_H */
