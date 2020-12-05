/*
 * YF
 * platform.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_PLATFORM_H
#define YF_PLATFORM_H

/* Initializes the required platforms. */
int yf_platform_init(void);

/* Queries which platform is being used by the wsi. */
int yf_platform_wsi(void);

/* Deinitializes all platforms. */
void yf_platform_deinit(void);

/* Platforms for wsi. */
#define YF_PLATFORM_WSI_NONE    0
#define YF_PLATFORM_WSI_WAYLAND 1
#define YF_PLATFORM_WSI_XCB     2

#endif /* YF_PLATFORM_H */
