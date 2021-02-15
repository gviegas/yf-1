/*
 * YF
 * data-gltf.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#include <yf/com/yf-error.h>

#include "data-gltf.h"
#include "vertex.h"

#define YF_SYMBOL_STR  0
#define YF_SYMBOL_OP   1
#define YF_SYMBOL_NUM  2
#define YF_SYMBOL_BOOL 3
#define YF_SYMBOL_NULL 4
#define YF_SYMBOL_END  5
#define YF_SYMBOL_ERR  6

#define YF_MAXTOKENS 1024

/* Type defining a symbol. */
typedef struct {
  int symbol;
  char tokens[YF_MAXTOKENS];
} L_symbol;

/* Gets the next symbol from a file stream.
   This function returns 'symbol->symbol'. Upon failure, the value returned
   is equal to 'YF_SYMBOL_ERR' - the global error variable is not set. */
static int next_symbol(FILE *file, L_symbol *symbol);

/* Type defining a string. */
typedef char *L_str;

/* Type defining a floating-point number. */
typedef YF_float L_num;

/* Type defining an integer number. */
typedef long long L_int;
#define YF_INT_MIN LLONG_MIN
#define YF_INT_MAX LLONG_MAX

/* Type defining a boolean value. */
typedef int L_bool;
#define YF_TRUE  1
#define YF_FALSE 0

/* Type defining the 'glTF.asset' property. */
typedef struct {
  L_str copyright;
  L_str generator;
  L_str version;
  L_str min_version;
} L_asset;

/* Type defining the 'glTF.scenes' property. */
typedef struct {
  struct {
    L_int *nodes;
    size_t node_n;
    L_str name;
  } *v;
  size_t n;
} L_scenes;

/* Type defining the 'glTF.nodes' property. */
typedef struct {
  struct {
    L_int *children;
    size_t child_n;
    L_int camera;
    L_int mesh;
#define YF_GLTF_XFORM_NONE 0
#define YF_GLTF_XFORM_M    0x01
#define YF_GLTF_XFORM_T    0x02
#define YF_GLTF_XFORM_R    0x04
#define YF_GLTF_XFORM_S    0x08
    unsigned xform_mask;
    union {
      L_num matrix[16];
      struct { L_num t[3]; L_num r[4]; L_num s[3]; } trs;
    };
    L_int skin;
    L_num *weights;
    size_t weight_n;
    L_str name;
  } *v;
  size_t n;
} L_nodes;

/* Type defining the 'glTF.cameras' property. */
typedef struct {
  struct {
#define YF_GLTF_CAMERA_PERSP 0
#define YF_GLTF_CAMERA_ORTHO 1
    int type;
    union {
      struct {
        L_num yfov;
        L_num aspect_ratio;
        L_num znear;
        L_num zfar;
      } persp;
      struct {
        L_num xmag;
        L_num ymag;
        L_num znear;
        L_num zfar;
      } ortho;
    };
    L_str name;
  } *v;
  size_t n;
} L_cameras;

/* Type defining the 'glTF.meshes.primitives.targets' property. */
typedef struct {
  struct {
    L_int position;
    L_int normal;
    L_int tangent;
  } *v;
  size_t n;
} L_targets;

/* Type defining the 'glTF.meshes.primitives' property. */
typedef struct {
  struct {
#define YF_GLTF_ATTR_POS  0
#define YF_GLTF_ATTR_NORM 1
#define YF_GLTF_ATTR_TAN  2
#define YF_GLTF_ATTR_TC0  3
#define YF_GLTF_ATTR_TC1  4
#define YF_GLTF_ATTR_CLR0 5
#define YF_GLTF_ATTR_JNT0 6
#define YF_GLTF_ATTR_WGT0 7
#define YF_GLTF_ATTR_N    8
    L_int attributes[YF_GLTF_ATTR_N];
    L_int indices;
    L_int material;
#define YF_GLTF_MODE_PTS      0
#define YF_GLTF_MODE_LNS      1
#define YF_GLTF_MODE_LNLOOP   2
#define YF_GLTF_MODE_LNSTRIP  3
#define YF_GLTF_MODE_TRIS     4
#define YF_GLTF_MODE_TRISTRIP 5
#define YF_GLTF_MODE_TRIFAN   6
    L_int mode;
    L_targets targets;
  } *v;
  size_t n;
} L_primitives;

/* Type defining the 'glTF.meshes' property. */
typedef struct {
  struct {
    L_primitives primitives;
    L_num *weights;
    size_t weight_n;
    L_str name;
  } *v;
  size_t n;
} L_meshes;

/* Type defining the 'glTF.skins' property. */
typedef struct {
  struct {
    L_int inv_bind_matrices;
    L_int skeleton;
    L_int *joints;
    size_t joint_n;
    L_str name;
  } *v;
  size_t n;
} L_skins;

/* Type defining the 'glTF.*.textureInfo' property. */
typedef struct {
  L_int index;
  L_int tex_coord;
  L_num scale;
  L_num strength;
} L_textureinfo;

/* Type defining the 'glTF.materials' property. */
typedef struct {
  struct {
    struct {
      L_num base_clr_fac[4];
      L_textureinfo base_clr_tex;
      L_num metallic_fac;
      L_num roughness_fac;
      L_textureinfo metal_rough_tex;
    } pbrmr;
    L_textureinfo normal_tex;
    L_textureinfo occlusion_tex;
    L_num emissive_fac[3];
    L_textureinfo emissive_tex;
#define YF_GLTF_ALPHA_OPAQUE 0
#define YF_GLTF_ALPHA_MASK   1
#define YF_GLTF_ALPHA_BLEND  2
    int alpha_mode;
    L_num alpha_cutoff;
    L_bool double_sided;
    L_str name;
  } *v;
  size_t n;
} L_materials;

/* Type defining the 'glTF.accessors.sparse' property. */
typedef struct {
  L_int count;
  struct {
    L_int buffer_view;
    L_int byte_off;
#define YF_GLTF_COMP_UBYTE  5121
#define YF_GLTF_COMP_USHORT 5123
#define YF_GLTF_COMP_UINT   5125
    L_int comp_type;
  } indices;
  struct {
    L_int buffer_view;
    L_int byte_off;
  } values;
} L_sparse;

/* Type defining the 'glTF.accessors' property. */
typedef struct {
  struct {
    L_int buffer_view;
    L_int byte_off;
    L_int count;
#define YF_GLTF_COMP_BYTE   5120
#define YF_GLTF_COMP_UBYTE  5121
#define YF_GLTF_COMP_SHORT  5122
#define YF_GLTF_COMP_USHORT 5123
#define YF_GLTF_COMP_UINT   5125
#define YF_GLTF_COMP_FLOAT  5126
    L_int comp_type;
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
      L_num s;
      L_num v2[2];
      L_num v3[3];
      L_num v4[4];
      L_num m2[4];
      L_num m3[9];
      L_num m4[16];
    } min, max;
    L_bool normalized;
    L_sparse sparse;
    L_str name;
  } *v;
  size_t n;
} L_accessors;

/* Type defining the 'glTF.bufferViews' property. */
typedef struct {
  struct {
    L_int buffer;
    L_int byte_off;
    L_int byte_len;
    L_int byte_strd;
#define YF_GLTF_TARGET_BUF     34962
#define YF_GLTF_TARGET_ELEMBUF 34963
    L_int target;
    L_str name;
  } *v;
  size_t n;
} L_bufferviews;

/* Type defining the 'glTF.buffers' property. */
typedef struct {
  struct {
    L_int byte_len;
    L_str uri;
    L_str name;
  } *v;
  size_t n;
} L_buffers;

/* Type defining the root glTF object. */
typedef struct {
  L_asset asset;
  L_int scene;
  L_scenes scenes;
  L_nodes nodes;
  L_meshes meshes;
  L_skins skins;
  L_materials materials;
  L_accessors accessors;
  L_bufferviews bufferviews;
  L_buffers buffers;
  /* TODO: Other properties. */
} L_gltf;

/* Consumes the current property.
   This allows unknown/unimplemented properties to be ignored. */
static int consume_prop(FILE *file, L_symbol *symbol);

/* Parses an array of unknown size. */
static int parse_array(FILE *file, L_symbol *symbol,
    void **array, size_t *n, size_t elem_sz,
    int (*fn)(FILE *, L_symbol *, size_t, void *), void *arg);

/* Parses a string. */
static int parse_str(FILE *file, L_symbol *symbol, L_str *str);

/* Parses a floating-point number. */
static int parse_num(FILE *file, L_symbol *symbol, L_num *num);

/* Parses an element of an array of floating-point numbers. */
static int parse_num_array(FILE *file, L_symbol *symbol,
    size_t index, void *num_pp);

/* Parses an integer number. */
static int parse_int(FILE *file, L_symbol *symbol, L_int *intr);

/* Parses an element of an array of integer numbers. */
static int parse_int_array(FILE *file, L_symbol *symbol,
    size_t index, void *int_pp);

/* Parses a boolean value. */
static int parse_bool(FILE *file, L_symbol *symbol, L_bool *booln);

/* Parses the root glTF object. */
static int parse_gltf(FILE *file, L_symbol *symbol, L_gltf *gltf);

/* Parses the 'glTF.asset' property. */
static int parse_asset(FILE *file, L_symbol *symbol, L_asset *asset);

/* Parses the 'glTF.scene' property. */
static int parse_scene(FILE *file, L_symbol *symbol, L_int *scene);

/* Parses the 'glTF.scenes' property. */
static int parse_scenes(FILE *file, L_symbol *symbol,
    size_t index, void *scenes_p);

/* Parses the 'glTF.nodes' property. */
static int parse_nodes(FILE *file, L_symbol *symbol,
    size_t index, void *nodes_p);

/* Parses the 'glTF.cameras' property. */
static int parse_cameras(FILE *file, L_symbol *symbol,
    size_t index, void *cameras_p);

/* Parses the 'glTF.meshes.primitives.targets' property. */
static int parse_targets(FILE *file, L_symbol *symbol,
    size_t index, void *targets_p);

/* Parses the 'glTF.meshes.primitives' property. */
static int parse_primitives(FILE *file, L_symbol *symbol,
    size_t index, void *primitives_p);

/* Parses the 'glTF.meshes' property. */
static int parse_meshes(FILE *file, L_symbol *symbol,
    size_t index, void *meshes_p);

/* Parses the 'glTF.skins' property. */
static int parse_skins(FILE *file, L_symbol *symbol,
    size_t index, void *skins_p);

/* Parses the 'glTF.*.textureInfo property. */
static int parse_textureinfo(FILE *file, L_symbol *symbol,
    L_textureinfo *textureinfo);

/* Parses the 'glTF.materials' property. */
static int parse_materials(FILE *file, L_symbol *symbol,
    size_t index, void *materials_p);

/* Parses the 'glTF.accessors.sparse' property. */
static int parse_sparse(FILE *file, L_symbol *symbol, L_sparse *sparse);

/* Parses the 'glTF.accessors' property. */
static int parse_accessors(FILE *file, L_symbol *symbol,
    size_t index, void *accessors_p);

/* Parses the 'glTF.bufferViews' property. */
static int parse_bufferviews(FILE *file, L_symbol *symbol,
    size_t index, void *bufferviews_p);

/* Parses the 'glTF.buffers' property. */
static int parse_buffers(FILE *file, L_symbol *symbol,
    size_t index, void *buffers_p);

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

static int parse_array(FILE *file, L_symbol *symbol,
    void **array, size_t *n, size_t elem_sz,
    int (*fn)(FILE *, L_symbol *, size_t, void *), void *arg)
{
  assert(!feof(file));
  assert(symbol != NULL);
  assert(n != NULL && *n == 0);
  assert(elem_sz > 0);
  assert(fn != NULL);

  next_symbol(file, symbol); /* : */
  size_t i = 0;

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_OP:
        if (symbol->tokens[0] != ']') {
          if (i == *n) {
            const size_t new_n = i == 0 ? 1 : i<<1;
            void *tmp = realloc(*array, new_n*elem_sz);
            if (tmp == NULL) {
              yf_seterr(YF_ERR_NOMEM, __func__);
              return -1;
            }
            *array = tmp;
            *n = new_n;
            memset((char *)*array+i*elem_sz, 0, (new_n-i)*elem_sz);
          }
          if (fn(file, symbol, i++, arg) != 0)
            return -1;
        }
        break;

      default:
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }
  } while (symbol->symbol != YF_SYMBOL_OP || symbol->tokens[0] != ']');

  if (i < *n) {
    *n = i;
    void *tmp = realloc(*array, i*elem_sz);
    if (tmp != NULL)
      *array = tmp;
  }
  return 0;
}

static int parse_str(FILE *file, L_symbol *symbol, L_str *str) {
  assert(!feof(file));
  assert(symbol != NULL);
  assert(str != NULL);

  switch (symbol->symbol) {
    case YF_SYMBOL_OP:
      break;
    default:
      next_symbol(file, symbol);
  }
  if (next_symbol(file, symbol) != YF_SYMBOL_STR) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

  *str = malloc(1+strlen(symbol->tokens));
  if (*str == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  strcpy(*str, symbol->tokens);
  return 0;
}

static int parse_num(FILE *file, L_symbol *symbol, L_num *num) {
  assert(!feof(file));
  assert(symbol != NULL);
  assert(num != NULL);

  switch (symbol->symbol) {
    case YF_SYMBOL_OP:
      break;
    default:
      next_symbol(file, symbol);
  }
  if (next_symbol(file, symbol) != YF_SYMBOL_NUM) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

  errno = 0;
  char *end;
#ifdef YF_USE_FLOAT64
  *num = strtod(symbol->tokens, &end);
#else
  *num = strtof(symbol->tokens, &end);
#endif
  if (errno != 0 || *end != '\0') {
    yf_seterr(YF_ERR_OTHER, __func__);
    return -1;
  }
  return 0;
}

static int parse_num_array(FILE *file, L_symbol *symbol,
    size_t index, void *num_pp)
{
  assert(!feof(file));
  assert(symbol != NULL);
  assert(num_pp != NULL);

  L_num *num_p = *(L_num **)num_pp;
  assert(num_p != NULL);

  return parse_num(file, symbol, num_p+index);
}

static int parse_int(FILE *file, L_symbol *symbol, L_int *intr) {
  assert(!feof(file));
  assert(symbol != NULL);
  assert(intr != NULL);

  switch (symbol->symbol) {
    case YF_SYMBOL_OP:
      break;
    default:
      next_symbol(file, symbol);
  }
  if (next_symbol(file, symbol) != YF_SYMBOL_NUM) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

  errno = 0;
  char *end;
  *intr = strtoll(symbol->tokens, &end, 0);
  if (errno != 0 || *end != '\0') {
    yf_seterr(YF_ERR_OTHER, __func__);
    return -1;
  }
  return 0;
}

static int parse_int_array(FILE *file, L_symbol *symbol,
    size_t index, void *int_pp)
{
  assert(!feof(file));
  assert(symbol != NULL);
  assert(int_pp != NULL);

  L_int *int_p = *(L_int **)int_pp;
  assert(int_p != NULL);

  return parse_int(file, symbol, int_p+index);
}

static int parse_bool(FILE *file, L_symbol *symbol, L_bool *booln) {
  assert(!feof(file));
  assert(symbol != NULL);
  assert(booln != NULL);

  switch (symbol->symbol) {
    case YF_SYMBOL_OP:
      break;
    default:
      next_symbol(file, symbol);
  }
  if (next_symbol(file, symbol) != YF_SYMBOL_BOOL) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

  if (strcmp("true", symbol->tokens) == 0)
    *booln = YF_TRUE;
  else
    *booln = YF_FALSE;
  return 0;
}

static int parse_gltf(FILE *file, L_symbol *symbol, L_gltf *gltf) {
  assert(!feof(file));
  assert(symbol != NULL);
  assert(gltf != NULL);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '{');

  gltf->scene = YF_INT_MIN;

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
          if (parse_array(file, symbol, (void **)&gltf->scenes.v,
                &gltf->scenes.n, sizeof *gltf->scenes.v, parse_scenes,
                &gltf->scenes) != 0)
            return -1;
        } else if (strcmp("nodes", symbol->tokens) == 0) {
          if (parse_array(file, symbol, (void **)&gltf->nodes.v,
                &gltf->nodes.n, sizeof *gltf->nodes.v, parse_nodes,
                &gltf->nodes) != 0)
            return -1;
        } else if (strcmp("meshes", symbol->tokens) == 0) {
          if (parse_array(file, symbol, (void **)&gltf->meshes.v,
                &gltf->meshes.n, sizeof *gltf->meshes.v, parse_meshes,
                &gltf->meshes) != 0)
            return -1;
        } else if (strcmp("skins", symbol->tokens) == 0) {
          if (parse_array(file, symbol, (void **)&gltf->skins.v,
                &gltf->skins.n, sizeof *gltf->skins.v, parse_skins,
                &gltf->skins) != 0)
            return -1;
        } else if (strcmp("materials", symbol->tokens) == 0) {
          if (parse_array(file, symbol, (void **)&gltf->materials.v,
                &gltf->materials.n, sizeof *gltf->materials.v, parse_materials,
                &gltf->materials) != 0)
            return -1;
        } else if (strcmp("accessors", symbol->tokens) == 0) {
          if (parse_array(file, symbol, (void **)&gltf->accessors.v,
                &gltf->accessors.n, sizeof *gltf->accessors.v, parse_accessors,
                &gltf->accessors) != 0)
            return -1;
        } else if (strcmp("bufferViews", symbol->tokens) == 0) {
          if (parse_array(file, symbol, (void **)&gltf->bufferviews.v,
                &gltf->bufferviews.n, sizeof *gltf->bufferviews.v,
                parse_bufferviews, &gltf->bufferviews) != 0)
            return -1;
        } else if (strcmp("buffers", symbol->tokens) == 0) {
          if (parse_array(file, symbol, (void **)&gltf->buffers.v,
                &gltf->buffers.n, sizeof *gltf->buffers.v, parse_buffers,
                &gltf->buffers) != 0)
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
          if (parse_str(file, symbol, &asset->copyright) != 0)
            return -1;
        } else if (strcmp("generator", symbol->tokens) == 0) {
          if (parse_str(file, symbol, &asset->generator) != 0)
            return -1;
        } else if (strcmp("version", symbol->tokens) == 0) {
          if (parse_str(file, symbol, &asset->version) != 0)
            return -1;
        } else if (strcmp("minVersion", symbol->tokens) == 0) {
          if (parse_str(file, symbol, &asset->min_version) != 0)
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

static int parse_scene(FILE *file, L_symbol *symbol, L_int *scene) {
  assert(!feof(file));
  assert(symbol != NULL);
  assert(scene != NULL);
  assert(symbol->symbol == YF_SYMBOL_STR);
  assert(strcmp(symbol->tokens, "scene") == 0);

  return parse_int(file, symbol, scene);
}

static int parse_scenes(FILE *file, L_symbol *symbol,
    size_t index, void *scenes_p)
{
  L_scenes *scenes = scenes_p;

  assert(!feof(file));
  assert(symbol != NULL);
  assert(scenes != NULL);
  assert(index < scenes->n);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '[' || symbol->tokens[0] == ',');

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp("nodes", symbol->tokens) == 0) {
          if (parse_array(file, symbol, (void **)&scenes->v[index].nodes,
                &scenes->v[index].node_n, sizeof *scenes->v[index].nodes,
                parse_int_array, &scenes->v[index].nodes) != 0)
            return -1;
        } else if (strcmp("name", symbol->tokens) == 0) {
          if (parse_str(file, symbol, &scenes->v[index].name) != 0)
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

static int parse_nodes(FILE *file, L_symbol *symbol,
    size_t index, void *nodes_p)
{
  L_nodes *nodes = nodes_p;

  assert(!feof(file));
  assert(symbol != NULL);
  assert(nodes != NULL);
  assert(index < nodes->n);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '[' || symbol->tokens[0] == ',');

  nodes->v[index].mesh = YF_INT_MIN;
  nodes->v[index].camera = YF_INT_MIN;
  nodes->v[index].skin = YF_INT_MIN;

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp("children", symbol->tokens) == 0) {
          if (parse_array(file, symbol, (void **)&nodes->v[index].children,
                &nodes->v[index].child_n, sizeof *nodes->v[index].children,
                parse_int_array, &nodes->v[index].children) != 0)
            return -1;
        } else if (strcmp("camera", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &nodes->v[index].camera) != 0)
            return -1;
        } else if (strcmp("mesh", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &nodes->v[index].mesh) != 0)
            return -1;
        } else if (strcmp("matrix", symbol->tokens) == 0) {
          nodes->v[index].xform_mask = YF_GLTF_XFORM_M;
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* [ */
          for (size_t i = 0; i < 16; ++i) {
            if (parse_num(file, symbol, nodes->v[index].matrix+i) != 0)
              return -1;
          }
          next_symbol(file, symbol); /* ] */
        } else if (strcmp("translation", symbol->tokens) == 0) {
          nodes->v[index].xform_mask |= YF_GLTF_XFORM_T;
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* [ */
          for (size_t i = 0; i < 3; ++i) {
            if (parse_num(file, symbol, nodes->v[index].trs.t+i) != 0)
              return -1;
          }
          next_symbol(file, symbol); /* ] */
        } else if (strcmp("rotation", symbol->tokens) == 0) {
          nodes->v[index].xform_mask |= YF_GLTF_XFORM_R;
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* [ */
          for (size_t i = 0; i < 4; ++i) {
            if (parse_num(file, symbol, nodes->v[index].trs.r+i) != 0)
              return -1;
          }
          next_symbol(file, symbol); /* ] */
        } else if (strcmp("scale", symbol->tokens) == 0) {
          nodes->v[index].xform_mask |= YF_GLTF_XFORM_S;
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* [ */
          for (size_t i = 0; i < 3; ++i) {
            if (parse_num(file, symbol, nodes->v[index].trs.s+i) != 0)
              return -1;
          }
          next_symbol(file, symbol); /* ] */
        } else if (strcmp("skin", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &nodes->v[index].skin) != 0)
            return -1;
        } else if (strcmp("weights", symbol->tokens) == 0) {
          if (parse_array(file, symbol, (void **)&nodes->v[index].weights,
                &nodes->v[index].weight_n, sizeof *nodes->v[index].weights,
                parse_num_array, &nodes->v[index].weights) != 0)
            return -1;
        } else if (strcmp("name", symbol->tokens) == 0) {
          if (parse_str(file, symbol, &nodes->v[index].name) != 0)
            return -1;
        } else {
          if (consume_prop(file, symbol) != 0)
            return -1;
        }
        break;

      case YF_SYMBOL_OP:
        if (symbol->tokens[0] == '}') {
          const unsigned mask = nodes->v[index].xform_mask;
          if (mask != YF_GLTF_XFORM_NONE && !(mask & YF_GLTF_XFORM_M)) {
            if (!(mask & YF_GLTF_XFORM_R))
              nodes->v[index].trs.r[3] = 1.0;
            if (!(mask & YF_GLTF_XFORM_S)) {
              nodes->v[index].trs.s[0] = 1.0;
              nodes->v[index].trs.s[1] = 1.0;
              nodes->v[index].trs.s[2] = 1.0;
            }
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

static int parse_cameras(FILE *file, L_symbol *symbol,
    size_t index, void *cameras_p)
{
  L_cameras *cameras = cameras_p;

  assert(!feof(file));
  assert(symbol != NULL);
  assert(cameras != NULL);
  assert(index < cameras->n);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '[' || symbol->tokens[0] == ',');

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp("type", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          if (strcmp("perspective", symbol->tokens) == 0) {
            cameras->v[index].type = YF_GLTF_CAMERA_PERSP;
          } else if (strcmp("orthographic", symbol->tokens) == 0) {
            cameras->v[index].type = YF_GLTF_CAMERA_ORTHO;
          } else {
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
          }
        } else if (strcmp("perspective", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* { */
          do {
            switch (next_symbol(file, symbol)) {
              case YF_SYMBOL_STR:
                if (strcmp("yfov", symbol->tokens) == 0) {
                  if (parse_num(file, symbol, &cameras->v[index].persp.yfov)
                      != 0)
                    return -1;
                } else if (strcmp("aspectRatio", symbol->tokens) == 0) {
                  if (parse_num(file, symbol,
                        &cameras->v[index].persp.aspect_ratio) != 0)
                    return -1;
                } else if (strcmp("znear", symbol->tokens) == 0) {
                  if (parse_num(file, symbol, &cameras->v[index].persp.znear)
                      != 0)
                    return -1;
                } else if (strcmp("zfar", symbol->tokens) == 0) {
                  if (parse_num(file, symbol, &cameras->v[index].persp.zfar)
                      != 0)
                    return -1;
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
        } else if (strcmp("orthographic", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* { */
          do {
            switch (next_symbol(file, symbol)) {
              case YF_SYMBOL_STR:
                if (strcmp("xmag", symbol->tokens) == 0) {
                  if (parse_num(file, symbol, &cameras->v[index].ortho.xmag)
                      != 0)
                    return -1;
                } else if (strcmp("ymag", symbol->tokens) == 0) {
                  if (parse_num(file, symbol, &cameras->v[index].ortho.ymag)
                      != 0)
                    return -1;
                } else if (strcmp("znear", symbol->tokens) == 0) {
                  if (parse_num(file, symbol, &cameras->v[index].ortho.znear)
                      != 0)
                    return -1;
                } else if (strcmp("zfar", symbol->tokens) == 0) {
                  if (parse_num(file, symbol, &cameras->v[index].ortho.zfar)
                      != 0)
                    return -1;
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
        } else if (strcmp("name", symbol->tokens) == 0) {
          if (parse_str(file, symbol, &cameras->v[index].name) != 0)
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

static int parse_targets(FILE *file, L_symbol *symbol,
    size_t index, void *targets_p)
{
  L_targets *targets = targets_p;

  assert(!feof(file));
  assert(symbol != NULL);
  assert(targets != NULL);
  assert(index < targets->n);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '[' || symbol->tokens[0] == ',');

  targets->v[index].position = YF_INT_MIN;
  targets->v[index].normal = YF_INT_MIN;
  targets->v[index].tangent = YF_INT_MIN;

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp("POSITION", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &targets->v[index].position) != 0)
            return -1;
        } else if (strcmp("NORMAL", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &targets->v[index].normal) != 0)
            return -1;
        } else if (strcmp("TANGENT", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &targets->v[index].tangent) != 0)
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

static int parse_primitives(FILE *file, L_symbol *symbol,
    size_t index, void *primitives_p)
{
  L_primitives *primitives = primitives_p;

  assert(!feof(file));
  assert(symbol != NULL);
  assert(primitives != NULL);
  assert(index < primitives->n);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '[' || symbol->tokens[0] == ',');

  primitives->v[index].indices = YF_INT_MIN;
  primitives->v[index].material = YF_INT_MIN;
  primitives->v[index].mode = YF_GLTF_MODE_TRIS;
  for (size_t i = 0; i < YF_GLTF_ATTR_N; ++i)
    primitives->v[index].attributes[i] = YF_INT_MIN;

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
                  if (parse_int(file, symbol,
                        &primitives->v[index].attributes[YF_GLTF_ATTR_POS])
                      != 0)
                    return -1;
                } else if (strcmp("NORMAL", symbol->tokens) == 0) {
                  if (parse_int(file, symbol,
                        &primitives->v[index].attributes[YF_GLTF_ATTR_NORM])
                      != 0)
                    return -1;
                } else if (strcmp("TANGENT", symbol->tokens) == 0) {
                  if (parse_int(file, symbol,
                        &primitives->v[index].attributes[YF_GLTF_ATTR_TAN])
                      != 0)
                    return -1;
                } else if (strcmp("TEXCOORD_0", symbol->tokens) == 0) {
                  if (parse_int(file, symbol,
                        &primitives->v[index].attributes[YF_GLTF_ATTR_TC0])
                      != 0)
                    return -1;
                } else if (strcmp("TEXCOORD_1", symbol->tokens) == 0) {
                  if (parse_int(file, symbol,
                        &primitives->v[index].attributes[YF_GLTF_ATTR_TC1])
                      != 0)
                    return -1;
                } else if (strcmp("COLOR_0", symbol->tokens) == 0) {
                  if (parse_int(file, symbol,
                        &primitives->v[index].attributes[YF_GLTF_ATTR_CLR0])
                      != 0)
                    return -1;
                } else if (strcmp("JOINTS_0", symbol->tokens) == 0) {
                  if (parse_int(file, symbol,
                        &primitives->v[index].attributes[YF_GLTF_ATTR_JNT0])
                      != 0)
                    return -1;
                } else if (strcmp("WEIGHTS_0", symbol->tokens) == 0) {
                  if (parse_int(file, symbol,
                        &primitives->v[index].attributes[YF_GLTF_ATTR_WGT0])
                      != 0)
                    return -1;
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
        } else if (strcmp("indices", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &primitives->v[index].indices) != 0)
            return -1;
        } else if (strcmp("material", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &primitives->v[index].material) != 0)
            return -1;
        } else if (strcmp("mode", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &primitives->v[index].mode) != 0)
            return -1;
        } else if (strcmp("targets", symbol->tokens) == 0) {
          if (parse_array(file, symbol,
                (void **)&primitives->v[index].targets.v,
                &primitives->v[index].targets.n,
                sizeof *primitives->v[index].targets.v,
                parse_targets, &primitives->v[index].targets) != 0)
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

static int parse_meshes(FILE *file, L_symbol *symbol,
    size_t index, void *meshes_p)
{
  L_meshes *meshes = meshes_p;

  assert(!feof(file));
  assert(symbol != NULL);
  assert(meshes != NULL);
  assert(index < meshes->n);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '[' || symbol->tokens[0] == ',');

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp("primitives", symbol->tokens) == 0) {
          if (parse_array(file, symbol,
                (void **)&meshes->v[index].primitives.v,
                &meshes->v[index].primitives.n,
                sizeof *meshes->v[index].primitives.v,
                parse_primitives, &meshes->v[index].primitives) != 0)
            return -1;
        } else if (strcmp("weights", symbol->tokens) == 0) {
          if (parse_array(file, symbol, (void **)&meshes->v[index].weights,
                &meshes->v[index].weight_n, sizeof *meshes->v[index].weights,
                parse_num_array, &meshes->v[index].weights) != 0)
            return -1;
        } else if (strcmp("name", symbol->tokens) == 0) {
          if (parse_str(file, symbol, &meshes->v[index].name) != 0)
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

static int parse_skins(FILE *file, L_symbol *symbol,
    size_t index, void *skins_p)
{
  L_skins *skins = skins_p;

  assert(!feof(file));
  assert(symbol != NULL);
  assert(skins != NULL);
  assert(index < skins->n);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '[' || symbol->tokens[0] == ',');

  skins->v[index].inv_bind_matrices = YF_INT_MIN;
  skins->v[index].skeleton = YF_INT_MIN;

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp("inverseBindMatrices", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &skins->v[index].inv_bind_matrices) != 0)
            return -1;
        } else if (strcmp("skeleton", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &skins->v[index].skeleton) != 0)
            return -1;
        } else if (strcmp("joints", symbol->tokens) == 0) {
          if (parse_array(file, symbol, (void **)&skins->v[index].joints,
                &skins->v[index].joint_n, sizeof *skins->v[index].joints,
                parse_int_array, &skins->v[index].joints) != 0)
            return -1;
        } else if (strcmp("name", symbol->tokens) == 0) {
          if (parse_str(file, symbol, &skins->v[index].name) != 0)
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

static int parse_textureinfo(FILE *file, L_symbol *symbol,
    L_textureinfo *textureinfo)
{
  assert(!feof(file));
  assert(symbol != NULL);
  assert(textureinfo != NULL);
  assert(textureinfo->tex_coord == 0);

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp("index", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &textureinfo->index) != 0)
            return -1;
        } else if (strcmp("texCoord", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &textureinfo->tex_coord) != 0)
            return -1;
        } else if (strcmp("scale", symbol->tokens) == 0) {
          if (parse_num(file, symbol, &textureinfo->scale) != 0)
            return -1;
        } else if (strcmp("strength", symbol->tokens) == 0) {
          if (parse_num(file, symbol, &textureinfo->strength) != 0)
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

static int parse_materials(FILE *file, L_symbol *symbol,
    size_t index, void *materials_p)
{
  L_materials *materials = materials_p;

  assert(!feof(file));
  assert(symbol != NULL);
  assert(materials != NULL);
  assert(index < materials->n);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '[' || symbol->tokens[0] == ',');

  materials->v[index].pbrmr.base_clr_fac[0] = 1.0;
  materials->v[index].pbrmr.base_clr_fac[1] = 1.0;
  materials->v[index].pbrmr.base_clr_fac[2] = 1.0;
  materials->v[index].pbrmr.base_clr_fac[3] = 1.0;
  materials->v[index].pbrmr.base_clr_tex.index = YF_INT_MIN;
  materials->v[index].pbrmr.metallic_fac = 1.0;
  materials->v[index].pbrmr.roughness_fac = 1.0;
  materials->v[index].pbrmr.metal_rough_tex.index = YF_INT_MIN;
  materials->v[index].normal_tex.index = YF_INT_MIN;
  materials->v[index].normal_tex.scale = 1.0;
  materials->v[index].occlusion_tex.index = YF_INT_MIN;
  materials->v[index].occlusion_tex.strength = 1.0;
  materials->v[index].emissive_tex.index = YF_INT_MIN;
  materials->v[index].alpha_cutoff = 0.5;

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
                  for (size_t i = 0; i < 4; ++i) {
                    if (parse_num(file, symbol,
                          materials->v[index].pbrmr.base_clr_fac+i) != 0)
                      return -1;
                  }
                  next_symbol(file, symbol); /* ] */
                } else if (strcmp("baseColorTexture", symbol->tokens) == 0) {
                  if (parse_textureinfo(file, symbol,
                        &materials->v[index].pbrmr.base_clr_tex) != 0)
                    return -1;
                  next_symbol(file, symbol); /* , } */
                } else if (strcmp("metallicFactor", symbol->tokens) == 0) {
                  if (parse_num(file, symbol,
                        &materials->v[index].pbrmr.metallic_fac) != 0)
                    return -1;
                } else if (strcmp("roughnessFactor", symbol->tokens) == 0) {
                  if (parse_num(file, symbol,
                        &materials->v[index].pbrmr.roughness_fac) != 0)
                    return -1;
                } else if (strcmp("metallicRoughnessTexture", symbol->tokens)
                    == 0)
                {
                  if (parse_textureinfo(file, symbol,
                        &materials->v[index].pbrmr.metal_rough_tex) != 0)
                    return -1;
                  next_symbol(file, symbol); /* , } */
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
        } else if (strcmp("normalTexture", symbol->tokens) == 0) {
          if (parse_textureinfo(file, symbol, &materials->v[index].normal_tex)
              != 0)
            return -1;
        } else if (strcmp("occlusionTexture", symbol->tokens) == 0) {
          if (parse_textureinfo(file, symbol,
                &materials->v[index].occlusion_tex) != 0)
            return -1;
        } else if (strcmp("emissiveFactor", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* [ */
          for (size_t i = 0; i < 3; ++i) {
            if (parse_num(file, symbol, materials->v[index].emissive_fac+i)
                != 0)
              return -1;
          }
          next_symbol(file, symbol); /* ] */
        } else if (strcmp("emissiveTexture", symbol->tokens) == 0) {
          if (parse_textureinfo(file, symbol,
                &materials->v[index].emissive_tex) != 0)
            return -1;
        } else if (strcmp("alphaMode", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol);
          if (strcmp("MASK", symbol->tokens) == 0) {
            materials->v[index].alpha_mode = YF_GLTF_ALPHA_MASK;
          } else if (strcmp("BLEND", symbol->tokens) == 0) {
            materials->v[index].alpha_mode = YF_GLTF_ALPHA_BLEND;
          } else if (strcmp("OPAQUE", symbol->tokens) != 0) {
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
          }
        } else if (strcmp("alphaCutoff", symbol->tokens) == 0) {
          if (parse_num(file, symbol, &materials->v[index].alpha_cutoff) != 0)
            return -1;
        } else if (strcmp("doubleSided", symbol->tokens) == 0) {
          if (parse_bool(file, symbol, &materials->v[index].double_sided) != 0)
            return -1;
        } else if (strcmp("name", symbol->tokens) == 0) {
          if (parse_str(file, symbol, &materials->v[index].name) != 0)
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

static int parse_sparse(FILE *file, L_symbol *symbol, L_sparse *sparse) {
  assert(!feof(file));
  assert(symbol != NULL);
  assert(sparse != NULL);

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp("count", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &sparse->count) != 0)
            return -1;
        } else if (strcmp("indices", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* { */
          do {
            switch (next_symbol(file, symbol)) {
              case YF_SYMBOL_STR:
                if (strcmp("bufferView", symbol->tokens) == 0) {
                  if (parse_int(file, symbol, &sparse->indices.buffer_view)
                      != 0)
                    return -1;
                } else if (strcmp("byteOffset", symbol->tokens) == 0) {
                  if (parse_int(file, symbol, &sparse->indices.byte_off) != 0)
                    return -1;
                } else if (strcmp("componentType", symbol->tokens) == 0) {
                  if (parse_int(file, symbol, &sparse->indices.comp_type) != 0)
                    return -1;
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
        } else if (strcmp("values", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* { */
          do {
            switch (next_symbol(file, symbol)) {
              case YF_SYMBOL_STR:
                if (strcmp("bufferView", symbol->tokens) == 0) {
                  if (parse_int(file, symbol, &sparse->values.buffer_view) != 0)
                    return -1;
                } else if (strcmp("byteOffset", symbol->tokens) == 0) {
                  if (parse_int(file, symbol, &sparse->values.byte_off) != 0)
                    return -1;
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
    size_t index, void *accessors_p)
{
  L_accessors *accessors = accessors_p;

  assert(!feof(file));
  assert(symbol != NULL);
  assert(accessors != NULL);
  assert(index < accessors->n);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '[' || symbol->tokens[0] == ',');

  accessors->v[index].buffer_view = YF_INT_MIN;
  accessors->v[index].sparse.indices.buffer_view = YF_INT_MIN;
  accessors->v[index].sparse.values.buffer_view = YF_INT_MIN;

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp("bufferView", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &accessors->v[index].buffer_view) != 0)
            return -1;
        } else if (strcmp("byteOffset", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &accessors->v[index].byte_off) != 0)
            return -1;
        } else if (strcmp("count", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &accessors->v[index].count) != 0)
            return -1;
        } else if (strcmp("componentType", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &accessors->v[index].comp_type) != 0)
            return -1;
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
            if (parse_num(file, symbol, accessors->v[index].min.m4+i) != 0)
              return -1;
            next_symbol(file, symbol); /* , ] */
            if (symbol->symbol == YF_SYMBOL_OP && symbol->tokens[0] == ']')
              break;
          }
        } else if (strcmp("max", symbol->tokens) == 0) {
          next_symbol(file, symbol); /* : */
          next_symbol(file, symbol); /* [ */
          for (size_t i = 0; i < 16; ++i) {
            if (parse_num(file, symbol, accessors->v[index].max.m4+i) != 0)
              return -1;
            next_symbol(file, symbol); /* , ] */
            if (symbol->symbol == YF_SYMBOL_OP && symbol->tokens[0] == ']')
              break;
          }
        } else if (strcmp("normalized", symbol->tokens) == 0) {
          if (parse_bool(file, symbol, &accessors->v[index].normalized) != 0)
            return -1;
        } else if (strcmp("sparse", symbol->tokens) == 0) {
          if (parse_sparse(file, symbol, &accessors->v[index].sparse) != 0)
            return -1;
        } else if (strcmp("name", symbol->tokens) == 0) {
          if (parse_str(file, symbol, &accessors->v[index].name) != 0)
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

static int parse_bufferviews(FILE *file, L_symbol *symbol,
    size_t index, void *bufferviews_p)
{
  L_bufferviews *bufferviews = bufferviews_p;

  assert(!feof(file));
  assert(symbol != NULL);
  assert(bufferviews != NULL);
  assert(index < bufferviews->n);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '[' || symbol->tokens[0] == ',');

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp("buffer", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &bufferviews->v[index].buffer) != 0)
            return -1;
        } else if (strcmp("byteOffset", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &bufferviews->v[index].byte_off) != 0)
            return -1;
        } else if (strcmp("byteLength", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &bufferviews->v[index].byte_len) != 0)
            return -1;
        } else if (strcmp("byteStride", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &bufferviews->v[index].byte_strd) != 0)
            return -1;
        } else if (strcmp("target", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &bufferviews->v[index].target) != 0)
            return -1;
        } else if (strcmp("name", symbol->tokens) == 0) {
          if (parse_str(file, symbol, &bufferviews->v[index].name) != 0)
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

static int parse_buffers(FILE *file, L_symbol *symbol,
    size_t index, void *buffers_p)
{
  L_buffers *buffers = buffers_p;

  assert(!feof(file));
  assert(symbol != NULL);
  assert(buffers != NULL);
  assert(index < buffers->n);
  assert(symbol->symbol == YF_SYMBOL_OP);
  assert(symbol->tokens[0] == '[' || symbol->tokens[0] == ',');

  do {
    switch (next_symbol(file, symbol)) {
      case YF_SYMBOL_STR:
        if (strcmp("byteLength", symbol->tokens) == 0) {
          if (parse_int(file, symbol, &buffers->v[index].byte_len) != 0)
            return -1;
        } else if (strcmp("uri", symbol->tokens) == 0) {
          if (parse_str(file, symbol, &buffers->v[index].uri) != 0)
            return -1;
        } else if (strcmp("name", symbol->tokens) == 0) {
          if (parse_str(file, symbol, &buffers->v[index].name) != 0)
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

  struct { L_int accessor, view, buffer; } idx, attrs[YF_GLTF_ATTR_N];
  const L_primitives *prim = &gltf->meshes.v[0].primitives;

  idx.accessor = prim->v[0].indices;
  if (idx.accessor != YF_INT_MIN) {
    idx.view = gltf->accessors.v[idx.accessor].buffer_view;
    idx.buffer = gltf->bufferviews.v[idx.view].buffer;
  }
  for (size_t i = 0; i < YF_GLTF_ATTR_N; ++i) {
    attrs[i].accessor = prim->v[0].attributes[i];
    if (attrs[i].accessor != YF_INT_MIN) {
      attrs[i].view = gltf->accessors.v[attrs[i].accessor].buffer_view;
      attrs[i].buffer = gltf->bufferviews.v[attrs[i].view].buffer;
    }
  }
  if (attrs[YF_GLTF_ATTR_POS].accessor == YF_INT_MIN) {
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
  if (idx.accessor != YF_INT_MIN) {
    i_n = gltf->accessors.v[idx.accessor].count;
    inds = malloc(i_n*i_strd);
    if (inds == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      free(verts);
      return -1;
    }
  }

  L_int buf_id = YF_INT_MIN;
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
        buf_id = attrs[i].buffer;
        break;

      case YF_GLTF_ATTR_NORM:
        assert(file != NULL);
        if (buf_id != attrs[i].buffer) {
          fclose(file);
          file = fopen(gltf->buffers.v[attrs[i].buffer].uri, "r");
          if (file == NULL) {
            yf_seterr(YF_ERR_NOFILE, __func__);
            free(verts);
            free(inds);
            return -1;
          }
          buf_id = attrs[i].buffer;
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

      case YF_GLTF_ATTR_TC0:
        assert(file != NULL);
        if (buf_id != attrs[i].buffer) {
          fclose(file);
          file = fopen(gltf->buffers.v[attrs[i].buffer].uri, "r");
          if (file == NULL) {
            yf_seterr(YF_ERR_NOFILE, __func__);
            free(verts);
            free(inds);
            return -1;
          }
          buf_id = attrs[i].buffer;
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
    if (buf_id != idx.buffer) {
      fclose(file);
      file = fopen(gltf->buffers.v[idx.buffer].uri, "r");
      if (file == NULL) {
        yf_seterr(YF_ERR_NOFILE, __func__);
        free(verts);
        free(inds);
        return -1;
      }
      buf_id = idx.buffer;
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

  for (size_t i = 0; i < gltf->nodes.n; ++i) {
    free(gltf->nodes.v[i].children);
    free(gltf->nodes.v[i].weights);
    free(gltf->nodes.v[i].name);
  }
  free(gltf->nodes.v);

  for (size_t i = 0; i < gltf->meshes.n; ++i) {
    for (size_t j = 0; j < gltf->meshes.v[i].primitives.n; ++j)
      free(gltf->meshes.v[i].primitives.v[j].targets.v);
    free(gltf->meshes.v[i].primitives.v);
    free(gltf->meshes.v[i].weights);
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
  printf(" #: %lld\n", gltf->scene);

  puts("glTF.scenes:");
  printf(" n: %lu\n", gltf->scenes.n);
  for (size_t i = 0; i < gltf->scenes.n; ++i) {
    printf(" scene '%s':\n", gltf->scenes.v[i].name);
    printf("  nodes: [ ");
    for (size_t j = 0; j < gltf->scenes.v[i].node_n; ++j)
      printf("%lld ", gltf->scenes.v[i].nodes[j]);
    puts("]");
  }

  puts("glTF.nodes:");
  printf(" n: %lu\n", gltf->nodes.n);
  for (size_t i = 0; i < gltf->nodes.n; ++i) {
    printf(" node '%s':\n", gltf->nodes.v[i].name);
    printf("  children: [ ");
    for (size_t j = 0; j < gltf->nodes.v[i].child_n; ++j)
      printf("%lld ", gltf->nodes.v[i].children[j]);
    puts("]");
    printf("  camera: %lld\n", gltf->nodes.v[i].camera);
    printf("  mesh: %lld\n", gltf->nodes.v[i].mesh);
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
    printf("  skin: %lld\n", gltf->nodes.v[i].skin);
    printf("  weights: [ ");
    for (size_t j = 0; j < gltf->nodes.v[i].weight_n; ++j)
      printf("%.9f ", gltf->nodes.v[i].weights[j]);
    puts("]");
  }

  puts("glTF.meshes:");
  printf(" n: %lu\n", gltf->meshes.n);
  for (size_t i = 0; i < gltf->meshes.n; ++i) {
    printf(" mesh '%s':\n", gltf->meshes.v[i].name);
    printf("  n: %lu\n", gltf->meshes.v[i].primitives.n);
    for (size_t j = 0; j < gltf->meshes.v[i].primitives.n; ++j) {
      printf("  primitives #%lu:\n", j);
      puts("   attributes:");
      printf("    POSITION: %lld\n",
          gltf->meshes.v[i].primitives.v[j].attributes[YF_GLTF_ATTR_POS]);
      printf("    NORMAL: %lld\n",
          gltf->meshes.v[i].primitives.v[j].attributes[YF_GLTF_ATTR_NORM]);
      printf("    TANGENT: %lld\n",
          gltf->meshes.v[i].primitives.v[j].attributes[YF_GLTF_ATTR_TAN]);
      printf("    TEXCOORD_0: %lld\n",
          gltf->meshes.v[i].primitives.v[j].attributes[YF_GLTF_ATTR_TC0]);
      printf("    TEXCOORD_1: %lld\n",
          gltf->meshes.v[i].primitives.v[j].attributes[YF_GLTF_ATTR_TC1]);
      printf("    JOINTS_0: %lld\n",
          gltf->meshes.v[i].primitives.v[j].attributes[YF_GLTF_ATTR_JNT0]);
      printf("    WEIGHTS_0: %lld\n",
          gltf->meshes.v[i].primitives.v[j].attributes[YF_GLTF_ATTR_WGT0]);
      printf("   indices: %lld\n", gltf->meshes.v[i].primitives.v[j].indices);
      printf("   material: %lld\n", gltf->meshes.v[i].primitives.v[j].material);
      printf("   mode: %lld\n", gltf->meshes.v[i].primitives.v[j].mode);
      if (gltf->meshes.v[i].primitives.v[j].targets.n == 0) {
        puts("   (no targets)");
      } else {
        const size_t target_n = gltf->meshes.v[i].primitives.v[j].targets.n;
        for (size_t k = 0; k < target_n; ++k) {
          printf("   targets #%lu:\n", k);
          printf("    POSITION: %lld\n",
              gltf->meshes.v[i].primitives.v[j].targets.v[k].position);
          printf("    NORMAL: %lld\n",
              gltf->meshes.v[i].primitives.v[j].targets.v[k].normal);
          printf("    TANGENT: %lld\n",
              gltf->meshes.v[i].primitives.v[j].targets.v[k].tangent);
        }
      }
    }
    printf("  weights: [ ");
    for (size_t j = 0; j < gltf->meshes.v[i].weight_n; ++j)
      printf("%.9f ", gltf->meshes.v[i].weights[j]);
    puts("]");
  }

  puts("glTF.skins:");
  printf(" n: %lu\n", gltf->skins.n);
  for (size_t i = 0; i < gltf->skins.n; ++i) {
    printf(" skin '%s':\n", gltf->skins.v[i].name);
    printf("  inverseBindMatrices: %lld\n", gltf->skins.v[i].inv_bind_matrices);
    printf("  skeleton: %lld\n", gltf->skins.v[i].skeleton);
    printf("  joints: [ ");
    for (size_t j = 0; j < gltf->skins.v[i].joint_n; ++j)
      printf("%lld ", gltf->skins.v[i].joints[j]);
    puts("]");
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
    puts("   baseColorTexture:");
    printf("    index: %lld\n",
        gltf->materials.v[i].pbrmr.base_clr_tex.index);
    printf("    texCoord: %lld\n",
        gltf->materials.v[i].pbrmr.base_clr_tex.tex_coord);
    printf("   metallicFactor: %.9f\n",
        gltf->materials.v[i].pbrmr.metallic_fac);
    printf("   roughnessFactor: %.9f\n",
        gltf->materials.v[i].pbrmr.roughness_fac);
    puts("   metallicRoughnessTexture:");
    printf("    index: %lld\n",
        gltf->materials.v[i].pbrmr.metal_rough_tex.index);
    printf("    texCoord: %lld\n",
        gltf->materials.v[i].pbrmr.metal_rough_tex.tex_coord);
    puts("  normalTexture:");
    printf("   index: %lld\n",
        gltf->materials.v[i].normal_tex.index);
    printf("   texCoord: %lld\n",
        gltf->materials.v[i].normal_tex.tex_coord);
    printf("   scale: %.9f\n",
        gltf->materials.v[i].normal_tex.scale);
    puts("  occlusionTexture:");
    printf("   index: %lld\n",
        gltf->materials.v[i].occlusion_tex.index);
    printf("   texCoord: %lld\n",
        gltf->materials.v[i].occlusion_tex.tex_coord);
    printf("   strength: %.9f\n",
        gltf->materials.v[i].occlusion_tex.strength);
    printf("   emissiveFactor: [%.9f, %.9f, %.9f]\n",
        gltf->materials.v[i].emissive_fac[0],
        gltf->materials.v[i].emissive_fac[1],
        gltf->materials.v[i].emissive_fac[2]);
    puts("  emissiveTexture:");
    printf("   index: %lld\n",
        gltf->materials.v[i].emissive_tex.index);
    printf("   texCoord: %lld\n",
        gltf->materials.v[i].emissive_tex.tex_coord);
    printf("  alphaMode: %d\n", gltf->materials.v[i].alpha_mode);
    printf("  alphaCutoff: %.9f\n", gltf->materials.v[i].alpha_cutoff);
    printf("  doubleSided: %s\n",
        gltf->materials.v[i].double_sided ? "true" : "false");
  }

  puts("glTF.accessors:");
  printf(" n: %lu\n", gltf->accessors.n);
  for (size_t i = 0; i < gltf->accessors.n; ++i) {
    printf(" accessor '%s':\n", gltf->accessors.v[i].name);
    printf("  bufferView: %lld\n", gltf->accessors.v[i].buffer_view);
    printf("  byteOffset: %lld\n", gltf->accessors.v[i].byte_off);
    printf("  count: %lld\n", gltf->accessors.v[i].count);
    printf("  componenType: %lld\n", gltf->accessors.v[i].comp_type);
    printf("  type: %d\n", gltf->accessors.v[i].type);
    size_t comp_n = 0;
    switch (gltf->accessors.v[i].type) {
      case YF_GLTF_TYPE_SCALAR:
        comp_n = 1;
        break;
      case YF_GLTF_TYPE_VEC2:
        comp_n = 2;
        break;
      case YF_GLTF_TYPE_VEC3:
        comp_n = 3;
        break;
      case YF_GLTF_TYPE_VEC4:
      case YF_GLTF_TYPE_MAT2:
        comp_n = 4;
        break;
      case YF_GLTF_TYPE_MAT3:
        comp_n = 9;
        break;
      case YF_GLTF_TYPE_MAT4:
        comp_n = 16;
        break;
        printf("  min: [%.9f, %.9f, %.9f, %.9f]\n",
            gltf->accessors.v[i].min.v4[0], gltf->accessors.v[i].min.v4[1],
            gltf->accessors.v[i].min.v4[2], gltf->accessors.v[i].min.v4[3]);
        printf("  max: [%.9f, %.9f, %.9f, %.9f]\n",
            gltf->accessors.v[i].max.v4[0], gltf->accessors.v[i].max.v4[1],
            gltf->accessors.v[i].max.v4[2], gltf->accessors.v[i].max.v4[3]);
        break;
    }
    if (comp_n > 1) {
      printf("  min: [ ");
      for (size_t j = 0; j < comp_n; ++j)
        printf("%.9f ", gltf->accessors.v[i].min.m4[j]);
      printf("]\n  max: [ ");
      for (size_t j = 0; j < comp_n; ++j)
        printf("%.9f ", gltf->accessors.v[i].max.m4[j]);
      puts("]");
    } else {
      printf("  min: %.9f\n", gltf->accessors.v[i].min.s);
      printf("  max: %.9f\n", gltf->accessors.v[i].max.s);
    }
    printf("  normalized: %s\n",
        gltf->accessors.v[i].normalized ? "true" : "false");
    puts("  sparse:");
    puts("   indices:");
    printf("    bufferView: %lld\n",
        gltf->accessors.v[i].sparse.indices.buffer_view);
    printf("    byteOffset: %lld\n",
        gltf->accessors.v[i].sparse.indices.byte_off);
    printf("    componenType: %lld\n",
        gltf->accessors.v[i].sparse.indices.comp_type);
    puts("   values:");
    printf("    bufferView: %lld\n",
        gltf->accessors.v[i].sparse.values.buffer_view);
    printf("    byteOffset: %lld\n",
        gltf->accessors.v[i].sparse.values.byte_off);
  }

  puts("glTF.bufferViews:");
  printf(" n: %lu\n", gltf->bufferviews.n);
  for (size_t i = 0; i < gltf->bufferviews.n; ++i) {
    printf(" buffer view '%s':\n", gltf->bufferviews.v[i].name);
    printf("  buffer: %lld\n", gltf->bufferviews.v[i].buffer);
    printf("  byteOffset: %lld\n", gltf->bufferviews.v[i].byte_off);
    printf("  byteLength: %lld\n", gltf->bufferviews.v[i].byte_len);
    printf("  byteStride: %lld\n", gltf->bufferviews.v[i].byte_strd);
    printf("  target: %lld\n", gltf->bufferviews.v[i].target);
  }

  puts("glTF.buffers:");
  printf(" n: %lu\n", gltf->buffers.n);
  for (size_t i = 0; i < gltf->buffers.n; ++i) {
    printf(" buffer '%s':\n", gltf->buffers.v[i].name);
    printf("  byteLength: %lld\n", gltf->buffers.v[i].byte_len);
    printf("  uri: %s\n", gltf->buffers.v[i].uri);
  }
}
#endif
