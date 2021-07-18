/*
 * YF
 * data-gltf.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>

#ifdef _DEFAULT_SOURCE
# include <endian.h>
#else
/* TODO */
# error "Invalid platform"
#endif

#include "yf/com/yf-error.h"

#include "data-gltf.h"
#include "mesh.h"
#include "texture.h"
#include "data-png.h"
#include "yf-scene.h"
#include "yf-node.h"
#include "yf-model.h"

/*
 * Token
 */

#define YF_TOKEN_STR  0
#define YF_TOKEN_NUM  1
#define YF_TOKEN_BOOL 2
#define YF_TOKEN_NULL 3
#define YF_TOKEN_OP   4
#define YF_TOKEN_END  5
#define YF_TOKEN_ERR  6

#define YF_TOKENMAX 1024

/* Type defining a token. */
typedef struct {
    int token;
    char data[YF_TOKENMAX];
} T_token;

/* Gets the next token from a file stream.
   This function returns 'token->token'. Upon failure, the value returned
   is equal to 'YF_TOKEN_ERR' - the global error variable is not set. */
static int next_token(FILE *file, T_token *token)
{
    static_assert(YF_TOKENMAX > 1);
    assert(file != NULL);
    assert(token != NULL);

    int c;
    do c = getc(file); while (isspace(c));

    token->data[0] = c;
    size_t i = 0;

    switch (c) {
    case '"':
        /* XXX: Delim. quotation marks not stored. */
        do {
            c = getc(file);
            token->data[i] = c;
            if (c == '"') {
                token->token = YF_TOKEN_STR;
                break;
            }
            if (c == EOF) {
                token->token = YF_TOKEN_ERR;
                break;
            }
            if (c == '\\') {
                c = getc(file);
                if (c == '"') {
                    token->data[i] = '"';
                } else if (c == '\\') {
                    token->data[i] = '\\';
                } else {
                    /* TODO: Other escape sequences. */
                    token->token = YF_TOKEN_ERR;
                    break;
                }
            }
        } while (++i < YF_TOKENMAX-1);
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
        while (++i < YF_TOKENMAX-1) {
            c = getc(file);
            if (isxdigit(c) || c == '.' || c == '-' || c == '+') {
                token->data[i] = c;
                continue;
            }
            if (c != EOF)
                ungetc(c, file);
            break;
        }
        token->token = YF_TOKEN_NUM;
        break;

    case 't':
        while (++i < YF_TOKENMAX-1) {
            c = getc(file);
            if (islower(c))
                token->data[i] = c;
            else
                break;
        }
        if (c != EOF)
            ungetc(c, file);
        token->data[i] = '\0';
        if (strcmp("true", token->data) == 0)
            token->token = YF_TOKEN_BOOL;
        else
            token->token = YF_TOKEN_ERR;
        break;

    case 'f':
        while (++i < YF_TOKENMAX-1) {
            c = getc(file);
            if (islower(c))
                token->data[i] = c;
            else
                break;
        }
        if (c != EOF)
            ungetc(c, file);
        token->data[i] = '\0';
        if (strcmp("false", token->data) == 0)
            token->token = YF_TOKEN_BOOL;
        else
            token->token = YF_TOKEN_ERR;
        break;

    case 'n':
        while (++i < YF_TOKENMAX-1) {
            c = getc(file);
            if (islower(c))
                token->data[i] = c;
            else
                break;
        }
        if (c != EOF)
            ungetc(c, file);
        token->data[i] = '\0';
        if (strcmp("null", token->data) == 0)
            token->token = YF_TOKEN_NULL;
        else
            token->token = YF_TOKEN_ERR;
        break;

    case ':':
    case ',':
    case '[':
    case ']':
    case '{':
    case '}':
        token->token = YF_TOKEN_OP;
        i++;
        break;

    case EOF:
        token->token = YF_TOKEN_END;
        break;

    default:
        token->token = YF_TOKEN_ERR;
    }

    token->data[i] = '\0';
    return token->token;
}

/*
 * Type parsing
 */

/* Type defining a string. */
typedef char *T_str;

/* Type defining a floating-point number. */
typedef YF_float T_num;

/* Type defining an integer number. */
typedef long long T_int;
#define YF_INT_MIN LLONG_MIN
#define YF_INT_MAX LLONG_MAX

/* Type defining a boolean value. */
typedef int T_bool;
#define YF_TRUE  1
#define YF_FALSE 0

/* Parses an array of unknown size. */
static int parse_array(FILE *file, T_token *token,
                       void **array, size_t *n, size_t elem_sz,
                       int (*fn)(FILE *, T_token *, size_t, void *), void *arg)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(n != NULL && *n == 0);
    assert(elem_sz > 0);
    assert(fn != NULL);

    next_token(file, token); /* ':' */
    size_t i = 0;

    do {
        switch (next_token(file, token)) {
        case YF_TOKEN_OP:
            if (token->data[0] != ']') {
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
                if (fn(file, token, i++, arg) != 0)
                    return -1;
            }
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    } while (token->token != YF_TOKEN_OP || token->data[0] != ']');

    if (i < *n) {
        *n = i;
        void *tmp = realloc(*array, i*elem_sz);
        if (tmp != NULL)
            *array = tmp;
    }
    return 0;
}

/* Parses a string. */
static int parse_str(FILE *file, T_token *token, T_str *str)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(str != NULL);

    switch (token->token) {
    case YF_TOKEN_OP:
        break;
    default:
        next_token(file, token);
    }
    if (next_token(file, token) != YF_TOKEN_STR) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }

    *str = malloc(1+strlen(token->data));
    if (*str == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }
    strcpy(*str, token->data);
    return 0;
}

/* Parses a floating-point number. */
static int parse_num(FILE *file, T_token *token, T_num *num)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(num != NULL);

    switch (token->token) {
    case YF_TOKEN_OP:
        break;
    default:
        next_token(file, token);
    }
    if (next_token(file, token) != YF_TOKEN_NUM) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }

    errno = 0;
    char *end;
#ifdef YF_USE_FLOAT64
    *num = strtod(token->data, &end);
#else
    *num = strtof(token->data, &end);
#endif
    if (errno != 0 || *end != '\0') {
        yf_seterr(YF_ERR_OTHER, __func__);
        return -1;
    }
    return 0;
}

/* Parses an element of an array of floating-point numbers. */
static int parse_num_array(FILE *file, T_token *token,
                           size_t index, void *num_pp)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(num_pp != NULL);

    T_num *num_p = *(T_num **)num_pp;
    assert(num_p != NULL);

    return parse_num(file, token, num_p+index);
}

/* Parses an integer number. */
static int parse_int(FILE *file, T_token *token, T_int *intr)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(intr != NULL);

    switch (token->token) {
    case YF_TOKEN_OP:
        break;
    default:
        next_token(file, token);
    }
    if (next_token(file, token) != YF_TOKEN_NUM) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }

    errno = 0;
    char *end;
    *intr = strtoll(token->data, &end, 0);
    if (errno != 0 || *end != '\0') {
        yf_seterr(YF_ERR_OTHER, __func__);
        return -1;
    }
    return 0;
}

/* Parses an element of an array of integer numbers. */
static int parse_int_array(FILE *file, T_token *token,
                           size_t index, void *int_pp)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(int_pp != NULL);

    T_int *int_p = *(T_int **)int_pp;
    assert(int_p != NULL);

    return parse_int(file, token, int_p+index);
}

/* Parses a boolean value. */
static int parse_bool(FILE *file, T_token *token, T_bool *booln)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(booln != NULL);

    switch (token->token) {
    case YF_TOKEN_OP:
        break;
    default:
        next_token(file, token);
    }
    if (next_token(file, token) != YF_TOKEN_BOOL) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }

    if (strcmp("true", token->data) == 0)
        *booln = YF_TRUE;
    else
        *booln = YF_FALSE;
    return 0;
}

/*
 * Property parsing
 */

/* Consumes the current property.
   This allows unknown/unimplemented properties to be ignored. */
static int consume_prop(FILE *file, T_token *token)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(token->token == YF_TOKEN_STR);

    next_token(file, token); /* ':' */

    if (token->token != YF_TOKEN_OP || token->data[0] != ':') {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }

    switch (next_token(file, token)) {
    case YF_TOKEN_STR:
    case YF_TOKEN_NUM:
    case YF_TOKEN_BOOL:
    case YF_TOKEN_NULL:
        break;

    case YF_TOKEN_OP: {
        char cl, op = token->data[0];
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
            switch (next_token(file, token)) {
            case YF_TOKEN_OP:
                if (token->data[0] == op)
                    n++;
                else if (token->data[0] == cl)
                    n--;
                break;

            case YF_TOKEN_END:
            case YF_TOKEN_ERR:
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

/* Type defining the 'glTF.asset' property. */
typedef struct {
    T_str copyright;
    T_str generator;
    T_str version;
    T_str min_version;
} T_asset;

/* Type defining the 'glTF.scenes' property. */
typedef struct {
    struct {
        T_int *nodes;
        size_t node_n;
        T_str name;
    } *v;
    size_t n;
} T_scenes;

/* Type defining the 'glTF.nodes' property. */
typedef struct {
    struct {
        T_int *children;
        size_t child_n;
        T_int camera;
        T_int mesh;
#define YF_GLTF_XFORM_NONE 0
#define YF_GLTF_XFORM_M    0x01
#define YF_GLTF_XFORM_T    0x02
#define YF_GLTF_XFORM_R    0x04
#define YF_GLTF_XFORM_S    0x08
        unsigned xform_mask;
        union {
            T_num matrix[16];
            struct { T_num t[3], r[4], s[3]; } trs;
        };
        T_int skin;
        T_num *weights;
        size_t weight_n;
        T_str name;
    } *v;
    size_t n;
} T_nodes;

/* Type defining the 'glTF.cameras.perspective' property. */
typedef struct {
    T_num yfov;
    T_num aspect_ratio;
    T_num znear;
    T_num zfar;
} T_perspective;

/* Type defining the 'glTF.cameras.orthographic' property. */
typedef struct {
    T_num xmag;
    T_num ymag;
    T_num znear;
    T_num zfar;
} T_orthographic;

/* Type defining the 'glTF.cameras' property. */
typedef struct {
    struct {
#define YF_GLTF_CAMERA_PERSP 0
#define YF_GLTF_CAMERA_ORTHO 1
        int type;
        union {
            T_perspective persp;
            T_orthographic ortho;
        };
        T_str name;
    } *v;
    size_t n;
} T_cameras;

/* Type defining the 'glTF.meshes.primitives.targets' property. */
typedef struct {
    struct {
        T_int position;
        T_int normal;
        T_int tangent;
    } *v;
    size_t n;
} T_targets;

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
        T_int attributes[YF_GLTF_ATTR_N];
        T_int indices;
        T_int material;
#define YF_GLTF_MODE_PTS      0
#define YF_GLTF_MODE_LNS      1
#define YF_GLTF_MODE_LNLOOP   2
#define YF_GLTF_MODE_LNSTRIP  3
#define YF_GLTF_MODE_TRIS     4
#define YF_GLTF_MODE_TRISTRIP 5
#define YF_GLTF_MODE_TRIFAN   6
        T_int mode;
        T_targets targets;
    } *v;
    size_t n;
} T_primitives;

/* Type defining the 'glTF.meshes' property. */
typedef struct {
    struct {
        T_primitives primitives;
        T_num *weights;
        size_t weight_n;
        T_str name;
    } *v;
    size_t n;
} T_meshes;

/* Type defining the 'glTF.skins' property. */
typedef struct {
    struct {
        T_int inv_bind_matrices;
        T_int skeleton;
        T_int *joints;
        size_t joint_n;
        T_str name;
    } *v;
    size_t n;
} T_skins;

/* Type defining the 'glTF.*.textureInfo' property. */
typedef struct {
    T_int index;
    T_int tex_coord;
    union {
        T_num scale;
        T_num strength;
    };
} T_textureinfo;

/* Type defining the 'glTF.materials.pbrMetallicRoughness' property. */
typedef struct {
    T_num base_clr_fac[4];
    T_textureinfo base_clr_tex;
    T_num metallic_fac;
    T_num roughness_fac;
    T_textureinfo metal_rough_tex;
} T_pbrmetalrough;

/* Type defining the 'glTF.materials' property. */
typedef struct {
    struct {
        T_pbrmetalrough pbrmr;
        T_textureinfo normal_tex;
        T_textureinfo occlusion_tex;
        T_num emissive_fac[3];
        T_textureinfo emissive_tex;
#define YF_GLTF_ALPHA_OPAQUE 0
#define YF_GLTF_ALPHA_MASK   1
#define YF_GLTF_ALPHA_BLEND  2
        int alpha_mode;
        T_num alpha_cutoff;
        T_bool double_sided;
        T_str name;
    } *v;
    size_t n;
} T_materials;

/* Type defining the 'glTF.animations.channels.target' property. */
typedef struct {
    T_int node;
#define YF_GLTF_PATH_XLATE  0
#define YF_GLTF_PATH_ROTATE 1
#define YF_GLTF_PATH_SCALE  2
#define YF_GLTF_PATH_WEIGHT 3
    int path;
} T_ctarget;

/* Type defining the 'glTF.animations.channels' property. */
typedef struct {
    struct {
        T_int sampler;
        T_ctarget target;
    } *v;
    size_t n;
} T_channels;

/* Type defining the 'glTF.animations.samplers' property. */
typedef struct {
    struct {
        T_int input;
        T_int output;
#define YF_GLTF_ERP_LINEAR 0
#define YF_GLTF_ERP_STEP   1
#define YF_GLTF_ERP_CUBIC  2
        int interpolation;
    } *v;
    size_t n;
} T_asamplers;

/* Type defining the 'glTF.animations' property. */
typedef struct {
    struct {
        T_channels channels;
        T_asamplers samplers;
        T_str name;
    } *v;
    size_t n;
} T_animations;

/* Type defining the 'glTF.accessors.sparse.indices' property. */
typedef struct {
    T_int buffer_view;
    T_int byte_off;
#define YF_GLTF_COMP_UBYTE  5121
#define YF_GLTF_COMP_USHORT 5123
#define YF_GLTF_COMP_UINT   5125
    T_int comp_type;
} T_sindices;

/* Type defining the 'glTF.accessors.sparse.values' property. */
typedef struct {
    T_int buffer_view;
    T_int byte_off;
} T_svalues;

/* Type defining the 'glTF.accessors.sparse' property. */
typedef struct {
    T_int count;
    T_sindices indices;
    T_svalues values;
} T_sparse;

/* Type defining the 'glTF.accessors' property. */
typedef struct {
    struct {
        T_int buffer_view;
        T_int byte_off;
        T_int count;
#define YF_GLTF_COMP_BYTE   5120
#define YF_GLTF_COMP_UBYTE  5121
#define YF_GLTF_COMP_SHORT  5122
#define YF_GLTF_COMP_USHORT 5123
#define YF_GLTF_COMP_UINT   5125
#define YF_GLTF_COMP_FLOAT  5126
        T_int comp_type;
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
            T_num s;
            T_num v2[2];
            T_num v3[3];
            T_num v4[4];
            T_num m2[4];
            T_num m3[9];
            T_num m4[16];
        } min, max;
        T_bool normalized;
        T_sparse sparse;
        T_str name;
    } *v;
    size_t n;
} T_accessors;

/* Type defining the 'glTF.bufferViews' property. */
typedef struct {
    struct {
        T_int buffer;
        T_int byte_off;
        T_int byte_len;
        T_int byte_strd;
#define YF_GLTF_TARGET_BUF     34962
#define YF_GLTF_TARGET_ELEMBUF 34963
        T_int target;
        T_str name;
    } *v;
    size_t n;
} T_bufferviews;

/* Type defining the 'glTF.buffers' property. */
typedef struct {
    struct {
        T_int byte_len;
        T_str uri;
        T_str name;
    } *v;
    size_t n;
} T_buffers;

/* Type defining the 'glTF.textures' property. */
typedef struct {
    struct {
        T_int sampler;
        T_int source;
        T_str name;
    } *v;
    size_t n;
} T_textures;

/* Type defining the 'glTF.images' property. */
typedef struct {
    struct {
        T_str uri;
        T_str mime_type;
        T_int buffer_view;
        T_str name;
    } *v;
    size_t n;
} T_images;

/* Type defining the 'glTF.samplers' property. */
typedef struct {
    struct {
#define YF_GLTF_FILTER_NEAREST 9728
#define YF_GLTF_FILTER_LINEAR  9729
#define YF_GLTF_FILTER_NRMIPNR 9984
#define YF_GLTF_FILTER_LNMIPNR 9985
#define YF_GLTF_FILTER_NRMIPLN 9986
#define YF_GLTF_FILTER_LNMIPLN 9987
        T_int min_filter;
        T_int mag_filter;
#define YF_GLTF_WRAP_CLAMP  33071
#define YF_GLTF_WRAP_MIRROR 33648
#define YF_GLTF_WRAP_REPEAT 10497
        T_int wrap_s;
        T_int wrap_t;
        T_str name;
    } *v;
    size_t n;
} T_samplers;

/* Type defining the root glTF object. */
typedef struct {
    T_asset asset;
    T_int scene;
    T_scenes scenes;
    T_nodes nodes;
    T_cameras cameras;
    T_meshes meshes;
    T_skins skins;
    T_materials materials;
    T_animations animations;
    T_accessors accessors;
    T_bufferviews bufferviews;
    T_buffers buffers;
    T_textures textures;
    T_images images;
    T_samplers samplers;
} T_gltf;

/* Parses the 'glTF.asset' property. */
static int parse_asset(FILE *file, T_token *token, T_asset *asset)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(asset != NULL);
    assert(token->token == YF_TOKEN_STR);
    assert(strcmp(token->data, "asset") == 0);

    next_token(file, token); /* ':' */
    next_token(file, token); /* '{' */

    if (token->token != YF_TOKEN_OP || token->data[0] != '{') {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("copyright", token->data) == 0) {
                if (parse_str(file, token, &asset->copyright) != 0)
                    return -1;

            } else if (strcmp("generator", token->data) == 0) {
                if (parse_str(file, token, &asset->generator) != 0)
                    return -1;

            } else if (strcmp("version", token->data) == 0) {
                if (parse_str(file, token, &asset->version) != 0)
                    return -1;

            } else if (strcmp("minVersion", token->data) == 0) {
                if (parse_str(file, token, &asset->min_version) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.scene' property. */
static int parse_scene(FILE *file, T_token *token, T_int *scene)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(scene != NULL);
    assert(token->token == YF_TOKEN_STR);
    assert(strcmp(token->data, "scene") == 0);

    return parse_int(file, token, scene);
}

/* Parses the 'glTF.scenes' property. */
static int parse_scenes(FILE *file, T_token *token,
                        size_t index, void *scenes_p)
{
    T_scenes *scenes = scenes_p;

    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(scenes != NULL);
    assert(index < scenes->n);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '[' || token->data[0] == ',');

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("nodes", token->data) == 0) {
                if (parse_array(file, token, (void **)&scenes->v[index].nodes,
                                &scenes->v[index].node_n,
                                sizeof *scenes->v[index].nodes,
                                parse_int_array, &scenes->v[index].nodes) != 0)
                    return -1;

            } else if (strcmp("name", token->data) == 0) {
                if (parse_str(file, token, &scenes->v[index].name) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.nodes' property. */
static int parse_nodes(FILE *file, T_token *token,
                       size_t index, void *nodes_p)
{
    T_nodes *nodes = nodes_p;

    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(nodes != NULL);
    assert(index < nodes->n);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '[' || token->data[0] == ',');

    nodes->v[index].mesh = YF_INT_MIN;
    nodes->v[index].camera = YF_INT_MIN;
    nodes->v[index].skin = YF_INT_MIN;

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("children", token->data) == 0) {
                if (parse_array(file, token, (void **)&nodes->v[index].children,
                                &nodes->v[index].child_n,
                                sizeof *nodes->v[index].children,
                                parse_int_array,
                                &nodes->v[index].children) != 0)
                    return -1;

            } else if (strcmp("camera", token->data) == 0) {
                if (parse_int(file, token, &nodes->v[index].camera) != 0)
                    return -1;

            } else if (strcmp("mesh", token->data) == 0) {
                if (parse_int(file, token, &nodes->v[index].mesh) != 0)
                    return -1;

            } else if (strcmp("matrix", token->data) == 0) {
                nodes->v[index].xform_mask = YF_GLTF_XFORM_M;
                next_token(file, token); /* ':' */
                next_token(file, token); /* '[' */
                for (size_t i = 0; i < 16; i++) {
                    if (parse_num(file, token, nodes->v[index].matrix+i) != 0)
                        return -1;
                }
                next_token(file, token); /* ']' */

            } else if (strcmp("translation", token->data) == 0) {
                nodes->v[index].xform_mask |= YF_GLTF_XFORM_T;
                next_token(file, token); /* ':' */
                next_token(file, token); /* '[' */
                for (size_t i = 0; i < 3; i++) {
                    if (parse_num(file, token, nodes->v[index].trs.t+i) != 0)
                        return -1;
                }
                next_token(file, token); /* ']' */

            } else if (strcmp("rotation", token->data) == 0) {
                nodes->v[index].xform_mask |= YF_GLTF_XFORM_R;
                next_token(file, token); /* ':' */
                next_token(file, token); /* '[' */
                for (size_t i = 0; i < 4; i++) {
                    if (parse_num(file, token, nodes->v[index].trs.r+i) != 0)
                        return -1;
                }
                next_token(file, token); /* ']' */

            } else if (strcmp("scale", token->data) == 0) {
                nodes->v[index].xform_mask |= YF_GLTF_XFORM_S;
                next_token(file, token); /* ':' */
                next_token(file, token); /* '[' */
                for (size_t i = 0; i < 3; i++) {
                    if (parse_num(file, token, nodes->v[index].trs.s+i) != 0)
                        return -1;
                }
                next_token(file, token); /* ']' */

            } else if (strcmp("skin", token->data) == 0) {
                if (parse_int(file, token, &nodes->v[index].skin) != 0)
                    return -1;

            } else if (strcmp("weights", token->data) == 0) {
                if (parse_array(file, token, (void **)&nodes->v[index].weights,
                                &nodes->v[index].weight_n,
                                sizeof *nodes->v[index].weights,
                                parse_num_array, &nodes->v[index].weights) != 0)
                    return -1;

            } else if (strcmp("name", token->data) == 0) {
                if (parse_str(file, token, &nodes->v[index].name) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}') {
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
    }

    return 0;
}

/* Parses the 'glTF.camera.perspective' property. */
static int parse_perspective(FILE *file, T_token *token,
                             T_perspective *perspective)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(perspective != NULL);
    assert(token->token == YF_TOKEN_STR);
    assert(strcmp(token->data, "perspective") == 0);

    next_token(file, token); /* ':' */
    next_token(file, token); /* '{' */

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("yfov", token->data) == 0) {
                if (parse_num(file, token, &perspective->yfov) != 0)
                    return -1;

            } else if (strcmp("aspectRatio", token->data) == 0) {
                if (parse_num(file, token, &perspective->aspect_ratio) != 0)
                    return -1;

            } else if (strcmp("znear", token->data) == 0) {
                if (parse_num(file, token, &perspective->znear) != 0)
                    return -1;

            } else if (strcmp("zfar", token->data) == 0) {
                if (parse_num(file, token, &perspective->zfar) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.camera.orthographic' property. */
static int parse_orthographic(FILE *file, T_token *token,
                              T_orthographic *orthographic)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(orthographic != NULL);
    assert(token->token == YF_TOKEN_STR);
    assert(strcmp(token->data, "orthographic") == 0);

    next_token(file, token); /* ':' */
    next_token(file, token); /* '{' */

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("xmag", token->data) == 0) {
                if (parse_num(file, token, &orthographic->xmag) != 0)
                    return -1;

            } else if (strcmp("ymag", token->data) == 0) {
                if (parse_num(file, token, &orthographic->ymag) != 0)
                    return -1;

            } else if (strcmp("znear", token->data) == 0) {
                if (parse_num(file, token, &orthographic->znear) != 0)
                    return -1;

            } else if (strcmp("zfar", token->data) == 0) {
                if (parse_num(file, token, &orthographic->zfar) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.cameras' property. */
static int parse_cameras(FILE *file, T_token *token,
                         size_t index, void *cameras_p)
{
    T_cameras *cameras = cameras_p;

    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(cameras != NULL);
    assert(index < cameras->n);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '[' || token->data[0] == ',');

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("type", token->data) == 0) {
                next_token(file, token); /* ':' */
                next_token(file, token);
                if (strcmp("perspective", token->data) == 0) {
                    cameras->v[index].type = YF_GLTF_CAMERA_PERSP;
                } else if (strcmp("orthographic", token->data) == 0) {
                    cameras->v[index].type = YF_GLTF_CAMERA_ORTHO;
                } else {
                    yf_seterr(YF_ERR_INVFILE, __func__);
                    return -1;
                }

            } else if (strcmp("perspective", token->data) == 0) {
                if (parse_perspective(file, token,
                                      &cameras->v[index].persp) != 0)
                    return -1;

            } else if (strcmp("orthographic", token->data) == 0) {
                if (parse_orthographic(file, token,
                                       &cameras->v[index].ortho) != 0)
                    return -1;

            } else if (strcmp("name", token->data) == 0) {
                if (parse_str(file, token, &cameras->v[index].name) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.meshes.primitives.attributes' property. */
static int parse_attributes(FILE *file, T_token *token, T_int *attributes)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(attributes != NULL);
    assert(token->token == YF_TOKEN_STR);
    assert(strcmp(token->data, "attributes") == 0);

    next_token(file, token); /* ':' */
    next_token(file, token); /* '{' */

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("POSITION", token->data) == 0) {
                if (parse_int(file, token, attributes+YF_GLTF_ATTR_POS) != 0)
                    return -1;

            } else if (strcmp("NORMAL", token->data) == 0) {
                if (parse_int(file, token, attributes+YF_GLTF_ATTR_NORM) != 0)
                    return -1;

            } else if (strcmp("TANGENT", token->data) == 0) {
                if (parse_int(file, token, attributes+YF_GLTF_ATTR_TAN) != 0)
                    return -1;

            } else if (strcmp("TEXCOORD_0", token->data) == 0) {
                if (parse_int(file, token, attributes+YF_GLTF_ATTR_TC0) != 0)
                    return -1;

            } else if (strcmp("TEXCOORD_1", token->data) == 0) {
                if (parse_int(file, token, attributes+YF_GLTF_ATTR_TC1) != 0)
                    return -1;

            } else if (strcmp("COLOR_0", token->data) == 0) {
                if (parse_int(file, token, attributes+YF_GLTF_ATTR_CLR0) != 0)
                    return -1;

            } else if (strcmp("JOINTS_0", token->data) == 0) {
                if (parse_int(file, token, attributes+YF_GLTF_ATTR_JNT0) != 0)
                    return -1;

            } else if (strcmp("WEIGHTS_0", token->data) == 0) {
                if (parse_int(file, token, attributes+YF_GLTF_ATTR_WGT0) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.meshes.primitives.targets' property. */
static int parse_targets(FILE *file, T_token *token,
                         size_t index, void *targets_p)
{
    T_targets *targets = targets_p;

    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(targets != NULL);
    assert(index < targets->n);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '[' || token->data[0] == ',');

    targets->v[index].position = YF_INT_MIN;
    targets->v[index].normal = YF_INT_MIN;
    targets->v[index].tangent = YF_INT_MIN;

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("POSITION", token->data) == 0) {
                if (parse_int(file, token, &targets->v[index].position) != 0)
                    return -1;

            } else if (strcmp("NORMAL", token->data) == 0) {
                if (parse_int(file, token, &targets->v[index].normal) != 0)
                    return -1;

            } else if (strcmp("TANGENT", token->data) == 0) {
                if (parse_int(file, token, &targets->v[index].tangent) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.meshes.primitives' property. */
static int parse_primitives(FILE *file, T_token *token,
                            size_t index, void *primitives_p)
{
    T_primitives *primitives = primitives_p;

    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(primitives != NULL);
    assert(index < primitives->n);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '[' || token->data[0] == ',');

    for (size_t i = 0; i < YF_GLTF_ATTR_N; i++)
        primitives->v[index].attributes[i] = YF_INT_MIN;
    primitives->v[index].indices = YF_INT_MIN;
    primitives->v[index].material = YF_INT_MIN;
    primitives->v[index].mode = YF_GLTF_MODE_TRIS;

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("attributes", token->data) == 0) {
                if (parse_attributes(file, token,
                                     primitives->v[index].attributes) != 0)
                    return -1;

            } else if (strcmp("indices", token->data) == 0) {
                if (parse_int(file, token,
                              &primitives->v[index].indices) != 0)
                    return -1;

            } else if (strcmp("material", token->data) == 0) {
                if (parse_int(file, token,
                              &primitives->v[index].material) != 0)
                    return -1;

            } else if (strcmp("mode", token->data) == 0) {
                if (parse_int(file, token,
                              &primitives->v[index].mode) != 0)
                    return -1;

            } else if (strcmp("targets", token->data) == 0) {
                if (parse_array(file, token,
                                (void **)&primitives->v[index].targets.v,
                                &primitives->v[index].targets.n,
                                sizeof *primitives->v[index].targets.v,
                                parse_targets,
                                &primitives->v[index].targets) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.meshes' property. */
static int parse_meshes(FILE *file, T_token *token,
                        size_t index, void *meshes_p)
{
    T_meshes *meshes = meshes_p;

    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(meshes != NULL);
    assert(index < meshes->n);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '[' || token->data[0] == ',');

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("primitives", token->data) == 0) {
                if (parse_array(file, token,
                                (void **)&meshes->v[index].primitives.v,
                                &meshes->v[index].primitives.n,
                                sizeof *meshes->v[index].primitives.v,
                                parse_primitives,
                                &meshes->v[index].primitives) != 0)
                    return -1;

            } else if (strcmp("weights", token->data) == 0) {
                if (parse_array(file, token, (void **)&meshes->v[index].weights,
                                &meshes->v[index].weight_n,
                                sizeof *meshes->v[index].weights,
                                parse_num_array,
                                &meshes->v[index].weights) != 0)
                    return -1;

            } else if (strcmp("name", token->data) == 0) {
                if (parse_str(file, token, &meshes->v[index].name) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.skins' property. */
static int parse_skins(FILE *file, T_token *token,
                       size_t index, void *skins_p)
{
    T_skins *skins = skins_p;

    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(skins != NULL);
    assert(index < skins->n);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '[' || token->data[0] == ',');

    skins->v[index].inv_bind_matrices = YF_INT_MIN;
    skins->v[index].skeleton = YF_INT_MIN;

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("inverseBindMatrices", token->data) == 0) {
                if (parse_int(file, token,
                              &skins->v[index].inv_bind_matrices) != 0)
                    return -1;

            } else if (strcmp("skeleton", token->data) == 0) {
                if (parse_int(file, token, &skins->v[index].skeleton) != 0)
                    return -1;

            } else if (strcmp("joints", token->data) == 0) {
                if (parse_array(file, token, (void **)&skins->v[index].joints,
                                &skins->v[index].joint_n,
                                sizeof *skins->v[index].joints,
                                parse_int_array, &skins->v[index].joints) != 0)
                    return -1;

            } else if (strcmp("name", token->data) == 0) {
                if (parse_str(file, token, &skins->v[index].name) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.*.textureInfo property. */
static int parse_textureinfo(FILE *file, T_token *token,
                             T_textureinfo *textureinfo)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(textureinfo != NULL);
    assert(textureinfo->tex_coord == 0);

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("index", token->data) == 0) {
                if (parse_int(file, token, &textureinfo->index) != 0)
                    return -1;

            } else if (strcmp("texCoord", token->data) == 0) {
                if (parse_int(file, token, &textureinfo->tex_coord) != 0)
                    return -1;

            } else if (strcmp("scale", token->data) == 0) {
                if (parse_num(file, token, &textureinfo->scale) != 0)
                    return -1;

            } else if (strcmp("strength", token->data) == 0) {
                if (parse_num(file, token, &textureinfo->strength) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.materials.pbrMetallicRoughness' property. */
static int parse_pbrmetalrough(FILE *file, T_token *token,
                               T_pbrmetalrough *pbrmetalrough)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(token->token == YF_TOKEN_STR);
    assert(strcmp(token->data, "pbrMetallicRoughness") == 0);

    next_token(file, token); /* ':' */
    next_token(file, token); /* '{' */

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("baseColorFactor", token->data) == 0) {
                next_token(file, token); /* ':' */
                next_token(file, token); /* '[' */
                for (size_t i = 0; i < 4; i++) {
                    if (parse_num(file, token,
                                  pbrmetalrough->base_clr_fac+i) != 0)
                        return -1;
                }
                next_token(file, token); /* ']' */

            } else if (strcmp("baseColorTexture", token->data) == 0) {
                if (parse_textureinfo(file, token,
                                      &pbrmetalrough->base_clr_tex) != 0)
                    return -1;

            } else if (strcmp("metallicFactor", token->data) == 0) {
                if (parse_num(file, token,
                              &pbrmetalrough->metallic_fac) != 0)
                    return -1;

            } else if (strcmp("roughnessFactor", token->data) == 0) {
                if (parse_num(file, token,
                              &pbrmetalrough->roughness_fac) != 0)
                    return -1;

            } else if (strcmp("metallicRoughnessTexture", token->data) == 0) {
                if (parse_textureinfo(file, token,
                                      &pbrmetalrough->metal_rough_tex) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.materials' property. */
static int parse_materials(FILE *file, T_token *token,
                           size_t index, void *materials_p)
{
    T_materials *materials = materials_p;

    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(materials != NULL);
    assert(index < materials->n);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '[' || token->data[0] == ',');

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

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("pbrMetallicRoughness", token->data) == 0) {
                if (parse_pbrmetalrough(file, token,
                                        &materials->v[index].pbrmr) != 0)
                    return -1;

            } else if (strcmp("normalTexture", token->data) == 0) {
                if (parse_textureinfo(file, token,
                                      &materials->v[index].normal_tex) != 0)
                    return -1;

            } else if (strcmp("occlusionTexture", token->data) == 0) {
                if (parse_textureinfo(file, token,
                                      &materials->v[index].occlusion_tex) != 0)
                    return -1;

            } else if (strcmp("emissiveFactor", token->data) == 0) {
                next_token(file, token); /* ':' */
                next_token(file, token); /* '[' */
                for (size_t i = 0; i < 3; i++) {
                    if (parse_num(file, token,
                                  materials->v[index].emissive_fac+i) != 0)
                        return -1;
                }
                next_token(file, token); /* ']' */

            } else if (strcmp("emissiveTexture", token->data) == 0) {
                if (parse_textureinfo(file, token,
                                      &materials->v[index].emissive_tex) != 0)
                    return -1;

            } else if (strcmp("alphaMode", token->data) == 0) {
                next_token(file, token); /* ':' */
                next_token(file, token);
                if (strcmp("OPAQUE", token->data) == 0) {
                    materials->v[index].alpha_mode = YF_GLTF_ALPHA_OPAQUE;
                } else if (strcmp("MASK", token->data) == 0) {
                    materials->v[index].alpha_mode = YF_GLTF_ALPHA_MASK;
                } else if (strcmp("BLEND", token->data) == 0) {
                    materials->v[index].alpha_mode = YF_GLTF_ALPHA_BLEND;
                } else {
                    yf_seterr(YF_ERR_INVFILE, __func__);
                    return -1;
                }

            } else if (strcmp("alphaCutoff", token->data) == 0) {
                if (parse_num(file, token,
                              &materials->v[index].alpha_cutoff) != 0)
                    return -1;

            } else if (strcmp("doubleSided", token->data) == 0) {
                if (parse_bool(file, token,
                               &materials->v[index].double_sided) != 0)
                    return -1;

            } else if (strcmp("name", token->data) == 0) {
                if (parse_str(file, token, &materials->v[index].name) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.animations.channels.target' property. */
static int parse_ctarget(FILE *file, T_token *token, T_ctarget *ctarget)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(ctarget != NULL);
    assert(token->token == YF_TOKEN_STR);
    assert(strcmp(token->data, "target") == 0);

    next_token(file, token); /* ':' */
    next_token(file, token); /* '{' */

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("node", token->data) == 0) {
                if (parse_int(file, token, &ctarget->node) != 0)
                    return -1;

            } else if (strcmp("path", token->data) == 0) {
                next_token(file, token); /* ':' */
                next_token(file, token);
                if (strcmp("translation", token->data) == 0) {
                    ctarget->path = YF_GLTF_PATH_XLATE;
                } else if (strcmp("rotation", token->data) == 0) {
                    ctarget->path = YF_GLTF_PATH_ROTATE;
                } else if (strcmp("scale", token->data) == 0) {
                    ctarget->path = YF_GLTF_PATH_SCALE;
                } else if (strcmp("weights", token->data) == 0) {
                    ctarget->path = YF_GLTF_PATH_WEIGHT;
                } else {
                    yf_seterr(YF_ERR_INVFILE, __func__);
                    return -1;
                }

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.animations.channels' property. */
static int parse_channels(FILE *file, T_token *token,
                          size_t index, void *channels_p)
{
    T_channels *channels = channels_p;

    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(channels != NULL);
    assert(index < channels->n);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '[' || token->data[0] == ',');

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("sampler", token->data) == 0) {
                if (parse_int(file, token, &channels->v[index].sampler) != 0)
                    return -1;

            } else if (strcmp("target", token->data) == 0) {
                if (parse_ctarget(file, token, &channels->v[index].target) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.animations.samplers' property. */
static int parse_asamplers(FILE *file, T_token *token,
                           size_t index, void *asamplers_p)
{
    T_asamplers *asamplers = asamplers_p;

    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(asamplers != NULL);
    assert(index < asamplers->n);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '[' || token->data[0] == ',');

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("input", token->data) == 0) {
                if (parse_int(file, token, &asamplers->v[index].input) != 0)
                    return -1;

            } else if (strcmp("output", token->data) == 0) {
                if (parse_int(file, token, &asamplers->v[index].output) != 0)
                    return -1;

            } else if (strcmp("interpolation", token->data) == 0) {
                next_token(file, token); /* ':' */
                next_token(file, token);
                if (strcmp("LINEAR", token->data) == 0) {
                    asamplers->v[index].interpolation = YF_GLTF_ERP_LINEAR;
                } else if (strcmp("STEP", token->data) == 0) {
                    asamplers->v[index].interpolation = YF_GLTF_ERP_STEP;
                } else if (strcmp("CUBICSPLINE", token->data) == 0) {
                    asamplers->v[index].interpolation = YF_GLTF_ERP_CUBIC;
                } else {
                    yf_seterr(YF_ERR_INVFILE, __func__);
                    return -1;
                }

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.animations' property. */
static int parse_animations(FILE *file, T_token *token,
                            size_t index, void *animations_p)
{
    T_animations *animations = animations_p;

    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(animations != NULL);
    assert(index < animations->n);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '[' || token->data[0] == ',');

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("channels", token->data) == 0) {
                if (parse_array(file, token,
                                (void **)&animations->v[index].channels.v,
                                &animations->v[index].channels.n,
                                sizeof *animations->v[index].channels.v,
                                parse_channels,
                                &animations->v[index].channels) != 0)
                    return -1;

            } else if (strcmp("samplers", token->data) == 0) {
                if (parse_array(file, token,
                                (void **)&animations->v[index].samplers.v,
                                &animations->v[index].samplers.n,
                                sizeof *animations->v[index].samplers.v,
                                parse_asamplers,
                                &animations->v[index].samplers) != 0)
                    return -1;

            } else if (strcmp("name", token->data) == 0) {
                if (parse_str(file, token, &animations->v[index].name) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.accessors.sparse.indices' property. */
static int parse_sindices(FILE *file, T_token *token, T_sindices *sindices)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(sindices != NULL);
    assert(token->token == YF_TOKEN_STR);
    assert(strcmp(token->data, "indices") == 0);

    next_token(file, token); /* ':' */
    next_token(file, token); /* '{' */

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("bufferView", token->data) == 0) {
                if (parse_int(file, token, &sindices->buffer_view) != 0)
                    return -1;

            } else if (strcmp("byteOffset", token->data) == 0) {
                if (parse_int(file, token, &sindices->byte_off) != 0)
                    return -1;

            } else if (strcmp("componentType", token->data) == 0) {
                if (parse_int(file, token, &sindices->comp_type) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.accessors.sparse.values' property. */
static int parse_svalues(FILE *file, T_token *token, T_svalues *svalues)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(svalues != NULL);
    assert(token->token == YF_TOKEN_STR);
    assert(strcmp(token->data, "values") == 0);

    next_token(file, token); /* ':' */
    next_token(file, token); /* '{' */

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("bufferView", token->data) == 0) {
                if (parse_int(file, token, &svalues->buffer_view) != 0)
                    return -1;

            } else if (strcmp("byteOffset", token->data) == 0) {
                if (parse_int(file, token, &svalues->byte_off) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.accessors.sparse' property. */
static int parse_sparse(FILE *file, T_token *token, T_sparse *sparse)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(sparse != NULL);

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("count", token->data) == 0) {
                if (parse_int(file, token, &sparse->count) != 0)
                    return -1;

            } else if (strcmp("indices", token->data) == 0) {
                if (parse_sindices(file, token, &sparse->indices) != 0)
                    return -1;

            } else if (strcmp("values", token->data) == 0) {
                if (parse_svalues(file, token, &sparse->values) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.accessors' property. */
static int parse_accessors(FILE *file, T_token *token,
                           size_t index, void *accessors_p)
{
    T_accessors *accessors = accessors_p;

    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(accessors != NULL);
    assert(index < accessors->n);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '[' || token->data[0] == ',');

    accessors->v[index].buffer_view = YF_INT_MIN;
    accessors->v[index].sparse.indices.buffer_view = YF_INT_MIN;
    accessors->v[index].sparse.values.buffer_view = YF_INT_MIN;

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("bufferView", token->data) == 0) {
                if (parse_int(file, token,
                              &accessors->v[index].buffer_view) != 0)
                    return -1;

            } else if (strcmp("byteOffset", token->data) == 0) {
                if (parse_int(file, token, &accessors->v[index].byte_off) != 0)
                    return -1;

            } else if (strcmp("count", token->data) == 0) {
                if (parse_int(file, token, &accessors->v[index].count) != 0)
                    return -1;

            } else if (strcmp("componentType", token->data) == 0) {
                if (parse_int(file, token,
                              &accessors->v[index].comp_type) != 0)
                    return -1;

            } else if (strcmp("type", token->data) == 0) {
                next_token(file, token); /* ':' */
                next_token(file, token);
                if (strcmp("SCALAR", token->data) == 0) {
                    accessors->v[index].type = YF_GLTF_TYPE_SCALAR;
                } else if (strcmp("VEC2", token->data) == 0) {
                    accessors->v[index].type = YF_GLTF_TYPE_VEC2;
                } else if (strcmp("VEC3", token->data) == 0) {
                    accessors->v[index].type = YF_GLTF_TYPE_VEC3;
                } else if (strcmp("VEC4", token->data) == 0) {
                    accessors->v[index].type = YF_GLTF_TYPE_VEC4;
                } else if (strcmp("MAT2", token->data) == 0) {
                    accessors->v[index].type = YF_GLTF_TYPE_MAT2;
                } else if (strcmp("MAT3", token->data) == 0) {
                    accessors->v[index].type = YF_GLTF_TYPE_MAT3;
                } else if (strcmp("MAT4", token->data) == 0) {
                    accessors->v[index].type = YF_GLTF_TYPE_MAT4;
                } else {
                    yf_seterr(YF_ERR_INVFILE, __func__);
                    return -1;
                }

            } else if (strcmp("min", token->data) == 0) {
                next_token(file, token); /* ':' */
                next_token(file, token); /* '[' */
                for (size_t i = 0; i < 16; i++) {
                    if (parse_num(file, token,
                                  accessors->v[index].min.m4+i) != 0)
                        return -1;
                    next_token(file, token); /* ',' or ']' */
                    if (token->token == YF_TOKEN_OP && token->data[0] == ']')
                        break;
                }

            } else if (strcmp("max", token->data) == 0) {
                next_token(file, token); /* ':' */
                next_token(file, token); /* '[' */
                for (size_t i = 0; i < 16; i++) {
                    if (parse_num(file, token,
                                  accessors->v[index].max.m4+i) != 0)
                        return -1;
                    next_token(file, token); /* ',' or ']' */
                    if (token->token == YF_TOKEN_OP && token->data[0] == ']')
                        break;
                }

            } else if (strcmp("normalized", token->data) == 0) {
                if (parse_bool(file, token,
                               &accessors->v[index].normalized) != 0)
                    return -1;

            } else if (strcmp("sparse", token->data) == 0) {
                if (parse_sparse(file, token,
                                 &accessors->v[index].sparse) != 0)
                    return -1;

            } else if (strcmp("name", token->data) == 0) {
                if (parse_str(file, token, &accessors->v[index].name) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.bufferViews' property. */
static int parse_bufferviews(FILE *file, T_token *token,
                             size_t index, void *bufferviews_p)
{
    T_bufferviews *bufferviews = bufferviews_p;

    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(bufferviews != NULL);
    assert(index < bufferviews->n);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '[' || token->data[0] == ',');

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("buffer", token->data) == 0) {
                if (parse_int(file, token, &bufferviews->v[index].buffer) != 0)
                    return -1;

            } else if (strcmp("byteOffset", token->data) == 0) {
                if (parse_int(file, token,
                              &bufferviews->v[index].byte_off) != 0)
                    return -1;

            } else if (strcmp("byteLength", token->data) == 0) {
                if (parse_int(file, token,
                              &bufferviews->v[index].byte_len) != 0)
                    return -1;

            } else if (strcmp("byteStride", token->data) == 0) {
                if (parse_int(file, token,
                              &bufferviews->v[index].byte_strd) != 0)
                    return -1;

            } else if (strcmp("target", token->data) == 0) {
                if (parse_int(file, token, &bufferviews->v[index].target) != 0)
                    return -1;

            } else if (strcmp("name", token->data) == 0) {
                if (parse_str(file, token, &bufferviews->v[index].name) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.buffers' property. */
static int parse_buffers(FILE *file, T_token *token,
                         size_t index, void *buffers_p)
{
    T_buffers *buffers = buffers_p;

    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(buffers != NULL);
    assert(index < buffers->n);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '[' || token->data[0] == ',');

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("byteLength", token->data) == 0) {
                if (parse_int(file, token, &buffers->v[index].byte_len) != 0)
                    return -1;

            } else if (strcmp("uri", token->data) == 0) {
                if (parse_str(file, token, &buffers->v[index].uri) != 0)
                    return -1;

            } else if (strcmp("name", token->data) == 0) {
                if (parse_str(file, token, &buffers->v[index].name) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.textures' property. */
static int parse_textures(FILE *file, T_token *token,
                          size_t index, void *textures_p)
{
    T_textures *textures = textures_p;

    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(textures != NULL);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '[' || token->data[0] == ',');

    textures->v[index].sampler = YF_INT_MIN;
    textures->v[index].source = YF_INT_MIN;

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("sampler", token->data) == 0) {
                if (parse_int(file, token, &textures->v[index].sampler) != 0)
                    return -1;

            } else if (strcmp("source", token->data) == 0) {
                if (parse_int(file, token, &textures->v[index].source) != 0)
                    return -1;

            } else if (strcmp("name", token->data) == 0) {
                if (parse_str(file, token, &textures->v[index].name) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.images' property. */
static int parse_images(FILE *file, T_token *token,
                        size_t index, void *images_p)
{
    T_images *images = images_p;

    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(images != NULL);
    assert(index < images->n);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '[' || token->data[0] == ',');

    images->v[index].buffer_view = YF_INT_MIN;

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("uri", token->data) == 0) {
                if (parse_str(file, token, &images->v[index].uri) != 0)
                    return -1;

            } else if (strcmp("mimeType", token->data) == 0) {
                if (parse_str(file, token, &images->v[index].mime_type) != 0)
                    return -1;

            } else if (strcmp("bufferView", token->data) == 0) {
                if (parse_int(file, token, &images->v[index].buffer_view) != 0)
                    return -1;

            } else if (strcmp("name", token->data) == 0) {
                if (parse_str(file, token, &images->v[index].name) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the 'glTF.samplers' property. */
static int parse_samplers(FILE *file, T_token *token,
                          size_t index, void *samplers_p)
{
    T_samplers *samplers = samplers_p;

    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(samplers != NULL);
    assert(index < samplers->n);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '[' || token->data[0] == ',');

    samplers->v[index].wrap_s = YF_GLTF_WRAP_REPEAT;
    samplers->v[index].wrap_t = YF_GLTF_WRAP_REPEAT;

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("minFilter", token->data) == 0) {
                if (parse_int(file, token, &samplers->v[index].min_filter) != 0)
                    return -1;

            } else if (strcmp("magFilter", token->data) == 0) {
                if (parse_int(file, token, &samplers->v[index].mag_filter) != 0)
                    return -1;

            } else if (strcmp("wrapS", token->data) == 0) {
                if (parse_int(file, token, &samplers->v[index].wrap_s) != 0)
                    return -1;

            } else if (strcmp("wrapT", token->data) == 0) {
                if (parse_int(file, token, &samplers->v[index].wrap_t) != 0)
                    return -1;

            } else if (strcmp("name", token->data) == 0) {
                if (parse_str(file, token, &samplers->v[index].name) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/* Parses the root glTF object. */
static int parse_gltf(FILE *file, T_token *token, T_gltf *gltf)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(gltf != NULL);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '{');

    gltf->scene = YF_INT_MIN;

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("asset", token->data) == 0) {
                if (parse_asset(file, token, &gltf->asset) != 0)
                    return -1;

            } else if (strcmp("scene", token->data) == 0) {
                if (parse_scene(file, token, &gltf->scene) != 0)
                    return -1;

            } else if (strcmp("scenes", token->data) == 0) {
                if (parse_array(file, token, (void **)&gltf->scenes.v,
                                &gltf->scenes.n, sizeof *gltf->scenes.v,
                                parse_scenes, &gltf->scenes) != 0)
                    return -1;

            } else if (strcmp("nodes", token->data) == 0) {
                if (parse_array(file, token, (void **)&gltf->nodes.v,
                                &gltf->nodes.n, sizeof *gltf->nodes.v,
                                parse_nodes, &gltf->nodes) != 0)
                    return -1;

            } else if (strcmp("cameras", token->data) == 0) {
                if (parse_array(file, token, (void **)&gltf->cameras.v,
                                &gltf->cameras.n, sizeof *gltf->cameras.v,
                                parse_cameras, &gltf->cameras) != 0)
                    return -1;

            } else if (strcmp("meshes", token->data) == 0) {
                if (parse_array(file, token, (void **)&gltf->meshes.v,
                                &gltf->meshes.n, sizeof *gltf->meshes.v,
                                parse_meshes, &gltf->meshes) != 0)
                    return -1;

            } else if (strcmp("skins", token->data) == 0) {
                if (parse_array(file, token, (void **)&gltf->skins.v,
                                &gltf->skins.n, sizeof *gltf->skins.v,
                                parse_skins, &gltf->skins) != 0)
                    return -1;

            } else if (strcmp("materials", token->data) == 0) {
                if (parse_array(file, token, (void **)&gltf->materials.v,
                                &gltf->materials.n, sizeof *gltf->materials.v,
                                parse_materials, &gltf->materials) != 0)
                    return -1;

            } else if (strcmp("animations", token->data) == 0) {
                if (parse_array(file, token, (void **)&gltf->animations.v,
                                &gltf->animations.n, sizeof *gltf->animations.v,
                                parse_animations, &gltf->animations) != 0)
                    return -1;

            } else if (strcmp("accessors", token->data) == 0) {
                if (parse_array(file, token, (void **)&gltf->accessors.v,
                                &gltf->accessors.n, sizeof *gltf->accessors.v,
                                parse_accessors, &gltf->accessors) != 0)
                    return -1;

            } else if (strcmp("bufferViews", token->data) == 0) {
                if (parse_array(file, token, (void **)&gltf->bufferviews.v,
                                &gltf->bufferviews.n,
                                sizeof *gltf->bufferviews.v,
                                parse_bufferviews, &gltf->bufferviews) != 0)
                    return -1;

            } else if (strcmp("buffers", token->data) == 0) {
                if (parse_array(file, token, (void **)&gltf->buffers.v,
                                &gltf->buffers.n, sizeof *gltf->buffers.v,
                                parse_buffers, &gltf->buffers) != 0)
                    return -1;

            } else if (strcmp("textures", token->data) == 0) {
                if (parse_array(file, token, (void **)&gltf->textures.v,
                                &gltf->textures.n, sizeof *gltf->textures.v,
                                parse_textures, &gltf->textures) != 0)
                    return -1;

            } else if (strcmp("images", token->data) == 0) {
                if (parse_array(file, token, (void **)&gltf->images.v,
                                &gltf->images.n, sizeof *gltf->images.v,
                                parse_images, &gltf->images) != 0)
                    return -1;

            } else if (strcmp("samplers", token->data) == 0) {
                if (parse_array(file, token, (void **)&gltf->samplers.v,
                                &gltf->samplers.n, sizeof *gltf->samplers.v,
                                parse_samplers, &gltf->samplers) != 0)
                    return -1;

            } else {
                if (consume_prop(file, token) != 0)
                    return -1;
            }
            break;

        case YF_TOKEN_OP:
            if (token->data[0] == '}')
                return 0;
            break;

        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    return 0;
}

/*
 * Data
 */

/* Type defining data for binary buffer access. */
typedef struct {
    char *path;
    /* offset into the embedded binary buffer (0), when applicable */
    T_int off_e;
    /* open file(s), mapping to 'gltf.buffers' */
    union {
        FILE *file;
        FILE **files;
    };
} T_fdata;

/* Type defining lists of contents mapping to a gltf object. */
typedef struct {
    YF_scene *scns;
    YF_node *nodes;
    YF_mesh *meshes;
    YF_texture *texs;
    YF_skin *skins;
    YF_material *matls;

    /* flag indicating that the contents must be destroyed
       when not set, only the allocated lists are freed */
    int deinit;
} T_cont;

#define YF_PATHOF(pathname, path) do { \
    const char *last = strrchr(pathname, '/'); \
    if (last != NULL) { \
        const size_t len = 1 + last - (pathname); \
        path = malloc(1+len); \
        if ((path) != NULL) { \
            memcpy(path, pathname, len); \
            (path)[len] = '\0'; \
        } \
    } else { \
        path = ""; \
    } } while (0)

#define YF_PATHCAT(path, name, pathname) do { \
    const size_t path_len = strlen(path); \
    const size_t name_len = strlen(name); \
    pathname = malloc(1+path_len+name_len); \
    if ((pathname) != NULL) { \
        memcpy(pathname, path, path_len); \
        memcpy((pathname)+path_len, name, name_len); \
        (pathname)[path_len+name_len] = '\0'; \
    } } while (0)

#ifdef YF_DEVEL
static void print_gltf(const T_gltf *gltf);
#endif

/* Deinitializes glTF contents. */
static void deinit_gltf(T_gltf *gltf, T_fdata *fdata, T_cont *cont)
{
    if (gltf == NULL) {
        assert(fdata == NULL);
        assert(cont == NULL);
        return;
    }

    if (fdata != NULL) {
        switch (gltf->buffers.n) {
        case 0:
            break;
        case 1:
            if (fdata->file != NULL && gltf->buffers.v[0].uri != NULL)
                fclose(fdata->file);
            break;
        default:
            if (fdata->files == NULL)
                break;
            if (gltf->buffers.v[0].uri != NULL)
                fclose(fdata->files[0]);
            for (size_t i = 1; i < gltf->buffers.n; i++)
                fclose(fdata->files[i]);
            free(fdata->files);
        }
        free(fdata->path);
    }

    if (cont != NULL) {
        if (cont->scns != NULL) {
            if (cont->deinit) {
                for (size_t i = 0; i < gltf->scenes.n; i++)
                    yf_scene_deinit(cont->scns[i]);
            }
            free(cont->scns);
        }
        if (cont->nodes != NULL) {
            if (cont->deinit) {
                for (size_t i = 0; i < gltf->nodes.n; i++)
                    yf_node_deinit(cont->nodes[i]);
            }
            free(cont->nodes);
        }
        if (cont->meshes != NULL) {
            if (cont->deinit) {
                for (size_t i = 0; i < gltf->meshes.n; i++)
                    yf_mesh_deinit(cont->meshes[i]);
            }
            free(cont->meshes);
        }
        if (cont->texs != NULL) {
            if (cont->deinit) {
                for (size_t i = 0; i < gltf->textures.n; i++)
                    yf_texture_deinit(cont->texs[i]);
            }
            free(cont->texs);
        }
        if (cont->skins != NULL) {
            if (cont->deinit) {
                for (size_t i = 0; i < gltf->skins.n; i++)
                    yf_skin_deinit(cont->skins[i]);
            }
            free(cont->skins);
        }
        if (cont->matls != NULL) {
            if (cont->deinit) {
                for (size_t i = 0; i < gltf->materials.n; i++)
                    yf_material_deinit(cont->matls[i]);
            }
            free(cont->matls);
        }
    }

    free(gltf->asset.copyright);
    free(gltf->asset.generator);
    free(gltf->asset.version);
    free(gltf->asset.min_version);

    for (size_t i = 0; i < gltf->scenes.n; i++) {
        free(gltf->scenes.v[i].nodes);
        free(gltf->scenes.v[i].name);
    }
    free(gltf->scenes.v);

    for (size_t i = 0; i < gltf->nodes.n; i++) {
        free(gltf->nodes.v[i].children);
        free(gltf->nodes.v[i].weights);
        free(gltf->nodes.v[i].name);
    }
    free(gltf->nodes.v);

    for (size_t i = 0; i < gltf->cameras.n; i++)
        free(gltf->cameras.v[i].name);
    free(gltf->cameras.v);

    for (size_t i = 0; i < gltf->meshes.n; i++) {
        for (size_t j = 0; j < gltf->meshes.v[i].primitives.n; j++)
            free(gltf->meshes.v[i].primitives.v[j].targets.v);
        free(gltf->meshes.v[i].primitives.v);
        free(gltf->meshes.v[i].weights);
        free(gltf->meshes.v[i].name);
    }
    free(gltf->meshes.v);

    for (size_t i = 0; i < gltf->skins.n; i++) {
        free(gltf->skins.v[i].joints);
        free(gltf->skins.v[i].name);
    }
    free(gltf->skins.v);

    for (size_t i = 0; i < gltf->materials.n; i++)
        free(gltf->materials.v[i].name);
    free(gltf->materials.v);

    for (size_t i = 0; i < gltf->animations.n; i++) {
        free(gltf->animations.v[i].channels.v);
        free(gltf->animations.v[i].samplers.v);
        free(gltf->animations.v[i].name);
    }
    free(gltf->animations.v);

    for (size_t i = 0; i < gltf->accessors.n; i++)
        free(gltf->accessors.v[i].name);
    free(gltf->accessors.v);

    for (size_t i = 0; i < gltf->bufferviews.n; i++)
        free(gltf->bufferviews.v[i].name);
    free(gltf->bufferviews.v);

    for (size_t i = 0; i < gltf->buffers.n; i++) {
        free(gltf->buffers.v[i].uri);
        free(gltf->buffers.v[i].name);
    }
    free(gltf->buffers.v);

    for (size_t i = 0; i < gltf->textures.n; i++)
        free(gltf->textures.v[i].name);
    free(gltf->textures.v);

    for (size_t i = 0; i < gltf->images.n; i++) {
        free(gltf->images.v[i].uri);
        free(gltf->images.v[i].mime_type);
        free(gltf->images.v[i].name);
    }
    free(gltf->images.v);

    for (size_t i = 0; i < gltf->samplers.n; i++)
        free(gltf->samplers.v[i].name);
    free(gltf->samplers.v);
}

/* Initializes glTF contents. */
static int init_gltf(FILE *file, T_gltf *gltf, T_fdata *fdata, T_cont *cont)
{
    assert(file != NULL);
    assert(gltf != NULL);
    assert(fdata != NULL);
    assert(cont != NULL);

    uint32_t magic;
    if (fread(&magic, sizeof magic, 1, file) != 1) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }

    if (le32toh(magic) == 0x46546c67) {
        /* .glb */
        uint32_t version;
        if (fread(&version, sizeof version, 1, file) != 1) {
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
        if (le32toh(version) != 2) {
            yf_seterr(YF_ERR_UNSUP, __func__);
            return -1;
        }
        uint32_t jlen;
        long pos;
        if (fseek(file, sizeof(uint32_t), SEEK_CUR) != 0 ||
            fread(&jlen, sizeof jlen, 1, file) != 1 ||
            fseek(file, sizeof(uint32_t), SEEK_CUR) != 0 ||
            (pos = ftell(file)) == -1) {
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
        fdata->off_e = pos + le32toh(jlen) + (sizeof(uint32_t) << 1);
    } else {
        /* .gltf */
        if (fseek(file, -(long)sizeof magic, SEEK_CUR) != 0) {
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
    }

    T_token token = {0};
    next_token(file, &token);
    if (token.token != YF_TOKEN_OP && token.data[0] != '{') {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }

    if (parse_gltf(file, &token, gltf) != 0) {
        deinit_gltf(gltf, NULL, NULL);
        return -1;
    }

    switch (gltf->buffers.n) {
    case 0:
        break;
    case 1:
        if (gltf->buffers.v[0].uri == NULL)
            fdata->file = file;
        break;
    default:
        fdata->files = calloc(gltf->buffers.n, sizeof *fdata->files);
        if (fdata->files == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            deinit_gltf(gltf, NULL, NULL);
            return -1;
        }
        if (gltf->buffers.v[0].uri == NULL)
            fdata->files[0] = file;
    }

    *cont = (T_cont){0};
    if (gltf->scenes.n > 0) {
        cont->scns = calloc(gltf->scenes.n, sizeof *cont->scns);
        if (cont->scns == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            deinit_gltf(gltf, fdata, NULL);
            return -1;
        }
    }
    if (gltf->nodes.n > 0) {
        cont->nodes = calloc(gltf->nodes.n, sizeof *cont->nodes);
        if (cont->nodes == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            deinit_gltf(gltf, fdata, cont);
            return -1;
        }
    }
    if (gltf->meshes.n > 0) {
        cont->meshes = calloc(gltf->meshes.n, sizeof *cont->meshes);
        if (cont->meshes == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            deinit_gltf(gltf, fdata, cont);
            return -1;
        }
    }
    if (gltf->textures.n > 0) {
        cont->texs = calloc(gltf->textures.n, sizeof *cont->texs);
        if (cont->texs == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            deinit_gltf(gltf, fdata, cont);
            return -1;
        }
    }
    if (gltf->skins.n > 0) {
        cont->skins = calloc(gltf->skins.n, sizeof *cont->skins);
        if (cont->skins == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            deinit_gltf(gltf, fdata, cont);
            return -1;
        }
    }
    if (gltf->materials.n > 0) {
        cont->matls = calloc(gltf->materials.n, sizeof *cont->matls);
        if (cont->matls == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            deinit_gltf(gltf, fdata, cont);
            return -1;
        }
    }

#ifdef YF_DEVEL
    print_gltf(gltf);
#endif

    return 0;
}

/* Seeks into data buffer as specified by an accessor or buffer view. */
static FILE *seek_data(const T_gltf *gltf, T_fdata *fdata,
                       T_int accessor, T_int buffer_view)
{
    assert(gltf != NULL);
    assert(fdata != NULL);
    assert(accessor != YF_INT_MIN || buffer_view != YF_INT_MIN);

    T_int buf, off;
    if (accessor != YF_INT_MIN) {
        T_int view = gltf->accessors.v[accessor].buffer_view;
        buf = gltf->bufferviews.v[view].buffer;
        off = gltf->accessors.v[accessor].byte_off +
            gltf->bufferviews.v[view].byte_off;
    } else {
        buf = gltf->bufferviews.v[buffer_view].buffer;
        off = gltf->bufferviews.v[buffer_view].byte_off;
    }

    FILE **file_p = NULL;
    FILE *file = NULL;

    switch (gltf->buffers.n) {
    case 0:
        yf_seterr(YF_ERR_INVFILE, __func__);
        return NULL;
    case 1:
        if (fdata->file == NULL) {
            file_p = &fdata->file;
        } else {
            file = fdata->file;
            if (gltf->buffers.v[buf].uri == NULL)
                off += fdata->off_e;
        }
        break;
    default:
        if (fdata->files[buf] == NULL) {
            file_p = fdata->files+buf;
        } else {
            file = fdata->files[buf];
            if (buf == 0 && gltf->buffers.v[buf].uri == NULL)
                off += fdata->off_e;
        }
    }

    if (file_p != NULL) {
        /* need to open file */
        assert(fdata->path != NULL);
        assert(gltf->buffers.v[buf].uri != NULL);

        char *pathname = NULL;
        YF_PATHCAT(fdata->path, gltf->buffers.v[buf].uri, pathname);
        if (pathname == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return NULL;
        }

        *file_p = fopen(pathname, "r");
        free(pathname);
        if (*file_p == NULL) {
            yf_seterr(YF_ERR_NOFILE, __func__);
            return NULL;
        }
        file = *file_p;
    }

    if (fseek(file, off, SEEK_SET) != 0) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return NULL;
    }

    return file;
}

/* Loads a single mesh from glTF contents. */
static int load_mesh(const T_gltf *gltf, T_fdata *fdata, T_cont *cont,
                     T_int mesh)
{
    assert(gltf != NULL);
    assert(fdata != NULL);
    assert(cont != NULL);
    assert(mesh >= 0);

    if (gltf->meshes.n <= (size_t)mesh) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    assert(cont->meshes != NULL);
    assert(cont->meshes[mesh] == NULL);

    const T_primitives *prim = &gltf->meshes.v[mesh].primitives;
    switch (prim->n) {
    case 0:
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    case 1:
        break;
    default:
        /* TODO: Support for multiple primitives. */
        yf_seterr(YF_ERR_UNSUP, __func__);
        return -1;
    }

    /* vertex data */
    struct { T_int acc, view, buf; } attrs[YF_GLTF_ATTR_N];
    for (size_t i = 0; i < YF_GLTF_ATTR_N; i++) {
        attrs[i].acc = prim->v[0].attributes[i];
        if (attrs[i].acc != YF_INT_MIN) {
            attrs[i].view = gltf->accessors.v[attrs[i].acc].buffer_view;
            attrs[i].buf = gltf->bufferviews.v[attrs[i].view].buffer;
        }
    }
    if (attrs[YF_GLTF_ATTR_POS].acc == YF_INT_MIN) {
        yf_seterr(YF_ERR_UNSUP, __func__);
        return -1;
    }

    const size_t v_n = gltf->accessors.v[attrs[YF_GLTF_ATTR_POS].acc].count;
    YF_vmdl *verts = malloc(v_n * sizeof *verts);
    if (verts == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }

    for (size_t i = 0; i < YF_GLTF_ATTR_N; i++) {
        if (attrs[i].acc == YF_INT_MIN)
            continue;

        const T_int comp_type = gltf->accessors.v[attrs[i].acc].comp_type;
        size_t comp_sz;
        switch (comp_type) {
        case YF_GLTF_COMP_BYTE:
        case YF_GLTF_COMP_UBYTE:
            comp_sz = 1;
            break;
        case YF_GLTF_COMP_SHORT:
        case YF_GLTF_COMP_USHORT:
            comp_sz = 2;
            break;
        case YF_GLTF_COMP_UINT:
        case YF_GLTF_COMP_FLOAT:
            comp_sz = 4;
            break;
        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            free(verts);
            return -1;
        }

        const int type = gltf->accessors.v[attrs[i].acc].type;
        size_t comp_n;
        switch (type) {
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
        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            free(verts);
            return -1;
        }

        size_t v_off;
        switch (i) {
        case YF_GLTF_ATTR_POS:
            v_off = offsetof(YF_vmdl, pos);
            break;
        case YF_GLTF_ATTR_NORM:
            v_off = offsetof(YF_vmdl, norm);
            break;
        case YF_GLTF_ATTR_TC0:
            /* TODO: Support for UBYTE and USHORT component types. */
            if (comp_type != YF_GLTF_COMP_FLOAT) {
                yf_seterr(YF_ERR_UNSUP, __func__);
                free(verts);
                return -1;
            }
            v_off = offsetof(YF_vmdl, tc);
            break;
        default:
#ifdef YF_DEVEL
            printf("\n[YF] WARNING (%s):", __func__);
            printf("\nglTF mesh attribute '%lu' ignored\n", i);
#endif
            continue;
        }

        FILE *file = seek_data(gltf, fdata, attrs[i].acc, YF_INT_MIN);
        if (file == NULL) {
            free(verts);
            return -1;
        }

        const T_int byte_strd = gltf->bufferviews.v[attrs[i].view].byte_strd;
        char *dt = (void *)verts;
        if (byte_strd == 0) {
            for (size_t j = 0; j < v_n; j++) {
                if (fread(dt+v_off, comp_sz, comp_n, file) < comp_n) {
                    yf_seterr(YF_ERR_INVFILE, __func__);
                    free(verts);
                    return -1;
                }
                dt += sizeof *verts;
            }
        } else {
            const size_t strd = byte_strd - comp_sz * comp_n;
            size_t j = 0;
            while (1) {
                if (fread(dt+v_off, comp_sz, comp_n, file) < comp_n) {
                    yf_seterr(YF_ERR_INVFILE, __func__);
                    free(verts);
                    return -1;
                }
                if (++j == v_n)
                    break;
                if (fseek(file, strd, SEEK_CUR) != 0) {
                    yf_seterr(YF_ERR_INVFILE, __func__);
                    free(verts);
                    return -1;
                }
                dt += sizeof *verts;
            }
        }
    }

    /* index data */
    const T_int i_acc = prim->v[0].indices;
    size_t i_n;
    void *inds;
    int itype;

    if (i_acc != YF_INT_MIN) {
        size_t i_sz;
        switch (gltf->accessors.v[i_acc].comp_type) {
        case YF_GLTF_COMP_USHORT:
            itype = YF_ITYPE_USHORT;
            i_sz = 2;
            break;
        case YF_GLTF_COMP_UINT:
            itype = YF_ITYPE_UINT;
            i_sz = 4;
            break;
        default:
            yf_seterr(YF_ERR_UNSUP, __func__);
            free(verts);
            return -1;
        }

        if (gltf->accessors.v[i_acc].type != YF_GLTF_TYPE_SCALAR) {
            yf_seterr(YF_ERR_UNSUP, __func__);
            free(verts);
            return -1;
        }

        i_n = gltf->accessors.v[i_acc].count;
        inds = malloc(i_n * i_sz);
        if (inds == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            free(verts);
            return -1;
        }

        FILE *file = seek_data(gltf, fdata, i_acc, YF_INT_MIN);
        if (file == NULL) {
            free(verts);
            free(inds);
            return -1;
        }

        const T_int i_view = gltf->accessors.v[i_acc].buffer_view;
        const T_int byte_strd = gltf->bufferviews.v[i_view].byte_strd;
        if (byte_strd == 0) {
            if (fread(inds, i_sz, i_n, file) < i_n) {
                yf_seterr(YF_ERR_INVFILE, __func__);
                free(verts);
                free(inds);
                return -1;
            }
        } else {
            const size_t strd = byte_strd - i_sz;
            char *dt = inds;
            size_t j = 0;
            while (1) {
                if (fread(dt+i_sz*j, i_sz, 1, file) < 1) {
                    yf_seterr(YF_ERR_INVFILE, __func__);
                    free(verts);
                    free(inds);
                    return -1;
                }
                if (++j == i_n)
                    break;
                if (fseek(file, strd, SEEK_CUR) != 0) {
                    yf_seterr(YF_ERR_INVFILE, __func__);
                    free(verts);
                    free(inds);
                    return -1;
                }
            }
        }
    } else {
        itype = i_n = 0;
        inds = NULL;
    }

    /* mesh */
    YF_meshdt data = {
        .v = {
            .vtype = YF_VTYPE_MDL,
            .data = verts,
            .n = v_n
        },
        .i = {
            .itype = itype,
            .data = inds,
            .n = i_n
        }
    };
    cont->meshes[mesh] = yf_mesh_initdt(&data);
    free(data.v.data);
    free(data.i.data);
    return cont->meshes[mesh] == NULL ? -1 : 0;
}

/* Loads a single texture from glTF contents. */
static int load_texture(const T_gltf *gltf, T_fdata *fdata, T_cont *cont,
                        T_int texture)
{
    assert(gltf != NULL);
    assert(fdata != NULL);
    assert(cont != NULL);
    assert(texture >= 0);

    if (gltf->textures.n <= (size_t)texture) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    assert(cont->texs != NULL);
    assert(cont->texs[texture] == NULL);

    const T_int image = gltf->textures.v[texture].source;
    if (gltf->images.v[image].mime_type != NULL &&
        strcmp(gltf->images.v[image].mime_type, "image/png") != 0) {
        yf_seterr(YF_ERR_UNSUP, __func__);
        return -1;
    }

    const T_int view = gltf->images.v[image].buffer_view;
    YF_texdt data;
    if (view != YF_INT_MIN) {
        /* image provided through binary buffer */
        FILE *file = seek_data(gltf, fdata, YF_INT_MIN, view);
        if (file == NULL || yf_loadpng2(file, &data) != 0)
            return -1;
    } else {
        /* image provided through external file */
        char *pathname = NULL;
        YF_PATHCAT(fdata->path, gltf->images.v[image].uri, pathname);
        if (pathname == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
        }
        int r = yf_loadpng(pathname, &data);
        free(pathname);
        if (r != 0)
            return -1;
    }

    cont->texs[texture] = yf_texture_initdt(&data);
    free(data.data);
    return cont->texs[texture] == NULL ? -1 : 0;
}

/* Loads a single skin from glTF contents. */
static int load_skin(const T_gltf *gltf, T_fdata *fdata, T_cont *cont,
                     T_int skin)
{
    assert(gltf != NULL);
    assert(fdata != NULL);
    assert(cont != NULL);
    assert(skin >= 0);

    if (gltf->skins.n <= (size_t)skin) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    assert(cont->skins != NULL);
    assert(cont->skins[skin] == NULL);

    const size_t jnt_n = gltf->skins.v[skin].joint_n;
    assert(jnt_n > 0);

    T_int *jnt_hier = calloc(gltf->nodes.n, sizeof *jnt_hier);
    YF_joint *jnts = malloc(jnt_n * sizeof *jnts);
    if (jnt_hier == NULL || jnts == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        free(jnt_hier);
        free(jnts);
        return -1;
    }

    for (size_t i = 0; i < jnt_n; i++) {
        const T_int node = gltf->skins.v[skin].joints[i];

        /* joint transform */
        const unsigned mask = gltf->nodes.v[node].xform_mask;
        if (mask != YF_GLTF_XFORM_NONE) {
            if (mask & YF_GLTF_XFORM_M) {
                yf_mat4_copy(jnts[i].xform, gltf->nodes.v[node].matrix);
            } else {
                if (mask & YF_GLTF_XFORM_T) {
                    const T_num *t = gltf->nodes.v[node].trs.t;
                    yf_mat4_xlate(jnts[i].xform, t[0], t[1], t[2]);
                } else {
                    yf_mat4_iden(jnts[i].xform);
                }
                if (mask & YF_GLTF_XFORM_R) {
                    const T_num *r = gltf->nodes.v[node].trs.r;
                    YF_mat4 mr, rot;
                    yf_mat4_rotq(mr, r);
                    yf_mat4_mul(rot, jnts[i].xform, mr);
                    yf_mat4_copy(jnts[i].xform, rot);
                }
                if (mask & YF_GLTF_XFORM_S) {
                    const T_num *s = gltf->nodes.v[node].trs.s;
                    YF_mat4 ms, scl;
                    yf_mat4_scale(ms, s[0], s[1], s[2]);
                    yf_mat4_mul(scl, jnts[i].xform, ms);
                    yf_mat4_copy(jnts[i].xform, scl);
                }
            }
        } else {
            yf_mat4_iden(jnts[i].xform);
        }

        /* joint name */
        if (gltf->nodes.v[node].name != NULL) {
            const size_t len = sizeof jnts[0].name - 1;
            strncpy(jnts[i].name, gltf->nodes.v[node].name, len);
            jnts[i].name[len] = '\0';
        } else {
            jnts[i].name[0] = '\0';
        }

        /* joint hierarchy */
        for (size_t j = 0; j < gltf->nodes.v[node].child_n; j++)
            jnt_hier[gltf->nodes.v[node].children[j]] = i + 1;
    }

    for (size_t i = 0; i < jnt_n; i++) {
        const T_int node = gltf->skins.v[skin].joints[i];
        /* negative 'pnt_i' means no parent */
        jnts[i].pnt_i = jnt_hier[node] - 1;
    }

    /* ibm data */
    const T_int acc = gltf->skins.v[skin].inv_bind_matrices;
    if (acc != YF_INT_MIN) {
        const T_int view = gltf->accessors.v[acc].buffer_view;
        const T_int buf = gltf->bufferviews.v[view].buffer;
        const size_t off = gltf->accessors.v[acc].byte_off +
            gltf->bufferviews.v[view].byte_off;

        assert(gltf->accessors.v[acc].count == (T_int)jnt_n);
        assert(gltf->accessors.v[acc].comp_type == YF_GLTF_COMP_FLOAT);
        assert(gltf->accessors.v[acc].type == YF_GLTF_TYPE_MAT4);
        assert(gltf->buffers.v[buf].byte_len - off >= sizeof(YF_mat4));

        FILE *file = seek_data(gltf, fdata, acc, YF_INT_MIN);
        if (file == NULL) {
            free(jnt_hier);
            free(jnts);
            return -1;
        }
        for (size_t i = 0; i < jnt_n; i++) {
            if (fread(jnts[i].ibm, sizeof jnts[i].ibm, 1, file) != 1) {
                yf_seterr(YF_ERR_INVFILE, __func__);
                free(jnt_hier);
                free(jnts);
                fclose(file);
                return -1;
            }
        }

    } else {
        for (size_t i = 0; i < jnt_n; i++)
            yf_mat4_iden(jnts[i].ibm);
    }

    /* skin */
    cont->skins[skin] = yf_skin_init(jnts, jnt_n);
    free(jnt_hier);
    free(jnts);
    return cont->skins[skin] == NULL ? -1 : 0;
}

/* Loads a single material from glTF contents. */
static int load_material(const T_gltf *gltf, T_fdata *fdata, T_cont *cont,
                         T_int material)
{
    assert(gltf != NULL);
    assert(fdata != NULL);
    assert(cont != NULL);
    assert(material >= 0);

    if (gltf->materials.n <= (size_t)material) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    assert(cont->matls != NULL);
    assert(cont->matls[material] == NULL);

    /* TODO: Specular-Glossiness and Unlit exts. */

    YF_matlprop prop;

    prop.pbr = YF_PBR_METALROUGH;
    memcpy(prop.pbrmr.color_fac,
           gltf->materials.v[material].pbrmr.base_clr_fac,
           sizeof prop.pbrmr.color_fac);
    prop.pbrmr.metallic_fac = gltf->materials.v[material].pbrmr.metallic_fac;
    prop.pbrmr.roughness_fac = gltf->materials.v[material].pbrmr.roughness_fac;

    prop.normal.scale = gltf->materials.v[material].normal_tex.scale;

    prop.occlusion.strength =
        gltf->materials.v[material].occlusion_tex.strength;

    memcpy(prop.emissive.factor, gltf->materials.v[material].emissive_fac,
           sizeof prop.emissive.factor);

    switch (gltf->materials.v[material].alpha_mode) {
    case YF_GLTF_ALPHA_OPAQUE:
        prop.alphamode = YF_ALPHAMODE_OPAQUE;
        break;
    case YF_GLTF_ALPHA_MASK:
        /* TODO */
        yf_seterr(YF_ERR_UNSUP, __func__);
        return -1;
    case YF_GLTF_ALPHA_BLEND:
        prop.alphamode = YF_ALPHAMODE_BLEND;
        break;
    default:
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }

    const T_int tex_i[] = {
        gltf->materials.v[material].pbrmr.base_clr_tex.index,
        gltf->materials.v[material].pbrmr.metal_rough_tex.index,
        gltf->materials.v[material].normal_tex.index,
        gltf->materials.v[material].occlusion_tex.index,
        gltf->materials.v[material].emissive_tex.index
    };

    YF_texture *const tex_p[] = {
        &prop.pbrmr.color_tex,
        &prop.pbrmr.metal_rough_tex,
        &prop.normal.tex,
        &prop.occlusion.tex,
        &prop.emissive.tex
    };

    for (size_t i = 0; i < (sizeof tex_i / sizeof *tex_i); i++) {
        if (tex_i[i] == YF_INT_MIN) {
            *tex_p[i] = NULL;
            continue;
        }
        if (cont->texs[tex_i[i]] == NULL) {
            if (load_texture(gltf, fdata, cont, tex_i[i]) != 0)
                return -1;
        }
        *tex_p[i] = cont->texs[tex_i[i]];
    }

    cont->matls[material] = yf_material_init(&prop);
    return cont->matls[material] == NULL ? -1 : 0;
}

/* Loads a single node from glTF contents. */
static int load_node(const T_gltf *gltf, T_fdata *fdata, T_cont *cont,
                     T_int node)
{
    assert(gltf != NULL);
    assert(fdata != NULL);
    assert(cont != NULL);
    assert(node >= 0);

    if (gltf->nodes.n <= (size_t)node) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    assert(cont->nodes != NULL);
    assert(cont->nodes[node] == NULL);

    /* node object */
    const T_int mesh = gltf->nodes.v[node].mesh;
    if (mesh != YF_INT_MIN) {
        /* model */
        YF_model mdl = yf_model_init();
        if (mdl == NULL)
            return -1;

        if (cont->meshes[mesh] == NULL &&
            load_mesh(gltf, fdata, cont, mesh) != 0) {
            yf_model_deinit(mdl);
            return -1;
        }
        yf_model_setmesh(mdl, cont->meshes[mesh]);

        /* TODO: Support for multiple primitives. */
        const T_int material = gltf->meshes.v[mesh].primitives.v[0].material;
        if (material != YF_INT_MIN) {
            if (cont->matls[material] == NULL &&
                load_material(gltf, fdata, cont, material) != 0) {
                yf_model_deinit(mdl);
                return -1;
            }
            yf_model_setmatl(mdl, cont->matls[material]);
        }

        /* TODO: Set skin and instantiate skeleton. */

        cont->nodes[node] = yf_model_getnode(mdl);

    } else {
        /* none */
        cont->nodes[node] = yf_node_init();
        if (cont->nodes[node] == NULL)
            return -1;
    }

    /* node transform */
    const unsigned mask = gltf->nodes.v[node].xform_mask;
    if (mask != YF_GLTF_XFORM_NONE) {
        if (mask & YF_GLTF_XFORM_M) {
            yf_mat4_copy(*yf_node_getxform(cont->nodes[node]),
                         gltf->nodes.v[node].matrix);
        } else {
            YF_mat4 *m = yf_node_getxform(cont->nodes[node]);
            if (mask & YF_GLTF_XFORM_T) {
                const T_num *t = gltf->nodes.v[node].trs.t;
                yf_mat4_xlate(*m, t[0], t[1], t[2]);
            }
            if (mask & YF_GLTF_XFORM_R) {
                const T_num *r = gltf->nodes.v[node].trs.r;
                YF_mat4 mr, rot;
                yf_mat4_rotq(rot, r);
                yf_mat4_mul(mr, *m, rot);
                yf_mat4_copy(*m, mr);
            }
            if (mask & YF_GLTF_XFORM_S) {
                const T_num *s = gltf->nodes.v[node].trs.s;
                YF_mat4 ms, scl;
                yf_mat4_scale(scl, s[0], s[1], s[2]);
                yf_mat4_mul(ms, *m, scl);
                yf_mat4_copy(*m, ms);
            }
        }
    }

    /* node name */
    yf_node_setname(cont->nodes[node], gltf->nodes.v[node].name);

    return 0;
}

/* Loads a node subgraph from glTF contents. */
static int load_subgraph(const T_gltf *gltf, T_fdata *fdata, T_cont *cont,
                         T_int node)
{
    assert(gltf != NULL);
    assert(fdata != NULL);
    assert(cont != NULL);
    assert(node >= 0);

    if (gltf->nodes.n <= (size_t)node) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    assert(cont->nodes != NULL);

    /* descendants will have been created by the time of insertion */
    for (size_t i = 0; i < gltf->nodes.v[node].child_n; i++) {
        if (load_subgraph(gltf, fdata, cont,
                          gltf->nodes.v[node].children[i]) != 0)
            return -1;
    }

    /* node may have been created already */
    if (cont->nodes[node] == NULL && load_node(gltf, fdata, cont, node) != 0)
        return -1;

    for (size_t i = 0; i < gltf->nodes.v[node].child_n; i++)
        yf_node_insert(cont->nodes[node],
                       cont->nodes[gltf->nodes.v[node].children[i]]);

    return 0;
}

/* Loads an entire scene from glTF contents. */
static int load_scene(const T_gltf *gltf, T_fdata *fdata, T_cont *cont,
                      T_int scene)
{
    assert(gltf != NULL);
    assert(fdata != NULL);
    assert(cont != NULL);
    assert(scene >= 0);

    if (gltf->scenes.n <= (size_t)scene) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    assert(cont->scns != NULL);
    assert(cont->scns[scene] == NULL);

    cont->scns[scene] = yf_scene_init();
    if (cont->scns[scene] == NULL)
        return -1;

    YF_node node = yf_scene_getnode(cont->scns[scene]);
    yf_node_setname(node, gltf->scenes.v[scene].name);
    for (size_t i = 0; i < gltf->scenes.v[scene].node_n; i++) {
        if (load_subgraph(gltf, fdata, cont,
                          gltf->scenes.v[scene].nodes[i]) != 0)
            return -1;
        yf_node_insert(node, cont->nodes[gltf->scenes.v[scene].nodes[i]]);
    }

    return 0;
}

/* Loads glTF contents. */
static int load_contents(const T_gltf *gltf, T_fdata *fdata, T_cont *cont)
{
    assert(gltf != NULL);
    assert(fdata != NULL);
    assert(cont != NULL);

    /* mesh creation */
    assert(gltf->meshes.n == 0 || cont->meshes != NULL);
    for (size_t i = 0; i < gltf->meshes.n; i++) {
        assert(cont->meshes[i] == NULL);
        if (load_mesh(gltf, fdata, cont, i) != 0)
            return -1;
    }

    /* texture creation */
    assert(gltf->textures.n == 0 || cont->texs != NULL);
    for (size_t i = 0; i < gltf->textures.n; i++) {
        assert(cont->texs[i] == NULL);
        if (load_texture(gltf, fdata, cont, i) != 0)
            return -1;
    }

    /* skin creation */
    assert(gltf->skins.n == 0 || cont->skins != NULL);
    for (size_t i = 0; i < gltf->skins.n; i++) {
        assert(cont->skins[i] == NULL);
        if (load_skin(gltf, fdata, cont, i) != 0)
            return -1;
    }

    /* material creation */
    assert(gltf->materials.n == 0 || cont->matls != NULL);
    for (size_t i = 0; i < gltf->materials.n; i++) {
        assert(cont->matls[i] == NULL);
        if (load_material(gltf, fdata, cont, i) != 0)
            return -1;
    }

    /* node creation */
    /* TODO: Filter joint nodes, since they must be instantiated from skin. */
    assert(gltf->nodes.n == 0 || cont->nodes != NULL);
    for (size_t i = 0; i < gltf->nodes.n; i++) {
        /* XXX: Can be non-null. */
        if (load_node(gltf, fdata, cont, i) != 0)
            return -1;
    }

    /* scene creation */
    assert(gltf->scenes.n == 0 || cont->scns != NULL);
    for (size_t i = 0; i < gltf->scenes.n; i++) {
        assert(cont->scns[i] == NULL);
        if (load_scene(gltf, fdata, cont, i) != 0)
            return -1;
    }

    return 0;
}

/* Manages created contents. */
static int manage_contents(const T_gltf *gltf, T_cont *cont,
                           YF_collection coll)
{
    assert(gltf != NULL);
    assert(cont != NULL);
    assert(coll != NULL);

    /* created scenes */
    if (cont->scns != NULL) {
        for (size_t i = 0; i < gltf->scenes.n; i++) {
            YF_scene scn = cont->scns[i];
            if (scn == NULL)
                continue;

            const char *name = gltf->scenes.v[i].name;
            if (yf_collection_manage(coll, YF_CITEM_SCENE, name, scn) != 0) {
                if (yf_geterr() != YF_ERR_EXIST ||
                    yf_collection_manage(coll, YF_CITEM_SCENE, NULL, scn) != 0)
                    return -1;
            }
        }
    }

    /* created nodes */
    if (cont->nodes != NULL) {
        for (size_t i = 0; i < gltf->nodes.n; i++) {
            YF_node node = cont->nodes[i];
            if (node == NULL)
                continue;

            const char *name = gltf->nodes.v[i].name;
            if (yf_collection_manage(coll, YF_CITEM_NODE, name, node) != 0) {
                if (yf_geterr() != YF_ERR_EXIST ||
                    yf_collection_manage(coll, YF_CITEM_NODE, NULL, node) != 0)
                    return -1;
            }
        }
    }

    /* created meshes */
    if (cont->meshes != NULL) {
        for (size_t i = 0; i < gltf->meshes.n; i++) {
            YF_mesh mesh = cont->meshes[i];
            if (mesh == NULL)
                continue;

            const char *name = gltf->meshes.v[i].name;
            if (yf_collection_manage(coll, YF_CITEM_MESH, name, mesh) != 0) {
                if (yf_geterr() != YF_ERR_EXIST ||
                    yf_collection_manage(coll, YF_CITEM_MESH, NULL, mesh) != 0)
                    return -1;
            }
        }
    }

    /* created textures */
    if (cont->texs != NULL) {
        for (size_t i = 0; i < gltf->textures.n; i++) {
            YF_texture tex = cont->texs[i];
            if (tex == NULL)
                continue;

            const char *name = gltf->textures.v[i].name;
            if (name == NULL) {
                const T_int image = gltf->textures.v[i].source;
                name = gltf->images.v[image].name;
                if (name == NULL)
                    name = gltf->images.v[image].uri;
            }

            if (yf_collection_manage(coll, YF_CITEM_TEXTURE, name, tex) != 0) {
                if (yf_geterr() != YF_ERR_EXIST ||
                    yf_collection_manage(coll, YF_CITEM_TEXTURE, NULL,
                                         tex) != 0)
                    return -1;
            }
        }
    }

    /* created skins */
    if (cont->skins != NULL) {
        for (size_t i = 0; i < gltf->skins.n; i++) {
            YF_skin skin = cont->skins[i];
            if (skin == NULL)
                continue;

            const char *name = gltf->skins.v[i].name;
            if (yf_collection_manage(coll, YF_CITEM_SKIN, name, skin) != 0) {
                if (yf_geterr() != YF_ERR_EXIST ||
                    yf_collection_manage(coll, YF_CITEM_SKIN, NULL, skin) != 0)
                    return -1;
            }
        }
    }

    /* created materials */
    if (cont->matls != NULL) {
        for (size_t i = 0; i < gltf->materials.n; i++) {
            YF_material matl = cont->matls[i];
            if (matl == NULL)
                continue;

            const char *name = gltf->materials.v[i].name;
            if (yf_collection_manage(coll, YF_CITEM_MATERIAL, name,
                                     matl) != 0) {
                if (yf_geterr() != YF_ERR_EXIST ||
                    yf_collection_manage(coll, YF_CITEM_MATERIAL, NULL,
                                         matl) != 0)
                    return -1;
            }
        }
    }

    return 0;
}

int yf_loadgltf(const char *pathname, size_t index, int datac, YF_datac *dst)
{
    assert(dst != NULL);

    if (pathname == NULL) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    FILE *file = fopen(pathname, "r");
    if (file == NULL) {
        yf_seterr(YF_ERR_NOFILE, __func__);
        return -1;
    }

    T_gltf gltf = {0};
    T_fdata fdata = {0};
    T_cont cont = {0};
    if (init_gltf(file, &gltf, &fdata, &cont) != 0) {
        fclose(file);
        return -1;
    }

    YF_PATHOF(pathname, fdata.path);
    if (fdata.path == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        cont.deinit = 1;
        deinit_gltf(&gltf, &fdata, &cont);
        fclose(file);
        return -1;
    }

    int r;
    switch (datac) {
    case YF_DATAC_COLL:
        if ((r = load_contents(&gltf, &fdata, &cont)) == 0)
            r = manage_contents(&gltf, &cont, dst->coll);
        break;
    case YF_DATAC_MESH:
        if ((r = load_mesh(&gltf, &fdata, &cont, index)) == 0)
            dst->mesh = cont.meshes[index];
        break;
    case YF_DATAC_TEX:
        if ((r = load_texture(&gltf, &fdata, &cont, index)) == 0)
            dst->tex = cont.texs[index];
        break;
    case YF_DATAC_SKIN:
        if ((r = load_skin(&gltf, &fdata, &cont, index)) == 0)
            dst->skin = cont.skins[index];
        break;
    case YF_DATAC_MATL:
        if ((r = load_material(&gltf, &fdata, &cont, index)) == 0)
            dst->matl = cont.matls[index];
        break;
    default:
        yf_seterr(YF_ERR_INVARG, __func__);
        r = -1;
    }

    cont.deinit = r;
    deinit_gltf(&gltf, &fdata, &cont);
    fclose(file);
    return r;
}

int yf_loadgltf2(FILE *file, size_t index, int datac, YF_datac *dst)
{
    assert(file != NULL && !feof(file));
    assert(dst != NULL);

    T_gltf gltf = {0};
    T_fdata fdata = {0};
    T_cont cont = {0};
    if (init_gltf(file, &gltf, &fdata, &cont) != 0)
        return -1;

    /* XXX: This function assumes that 'fdata.path' is not used, i.e., the
       whole data is embedded, thus it won't be set. */

    int r;
    switch (datac) {
    case YF_DATAC_COLL:
        if ((r = load_contents(&gltf, &fdata, &cont)) == 0)
            r = manage_contents(&gltf, &cont, dst->coll);
        break;
    case YF_DATAC_MESH:
        if ((r = load_mesh(&gltf, &fdata, &cont, index)) == 0)
            dst->mesh = cont.meshes[index];
        break;
    case YF_DATAC_TEX:
        if ((r = load_texture(&gltf, &fdata, &cont, index)) == 0)
            dst->tex = cont.texs[index];
        break;
    case YF_DATAC_SKIN:
        if ((r = load_skin(&gltf, &fdata, &cont, index)) == 0)
            dst->skin = cont.skins[index];
        break;
    case YF_DATAC_MATL:
        if ((r = load_material(&gltf, &fdata, &cont, index)) == 0)
            dst->matl = cont.matls[index];
        break;
    default:
        yf_seterr(YF_ERR_INVARG, __func__);
        r = -1;
    }

    cont.deinit = r;
    deinit_gltf(&gltf, &fdata, &cont);
    return r;
}

/*
 * DEVEL
 */

#ifdef YF_DEVEL
static void print_gltf(const T_gltf *gltf)
{
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
    for (size_t i = 0; i < gltf->scenes.n; i++) {
        printf(" scene '%s':\n", gltf->scenes.v[i].name);
        printf("  nodes: [ ");
        for (size_t j = 0; j < gltf->scenes.v[i].node_n; j++)
            printf("%lld ", gltf->scenes.v[i].nodes[j]);
        puts("]");
    }

    puts("glTF.nodes:");
    printf(" n: %lu\n", gltf->nodes.n);
    for (size_t i = 0; i < gltf->nodes.n; i++) {
        printf(" node '%s':\n", gltf->nodes.v[i].name);
        printf("  children: [ ");
        for (size_t j = 0; j < gltf->nodes.v[i].child_n; j++)
            printf("%lld ", gltf->nodes.v[i].children[j]);
        puts("]");
        printf("  camera: %lld\n", gltf->nodes.v[i].camera);
        printf("  mesh: %lld\n", gltf->nodes.v[i].mesh);
        if (gltf->nodes.v[i].xform_mask == YF_GLTF_XFORM_NONE) {
            puts("  (no transform)");
        } else if (gltf->nodes.v[i].xform_mask == YF_GLTF_XFORM_NONE) {
            printf("  matrix: [ ");
            for (size_t j = 0; j < 16; j++)
                printf("%.4f ", gltf->nodes.v[i].matrix[j]);
            puts("]");
        } else {
            printf("  translation: [ ");
            for (size_t j = 0; j < 3; j++)
                printf("%.4f ", gltf->nodes.v[i].trs.t[j]);
            puts("]");
            printf("  rotation: [ ");
            for (size_t j = 0; j < 4; j++)
                printf("%.4f ", gltf->nodes.v[i].trs.r[j]);
            puts("]");
            printf("  scale: [ ");
            for (size_t j = 0; j < 3; j++)
                printf("%.4f ", gltf->nodes.v[i].trs.s[j]);
            puts("]");
        }
        printf("  skin: %lld\n", gltf->nodes.v[i].skin);
        printf("  weights: [ ");
        for (size_t j = 0; j < gltf->nodes.v[i].weight_n; j++)
            printf("%.9f ", gltf->nodes.v[i].weights[j]);
        puts("]");
    }

    puts("glTF.cameras:");
    printf(" n: %lu\n", gltf->cameras.n);
    for (size_t i = 0; i < gltf->cameras.n; i++) {
        printf(" camera '%s':\n", gltf->cameras.v[i].name);
        if (gltf->cameras.v[i].type == YF_GLTF_CAMERA_PERSP) {
            puts("  pespective:");
            printf("   yfov: %.9f\n", gltf->cameras.v[i].persp.yfov);
            printf("   aspectRatio: %.9f\n",
                   gltf->cameras.v[i].persp.aspect_ratio);
            printf("   znear: %.9f\n", gltf->cameras.v[i].persp.znear);
            printf("   zfar: %.9f\n", gltf->cameras.v[i].persp.zfar);
        } else {
            puts("  orthographic:");
            printf("   xmag: %.9f\n", gltf->cameras.v[i].ortho.xmag);
            printf("   ymag: %.9f\n", gltf->cameras.v[i].ortho.ymag);
            printf("   znear: %.9f\n", gltf->cameras.v[i].ortho.znear);
            printf("   zfar: %.9f\n", gltf->cameras.v[i].ortho.zfar);
        }
    }

    puts("glTF.meshes:");
    printf(" n: %lu\n", gltf->meshes.n);
    for (size_t i = 0; i < gltf->meshes.n; i++) {
        printf(" mesh '%s':\n", gltf->meshes.v[i].name);
        printf("  n: %lu\n", gltf->meshes.v[i].primitives.n);
        for (size_t j = 0; j < gltf->meshes.v[i].primitives.n; j++) {
            printf("  primitives #%lu:\n", j);
            puts("   attributes:");
            printf("    POSITION: %lld\n",
                   gltf->meshes.v[i].primitives.v[j]
                   .attributes[YF_GLTF_ATTR_POS]);
            printf("    NORMAL: %lld\n",
                   gltf->meshes.v[i].primitives.v[j]
                   .attributes[YF_GLTF_ATTR_NORM]);
            printf("    TANGENT: %lld\n",
                   gltf->meshes.v[i].primitives.v[j]
                   .attributes[YF_GLTF_ATTR_TAN]);
            printf("    TEXCOORD_0: %lld\n",
                   gltf->meshes.v[i].primitives.v[j]
                   .attributes[YF_GLTF_ATTR_TC0]);
            printf("    TEXCOORD_1: %lld\n",
                   gltf->meshes.v[i].primitives.v[j]
                   .attributes[YF_GLTF_ATTR_TC1]);
            printf("    JOINTS_0: %lld\n",
                   gltf->meshes.v[i].primitives.v[j]
                   .attributes[YF_GLTF_ATTR_JNT0]);
            printf("    WEIGHTS_0: %lld\n",
                   gltf->meshes.v[i].primitives.v[j]
                   .attributes[YF_GLTF_ATTR_WGT0]);
            printf("   indices: %lld\n",
                   gltf->meshes.v[i].primitives.v[j].indices);
            printf("   material: %lld\n",
                   gltf->meshes.v[i].primitives.v[j].material);
            printf("   mode: %lld\n", gltf->meshes.v[i].primitives.v[j].mode);
            if (gltf->meshes.v[i].primitives.v[j].targets.n == 0) {
                puts("   (no targets)");
            } else {
                const size_t target_n =
                    gltf->meshes.v[i].primitives.v[j].targets.n;
                for (size_t k = 0; k < target_n; k++) {
                    printf("   targets #%lu:\n", k);
                    printf("    POSITION: %lld\n",
                           gltf->meshes.v[i].primitives.v[j]
                           .targets.v[k].position);
                    printf("    NORMAL: %lld\n",
                           gltf->meshes.v[i].primitives.v[j]
                           .targets.v[k].normal);
                    printf("    TANGENT: %lld\n",
                           gltf->meshes.v[i].primitives.v[j]
                           .targets.v[k].tangent);
                }
            }
        }
        printf("  weights: [ ");
        for (size_t j = 0; j < gltf->meshes.v[i].weight_n; j++)
            printf("%.9f ", gltf->meshes.v[i].weights[j]);
        puts("]");
    }

    puts("glTF.skins:");
    printf(" n: %lu\n", gltf->skins.n);
    for (size_t i = 0; i < gltf->skins.n; i++) {
        printf(" skin '%s':\n", gltf->skins.v[i].name);
        printf("  inverseBindMatrices: %lld\n",
               gltf->skins.v[i].inv_bind_matrices);
        printf("  skeleton: %lld\n", gltf->skins.v[i].skeleton);
        printf("  joints: [ ");
        for (size_t j = 0; j < gltf->skins.v[i].joint_n; j++)
            printf("%lld ", gltf->skins.v[i].joints[j]);
        puts("]");
    }

    puts("glTF.materials:");
    printf(" n: %lu\n", gltf->materials.n);
    for (size_t i = 0; i < gltf->materials.n; i++) {
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

    puts("glTF.animations:");
    printf(" n: %lu\n", gltf->animations.n);
    for (size_t i = 0; i < gltf->animations.n; i++) {
        printf(" animation '%s':\n", gltf->animations.v[i].name);
        puts("  channels:");
        printf("  n: %lu\n", gltf->animations.v[i].channels.n);
        for (size_t j = 0; j < gltf->animations.v[i].channels.n; j++) {
            printf("  channel #%lu:\n", j);
            printf("   sampler: %lld\n",
                   gltf->animations.v[i].channels.v[j].sampler);
            puts("   target:");
            printf("    node: %lld\n",
                   gltf->animations.v[i].channels.v[j].target.node);
            printf("    path: %d\n",
                   gltf->animations.v[i].channels.v[j].target.path);
        }
        puts("  samplers:");
        printf("  n: %lu\n", gltf->animations.v[i].samplers.n);
        for (size_t j = 0; j < gltf->animations.v[i].samplers.n; j++) {
            printf("  channel #%lu:\n", j);
            printf("   input: %lld\n",
                   gltf->animations.v[i].samplers.v[j].input);
            printf("   output: %lld\n",
                   gltf->animations.v[i].samplers.v[j].output);
            printf("   interpolation: %d\n",
                   gltf->animations.v[i].samplers.v[j].interpolation);
        }
    }

    puts("glTF.accessors:");
    printf(" n: %lu\n", gltf->accessors.n);
    for (size_t i = 0; i < gltf->accessors.n; i++) {
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
        }
        if (comp_n > 1) {
            printf("  min: [ ");
            for (size_t j = 0; j < comp_n; j++)
                printf("%.9f ", gltf->accessors.v[i].min.m4[j]);
            printf("]\n  max: [ ");
            for (size_t j = 0; j < comp_n; j++)
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
    for (size_t i = 0; i < gltf->bufferviews.n; i++) {
        printf(" buffer view '%s':\n", gltf->bufferviews.v[i].name);
        printf("  buffer: %lld\n", gltf->bufferviews.v[i].buffer);
        printf("  byteOffset: %lld\n", gltf->bufferviews.v[i].byte_off);
        printf("  byteLength: %lld\n", gltf->bufferviews.v[i].byte_len);
        printf("  byteStride: %lld\n", gltf->bufferviews.v[i].byte_strd);
        printf("  target: %lld\n", gltf->bufferviews.v[i].target);
    }

    puts("glTF.buffers:");
    printf(" n: %lu\n", gltf->buffers.n);
    for (size_t i = 0; i < gltf->buffers.n; i++) {
        printf(" buffer '%s':\n", gltf->buffers.v[i].name);
        printf("  byteLength: %lld\n", gltf->buffers.v[i].byte_len);
        printf("  uri: %s\n", gltf->buffers.v[i].uri);
    }

    puts("glTF.textures:");
    printf(" n: %lu\n", gltf->textures.n);
    for (size_t i = 0; i < gltf->textures.n; i++) {
        printf(" texture '%s':\n", gltf->textures.v[i].name);
        printf("  sampler: %lld\n", gltf->textures.v[i].sampler);
        printf("  source: %lld\n", gltf->textures.v[i].source);
    }

    puts("glTF.images:");
    printf(" n: %lu\n", gltf->images.n);
    for (size_t i = 0; i < gltf->images.n; i++) {
        printf(" image '%s':\n", gltf->images.v[i].name);
        printf("  uri: %s\n", gltf->images.v[i].uri);
        printf("  mimeType: %s\n", gltf->images.v[i].mime_type);
        printf("  bufferView: %lld\n", gltf->images.v[i].buffer_view);
    }

    puts("glTF.samplers:");
    printf(" n: %lu\n", gltf->samplers.n);
    for (size_t i = 0; i < gltf->samplers.n; i++) {
        printf(" sampler '%s':\n", gltf->samplers.v[i].name);
        printf("  minFilter: %lld\n", gltf->samplers.v[i].min_filter);
        printf("  magFilter: %lld\n", gltf->samplers.v[i].mag_filter);
        printf("  wrapS: %lld\n", gltf->samplers.v[i].wrap_s);
        printf("  wrapT: %lld\n", gltf->samplers.v[i].wrap_t);
    }
}
#endif
