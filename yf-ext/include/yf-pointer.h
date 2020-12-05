/*
 * YF
 * yf-pointer.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_YF_POINTER_H
#define YF_YF_POINTER_H

#include "yf-common.h"

YF_DECLS_BEGIN

/* Gets the pointer coordinates. */
YF_coord2 yf_pointer_getcoord(void);

/* Checks whether or not the pointer is at a valid position. */
int yf_pointer_isvalid(void);

/* Pointer buttons. */
#define YF_BTN_UNKNOWN  0
#define YF_BTN_LEFT     1
#define YF_BTN_RIGHT    2
#define YF_BTN_MIDDLE   3
#define YF_BTN_SIDE     4
#define YF_BTN_FORWARD  5
#define YF_BTN_BACKWARD 6

/* Pointer button states. */
#define YF_BTNSTATE_RELEASED 0
#define YF_BTNSTATE_PRESSED  1

YF_DECLS_END

#endif /* YF_YF_POINTER_H */
