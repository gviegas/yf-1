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
    size_t material;
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

/* Type defining the 'materials' property. */
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

/* Type defining the 'accessors' property. */
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

/* Type defining the root glTF object. */
typedef struct {
  L_asset asset;
  size_t scene;
  L_scenes scenes;
  L_nodes nodes;
  L_meshes meshes;
  L_materials materials;
  L_accessors accessors;
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
static int parse_meshes(FILE *file, L_symbol *symbol, L_meshes *meshes);
static int parse_meshes_i(FILE *file, L_symbol *symbol,
    L_meshes *meshes, size_t index);
static int parse_materials(FILE *file, L_symbol *symbol,
    L_materials *materials);
static int parse_materials_i(FILE *file, L_symbol *symbol,
    L_materials *materials, size_t index);
static int parse_accessors(FILE *file, L_symbol *symbol,
    L_accessors *accessors);
static int parse_accessors_i(FILE *file, L_symbol *symbol,
    L_accessors *accessors, size_t index);
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
  printf(" copyright: %s\n", gltf.asset.copyright);
  printf(" generator: %s\n", gltf.asset.generator);
  printf(" version: %s\n", gltf.asset.version);
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

  puts("glTF.meshes:");
  printf(" n: %lu\n", gltf.meshes.n);
  for (size_t i = 0; i < gltf.meshes.n; ++i) {
    printf(" mesh '%s':\n", gltf.meshes.v[i].name);
    printf("  n: %lu\n", gltf.meshes.v[i].primitives.n);
    for (size_t j = 0; j < gltf.meshes.v[i].primitives.n; ++j) {
      printf("  primitives #%lu:\n", j);
      printf("   POSITION: %lu\n",
          gltf.meshes.v[i].primitives.v[j].attributes[YF_GLTF_ATTR_POS]);
      printf("   NORMAL: %lu\n",
          gltf.meshes.v[i].primitives.v[j].attributes[YF_GLTF_ATTR_NORM]);
      printf("   TEXTURE_0: %lu\n",
          gltf.meshes.v[i].primitives.v[j].attributes[YF_GLTF_ATTR_TEX0]);
      printf("  indices: %lu\n", gltf.meshes.v[i].primitives.v[j].indices);
      printf("  material: %lu\n", gltf.meshes.v[i].primitives.v[j].material);
    }
  }

  puts("glTF.materials:");
  printf(" n: %lu\n", gltf.materials.n);
  for (size_t i = 0; i < gltf.materials.n; ++i) {
    printf(" material '%s':\n", gltf.materials.v[i].name);
    puts("  pbrMetallicRoughness:");
    printf("   baseColorFactor: [%.9f, %.9f, %.9f, %.9f]\n",
        gltf.materials.v[i].pbrmr.base_clr_fac[0],
        gltf.materials.v[i].pbrmr.base_clr_fac[1],
        gltf.materials.v[i].pbrmr.base_clr_fac[2],
        gltf.materials.v[i].pbrmr.base_clr_fac[3]);
    printf("   metallicFactor: %.9f\n", gltf.materials.v[i].pbrmr.metallic_fac);
    printf("   roughnessFactor: %.9f\n",
        gltf.materials.v[i].pbrmr.roughness_fac);
    printf("  doubleSided: %d\n", gltf.materials.v[i].double_sided);
  }

  puts("glTF.accessors:");
  printf(" n: %lu\n", gltf.accessors.n);
  for (size_t i = 0; i < gltf.accessors.n; ++i) {
    printf(" accessor '%s':\n", gltf.accessors.v[i].name);
    printf("  bufferView: %lu\n", gltf.accessors.v[i].buffer_view);
    printf("  byteOffset: %lu\n", gltf.accessors.v[i].byte_off);
    printf("  count: %lu\n", gltf.accessors.v[i].count);
    printf("  componenType: %d\n", gltf.accessors.v[i].comp_type);
    printf("  type: %d\n", gltf.accessors.v[i].type);
    switch (gltf.accessors.v[i].type) {
      case YF_GLTF_TYPE_SCALAR:
        printf("  min: %.9f\n", gltf.accessors.v[i].min.s);
        printf("  max: %.9f\n", gltf.accessors.v[i].max.s);
        break;
      case YF_GLTF_TYPE_VEC2:
        printf("  min: [%.9f, %.9f]\n",
            gltf.accessors.v[i].min.v2[0], gltf.accessors.v[i].min.v2[1]);
        printf("  max: [%.9f, %.9f]\n",
            gltf.accessors.v[i].max.v2[0], gltf.accessors.v[i].max.v2[1]);
        break;
      case YF_GLTF_TYPE_VEC3:
        printf("  min: [%.9f, %.9f, %.9f]\n",
            gltf.accessors.v[i].min.v3[0], gltf.accessors.v[i].min.v3[1],
            gltf.accessors.v[i].min.v3[2]);
        printf("  max: [%.9f, %.9f, %.9f]\n",
            gltf.accessors.v[i].max.v3[0], gltf.accessors.v[i].max.v3[1],
            gltf.accessors.v[i].max.v3[2]);
        break;
      case YF_GLTF_TYPE_VEC4:
        printf("  min: [%.9f, %.9f, %.9f, %.9f]\n",
            gltf.accessors.v[i].min.v4[0], gltf.accessors.v[i].min.v4[1],
            gltf.accessors.v[i].min.v4[2], gltf.accessors.v[i].min.v4[3]);
        printf("  max: [%.9f, %.9f, %.9f, %.9f]\n",
            gltf.accessors.v[i].max.v4[0], gltf.accessors.v[i].max.v4[1],
            gltf.accessors.v[i].max.v4[2], gltf.accessors.v[i].max.v4[3]);
        break;
      default:
        puts("  missing min/max output...");
    }
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
        } else if (strcmp(YF_GLTF_PROP("meshes"), symbol->tokens) == 0) {
          if (parse_meshes(file, symbol, &gltf->meshes) != 0)
            return -1;
        } else if (strcmp(YF_GLTF_PROP("materials"), symbol->tokens) == 0) {
          if (parse_materials(file, symbol, &gltf->materials) != 0)
            return -1;
        } else if (strcmp(YF_GLTF_PROP("accessors"), symbol->tokens) == 0) {
          if (parse_accessors(file, symbol, &gltf->accessors) != 0)
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

  for (size_t i = 0; i < YF_GLTF_ATTR_N; ++i)
    primitives->v[index].attributes[i] = SIZE_MAX;

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
        } else if (strcmp(YF_GLTF_PROP("material"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          errno = 0;
          primitives->v[index].material = strtoll(symbol->tokens, NULL, 0);
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

static int parse_meshes(FILE *file, L_symbol *symbol, L_meshes *meshes) {
  assert(!feof(file));
  assert(symbol != NULL);
  assert(meshes != NULL);
  assert(symbol->symbol == YF_SYMBOL_STR);
  assert(strcmp(symbol->tokens, YF_GLTF_PROP("meshes")) == 0);

  next_symbol(file, symbol); /* : */
  next_symbol(file, symbol); /* [ */

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
        if (strcmp(YF_GLTF_PROP("primitives"), symbol->tokens) == 0) {
          if (parse_primitives(file, symbol, &meshes->v[index].primitives) != 0)
            return -1;
        } else if (strcmp(YF_GLTF_PROP("name"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          meshes->v[index].name = malloc(1+strlen(symbol->tokens));
          if (meshes->v[index].name == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
          }
          strcpy(meshes->v[index].name, symbol->tokens);
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

static int parse_materials(FILE *file, L_symbol *symbol,
    L_materials *materials)
{
  assert(!feof(file));
  assert(symbol != NULL);
  assert(materials != NULL);
  assert(symbol->symbol == YF_SYMBOL_STR);
  assert(strcmp(symbol->tokens, YF_GLTF_PROP("materials")) == 0);

  next_symbol(file, symbol); /* : */
  next_symbol(file, symbol); /* [ */

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

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp(YF_GLTF_PROP("pbrMetallicRoughness"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* { */
          do {
            switch (next_symbol(file, symbol)) {
              case YF_SYMBOL_STR:
                if (strcmp(YF_GLTF_PROP("baseColorFactor"),
                    symbol->tokens) == 0)
                {
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
                } else if (strcmp(YF_GLTF_PROP("metallicFactor"),
                    symbol->tokens) == 0)
                {
                  next_symbol(file, symbol); /* : */
                  next_symbol(file, symbol);
                  errno = 0;

                  materials->v[index].pbrmr.metallic_fac =
                    strtod(symbol->tokens, NULL);

                  if (errno != 0) {
                    yf_seterr(YF_ERR_OTHER, __func__);
                    return -1;
                  }
                } else if (strcmp(YF_GLTF_PROP("roughnessFactor"),
                    symbol->tokens) == 0)
                {
                  next_symbol(file, symbol); /* : */
                  next_symbol(file, symbol);
                  errno = 0;

                  materials->v[index].pbrmr.roughness_fac =
                    strtod(symbol->tokens, NULL);

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
        } else if (strcmp(YF_GLTF_PROP("doubleSided"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          materials->v[index].double_sided =
            strcmp("true", symbol->tokens) == 0;
        } else if (strcmp(YF_GLTF_PROP("name"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          materials->v[index].name = malloc(1+strlen(symbol->tokens));
          if (materials->v[index].name == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
          }
          strcpy(materials->v[index].name, symbol->tokens);
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

static int parse_accessors(FILE *file, L_symbol *symbol,
    L_accessors *accessors)
{
  assert(!feof(file));
  assert(symbol != NULL);
  assert(accessors != NULL);
  assert(symbol->symbol == YF_SYMBOL_STR);
  assert(strcmp(symbol->tokens, YF_GLTF_PROP("accessors")) == 0);

  next_symbol(file, symbol); /* : */
  next_symbol(file, symbol); /* [ */

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

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp(YF_GLTF_PROP("bufferView"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          errno = 0;
          accessors->v[index].buffer_view = strtoll(symbol->tokens, NULL, 0);
          if (errno != 0) {
            yf_seterr(YF_ERR_OTHER, __func__);
            return -1;
          }
        } else if (strcmp(YF_GLTF_PROP("byteOffset"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          errno = 0;
          accessors->v[index].byte_off = strtoll(symbol->tokens, NULL, 0);
          if (errno != 0) {
            yf_seterr(YF_ERR_OTHER, __func__);
            return -1;
          }
        } else if (strcmp(YF_GLTF_PROP("count"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          errno = 0;
          accessors->v[index].count = strtoll(symbol->tokens, NULL, 0);
          if (errno != 0) {
            yf_seterr(YF_ERR_OTHER, __func__);
            return -1;
          }
        } else if (strcmp(YF_GLTF_PROP("componentType"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          errno = 0;
          accessors->v[index].comp_type = strtol(symbol->tokens, NULL, 0);
          if (errno != 0) {
            yf_seterr(YF_ERR_OTHER, __func__);
            return -1;
          }
        } else if (strcmp(YF_GLTF_PROP("type"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          if (strcmp(YF_GLTF_PROP("SCALAR"), symbol->tokens) == 0)
            accessors->v[index].type = YF_GLTF_TYPE_SCALAR;
          else if (strcmp(YF_GLTF_PROP("VEC2"), symbol->tokens) == 0)
            accessors->v[index].type = YF_GLTF_TYPE_VEC2;
          else if (strcmp(YF_GLTF_PROP("VEC3"), symbol->tokens) == 0)
            accessors->v[index].type = YF_GLTF_TYPE_VEC3;
          else if (strcmp(YF_GLTF_PROP("VEC4"), symbol->tokens) == 0)
            accessors->v[index].type = YF_GLTF_TYPE_VEC4;
          else if (strcmp(YF_GLTF_PROP("MAT2"), symbol->tokens) == 0)
            accessors->v[index].type = YF_GLTF_TYPE_MAT2;
          else if (strcmp(YF_GLTF_PROP("MAT3"), symbol->tokens) == 0)
            accessors->v[index].type = YF_GLTF_TYPE_MAT3;
          else if (strcmp(YF_GLTF_PROP("MAT4"), symbol->tokens) == 0)
            accessors->v[index].type = YF_GLTF_TYPE_MAT4;
        } else if (strcmp(YF_GLTF_PROP("min"), symbol->tokens) == 0) {
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
        } else if (strcmp(YF_GLTF_PROP("max"), symbol->tokens) == 0) {
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
        } else if (strcmp(YF_GLTF_PROP("name"), symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          accessors->v[index].name = malloc(1+strlen(symbol->tokens));
          if (accessors->v[index].name == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
          }
          strcpy(accessors->v[index].name, symbol->tokens);
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
