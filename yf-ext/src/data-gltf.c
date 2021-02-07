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
#define YF_SYMBOL_NULL 4
#define YF_SYMBOL_END  5
#define YF_SYMBOL_ERR  6

#define YF_MAXTOKENS 512

/* Type defining a symbol. */
typedef struct {
  int symbol;
  char tokens[YF_MAXTOKENS];
} L_symbol;

/* Gets the next symbol from a file stream. */
static int next_symbol(FILE *file, L_symbol *symbol);

#define YF_GLTF_MAKEPROP(prop) (#prop)

/* Type defining the asset property. */
typedef struct {
  char *copyright;
  char *generator;
  char *version;
  char *min_version;
} L_asset;

/* Type defining the root glTF object. */
typedef struct {
  L_asset asset;
  /* TODO */
} L_gltf;

/* Structured glTF content parsing functions. */
static int parse_gltf(FILE *file, L_symbol *symbol, L_gltf *gltf);
static int parse_asset(FILE *file, L_symbol *symbol, L_asset *asset);
/* TODO */

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
  next_symbol(file, &symbol);
  if (symbol.symbol != YF_SYMBOL_OP && symbol.tokens[0] != '{') {
    /* TODO: Set error (either here or on caller). */
    fclose(file);
    return -1;
  }

  L_gltf gltf = {0};
  if (parse_gltf(file, &symbol, &gltf) != 0) {
    /* TODO: Set error; dealloc. */
    fclose(file);
    return -1;
  }

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

    case '-':
    case '+':
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
        c = getc(file);
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

    case 'n':
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
      if (strcmp("null", symbol->tokens) == 0)
        symbol->symbol = YF_SYMBOL_NULL;
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

static int parse_gltf(FILE *file, L_symbol *symbol, L_gltf *gltf) {
  assert(!feof(file));
  assert(symbol != NULL);
  assert(gltf != NULL);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '{');

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp(YF_GLTF_MAKEPROP("asset"), symbol->tokens) == 0) {
          if (parse_asset(file, symbol, &gltf->asset) != 0)
            return -1;
        } else {
          /* TODO */
          printf("! %s parsing unimplemented\n", symbol->tokens);
        }
        break;

      case YF_SYMBOL_OP:
        if (symbol->tokens[0] == '}')
          return 0;
        break;

      default:
        return -1;
    }
  } while (1);

  return 0;
}

static int parse_asset(FILE *file, L_symbol *symbol, L_asset *asset) {
  assert(!feof(file));
  assert(symbol != NULL);
  assert(asset != NULL);
  assert(symbol->symbol == YF_SYMBOL_STR);
  assert(strcmp(symbol->tokens, YF_GLTF_MAKEPROP("asset")) == 0);

  /* TODO */
  printf("%s\n", __func__);

  return 0;
}
