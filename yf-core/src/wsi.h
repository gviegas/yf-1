/*
 * YF
 * wsi.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_WSI_H
#define YF_WSI_H

typedef struct YF_window_o *YF_window;

/* Opaque type defining the interface with a window system. */
typedef struct YF_wsi_o *YF_wsi;

/* Loads a given wsi platform. */
int yf_wsi_load(int platform);

/* Initializes a new wsi window. */
YF_wsi yf_wsi_init(YF_window win);

/* Dequeues wsi events and calls installed handlers. */
int yf_wsi_poll(void);

/* Resizes a wsi window. */
int yf_wsi_resize(YF_wsi wsi);

/* Closes a wsi window. */
int yf_wsi_close(YF_wsi wsi);

/* Deinitializes a wsi. */
void yf_wsi_deinit(YF_wsi wsi);

/* Unloads the current wsi platform. */
void yf_wsi_unload(void);

#endif /* YF_WSI_H */
