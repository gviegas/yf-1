/*
 * YF
 * wsi-wayland.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <assert.h>

#include <yf/com/yf-list.h>
#include <yf/com/yf-error.h>

#include <wayland-client-core.h>
#include "vk.h"
#include <vulkan/vulkan_wayland.h>
#ifdef __linux__
# include <linux/input-event-codes.h>
#else
# error "Invalid platform"
#endif

#include "wsi-wayland.h"
#include "context.h"
#include "window.h"
#include "event.h"
#include "pointer.h"
#include "keyboard.h"

#ifndef YF__UNUSED
# ifdef __GNUC__
#  define YF__UNUSED __attribute__ ((unused))
# else
#  define YF__UNUSED
# endif
#endif

/* TODO: Make sure that this value is defined as a null-terminated string. */
#ifndef YF_APP_ID
# define YF_APP_ID "unknown.app.id"
#endif

/* Function pointer for surface creation. */
static YF_DEFVK(vkCreateWaylandSurfaceKHR);

/* Protocol interfaces. */
extern const struct wl_interface wl_registry_interface;
extern const struct wl_interface wl_compositor_interface;
extern const struct wl_interface wl_shell_interface;
extern const struct wl_interface wl_shell_surface_interface;
extern const struct wl_interface wl_surface_interface;
extern const struct wl_interface wl_seat_interface;
extern const struct wl_interface wl_pointer_interface;
extern const struct wl_interface wl_keyboard_interface;
extern const struct wl_interface xdg_wm_base_interface;
extern const struct wl_interface xdg_surface_interface;
extern const struct wl_interface xdg_toplevel_interface;

/* Protocol opcodes. */
#define YF_WL_DISPLAY_GET_REGISTRY       1
#define YF_WL_REGISTRY_BIND              0
#define YF_WL_COMPOSITOR_CREATE_SURFACE  0
#define YF_WL_SURFACE_DESTROY            0
#define YF_WL_SURFACE_COMMIT             6
#define YF_WL_SHELL_GET_SHELL_SURFACE    0
#define YF_WL_SHELL_SURFACE_PONG         0
#define YF_WL_SHELL_SURFACE_SET_TOPLEVEL 3
#define YF_WL_SHELL_SURFACE_SET_TITLE    8
#define YF_WL_SHELL_SURFACE_SET_CLASS    9
#define YF_WL_SEAT_GET_POINTER           0
#define YF_WL_SEAT_GET_KEYBOARD          1
#define YF_WL_SEAT_RELEASE               3
#define YF_WL_POINTER_RELEASE            1
#define YF_WL_KEYBOARD_RELEASE           0
#define YF_XDG_WM_BASE_DESTROY           0
#define YF_XDG_WM_BASE_GET_XDG_SURFACE   2
#define YF_XDG_WM_BASE_PONG              3
#define YF_XDG_SURFACE_DESTROY           0
#define YF_XDG_SURFACE_GET_TOPLEVEL      1
#define YF_XDG_SURFACE_ACK_CONFIGURE     4
#define YF_XDG_TOPLEVEL_DESTROY          0
#define YF_XDG_TOPLEVEL_SET_TITLE        2
#define YF_XDG_TOPLEVEL_SET_APP_ID       3

/* Protocol listener - registry. */
static void registry_global(
  void *data,
  struct wl_proxy *registry,
  uint32_t name,
  const char *interface,
  uint32_t version);

static void registry_global_remove(
  void *data,
  struct wl_proxy *registry,
  uint32_t name);

static void (*const l_registry_listener[2])(void) = {
  (void (*)(void))registry_global,
  (void (*)(void))registry_global_remove
};

/* Protocol listener - shell surface. */
static void shell_surface_ping(
  void *data,
  struct wl_proxy *shell_surface,
  uint32_t serial);

static void shell_surface_configure(
  void *data,
  struct wl_proxy *shell_surface,
  uint32_t edges,
  int32_t width,
  int32_t height);

static void shell_surface_popup_done(
  void *data,
  struct wl_proxy *shell_surface);

static void (*const l_shell_surface_listener[3])(void) = {
  (void (*)(void))shell_surface_ping,
  (void (*)(void))shell_surface_configure,
  (void (*)(void))shell_surface_popup_done
};

/* Protocol listener - seat. */
static void seat_capabilities(
  void *data,
  struct wl_proxy *seat,
  uint32_t capabilities);

static void seat_name(void *data, struct wl_proxy *seat, const char *name);

static void (*const l_seat_listener[2])(void) = {
  (void (*)(void))seat_capabilities,
  (void (*)(void))seat_name
};

/* Protocol listener - pointer. */
static void pointer_enter(
  void *data,
  struct wl_proxy *pointer,
  uint32_t serial,
  struct wl_proxy *surface,
  wl_fixed_t surface_x,
  wl_fixed_t surface_y);

static void pointer_leave(
  void *data,
  struct wl_proxy *pointer,
  uint32_t serial,
  struct wl_proxy *surface);

static void pointer_motion(
  void *data,
  struct wl_proxy *pointer,
  uint32_t time,
  wl_fixed_t surface_x,
  wl_fixed_t surface_y);

static void pointer_button(
  void *data,
  struct wl_proxy *pointer,
  uint32_t serial,
  uint32_t time,
  uint32_t button,
  uint32_t state);

static void pointer_axis(
  void *data,
  struct wl_proxy *pointer,
  uint32_t time,
  uint32_t axis,
  wl_fixed_t value);

static void pointer_frame(void *data, struct wl_proxy *pointer);

static void pointer_axis_source(
  void *data,
  struct wl_proxy *pointer,
  uint32_t axis_source);

static void pointer_axis_stop(
  void *data,
  struct wl_proxy *pointer,
  uint32_t time,
  uint32_t axis);

static void pointer_axis_discrete(
  void *data,
  struct wl_proxy *pointer,
  uint32_t axis,
  int32_t discrete);

static void (*const l_pointer_listener[9])(void) = {
  (void (*)(void))pointer_enter,
  (void (*)(void))pointer_leave,
  (void (*)(void))pointer_motion,
  (void (*)(void))pointer_button,
  (void (*)(void))pointer_axis,
  (void (*)(void))pointer_frame,
  (void (*)(void))pointer_axis_source,
  (void (*)(void))pointer_axis_stop,
  (void (*)(void))pointer_axis_discrete
};

/* Protocol listener - keyboard. */
static void keyboard_keymap(
  void *data,
  struct wl_proxy *keyboard,
  uint32_t format,
  int32_t fd,
  uint32_t size);

static void keyboard_enter(
  void *data,
  struct wl_proxy *keyboard,
  uint32_t serial,
  struct wl_proxy *surface,
  struct wl_array *keys);

static void keyboard_leave(
  void *data,
  struct wl_proxy *keyboard,
  uint32_t serial,
  struct wl_proxy *surface);

static void keyboard_key(
  void *data,
  struct wl_proxy *keyboard,
  uint32_t serial,
  uint32_t time,
  uint32_t key,
  uint32_t state);

static void keyboard_modifiers(
  void *data,
  struct wl_proxy *keyboard,
  uint32_t serial,
  uint32_t mods_depressed,
  uint32_t mods_latched,
  uint32_t mods_locked,
  uint32_t group);

static void keyboard_repeat_info(
  void *data,
  struct wl_proxy *keyboard,
  int32_t rate,
  int32_t delay);

static void (*const l_keyboard_listener[6])(void) = {
  (void (*)(void))keyboard_keymap,
  (void (*)(void))keyboard_enter,
  (void (*)(void))keyboard_leave,
  (void (*)(void))keyboard_key,
  (void (*)(void))keyboard_modifiers,
  (void (*)(void))keyboard_repeat_info
};

/* Protocol listener - wm base. */
static void wm_base_ping(void *data, struct wl_proxy *wm_base, uint32_t serial);

static void (*const l_wm_base_listener[1])(void) = {
  (void (*)(void))wm_base_ping
};

/* Protocol listener - xdg surface. */
static void xdg_surface_configure(
  void *data,
  struct wl_proxy *xdg_surface,
  uint32_t serial);

static void (*const l_xdg_surface_listener[1])(void) = {
  (void (*)(void))xdg_surface_configure
};

/* Protocol listener - toplevel. */
static void toplevel_configure(
  void *data,
  struct wl_proxy *toplevel,
  int32_t width,
  int32_t height,
  struct wl_array *states);

static void toplevel_close(void *data, struct wl_proxy *toplevel);

static void (*const l_toplevel_listener[2])(void) = {
  (void (*)(void))toplevel_configure,
  (void (*)(void))toplevel_close
};

/* Identifiers for the desktop shell type in use. */
#define YF_DSH_UNDEF 0
#define YF_DSH_CORE  1
#define YF_DSH_XDG   2

/* Variables used by all wsi-wayland objects. */
typedef struct {
  struct wl_display *display;
  struct wl_proxy *registry;
  struct wl_proxy *compositor;
  struct wl_proxy *seat;
  struct wl_proxy *pointer;
  struct wl_proxy *keyboard;
  int dsh;
  union {
    struct {
      struct wl_proxy *shell;
    } core_shell;
    struct {
      struct wl_proxy *wm_base;
    } xdg_shell;
  };
  unsigned event_n;
  unsigned mod_mask;
} L_vars;

/* Data of a wsi-wayland object. */
typedef struct {
  YF_window win;
  struct wl_proxy *surface;
  int dsh;
  union {
    struct {
      struct wl_proxy *surface;
    } core_shell;
    struct {
      struct wl_proxy *surface;
      struct wl_proxy *toplevel;
    } xdg_shell;
  };
} L_data;

/* Variables' instance. */
static L_vars l_vars = {0};

/* List of data objects created. */
static YF_list l_data_list = NULL;

/* Symbol names. */
static const char *l_sym_names[] = {
#define YF_SYM_PROXY_MARSHAL 0
  "wl_proxy_marshal",
#define YF_SYM_PROXY_MARSHAL_CONSTRUCTOR 1
  "wl_proxy_marshal_constructor",
#define YF_SYM_PROXY_MARSHAL_CONSTRUCTOR_VERSIONED 2
  "wl_proxy_marshal_constructor_versioned",
#define YF_SYM_PROXY_DESTROY 3
  "wl_proxy_destroy",
#define YF_SYM_PROXY_ADD_LISTENER 4
  "wl_proxy_add_listener",
#define YF_SYM_DISPLAY_CONNECT 5
  "wl_display_connect",
#define YF_SYM_DISPLAY_DISCONNECT 6
  "wl_display_disconnect",
#define YF_SYM_DISPLAY_DISPATCH_PENDING 7
  "wl_display_dispatch_pending",
#define YF_SYM_DISPLAY_FLUSH 8
  "wl_display_flush",
#define YF_SYM_DISPLAY_ROUNDTRIP 9
  "wl_display_roundtrip"
};

/* Symbol addresses. */
static void *l_sym_addrs[sizeof l_sym_names / sizeof l_sym_names[0]] = {0};

/* Shared object handle. */
static void *l_handle = NULL;

/* Wrappers. */
#define YF_WL_PROXY_MARSHAL(proxy, opcode, ...) do { \
  void (*fn)(struct wl_proxy *, uint32_t, ...); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_PROXY_MARSHAL]; \
  fn(proxy, opcode, ## __VA_ARGS__); } while (0)

#define YF_WL_PROXY_MARSHAL_CONSTRUCTOR(res, proxy, opcode, iface, ...) do { \
  struct wl_proxy *(*fn)( \
    struct wl_proxy *, uint32_t, const struct wl_interface *, ...); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_PROXY_MARSHAL_CONSTRUCTOR]; \
  res = fn(proxy, opcode, iface, __VA_ARGS__); } while (0)

#define YF_WL_PROXY_MARSHAL_CONSTRUCTOR_VERSIONED( \
res, proxy, opcode, iface, version, ...) do { \
  struct wl_proxy *(*fn)( \
    struct wl_proxy *, uint32_t, const struct wl_interface *, uint32_t, ...); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_PROXY_MARSHAL_CONSTRUCTOR_VERSIONED]; \
  res = fn(proxy, opcode, iface, version, __VA_ARGS__); } while (0)

#define YF_WL_PROXY_DESTROY(proxy) do { \
  void (*fn)(struct wl_proxy *); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_PROXY_DESTROY]; \
  fn(proxy); } while (0)

#define YF_WL_PROXY_ADD_LISTENER(res, proxy, impl, data) do { \
  int (*fn)(struct wl_proxy *, void (**)(void), void *); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_PROXY_ADD_LISTENER]; \
  res = fn(proxy, impl, data); } while (0)

#define YF_WL_DISPLAY_CONNECT(res, name) do { \
  struct wl_display *(*fn)(const char *); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_DISPLAY_CONNECT]; \
  res = fn(name); } while (0)

#define YF_WL_DISPLAY_DISCONNECT(display) do { \
  void (*fn)(struct wl_display *); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_DISPLAY_DISCONNECT]; \
  fn(display); } while (0)

#define YF_WL_DISPLAY_DISPATCH_PENDING(res, display) do { \
  int (*fn)(struct wl_display *); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_DISPLAY_DISPATCH_PENDING]; \
  res = fn(display); } while (0)

#define YF_WL_DISPLAY_FLUSH(res, display) do { \
  int (*fn)(struct wl_display *); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_DISPLAY_FLUSH]; \
  res = fn(display); } while (0)

#define YF_WL_DISPLAY_ROUNDTRIP(res, display) do { \
  int (*fn)(struct wl_display *); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_DISPLAY_ROUNDTRIP]; \
  res = fn(display); } while (0)

/* Initializes the variables' instance. */
static int init_vars(void);

/* Initializes a new data object. */
static void *init_data(YF_window win);

/* Gets the data object that contains at least one of the provided values. */
static L_data *get_data(YF_window win, struct wl_proxy *surface);

/* Deinitializes the variables' instance. */
static void deinit_vars(void);

int yf_wsi_wayland_load(void) {
  if (l_handle != NULL)
    return 0;
  l_handle = dlopen("libwayland-client.so", RTLD_LAZY);
  if (l_handle == NULL) {
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  char *err;
  for (unsigned i = 0; i < sizeof l_sym_names / sizeof l_sym_names[0]; ++i) {
    dlerror();
    l_sym_addrs[i] = dlsym(l_handle, l_sym_names[i]);
    err = dlerror();
    if (err != NULL) {
      yf_wsi_wayland_unload();
      yf_seterr(YF_ERR_DEVGEN, __func__);
      return -1;
    }
  }
  return 0;
}

void *yf_wsi_wayland_init(YF_window win) {
  assert(l_handle != NULL);
  assert(win != NULL);
  if (l_vars.display == NULL && init_vars() != 0)
    return NULL;
  if (l_data_list == NULL && (l_data_list = yf_list_init(NULL)) == NULL)
    return NULL;
  if (vkCreateWaylandSurfaceKHR == NULL &&
    (YF_IPROCVK(win->ctx->instance, vkCreateWaylandSurfaceKHR)) == NULL)
  {
    yf_seterr(YF_ERR_UNSUP, __func__);
    return NULL;
  }
  return init_data(win);
}

int yf_wsi_wayland_poll(void) {
  assert(l_vars.display != NULL);
  int res;
  YF_WL_DISPLAY_DISPATCH_PENDING(res, l_vars.display);
  if (res == -1)
    yf_seterr(YF_ERR_DEVGEN, __func__);
  else
    res = l_vars.event_n;
  l_vars.event_n = 0;
  return res;
}

int yf_wsi_wayland_resize(void *data) {
  assert(data != NULL);
  return 0;
}

int yf_wsi_wayland_close(void *data) {
  assert(data != NULL);
  L_data *dt = (L_data *)data;
  int r = 0;
  switch (dt->dsh) {
    case YF_DSH_CORE:
      /* XXX: It doesn't seem possible to safely unmap a shell surface -
         the underlying wl surface would have to be destroyed, and doing
         so would invalidate the vulkan surface. */
      yf_seterr(YF_ERR_UNSUP, __func__);
      r = -1;
      break;
    case YF_DSH_XDG:
      if (dt->xdg_shell.toplevel != NULL) {
        YF_WL_PROXY_MARSHAL(
          dt->xdg_shell.toplevel,
          YF_XDG_TOPLEVEL_DESTROY);
        YF_WL_PROXY_DESTROY(dt->xdg_shell.toplevel);
        dt->xdg_shell.toplevel = NULL;
      }
      if (dt->xdg_shell.surface != NULL) {
        YF_WL_PROXY_MARSHAL(
          dt->xdg_shell.surface,
          YF_XDG_SURFACE_DESTROY);
        YF_WL_PROXY_DESTROY(dt->xdg_shell.surface);
        dt->xdg_shell.surface = NULL;
      }
      break;
  }
  return r;
}

void yf_wsi_wayland_deinit(void *data) {
  if (data != NULL) {
    L_data *dt = (L_data *)data;
    switch (dt->dsh) {
      case YF_DSH_CORE:
        if (dt->core_shell.surface != NULL)
          YF_WL_PROXY_DESTROY(dt->core_shell.surface);
        break;
      case YF_DSH_XDG:
        if (dt->xdg_shell.toplevel != NULL) {
          YF_WL_PROXY_MARSHAL(
            dt->xdg_shell.toplevel,
            YF_XDG_TOPLEVEL_DESTROY);
          YF_WL_PROXY_DESTROY(dt->xdg_shell.toplevel);
        }
        if (dt->xdg_shell.surface != NULL) {
          YF_WL_PROXY_MARSHAL(
            dt->xdg_shell.surface,
            YF_XDG_SURFACE_DESTROY);
          YF_WL_PROXY_DESTROY(dt->xdg_shell.surface);
        }
        break;
    }
    if (dt->surface != NULL) {
      YF_WL_PROXY_MARSHAL(dt->surface, YF_WL_SURFACE_DESTROY);
      YF_WL_PROXY_DESTROY(dt->surface);
    }
    yf_list_remove(l_data_list, data);
    if (yf_list_getlen(l_data_list) == 0)
      deinit_vars();
    free(data);
  }
}

void yf_wsi_wayland_unload(void) {
  if (l_handle != NULL) {
    dlclose(l_handle);
    l_handle = NULL;
    vkCreateWaylandSurfaceKHR = NULL;
  }
}

static void registry_global(
  void YF__UNUSED *data,
  struct wl_proxy *registry,
  uint32_t name,
  const char *interface,
  uint32_t version)
{
  static const char *compositor = "wl_compositor";
  static const char *shell = "wl_shell";
  static const char *seat = "wl_seat";
  static const char *wm_base = "xdg_wm_base";

  struct wl_proxy **proxy = NULL;
  const struct wl_interface *iface = NULL;
  void (**listener)(void) = NULL;
  if (strncmp(interface, compositor, strlen(compositor)) == 0) {
    proxy = &l_vars.compositor;
    iface = &wl_compositor_interface;
  } else if (strncmp(interface, shell, strlen(shell)) == 0) {
    if (l_vars.dsh == YF_DSH_UNDEF) {
      proxy = &l_vars.core_shell.shell;
      iface = &wl_shell_interface;
      l_vars.dsh = YF_DSH_CORE;
    }
  } else if (strncmp(interface, seat, strlen(seat)) == 0) {
    proxy = &l_vars.seat;
    iface = &wl_seat_interface;
    listener = (void (**)(void))l_seat_listener;
  } else if (strncmp(interface, wm_base, strlen(wm_base)) == 0) {
    if (l_vars.dsh != YF_DSH_XDG) {
      if (l_vars.dsh == YF_DSH_CORE)
        YF_WL_PROXY_DESTROY(l_vars.core_shell.shell);
      proxy = &l_vars.xdg_shell.wm_base;
      iface = &xdg_wm_base_interface;
      listener = (void (**)(void))l_wm_base_listener;
      l_vars.dsh = YF_DSH_XDG;
    } else {
      assert(0);
    }
  }
  if (proxy != NULL) {
    YF_WL_PROXY_MARSHAL_CONSTRUCTOR_VERSIONED(
      *proxy,
      registry,
      YF_WL_REGISTRY_BIND,
      iface,
      version,
      name,
      iface->name,
      version,
      NULL);
    if (listener != NULL) {
      int res;
      YF_WL_PROXY_ADD_LISTENER(res, *proxy, listener, NULL);
      assert(res != -1);
    }
  }
}

static void registry_global_remove(
  void *data,
  struct wl_proxy *registry,
  uint32_t name)
{
  /* TODO */
}

static void shell_surface_ping(
  void YF__UNUSED *data,
  struct wl_proxy *shell_surface,
  uint32_t serial)
{
  YF_WL_PROXY_MARSHAL(shell_surface, YF_WL_SHELL_SURFACE_PONG, serial);
}

static void shell_surface_configure(
  void *data,
  struct wl_proxy *shell_surface,
  uint32_t edges,
  int32_t width,
  int32_t height)
{
  /* TODO */
  /* XXX: The `data` argument is the L_data object. */
}

static void shell_surface_popup_done(
  void YF__UNUSED *data,
  struct wl_proxy YF__UNUSED *shell_surface)
{
  /* XXX: Unused. */
}

static void seat_capabilities(
  void YF__UNUSED *data,
  struct wl_proxy *seat,
  uint32_t capabilities)
{
#define YF_WL_SEAT_CAPABILITY_POINTER  0x01
#define YF_WL_SEAT_CAPABILITY_KEYBOARD 0x02
/* #define YF_WL_SEAT_CAPABILITY_TOUCH    0x04 */
  int res;
  if (capabilities & YF_WL_SEAT_CAPABILITY_POINTER) {
    if (l_vars.pointer != NULL) {
      YF_WL_PROXY_MARSHAL(l_vars.pointer, YF_WL_POINTER_RELEASE);
      YF_WL_PROXY_DESTROY(l_vars.pointer);
    }
    YF_WL_PROXY_MARSHAL_CONSTRUCTOR(
      l_vars.pointer,
      seat,
      YF_WL_SEAT_GET_POINTER,
      &wl_pointer_interface,
      NULL);
    YF_WL_PROXY_ADD_LISTENER(
      res,
      l_vars.pointer,
      (void (**)(void))l_pointer_listener,
      NULL);
    assert(res != -1);
  } else if (l_vars.pointer != NULL) {
    YF_WL_PROXY_MARSHAL(l_vars.pointer, YF_WL_POINTER_RELEASE);
    YF_WL_PROXY_DESTROY(l_vars.pointer);
    l_vars.pointer = NULL;
  }
  if (capabilities & YF_WL_SEAT_CAPABILITY_KEYBOARD) {
    if (l_vars.keyboard != NULL) {
      YF_WL_PROXY_MARSHAL(l_vars.keyboard, YF_WL_KEYBOARD_RELEASE);
      YF_WL_PROXY_DESTROY(l_vars.keyboard);
    }
    YF_WL_PROXY_MARSHAL_CONSTRUCTOR(
      l_vars.keyboard,
      seat,
      YF_WL_SEAT_GET_KEYBOARD,
      &wl_keyboard_interface,
      NULL);
    YF_WL_PROXY_ADD_LISTENER(
      res,
      l_vars.keyboard,
      (void (**)(void))l_keyboard_listener,
      NULL);
    assert(res != -1);
  } else if (l_vars.keyboard != NULL) {
    YF_WL_PROXY_MARSHAL(l_vars.keyboard, YF_WL_KEYBOARD_RELEASE);
    YF_WL_PROXY_DESTROY(l_vars.keyboard);
    l_vars.keyboard = NULL;
  }
}

static void seat_name(
  void YF__UNUSED *data,
  struct wl_proxy YF__UNUSED *seat,
  const char YF__UNUSED *name)
{
  /* XXX: Unused. */
}

static void pointer_enter(
  void YF__UNUSED *data,
  struct wl_proxy YF__UNUSED *pointer,
  uint32_t YF__UNUSED serial,
  struct wl_proxy *surface,
  wl_fixed_t surface_x,
  wl_fixed_t surface_y)
{
  YF_coord2 coord = {
    .x = wl_fixed_to_double(surface_x),
    .y = wl_fixed_to_double(surface_y)
  };
  yf_pointer_setcoord(coord);
  L_data *d = get_data(NULL, surface);
  if (d != NULL) {
    YF_eventfn fn;
    void *dt;
    yf_event_getfn(YF_EVENT_PT_ENTER, &fn, &dt);
    if (fn.pt_enter != NULL) {
      fn.pt_enter(d->win, coord, dt);
      l_vars.event_n++;
    }
  } else {
    assert(0);
  }
}

static void pointer_leave(
  void YF__UNUSED *data,
  struct wl_proxy YF__UNUSED *pointer,
  uint32_t YF__UNUSED serial,
  struct wl_proxy *surface)
{
  yf_pointer_invalidate();
  L_data *d = get_data(NULL, surface);
  if (d != NULL) {
    YF_eventfn fn;
    void *dt;
    yf_event_getfn(YF_EVENT_PT_LEAVE, &fn, &dt);
    if (fn.pt_leave != NULL) {
      fn.pt_leave(d->win, dt);
      l_vars.event_n++;
    }
  } else {
    assert(0);
  }
}

static void pointer_motion(
  void YF__UNUSED *data,
  struct wl_proxy YF__UNUSED *pointer,
  uint32_t YF__UNUSED time,
  wl_fixed_t surface_x,
  wl_fixed_t surface_y)
{
  YF_coord2 coord = {
    .x = wl_fixed_to_double(surface_x),
    .y = wl_fixed_to_double(surface_y)
  };
  yf_pointer_setcoord(coord);
  YF_eventfn fn;
  void *dt;
  yf_event_getfn(YF_EVENT_PT_MOTION, &fn, &dt);
  if (fn.pt_motion != NULL) {
    fn.pt_motion(coord, dt);
    l_vars.event_n++;
  }
}

static void pointer_button(
  void YF__UNUSED *data,
  struct wl_proxy YF__UNUSED *pointer,
  uint32_t YF__UNUSED serial,
  uint32_t YF__UNUSED time,
  uint32_t button,
  uint32_t state)
{
#define YF_WL_POINTER_BUTTON_STATE_RELEASED 0
#define YF_WL_POINTER_BUTTON_STATE_PRESSED  1
  YF_eventfn fn;
  void *dt;
  yf_event_getfn(YF_EVENT_PT_BUTTON, &fn, &dt);
  if (fn.pt_button != NULL) {
    int btn;
    switch (button) {
      case BTN_LEFT:
        btn = YF_BTN_LEFT;
        break;
      case BTN_RIGHT:
        btn = YF_BTN_RIGHT;
        break;
      case BTN_MIDDLE:
        btn = YF_BTN_MIDDLE;
        break;
      case BTN_SIDE:
        btn = YF_BTN_SIDE;
        break;
      case BTN_FORWARD:
        btn = YF_BTN_FORWARD;
        break;
      case BTN_BACK:
        btn = YF_BTN_BACKWARD;
        break;
      default:
        btn = YF_BTN_UNKNOWN;
        break;
    }
    int st;
    if (state == YF_WL_POINTER_BUTTON_STATE_RELEASED)
      st = YF_BTNSTATE_RELEASED;
    else
      st = YF_BTNSTATE_PRESSED;
    fn.pt_button(btn, st, dt);
    l_vars.event_n++;
  }
}

static void pointer_axis(
  void *data,
  struct wl_proxy *pointer,
  uint32_t time,
  uint32_t axis,
  wl_fixed_t value)
{
  /* TODO */
}

static void pointer_frame(void *data, struct wl_proxy *pointer) {
  /* TODO */
}

static void pointer_axis_source(
  void *data,
  struct wl_proxy *pointer,
  uint32_t axis_source)
{
  /* TODO */
}

static void pointer_axis_stop(
  void *data,
  struct wl_proxy *pointer,
  uint32_t time,
  uint32_t axis)
{
  /* TODO */
}

static void pointer_axis_discrete(
  void *data,
  struct wl_proxy *pointer,
  uint32_t axis,
  int32_t discrete)
{
  /* TODO */
}

static void keyboard_keymap(
  void *data,
  struct wl_proxy *keyboard,
  uint32_t format,
  int32_t fd,
  uint32_t size)
{
  /* TODO */
}

static void keyboard_enter(
  void YF__UNUSED *data,
  struct wl_proxy YF__UNUSED *keyboard,
  uint32_t YF__UNUSED serial,
  struct wl_proxy *surface,
  struct wl_array YF__UNUSED *keys)
{
  L_data *d = get_data(NULL, surface);
  if (d != NULL) {
    YF_eventfn fn;
    void *dt;
    yf_event_getfn(YF_EVENT_KB_ENTER, &fn, &dt);
    if (fn.kb_enter != NULL) {
      fn.kb_enter(d->win, dt);
      l_vars.event_n++;
    }
  } else {
    assert(0);
  }
}

static void keyboard_leave(
  void YF__UNUSED *data,
  struct wl_proxy YF__UNUSED *keyboard,
  uint32_t YF__UNUSED serial,
  struct wl_proxy *surface)
{
  L_data *d = get_data(NULL, surface);
  if (d != NULL) {
    YF_eventfn fn;
    void *dt;
    yf_event_getfn(YF_EVENT_KB_LEAVE, &fn, &dt);
    if (fn.kb_leave != NULL) {
      fn.kb_leave(d->win, dt);
      l_vars.event_n++;
    }
  } else {
    assert(0);
  }
}

static void keyboard_key(
  void YF__UNUSED *data,
  struct wl_proxy YF__UNUSED *keyboard,
  uint32_t YF__UNUSED serial,
  uint32_t YF__UNUSED time,
  uint32_t key,
  uint32_t state)
{
#define YF_WL_KEYBOARD_KEY_STATE_RELEASED 0
#define YF_WL_KEYBOARD_KEY_STATE_PRESSED  1
  YF_eventfn fn;
  void *dt;
  yf_event_getfn(YF_EVENT_KB_KEY, &fn, &dt);
  if (fn.kb_key != NULL) {
    int kc = yf_keyboard_convert(key);
    int st;
    if (state == YF_WL_KEYBOARD_KEY_STATE_RELEASED)
      st = YF_KEYSTATE_RELEASED;
    else
      st = YF_KEYSTATE_PRESSED;
    fn.kb_key(kc, st, l_vars.mod_mask, dt);
    l_vars.event_n++;
  }
}

static void keyboard_modifiers(
  void YF__UNUSED *data,
  struct wl_proxy YF__UNUSED *keyboard,
  uint32_t YF__UNUSED serial,
  uint32_t mods_depressed,
  uint32_t YF__UNUSED mods_latched,
  uint32_t mods_locked,
  uint32_t YF__UNUSED group)
{
#define YF_COMMOD_SHIFT    0x01
#define YF_COMMOD_CAPSLOCK 0x02
#define YF_COMMOD_CTRL     0x04
#define YF_COMMOD_ALT      0x08
  unsigned mod_mask = 0;
  if ((mods_depressed | mods_locked) & YF_COMMOD_CAPSLOCK)
    mod_mask |= YF_KEYMOD_CAPSLOCK;
  if (mods_depressed & YF_COMMOD_SHIFT)
    mod_mask |= YF_KEYMOD_SHIFT;
  if (mods_depressed & YF_COMMOD_CTRL)
    mod_mask |= YF_KEYMOD_CTRL;
  if (mods_depressed & YF_COMMOD_ALT)
    mod_mask |= YF_KEYMOD_ALT;
  l_vars.mod_mask = mod_mask;
}

static void keyboard_repeat_info(
  void *data,
  struct wl_proxy *keyboard,
  int32_t rate,
  int32_t delay)
{
  /* TODO */
}

static void wm_base_ping(
  void YF__UNUSED *data,
  struct wl_proxy *wm_base,
  uint32_t serial)
{
  YF_WL_PROXY_MARSHAL(wm_base, YF_XDG_WM_BASE_PONG, serial);
}

static void xdg_surface_configure(
  void *data,
  struct wl_proxy *xdg_surface,
  uint32_t serial)
{
  /* TODO: Handle configuration requests sent to roles before replying. */
  /* XXX: The `data` argument is the L_data object. */
  YF_WL_PROXY_MARSHAL(xdg_surface, YF_XDG_SURFACE_ACK_CONFIGURE, serial);
}

static void toplevel_configure(
  void *data,
  struct wl_proxy *toplevel,
  int32_t width,
  int32_t height,
  struct wl_array *states)
{
  /* TODO */
  /* XXX: The `data` argument is the L_data object. */
}

static void toplevel_close(void *data, struct wl_proxy YF__UNUSED *toplevel) {
  /* XXX: The `data` argument is the L_data object. */
  YF_eventfn fn;
  void *dt;
  yf_event_getfn(YF_EVENT_WD_CLOSE, &fn, &dt);
  if (fn.wd_close != NULL) {
    fn.wd_close(((L_data *)data)->win, dt);
    l_vars.event_n++;
  }
}

static int init_vars(void) {
  int res;

  YF_WL_DISPLAY_CONNECT(l_vars.display, NULL);
  if (l_vars.display == NULL)
    goto on_error;

  YF_WL_PROXY_MARSHAL_CONSTRUCTOR(
    l_vars.registry,
    (struct wl_proxy *)l_vars.display,
    YF_WL_DISPLAY_GET_REGISTRY,
    &wl_registry_interface,
    NULL);
  if (l_vars.registry == NULL)
    goto on_error;

  YF_WL_PROXY_ADD_LISTENER(
    res,
    l_vars.registry,
    (void (**)(void))l_registry_listener,
    NULL);
  if (res == -1)
    goto on_error;

  YF_WL_DISPLAY_ROUNDTRIP(res, l_vars.display);
  if (res == -1 || l_vars.compositor == NULL)
    goto on_error;

  return 0;

  on_error:
  deinit_vars();
  yf_seterr(YF_ERR_DEVGEN, __func__);
  return -1;
}

static void *init_data(YF_window win) {
  L_data *data = calloc(1, sizeof(L_data));
  if (data == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  data->win = win;

  int res;
  VkResult rv;

  YF_WL_PROXY_MARSHAL_CONSTRUCTOR(
    data->surface,
    l_vars.compositor,
    YF_WL_COMPOSITOR_CREATE_SURFACE,
    &wl_surface_interface,
    NULL);
  if (data->surface == NULL)
    goto on_error;

  switch (l_vars.dsh) {
    case YF_DSH_CORE:
      data->dsh = YF_DSH_CORE;
      YF_WL_PROXY_MARSHAL_CONSTRUCTOR(
        data->core_shell.surface,
        l_vars.core_shell.shell,
        YF_WL_SHELL_GET_SHELL_SURFACE,
        &wl_shell_surface_interface,
        NULL,
        data->surface);
      if (data->core_shell.surface == NULL)
        goto on_error;
      YF_WL_PROXY_ADD_LISTENER(
        res,
        data->core_shell.surface,
        (void (**)(void))l_shell_surface_listener,
        data);
      if (res == -1)
        goto on_error;
      YF_WL_PROXY_MARSHAL(
        data->core_shell.surface,
        YF_WL_SHELL_SURFACE_SET_TOPLEVEL);
      YF_WL_PROXY_MARSHAL(
        data->core_shell.surface,
        YF_WL_SHELL_SURFACE_SET_TITLE,
        win->title == NULL ? "" : win->title);
      YF_WL_PROXY_MARSHAL(
        data->core_shell.surface,
        YF_WL_SHELL_SURFACE_SET_CLASS,
        YF_APP_ID);
      break;

    case YF_DSH_XDG:
      data->dsh = YF_DSH_XDG;
      YF_WL_PROXY_MARSHAL_CONSTRUCTOR(
        data->xdg_shell.surface,
        l_vars.xdg_shell.wm_base,
        YF_XDG_WM_BASE_GET_XDG_SURFACE,
        &xdg_surface_interface,
        NULL,
        data->surface);
      if (data->xdg_shell.surface == NULL)
        goto on_error;
      YF_WL_PROXY_ADD_LISTENER(
        res,
        data->xdg_shell.surface,
        (void (**)(void))l_xdg_surface_listener,
        data);
      if (res == -1)
        goto on_error;
      YF_WL_PROXY_MARSHAL_CONSTRUCTOR(
        data->xdg_shell.toplevel,
        data->xdg_shell.surface,
        YF_XDG_SURFACE_GET_TOPLEVEL,
        &xdg_toplevel_interface,
        NULL);
      if (data->xdg_shell.toplevel == NULL)
        goto on_error;
      YF_WL_PROXY_ADD_LISTENER(
        res,
        data->xdg_shell.toplevel,
        (void (**)(void))l_toplevel_listener,
        data);
      if (res == -1)
        goto on_error;
      YF_WL_PROXY_MARSHAL(
        data->xdg_shell.toplevel,
        YF_XDG_TOPLEVEL_SET_TITLE,
        win->title == NULL ? "" : win->title);
      YF_WL_PROXY_MARSHAL(
        data->xdg_shell.toplevel,
        YF_XDG_TOPLEVEL_SET_APP_ID,
        YF_APP_ID);
      YF_WL_PROXY_MARSHAL(data->surface, YF_WL_SURFACE_COMMIT);
      break;

    default:
      goto on_error;
  }

  YF_WL_DISPLAY_ROUNDTRIP(res, l_vars.display);
  if (res == -1)
    goto on_error;

  VkWaylandSurfaceCreateInfoKHR info = {
    .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
    .pNext = NULL,
    .flags = 0,
    .display = l_vars.display,
    .surface = (struct wl_surface *)data->surface
  };
  rv = vkCreateWaylandSurfaceKHR(
    win->ctx->instance,
    &info,
    NULL,
    &win->surface);
  if (rv != VK_SUCCESS)
    goto on_error;

  if (yf_list_insert(l_data_list, data) != 0)
    goto on_error;

  return data;

  on_error:
  yf_wsi_wayland_deinit(data);
  yf_seterr(YF_ERR_DEVGEN, __func__);
  return NULL;
}

static L_data *get_data(YF_window win, struct wl_proxy *surface) {
  L_data *data = NULL;
  YF_iter it = YF_NILIT;
  do {
    data = yf_list_next(l_data_list, &it);
    if (YF_IT_ISNIL(it) || (data->win == win || data->surface == surface))
      break;
  } while (1);
  return data;
}

static void deinit_vars(void) {
  switch (l_vars.dsh) {
    case YF_DSH_CORE:
      if (l_vars.core_shell.shell != NULL)
        YF_WL_PROXY_DESTROY(l_vars.core_shell.shell);
      break;
    case YF_DSH_XDG:
      if (l_vars.xdg_shell.wm_base != NULL) {
        YF_WL_PROXY_MARSHAL(
          l_vars.xdg_shell.wm_base,
          YF_XDG_WM_BASE_DESTROY);
        YF_WL_PROXY_DESTROY(l_vars.xdg_shell.wm_base);
      }
      break;
  }
  if (l_vars.keyboard != NULL) {
    YF_WL_PROXY_MARSHAL(l_vars.keyboard, YF_WL_KEYBOARD_RELEASE);
    YF_WL_PROXY_DESTROY(l_vars.keyboard);
  }
  if (l_vars.pointer != NULL) {
    YF_WL_PROXY_MARSHAL(l_vars.pointer, YF_WL_POINTER_RELEASE);
    YF_WL_PROXY_DESTROY(l_vars.pointer);
  }
  if (l_vars.seat != NULL) {
    YF_WL_PROXY_MARSHAL(l_vars.seat, YF_WL_SEAT_RELEASE);
    YF_WL_PROXY_DESTROY(l_vars.seat);
  }
  if (l_vars.compositor != NULL)
    YF_WL_PROXY_DESTROY(l_vars.compositor);
  if (l_vars.registry != NULL)
    YF_WL_PROXY_DESTROY(l_vars.registry);
  if (l_vars.display != NULL)
    YF_WL_DISPLAY_DISCONNECT(l_vars.display);
  memset(&l_vars, 0, sizeof l_vars);
}
