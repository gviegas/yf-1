/*
 * YF
 * data-gltf.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <yf/com/yf-error.h>

#include "data-gltf.h"

#define YF_SYMBOL_STR  0
#define YF_SYMBOL_OP   1
#define YF_SYMBOL_NUM  2
#define YF_SYMBOL_BOOL 3
#define YF_SYMBOL_END  4
#define YF_SYMBOL_ERR  5

#define YF_MAXTOKENS 512

/* Type defining a symbol. */
typedef struct {
  int symbol;
  char tokens[YF_MAXTOKENS];
} L_symbol;

/* Gets the next symbol from a file stream. */
static int next_symbol(FILE *file, L_symbol *symbol);

int yf_loadgltf(const char *pathname, void *data) {
  if (pathname == NULL) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
  }

  FILE *file = fopen(pathname, "r");
  if (file == NULL) {
    yf_seterr(YF_ERR_NOFILE, __func__);
    return -1;
  }

  L_symbol symbol = {0};

  ////////////////////
  while (!feof(file)) {
    next_symbol(file, &symbol);
    printf("> [%d] %s\n", symbol.symbol, symbol.tokens);
  }
  ////////////////////

  fclose(file);
  return 0;
}

static int next_symbol(FILE *file, L_symbol *symbol) {
  int c;
  do c = getc(file); while (isspace(c));

  symbol->tokens[0] = c;
  size_t i = 0;

  switch (c) {
    case '"':
      while (++i < YF_MAXTOKENS-1) {
        c = getc(file);
        symbol->tokens[i] = c;
        if (c == '"') {
          symbol->symbol = YF_SYMBOL_STR;
          ++i;
          break;
        }
        if (c == EOF) {
          symbol->symbol = YF_SYMBOL_ERR;
          break;
        }
      }
      break;

    case ':':
    case ',':
    case '[':
    case ']':
    case '{':
    case '}':
      symbol->symbol = YF_SYMBOL_OP;
      ++i;
      break;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      while (++i < YF_MAXTOKENS-1) {
        do c = getc(file); while (isspace(c));
        if (isxdigit(c) || c == '.') {
          symbol->tokens[i] = c;
          continue;
        }
        if (c != EOF)
          ungetc(c, file);
        break;
      }
      symbol->symbol = YF_SYMBOL_NUM;
      break;

    case 't':
      while (++i < YF_MAXTOKENS-1) {
        c = getc(file);
        if (islower(c))
          symbol->tokens[i] = c;
        else
          break;
      }
      if (c != EOF)
        ungetc(c, file);
      symbol->tokens[i] = '\0';
      if (strcmp("true", symbol->tokens) == 0)
        symbol->symbol = YF_SYMBOL_BOOL;
      else
        symbol->symbol = YF_SYMBOL_ERR;
      break;

    case 'f':
      while (++i < YF_MAXTOKENS-1) {
        c = getc(file);
        if (islower(c))
          symbol->tokens[i] = c;
        else
          break;
      }
      if (c != EOF)
        ungetc(c, file);
      symbol->tokens[i] = '\0';
      if (strcmp("false", symbol->tokens) == 0)
        symbol->symbol = YF_SYMBOL_BOOL;
      else
        symbol->symbol = YF_SYMBOL_ERR;
      break;

    case EOF:
      symbol->symbol = YF_SYMBOL_END;
      break;

    default:
      symbol->symbol = YF_SYMBOL_ERR;
  }

  symbol->tokens[i] = '\0';
  return symbol->symbol;
}
