/*
 * YF
 * platform.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>

#include "platform.h"
#include "wsi.h"
#include "error.h"

/* The current platform for wsi. */
static int l_pfm_wsi = YF_PLATFORM_WSI_NONE;

int yf_platform_init(void) {
  int r;
  if (l_pfm_wsi != YF_PLATFORM_WSI_NONE) {
    r = 0;
  } else if (getenv("WAYLAND_DISPLAY") != NULL) {
    l_pfm_wsi = YF_PLATFORM_WSI_WAYLAND;
    r = yf_wsi_load(l_pfm_wsi);
  } else if (getenv("DISPLAY") != NULL) {
    l_pfm_wsi = YF_PLATFORM_WSI_XCB;
    r = yf_wsi_load(l_pfm_wsi);
  } else {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    r = -1;
  }
  return r;
}

int yf_platform_wsi(void) {
  return l_pfm_wsi;
}

void yf_platform_deinit(void) {
  if (l_pfm_wsi != YF_PLATFORM_WSI_NONE) {
    yf_wsi_unload();
    l_pfm_wsi = YF_PLATFORM_WSI_NONE;
  }
}
