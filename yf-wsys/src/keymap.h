/*
 * YF
 * keymap.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_KEYMAP_H
#define YF_KEYMAP_H

#include <stddef.h>

/* Converts from a system-specific keycode to a 'YF_KEY' value. */
#define YF_KEY_FROM(code) \
    ((size_t)(code) < yf_g_keymap_n ? yf_g_keymap[(code)] : YF_KEY_UNKNOWN)

/* Keymap. */
extern const int yf_g_keymap[];
extern const size_t yf_g_keymap_n;

#endif /* YF_KEYMAP_H */
