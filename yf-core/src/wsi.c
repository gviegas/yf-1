/*
 * YF
 * wsi.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <assert.h>

#include "wsi.h"
#include "wsi-wayland.h"
#include "wsi-xcb.h"
#include "platform.h"
#include "error.h"

struct YF_wsi_o {
  YF_window win;
  void *data;
};

int yf_wsi_load(int platform) {
  switch (platform) {
    case YF_PLATFORM_WSI_WAYLAND:
      return yf_wsi_wayland_load();
    case YF_PLATFORM_WSI_XCB:
      return yf_wsi_xcb_load();
  }
  yf_seterr(YF_ERR_DEVGEN, __func__);
  return -1;
}

YF_wsi yf_wsi_init(YF_window win) {
  assert(win != NULL);
  YF_wsi wsi = calloc(1, sizeof(struct YF_wsi_o));
  if (wsi == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  wsi->win = win;
  switch (yf_platform_wsi()) {
    case YF_PLATFORM_WSI_WAYLAND:
      if ((wsi->data = yf_wsi_wayland_init(win)) == NULL) {
        free(wsi);
        return NULL;
      }
      break;
    case YF_PLATFORM_WSI_XCB:
      if ((wsi->data = yf_wsi_xcb_init(win)) == NULL) {
        free(wsi);
        return NULL;
      }
      break;
    default:
      yf_seterr(YF_ERR_DEVGEN, __func__);
      free(wsi);
      return NULL;
  }
  return wsi;
}

int yf_wsi_poll(void) {
  int r;
  switch (yf_platform_wsi()) {
    case YF_PLATFORM_WSI_WAYLAND:
      r = yf_wsi_wayland_poll();
      break;
    case YF_PLATFORM_WSI_XCB:
      r = yf_wsi_xcb_poll();
      break;
    default:
      yf_seterr(YF_ERR_DEVGEN, __func__);
      r = -1;
  }
  return r;
}

int yf_wsi_resize(YF_wsi wsi) {
  assert(wsi != NULL);
  int r;
  switch (yf_platform_wsi()) {
    case YF_PLATFORM_WSI_WAYLAND:
      r = yf_wsi_wayland_resize(wsi->data);
      break;
    case YF_PLATFORM_WSI_XCB:
      r = yf_wsi_xcb_resize(wsi->data);
      break;
    default:
      yf_seterr(YF_ERR_DEVGEN, __func__);
      r = -1;
  }
  return r;
}

int yf_wsi_close(YF_wsi wsi) {
  assert(wsi != NULL);
  int r;
  switch (yf_platform_wsi()) {
    case YF_PLATFORM_WSI_WAYLAND:
      r = yf_wsi_wayland_close(wsi->data);
      break;
    case YF_PLATFORM_WSI_XCB:
      r = yf_wsi_xcb_close(wsi->data);
      break;
    default:
      yf_seterr(YF_ERR_DEVGEN, __func__);
      r = -1;
  }
  return r;
}

void yf_wsi_deinit(YF_wsi wsi) {
  if (wsi == NULL)
    return;
  switch (yf_platform_wsi()) {
    case YF_PLATFORM_WSI_WAYLAND:
      yf_wsi_wayland_deinit(wsi->data);
      break;
    case YF_PLATFORM_WSI_XCB:
      yf_wsi_xcb_deinit(wsi->data);
      break;
  }
}

void yf_wsi_unload(void) {
  switch (yf_platform_wsi()) {
    case YF_PLATFORM_WSI_WAYLAND:
      yf_wsi_wayland_unload();
      break;
    case YF_PLATFORM_WSI_XCB:
      yf_wsi_xcb_unload();
      break;
  }
}
