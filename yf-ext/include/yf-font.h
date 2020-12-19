/*
 * YF
 * yf-font.h.
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_FONT_H
#define YF_YF_FONT_H

#include <yf/com/yf-defs.h>

YF_DECLS_BEGIN

/**
 * Opaque type defining a font resource.
 */
typedef struct YF_font_o *YF_font;

/**
 * Initializes a new font.
 *
 * @param filetype: The 'YF_FILETYPE' value indicating the format of the font
 *  file.
 * @param pathname: The pathname of the font file.
 * @return: On success, returns a new font. Otherwise, 'NULL' is returned and
 *  the global error is set to indicate the cause.
 */
YF_font yf_font_init(int filetype, const char *pathname);

/**
 * Deinitializes a font.
 *
 * @param font: The font to deinitialize. Can be 'NULL'.
 */
void yf_font_deinit(YF_font font);

YF_DECLS_END

#endif /* YF_YF_FONT_H */
