/*
 * YF
 * keymap.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include "keymap.h"
#include "yf-keyboard.h"

#if defined(__linux__)
/* Mapping between linux event codes and 'YF_KEY' values. */
const int yf_g_keymap[] = {
    YF_KEY_UNKNOWN,
    YF_KEY_ESC,
    YF_KEY_1,
    YF_KEY_2,
    YF_KEY_3,
    YF_KEY_4,
    YF_KEY_5,
    YF_KEY_6,
    YF_KEY_7,
    YF_KEY_8,
    YF_KEY_9,
    YF_KEY_0,
    YF_KEY_MINUS,
    YF_KEY_EQUAL,
    YF_KEY_BACKSPACE,
    YF_KEY_TAB,
    YF_KEY_Q,
    YF_KEY_W,
    YF_KEY_E,
    YF_KEY_R,
    YF_KEY_T,
    YF_KEY_Y,
    YF_KEY_U,
    YF_KEY_I,
    YF_KEY_O,
    YF_KEY_P,
    YF_KEY_LBRACKET,
    YF_KEY_RBRACKET,
    YF_KEY_RETURN,
    YF_KEY_LCTRL,
    YF_KEY_A,
    YF_KEY_S,
    YF_KEY_D,
    YF_KEY_F,
    YF_KEY_G,
    YF_KEY_H,
    YF_KEY_J,
    YF_KEY_K,
    YF_KEY_L,
    YF_KEY_SEMICOLON,
    YF_KEY_APOSTROPHE,
    YF_KEY_GRAVE,
    YF_KEY_LSHIFT,
    YF_KEY_BACKSLASH,
    YF_KEY_Z,
    YF_KEY_X,
    YF_KEY_C,
    YF_KEY_V,
    YF_KEY_B,
    YF_KEY_N,
    YF_KEY_M,
    YF_KEY_COMMA,
    YF_KEY_DOT,
    YF_KEY_SLASH,
    YF_KEY_RSHIFT,
    YF_KEY_PADSTAR,
    YF_KEY_LALT,
    YF_KEY_SPACE,
    YF_KEY_CAPSLOCK,
    YF_KEY_F1,
    YF_KEY_F2,
    YF_KEY_F3,
    YF_KEY_F4,
    YF_KEY_F5,
    YF_KEY_F6,
    YF_KEY_F7,
    YF_KEY_F8,
    YF_KEY_F9,
    YF_KEY_F10,
    YF_KEY_PADNUMLOCK,
    YF_KEY_SCROLLLOCK,
    YF_KEY_PAD7,
    YF_KEY_PAD8,
    YF_KEY_PAD9,
    YF_KEY_PADMINUS,
    YF_KEY_PAD4,
    YF_KEY_PAD5,
    YF_KEY_PAD6,
    YF_KEY_PADPLUS,
    YF_KEY_PAD1,
    YF_KEY_PAD2,
    YF_KEY_PAD3,
    YF_KEY_PAD0,
    YF_KEY_PADDOT,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_F11,
    YF_KEY_F12,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_PADENTER,
    YF_KEY_RCTRL,
    YF_KEY_PADSLASH,
    YF_KEY_SYSRQ,
    YF_KEY_RALT,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UP,
    YF_KEY_PAGEUP,
    YF_KEY_LEFT,
    YF_KEY_RIGHT,
    YF_KEY_END,
    YF_KEY_DOWN,
    YF_KEY_PAGEDOWN,
    YF_KEY_INSERT,
    YF_KEY_DELETE,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_PADEQUAL,
    YF_KEY_UNKNOWN,
    YF_KEY_PAUSE,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_LMETA,
    YF_KEY_RMETA,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_UNKNOWN,
    YF_KEY_F13,
    YF_KEY_F14,
    YF_KEY_F15,
    YF_KEY_F16,
    YF_KEY_F17,
    YF_KEY_F18,
    YF_KEY_F19,
    YF_KEY_F20,
    YF_KEY_F21,
    YF_KEY_F22,
    YF_KEY_F23,
    YF_KEY_F24
};

const size_t yf_g_keymap_n = sizeof yf_g_keymap / sizeof yf_g_keymap[0];
#else
/* TODO */
# error "Invalid platform"
#endif /* defined(__linux__) */
