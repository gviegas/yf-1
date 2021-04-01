/*
 * YF
 * test-label.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include <yf/wsys/yf-event.h>
#include <yf/wsys/yf-keyboard.h>

#include "yf-ngn.h"

#define YF_WINW 960
#define YF_WINH 600
#define YF_WINT "Label"
#define YF_FPS  30

/* Local variables. */
struct L_vars {
  YF_window win;
  YF_view view;
  YF_scene scn;
  YF_label labl;
  YF_font font;

  struct {
    int insert;
    int quit;
  } input;
};
static struct L_vars l_vars = {0};

/* Inserts wide-characters. */
static void insert_wc(wchar_t wc) {
#define YF_MAXN 255
  static wchar_t wcs[YF_MAXN+1] = {0};
  static size_t n = 0;

  if (wc == L'\b') {
    if (n != 0) {
      wcs[--n] = L'\0';
      yf_label_setstr(l_vars.labl, wcs);
    }
  } else if (n < YF_MAXN) {
    wcs[n++] = wc;
    yf_label_setstr(l_vars.labl, wcs);
  }
}

/* Handles key events. */
static void on_key(int key, int state, unsigned mod_mask, YF_UNUSED void *arg) {
  if (state == YF_KEYSTATE_RELEASED)
    return;

  if (l_vars.input.insert) {
    const int shft = mod_mask & YF_KEYMOD_SHIFT;
    const int caps = mod_mask & YF_KEYMOD_CAPSLOCK ? !shft : shft;
    switch (key) {
      case YF_KEY_ESC:
        l_vars.input.insert = 0;
        break;
      case YF_KEY_BACKSPACE:
        insert_wc(L'\b');
        break;
      case YF_KEY_RETURN:
        insert_wc(L'\n');
        break;
      case YF_KEY_SPACE:
        insert_wc(L' ');
        break;
      case YF_KEY_A:
        insert_wc(caps ? L'A' : L'a');
        break;
      case YF_KEY_B:
        insert_wc(caps ? L'B' : L'b');
        break;
      case YF_KEY_C:
        insert_wc(caps ? L'C' : L'c');
        break;
      case YF_KEY_D:
        insert_wc(caps ? L'D' : L'd');
        break;
      case YF_KEY_E:
        insert_wc(caps ? L'E' : L'e');
        break;
      case YF_KEY_F:
        insert_wc(caps ? L'F' : L'f');
        break;
      case YF_KEY_G:
        insert_wc(caps ? L'G' : L'g');
        break;
      case YF_KEY_H:
        insert_wc(caps ? L'H' : L'h');
        break;
      case YF_KEY_I:
        insert_wc(caps ? L'I' : L'i');
        break;
      case YF_KEY_J:
        insert_wc(caps ? L'J' : L'j');
        break;
      case YF_KEY_K:
        insert_wc(caps ? L'K' : L'k');
        break;
      case YF_KEY_L:
        insert_wc(caps ? L'L' : L'l');
        break;
      case YF_KEY_M:
        insert_wc(caps ? L'M' : L'm');
        break;
      case YF_KEY_N:
        insert_wc(caps ? L'N' : L'n');
        break;
      case YF_KEY_O:
        insert_wc(caps ? L'O' : L'o');
        break;
      case YF_KEY_P:
        insert_wc(caps ? L'P' : L'p');
        break;
      case YF_KEY_Q:
        insert_wc(caps ? L'Q' : L'q');
        break;
      case YF_KEY_R:
        insert_wc(caps ? L'R' : L'r');
        break;
      case YF_KEY_S:
        insert_wc(caps ? L'S' : L's');
        break;
      case YF_KEY_T:
        insert_wc(caps ? L'T' : L't');
        break;
      case YF_KEY_U:
        insert_wc(caps ? L'U' : L'u');
        break;
      case YF_KEY_V:
        insert_wc(caps ? L'V' : L'v');
        break;
      case YF_KEY_W:
        insert_wc(caps ? L'W' : L'w');
        break;
      case YF_KEY_X:
        insert_wc(caps ? L'X' : L'x');
        break;
      case YF_KEY_Y:
        insert_wc(caps ? L'Y' : L'y');
        break;
      case YF_KEY_Z:
        insert_wc(caps ? L'Z' : L'z');
        break;
      case YF_KEY_1:
        insert_wc(shft ? L'!' : L'1');
        break;
      case YF_KEY_2:
        insert_wc(shft ? L'@' : L'2');
        break;
      case YF_KEY_3:
        insert_wc(shft ? L'#' : L'3');
        break;
      case YF_KEY_4:
        insert_wc(shft ? L'$' : L'4');
        break;
      case YF_KEY_5:
        insert_wc(shft ? L'%' : L'5');
        break;
      case YF_KEY_6:
        insert_wc(shft ? L'^' : L'6');
        break;
      case YF_KEY_7:
        insert_wc(shft ? L'&' : L'7');
        break;
      case YF_KEY_8:
        insert_wc(shft ? L'*' : L'8');
        break;
      case YF_KEY_9:
        insert_wc(shft ? L'(' : L'9');
        break;
      case YF_KEY_0:
        insert_wc(shft ? L')' : L'0');
        break;
      case YF_KEY_MINUS:
        insert_wc(shft ? L'_' : L'-');
        break;
      case YF_KEY_EQUAL:
        insert_wc(shft ? L'+' : L'=');
        break;
      case YF_KEY_GRAVE:
        insert_wc(shft ? L'~' : L'`');
        break;
      case YF_KEY_LBRACKET:
        insert_wc(shft ? L'{' : L'[');
        break;
      case YF_KEY_RBRACKET:
        insert_wc(shft ? L'}' : L']');
        break;
      case YF_KEY_BACKSLASH:
        insert_wc(shft ? L'|' : L'\\');
        break;
      case YF_KEY_SEMICOLON:
        insert_wc(shft ? L':' : L';');
        break;
      case YF_KEY_APOSTROPHE:
        insert_wc(shft ? L'"' : L'\'');
        break;
      case YF_KEY_COMMA:
        insert_wc(shft ? L'<' : L',');
        break;
      case YF_KEY_DOT:
        insert_wc(shft ? L'>' : L'.');
        break;
      case YF_KEY_SLASH:
        insert_wc(shft ? L'?' : L'/');
        break;
    }
  } else {
    switch (key) {
      case YF_KEY_I:
        l_vars.input.insert = 1;
        break;
      default:
        l_vars.input.quit = 1;
    }
  }
}

/* Updates content. */
static void update(double elapsed_time) {
  printf("update (%.4f)\n", elapsed_time);

  if (l_vars.input.quit) {
    printf("quit\n");
    yf_view_stop(l_vars.view);
  }
}

/* Tests label rendering. */
int yf_test_label(void) {
  YF_evtfn evtfn = {.key_kb = on_key};
  yf_setevtfn(YF_EVT_KEYKB, evtfn, NULL);

  l_vars.win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
  assert(l_vars.win != NULL);

  l_vars.view = yf_view_init(l_vars.win);
  assert(l_vars.view != NULL);

  l_vars.scn = yf_scene_init();
  assert(l_vars.scn != NULL);

  l_vars.labl = yf_label_init();
  assert(l_vars.labl != NULL);

  l_vars.font = yf_font_init(YF_FILETYPE_TTF, "tmp/font.ttf");
  assert(l_vars.font != NULL);

  yf_label_setfont(l_vars.labl, l_vars.font);
  yf_label_setstr(l_vars.labl, L"Label");
  yf_label_setpt(l_vars.labl, 44);

  yf_node_insert(yf_scene_getnode(l_vars.scn), yf_label_getnode(l_vars.labl));

  yf_view_setscene(l_vars.view, l_vars.scn);

  if (yf_view_start(l_vars.view, YF_FPS, update) != 0)
    assert(0);

  yf_view_deinit(l_vars.view);
  yf_scene_deinit(l_vars.scn);
  yf_label_deinit(l_vars.labl);
  yf_font_deinit(l_vars.font);
  yf_window_deinit(l_vars.win);

  return 0;
}
