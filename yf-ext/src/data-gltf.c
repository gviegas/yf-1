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
#include <errno.h>
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

#define YF_GLTF_PROP(prop) (#prop)

/* Type defining the 'asset' property. */
typedef struct {
  char *copyright;
  char *generator;
  char *version;
  char *min_version;
} L_asset;

/* Type defining the 'scenes' property. */
typedef struct {
  struct {
    size_t *nodes;
    size_t node_n;
    char *name;
  } *v;
  size_t n;
} L_scenes;

/* Type defining the 'nodes' property. */
typedef struct {
  struct {
    size_t mesh;
    char *name;
  } *v;
  size_t n;
} L_nodes;

/* Type defining the 'meshes.primitives' property. */
typedef struct {
  struct {
#define YF_GLTF_ATTR_POS  0
#define YF_GLTF_ATTR_NORM 1
#define YF_GLTF_ATTR_TEX0 2
#define YF_GLTF_ATTR_N    3
    size_t attributes[YF_GLTF_ATTR_N];
    size_t indices;
    size_t materials;
  } *v;
  size_t n;
} L_primitives;

/* Type defining the 'meshes' property. */
typedef struct {
  struct {
    L_primitives primitives;
    char *name;
  } *v;
  size_t n;
} L_meshes;

/* Type defining the root glTF object. */
typedef struct {
  L_asset asset;
  size_t scene;
  L_scenes scenes;
  L_nodes nodes;
  /* TODO */
} L_gltf;

/* Structured glTF content parsing functions. */
static int parse_gltf(FILE *file, L_symbol *symbol, L_gltf *gltf);
static int parse_asset(FILE *file, L_symbol *symbol, L_asset *asset);
static int parse_scene(FILE *file, L_symbol *symbol, size_t *scene);
static int parse_scenes(FILE *file, L_symbol *symbol, L_scenes *scenes);
static int parse_scenes_i(FILE *file, L_symbol *symbol,
    L_scenes *scenes, size_t index);
static int parse_nodes(FILE *file, L_symbol *symbol, L_nodes *nodes);
static int parse_nodes_i(FILE *file, L_symbol *symbol,
    L_nodes *nodes, size_t index);
static int parse_primitives(FILE *file, L_symbol *symbol,
    L_primitives *primitives);
static int parse_primitives_i(FILE *file, L_symbol *symbol,
    L_primitives *primitives, size_t index);
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
    //fclose(file);
    //return -1;
  }

  ////////////////////
  puts("glTF.asset:");
  printf(" copyright:  %s\n", gltf.asset.copyright);
  printf(" generator:  %s\n", gltf.asset.generator);
  printf(" version:    %s\n", gltf.asset.version);
  printf(" minVersion: %s\n", gltf.asset.min_version);

  puts("glTF.scene:");
  printf(" #: %ld\n", gltf.scene);

  puts("glTF.scenes:");
  printf(" n: %lu\n", gltf.scenes.n);
  for (size_t i = 0; i < gltf.scenes.n; ++i) {
    printf(" scene '%s': [ ", gltf.scenes.v[i].name);
    for (size_t j = 0; j < gltf.scenes.v[i].node_n; ++j)
      printf("%lu ", gltf.scenes.v[i].nodes[j]);
    puts("]");
  }

  puts("glTF.nodes:");
  printf(" n: %lu\n", gltf.nodes.n);
  for (size_t i = 0; i < gltf.nodes.n; ++i) {
    printf(" node '%s':\n", gltf.nodes.v[i].name);
    printf("  mesh: %lu\n", gltf.nodes.v[i].mesh);
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
        if (strcmp(YF_GLTF_PROP("asset"), symbol->tokens) == 0) {
          if (parse_asset(file, symbol, &gltf->asset) != 0)
            return -1;
        } else if (strcmp(YF_GLTF_PROP("scene"), symbol->tokens) == 0) {
          if (parse_scene(file, symbol, &gltf->scene) != 0)
            return -1;
        } else if (strcmp(YF_GLTF_PROP("scenes"), symbol->tokens) == 0) {
          if (parse_scenes(file, symbol, &gltf->scenes) != 0)
            return -1;
        } else if (strcmp(YF_GLTF_PROP("nodes"), symbol->tokens) == 0) {
          if (parse_nodes(file, symbol, &gltf->nodes) != 0)
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
  assert(strcmp(symbol->tokens, YF_GLTF_PROP("asset")) == 0);

  next_symbol(file, symbol); /* : */
  next_symbol(file, symbol); /* { */

  if (symbol->symbol != YF_SYMBOL_OP || symbol->tokens[0] != '{')
    return -1;

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp(YF_GLTF_PROP("copyright"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          asset->copyright = malloc(1+strlen(symbol->tokens));
          if (asset->copyright == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
          }
          strcpy(asset->copyright, symbol->tokens);
        } else if (strcmp(YF_GLTF_PROP("generator"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          asset->generator = malloc(1+strlen(symbol->tokens));
          if (asset->generator == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
          }
          strcpy(asset->generator, symbol->tokens);
        } else if (strcmp(YF_GLTF_PROP("version"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          asset->version = malloc(1+strlen(symbol->tokens));
          if (asset->version == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
          }
          strcpy(asset->version, symbol->tokens);
        } else if (strcmp(YF_GLTF_PROP("minVersion"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          asset->min_version = malloc(1+strlen(symbol->tokens));
          if (asset->min_version == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
          }
          strcpy(asset->min_version, symbol->tokens);
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

static int parse_scene(FILE *file, L_symbol *symbol, size_t *scene) {
  assert(!feof(file));
  assert(symbol != NULL);
  assert(scene != NULL);
  assert(symbol->symbol == YF_SYMBOL_STR);
  assert(strcmp(symbol->tokens, YF_GLTF_PROP("scene")) == 0);

  next_symbol(file, symbol); /* : */
  next_symbol(file, symbol);

  if (symbol->symbol != YF_SYMBOL_NUM)
    return -1;

  errno = 0;
  *scene = strtoll(symbol->tokens, NULL, 0);
  return errno;
}

static int parse_scenes(FILE *file, L_symbol *symbol, L_scenes *scenes) {
  assert(!feof(file));
  assert(symbol != NULL);
  assert(scenes != NULL);
  assert(symbol->symbol == YF_SYMBOL_STR);
  assert(strcmp(symbol->tokens, YF_GLTF_PROP("scenes")) == 0);

  next_symbol(file, symbol); /* : */
  next_symbol(file, symbol); /* [ */

  size_t i = 0;

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_OP:
        if (symbol->tokens[0] == '{') {
          if (i == scenes->n) {
            const size_t n = i == 0 ? 1 : i<<1;
            void *tmp = realloc(scenes->v, n*sizeof *scenes->v);
            if (tmp == NULL) {
              yf_seterr(YF_ERR_NOMEM, __func__);
              return -1;
            }
            scenes->v = tmp;
            scenes->n = n;
            memset(scenes->v+i, 0, (n-i)*sizeof *scenes->v);
            if (parse_scenes_i(file, symbol, scenes, i++) != 0)
              return -1;
          }
        } else if (symbol->tokens[0] == ']') {
          if (i < scenes->n) {
            scenes->n = i;
            void *tmp = realloc(scenes->v, i*sizeof *scenes->v);
            if (tmp != NULL)
              scenes->v = tmp;
          }
          return 0;
        }
        break;

      default:
        return -1;
    }
  } while (1);

  return 0;
}

static int parse_scenes_i(FILE *file, L_symbol *symbol,
    L_scenes *scenes, size_t index)
{
  assert(!feof(file));
  assert(symbol != NULL);
  assert(scenes != NULL);
  assert(index < scenes->n);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '{');

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp(YF_GLTF_PROP("nodes"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* [ */
          size_t i = 0;
          long long node;
          do {
            switch (next_symbol(file, symbol)) {
              case YF_SYMBOL_NUM:
                errno = 0;
                node = strtoll(symbol->tokens, NULL, 0);
                if (errno != 0) {
                  yf_seterr(YF_ERR_OTHER, __func__);
                  return -1;
                }
                if (i == scenes->v[index].node_n) {
                  const size_t n = i == 0 ? 1 : i<<1;
                  void *tmp = realloc(scenes->v[index].nodes,
                      n*sizeof *scenes->v[index].nodes);
                  if (tmp == NULL) {
                    yf_seterr(YF_ERR_NOMEM, __func__);
                    return -1;
                  }
                  scenes->v[index].nodes = tmp;
                  scenes->v[index].node_n = n;
                }
                scenes->v[index].nodes[i++] = node;
                break;

              case YF_SYMBOL_OP:
                if (symbol->tokens[0] == ']') {
                  if (i < scenes->v[index].node_n) {
                    scenes->v[index].node_n = i;
                    void *tmp = realloc(scenes->v[index].nodes,
                        i*sizeof *scenes->v[index].nodes);
                    if (tmp != NULL)
                      scenes->v[index].nodes = tmp;
                  }
                }
                break;

              default:
                return -1;
            }
          } while (symbol->symbol != YF_SYMBOL_OP || symbol->tokens[0] != ']');
        } else if (strcmp(YF_GLTF_PROP("name"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          scenes->v[index].name = malloc(1+strlen(symbol->tokens));
          if (scenes->v[index].name == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
          }
          strcpy(scenes->v[index].name, symbol->tokens);
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

static int parse_nodes(FILE *file, L_symbol *symbol, L_nodes *nodes) {
  assert(!feof(file));
  assert(symbol != NULL);
  assert(nodes != NULL);
  assert(symbol->symbol == YF_SYMBOL_STR);
  assert(strcmp(symbol->tokens, YF_GLTF_PROP("nodes")) == 0);

  next_symbol(file, symbol); /* : */
  next_symbol(file, symbol); /* [ */

  size_t i = 0;

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_OP:
        if (symbol->tokens[0] == '{') {
          if (i == nodes->n) {
            const size_t n = i == 0 ? 1 : i<<1;
            void *tmp = realloc(nodes->v, n*sizeof *nodes->v);
            if (tmp == NULL) {
              yf_seterr(YF_ERR_NOMEM, __func__);
              return -1;
            }
            nodes->v = tmp;
            nodes->n = n;
            memset(nodes->v+i, 0, (n-i)*sizeof *nodes->v);
            if (parse_nodes_i(file, symbol, nodes, i++) != 0)
              return -1;
          }
        } else if (symbol->tokens[0] == ']') {
          if (i < nodes->n) {
            nodes->n = i;
            void *tmp = realloc(nodes->v, i*sizeof *nodes->v);
            if (tmp != NULL)
              nodes->v = tmp;
          }
          return 0;
        }
        break;

      default:
        return -1;
    }
  } while (1);

  return 0;
}

static int parse_nodes_i(FILE *file, L_symbol *symbol,
    L_nodes *nodes, size_t index)
{
  assert(!feof(file));
  assert(symbol != NULL);
  assert(nodes != NULL);
  assert(index < nodes->n);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '{');

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp(YF_GLTF_PROP("mesh"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          errno = 0;
          nodes->v[index].mesh = strtoll(symbol->tokens, NULL, 0);
          if (errno != 0) {
            yf_seterr(YF_ERR_OTHER, __func__);
            return -1;
          }
        } else if (strcmp(YF_GLTF_PROP("name"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          nodes->v[index].name = malloc(1+strlen(symbol->tokens));
          if (nodes->v[index].name == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
          }
          strcpy(nodes->v[index].name, symbol->tokens);
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

static int parse_primitives(FILE *file, L_symbol *symbol,
    L_primitives *primitives)
{
  assert(!feof(file));
  assert(symbol != NULL);
  assert(primitives != NULL);
  assert(symbol->symbol == YF_SYMBOL_STR);
  assert(strcmp(symbol->tokens, YF_GLTF_PROP("primitives")) == 0);

  next_symbol(file, symbol); /* : */
  next_symbol(file, symbol); /* [ */

  size_t i = 0;

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_OP:
        if (symbol->tokens[0] == '{') {
          if (i == primitives->n) {
            const size_t n = i == 0 ? 1 : i<<1;
            void *tmp = realloc(primitives->v, n*sizeof *primitives->v);
            if (tmp == NULL) {
              yf_seterr(YF_ERR_NOMEM, __func__);
              return -1;
            }
            primitives->v = tmp;
            primitives->n = n;
            memset(primitives->v+i, 0, (n-i)*sizeof *primitives->v);
            if (parse_primitives_i(file, symbol, primitives, i++) != 0)
              return -1;
          }
        } else if (symbol->tokens[0] == ']') {
          if (i < primitives->n) {
            primitives->n = i;
            void *tmp = realloc(primitives->v, i*sizeof *primitives->v);
            if (tmp != NULL)
              primitives->v = tmp;
          }
          return 0;
        }
        break;

      default:
        return -1;
    }
  } while (1);

  return 0;
}

static int parse_primitives_i(FILE *file, L_symbol *symbol,
    L_primitives *primitives, size_t index)
{
  assert(!feof(file));
  assert(symbol != NULL);
  assert(primitives != NULL);
  assert(index < primitives->n);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '{');

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp(YF_GLTF_PROP("attributes"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* { */
          do {
            switch (next_symbol(file, symbol)) {
              case YF_SYMBOL_STR:
                if (strcmp(YF_GLTF_PROP("POSITION"), symbol->tokens) == 0) {
                  next_symbol(file, symbol); /* : */
                  next_symbol(file, symbol);
                  errno = 0;
                  primitives->v[index].attributes[YF_GLTF_ATTR_POS] =
                    strtoll(symbol->tokens, NULL, 0);
                  if (errno != 0) {
                    yf_seterr(YF_ERR_OTHER, __func__);
                    return -1;
                  }
                } else if (strcmp(YF_GLTF_PROP("NORMAL"), symbol->tokens)
                  == 0)
                {
                  next_symbol(file, symbol); /* : */
                  next_symbol(file, symbol);
                  errno = 0;
                  primitives->v[index].attributes[YF_GLTF_ATTR_NORM] =
                    strtoll(symbol->tokens, NULL, 0);
                  if (errno != 0) {
                    yf_seterr(YF_ERR_OTHER, __func__);
                    return -1;
                  }
                } else if (strcmp(YF_GLTF_PROP("TEXCOORD_0"), symbol->tokens)
                  == 0)
                {
                  next_symbol(file, symbol); /* : */
                  next_symbol(file, symbol);
                  errno = 0;
                  primitives->v[index].attributes[YF_GLTF_ATTR_TEX0] =
                    strtoll(symbol->tokens, NULL, 0);
                  if (errno != 0) {
                    yf_seterr(YF_ERR_OTHER, __func__);
                    return -1;
                  }
                }
                break;

              case YF_SYMBOL_OP:
                break;

              default:
                return -1;
            }
          } while (symbol->symbol != YF_SYMBOL_OP || symbol->tokens[0] != '}');
        } else if (strcmp(YF_GLTF_PROP("indices"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          errno = 0;
          primitives->v[index].indices = strtoll(symbol->tokens, NULL, 0);
          if (errno != 0) {
            yf_seterr(YF_ERR_OTHER, __func__);
            return -1;
          }
        } else if (strcmp(YF_GLTF_PROP("materials"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          errno = 0;
          primitives->v[index].materials = strtoll(symbol->tokens, NULL, 0);
          if (errno != 0) {
            yf_seterr(YF_ERR_OTHER, __func__);
            return -1;
          }
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
