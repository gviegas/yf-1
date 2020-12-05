/*
 * YF
 * wsi-xcb.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_WSI_XCB_H
#define YF_WSI_XCB_H

typedef struct YF_window_o *YF_window;

/* Loads required symbols for wsi-xcb. */
int yf_wsi_xcb_load(void);

/* Initializes a new wsi-xcb window. */
void *yf_wsi_xcb_init(YF_window win);

/* Dequeues wsi-xcb events and calls installed handlers. */
int yf_wsi_xcb_poll(void);

/* Resizes a wsi-xcb window. */
int yf_wsi_xcb_resize(void *data);

/* Closes a wsi-xcb window. */
int yf_wsi_xcb_close(void *data);

/* Deinitializes a wsi-xcb window. */
void yf_wsi_xcb_deinit(void *data);

/* Unloads wsi-xcb symbols. */
void yf_wsi_xcb_unload(void);

#endif /* YF_WSI_XCB_H */
