/*
 * YF
 * yf-keyboard.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_KEYBOARD_H
#define YF_YF_KEYBOARD_H

/**
 * Key codes.
 */
#define YF_KEY_UNKNOWN    0
#define YF_KEY_GRAVE      1
#define YF_KEY_1          2
#define YF_KEY_2          3
#define YF_KEY_3          4
#define YF_KEY_4          5
#define YF_KEY_5          6
#define YF_KEY_6          7
#define YF_KEY_7          8
#define YF_KEY_8          9
#define YF_KEY_9          10
#define YF_KEY_0          11
#define YF_KEY_MINUS      12
#define YF_KEY_EQUAL      13
#define YF_KEY_BACKSPACE  14
#define YF_KEY_TAB        15
#define YF_KEY_Q          16
#define YF_KEY_W          17
#define YF_KEY_E          18
#define YF_KEY_R          19
#define YF_KEY_T          20
#define YF_KEY_Y          21
#define YF_KEY_U          22
#define YF_KEY_I          23
#define YF_KEY_O          24
#define YF_KEY_P          25
#define YF_KEY_LBRACKET   26
#define YF_KEY_RBRACKET   27
#define YF_KEY_BACKSLASH  28
#define YF_KEY_CAPSLOCK   29
#define YF_KEY_A          30
#define YF_KEY_S          31
#define YF_KEY_D          32
#define YF_KEY_F          33
#define YF_KEY_G          34
#define YF_KEY_H          35
#define YF_KEY_J          36
#define YF_KEY_K          37
#define YF_KEY_L          38
#define YF_KEY_SEMICOLON  39
#define YF_KEY_APOSTROPHE 40
#define YF_KEY_RETURN     41
#define YF_KEY_LSHIFT     42
#define YF_KEY_Z          43
#define YF_KEY_X          44
#define YF_KEY_C          45
#define YF_KEY_V          46
#define YF_KEY_B          47
#define YF_KEY_N          48
#define YF_KEY_M          49
#define YF_KEY_COMMA      50
#define YF_KEY_DOT        51
#define YF_KEY_SLASH      52
#define YF_KEY_RSHIFT     53
#define YF_KEY_LCTRL      54
#define YF_KEY_LALT       55
#define YF_KEY_LMETA      56
#define YF_KEY_SPACE      57
#define YF_KEY_RMETA      58
#define YF_KEY_RALT       59
#define YF_KEY_RCTRL      60
#define YF_KEY_ESC        61
#define YF_KEY_F1         62
#define YF_KEY_F2         63
#define YF_KEY_F3         64
#define YF_KEY_F4         65
#define YF_KEY_F5         66
#define YF_KEY_F6         67
#define YF_KEY_F7         68
#define YF_KEY_F8         69
#define YF_KEY_F9         70
#define YF_KEY_F10        71
#define YF_KEY_F11        72
#define YF_KEY_F12        73
#define YF_KEY_INSERT     74
#define YF_KEY_DELETE     75
#define YF_KEY_HOME       76
#define YF_KEY_END        77
#define YF_KEY_PAGEUP     78
#define YF_KEY_PAGEDOWN   79
#define YF_KEY_UP         80
#define YF_KEY_DOWN       81
#define YF_KEY_LEFT       82
#define YF_KEY_RIGHT      83
#define YF_KEY_SYSRQ      84
#define YF_KEY_SCROLLLOCK 85
#define YF_KEY_PAUSE      86
#define YF_KEY_PADNUMLOCK 87
#define YF_KEY_PADSLASH   88
#define YF_KEY_PADSTAR    89
#define YF_KEY_PADMINUS   90
#define YF_KEY_PADPLUS    91
#define YF_KEY_PAD1       92
#define YF_KEY_PAD2       93
#define YF_KEY_PAD3       94
#define YF_KEY_PAD4       95
#define YF_KEY_PAD5       96
#define YF_KEY_PAD6       97
#define YF_KEY_PAD7       98
#define YF_KEY_PAD8       99
#define YF_KEY_PAD9       100
#define YF_KEY_PAD0       101
#define YF_KEY_PADDOT     102
#define YF_KEY_PADENTER   103
#define YF_KEY_PADEQUAL   104
#define YF_KEY_F13        105
#define YF_KEY_F14        106
#define YF_KEY_F15        107
#define YF_KEY_F16        108
#define YF_KEY_F17        109
#define YF_KEY_F18        110
#define YF_KEY_F19        111
#define YF_KEY_F20        112
#define YF_KEY_F21        113
#define YF_KEY_F22        114
#define YF_KEY_F23        115
#define YF_KEY_F24        116

/**
 * Key modifiers.
 */
#define YF_KEYMOD_CAPSLOCK 0x01
#define YF_KEYMOD_SHIFT    0x02
#define YF_KEYMOD_CTRL     0x04
#define YF_KEYMOD_ALT      0x08

/**
 * Key states.
 */
#define YF_KEYSTATE_RELEASED 0
#define YF_KEYSTATE_PRESSED  1

#endif /* YF_YF_KEYBOARD_H */
