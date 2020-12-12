/*
 * YF
 * platform-xcb.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <assert.h>

#include <yf/com/yf-list.h>
#include <yf/com/yf-error.h>

#include "platform-xcb.h"
#include "yf-keyboard.h"
#include "yf-pointer.h"
#include "keymap.h"

#ifndef YF_APP_ID
# define YF_APP_ID "unknown.app.id"
#endif

#ifndef YF_APP_NAME
# define YF_APP_NAME "Unknown app"
#endif

#undef YF_STR_MAXLEN
#define YF_STR_MAXLEN 60

#define YF_LIBXCB "libxcb.so"

/* Window implementation functions. */
static void *init_win(unsigned, unsigned, const char *, unsigned, YF_window);
static int open_win(void *);
static int close_win(void *);
static int resize_win(void *, unsigned, unsigned);
static int toggle_win(void *);
static int settitle_win(void *, const char *);
static void getsize_win(void *, unsigned *, unsigned *);
static void deinit_win(void *);

const YF_win_imp yf_g_winxcb = {
  .init = init_win,
  .open = open_win,
  .close = close_win,
  .resize = resize_win,
  .toggle = toggle_win,
  .settitle = settitle_win,
  .getsize = getsize_win,
  .deinit = deinit_win
};

/* Event implementation functions. */
static int poll_evt(unsigned);
static void changed_evt(int);

const YF_evt_imp yf_g_evtxcb = {.poll = poll_evt, .changed = changed_evt};

/* Global variables setting. */
static int set_vars(void);

YF_varsxcb yf_g_varsxcb = {0};

/* Shared object handle. */
static void *l_handle = NULL;

/* Symbol names. */
static const char *l_names[] = {
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
static void *l_addrs[sizeof l_names / sizeof l_names[0]] = {0};

/* Wrappers. */
#define YF_XCB_CONNECT(res, name, screen) do { \
  xcb_connection_t *(*fn)(const char *, int *); \
  *(void **)(&fn) = l_addrs[YF_SYM_CONNECT]; \
  res = fn(name, screen); } while (0)

#define YF_XCB_DISCONNECT(connection) do { \
  void (*fn)(xcb_connection_t *); \
  *(void **)(&fn) = l_addrs[YF_SYM_DISCONNECT]; \
  fn(connection); } while (0)

#define YF_XCB_FLUSH(res, connection) do { \
  int (*fn)(xcb_connection_t *); \
  *(void **)(&fn) = l_addrs[YF_SYM_FLUSH]; \
  res = fn(connection); } while (0)

#define YF_XCB_CONNECTION_HAS_ERROR(res, connection) do { \
  int (*fn)(xcb_connection_t *); \
  *(void **)(&fn) = l_addrs[YF_SYM_CONNECTION_HAS_ERROR]; \
  res = fn(connection); } while (0)

#define YF_XCB_GENERATE_ID(res, connection) do { \
  uint32_t (*fn)(xcb_connection_t *); \
  *(void **)(&fn) = l_addrs[YF_SYM_GENERATE_ID]; \
  res = fn(connection); } while (0)

#define YF_XCB_POLL_FOR_EVENT(res, connection) do { \
  xcb_generic_event_t *(*fn)(xcb_connection_t *); \
  *(void **)(&fn) = l_addrs[YF_SYM_POLL_FOR_EVENT]; \
  res = fn(connection); } while (0)

#define YF_XCB_REQUEST_CHECK(res, connection, cookie) do { \
  xcb_generic_error_t *(*fn)(xcb_connection_t *, xcb_void_cookie_t); \
  *(void **)(&fn) = l_addrs[YF_SYM_REQUEST_CHECK]; \
  res = fn(connection, cookie); } while (0)

#define YF_XCB_GET_SETUP(res, connection) do { \
  const struct xcb_setup_t *(*fn)(xcb_connection_t *); \
  *(void **)(&fn) = l_addrs[YF_SYM_GET_SETUP]; \
  res = fn(connection); } while (0)

#define YF_XCB_SETUP_ROOTS_ITERATOR(res, setup) do { \
  xcb_screen_iterator_t (*fn)(const xcb_setup_t *); \
  *(void **)(&fn) = l_addrs[YF_SYM_SETUP_ROOTS_ITERATOR]; \
  res = fn(setup); } while (0)

#define YF_XCB_CREATE_WINDOW_CHECKED(res, connection, depth, id, parent, \
x, y, w, h, border_w, class, visual, value_mask, value_list) do { \
  xcb_void_cookie_t (*fn)( \
    xcb_connection_t *, uint8_t, xcb_window_t, xcb_window_t, \
    int16_t, int16_t, uint16_t, uint16_t, uint16_t, uint16_t, \
    xcb_visualid_t, uint32_t, const void *); \
  *(void **)(&fn) = l_addrs[YF_SYM_CREATE_WINDOW_CHECKED]; \
  res = fn( \
    connection, depth, id, parent, \
    x, y, w, h, border_w, class, \
    visual, value_mask, value_list); } while (0)

#define YF_XCB_DESTROY_WINDOW(res, connection, window) do { \
  xcb_void_cookie_t (*fn)(xcb_connection_t *, xcb_window_t); \
  *(void **)(&fn) = l_addrs[YF_SYM_DESTROY_WINDOW]; \
  res = fn(connection, window); } while (0)

#define YF_XCB_MAP_WINDOW_CHECKED(res, connection, window) do { \
  xcb_void_cookie_t (*fn)(xcb_connection_t *, xcb_window_t); \
  *(void **)(&fn) = l_addrs[YF_SYM_MAP_WINDOW_CHECKED]; \
  res = fn(connection, window); } while (0)

#define YF_XCB_UNMAP_WINDOW_CHECKED(res, connection, window) do { \
  xcb_void_cookie_t (*fn)(xcb_connection_t *, xcb_window_t); \
  *(void **)(&fn) = l_addrs[YF_SYM_UNMAP_WINDOW_CHECKED]; \
  res = fn(connection, window); } while (0)

#define YF_XCB_CONFIGURE_WINDOW_CHECKED( \
res, connection, window, value_mask, value_list) do { \
  xcb_void_cookie_t (*fn)( \
    xcb_connection_t *, xcb_window_t, uint32_t, const void *); \
  *(void **)(&fn) = l_addrs[YF_SYM_CONFIGURE_WINDOW_CHECKED]; \
  res = fn(connection, window, value_mask, value_list); } while (0)

#define YF_XCB_INTERN_ATOM(res, connection, dont_create, name_len, name) do { \
  xcb_intern_atom_cookie_t (*fn)( \
    xcb_connection_t *, uint8_t, uint16_t, const char *); \
  *(void **)(&fn) = l_addrs[YF_SYM_INTERN_ATOM]; \
  res = fn(connection, dont_create, name_len, name); } while (0)

#define YF_XCB_INTERN_ATOM_REPLY(res, connection, cookie, error) do { \
  xcb_intern_atom_reply_t *(*fn)( \
    xcb_connection_t *, xcb_intern_atom_cookie_t, xcb_generic_error_t **); \
  *(void **)(&fn) = l_addrs[YF_SYM_INTERN_ATOM_REPLY]; \
  res = fn(connection, cookie, error); } while (0)

#define YF_XCB_CHANGE_PROPERTY_CHECKED( \
res, connection, mode, window, property, type, format, data_len, data) do { \
  xcb_void_cookie_t (*fn)( \
    xcb_connection_t *, uint8_t, xcb_window_t, xcb_atom_t, \
    xcb_atom_t, uint8_t, uint32_t, const void *); \
  *(void **)(&fn) = l_addrs[YF_SYM_CHANGE_PROPERTY_CHECKED]; \
  res = fn(connection, mode, window, property, type, format, data_len, data); \
  } while (0)

#define YF_XCB_CHANGE_KEYBOARD_CONTROL_CHECKED( \
res, connection, value_mask, value_list) do { \
  xcb_void_cookie_t (*fn)(xcb_connection_t *, uint32_t, const void *); \
  *(void **)(&fn) = l_addrs[YF_SYM_CHANGE_KEYBOARD_CONTROL_CHECKED]; \
  res = fn(connection, value_mask, value_list); } while (0)

/* Type defining the data of a window object. */
typedef struct {
  YF_window wrapper;
  unsigned creat_mask;
  unsigned width;
  unsigned height;
  int open;
  int fullscreen;
  xcb_window_t win_id;
} L_win;

/* List containing the 'L_win' data of all created windows. */
static YF_list l_wins = NULL;

/* Gets the 'L_win' data for a given xcb window ID.
   If not found, 'data' will contain the null value. */
#define YF_GETWINDATA(data, id) do { \
  assert(l_wins != NULL && yf_list_getlen(l_wins) > 0); \
  YF_iter it = YF_NILIT; \
  for (;;) { \
    data = yf_list_next(l_wins, &it); \
    if ((data) == NULL || (data)->win_id == (id)) break; \
  }} while (0)

xcb_connection_t *yf_getconnxcb(void) {
  return yf_g_varsxcb.conn;
}

xcb_visualid_t yf_getvisualxcb(void) {
  return yf_g_varsxcb.visual;
}

xcb_window_t yf_getwindowxcb(YF_window win) {
  assert(win != NULL);

  /* XXX: Consider querying the 'YF_window' for the 'L_win' data instead. */
  YF_iter it = YF_NILIT;
  L_win *data;
  do
    data = yf_list_next(l_wins, &it);
  while (data->wrapper != win);

  return data->win_id;
}

int yf_loadxcb(void) {
  if (l_handle != NULL)
    return 0;

  l_handle = dlopen(YF_LIBXCB, RTLD_LAZY);
  if (l_handle == NULL) {
    yf_seterr(YF_ERR_OTHER, __func__);
    return -1;
  }

  char *err;
  for (size_t i = 0; i < (sizeof l_names / sizeof l_names[0]); ++i) {
    dlerror();
    l_addrs[i] = dlsym(l_handle, l_names[i]);
    err = dlerror();
    if (err != NULL) {
      yf_unldxcb();
      yf_seterr(YF_ERR_OTHER, err);
      return -1;
    }
  }

  if (set_vars() != 0) {
    yf_unldxcb();
    return -1;
  }

  if ((l_wins = yf_list_init(NULL)) == NULL) {
    yf_unldxcb();
    return -1;
  }

  return 0;
}

void yf_unldxcb(void) {
  if (l_wins != NULL) {
    L_win *win;
    YF_iter it = YF_NILIT;
    for (;;) {
      win = yf_list_next(l_wins, &it);
      if (YF_IT_ISNIL(it))
        break;
      deinit_win(win);
    }
    yf_list_deinit(l_wins);
    l_wins = NULL;
  }

  if (yf_g_varsxcb.conn != NULL) {
    YF_XCB_DISCONNECT(yf_g_varsxcb.conn);
    memset(&yf_g_varsxcb, 0, sizeof yf_g_varsxcb);
  }

  if (l_handle != NULL) {
    dlclose(l_handle);
    l_handle = NULL;
    memset(l_addrs, 0, sizeof l_addrs);
  }
}

static int set_vars(void) {
  assert(l_handle != NULL);
  assert(yf_g_varsxcb.conn == NULL);

  int res;

  YF_XCB_CONNECT(yf_g_varsxcb.conn, NULL, NULL);
  YF_XCB_CONNECTION_HAS_ERROR(res, yf_g_varsxcb.conn);
  if (res != 0) {
    yf_seterr(YF_ERR_OTHER, __func__);
    return -1;
  }

  const xcb_setup_t *setup = NULL;
  xcb_screen_iterator_t screen_it;

  YF_XCB_GET_SETUP(setup, yf_g_varsxcb.conn);
  if (setup == NULL) {
    yf_seterr(YF_ERR_OTHER, __func__);
    return -1;
  }
  YF_XCB_SETUP_ROOTS_ITERATOR(screen_it, setup);
  yf_g_varsxcb.visual = screen_it.data->root_visual;
  yf_g_varsxcb.root_win = screen_it.data->root;
  yf_g_varsxcb.white_px = screen_it.data->white_pixel;
  yf_g_varsxcb.black_px = screen_it.data->black_pixel;

  xcb_atom_t *const atoms[] = {
    &yf_g_varsxcb.atom.proto,
    &yf_g_varsxcb.atom.del,
    &yf_g_varsxcb.atom.title,
    &yf_g_varsxcb.atom.utf8,
    &yf_g_varsxcb.atom.clss
  };
  const char *atom_names[] = {
    "WM_PROTOCOLS",
    "WM_DELETE_WINDOW",
    "WM_NAME",
    "UTF8_STRING",
    "WM_CLASS"
  };
  xcb_intern_atom_cookie_t atom_cookie;
  xcb_intern_atom_reply_t *atom_reply = NULL;
  xcb_generic_error_t *err = NULL;

  for (size_t i = 0; i < (sizeof atoms / sizeof atoms[0]); ++i) {
    YF_XCB_INTERN_ATOM(atom_cookie, yf_g_varsxcb.conn, 0,
        strlen(atom_names[i]), atom_names[i]);
    YF_XCB_INTERN_ATOM_REPLY(atom_reply, yf_g_varsxcb.conn, atom_cookie, &err);
    if (err != NULL || atom_reply == NULL) {
      yf_seterr(YF_ERR_OTHER, __func__);
      free(err);
      free(atom_reply);
      return -1;
    }
    *atoms[i] = atom_reply->atom;
    free(atom_reply);
    atom_reply = NULL;
  }

  uint32_t val_mask = XCB_KB_AUTO_REPEAT_MODE;
  uint32_t val_list[] = {XCB_AUTO_REPEAT_MODE_OFF};
  xcb_void_cookie_t cookie;

  YF_XCB_CHANGE_KEYBOARD_CONTROL_CHECKED(cookie, yf_g_varsxcb.conn, val_mask,
      val_list);
  YF_XCB_REQUEST_CHECK(err, yf_g_varsxcb.conn, cookie);
  if (err != NULL) {
    yf_seterr(YF_ERR_OTHER, __func__);
    free(err);
    return -1;
  }

  YF_XCB_FLUSH(res, yf_g_varsxcb.conn);
  if (res <= 0) {
    yf_seterr(YF_ERR_OTHER, __func__);
    return -1;
  }

  return 0;
}

static void *init_win(unsigned width, unsigned height, const char *title,
    unsigned creat_mask, YF_window wrapper)
{
  assert(l_handle != NULL);
  assert(yf_g_varsxcb.conn != NULL);

  if (width == 0 || height == 0) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return NULL;
  }

  L_win *win = calloc(1, sizeof(L_win));
  if (win == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }

  win->wrapper = wrapper;
  win->creat_mask = creat_mask;
  win->width = width;
  win->height = height;
  win->open = 0;
  win->fullscreen = 0;

  YF_XCB_GENERATE_ID(win->win_id, yf_g_varsxcb.conn);

  xcb_generic_error_t *err = NULL;
  xcb_void_cookie_t cookie;
  uint32_t val_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  uint32_t val_list[2];
  val_list[0] = yf_g_varsxcb.black_px;
  val_list[1] =
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

  YF_XCB_CREATE_WINDOW_CHECKED(cookie, yf_g_varsxcb.conn, 0, win->win_id,
      yf_g_varsxcb.root_win, 0, 0, width, height, 0,
      XCB_WINDOW_CLASS_INPUT_OUTPUT, yf_g_varsxcb.visual, val_mask, val_list);
  YF_XCB_REQUEST_CHECK(err, yf_g_varsxcb.conn, cookie);
  if (err != NULL) {
    yf_seterr(YF_ERR_OTHER, __func__);
    free(win);
    free(err);
    return NULL;
  }

  YF_XCB_CHANGE_PROPERTY_CHECKED(cookie, yf_g_varsxcb.conn,
      XCB_PROP_MODE_REPLACE, win->win_id, yf_g_varsxcb.atom.proto,
      XCB_ATOM_ATOM, 32, 1, &yf_g_varsxcb.atom.del);
  YF_XCB_REQUEST_CHECK(err, yf_g_varsxcb.conn, cookie);
  if (err != NULL) {
    yf_seterr(YF_ERR_OTHER, __func__);
    deinit_win(win);
    free(err);
    return NULL;
  }

  size_t len = title == NULL ? 0 : strnlen(title, YF_STR_MAXLEN-1);

  YF_XCB_CHANGE_PROPERTY_CHECKED(cookie, yf_g_varsxcb.conn,
      XCB_PROP_MODE_REPLACE, win->win_id, yf_g_varsxcb.atom.title,
      yf_g_varsxcb.atom.utf8, 8, len, title);
  YF_XCB_REQUEST_CHECK(err, yf_g_varsxcb.conn, cookie);
  if (err != NULL) {
    yf_seterr(YF_ERR_OTHER, __func__);
    deinit_win(win);
    free(err);
    return NULL;
  }

  len = strnlen(YF_APP_NAME, YF_STR_MAXLEN);
  size_t len2 = strnlen(YF_APP_ID, YF_STR_MAXLEN);
  char clss[YF_STR_MAXLEN * 2] = {'\0', '\0'};
  if (len < YF_STR_MAXLEN && len2 < YF_STR_MAXLEN){
    strcpy(clss, YF_APP_NAME);
    strcpy(clss+len+1, YF_APP_ID);
    len += len2 + 2;
  } else {
    len = 2;
  }

  YF_XCB_CHANGE_PROPERTY_CHECKED(cookie, yf_g_varsxcb.conn,
      XCB_PROP_MODE_REPLACE, win->win_id, yf_g_varsxcb.atom.clss,
      XCB_ATOM_STRING, 8, len, clss);
  YF_XCB_REQUEST_CHECK(err, yf_g_varsxcb.conn, cookie);
  if (err != NULL) {
    yf_seterr(YF_ERR_OTHER, __func__);
    deinit_win(win);
    free(err);
    return NULL;
  }

  int res;

  YF_XCB_FLUSH(res, yf_g_varsxcb.conn);
  if (res <= 0) {
    yf_seterr(YF_ERR_OTHER, __func__);
    deinit_win(win);
    return NULL;
  }

  /* TODO: Handle remaining creation flags. */

  if (!(creat_mask & YF_WINCREAT_HIDDEN)) {
    if (open_win(win) != 0) {
      deinit_win(win);
      return NULL;
    }
  }

  if (creat_mask & YF_WINCREAT_FULLSCREEN) {
    if (toggle_win(win) != 0) {
      deinit_win(win);
      return NULL;
    }
  }

  yf_list_insert(l_wins, win);
  return win;
}

static int open_win(void *win) {
  assert(l_handle != NULL);
  assert(yf_g_varsxcb.conn != NULL);
  assert(win != NULL);

  if (((L_win *)win)->open)
    return 0;

  xcb_window_t win_id = ((L_win *)win)->win_id;
  xcb_void_cookie_t cookie;
  xcb_generic_error_t *err = NULL;

  YF_XCB_MAP_WINDOW_CHECKED(cookie, yf_g_varsxcb.conn, win_id);
  YF_XCB_REQUEST_CHECK(err, yf_g_varsxcb.conn, cookie);
  if (err != NULL) {
    yf_seterr(YF_ERR_OTHER, __func__);
    free(err);
    return -1;
  }

  ((L_win *)win)->open = 1;
  return 0;
}

static int close_win(void *win) {
  assert(l_handle != NULL);
  assert(yf_g_varsxcb.conn != NULL);
  assert(win != NULL);

  if (!((L_win *)win)->open)
    return 0;

  xcb_window_t win_id = ((L_win *)win)->win_id;
  xcb_void_cookie_t cookie;
  xcb_generic_error_t *err = NULL;

  YF_XCB_UNMAP_WINDOW_CHECKED(cookie, yf_g_varsxcb.conn, win_id);
  YF_XCB_REQUEST_CHECK(err, yf_g_varsxcb.conn, cookie);
  if (err != NULL) {
    yf_seterr(YF_ERR_OTHER, __func__);
    free(err);
    return -1;
  }

  ((L_win *)win)->open = 0;
  return 0;
}

static int resize_win(void *win, unsigned width, unsigned height) {
  assert(l_handle != NULL);
  assert(yf_g_varsxcb.conn != NULL);
  assert(win != NULL);

  if (width == 0 || height == 0) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
  }

  xcb_window_t win_id = ((L_win *)win)->win_id;
  xcb_void_cookie_t cookie;
  xcb_generic_error_t *err = NULL;
  uint32_t val_mask = XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
  uint32_t val_list[] = {width, height};

  YF_XCB_CONFIGURE_WINDOW_CHECKED(cookie, yf_g_varsxcb.conn, win_id, val_mask,
      val_list);
  YF_XCB_REQUEST_CHECK(err, yf_g_varsxcb.conn, cookie);
  if (err != NULL) {
    yf_seterr(YF_ERR_OTHER, __func__);
    free(err);
    return -1;
  }

  ((L_win *)win)->width = width;
  ((L_win *)win)->height = height;
  return 0;
}

static int toggle_win(void *win) {
  assert(l_handle != NULL);
  assert(yf_g_varsxcb.conn != NULL);
  assert(win != NULL);

  /* TODO */

  return -1;
}

static int settitle_win(void *win, const char *title) {
  assert(l_handle != NULL);
  assert(yf_g_varsxcb.conn != NULL);
  assert(win != NULL);

  xcb_window_t win_id = ((L_win *)win)->win_id;
  xcb_void_cookie_t cookie;
  xcb_generic_error_t *err = NULL;
  size_t len = title == NULL ? 0 : strnlen(title, YF_STR_MAXLEN-1);

  YF_XCB_CHANGE_PROPERTY_CHECKED(cookie, yf_g_varsxcb.conn,
      XCB_PROP_MODE_REPLACE, win_id, yf_g_varsxcb.atom.title,
      yf_g_varsxcb.atom.utf8, 8, len, title);
  YF_XCB_REQUEST_CHECK(err, yf_g_varsxcb.conn, cookie);
  if (err != NULL) {
    yf_seterr(YF_ERR_OTHER, __func__);
    free(err);
    return -1;
  }

  return 0;
}

static void getsize_win(void *win, unsigned *width, unsigned *height) {
  assert(l_handle != NULL);
  assert(yf_g_varsxcb.conn != NULL);
  assert(win != NULL);
  assert(width != NULL && height != NULL);

  *width = ((L_win *)win)->width;
  *height = ((L_win *)win)->height;
}

static void deinit_win(void *win) {
  if (l_handle != NULL && yf_g_varsxcb.conn != NULL) {
    xcb_void_cookie_t unused;
    YF_XCB_DESTROY_WINDOW(unused, yf_g_varsxcb.conn, ((L_win *)win)->win_id);
  }

  yf_list_remove(l_wins, win);
  free(win);
}

static int poll_evt(unsigned evt_mask) {
  assert(l_handle != NULL);
  assert(yf_g_varsxcb.conn != NULL);

  const unsigned mask = evt_mask & yf_getevtmask();
  if (evt_mask == YF_EVT_NONE)
    return 0;

  unsigned type;
  xcb_generic_event_t *event = NULL;

  do {
    YF_XCB_POLL_FOR_EVENT(event, yf_g_varsxcb.conn);
    if (event == NULL)
      break;
    type = event->response_type & ~0x80;

    switch (type) {
      case XCB_KEY_PRESS:
      case XCB_KEY_RELEASE: {
        if (!(mask & YF_EVT_KEYKB))
          break;
        xcb_key_press_event_t *key_evt = (xcb_key_press_event_t *)event;

        unsigned code = key_evt->detail - 8;
        int key = YF_KEY_FROM(code);

        int state;
        if (type == XCB_KEY_PRESS)
          state = YF_KEYSTATE_PRESSED;
        else
          state = YF_KEYSTATE_RELEASED;

        static uint16_t prev_evt_state = 0;
        static unsigned prev_mod_mask = 0;
        unsigned mod_mask = prev_mod_mask;

        if (key_evt->state != prev_evt_state) {
          mod_mask = 0;

          if (key_evt->state & XCB_MOD_MASK_LOCK)
            mod_mask |= YF_KEYMOD_CAPSLOCK;
          if (key_evt->state & XCB_MOD_MASK_SHIFT)
            mod_mask |= YF_KEYMOD_SHIFT;
          if (key_evt->state & XCB_MOD_MASK_CONTROL)
            mod_mask |= YF_KEYMOD_CTRL;
          if (key_evt->state & XCB_MOD_MASK_1)
            mod_mask |= YF_KEYMOD_ALT;

          prev_mod_mask = mod_mask;
          prev_evt_state = key_evt->state;
        }

        YF_evtfn fn;
        void *data;
        yf_getevtfn(YF_EVT_KEYKB, &fn, &data);
        fn.key_kb(key, state, mod_mask, data);
      } break;

      case XCB_BUTTON_PRESS:
      case XCB_BUTTON_RELEASE: {
        if (!(mask & YF_EVT_BUTTONPT))
          break;
        xcb_button_press_event_t *btn_evt = (xcb_button_press_event_t*)event;

        int btn;
        switch (btn_evt->detail) {
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
        if (type == XCB_BUTTON_PRESS)
          state = YF_BTNSTATE_PRESSED;
        else
          state = YF_BTNSTATE_RELEASED;

        int x = btn_evt->event_x;
        int y = btn_evt->event_y;

        YF_evtfn fn;
        void *data;
        yf_getevtfn(YF_EVT_BUTTONPT, &fn, &data);
        fn.button_pt(btn, state, x, y, data);
      } break;

      case XCB_MOTION_NOTIFY: {
        if (!(mask & YF_EVT_MOTIONPT))
          break;
        xcb_motion_notify_event_t *mot_evt = (xcb_motion_notify_event_t *)event;

        int x = mot_evt->event_x;
        int y = mot_evt->event_y;

        YF_evtfn fn;
        void *data;
        yf_getevtfn(YF_EVT_MOTIONPT, &fn, &data);
        fn.motion_pt(x, y, data);
      } break;

      case XCB_ENTER_NOTIFY: {
        if (!(mask & YF_EVT_ENTERPT))
          break;
        xcb_enter_notify_event_t *entr_evt = (xcb_enter_notify_event_t *)event;

        L_win *win;
        YF_GETWINDATA(win, entr_evt->event);
        if (win == NULL)
          break;

        int x = entr_evt->event_x;
        int y = entr_evt->event_y;

        YF_evtfn fn;
        void *data;
        yf_getevtfn(YF_EVT_ENTERPT, &fn, &data);
        fn.enter_pt(win->wrapper, x, y, data);
      } break;

      case XCB_LEAVE_NOTIFY: {
        if (!(mask & YF_EVT_LEAVEPT))
          break;
        xcb_leave_notify_event_t *leav_evt = (xcb_leave_notify_event_t *)event;

        L_win *win;
        YF_GETWINDATA(win, leav_evt->event);
        if (win == NULL)
          break;

        YF_evtfn fn;
        void *data;
        yf_getevtfn(YF_EVT_LEAVEPT, &fn, &data);
        fn.leave_pt(win->wrapper, data);
      } break;

      case XCB_FOCUS_IN: {
        if (!(mask & YF_EVT_ENTERKB))
          break;
        xcb_focus_in_event_t *foc_evt = (xcb_focus_in_event_t *)event;

        L_win *win;
        YF_GETWINDATA(win, foc_evt->event);
        if (win == NULL)
          break;

        YF_evtfn fn;
        void *data;
        yf_getevtfn(YF_EVT_ENTERKB, &fn, &data);
        fn.enter_kb(win->wrapper, data);
      } break;

      case XCB_FOCUS_OUT: {
        if (!(mask & YF_EVT_LEAVEKB))
          break;
        xcb_focus_out_event_t *foc_evt = (xcb_focus_out_event_t *)event;

        L_win *win;
        YF_GETWINDATA(win, foc_evt->event);
        if (win == NULL)
          break;

        YF_evtfn fn;
        void *data;
        yf_getevtfn(YF_EVT_LEAVEKB, &fn, &data);
        fn.leave_kb(win->wrapper, data);
      } break;

      case XCB_EXPOSE: {
        /* TODO */
      } break;

      case XCB_CONFIGURE_NOTIFY: {
        if (!(mask & YF_EVT_RESIZEWD))
          break;
        xcb_configure_notify_event_t *conf_evt;
        conf_evt = (xcb_configure_notify_event_t *)event;

        L_win *win;
        YF_GETWINDATA(win, conf_evt->window);
        if (win == NULL)
          break;

        if (win->width == conf_evt->width && win->height == conf_evt->height)
          break;
        win->width = conf_evt->width;
        win->height = conf_evt->height;

        YF_evtfn fn;
        void *data;
        yf_getevtfn(YF_EVT_RESIZEWD, &fn, &data);
        fn.resize_wd(win->wrapper, win->width, win->height, data);
      } break;

      case XCB_CLIENT_MESSAGE: {
        if (!(mask & YF_EVT_CLOSEWD))
          break;
        xcb_client_message_event_t *cli_evt;
        cli_evt = (xcb_client_message_event_t *)event;

        if (cli_evt->type != yf_g_varsxcb.atom.proto ||
            cli_evt->data.data32[0] != yf_g_varsxcb.atom.del)
          break;

        L_win *win;
        YF_GETWINDATA(win, cli_evt->window);
        if (win == NULL)
          break;

        YF_evtfn fn;
        void *data;
        yf_getevtfn(YF_EVT_CLOSEWD, &fn, &data);
        fn.close_wd(win->wrapper, data);
      } break;

      default:
        break;
    }

    free(event);
  } while (1);

  return 0;
}

static void changed_evt(int evt) {
  /* TODO */
}