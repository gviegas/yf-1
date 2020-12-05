/*
 * YF
 * wsi-xcb.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <assert.h>

#include <xcb/xcb.h>
#include "vk.h"
#include <vulkan/vulkan_xcb.h>

#include "wsi-xcb.h"
#include "context.h"
#include "window.h"
#include "event.h"
#include "pointer.h"
#include "keyboard.h"
#include "list.h"
#include "error.h"

#ifndef YF__UNUSED
# ifdef __GNUC__
#  define YF__UNUSED __attribute__ ((unused))
# else
#  define YF__UNUSED
# endif
#endif

#ifndef YF_APP_ID
# define YF_APP_ID "unknown.app.id"
#endif
#ifndef YF_APP
# define YF_APP "Unknown app"
#endif
#undef YF_STR_MAXLEN
#define YF_STR_MAXLEN 60

/* Function pointer for surface creation. */
static YF_DEFVK(vkCreateXcbSurfaceKHR);

/* Variables used by all wsi-xcb objects. */
typedef struct {
  xcb_connection_t *connection;
  xcb_atom_t protocol_atom;
  xcb_atom_t delete_atom;
  xcb_atom_t title_atom;
  xcb_atom_t utf8_atom;
  xcb_atom_t class_atom;
} L_vars;

/* Data of a wsi-xcb object. */
typedef struct {
  YF_window win;
  xcb_window_t window;
} L_data;

/* Variables' instance. */
static L_vars l_vars = {0};

/* List of data objects created. */
static YF_list l_data_list = NULL;

/* Symbol names. */
static const char *l_sym_names[] = {
#define YF_SYM_CONNECT 0
  "xcb_connect",
#define YF_SYM_DISCONNECT 1
  "xcb_disconnect",
#define YF_SYM_FLUSH 2
  "xcb_flush",
#define YF_SYM_CONNECTION_HAS_ERROR 3
  "xcb_connection_has_error",
#define YF_SYM_GENERATE_ID 4
  "xcb_generate_id",
#define YF_SYM_POLL_FOR_EVENT 5
  "xcb_poll_for_event",
#define YF_SYM_REQUEST_CHECK 6
  "xcb_request_check",
#define YF_SYM_GET_SETUP 7
  "xcb_get_setup",
#define YF_SYM_SETUP_ROOTS_ITERATOR 8
  "xcb_setup_roots_iterator",
#define YF_SYM_CREATE_WINDOW_CHECKED 9
  "xcb_create_window_checked",
#define YF_SYM_DESTROY_WINDOW 10
  "xcb_destroy_window",
#define YF_SYM_MAP_WINDOW_CHECKED 11
  "xcb_map_window_checked",
#define YF_SYM_UNMAP_WINDOW_CHECKED 12
  "xcb_unmap_window_checked",
#define YF_SYM_CONFIGURE_WINDOW_CHECKED 13
  "xcb_configure_window_checked",
#define YF_SYM_INTERN_ATOM 14
  "xcb_intern_atom",
#define YF_SYM_INTERN_ATOM_REPLY 15
  "xcb_intern_atom_reply",
#define YF_SYM_CHANGE_PROPERTY_CHECKED 16
  "xcb_change_property_checked",
#define YF_SYM_CHANGE_KEYBOARD_CONTROL_CHECKED 17
  "xcb_change_keyboard_control_checked"
};

/* Symbol addresses. */
static void *l_sym_addrs[sizeof l_sym_names / sizeof l_sym_names[0]] = {0};

/* Shared object handle. */
static void *l_handle = NULL;

/* Wrappers. */
#define YF_XCB_CONNECT(res, name, screen) do { \
  xcb_connection_t *(*fn)(const char *, int *); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_CONNECT]; \
  res = fn(name, screen); } while (0)

#define YF_XCB_DISCONNECT(connection) do { \
  void (*fn)(xcb_connection_t *); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_DISCONNECT]; \
  fn(connection); } while (0)

#define YF_XCB_FLUSH(res, connection) do { \
  int (*fn)(xcb_connection_t *); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_FLUSH]; \
  res = fn(connection); } while (0)

#define YF_XCB_CONNECTION_HAS_ERROR(res, connection) do { \
  int (*fn)(xcb_connection_t *); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_CONNECTION_HAS_ERROR]; \
  res = fn(connection); } while (0)

#define YF_XCB_GENERATE_ID(res, connection) do { \
  uint32_t (*fn)(xcb_connection_t *); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_GENERATE_ID]; \
  res = fn(connection); } while (0)

#define YF_XCB_POLL_FOR_EVENT(res, connection) do { \
  xcb_generic_event_t *(*fn)(xcb_connection_t *); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_POLL_FOR_EVENT]; \
  res = fn(connection); } while (0)

#define YF_XCB_REQUEST_CHECK(res, connection, cookie) do { \
  xcb_generic_error_t *(*fn)(xcb_connection_t *, xcb_void_cookie_t); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_REQUEST_CHECK]; \
  res = fn(connection, cookie); } while (0)

#define YF_XCB_GET_SETUP(res, connection) do { \
  const struct xcb_setup_t *(*fn)(xcb_connection_t *); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_GET_SETUP]; \
  res = fn(connection); } while (0)

#define YF_XCB_SETUP_ROOTS_ITERATOR(res, setup) do { \
  xcb_screen_iterator_t (*fn)(const xcb_setup_t *); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_SETUP_ROOTS_ITERATOR]; \
  res = fn(setup); } while (0)

#define YF_XCB_CREATE_WINDOW_CHECKED(res, connection, depth, id, parent, \
x, y, w, h, border_w, class, visual, value_mask, value_list) do { \
  xcb_void_cookie_t (*fn)( \
    xcb_connection_t *, uint8_t, xcb_window_t, xcb_window_t, \
    int16_t, int16_t, uint16_t, uint16_t, uint16_t, uint16_t, \
    xcb_visualid_t, uint32_t, const void *); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_CREATE_WINDOW_CHECKED]; \
  res = fn( \
    connection, depth, id, parent, \
    x, y, w, h, border_w, class, \
    visual, value_mask, value_list); } while (0)

#define YF_XCB_DESTROY_WINDOW(res, connection, window) do { \
  xcb_void_cookie_t (*fn)(xcb_connection_t *, xcb_window_t); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_DESTROY_WINDOW]; \
  res = fn(connection, window); } while (0)

#define YF_XCB_MAP_WINDOW_CHECKED(res, connection, window) do { \
  xcb_void_cookie_t (*fn)(xcb_connection_t *, xcb_window_t); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_MAP_WINDOW_CHECKED]; \
  res = fn(connection, window); } while (0)

#define YF_XCB_UNMAP_WINDOW_CHECKED(res, connection, window) do { \
  xcb_void_cookie_t (*fn)(xcb_connection_t *, xcb_window_t); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_UNMAP_WINDOW_CHECKED]; \
  res = fn(connection, window); } while (0)

#define YF_XCB_CONFIGURE_WINDOW_CHECKED( \
res, connection, window, value_mask, value_list) do { \
  xcb_void_cookie_t (*fn)( \
    xcb_connection_t *, xcb_window_t, uint32_t, const void *); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_CONFIGURE_WINDOW_CHECKED]; \
  res = fn(connection, window, value_mask, value_list); } while (0)

#define YF_XCB_INTERN_ATOM(res, connection, dont_create, name_len, name) do { \
  xcb_intern_atom_cookie_t (*fn)( \
    xcb_connection_t *, uint8_t, uint16_t, const char *); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_INTERN_ATOM]; \
  res = fn(connection, dont_create, name_len, name); } while (0)

#define YF_XCB_INTERN_ATOM_REPLY(res, connection, cookie, error) do { \
  xcb_intern_atom_reply_t *(*fn)( \
    xcb_connection_t *, xcb_intern_atom_cookie_t, xcb_generic_error_t **); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_INTERN_ATOM_REPLY]; \
  res = fn(connection, cookie, error); } while (0)

#define YF_XCB_CHANGE_PROPERTY_CHECKED( \
res, connection, mode, window, property, type, format, data_len, data) do { \
  xcb_void_cookie_t (*fn)( \
    xcb_connection_t *, uint8_t, xcb_window_t, xcb_atom_t, \
    xcb_atom_t, uint8_t, uint32_t, const void *); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_CHANGE_PROPERTY_CHECKED]; \
  res = fn(connection, mode, window, property, type, format, data_len, data); \
  } while (0)

#define YF_XCB_CHANGE_KEYBOARD_CONTROL_CHECKED( \
res, connection, value_mask, value_list) do { \
  xcb_void_cookie_t (*fn)(xcb_connection_t *, uint32_t, const void *); \
  *(void **)(&fn) = l_sym_addrs[YF_SYM_CHANGE_KEYBOARD_CONTROL_CHECKED]; \
  res = fn(connection, value_mask, value_list); } while (0)

/* Initializes the variables' instance. */
static int init_vars(void);

/* Initializes a new data object. */
static void *init_data(YF_window win);

/* Gets the data object that contains at least one of the provided values. */
static L_data *get_data(YF_window win, xcb_window_t window);

int yf_wsi_xcb_load(void) {
  if (l_handle != NULL)
    return 0;
  l_handle = dlopen("libxcb.so", RTLD_LAZY);
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
      yf_wsi_xcb_unload();
      yf_seterr(YF_ERR_DEVGEN, __func__);
      return -1;
    }
  }
  return 0;
}

void *yf_wsi_xcb_init(YF_window win) {
  assert(l_handle != NULL);
  assert(win != NULL);
  if (l_vars.connection == NULL && init_vars() != 0)
    return NULL;
  if (l_data_list == NULL && (l_data_list = yf_list_init(NULL)) == NULL)
    return NULL;
  if (vkCreateXcbSurfaceKHR == NULL &&
    (YF_IPROCVK(win->ctx->instance, vkCreateXcbSurfaceKHR)) == NULL)
  {
    yf_seterr(YF_ERR_UNSUP, __func__);
    return NULL;
  }
  return init_data(win);
}

int yf_wsi_xcb_poll(void) {
  assert(l_vars.connection != NULL);
  xcb_generic_event_t *event = NULL;
  int n = 0;
  unsigned type;
  do {
    YF_XCB_POLL_FOR_EVENT(event, l_vars.connection);
    if (event == NULL)
      break;
    type = event->response_type & ~0x80;
    switch (type) {
      case XCB_KEY_PRESS:
      case XCB_KEY_RELEASE: {
        xcb_key_release_event_t *ev = (xcb_key_release_event_t *)event;
        int key = yf_keyboard_convert(ev->detail - 8);
        YF_eventfn fn;
        void *dt;
        yf_event_getfn(YF_EVENT_KB_KEY, &fn, &dt);
        if (fn.kb_key != NULL) {
          int state;
          if (type == XCB_KEY_RELEASE)
            state = YF_KEYSTATE_RELEASED;
          else
            state = YF_KEYSTATE_PRESSED;
          static uint16_t prev_ev_state = 0;
          static unsigned prev_mod_mask = 0;
          unsigned mod_mask;
          if (ev->state != prev_ev_state) {
            mod_mask = 0;
            if (ev->state & XCB_MOD_MASK_LOCK)
              mod_mask |= YF_KEYMOD_CAPSLOCK;
            if (ev->state & XCB_MOD_MASK_SHIFT)
              mod_mask |= YF_KEYMOD_SHIFT;
            if (ev->state & XCB_MOD_MASK_CONTROL)
              mod_mask |= YF_KEYMOD_CTRL;
            if (ev->state & XCB_MOD_MASK_1)
              mod_mask |= YF_KEYMOD_ALT;
            prev_mod_mask = mod_mask;
            prev_ev_state = ev->state;
          } else {
            mod_mask = prev_mod_mask;
          }
          fn.kb_key(key, state, mod_mask, dt);
          ++n;
        }
      } break;

      case XCB_BUTTON_PRESS:
      case XCB_BUTTON_RELEASE: {
        xcb_button_release_event_t *ev = (xcb_button_release_event_t *)event;
        YF_eventfn fn;
        void *dt;
        yf_event_getfn(YF_EVENT_PT_BUTTON, &fn, &dt);
        if (fn.pt_button != NULL) {
          int btn;
          switch (ev->detail) {
            case XCB_BUTTON_INDEX_1:
              btn = YF_BTN_LEFT;
              break;
            case XCB_BUTTON_INDEX_2:
              btn = YF_BTN_MIDDLE;
              break;
            case XCB_BUTTON_INDEX_3:
              btn = YF_BTN_RIGHT;
              break;
            case XCB_BUTTON_INDEX_4:
            case XCB_BUTTON_INDEX_5:
              /* TODO: Scroll. */
            default:
              btn = YF_BTN_UNKNOWN;
          }
          int state;
          if (type == XCB_BUTTON_RELEASE)
            state = YF_BTNSTATE_RELEASED;
          else
            state = YF_BTNSTATE_PRESSED;
          fn.pt_button(btn, state, dt);
          ++n;
        }
      } break;

      case XCB_MOTION_NOTIFY: {
        xcb_motion_notify_event_t *ev = (xcb_motion_notify_event_t *)event;
        YF_coord2 coord = {ev->event_x, ev->event_y};
        yf_pointer_setcoord(coord);
        YF_eventfn fn;
        void *dt;
        yf_event_getfn(YF_EVENT_PT_MOTION, &fn, &dt);
        if (fn.pt_motion != NULL) {
          fn.pt_motion(coord, dt);
          ++n;
        }
      } break;

      case XCB_ENTER_NOTIFY: {
        xcb_enter_notify_event_t *ev = (xcb_enter_notify_event_t *)event;
        YF_coord2 coord = {ev->event_x, ev->event_y};
        yf_pointer_setcoord(coord);
        L_data *data = get_data(NULL, ev->event);
        if (data != NULL) {
          YF_eventfn fn;
          void *dt;
          yf_event_getfn(YF_EVENT_PT_ENTER, &fn, &dt);
          if (fn.pt_enter != NULL) {
            fn.pt_enter(data->win, coord, dt);
            ++n;
          }
        } else {
          assert(0);
        }
      } break;

      case XCB_LEAVE_NOTIFY: {
        xcb_leave_notify_event_t *ev = (xcb_leave_notify_event_t *)event;
        yf_pointer_invalidate();
        L_data *data = get_data(NULL, ev->event);
        if (data != NULL) {
          YF_eventfn fn;
          void *dt;
          yf_event_getfn(YF_EVENT_PT_LEAVE, &fn, &dt);
          if (fn.pt_leave != NULL) {
            fn.pt_leave(data->win, dt);
            ++n;
          }
        } else {
          assert(0);
        }
      } break;

      case XCB_FOCUS_IN: {
        xcb_focus_in_event_t *ev = (xcb_focus_in_event_t *)event;
        L_data *data = get_data(NULL, ev->event);
        if (data != NULL) {
          YF_eventfn fn;
          void *dt;
          yf_event_getfn(YF_EVENT_KB_ENTER, &fn, &dt);
          if (fn.kb_enter != NULL) {
            fn.kb_enter(data->win, dt);
            ++n;
          }
        } else {
          assert(0);
        }
      } break;

      case XCB_FOCUS_OUT: {
        xcb_focus_out_event_t *ev = (xcb_focus_out_event_t *)event;
        L_data *data = get_data(NULL, ev->event);
        if (data != NULL) {
          YF_eventfn fn;
          void *dt;
          yf_event_getfn(YF_EVENT_KB_LEAVE, &fn, &dt);
          if (fn.kb_leave != NULL) {
            fn.kb_leave(data->win, dt);
            ++n;
          }
        } else {
          assert(0);
        }
      } break;

      case XCB_EXPOSE: {
        /* TODO */
      } break;

      case XCB_CONFIGURE_NOTIFY: {
        xcb_configure_notify_event_t *ev;
        ev = (xcb_configure_notify_event_t *)event;
        L_data *data = get_data(NULL, ev->window);
        if (data != NULL) {
          YF_dim2 new_dim = {ev->width, ev->height};
          YF_dim2 dim = data->win->dim;
          if (new_dim.width != dim.width || new_dim.height != dim.height) {
            data->win->dim = new_dim;
            YF_eventfn fn;
            void *dt;
            yf_event_getfn(YF_EVENT_WD_RESIZE, &fn, &dt);
            if (fn.wd_resize != NULL) {
              fn.wd_resize(data->win, new_dim, dt);
              ++n;
            }
          }
        } else {
          assert(0);
        }
      } break;

      case XCB_CLIENT_MESSAGE: {
        xcb_client_message_event_t *ev = (xcb_client_message_event_t *)event;
        xcb_atom_t proto_atom = l_vars.protocol_atom;
        xcb_atom_t del_atom = l_vars.delete_atom;
        if (ev->type == proto_atom && ev->data.data32[0] == del_atom) {
          L_data *data = get_data(NULL, ev->window);
          if (data != NULL) {
            YF_eventfn fn;
            void *dt;
            yf_event_getfn(YF_EVENT_WD_CLOSE, &fn, &dt);
            if (fn.wd_close != NULL) {
              fn.wd_close(data->win, dt);
              ++n;
            }
          } else {
            assert(0);
          }
        }
      } break;
    }
    free(event);
  } while (1);
  return n;
}

int yf_wsi_xcb_resize(void *data) {
  assert(data != NULL);
  L_data *dt = (L_data *)data;
  unsigned value_mask = XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
  unsigned value_list[2] = {dt->win->dim.width, dt->win->dim.height};
  xcb_void_cookie_t cookie;
  xcb_generic_error_t *err;
  YF_XCB_CONFIGURE_WINDOW_CHECKED(
    cookie,
    l_vars.connection,
    dt->window,
    value_mask,
    value_list);
  YF_XCB_REQUEST_CHECK(err, l_vars.connection, cookie);
  if (err != NULL) {
    free(err);
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  return 0;
}

int yf_wsi_xcb_close(void *data) {
  assert(data != NULL);
  L_data *dt = (L_data *)data;
  xcb_void_cookie_t cookie;
  xcb_generic_error_t *err = NULL;
  YF_XCB_UNMAP_WINDOW_CHECKED(cookie, l_vars.connection, dt->window);
  YF_XCB_REQUEST_CHECK(err, l_vars.connection, cookie);
  if (err != NULL) {
    free(err);
    yf_seterr(YF_ERR_DEVGEN, __func__);
    return -1;
  }
  return 0;
}

void yf_wsi_xcb_deinit(void *data) {
  if (data != NULL) {
    L_data *dt = (L_data *)data;
    if (dt->window) {
      xcb_void_cookie_t YF__UNUSED _;
      YF_XCB_DESTROY_WINDOW(_, l_vars.connection, dt->window);
    }
    yf_list_remove(l_data_list, data);
    if (yf_list_getlen(l_data_list) == 0 && l_vars.connection != NULL) {
      YF_XCB_DISCONNECT(l_vars.connection);
      memset(&l_vars, 0, sizeof l_vars);
    }
    free(data);
  }
}

void yf_wsi_xcb_unload(void) {
  if (l_handle != NULL) {
    dlclose(l_handle);
    l_handle = NULL;
    vkCreateXcbSurfaceKHR = NULL;
  }
}

static int init_vars(void) {
  const char *proto_name = "WM_PROTOCOLS";
  const char *del_name = "WM_DELETE_WINDOW";
  const char *title_name = "WM_NAME";
  const char *utf8_name = "UTF8_STRING";
  const char *class_name = "WM_CLASS";
  xcb_void_cookie_t cookie;
  xcb_intern_atom_cookie_t atom_cookie;
  xcb_intern_atom_reply_t *atom_reply = NULL;
  xcb_generic_error_t *err = NULL;
  unsigned value_mask;
  unsigned value_list[1];
  int res;

  YF_XCB_CONNECT(l_vars.connection, NULL, NULL);
  YF_XCB_CONNECTION_HAS_ERROR(res, l_vars.connection);
  if (res != 0)
    goto on_error;

  YF_XCB_INTERN_ATOM(
    atom_cookie,
    l_vars.connection,
    0,
    strlen(proto_name),
    proto_name);
  YF_XCB_INTERN_ATOM_REPLY(atom_reply, l_vars.connection, atom_cookie, &err);
  if (err != NULL || atom_reply == NULL)
    goto on_error;
  l_vars.protocol_atom = atom_reply->atom;
  free(atom_reply);
  atom_reply = NULL;

  YF_XCB_INTERN_ATOM(
    atom_cookie,
    l_vars.connection,
    0,
    strlen(del_name),
    del_name);
  YF_XCB_INTERN_ATOM_REPLY(atom_reply, l_vars.connection, atom_cookie, &err);
  if (err != NULL || atom_reply == NULL)
    goto on_error;
  l_vars.delete_atom = atom_reply->atom;
  free(atom_reply);
  atom_reply = NULL;

  YF_XCB_INTERN_ATOM(
    atom_cookie,
    l_vars.connection,
    0,
    strlen(title_name),
    title_name);
  YF_XCB_INTERN_ATOM_REPLY(atom_reply, l_vars.connection, atom_cookie, &err);
  if (err != NULL || atom_reply == NULL)
    goto on_error;
  l_vars.title_atom = atom_reply->atom;
  free(atom_reply);
  atom_reply = NULL;

  YF_XCB_INTERN_ATOM(
    atom_cookie,
    l_vars.connection,
    0,
    strlen(utf8_name),
    utf8_name);
  YF_XCB_INTERN_ATOM_REPLY(atom_reply, l_vars.connection, atom_cookie, &err);
  if (err != NULL || atom_reply == NULL)
    goto on_error;
  l_vars.utf8_atom = atom_reply->atom;
  free(atom_reply);
  atom_reply = NULL;

  YF_XCB_INTERN_ATOM(
    atom_cookie,
    l_vars.connection,
    0,
    strlen(class_name),
    class_name);
  YF_XCB_INTERN_ATOM_REPLY(atom_reply, l_vars.connection, atom_cookie, &err);
  if (err != NULL || atom_reply == NULL)
    goto on_error;
  l_vars.class_atom = atom_reply->atom;
  free(atom_reply);
  atom_reply = NULL;

  value_mask = XCB_KB_AUTO_REPEAT_MODE;
  value_list[0] = XCB_AUTO_REPEAT_MODE_OFF;
  YF_XCB_CHANGE_KEYBOARD_CONTROL_CHECKED(
    cookie,
    l_vars.connection,
    value_mask,
    value_list);
  YF_XCB_REQUEST_CHECK(err, l_vars.connection, cookie);
  if (err != NULL)
    goto on_error;

  YF_XCB_FLUSH(res, l_vars.connection);
  if (res <= 0)
    goto on_error;

  return 0;

  on_error:
  free(err);
  free(atom_reply);
  if (l_vars.connection != NULL)
    YF_XCB_DISCONNECT(l_vars.connection);
  memset(&l_vars, 0, sizeof l_vars);
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

  xcb_generic_error_t *err = NULL;
  const xcb_setup_t *setup = NULL;
  xcb_screen_iterator_t screen_it;
  xcb_void_cookie_t cookie;
  unsigned value_mask;
  unsigned value_list[2];
  char class[YF_STR_MAXLEN * 2] = {'\0', '\0'};
  size_t len;
  int res;
  VkResult rv;

  YF_XCB_GET_SETUP(setup, l_vars.connection);
  if (setup == NULL)
    goto on_error;
  YF_XCB_SETUP_ROOTS_ITERATOR(screen_it, setup);
  YF_XCB_GENERATE_ID(data->window, l_vars.connection);

  value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  value_list[0] = screen_it.data->black_pixel;
  value_list[1] =
    XCB_EVENT_MASK_KEY_PRESS |
    XCB_EVENT_MASK_KEY_RELEASE |
    XCB_EVENT_MASK_BUTTON_PRESS |
    XCB_EVENT_MASK_BUTTON_RELEASE |
    XCB_EVENT_MASK_ENTER_WINDOW |
    XCB_EVENT_MASK_LEAVE_WINDOW |
    XCB_EVENT_MASK_POINTER_MOTION |
    XCB_EVENT_MASK_BUTTON_MOTION |
    XCB_EVENT_MASK_EXPOSURE |
    XCB_EVENT_MASK_STRUCTURE_NOTIFY |
    XCB_EVENT_MASK_FOCUS_CHANGE;
  YF_XCB_CREATE_WINDOW_CHECKED(
    cookie,
    l_vars.connection,
    0,
    data->window,
    screen_it.data->root,
    0,
    0,
    win->dim.width,
    win->dim.height,
    0,
    XCB_WINDOW_CLASS_INPUT_OUTPUT,
    screen_it.data->root_visual,
    value_mask,
    value_list);
  YF_XCB_REQUEST_CHECK(err, l_vars.connection, cookie);
  if (err != NULL)
    goto on_error;

  YF_XCB_CHANGE_PROPERTY_CHECKED(
    cookie,
    l_vars.connection,
    XCB_PROP_MODE_REPLACE,
    data->window,
    l_vars.protocol_atom,
    XCB_ATOM_ATOM,
    32,
    1,
    &l_vars.delete_atom);
  YF_XCB_REQUEST_CHECK(err, l_vars.connection, cookie);
  if (err != NULL)
    goto on_error;

  YF_XCB_CHANGE_PROPERTY_CHECKED(
    cookie,
    l_vars.connection,
    XCB_PROP_MODE_REPLACE,
    data->window,
    l_vars.title_atom,
    l_vars.utf8_atom,
    8,
    win->title == NULL ? 0 : strnlen(win->title, YF_STR_MAXLEN - 1),
    win->title);
  YF_XCB_REQUEST_CHECK(err, l_vars.connection, cookie);
  if (err != NULL)
    goto on_error;

  if ((len = strnlen(YF_APP, YF_STR_MAXLEN)) < YF_STR_MAXLEN &&
    (strnlen(YF_APP_ID, YF_STR_MAXLEN) < YF_STR_MAXLEN))
  {
    strcpy(class, YF_APP);
    strcpy(class+len+1, YF_APP_ID);
    len += strlen(YF_APP_ID) + 2;
  } else {
    len = 2;
  }
  YF_XCB_CHANGE_PROPERTY_CHECKED(
    cookie,
    l_vars.connection,
    XCB_PROP_MODE_REPLACE,
    data->window,
    l_vars.class_atom,
    XCB_ATOM_STRING,
    8,
    len,
    class);
  YF_XCB_REQUEST_CHECK(err, l_vars.connection, cookie);
  if (err != NULL)
    goto on_error;

  YF_XCB_MAP_WINDOW_CHECKED(cookie, l_vars.connection, data->window);
  YF_XCB_REQUEST_CHECK(err, l_vars.connection, cookie);
  if (err != NULL)
    goto on_error;

  YF_XCB_FLUSH(res, l_vars.connection);
  if (res <= 0)
    goto on_error;

  VkXcbSurfaceCreateInfoKHR info = {
    .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
    .pNext = NULL,
    .flags = 0,
    .connection = l_vars.connection,
    .window = data->window
  };
  rv = vkCreateXcbSurfaceKHR(win->ctx->instance, &info, NULL, &win->surface);
  if (rv != VK_SUCCESS)
    goto on_error;

  if (yf_list_insert(l_data_list, data) != 0)
    goto on_error;

  return data;

  on_error:
  free(err);
  yf_wsi_xcb_deinit(data);
  yf_seterr(YF_ERR_DEVGEN, __func__);
  return NULL;
}

static L_data *get_data(YF_window win, xcb_window_t window) {
  L_data *data = NULL;
  YF_iter it = YF_NILIT;
  do {
    data = yf_list_next(l_data_list, &it);
    if (YF_IT_ISNIL(it) || (data->win == win || data->window == window))
      break;
  } while (1);
  return data;
}
