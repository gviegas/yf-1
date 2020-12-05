/*
 * YF
 * wsi-wayland.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_WSI_WAYLAND_H
#define YF_WSI_WAYLAND_H

typedef struct YF_window_o *YF_window;

/* Loads required symbols for wsi-wayland. */
int yf_wsi_wayland_load(void);

/* Initializes a new wsi-wayland window. */
void *yf_wsi_wayland_init(YF_window win);

/* Dequeues wsi-wayland events and calls installed handlers. */
int yf_wsi_wayland_poll(void);

/* Resizes a wsi-wayland window. */
int yf_wsi_wayland_resize(void *data);

/* Closes a wsi-wayland window. */
int yf_wsi_wayland_close(void *data);

/* Deinitializes a wsi_wayland. */
void yf_wsi_wayland_deinit(void *data);

/* Unloads wsi-wayland symbols. */
void yf_wsi_wayland_unload(void);

#endif /* YF_WSI_WAYLAND_H */
