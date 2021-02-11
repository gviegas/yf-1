/*
 * YF
 * data-gltf.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#include <yf/com/yf-error.h>

#include "data-gltf.h"
#include "matrix.h"
#include "vertex.h"

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

/* Gets the next symbol from a file stream.
   This function returns 'symbol->symbol'. Upon failure, the value returned
   is equal to 'YF_SYMBOL_ERR' - the global error variable is not set. */
static int next_symbol(FILE *file, L_symbol *symbol);

/* Type defining the 'glTF.asset' property. */
typedef struct {
  char *copyright;
  char *generator;
  char *version;
  char *min_version;
} L_asset;

/* Type defining the 'glTF.scenes' property. */
typedef struct {
  struct {
    size_t *nodes;
    size_t node_n;
    char *name;
  } *v;
  size_t n;
} L_scenes;

/* Type defining the 'glTF.nodes' property. */
typedef struct {
  struct {
    size_t *children;
    size_t child_n;
    size_t camera;
    size_t mesh;
#define YF_GLTF_XFORM_NONE 0
#define YF_GLTF_XFORM_M    0x01
#define YF_GLTF_XFORM_T    0x02
#define YF_GLTF_XFORM_R    0x04
#define YF_GLTF_XFORM_S    0x08
    unsigned xform_mask;
    union {
      YF_mat4 matrix;
      struct { YF_vec3 t; YF_vec4 r; YF_vec3 s; } trs;
    };
    char *name;
  } *v;
  size_t n;
} L_nodes;

/* Type defining the 'glTF.meshes.primitives' property. */
typedef struct {
  struct {
#define YF_GLTF_ATTR_POS  0
#define YF_GLTF_ATTR_NORM 1
#define YF_GLTF_ATTR_TEX0 2
#define YF_GLTF_ATTR_N    3
    size_t attributes[YF_GLTF_ATTR_N];
    size_t indices;
    size_t material;
  } *v;
  size_t n;
} L_primitives;

/* Type defining the 'glTF.meshes' property. */
typedef struct {
  struct {
    L_primitives primitives;
    char *name;
  } *v;
  size_t n;
} L_meshes;

/* Type defining the 'glTF.materials' property. */
typedef struct {
  struct {
    struct {
      YF_vec4 base_clr_fac;
      YF_float metallic_fac;
      YF_float roughness_fac;
    } pbrmr;
    int double_sided;
    char *name;
  } *v;
  size_t n;
} L_materials;

/* Type defining the 'glTF.accessors' property. */
typedef struct {
  struct {
    size_t buffer_view;
    size_t byte_off;
    size_t count;
#define YF_GLTF_COMP_BYTE   5120
#define YF_GLTF_COMP_UBYTE  5121
#define YF_GLTF_COMP_SHORT  5122
#define YF_GLTF_COMP_USHORT 5123
#define YF_GLTF_COMP_UINT   5125
#define YF_GLTF_COMP_FLOAT  5126
    int comp_type;
#define YF_GLTF_TYPE_SCALAR 1
#define YF_GLTF_TYPE_VEC2   2
#define YF_GLTF_TYPE_VEC3   3
#define YF_GLTF_TYPE_VEC4   4
#define YF_GLTF_TYPE_MAT2   22
#define YF_GLTF_TYPE_MAT3   33
#define YF_GLTF_TYPE_MAT4   44
    int type;
    union {
      /* XXX: May not suffice for uint32. */
      YF_float s;
      YF_vec2 v2;
      YF_vec3 v3;
      YF_vec4 v4;
      YF_mat2 m2;
      YF_mat3 m3;
      YF_mat4 m4;
    } min, max;
    char *name;
  } *v;
  size_t n;
} L_accessors;

/* Type defining the 'glTF.bufferViews' property. */
typedef struct {
  struct {
    size_t buffer;
    size_t byte_off;
    size_t byte_len;
    size_t byte_strd;
    char *name;
  } *v;
  size_t n;
} L_bufferviews;

/* Type defining the 'glTF.buffers' property. */
typedef struct {
  struct {
    size_t byte_len;
    char *uri;
    char *name;
  } *v;
  size_t n;
} L_buffers;

/* Type defining the root glTF object. */
typedef struct {
  L_asset asset;
  size_t scene;
  L_scenes scenes;
  L_nodes nodes;
  L_meshes meshes;
  L_materials materials;
  L_accessors accessors;
  L_bufferviews bufferviews;
  L_buffers buffers;
  /* TODO: Other properties. */
} L_gltf;

/* Consumes the current property.
   This allows unknown/unimplemented properties to be ignored. */
static int consume_prop(FILE *file, L_symbol *symbol);

/* Parses the root glTF object. */
static int parse_gltf(FILE *file, L_symbol *symbol, L_gltf *gltf);

/* Parses the 'glTF.asset' property. */
static int parse_asset(FILE *file, L_symbol *symbol, L_asset *asset);

/* Parses the 'glTF.scene' property. */
static int parse_scene(FILE *file, L_symbol *symbol, size_t *scene);

/* Parses the 'glTF.scenes' property. */
static int parse_scenes(FILE *file, L_symbol *symbol, L_scenes *scenes);

/* Parses a given element from the 'glTF.scenes' property. */
static int parse_scenes_i(FILE *file, L_symbol *symbol,
    L_scenes *scenes, size_t index);

/* Parses the 'glTF.nodes' property. */
static int parse_nodes(FILE *file, L_symbol *symbol, L_nodes *nodes);

/* Parses a given element from the 'glTF.nodes' property. */
static int parse_nodes_i(FILE *file, L_symbol *symbol,
    L_nodes *nodes, size_t index);

/* Parses the 'glTF.meshes.primitives' property. */
static int parse_primitives(FILE *file, L_symbol *symbol,
    L_primitives *primitives);

/* Parses a given element from the 'glTF.meshes.primitives' property. */
static int parse_primitives_i(FILE *file, L_symbol *symbol,
    L_primitives *primitives, size_t index);

/* Parses the 'glTF.meshes' property. */
static int parse_meshes(FILE *file, L_symbol *symbol, L_meshes *meshes);

/* Parses a given element from the 'glTF.meshes' property. */
static int parse_meshes_i(FILE *file, L_symbol *symbol,
    L_meshes *meshes, size_t index);

/* Parses the 'glTF.materials' property. */
static int parse_materials(FILE *file, L_symbol *symbol,
    L_materials *materials);

/* Parses a given element from the 'glTF.materials' property. */
static int parse_materials_i(FILE *file, L_symbol *symbol,
    L_materials *materials, size_t index);

/* Parses the 'glTF.accessors' property. */
static int parse_accessors(FILE *file, L_symbol *symbol,
    L_accessors *accessors);

/* Parses a given element from the 'glTF.accessors' property. */
static int parse_accessors_i(FILE *file, L_symbol *symbol,
    L_accessors *accessors, size_t index);

/* Parses the 'glTF.bufferViews' property. */
static int parse_bufferviews(FILE *file, L_symbol *symbol,
    L_bufferviews *bufferviews);

/* Parses a given element from the 'glTF.bufferViews' property. */
static int parse_bufferviews_i(FILE *file, L_symbol *symbol,
    L_bufferviews *bufferviews, size_t index);

/* Parses the 'glTF.buffers' property. */
static int parse_buffers(FILE *file, L_symbol *symbol, L_buffers *buffers);

/* Parses a given element from the 'glTF.buffers' property. */
static int parse_buffers_i(FILE *file, L_symbol *symbol,
    L_buffers *buffers, size_t index);

/* Loads a single mesh from glTF contents. */
static int load_meshdt(const L_gltf *gltf, YF_meshdt *data);

/* Deinitializes glTF contents. */
static void deinit_gltf(L_gltf *gltf);

#ifdef YF_DEVEL
static void print_gltf(const L_gltf *gltf);
#endif

int yf_loadgltf(const char *pathname, YF_meshdt *data) {
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
  /* TODO: .glb */
  if (symbol.symbol != YF_SYMBOL_OP && symbol.tokens[0] != '{') {
    yf_seterr(YF_ERR_INVFILE, __func__);
    fclose(file);
    return -1;
  }

  L_gltf gltf = {0};
  if (parse_gltf(file, &symbol, &gltf) != 0) {
    deinit_gltf(&gltf);
    fclose(file);
    return -1;
  }

#ifdef YF_DEVEL
  print_gltf(&gltf);
#endif

  if (load_meshdt(&gltf, data) != 0) {
    deinit_gltf(&gltf);
    fclose(file);
    return -1;
  }

  deinit_gltf(&gltf);
  fclose(file);
  return 0;
}

static int next_symbol(FILE *file, L_symbol *symbol) {
  static_assert(YF_MAXTOKENS > 1);

  int c;
  do c = getc(file); while (isspace(c));

  symbol->tokens[0] = c;
  size_t i = 0;

  switch (c) {
    case '"':
      /* XXX: Delim. quotation marks not stored. */
      do {
        c = getc(file);
        /* TODO: Handle escape characters. */
        symbol->tokens[i] = c;
        if (c == '"') {
          symbol->symbol = YF_SYMBOL_STR;
          break;
        }
        if (c == EOF) {
          symbol->symbol = YF_SYMBOL_ERR;
          break;
        }
      } while (++i < YF_MAXTOKENS-1);
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

static int consume_prop(FILE *file, L_symbol *symbol) {
  assert(!feof(file));
  assert(symbol != NULL);
  assert(symbol->symbol == YF_SYMBOL_STR);

  next_symbol(file, symbol); /* : */

  if (symbol->symbol != YF_SYMBOL_OP || symbol->tokens[0] != ':') {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

  switch (next_symbol(file, symbol)) {
    case YF_SYMBOL_STR:
    case YF_SYMBOL_NUM:
    case YF_SYMBOL_BOOL:
    case YF_SYMBOL_NULL:
      break;

    case YF_SYMBOL_OP: {
      char cl, op = symbol->tokens[0];
      switch (op) {
        case '[':
          cl = ']';
          break;
        case '{':
          cl = '}';
          break;
        default:
          yf_seterr(YF_ERR_INVFILE, __func__);
          return -1;
      }
      int n = 1;
      do {
        switch (next_symbol(file, symbol)) {
          case YF_SYMBOL_OP:
            if (symbol->tokens[0] == op)
              ++n;
            else if (symbol->tokens[0] == cl)
              --n;
            break;

          case YF_SYMBOL_END:
          case YF_SYMBOL_ERR:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
      } while (n > 0);
    } break;

    default:
      yf_seterr(YF_ERR_INVFILE, __func__);
      return -1;
  }

  return 0;
}

static int parse_gltf(FILE *file, L_symbol *symbol, L_gltf *gltf) {
  assert(!feof(file));
  assert(symbol != NULL);
  assert(gltf != NULL);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '{');

  gltf->scene = SIZE_MAX;

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp("asset", symbol->tokens) == 0) {
          if (parse_asset(file, symbol, &gltf->asset) != 0)
            return -1;
        } else if (strcmp("scene", symbol->tokens) == 0) {
          if (parse_scene(file, symbol, &gltf->scene) != 0)
            return -1;
        } else if (strcmp("scenes", symbol->tokens) == 0) {
          if (parse_scenes(file, symbol, &gltf->scenes) != 0)
            return -1;
        } else if (strcmp("nodes", symbol->tokens) == 0) {
          if (parse_nodes(file, symbol, &gltf->nodes) != 0)
            return -1;
        } else if (strcmp("meshes", symbol->tokens) == 0) {
          if (parse_meshes(file, symbol, &gltf->meshes) != 0)
            return -1;
        } else if (strcmp("materials", symbol->tokens) == 0) {
          if (parse_materials(file, symbol, &gltf->materials) != 0)
            return -1;
        } else if (strcmp("accessors", symbol->tokens) == 0) {
          if (parse_accessors(file, symbol, &gltf->accessors) != 0)
            return -1;
        } else if (strcmp("bufferViews", symbol->tokens) == 0) {
          if (parse_bufferviews(file, symbol, &gltf->bufferviews) != 0)
            return -1;
        } else if (strcmp("buffers", symbol->tokens) == 0) {
          if (parse_buffers(file, symbol, &gltf->buffers) != 0)
            return -1;
        } else {
          if (consume_prop(file, symbol) != 0)
            return -1;
        }
        break;

      case YF_SYMBOL_OP:
        if (symbol->tokens[0] == '}')
          return 0;
        break;

      default:
        yf_seterr(YF_ERR_INVFILE, __func__);
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
  assert(strcmp(symbol->tokens, "asset") == 0);

  next_symbol(file, symbol); /* : */
  next_symbol(file, symbol); /* { */

  if (symbol->symbol != YF_SYMBOL_OP || symbol->tokens[0] != '{') {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp("copyright", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          asset->copyright = malloc(1+strlen(symbol->tokens));
          if (asset->copyright == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
          }
          strcpy(asset->copyright, symbol->tokens);
        } else if (strcmp("generator", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          asset->generator = malloc(1+strlen(symbol->tokens));
          if (asset->generator == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
          }
          strcpy(asset->generator, symbol->tokens);
        } else if (strcmp("version", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          asset->version = malloc(1+strlen(symbol->tokens));
          if (asset->version == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
          }
          strcpy(asset->version, symbol->tokens);
        } else if (strcmp("minVersion", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          asset->min_version = malloc(1+strlen(symbol->tokens));
          if (asset->min_version == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
          }
          strcpy(asset->min_version, symbol->tokens);
        } else {
          if (consume_prop(file, symbol) != 0)
            return -1;
        }
        break;

      case YF_SYMBOL_OP:
        if (symbol->tokens[0] == '}')
          return 0;
        break;

      default:
        yf_seterr(YF_ERR_INVFILE, __func__);
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
  assert(strcmp(symbol->tokens, "scene") == 0);

  next_symbol(file, symbol); /* : */
  next_symbol(file, symbol);

  if (symbol->symbol != YF_SYMBOL_NUM) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

  errno = 0;
  *scene = strtoll(symbol->tokens, NULL, 0);
  return errno;
}

static int parse_scenes(FILE *file, L_symbol *symbol, L_scenes *scenes) {
  assert(!feof(file));
  assert(symbol != NULL);
  assert(scenes != NULL);
  assert(symbol->symbol == YF_SYMBOL_STR);
  assert(strcmp(symbol->tokens, "scenes") == 0);

  next_symbol(file, symbol); /* : */
  next_symbol(file, symbol); /* [ */

  if (symbol->symbol != YF_SYMBOL_OP || symbol->tokens[0] != '[') {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

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
          }
          if (parse_scenes_i(file, symbol, scenes, i++) != 0)
            return -1;
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
        yf_seterr(YF_ERR_INVFILE, __func__);
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
        if (strcmp("nodes", symbol->tokens) == 0) {
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
                yf_seterr(YF_ERR_INVFILE, __func__);
                return -1;
            }
          } while (symbol->symbol != YF_SYMBOL_OP || symbol->tokens[0] != ']');
        } else if (strcmp("name", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          scenes->v[index].name = malloc(1+strlen(symbol->tokens));
          if (scenes->v[index].name == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
          }
          strcpy(scenes->v[index].name, symbol->tokens);
        } else {
          if (consume_prop(file, symbol) != 0)
            return -1;
        }
        break;

      case YF_SYMBOL_OP:
        if (symbol->tokens[0] == '}')
          return 0;
        break;

      default:
        yf_seterr(YF_ERR_INVFILE, __func__);
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
  assert(strcmp(symbol->tokens, "nodes") == 0);

  next_symbol(file, symbol); /* : */
  next_symbol(file, symbol); /* [ */

  if (symbol->symbol != YF_SYMBOL_OP || symbol->tokens[0] != '[') {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

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
          }
          if (parse_nodes_i(file, symbol, nodes, i++) != 0)
            return -1;
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
        yf_seterr(YF_ERR_INVFILE, __func__);
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

  nodes->v[index].mesh = SIZE_MAX;
  nodes->v[index].camera = SIZE_MAX;

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp("children", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* [ */
          size_t i = 0;
          long long child;
          do {
            switch (next_symbol(file, symbol)) {
              case YF_SYMBOL_NUM:
                errno = 0;
                child = strtoll(symbol->tokens, NULL, 0);
                if (errno != 0) {
                  yf_seterr(YF_ERR_OTHER, __func__);
                  return -1;
                }
                if (i == nodes->v[index].child_n) {
                  const size_t n = i == 0 ? 1 : i<<1;
                  void *tmp = realloc(nodes->v[index].children,
                      n*sizeof *nodes->v[index].children);
                  if (tmp == NULL) {
                    yf_seterr(YF_ERR_NOMEM, __func__);
                    return -1;
                  }
                  nodes->v[index].children = tmp;
                  nodes->v[index].child_n = n;
                }
                nodes->v[index].children[i++] = child;
                break;

              case YF_SYMBOL_OP:
                if (symbol->tokens[0] == ']') {
                  if (i < nodes->v[index].child_n) {
                    nodes->v[index].child_n = i;
                    void *tmp = realloc(nodes->v[index].children,
                        i*sizeof *nodes->v[index].children);
                    if (tmp != NULL)
                      nodes->v[index].children = tmp;
                  }
                }
                break;

              default:
                yf_seterr(YF_ERR_INVFILE, __func__);
                return -1;
            }
          } while (symbol->symbol != YF_SYMBOL_OP || symbol->tokens[0] != ']');
        } else if (strcmp("camera", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          errno = 0;
          nodes->v[index].camera = strtoll(symbol->tokens, NULL, 0);
          if (errno != 0) {
            yf_seterr(YF_ERR_OTHER, __func__);
            return -1;
          }
        } else if (strcmp("mesh", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          errno = 0;
          nodes->v[index].mesh = strtoll(symbol->tokens, NULL, 0);
          if (errno != 0) {
            yf_seterr(YF_ERR_OTHER, __func__);
            return -1;
          }
        } else if (strcmp("matrix", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* [ */
          nodes->v[index].xform_mask = YF_GLTF_XFORM_M;
          for (size_t i = 0; i < 16; ++i) {
            next_symbol(file, symbol);
            errno = 0;
            nodes->v[index].matrix[i] = strtof(symbol->tokens, NULL);
            if (errno != 0) {
              yf_seterr(YF_ERR_OTHER, __func__);
              return -1;
            }
            next_symbol(file, symbol);
          }
        } else if (strcmp("translation", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* [ */
          nodes->v[index].xform_mask |= YF_GLTF_XFORM_T;
          for (size_t i = 0; i < 3; ++i) {
            next_symbol(file, symbol);
            errno = 0;
            nodes->v[index].trs.t[i] = strtof(symbol->tokens, NULL);
            if (errno != 0) {
              yf_seterr(YF_ERR_OTHER, __func__);
              return -1;
            }
            next_symbol(file, symbol);
          }
        } else if (strcmp("rotation", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* [ */
          nodes->v[index].xform_mask |= YF_GLTF_XFORM_R;
          for (size_t i = 0; i < 4; ++i) {
            next_symbol(file, symbol);
            errno = 0;
            nodes->v[index].trs.r[i] = strtof(symbol->tokens, NULL);
            if (errno != 0) {
              yf_seterr(YF_ERR_OTHER, __func__);
              return -1;
            }
            next_symbol(file, symbol);
          }
        } else if (strcmp("scale", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* [ */
          nodes->v[index].xform_mask |= YF_GLTF_XFORM_S;
          for (size_t i = 0; i < 3; ++i) {
            next_symbol(file, symbol);
            errno = 0;
            nodes->v[index].trs.s[i] = strtof(symbol->tokens, NULL);
            if (errno != 0) {
              yf_seterr(YF_ERR_OTHER, __func__);
              return -1;
            }
            next_symbol(file, symbol);
          }
        } else if (strcmp("name", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          nodes->v[index].name = malloc(1+strlen(symbol->tokens));
          if (nodes->v[index].name == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
          }
          strcpy(nodes->v[index].name, symbol->tokens);
        } else {
          if (consume_prop(file, symbol) != 0)
            return -1;
        }
        break;

      case YF_SYMBOL_OP:
        if (symbol->tokens[0] == '}') {
          const unsigned mask = nodes->v[index].xform_mask;
          if (mask != YF_GLTF_XFORM_NONE && !(mask & YF_GLTF_XFORM_M)) {
            if (!(mask & YF_GLTF_XFORM_T))
              yf_vec3_set(nodes->v[index].trs.t, 0.0);
            if (!(mask & YF_GLTF_XFORM_R)) {
              yf_vec4_set(nodes->v[index].trs.r, 0.0);
              nodes->v[index].trs.r[3] = 1.0;
            }
            if (!(mask & YF_GLTF_XFORM_S))
              yf_vec3_set(nodes->v[index].trs.s, 1.0);
          }
          return 0;
        }
        break;

      default:
        yf_seterr(YF_ERR_INVFILE, __func__);
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
  assert(strcmp(symbol->tokens, "primitives") == 0);

  next_symbol(file, symbol); /* : */
  next_symbol(file, symbol); /* [ */

  if (symbol->symbol != YF_SYMBOL_OP || symbol->tokens[0] != '[') {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

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
          }
          if (parse_primitives_i(file, symbol, primitives, i++) != 0)
            return -1;
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
        yf_seterr(YF_ERR_INVFILE, __func__);
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

  primitives->v[index].material = SIZE_MAX;
  primitives->v[index].indices = SIZE_MAX;
  for (size_t i = 0; i < YF_GLTF_ATTR_N; ++i)
    primitives->v[index].attributes[i] = SIZE_MAX;

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp("attributes", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* { */
          do {
            switch (next_symbol(file, symbol)) {
              case YF_SYMBOL_STR:
                if (strcmp("POSITION", symbol->tokens) == 0) {
                  next_symbol(file, symbol); /* : */
                  next_symbol(file, symbol);
                  errno = 0;
                  primitives->v[index].attributes[YF_GLTF_ATTR_POS] =
                    strtoll(symbol->tokens, NULL, 0);
                  if (errno != 0) {
                    yf_seterr(YF_ERR_OTHER, __func__);
                    return -1;
                  }
                } else if (strcmp("NORMAL", symbol->tokens) == 0) {
                  next_symbol(file, symbol); /* : */
                  next_symbol(file, symbol);
                  errno = 0;
                  primitives->v[index].attributes[YF_GLTF_ATTR_NORM] =
                    strtoll(symbol->tokens, NULL, 0);
                  if (errno != 0) {
                    yf_seterr(YF_ERR_OTHER, __func__);
                    return -1;
                  }
                } else if (strcmp("TEXCOORD_0", symbol->tokens) == 0) {
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
                yf_seterr(YF_ERR_INVFILE, __func__);
                return -1;
            }
          } while (symbol->symbol != YF_SYMBOL_OP || symbol->tokens[0] != '}');
        } else if (strcmp("indices", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          errno = 0;
          primitives->v[index].indices = strtoll(symbol->tokens, NULL, 0);
          if (errno != 0) {
            yf_seterr(YF_ERR_OTHER, __func__);
            return -1;
          }
        } else if (strcmp("material", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          errno = 0;
          primitives->v[index].material = strtoll(symbol->tokens, NULL, 0);
          if (errno != 0) {
            yf_seterr(YF_ERR_OTHER, __func__);
            return -1;
          }
        } else {
          if (consume_prop(file, symbol) != 0)
            return -1;
        }
        break;

      case YF_SYMBOL_OP:
        if (symbol->tokens[0] == '}')
          return 0;
        break;

      default:
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }
  } while (1);

  return 0;
}

static int parse_meshes(FILE *file, L_symbol *symbol, L_meshes *meshes) {
  assert(!feof(file));
  assert(symbol != NULL);
  assert(meshes != NULL);
  assert(symbol->symbol == YF_SYMBOL_STR);
  assert(strcmp(symbol->tokens, "meshes") == 0);

  next_symbol(file, symbol); /* : */
  next_symbol(file, symbol); /* [ */

  if (symbol->symbol != YF_SYMBOL_OP || symbol->tokens[0] != '[') {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

  size_t i = 0;

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_OP:
        if (symbol->tokens[0] == '{') {
          if (i == meshes->n) {
            const size_t n = i == 0 ? 1 : i<<1;
            void *tmp = realloc(meshes->v, n*sizeof *meshes->v);
            if (tmp == NULL) {
              yf_seterr(YF_ERR_NOMEM, __func__);
              return -1;
            }
            meshes->v = tmp;
            meshes->n = n;
            memset(meshes->v+i, 0, (n-i)*sizeof *meshes->v);
          }
          if (parse_meshes_i(file, symbol, meshes, i++) != 0)
            return -1;
        } else if (symbol->tokens[0] == ']') {
          if (i < meshes->n) {
            meshes->n = i;
            void *tmp = realloc(meshes->v, i*sizeof *meshes->v);
            if (tmp != NULL)
              meshes->v = tmp;
          }
          return 0;
        }
        break;

      default:
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }
  } while (1);

  return 0;
}

static int parse_meshes_i(FILE *file, L_symbol *symbol,
    L_meshes *meshes, size_t index)
{
  assert(!feof(file));
  assert(symbol != NULL);
  assert(meshes != NULL);
  assert(index < meshes->n);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '{');

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp("primitives", symbol->tokens) == 0) {
          if (parse_primitives(file, symbol, &meshes->v[index].primitives) != 0)
            return -1;
        } else if (strcmp("name", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          meshes->v[index].name = malloc(1+strlen(symbol->tokens));
          if (meshes->v[index].name == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
          }
          strcpy(meshes->v[index].name, symbol->tokens);
        } else {
          if (consume_prop(file, symbol) != 0)
            return -1;
        }
        break;

      case YF_SYMBOL_OP:
        if (symbol->tokens[0] == '}')
          return 0;
        break;

      default:
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }
  } while (1);

  return 0;
}

static int parse_materials(FILE *file, L_symbol *symbol,
    L_materials *materials)
{
  assert(!feof(file));
  assert(symbol != NULL);
  assert(materials != NULL);
  assert(symbol->symbol == YF_SYMBOL_STR);
  assert(strcmp(symbol->tokens, "materials") == 0);

  next_symbol(file, symbol); /* : */
  next_symbol(file, symbol); /* [ */

  if (symbol->symbol != YF_SYMBOL_OP || symbol->tokens[0] != '[') {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

  size_t i = 0;

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_OP:
        if (symbol->tokens[0] == '{') {
          if (i == materials->n) {
            const size_t n = i == 0 ? 1 : i<<1;
            void *tmp = realloc(materials->v, n*sizeof *materials->v);
            if (tmp == NULL) {
              yf_seterr(YF_ERR_NOMEM, __func__);
              return -1;
            }
            materials->v = tmp;
            materials->n = n;
            memset(materials->v+i, 0, (n-i)*sizeof *materials->v);
          }
          if (parse_materials_i(file, symbol, materials, i++) != 0)
            return -1;
        } else if (symbol->tokens[0] == ']') {
          if (i < materials->n) {
            materials->n = i;
            void *tmp = realloc(materials->v, i*sizeof *materials->v);
            if (tmp != NULL)
              materials->v = tmp;
          }
          return 0;
        }
        break;

      default:
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }
  } while (1);

  return 0;
}

static int parse_materials_i(FILE *file, L_symbol *symbol,
    L_materials *materials, size_t index)
{
  assert(!feof(file));
  assert(symbol != NULL);
  assert(materials != NULL);
  assert(index < materials->n);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '{');

  materials->v[index].pbrmr.base_clr_fac[0] = 1.0;
  materials->v[index].pbrmr.base_clr_fac[1] = 1.0;
  materials->v[index].pbrmr.base_clr_fac[2] = 1.0;
  materials->v[index].pbrmr.base_clr_fac[3] = 1.0;
  materials->v[index].pbrmr.metallic_fac = 1.0;
  materials->v[index].pbrmr.roughness_fac = 1.0;

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp("pbrMetallicRoughness", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* { */
          do {
            switch (next_symbol(file, symbol)) {
              case YF_SYMBOL_STR:
                if (strcmp("baseColorFactor", symbol->tokens) == 0) {
                  next_symbol(file, symbol); /* : */
                  next_symbol(file, symbol); /* [ */
                  errno = 0;

                  next_symbol(file, symbol);
                  materials->v[index].pbrmr.base_clr_fac[0] =
                    strtod(symbol->tokens, NULL);
                  next_symbol(file, symbol); /* , */

                  next_symbol(file, symbol);
                  materials->v[index].pbrmr.base_clr_fac[1] =
                    strtod(symbol->tokens, NULL);
                  next_symbol(file, symbol); /* , */

                  next_symbol(file, symbol);
                  materials->v[index].pbrmr.base_clr_fac[2] =
                    strtod(symbol->tokens, NULL);
                  next_symbol(file, symbol); /* , */

                  next_symbol(file, symbol);
                  materials->v[index].pbrmr.base_clr_fac[3] =
                    strtod(symbol->tokens, NULL);
                  next_symbol(file, symbol); /* ] */

                  if (errno != 0) {
                    yf_seterr(YF_ERR_OTHER, __func__);
                    return -1;
                  }
                } else if (strcmp("metallicFactor", symbol->tokens) == 0) {
                  next_symbol(file, symbol); /* : */
                  next_symbol(file, symbol);
                  errno = 0;

                  materials->v[index].pbrmr.metallic_fac =
                    strtod(symbol->tokens, NULL);

                  if (errno != 0) {
                    yf_seterr(YF_ERR_OTHER, __func__);
                    return -1;
                  }
                } else if (strcmp("roughnessFactor", symbol->tokens) == 0) {
                  next_symbol(file, symbol); /* : */
                  next_symbol(file, symbol);
                  errno = 0;

                  materials->v[index].pbrmr.roughness_fac =
                    strtod(symbol->tokens, NULL);

                  if (errno != 0) {
                    yf_seterr(YF_ERR_OTHER, __func__);
                    return -1;
                  }
                } else {
                  if (consume_prop(file, symbol) != 0)
                    return -1;
                }
                break;

              case YF_SYMBOL_OP:
                break;

              default:
                yf_seterr(YF_ERR_INVFILE, __func__);
                return -1;
            }
          } while (symbol->symbol != YF_SYMBOL_OP || symbol->tokens[0] != '}');
        } else if (strcmp("doubleSided", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          materials->v[index].double_sided =
            strcmp("true", symbol->tokens) == 0;
        } else if (strcmp("name", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          materials->v[index].name = malloc(1+strlen(symbol->tokens));
          if (materials->v[index].name == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
          }
          strcpy(materials->v[index].name, symbol->tokens);
        } else {
          if (consume_prop(file, symbol) != 0)
            return -1;
        }
        break;

      case YF_SYMBOL_OP:
        if (symbol->tokens[0] == '}')
          return 0;
        break;

      default:
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }
  } while (1);

  return 0;
}

static int parse_accessors(FILE *file, L_symbol *symbol,
    L_accessors *accessors)
{
  assert(!feof(file));
  assert(symbol != NULL);
  assert(accessors != NULL);
  assert(symbol->symbol == YF_SYMBOL_STR);
  assert(strcmp(symbol->tokens, "accessors") == 0);

  next_symbol(file, symbol); /* : */
  next_symbol(file, symbol); /* [ */

  if (symbol->symbol != YF_SYMBOL_OP || symbol->tokens[0] != '[') {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

  size_t i = 0;

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_OP:
        if (symbol->tokens[0] == '{') {
          if (i == accessors->n) {
            const size_t n = i == 0 ? 1 : i<<1;
            void *tmp = realloc(accessors->v, n*sizeof *accessors->v);
            if (tmp == NULL) {
              yf_seterr(YF_ERR_NOMEM, __func__);
              return -1;
            }
            accessors->v = tmp;
            accessors->n = n;
            memset(accessors->v+i, 0, (n-i)*sizeof *accessors->v);
          }
          if (parse_accessors_i(file, symbol, accessors, i++) != 0)
            return -1;
        } else if (symbol->tokens[0] == ']') {
          if (i < accessors->n) {
            accessors->n = i;
            void *tmp = realloc(accessors->v, i*sizeof *accessors->v);
            if (tmp != NULL)
              accessors->v = tmp;
          }
          return 0;
        }
        break;

      default:
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }
  } while (1);

  return 0;
}

static int parse_accessors_i(FILE *file, L_symbol *symbol,
    L_accessors *accessors, size_t index)
{
  assert(!feof(file));
  assert(symbol != NULL);
  assert(accessors != NULL);
  assert(index < accessors->n);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '{');

  accessors->v[index].buffer_view = SIZE_MAX;

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp("bufferView", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          errno = 0;
          accessors->v[index].buffer_view = strtoll(symbol->tokens, NULL, 0);
          if (errno != 0) {
            yf_seterr(YF_ERR_OTHER, __func__);
            return -1;
          }
        } else if (strcmp("byteOffset", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          errno = 0;
          accessors->v[index].byte_off = strtoll(symbol->tokens, NULL, 0);
          if (errno != 0) {
            yf_seterr(YF_ERR_OTHER, __func__);
            return -1;
          }
        } else if (strcmp("count", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          errno = 0;
          accessors->v[index].count = strtoll(symbol->tokens, NULL, 0);
          if (errno != 0) {
            yf_seterr(YF_ERR_OTHER, __func__);
            return -1;
          }
        } else if (strcmp("componentType", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          errno = 0;
          accessors->v[index].comp_type = strtol(symbol->tokens, NULL, 0);
          if (errno != 0) {
            yf_seterr(YF_ERR_OTHER, __func__);
            return -1;
          }
        } else if (strcmp("type", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          if (strcmp("SCALAR", symbol->tokens) == 0)
            accessors->v[index].type = YF_GLTF_TYPE_SCALAR;
          else if (strcmp("VEC2", symbol->tokens) == 0)
            accessors->v[index].type = YF_GLTF_TYPE_VEC2;
          else if (strcmp("VEC3", symbol->tokens) == 0)
            accessors->v[index].type = YF_GLTF_TYPE_VEC3;
          else if (strcmp("VEC4", symbol->tokens) == 0)
            accessors->v[index].type = YF_GLTF_TYPE_VEC4;
          else if (strcmp("MAT2", symbol->tokens) == 0)
            accessors->v[index].type = YF_GLTF_TYPE_MAT2;
          else if (strcmp("MAT3", symbol->tokens) == 0)
            accessors->v[index].type = YF_GLTF_TYPE_MAT3;
          else if (strcmp("MAT4", symbol->tokens) == 0)
            accessors->v[index].type = YF_GLTF_TYPE_MAT4;
        } else if (strcmp("min", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* [ */
          for (size_t i = 0; i < 16; ++i) {
            next_symbol(file, symbol);
            if (symbol->symbol != YF_SYMBOL_NUM)
              break;
            errno = 0;
            accessors->v[index].min.m4[i] = strtof(symbol->tokens, NULL);
            if (errno != 0) {
              errno = 0;
              accessors->v[i].min.m4[index] = strtol(symbol->tokens, NULL, 0);
              if (errno != 0) {
                yf_seterr(YF_ERR_OTHER, __func__);
                return -1;
              }
            }
            next_symbol(file, symbol);
            if (symbol->symbol == YF_SYMBOL_OP && symbol->tokens[0] == ']')
              break;
          }
        } else if (strcmp("max", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* [ */
          for (size_t i = 0; i < 16; ++i) {
            next_symbol(file, symbol);
            if (symbol->symbol != YF_SYMBOL_NUM)
              break;
            errno = 0;
            accessors->v[index].max.m4[i] = strtof(symbol->tokens, NULL);
            if (errno != 0) {
              accessors->v[index].max.m4[i] = strtol(symbol->tokens, NULL, 0);
              if (errno != 0) {
                yf_seterr(YF_ERR_OTHER, __func__);
                return -1;
              }
            }
            next_symbol(file, symbol);
            if (symbol->symbol == YF_SYMBOL_OP && symbol->tokens[0] == ']')
              break;
          }
        } else if (strcmp("name", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          accessors->v[index].name = malloc(1+strlen(symbol->tokens));
          if (accessors->v[index].name == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
          }
          strcpy(accessors->v[index].name, symbol->tokens);
        } else {
          if (consume_prop(file, symbol) != 0)
            return -1;
        }
        break;

      case YF_SYMBOL_OP:
        if (symbol->tokens[0] == '}')
          return 0;
        break;

      default:
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }
  } while (1);

  return 0;
}

static int parse_bufferviews(FILE *file, L_symbol *symbol,
    L_bufferviews *bufferviews)
{
  assert(!feof(file));
  assert(symbol != NULL);
  assert(bufferviews != NULL);
  assert(symbol->symbol == YF_SYMBOL_STR);
  assert(strcmp(symbol->tokens, "bufferViews") == 0);

  next_symbol(file, symbol); /* : */
  next_symbol(file, symbol); /* [ */

  if (symbol->symbol != YF_SYMBOL_OP || symbol->tokens[0] != '[') {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

  size_t i = 0;

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_OP:
        if (symbol->tokens[0] == '{') {
          if (i == bufferviews->n) {
            const size_t n = i == 0 ? 1 : i<<1;
            void *tmp = realloc(bufferviews->v, n*sizeof *bufferviews->v);
            if (tmp == NULL) {
              yf_seterr(YF_ERR_NOMEM, __func__);
              return -1;
            }
            bufferviews->v = tmp;
            bufferviews->n = n;
            memset(bufferviews->v+i, 0, (n-i)*sizeof *bufferviews->v);
          }
          if (parse_bufferviews_i(file, symbol, bufferviews, i++) != 0)
            return -1;
        } else if (symbol->tokens[0] == ']') {
          if (i < bufferviews->n) {
            bufferviews->n = i;
            void *tmp = realloc(bufferviews->v, i*sizeof *bufferviews->v);
            if (tmp != NULL)
              bufferviews->v = tmp;
          }
          return 0;
        }
        break;

      default:
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }
  } while (1);

  return 0;
}

static int parse_bufferviews_i(FILE *file, L_symbol *symbol,
    L_bufferviews *bufferviews, size_t index)
{
  assert(!feof(file));
  assert(symbol != NULL);
  assert(bufferviews != NULL);
  assert(index < bufferviews->n);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '{');

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp("buffer", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          errno = 0;
          bufferviews->v[index].buffer = strtoll(symbol->tokens, NULL, 0);
          if (errno != 0) {
            yf_seterr(YF_ERR_OTHER, __func__);
            return -1;
          }
        } else if (strcmp("byteOffset", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          errno = 0;
          bufferviews->v[index].byte_off = strtoll(symbol->tokens, NULL, 0);
          if (errno != 0) {
            yf_seterr(YF_ERR_OTHER, __func__);
            return -1;
          }
        } else if (strcmp("byteLength", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          errno = 0;
          bufferviews->v[index].byte_len = strtoll(symbol->tokens, NULL, 0);
          if (errno != 0) {
            yf_seterr(YF_ERR_OTHER, __func__);
            return -1;
          }
        } else if (strcmp("byteStride", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          errno = 0;
          bufferviews->v[index].byte_strd = strtoll(symbol->tokens, NULL, 0);
          if (errno != 0) {
            yf_seterr(YF_ERR_OTHER, __func__);
            return -1;
          }
        } else if (strcmp("name", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          bufferviews->v[index].name = malloc(1+strlen(symbol->tokens));
          if (bufferviews->v[index].name == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
          }
          strcpy(bufferviews->v[index].name, symbol->tokens);
        } else {
          if (consume_prop(file, symbol) != 0)
            return -1;
        }
        break;

      case YF_SYMBOL_OP:
        if (symbol->tokens[0] == '}')
          return 0;
        break;

      default:
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }
  } while (1);

  return 0;
}

static int parse_buffers(FILE *file, L_symbol *symbol, L_buffers *buffers) {
  assert(!feof(file));
  assert(symbol != NULL);
  assert(buffers != NULL);
  assert(symbol->symbol == YF_SYMBOL_STR);
  assert(strcmp(symbol->tokens, "buffers") == 0);

  next_symbol(file, symbol); /* : */
  next_symbol(file, symbol); /* [ */

  if (symbol->symbol != YF_SYMBOL_OP || symbol->tokens[0] != '[') {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

  size_t i = 0;

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_OP:
        if (symbol->tokens[0] == '{') {
          if (i == buffers->n) {
            const size_t n = i == 0 ? 1 : i<<1;
            void *tmp = realloc(buffers->v, n*sizeof *buffers->v);
            if (tmp == NULL) {
              yf_seterr(YF_ERR_NOMEM, __func__);
              return -1;
            }
            buffers->v = tmp;
            buffers->n = n;
            memset(buffers->v+i, 0, (n-i)*sizeof *buffers->v);
          }
          if (parse_buffers_i(file, symbol, buffers, i++) != 0)
            return -1;
        } else if (symbol->tokens[0] == ']') {
          if (i < buffers->n) {
            buffers->n = i;
            void *tmp = realloc(buffers->v, i*sizeof *buffers->v);
            if (tmp != NULL)
              buffers->v = tmp;
          }
          return 0;
        }
        break;

      default:
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }
  } while (1);

  return 0;
}

static int parse_buffers_i(FILE *file, L_symbol *symbol,
    L_buffers *buffers, size_t index)
{
  assert(!feof(file));
  assert(symbol != NULL);
  assert(buffers != NULL);
  assert(index < buffers->n);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '{');

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp("byteLength", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          errno = 0;
          buffers->v[index].byte_len = strtoll(symbol->tokens, NULL, 0);
          if (errno != 0) {
            yf_seterr(YF_ERR_OTHER, __func__);
            return -1;
          }
        } else if (strcmp("uri", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          buffers->v[index].uri = malloc(1+strlen(symbol->tokens));
          if (buffers->v[index].uri == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
          }
          strcpy(buffers->v[index].uri, symbol->tokens);
        } else if (strcmp("name", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          buffers->v[index].name = malloc(1+strlen(symbol->tokens));
          if (buffers->v[index].name == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
          }
          strcpy(buffers->v[index].name, symbol->tokens);
        } else {
          if (consume_prop(file, symbol) != 0)
            return -1;
        }
        break;

      case YF_SYMBOL_OP:
        if (symbol->tokens[0] == '}')
          return 0;
        break;

      default:
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }
  } while (1);

  return 0;
}

static int load_meshdt(const L_gltf *gltf, YF_meshdt *data) {
  assert(gltf != NULL);
  assert(data != NULL);

  if (gltf->accessors.n == 0 || gltf->bufferviews.n == 0 ||
      gltf->buffers.n == 0 || gltf->meshes.n == 0 ||
      gltf->meshes.v[0].primitives.n == 0)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

  struct { size_t accessor, view, buffer; } idx, attrs[YF_GLTF_ATTR_N];
  const L_primitives *prim = &gltf->meshes.v[0].primitives;

  idx.accessor = prim->v[0].indices;
  if (idx.accessor != SIZE_MAX) {
    idx.view = gltf->accessors.v[idx.accessor].buffer_view;
    idx.buffer = gltf->bufferviews.v[idx.view].buffer;
  }
  for (size_t i = 0; i < YF_GLTF_ATTR_N; ++i) {
    attrs[i].accessor = prim->v[0].attributes[i];
    if (attrs[i].accessor != SIZE_MAX) {
      attrs[i].view = gltf->accessors.v[attrs[i].accessor].buffer_view;
      attrs[i].buffer = gltf->bufferviews.v[attrs[i].view].buffer;
    }
  }
  if (attrs[YF_GLTF_ATTR_POS].accessor == SIZE_MAX) {
    yf_seterr(YF_ERR_UNSUP, __func__);
    return -1;
  }

  const size_t v_n = gltf->accessors.v[attrs[YF_GLTF_ATTR_POS].accessor].count;
  YF_vmdl *verts = malloc(v_n*sizeof *verts);
  if (verts == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  const size_t i_strd = v_n < UINT16_MAX ? 2 : 4;
  size_t i_n = 0;
  void *inds = NULL;
  if (idx.accessor != SIZE_MAX) {
    i_n = gltf->accessors.v[idx.accessor].count;
    inds = malloc(i_n*i_strd);
    if (inds == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      free(verts);
      return -1;
    }
  }

  size_t buf_i = SIZE_MAX;
  FILE *file = NULL;
  size_t comp_sz = 0;
  size_t comp_n = 0;

  for (size_t i = 0; i < YF_GLTF_ATTR_N; ++i) {
    switch (i) {
      case YF_GLTF_ATTR_POS:
        assert(file == NULL);
        file = fopen(gltf->buffers.v[attrs[i].buffer].uri, "r");
        if (file == NULL) {
          yf_seterr(YF_ERR_NOFILE, __func__);
          free(verts);
          free(inds);
          return -1;
        }
        if (fseek(file, gltf->bufferviews.v[attrs[i].view].byte_off,
              SEEK_SET))
        {
          yf_seterr(YF_ERR_INVFILE, __func__);
          free(verts);
          free(inds);
          fclose(file);
          return -1;
        }
        /* TODO: Other comp/type combinations. */
        assert(gltf->accessors.v[attrs[i].accessor].comp_type ==
            YF_GLTF_COMP_FLOAT);
        assert(gltf->accessors.v[attrs[i].accessor].type == YF_GLTF_TYPE_VEC3);
        comp_sz = 4;
        comp_n = 3;
        for (size_t j = 0; j < v_n; ++j) {
          if (fread(verts[j].pos, comp_sz, comp_n, file) < comp_n) {
            yf_seterr(YF_ERR_INVFILE, __func__);
            free(verts);
            free(inds);
            fclose(file);
            return -1;
          }
        }
        buf_i = attrs[i].buffer;
        break;

      case YF_GLTF_ATTR_NORM:
        assert(file != NULL);
        if (buf_i != attrs[i].buffer) {
          fclose(file);
          file = fopen(gltf->buffers.v[attrs[i].buffer].uri, "r");
          if (file == NULL) {
            yf_seterr(YF_ERR_NOFILE, __func__);
            free(verts);
            free(inds);
            return -1;
          }
          buf_i = attrs[i].buffer;
        }
        if (fseek(file, gltf->bufferviews.v[attrs[i].view].byte_off,
              SEEK_SET))
        {
          yf_seterr(YF_ERR_INVFILE, __func__);
          free(verts);
          free(inds);
          fclose(file);
          return -1;
        }
        /* TODO: Other comp/type combinations. */
        assert(gltf->accessors.v[attrs[i].accessor].comp_type ==
            YF_GLTF_COMP_FLOAT);
        assert(gltf->accessors.v[attrs[i].accessor].type == YF_GLTF_TYPE_VEC3);
        comp_sz = 4;
        comp_n = 3;
        for (size_t j = 0; j < v_n; ++j) {
          if (fread(verts[j].norm, comp_sz, comp_n, file) < comp_n) {
            yf_seterr(YF_ERR_INVFILE, __func__);
            free(verts);
            free(inds);
            fclose(file);
            return -1;
          }
        }
        break;

      case YF_GLTF_ATTR_TEX0:
        assert(file != NULL);
        if (buf_i != attrs[i].buffer) {
          fclose(file);
          file = fopen(gltf->buffers.v[attrs[i].buffer].uri, "r");
          if (file == NULL) {
            yf_seterr(YF_ERR_NOFILE, __func__);
            free(verts);
            free(inds);
            return -1;
          }
          buf_i = attrs[i].buffer;
        }
        if (fseek(file, gltf->bufferviews.v[attrs[i].view].byte_off,
              SEEK_SET))
        {
          yf_seterr(YF_ERR_INVFILE, __func__);
          free(verts);
          free(inds);
          fclose(file);
          return -1;
        }
        /* TODO: Other comp/type combinations. */
        assert(gltf->accessors.v[attrs[i].accessor].comp_type ==
            YF_GLTF_COMP_FLOAT);
        assert(gltf->accessors.v[attrs[i].accessor].type == YF_GLTF_TYPE_VEC2);
        comp_sz = 4;
        comp_n = 2;
        for (size_t j = 0; j < v_n; ++j) {
          if (fread(verts[j].tc, comp_sz, comp_n, file) < comp_n) {
            yf_seterr(YF_ERR_INVFILE, __func__);
            free(verts);
            free(inds);
            fclose(file);
            return -1;
          }
        }
        break;
    }
  }

  if (inds != NULL) {
    assert(file != NULL);
    if (buf_i != idx.buffer) {
      fclose(file);
      file = fopen(gltf->buffers.v[idx.buffer].uri, "r");
      if (file == NULL) {
        yf_seterr(YF_ERR_NOFILE, __func__);
        free(verts);
        free(inds);
        return -1;
      }
      buf_i = idx.buffer;
    }
    if (fseek(file, gltf->bufferviews.v[idx.view].byte_off, SEEK_SET)) {
      yf_seterr(YF_ERR_INVFILE, __func__);
      free(verts);
      free(inds);
      fclose(file);
      return -1;
    }
    /* TODO: Other comps. */
    switch (gltf->accessors.v[idx.accessor].comp_type) {
      case YF_GLTF_COMP_USHORT:
        comp_sz = 2;
        break;
      case YF_GLTF_COMP_UINT:
        comp_sz = 4;
        break;
      default:
        assert(0);
        return -1;
    }
    assert(gltf->accessors.v[idx.accessor].type == YF_GLTF_TYPE_SCALAR);
    /* TODO */
    assert(comp_sz == i_strd);
    comp_n = 1;
    for (size_t j = 0; j < i_n; ++j) {
      if (fread((char *)inds+j*comp_sz, comp_sz, comp_n, file) < comp_n) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        free(verts);
        free(inds);
        fclose(file);
        return -1;
      }
    }
  }

  data->v.vtype = YF_VTYPE_MDL;
  data->v.data = verts;
  data->v.n = v_n;
  data->i.data = inds;
  data->i.stride = i_strd;
  data->i.n = i_n;

  return 0;
}

static void deinit_gltf(L_gltf *gltf) {
  if (gltf == NULL)
    return;

  free(gltf->asset.copyright);
  free(gltf->asset.generator);
  free(gltf->asset.version);
  free(gltf->asset.min_version);

  for (size_t i = 0; i < gltf->scenes.n; ++i) {
    free(gltf->scenes.v[i].nodes);
    free(gltf->scenes.v[i].name);
  }
  free(gltf->scenes.v);

  for (size_t i = 0; i < gltf->nodes.n; ++i)
    free(gltf->nodes.v[i].name);
  free(gltf->nodes.v);

  for (size_t i = 0; i < gltf->meshes.n; ++i) {
    free(gltf->meshes.v[i].primitives.v);
    free(gltf->meshes.v[i].name);
  }
  free(gltf->meshes.v);

  for (size_t i = 0; i < gltf->materials.n; ++i)
    free(gltf->materials.v[i].name);
  free(gltf->materials.v);

  for (size_t i = 0; i < gltf->accessors.n; ++i)
    free(gltf->accessors.v[i].name);
  free(gltf->accessors.v);

  for (size_t i = 0; i < gltf->bufferviews.n; ++i)
    free(gltf->bufferviews.v[i].name);
  free(gltf->bufferviews.v);

  for (size_t i = 0; i < gltf->buffers.n; ++i) {
    free(gltf->buffers.v[i].uri);
    free(gltf->buffers.v[i].name);
  }
  free(gltf->buffers.v);
}


/*
 * DEVEL
 */

#ifdef YF_DEVEL
static void print_gltf(const L_gltf *gltf) {
  printf("\n[YF] OUTPUT (%s):\n", __func__);

  puts("glTF.asset:");
  printf(" copyright: %s\n", gltf->asset.copyright);
  printf(" generator: %s\n", gltf->asset.generator);
  printf(" version: %s\n", gltf->asset.version);
  printf(" minVersion: %s\n", gltf->asset.min_version);

  puts("glTF.scene:");
  printf(" #: %ld\n", gltf->scene);

  puts("glTF.scenes:");
  printf(" n: %lu\n", gltf->scenes.n);
  for (size_t i = 0; i < gltf->scenes.n; ++i) {
    printf(" scene '%s':\n", gltf->scenes.v[i].name);
    printf("  nodes: [ ");
    for (size_t j = 0; j < gltf->scenes.v[i].node_n; ++j)
      printf("%lu ", gltf->scenes.v[i].nodes[j]);
    puts("]");
  }

  puts("glTF.nodes:");
  printf(" n: %lu\n", gltf->nodes.n);
  for (size_t i = 0; i < gltf->nodes.n; ++i) {
    printf(" node '%s':\n", gltf->nodes.v[i].name);
    printf("  children: [ ");
    for (size_t j = 0; j < gltf->nodes.v[i].child_n; ++j)
      printf("%lu ", gltf->nodes.v[i].children[j]);
    puts("]");
    printf("  camera: %lu\n", gltf->nodes.v[i].camera);
    printf("  mesh: %lu\n", gltf->nodes.v[i].mesh);
    if (gltf->nodes.v[i].xform_mask == YF_GLTF_XFORM_NONE) {
      puts("  (no transform)");
    } else if (gltf->nodes.v[i].xform_mask == YF_GLTF_XFORM_NONE) {
      printf("  matrix: [ ");
      for (size_t j = 0; j < 16; ++j)
        printf("%.4f ", gltf->nodes.v[i].matrix[j]);
      puts("]");
    } else {
      printf("  translation: [ ");
      for (size_t j = 0; j < 3; ++j)
        printf("%.4f ", gltf->nodes.v[i].trs.t[j]);
      puts("]");
      printf("  rotation: [ ");
      for (size_t j = 0; j < 4; ++j)
        printf("%.4f ", gltf->nodes.v[i].trs.r[j]);
      puts("]");
      printf("  scale: [ ");
      for (size_t j = 0; j < 3; ++j)
        printf("%.4f ", gltf->nodes.v[i].trs.s[j]);
      puts("]");
    }
  }

  puts("glTF.meshes:");
  printf(" n: %lu\n", gltf->meshes.n);
  for (size_t i = 0; i < gltf->meshes.n; ++i) {
    printf(" mesh '%s':\n", gltf->meshes.v[i].name);
    printf("  n: %lu\n", gltf->meshes.v[i].primitives.n);
    for (size_t j = 0; j < gltf->meshes.v[i].primitives.n; ++j) {
      printf("  primitives #%lu:\n", j);
      printf("   POSITION: %lu\n",
          gltf->meshes.v[i].primitives.v[j].attributes[YF_GLTF_ATTR_POS]);
      printf("   NORMAL: %lu\n",
          gltf->meshes.v[i].primitives.v[j].attributes[YF_GLTF_ATTR_NORM]);
      printf("   TEXTURE_0: %lu\n",
          gltf->meshes.v[i].primitives.v[j].attributes[YF_GLTF_ATTR_TEX0]);
      printf("  indices: %lu\n", gltf->meshes.v[i].primitives.v[j].indices);
      printf("  material: %lu\n", gltf->meshes.v[i].primitives.v[j].material);
    }
  }

  puts("glTF.materials:");
  printf(" n: %lu\n", gltf->materials.n);
  for (size_t i = 0; i < gltf->materials.n; ++i) {
    printf(" material '%s':\n", gltf->materials.v[i].name);
    puts("  pbrMetallicRoughness:");
    printf("   baseColorFactor: [%.9f, %.9f, %.9f, %.9f]\n",
        gltf->materials.v[i].pbrmr.base_clr_fac[0],
        gltf->materials.v[i].pbrmr.base_clr_fac[1],
        gltf->materials.v[i].pbrmr.base_clr_fac[2],
        gltf->materials.v[i].pbrmr.base_clr_fac[3]);
    printf("   metallicFactor: %.9f\n",
        gltf->materials.v[i].pbrmr.metallic_fac);
    printf("   roughnessFactor: %.9f\n",
        gltf->materials.v[i].pbrmr.roughness_fac);
    printf("  doubleSided: %d\n", gltf->materials.v[i].double_sided);
  }

  puts("glTF.accessors:");
  printf(" n: %lu\n", gltf->accessors.n);
  for (size_t i = 0; i < gltf->accessors.n; ++i) {
    printf(" accessor '%s':\n", gltf->accessors.v[i].name);
    printf("  bufferView: %lu\n", gltf->accessors.v[i].buffer_view);
    printf("  byteOffset: %lu\n", gltf->accessors.v[i].byte_off);
    printf("  count: %lu\n", gltf->accessors.v[i].count);
    printf("  componenType: %d\n", gltf->accessors.v[i].comp_type);
    printf("  type: %d\n", gltf->accessors.v[i].type);
    switch (gltf->accessors.v[i].type) {
      case YF_GLTF_TYPE_SCALAR:
        printf("  min: %.9f\n", gltf->accessors.v[i].min.s);
        printf("  max: %.9f\n", gltf->accessors.v[i].max.s);
        break;
      case YF_GLTF_TYPE_VEC2:
        printf("  min: [%.9f, %.9f]\n",
            gltf->accessors.v[i].min.v2[0], gltf->accessors.v[i].min.v2[1]);
        printf("  max: [%.9f, %.9f]\n",
            gltf->accessors.v[i].max.v2[0], gltf->accessors.v[i].max.v2[1]);
        break;
      case YF_GLTF_TYPE_VEC3:
        printf("  min: [%.9f, %.9f, %.9f]\n",
            gltf->accessors.v[i].min.v3[0], gltf->accessors.v[i].min.v3[1],
            gltf->accessors.v[i].min.v3[2]);
        printf("  max: [%.9f, %.9f, %.9f]\n",
            gltf->accessors.v[i].max.v3[0], gltf->accessors.v[i].max.v3[1],
            gltf->accessors.v[i].max.v3[2]);
        break;
      case YF_GLTF_TYPE_VEC4:
        printf("  min: [%.9f, %.9f, %.9f, %.9f]\n",
            gltf->accessors.v[i].min.v4[0], gltf->accessors.v[i].min.v4[1],
            gltf->accessors.v[i].min.v4[2], gltf->accessors.v[i].min.v4[3]);
        printf("  max: [%.9f, %.9f, %.9f, %.9f]\n",
            gltf->accessors.v[i].max.v4[0], gltf->accessors.v[i].max.v4[1],
            gltf->accessors.v[i].max.v4[2], gltf->accessors.v[i].max.v4[3]);
        break;
      default:
        puts("  missing min/max output...");
    }
  }

  puts("glTF.bufferViews:");
  printf(" n: %lu\n", gltf->bufferviews.n);
  for (size_t i = 0; i < gltf->bufferviews.n; ++i) {
    printf(" buffer view '%s':\n", gltf->bufferviews.v[i].name);
    printf("  buffer: %lu\n", gltf->bufferviews.v[i].buffer);
    printf("  byteOffset: %lu\n", gltf->bufferviews.v[i].byte_off);
    printf("  byteLength: %lu\n", gltf->bufferviews.v[i].byte_len);
    printf("  byteStride: %lu\n", gltf->bufferviews.v[i].byte_strd);
  }

  puts("glTF.buffers:");
  printf(" n: %lu\n", gltf->buffers.n);
  for (size_t i = 0; i < gltf->buffers.n; ++i) {
    printf(" buffer '%s':\n", gltf->buffers.v[i].name);
    printf("  byteLength: %lu\n", gltf->buffers.v[i].byte_len);
    printf("  uri: %s\n", gltf->buffers.v[i].uri);
  }
}
#endif
