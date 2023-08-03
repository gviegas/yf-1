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

#ifdef YF_DEVEL
# include <stdio.h>
#endif

#include "yf/com/yf-error.h"

#include "data-gltf.h"
#include "data-png.h"
#include "yf-model.h"
#include "yf-light.h"

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

/* TODO: Make token length dynamic to support base64. */
#define YF_TOKENMAX 1024

/* Token type/data. */
typedef struct {
    int token;
    char data[YF_TOKENMAX];
} token_t;

/* Gets the next token from a file stream.
   This function returns 'token->token'. Upon failure, the value returned
   is equal to 'YF_TOKEN_ERR' - the global error variable is not set. */
static int next_token(FILE *file, token_t *token)
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

/* String. */
typedef char *str_t;

/* Floating-point number. */
typedef float num_t;

/* Integer number. */
typedef long long int_t;
#define YF_INT_MIN LLONG_MIN
#define YF_INT_MAX LLONG_MAX

/* Boolean value. */
typedef int bool_t;
#define YF_TRUE  1
#define YF_FALSE 0

/* Parses an array of unknown size. */
static int parse_array(FILE *file, token_t *token,
                       void **array, size_t *n, size_t elem_sz,
                       int (*fn)(FILE *, token_t *, size_t index, void *arg),
                       void *arg)
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
static int parse_str(FILE *file, token_t *token, str_t *str)
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

/* Parses an element of an array of strings. */
static int parse_str_array(FILE *file, token_t *token, size_t index,
                           void *str_pp)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(str_pp != NULL);

    str_t *str_p = *(str_t **)str_pp;
    assert(str_p != NULL);

    return parse_str(file, token, str_p+index);
}

/* Parses a floating-point number. */
static int parse_num(FILE *file, token_t *token, num_t *num)
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
    *num = strtof(token->data, &end);
    if (errno != 0 || *end != '\0') {
        yf_seterr(YF_ERR_OTHER, __func__);
        return -1;
    }
    return 0;
}

/* Parses an element of an array of floating-point numbers. */
static int parse_num_array(FILE *file, token_t *token, size_t index,
                           void *num_pp)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(num_pp != NULL);

    num_t *num_p = *(num_t **)num_pp;
    assert(num_p != NULL);

    return parse_num(file, token, num_p+index);
}

/* Parses an integer number. */
static int parse_int(FILE *file, token_t *token, int_t *intr)
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
static int parse_int_array(FILE *file, token_t *token, size_t index,
                           void *int_pp)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(int_pp != NULL);

    int_t *int_p = *(int_t **)int_pp;
    assert(int_p != NULL);

    return parse_int(file, token, int_p+index);
}

/* Parses a boolean value. */
static int parse_bool(FILE *file, token_t *token, bool_t *booln)
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
static int consume_prop(FILE *file, token_t *token)
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

/* The 'glTF.asset' property. */
typedef struct {
    str_t copyright;
    str_t generator;
    str_t version;
    str_t min_version;
} asset_t;

/* The 'glTF.scenes' property. */
typedef struct {
    struct {
        int_t *nodes;
        size_t node_n;
        str_t name;
    } *v;
    size_t n;
} scenes_t;

/* The 'glTF.nodes' property. */
typedef struct {
    struct {
        int_t *children;
        size_t child_n;
        int_t camera;
        int_t mesh;
#define YF_GLTF_XFORM_NONE 0
#define YF_GLTF_XFORM_M    0x01
#define YF_GLTF_XFORM_T    0x02
#define YF_GLTF_XFORM_R    0x04
#define YF_GLTF_XFORM_S    0x08
        unsigned xform_mask;
        union {
            num_t matrix[16];
            struct { num_t t[3], r[4], s[3]; } trs;
        };
        int_t skin;
        num_t *weights;
        size_t weight_n;
        str_t name;
        struct {
            int_t light;
        } ext;
    } *v;
    size_t n;
} nodes_t;

/* The 'glTF.cameras.perspective' property. */
typedef struct {
    num_t yfov;
    num_t aspect_ratio;
    num_t znear;
    num_t zfar;
} perspective_t;

/* The 'glTF.cameras.orthographic' property. */
typedef struct {
    num_t xmag;
    num_t ymag;
    num_t znear;
    num_t zfar;
} orthographic_t;

/* The 'glTF.cameras' property. */
typedef struct {
    struct {
#define YF_GLTF_CAMERA_PERSP 0
#define YF_GLTF_CAMERA_ORTHO 1
        int type;
        union {
            perspective_t persp;
            orthographic_t ortho;
        };
        str_t name;
    } *v;
    size_t n;
} cameras_t;

/* The 'glTF.meshes.primitives.targets' property. */
typedef struct {
    struct {
        int_t position;
        int_t normal;
        int_t tangent;
    } *v;
    size_t n;
} targets_t;

/* The 'glTF.meshes.primitives' property. */
typedef struct {
    struct {
#define YF_GLTF_ATTR_POS  0
#define YF_GLTF_ATTR_NORM 1
#define YF_GLTF_ATTR_TGNT 2
#define YF_GLTF_ATTR_TC0  3
#define YF_GLTF_ATTR_TC1  4
#define YF_GLTF_ATTR_CLR0 5
#define YF_GLTF_ATTR_JNT0 6
#define YF_GLTF_ATTR_WGT0 7
#define YF_GLTF_ATTR_N    8
        /* XXX: Expected to match 'VSEMT' order. */
        int_t attributes[YF_GLTF_ATTR_N];
        int_t indices;
        int_t material;
#define YF_GLTF_MODE_PTS      0
#define YF_GLTF_MODE_LNS      1
#define YF_GLTF_MODE_LNLOOP   2
#define YF_GLTF_MODE_LNSTRIP  3
#define YF_GLTF_MODE_TRIS     4
#define YF_GLTF_MODE_TRISTRIP 5
#define YF_GLTF_MODE_TRIFAN   6
        int_t mode;
        targets_t targets;
    } *v;
    size_t n;
} primitives_t;

/* The 'glTF.meshes' property. */
typedef struct {
    struct {
        primitives_t primitives;
        num_t *weights;
        size_t weight_n;
        str_t name;
    } *v;
    size_t n;
} meshes_t;

/* The 'glTF.skins' property. */
typedef struct {
    struct {
        int_t inv_bind_matrices;
        int_t skeleton;
        int_t *joints;
        size_t joint_n;
        str_t name;
    } *v;
    size_t n;
} skins_t;

/* The 'glTF.*.textureInfo' property. */
typedef struct {
    int_t index;
    int_t tex_coord;
    union {
        num_t scale;
        num_t strength;
    };
} textureinfo_t;

/* The 'glTF.materials.extensions.KHR_materials_pbrSpecularGlossiness'
   property. */
/* TODO: This extension is deprecated. */
typedef struct {
    num_t diffuse_fac[4];
    textureinfo_t diffuse_tex;
    num_t specular_fac[3];
    num_t glossiness_fac;
    textureinfo_t spec_gloss_tex;
} pbrspecgloss_t;

/* The 'glTF.materials.pbrMetallicRoughness' property. */
typedef struct {
    num_t base_clr_fac[4];
    textureinfo_t base_clr_tex;
    num_t metallic_fac;
    num_t roughness_fac;
    textureinfo_t metal_rough_tex;
} pbrmetalrough_t;

/* The 'glTF.materials' property. */
typedef struct {
    struct {
        pbrmetalrough_t pbrmr;
        textureinfo_t normal_tex;
        textureinfo_t occlusion_tex;
        num_t emissive_fac[3];
        textureinfo_t emissive_tex;
#define YF_GLTF_ALPHA_OPAQUE 0
#define YF_GLTF_ALPHA_MASK   1
#define YF_GLTF_ALPHA_BLEND  2
        int alpha_mode;
        num_t alpha_cutoff;
        bool_t double_sided;
        str_t name;
        struct {
            pbrspecgloss_t *pbrsg;
            int unlit;
        } ext;
    } *v;
    size_t n;
} materials_t;

/* The 'glTF.animations.channels.target' property. */
typedef struct {
    int_t node;
#define YF_GLTF_PATH_XLATE  0
#define YF_GLTF_PATH_ROTATE 1
#define YF_GLTF_PATH_SCALE  2
#define YF_GLTF_PATH_WEIGHT 3
    int path;
} ctarget_t;

/* The 'glTF.animations.channels' property. */
typedef struct {
    struct {
        int_t sampler;
        ctarget_t target;
    } *v;
    size_t n;
} channels_t;

/* The 'glTF.animations.samplers' property. */
typedef struct {
    struct {
        int_t input;
        int_t output;
#define YF_GLTF_ERP_LINEAR 0
#define YF_GLTF_ERP_STEP   1
#define YF_GLTF_ERP_CUBIC  2
        int interpolation;
    } *v;
    size_t n;
} asamplers_t;

/* The 'glTF.animations' property. */
typedef struct {
    struct {
        channels_t channels;
        asamplers_t samplers;
        str_t name;
    } *v;
    size_t n;
} animations_t;

/* The 'glTF.accessors.sparse.indices' property. */
typedef struct {
    int_t buffer_view;
    int_t byte_off;
#define YF_GLTF_COMP_UBYTE  5121
#define YF_GLTF_COMP_USHORT 5123
#define YF_GLTF_COMP_UINT   5125
    int_t comp_type;
} sindices_t;

/* The 'glTF.accessors.sparse.values' property. */
typedef struct {
    int_t buffer_view;
    int_t byte_off;
} svalues_t;

/* The 'glTF.accessors.sparse' property. */
typedef struct {
    int_t count;
    sindices_t indices;
    svalues_t values;
} sparse_t;

/* The 'glTF.accessors' property. */
typedef struct {
    struct {
        int_t buffer_view;
        int_t byte_off;
        int_t count;
#define YF_GLTF_COMP_BYTE   5120
#define YF_GLTF_COMP_UBYTE  5121
#define YF_GLTF_COMP_SHORT  5122
#define YF_GLTF_COMP_USHORT 5123
#define YF_GLTF_COMP_UINT   5125
#define YF_GLTF_COMP_FLOAT  5126
        int_t comp_type;
#define YF_GLTF_TYPE_SCALAR 1
#define YF_GLTF_TYPE_VEC2   2
#define YF_GLTF_TYPE_VEC3   3
#define YF_GLTF_TYPE_VEC4   4
#define YF_GLTF_TYPE_MAT2   22
#define YF_GLTF_TYPE_MAT3   33
#define YF_GLTF_TYPE_MAT4   44
        int type;
        union {
            /* XXX: May not suffice. */
            num_t s;
            num_t v2[2];
            num_t v3[3];
            num_t v4[4];
            num_t m2[4];
            num_t m3[9];
            num_t m4[16];
        } min, max;
        bool_t normalized;
        sparse_t sparse;
        str_t name;
    } *v;
    size_t n;
} accessors_t;

/* The 'glTF.bufferViews' property. */
typedef struct {
    struct {
        int_t buffer;
        int_t byte_off;
        int_t byte_len;
        int_t byte_strd;
#define YF_GLTF_TARGET_BUF     34962
#define YF_GLTF_TARGET_ELEMBUF 34963
        int_t target;
        str_t name;
    } *v;
    size_t n;
} bufferviews_t;

/* The 'glTF.buffers' property. */
typedef struct {
    struct {
        int_t byte_len;
        str_t uri;
        str_t name;
    } *v;
    size_t n;
} buffers_t;

/* The 'glTF.textures' property. */
typedef struct {
    struct {
        int_t sampler;
        int_t source;
        str_t name;
    } *v;
    size_t n;
} textures_t;

/* The 'glTF.images' property. */
typedef struct {
    struct {
        str_t uri;
        str_t mime_type;
        int_t buffer_view;
        str_t name;
    } *v;
    size_t n;
} images_t;

/* The 'glTF.samplers' property. */
typedef struct {
    struct {
#define YF_GLTF_WRAP_CLAMP  33071
#define YF_GLTF_WRAP_MIRROR 33648
#define YF_GLTF_WRAP_REPEAT 10497
        int_t wrap_s;
        int_t wrap_t;
#define YF_GLTF_FILTER_NEAREST 9728
#define YF_GLTF_FILTER_LINEAR  9729
#define YF_GLTF_FILTER_NRMIPNR 9984
#define YF_GLTF_FILTER_LNMIPNR 9985
#define YF_GLTF_FILTER_NRMIPLN 9986
#define YF_GLTF_FILTER_LNMIPLN 9987
        int_t mag_filter;
        int_t min_filter;
        str_t name;
    } *v;
    size_t n;
} samplers_t;

/* The 'glTF.extensions.KHR_lights_punctual.lights.spot' property. */
typedef struct {
    num_t inner_cone_angle;
    num_t outer_cone_angle;
} spot_t;

/* The 'glTF.extensions.KHR_lights_punctual.lights' property. */
typedef struct {
    struct {
#define YF_GLTF_LIGHT_POINT  0
#define YF_GLTF_LIGHT_SPOT   1
#define YF_GLTF_LIGHT_DIRECT 2
        int type;
        num_t color[3];
        num_t intensity;
        num_t range;
        spot_t spot;
        str_t name;
    } *v;
    size_t n;
} lights_t;

/* The root glTF object. */
typedef struct {
    str_t *ext_used;
    size_t ext_used_n;
    str_t *ext_req;
    size_t ext_req_n;
    asset_t asset;
    int_t scene;
    scenes_t scenes;
    nodes_t nodes;
    cameras_t cameras;
    meshes_t meshes;
    skins_t skins;
    materials_t materials;
    animations_t animations;
    accessors_t accessors;
    bufferviews_t bufferviews;
    buffers_t buffers;
    textures_t textures;
    images_t images;
    samplers_t samplers;
    struct {
        lights_t lights;
    } ext;
} gltf_t;

/* Parses the 'glTF.asset' property. */
static int parse_asset(FILE *file, token_t *token, asset_t *asset)
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
static int parse_scene(FILE *file, token_t *token, int_t *scene)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(scene != NULL);
    assert(token->token == YF_TOKEN_STR);
    assert(strcmp(token->data, "scene") == 0);

    return parse_int(file, token, scene);
}

/* Parses the 'glTF.scenes' property. */
static int parse_scenes(FILE *file, token_t *token, size_t index,
                        void *scenes_p)
{
    scenes_t *scenes = scenes_p;

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

/* Parses the 'glTF.nodes.extensions.KHR_lights_punctual' property. */
static int parse_node_punctual(FILE *file, token_t *token, size_t index,
                               nodes_t *nodes)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(nodes != NULL);
    assert(index < nodes->n);
    assert(token->token == YF_TOKEN_STR);
    assert(strcmp(token->data, "KHR_lights_punctual") == 0);

    next_token(file, token); /* ':' */
    next_token(file, token); /* '{' */

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("light", token->data) == 0) {
                if (parse_int(file, token, &nodes->v[index].ext.light) != 0)
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

/* Parses the 'glTF.nodes.extensions' property. */
static int parse_node_ext(FILE *file, token_t *token, size_t index,
                          nodes_t *nodes)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(nodes != NULL);
    assert(index < nodes->n);
    assert(token->token == YF_TOKEN_STR);
    assert(strcmp(token->data, "extensions") == 0);

    next_token(file, token); /* ':' */
    next_token(file, token); /* '{' */

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("KHR_lights_punctual", token->data) == 0) {
                if (parse_node_punctual(file, token, index, nodes) != 0)
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
static int parse_nodes(FILE *file, token_t *token, size_t index, void *nodes_p)
{
    nodes_t *nodes = nodes_p;

    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(nodes != NULL);
    assert(index < nodes->n);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '[' || token->data[0] == ',');

    nodes->v[index].mesh = YF_INT_MIN;
    nodes->v[index].camera = YF_INT_MIN;
    nodes->v[index].skin = YF_INT_MIN;
    nodes->v[index].ext.light = YF_INT_MIN;

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

            } else if (strcmp("extensions", token->data) == 0) {
                if (parse_node_ext(file, token, index, nodes) != 0)
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
                        nodes->v[index].trs.r[3] = 1.0f;
                    if (!(mask & YF_GLTF_XFORM_S)) {
                        nodes->v[index].trs.s[0] = 1.0f;
                        nodes->v[index].trs.s[1] = 1.0f;
                        nodes->v[index].trs.s[2] = 1.0f;
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

/* Parses the 'glTF.cameras.perspective' property. */
static int parse_perspective(FILE *file, token_t *token,
                             perspective_t *perspective)
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

/* Parses the 'glTF.cameras.orthographic' property. */
static int parse_orthographic(FILE *file, token_t *token,
                              orthographic_t *orthographic)
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
static int parse_cameras(FILE *file, token_t *token, size_t index,
                         void *cameras_p)
{
    cameras_t *cameras = cameras_p;

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
static int parse_attributes(FILE *file, token_t *token, int_t *attributes)
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
                if (parse_int(file, token, attributes+YF_GLTF_ATTR_TGNT) != 0)
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
static int parse_targets(FILE *file, token_t *token, size_t index,
                         void *targets_p)
{
    targets_t *targets = targets_p;

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
static int parse_primitives(FILE *file, token_t *token, size_t index,
                            void *primitives_p)
{
    primitives_t *primitives = primitives_p;

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
static int parse_meshes(FILE *file, token_t *token, size_t index,
                        void *meshes_p)
{
    meshes_t *meshes = meshes_p;

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
static int parse_skins(FILE *file, token_t *token, size_t index, void *skins_p)
{
    skins_t *skins = skins_p;

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
static int parse_textureinfo(FILE *file, token_t *token,
                             textureinfo_t *textureinfo)
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

/* Parses the 'glTF.materials.extensions.KHR_materials_pbrSpecularGlossiness'
   property. */
static int parse_pbrspecgloss(FILE *file, token_t *token,
                              pbrspecgloss_t *pbrspecgloss)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(pbrspecgloss != NULL);
    assert(token->token == YF_TOKEN_STR);
    assert(strcmp(token->data, "KHR_materials_pbrSpecularGlossiness") == 0);

    next_token(file, token); /* ':' */
    next_token(file, token); /* '{' */

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("diffuseFactor", token->data) == 0) {
                next_token(file, token); /* ':' */
                next_token(file, token); /* '[' */
                for (size_t i = 0; i < 4; i++) {
                    if (parse_num(file, token,
                                  pbrspecgloss->diffuse_fac+i) != 0)
                        return -1;
                }
                next_token(file, token); /* ']' */

            } else if (strcmp("diffuseTexture", token->data) == 0) {
                if (parse_textureinfo(file, token,
                                      &pbrspecgloss->diffuse_tex) != 0)
                    return -1;

            } else if (strcmp("specularFactor", token->data) == 0) {
                next_token(file, token); /* ':' */
                next_token(file, token); /* '[' */
                for (size_t i = 0; i < 3; i++) {
                    if (parse_num(file, token,
                                  pbrspecgloss->specular_fac+i) != 0)
                        return -1;
                }
                next_token(file, token); /* ']' */

            } else if (strcmp("glossinessFactor", token->data) == 0) {
                if (parse_num(file, token, &pbrspecgloss->glossiness_fac) != 0)
                    return -1;

            } else if (strcmp("specularGlossinessTexture", token->data) == 0) {
                if (parse_textureinfo(file, token,
                                      &pbrspecgloss->spec_gloss_tex) != 0)
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

/* Parses the 'glTF.materials.extensions' property. */
static int parse_material_ext(FILE *file, token_t *token, size_t index,
                              materials_t *materials)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(materials != NULL);
    assert(index < materials->n);
    assert(token->token == YF_TOKEN_STR);
    assert(strcmp(token->data, "extensions") == 0);

    next_token(file, token); /* ':' */
    next_token(file, token); /* '{' */

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("KHR_materials_pbrSpecularGlossiness",
                       token->data) == 0) {

                if (materials->v[index].ext.pbrsg != NULL ||
                    materials->v[index].ext.unlit) {
                    yf_seterr(YF_ERR_INVFILE, __func__);
                    return -1;
                }

                pbrspecgloss_t *pbrsg = malloc(sizeof *pbrsg);
                if (pbrsg == NULL) {
                    yf_seterr(YF_ERR_NOMEM, __func__);
                    return -1;
                }
                pbrsg->diffuse_fac[0] = 1.0f;
                pbrsg->diffuse_fac[1] = 1.0f;
                pbrsg->diffuse_fac[2] = 1.0f;
                pbrsg->diffuse_fac[3] = 1.0f;
                pbrsg->diffuse_tex.index = YF_INT_MIN;
                pbrsg->specular_fac[0] = 1.0f;
                pbrsg->specular_fac[1] = 1.0f;
                pbrsg->specular_fac[2] = 1.0f;
                pbrsg->glossiness_fac = 1.0f;
                pbrsg->spec_gloss_tex.index = YF_INT_MIN;

                if (parse_pbrspecgloss(file, token, pbrsg) != 0) {
                    free(pbrsg);
                    return -1;
                }
                materials->v[index].ext.pbrsg = pbrsg;

            } else if (strcmp("KHR_materials_unlit", token->data) == 0) {
                if (materials->v[index].ext.pbrsg != NULL ||
                    materials->v[index].ext.unlit) {
                    yf_seterr(YF_ERR_INVFILE, __func__);
                    return -1;
                }

                if (consume_prop(file, token) != 0)
                    return -1;
                materials->v[index].ext.unlit = 1;

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
static int parse_pbrmetalrough(FILE *file, token_t *token,
                               pbrmetalrough_t *pbrmetalrough)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(pbrmetalrough != NULL);
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
static int parse_materials(FILE *file, token_t *token, size_t index,
                           void *materials_p)
{
    materials_t *materials = materials_p;

    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(materials != NULL);
    assert(index < materials->n);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '[' || token->data[0] == ',');

    materials->v[index].pbrmr.base_clr_fac[0] = 1.0f;
    materials->v[index].pbrmr.base_clr_fac[1] = 1.0f;
    materials->v[index].pbrmr.base_clr_fac[2] = 1.0f;
    materials->v[index].pbrmr.base_clr_fac[3] = 1.0f;
    materials->v[index].pbrmr.base_clr_tex.index = YF_INT_MIN;
    materials->v[index].pbrmr.metallic_fac = 1.0f;
    materials->v[index].pbrmr.roughness_fac = 1.0f;
    materials->v[index].pbrmr.metal_rough_tex.index = YF_INT_MIN;
    materials->v[index].normal_tex.index = YF_INT_MIN;
    materials->v[index].normal_tex.scale = 1.0f;
    materials->v[index].occlusion_tex.index = YF_INT_MIN;
    materials->v[index].occlusion_tex.strength = 1.0f;
    materials->v[index].emissive_tex.index = YF_INT_MIN;
    materials->v[index].alpha_cutoff = 0.5f;

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

            } else if (strcmp("extensions", token->data) == 0) {
                if (parse_material_ext(file, token, index, materials) != 0)
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
static int parse_ctarget(FILE *file, token_t *token, ctarget_t *ctarget)
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
static int parse_channels(FILE *file, token_t *token, size_t index,
                          void *channels_p)
{
    channels_t *channels = channels_p;

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
static int parse_asamplers(FILE *file, token_t *token, size_t index,
                           void *asamplers_p)
{
    asamplers_t *asamplers = asamplers_p;

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
static int parse_animations(FILE *file, token_t *token, size_t index,
                            void *animations_p)
{
    animations_t *animations = animations_p;

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
static int parse_sindices(FILE *file, token_t *token, sindices_t *sindices)
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
static int parse_svalues(FILE *file, token_t *token, svalues_t *svalues)
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
static int parse_sparse(FILE *file, token_t *token, sparse_t *sparse)
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
static int parse_accessors(FILE *file, token_t *token, size_t index,
                           void *accessors_p)
{
    accessors_t *accessors = accessors_p;

    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(accessors != NULL);
    assert(index < accessors->n);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '[' || token->data[0] == ',');

    accessors->v[index].buffer_view = YF_INT_MIN;
    accessors->v[index].sparse.indices.buffer_view = YF_INT_MIN;
    accessors->v[index].sparse.values.buffer_view = YF_INT_MIN;
    for (size_t i = 0; i < 16; i++) {
        accessors->v[index].min.m4[i] = YF_INT_MIN;
        accessors->v[index].max.m4[i] = YF_INT_MAX;
    }

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
static int parse_bufferviews(FILE *file, token_t *token, size_t index,
                             void *bufferviews_p)
{
    bufferviews_t *bufferviews = bufferviews_p;

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
static int parse_buffers(FILE *file, token_t *token, size_t index,
                         void *buffers_p)
{
    buffers_t *buffers = buffers_p;

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
static int parse_textures(FILE *file, token_t *token, size_t index,
                          void *textures_p)
{
    textures_t *textures = textures_p;

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
static int parse_images(FILE *file, token_t *token, size_t index,
                        void *images_p)
{
    images_t *images = images_p;

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
static int parse_samplers(FILE *file, token_t *token, size_t index,
                          void *samplers_p)
{
    samplers_t *samplers = samplers_p;

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
            if (strcmp("wrapS", token->data) == 0) {
                if (parse_int(file, token, &samplers->v[index].wrap_s) != 0)
                    return -1;

            } else if (strcmp("wrapT", token->data) == 0) {
                if (parse_int(file, token, &samplers->v[index].wrap_t) != 0)
                    return -1;

            } else if (strcmp("magFilter", token->data) == 0) {
                if (parse_int(file, token, &samplers->v[index].mag_filter) != 0)
                    return -1;

            } else if (strcmp("minFilter", token->data) == 0) {
                if (parse_int(file, token, &samplers->v[index].min_filter) != 0)
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

/* Parses the 'glTF.extensions.KHR_lights_punctual.lights.spot' property. */
static int parse_spot(FILE *file, token_t *token, spot_t *spot)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(spot != NULL);
    assert(token->token == YF_TOKEN_STR);
    assert(strcmp(token->data, "spot") == 0);

    next_token(file, token); /* ':' */
    next_token(file, token); /* '{' */

    spot->inner_cone_angle = 0.0f;
    spot->outer_cone_angle = 0.7853981633974483f;

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("innerConeAngle", token->data) == 0) {
                if (parse_num(file, token, &spot->inner_cone_angle) != 0)
                    return -1;

            } else if (strcmp("outerConeAngle", token->data) == 0) {
                if (parse_num(file, token, &spot->outer_cone_angle) != 0)
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

/* Parses the 'glTF.extensions.KHR_lights_punctual.lights' property. */
static int parse_lights(FILE *file, token_t *token, size_t index,
                        void *lights_p)
{
    lights_t *lights = lights_p;

    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(lights != NULL);
    assert(index < lights->n);
    assert(token->token == YF_TOKEN_OP);
    assert(token->data[0] == '[' || token->data[0] == ',');

    lights->v[index].color[0] = 1.0f;
    lights->v[index].color[1] = 1.0f;
    lights->v[index].color[2] = 1.0f;
    lights->v[index].intensity = 1.0f;
    lights->v[index].range = 0.0f;

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("type", token->data) == 0) {
                next_token(file, token); /* ':' */
                next_token(file, token);
                if (strcmp("directional", token->data) == 0) {
                    lights->v[index].type = YF_GLTF_LIGHT_DIRECT;
                } else if (strcmp("point", token->data) == 0) {
                    lights->v[index].type = YF_GLTF_LIGHT_POINT;
                } else if (strcmp("spot", token->data) == 0) {
                    lights->v[index].type = YF_GLTF_LIGHT_SPOT;
                } else {
                    yf_seterr(YF_ERR_INVFILE, __func__);
                    return -1;
                }

            } else if (strcmp("color", token->data) == 0) {
                next_token(file, token); /* ':' */
                next_token(file, token); /* '[' */
                for (size_t i = 0; i < 3; i++) {
                    if (parse_num(file, token, lights->v[index].color+i) != 0)
                        return -1;
                }
                next_token(file, token); /* ']' */

            } else if (strcmp("intensity", token->data) == 0) {
                if (parse_num(file, token, &lights->v[index].intensity) != 0)
                    return -1;

            } else if (strcmp("range", token->data) == 0) {
                if (parse_num(file, token, &lights->v[index].range) != 0)
                    return -1;

            } else if (strcmp("spot", token->data) == 0) {
                if (parse_spot(file, token, &lights->v[index].spot) != 0)
                    return -1;

            } else if (strcmp("name", token->data) == 0) {
                if (parse_str(file, token, &lights->v[index].name) != 0)
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

/* Parses the 'glTF.extensions.KHR_lights_punctual' property. */
static int parse_punctual(FILE *file, token_t *token, gltf_t *gltf)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(gltf != NULL);
    assert(token->token == YF_TOKEN_STR);
    assert(strcmp(token->data, "KHR_lights_punctual") == 0);

    next_token(file, token); /* ':' */
    next_token(file, token); /* '{' */

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("lights", token->data) == 0) {
                if (parse_array(file, token, (void **)&gltf->ext.lights.v,
                                &gltf->ext.lights.n, sizeof *gltf->ext.lights.v,
                                parse_lights, &gltf->ext.lights) != 0)
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

/* Parses the 'glTF.extensions' property. */
static int parse_extensions(FILE *file, token_t *token, gltf_t *gltf)
{
    assert(file != NULL && !feof(file));
    assert(token != NULL);
    assert(gltf != NULL);
    assert(token->token == YF_TOKEN_STR);
    assert(strcmp(token->data, "extensions") == 0);

    next_token(file, token); /* ':' */
    next_token(file, token); /* '{' */

    while (1) {
        switch (next_token(file, token)) {
        case YF_TOKEN_STR:
            if (strcmp("KHR_lights_punctual", token->data) == 0) {
                if (parse_punctual(file, token, gltf) != 0)
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
static int parse_gltf(FILE *file, token_t *token, gltf_t *gltf)
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
            if (strcmp("extensionsUsed", token->data) == 0) {
                if (parse_array(file, token, (void **)&gltf->ext_used,
                                &gltf->ext_used_n, sizeof *gltf->ext_used,
                                parse_str_array, &gltf->ext_used) != 0)
                    return -1;

            } else if (strcmp("extensionsRequired", token->data) == 0) {
                if (parse_array(file, token, (void **)&gltf->ext_req,
                                &gltf->ext_req_n, sizeof *gltf->ext_req,
                                parse_str_array, &gltf->ext_req) != 0)
                    return -1;

            } else if (strcmp("asset", token->data) == 0) {
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

            } else if (strcmp("extensions", token->data) == 0) {
                if (parse_extensions(file, token, gltf) != 0)
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

/* Data for binary buffer access. */
typedef struct {
    char *path;
    /* offset into the embedded binary buffer (0), when applicable */
    int_t off_e;
    /* open file(s), mapping to 'gltf.buffers' */
    union {
        FILE *file;
        FILE **files;
    };
} fdata_t;

/* Lists of contents mapping to a gltf object. */
typedef struct {
    yf_scene_t **scns;
    yf_node_t **nodes;
    yf_mesh_t **meshes;
    yf_skin_t **skins;
    yf_material_t **matls;
    yf_kfanim_t **anims;

    /* the texture list contain source image indices */
    int_t *texs;
    yf_texture_t **imgs;

    /* default sampler is used when not specified */
    yf_sampler_t dfl_splr;
    yf_sampler_t *splrs;
#define YF_NILSPLR(splr) ((splr).wrapmode.u < 0)

    /* flag indicating that the contents must be destroyed
       when not set, only the allocated lists are freed */
    int deinit;
} cont_t;

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

#if defined(YF_DEVEL) && defined(YF_PRINT)
static void print_gltf(const gltf_t *gltf);
#endif

/* Deinitializes glTF contents. */
static void deinit_gltf(gltf_t *gltf, fdata_t *fdata, cont_t *cont)
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
        if (cont->anims != NULL) {
            if (cont->deinit) {
                for (size_t i = 0; i < gltf->animations.n; i++)
                    yf_kfanim_deinit(cont->anims[i]);
            }
            free(cont->anims);
        }
        free(cont->texs);
        if (cont->imgs != NULL) {
            if (cont->deinit) {
                for (size_t i = 0; i < gltf->images.n; i++)
                    yf_texture_deinit(cont->imgs[i]);
            }
            free(cont->imgs);
        }
        free(cont->splrs);
    }

    for (size_t i = 0; i < gltf->ext_used_n; i++)
        free(gltf->ext_used[i]);
    free(gltf->ext_used);

    for (size_t i = 0; i < gltf->ext_req_n; i++)
        free(gltf->ext_req[i]);
    free(gltf->ext_req);

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

    for (size_t i = 0; i < gltf->materials.n; i++) {
        free(gltf->materials.v[i].name);
        free(gltf->materials.v[i].ext.pbrsg);
    }
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

    for (size_t i = 0; i < gltf->ext.lights.n; i++)
        free(gltf->ext.lights.v[i].name);
    free(gltf->ext.lights.v);
}

/* Initializes glTF contents. */
static int init_gltf(FILE *file, gltf_t *gltf, fdata_t *fdata, cont_t *cont)
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

    token_t token = {0};
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

    *cont = (cont_t){0};
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
    if (gltf->animations.n > 0) {
        cont->anims = calloc(gltf->animations.n, sizeof *cont->anims);
        if (cont->anims == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            deinit_gltf(gltf, fdata, cont);
            return -1;
        }
    }
    if (gltf->textures.n > 0) {
        if (gltf->images.n == 0) {
            yf_seterr(YF_ERR_INVFILE, __func__);
            deinit_gltf(gltf, fdata, cont);
            return -1;
        }
        cont->texs = malloc(gltf->textures.n * sizeof *cont->texs);
        cont->imgs = calloc(gltf->images.n, sizeof *cont->imgs);
        if (cont->texs == NULL || cont->imgs == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            deinit_gltf(gltf, fdata, cont);
            return -1;
        }
        for (size_t i = 0; i < gltf->textures.n; i++)
            cont->texs[i] = YF_INT_MIN;
    }
    if (gltf->samplers.n > 0) {
        cont->splrs = malloc(gltf->samplers.n * sizeof *cont->splrs);
        if (cont->splrs == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            deinit_gltf(gltf, fdata, cont);
            return -1;
        }
        for (size_t i = 0; i < gltf->samplers.n; i++)
            cont->splrs[i].wrapmode.u = -1;
    }

#if defined(YF_DEVEL) && defined(YF_PRINT)
    print_gltf(gltf);
#endif

    return 0;
}

/* Seeks into data buffer as specified by an accessor or buffer view. */
static FILE *seek_data(const gltf_t *gltf, fdata_t *fdata, int_t accessor,
                       int_t buffer_view)
{
    assert(gltf != NULL);
    assert(fdata != NULL);
    assert(accessor != YF_INT_MIN || buffer_view != YF_INT_MIN);

    int_t buf, off;
    if (accessor != YF_INT_MIN) {
        int_t view = gltf->accessors.v[accessor].buffer_view;
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
static int load_mesh(const gltf_t *gltf, fdata_t *fdata, cont_t *cont,
                     int_t mesh)
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
    if (cont->meshes[mesh] != NULL)
        return 0;

    const primitives_t *primitives = &gltf->meshes.v[mesh].primitives;
    if (primitives->n == 0) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }

    yf_meshdt_t data;
    data.prims = malloc(primitives->n * sizeof *data.prims);
    if (data.prims == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }
    data.prim_n = primitives->n;
    data.data = NULL;
    data.data_sz = 0;

#define YF_DEALLOCDT(prim_n) do { \
    for (unsigned prim_i = 0; prim_i < prim_n; prim_i++) \
        free(data.prims[prim_i].attrs); \
    free(data.prims); \
    free(data.data); } while (0)

    /* each of these correspond to a separate draw call */
    for (size_t i = 0; i < primitives->n; i++) {
        data.prims[i].data_off = data.data_sz;

        /* primitive mode */
        switch (primitives->v[i].mode) {
        case YF_GLTF_MODE_PTS:
            data.prims[i].topology = YF_TOPOLOGY_POINT;
            break;
        case YF_GLTF_MODE_LNS:
            data.prims[i].topology = YF_TOPOLOGY_LINE;
            break;
        case YF_GLTF_MODE_TRIS:
            data.prims[i].topology = YF_TOPOLOGY_TRIANGLE;
            break;
        case YF_GLTF_MODE_LNSTRIP:
            data.prims[i].topology = YF_TOPOLOGY_LNSTRIP;
            break;
        case YF_GLTF_MODE_TRISTRIP:
            data.prims[i].topology = YF_TOPOLOGY_TRISTRIP;
            break;
        case YF_GLTF_MODE_TRIFAN:
            data.prims[i].topology = YF_TOPOLOGY_TRIFAN;
            break;
        case YF_GLTF_MODE_LNLOOP:
            /* TODO */
            yf_seterr(YF_ERR_UNSUP, __func__);
            YF_DEALLOCDT(i);
            return -1;
        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            YF_DEALLOCDT(i);
            return -1;
        }

        /* vertex attributes */
        /* TODO: Endianness. */
        const int_t pos_acc = primitives->v[i].attributes[YF_GLTF_ATTR_POS];
        if (pos_acc == YF_INT_MIN) {
            yf_seterr(YF_ERR_UNSUP, __func__);
            YF_DEALLOCDT(i);
            return -1;
        }
        data.prims[i].vert_n = gltf->accessors.v[pos_acc].count;

        data.prims[i].vsemt_mask = 0;

        size_t attr_n = 0;
        for (size_t j = 0; j < YF_GLTF_ATTR_N; j++) {
            if (primitives->v[i].attributes[j] != YF_INT_MIN)
                attr_n++;
        }
        data.prims[i].attrs = malloc(attr_n * sizeof *data.prims[i].attrs);
        if (data.prims[i].attrs == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            YF_DEALLOCDT(i);
            return -1;
        }
        data.prims[i].attr_n = attr_n;

        for (size_t j = 0, k = 0; j < attr_n; k++) {
            const int_t attr_acc = primitives->v[i].attributes[k];
            if (attr_acc == YF_INT_MIN)
                continue;

            /* XXX */
            data.prims[i].attrs[j].vsemt = 1 << k;
            data.prims[i].vsemt_mask |= 1 << k;

            const int_t comp_type = gltf->accessors.v[attr_acc].comp_type;
            const int type = gltf->accessors.v[attr_acc].type;
            size_t comp_sz;
            size_t comp_n;

            /* need 'VFMT' value and component's size/count */
            switch (comp_type) {
            case YF_GLTF_COMP_BYTE:
                comp_sz = 1;
                switch (type) {
                case YF_GLTF_TYPE_SCALAR:
                    comp_n = 1;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_BYTE;
                    break;
                case YF_GLTF_TYPE_VEC2:
                    comp_n = 2;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_BYTE2;
                    break;
                case YF_GLTF_TYPE_VEC3:
                    comp_n = 3;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_BYTE3;
                    break;
                case YF_GLTF_TYPE_VEC4:
                    comp_n = 4;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_BYTE4;
                    break;
                case YF_GLTF_TYPE_MAT2:
                case YF_GLTF_TYPE_MAT3:
                case YF_GLTF_TYPE_MAT4:
                    /* TODO: Support for this types on core. */
                    yf_seterr(YF_ERR_UNSUP, __func__);
                    YF_DEALLOCDT(i + 1);
                    return -1;
                default:
                    yf_seterr(YF_ERR_INVFILE, __func__);
                    YF_DEALLOCDT(i + 1);
                    return -1;
                }
                break;

            case YF_GLTF_COMP_UBYTE:
                comp_sz = 1;
                switch (type) {
                case YF_GLTF_TYPE_SCALAR:
                    comp_n = 1;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_UBYTE;
                    break;
                case YF_GLTF_TYPE_VEC2:
                    comp_n = 2;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_UBYTE2;
                    break;
                case YF_GLTF_TYPE_VEC3:
                    comp_n = 3;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_UBYTE3;
                    break;
                case YF_GLTF_TYPE_VEC4:
                    comp_n = 4;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_UBYTE4;
                    break;
                case YF_GLTF_TYPE_MAT2:
                case YF_GLTF_TYPE_MAT3:
                case YF_GLTF_TYPE_MAT4:
                    /* TODO: Support for this types on core. */
                    yf_seterr(YF_ERR_UNSUP, __func__);
                    YF_DEALLOCDT(i + 1);
                    return -1;
                default:
                    yf_seterr(YF_ERR_INVFILE, __func__);
                    YF_DEALLOCDT(i + 1);
                    return -1;
                }
                break;

            case YF_GLTF_COMP_SHORT:
                comp_sz = 2;
                switch (type) {
                case YF_GLTF_TYPE_SCALAR:
                    comp_n = 1;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_SHORT;
                    break;
                case YF_GLTF_TYPE_VEC2:
                    comp_n = 2;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_SHORT2;
                    break;
                case YF_GLTF_TYPE_VEC3:
                    comp_n = 3;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_SHORT3;
                    break;
                case YF_GLTF_TYPE_VEC4:
                    comp_n = 4;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_SHORT4;
                    break;
                case YF_GLTF_TYPE_MAT2:
                case YF_GLTF_TYPE_MAT3:
                case YF_GLTF_TYPE_MAT4:
                    /* TODO: Support for this types on core. */
                    yf_seterr(YF_ERR_UNSUP, __func__);
                    YF_DEALLOCDT(i + 1);
                    return -1;
                default:
                    yf_seterr(YF_ERR_INVFILE, __func__);
                    YF_DEALLOCDT(i + 1);
                    return -1;
                }
                break;

            case YF_GLTF_COMP_USHORT:
                comp_sz = 2;
                switch (type) {
                case YF_GLTF_TYPE_SCALAR:
                    comp_n = 1;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_USHORT;
                    break;
                case YF_GLTF_TYPE_VEC2:
                    comp_n = 2;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_USHORT2;
                    break;
                case YF_GLTF_TYPE_VEC3:
                    comp_n = 3;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_USHORT3;
                    break;
                case YF_GLTF_TYPE_VEC4:
                    comp_n = 4;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_USHORT4;
                    break;
                case YF_GLTF_TYPE_MAT2:
                case YF_GLTF_TYPE_MAT3:
                case YF_GLTF_TYPE_MAT4:
                    /* TODO: Support for this types on core. */
                    yf_seterr(YF_ERR_UNSUP, __func__);
                    YF_DEALLOCDT(i + 1);
                    return -1;
                default:
                    yf_seterr(YF_ERR_INVFILE, __func__);
                    YF_DEALLOCDT(i + 1);
                    return -1;
                }
                break;

            case YF_GLTF_COMP_UINT:
                comp_sz = 4;
                switch (type) {
                case YF_GLTF_TYPE_SCALAR:
                    comp_n = 1;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_UINT;
                    break;
                case YF_GLTF_TYPE_VEC2:
                    comp_n = 2;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_UINT2;
                    break;
                case YF_GLTF_TYPE_VEC3:
                    comp_n = 3;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_UINT3;
                    break;
                case YF_GLTF_TYPE_VEC4:
                    comp_n = 4;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_UINT4;
                    break;
                case YF_GLTF_TYPE_MAT2:
                case YF_GLTF_TYPE_MAT3:
                case YF_GLTF_TYPE_MAT4:
                    /* TODO: Support for this types on core. */
                    yf_seterr(YF_ERR_UNSUP, __func__);
                    YF_DEALLOCDT(i + 1);
                    return -1;
                default:
                    yf_seterr(YF_ERR_INVFILE, __func__);
                    YF_DEALLOCDT(i + 1);
                    return -1;
                }
                break;

            case YF_GLTF_COMP_FLOAT:
                comp_sz = 4;
                switch (type) {
                case YF_GLTF_TYPE_SCALAR:
                    comp_n = 1;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_FLOAT;
                    break;
                case YF_GLTF_TYPE_VEC2:
                    comp_n = 2;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_FLOAT2;
                    break;
                case YF_GLTF_TYPE_VEC3:
                    comp_n = 3;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_FLOAT3;
                    break;
                case YF_GLTF_TYPE_VEC4:
                    comp_n = 4;
                    data.prims[i].attrs[j].vfmt = YF_VFMT_FLOAT4;
                    break;
                case YF_GLTF_TYPE_MAT2:
                case YF_GLTF_TYPE_MAT3:
                case YF_GLTF_TYPE_MAT4:
                    /* TODO: Support for this types on core. */
                    yf_seterr(YF_ERR_UNSUP, __func__);
                    YF_DEALLOCDT(i + 1);
                    return -1;
                default:
                    yf_seterr(YF_ERR_INVFILE, __func__);
                    YF_DEALLOCDT(i + 1);
                    return -1;
                }
                break;

            default:
                yf_seterr(YF_ERR_INVFILE, __func__);
                YF_DEALLOCDT(i + 1);
                return -1;
            }

            data.prims[i].attrs[j].data_off =
                data.data_sz - data.prims[i].data_off;

            const size_t attr_sz = comp_sz * comp_n;
            const size_t copy_sz = data.prims[i].vert_n * attr_sz;
            size_t new_sz = data.data_sz + copy_sz;
            if (new_sz & 3)
                new_sz += 4 - (new_sz & 3);

            unsigned char *dt = realloc(data.data, new_sz);
            if (dt == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                YF_DEALLOCDT(i + 1);
                return -1;
            }
            data.data = dt;
            data.data_sz = new_sz;

            FILE *file = seek_data(gltf, fdata, attr_acc, YF_INT_MIN);
            if (file == NULL) {
                YF_DEALLOCDT(i + 1);
                return -1;
            }

            const int_t attr_view = gltf->accessors.v[attr_acc].buffer_view;
            const int_t byte_strd = gltf->bufferviews.v[attr_view].byte_strd;

            dt += data.prims[i].data_off + data.prims[i].attrs[j].data_off;
            if (byte_strd == 0) {
                /* tightly packed */
                if (fread(dt, 1, copy_sz, file) != copy_sz) {
                    yf_seterr(YF_ERR_INVFILE, __func__);
                    YF_DEALLOCDT(i + 1);
                    return -1;
                }
            } else {
                /* interleaved */
                const size_t strd = byte_strd - attr_sz;
                size_t n = 0;
                while (1) {
                    if (fread(dt, 1, attr_sz, file) != attr_sz) {
                        yf_seterr(YF_ERR_INVFILE, __func__);
                        YF_DEALLOCDT(i + 1);
                        return -1;
                    }
                    if (++n == data.prims[i].vert_n)
                        break;
                    if (fseek(file, strd, SEEK_CUR) != 0) {
                        yf_seterr(YF_ERR_INVFILE, __func__);
                        return -1;
                    }
                    dt += attr_sz;
                }
            }

            j++;
        }

        /* vertex indices */
        /* TODO: Endianness. */
        const int_t indx_acc = primitives->v[i].indices;
        if (indx_acc == YF_INT_MIN) {
            data.prims[i].indx_n = 0;
            data.prims[i].itype = 0;
            data.prims[i].indx_data_off = 0;
        } else {
            data.prims[i].indx_n = gltf->accessors.v[indx_acc].count;

            if (gltf->accessors.v[indx_acc].type != YF_GLTF_TYPE_SCALAR) {
                yf_seterr(YF_ERR_UNSUP, __func__);
                YF_DEALLOCDT(i + 1);
                return -1;
            }

            size_t comp_sz;
            switch (gltf->accessors.v[indx_acc].comp_type) {
            case YF_GLTF_COMP_USHORT:
                data.prims[i].itype = YF_ITYPE_USHORT;
                comp_sz = 2;
                break;
            case YF_GLTF_COMP_UINT:
                data.prims[i].itype = YF_ITYPE_UINT;
                comp_sz = 4;
                break;
            default:
                yf_seterr(YF_ERR_UNSUP, __func__);
                YF_DEALLOCDT(i + 1);
                return -1;
            }

            data.prims[i].indx_data_off =
                data.data_sz - data.prims[i].data_off;

            const size_t copy_sz = data.prims[i].indx_n * comp_sz;
            size_t new_sz = data.data_sz + copy_sz;
            if (new_sz & 3)
                new_sz += 4 - (new_sz & 3);

            unsigned char *dt = realloc(data.data, new_sz);
            if (dt == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                YF_DEALLOCDT(i + 1);
                return -1;
            }
            data.data = dt;
            data.data_sz = new_sz;

            FILE *file = seek_data(gltf, fdata, indx_acc, YF_INT_MIN);
            if (file == NULL) {
                YF_DEALLOCDT(i + 1);
                return -1;
            }

            dt += data.prims[i].data_off + data.prims[i].indx_data_off;
            if (fread(dt, 1, copy_sz, file) != copy_sz) {
                yf_seterr(YF_ERR_INVFILE, __func__);
                YF_DEALLOCDT(i + 1);
                return -1;
            }
        }

        /* XXX: Caller must explicitly load/set the material. */
        data.prims[i].matl = NULL;
    }

    cont->meshes[mesh] = yf_mesh_init(&data);
    YF_DEALLOCDT(data.prim_n);
    return cont->meshes[mesh] == NULL ? -1 : 0;

#undef YF_DEALLOCDT
}

/* Loads a single sampler from glTF contents. */
static int load_sampler(const gltf_t *gltf, cont_t *cont, int_t sampler)
{
    assert(gltf != NULL);
    assert(cont != NULL);
    assert(sampler >= 0);

    if (gltf->samplers.n <= (size_t)sampler) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    assert(cont->splrs != NULL);
    if (!YF_NILSPLR(cont->splrs[sampler]))
        return 0;

    /* wrap modes */
    struct {
        const int_t from;
        int *const to;
    } wrap[2] = {
        {gltf->samplers.v[sampler].wrap_s, &cont->splrs[sampler].wrapmode.u},
        {gltf->samplers.v[sampler].wrap_t, &cont->splrs[sampler].wrapmode.v}
    };

    for (unsigned i = 0; i < 2; i++) {
        switch (wrap[i].from) {
        case YF_GLTF_WRAP_CLAMP:
            *wrap[i].to = YF_WRAPMODE_CLAMP;
            break;
        case YF_GLTF_WRAP_MIRROR:
            *wrap[i].to = YF_WRAPMODE_MIRROR;
            break;
        case YF_GLTF_WRAP_REPEAT:
        default:
            *wrap[i].to = YF_WRAPMODE_REPEAT;
            break;
        }
    }

    /* XXX: Currently unused. */
    cont->splrs[sampler].wrapmode.w = YF_WRAPMODE_REPEAT;

    /* filters */
    switch (gltf->samplers.v[sampler].mag_filter) {
    case YF_GLTF_FILTER_LINEAR:
        cont->splrs[sampler].filter.mag = YF_FILTER_LINEAR;
        break;
    default:
        cont->splrs[sampler].filter.mag = YF_FILTER_NEAREST;
        break;
    }

    switch (gltf->samplers.v[sampler].min_filter) {
    case YF_GLTF_FILTER_LINEAR:
    case YF_GLTF_FILTER_LNMIPLN:
        cont->splrs[sampler].filter.min = YF_FILTER_LINEAR;
        cont->splrs[sampler].filter.mipmap = YF_FILTER_LINEAR;
        break;
    case YF_GLTF_FILTER_LNMIPNR:
        cont->splrs[sampler].filter.min = YF_FILTER_LINEAR;
        cont->splrs[sampler].filter.mipmap = YF_FILTER_NEAREST;
        break;
    case YF_GLTF_FILTER_NRMIPLN:
        cont->splrs[sampler].filter.min = YF_FILTER_NEAREST;
        cont->splrs[sampler].filter.mipmap = YF_FILTER_LINEAR;
        break;
    default:
        cont->splrs[sampler].filter.min = YF_FILTER_NEAREST;
        cont->splrs[sampler].filter.mipmap = YF_FILTER_NEAREST;
        break;
    }

    return 0;
}

/* Loads a single texture from glTF contents. */
static int load_texture(const gltf_t *gltf, fdata_t *fdata, cont_t *cont,
                        int_t texture)
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
    if (cont->texs[texture] != YF_INT_MIN)
        return 0;

    assert(cont->imgs != NULL);
    const int_t image = gltf->textures.v[texture].source;
    if (cont->imgs[image] != NULL) {
        /* XXX: Texture samplers might differ. */
        cont->texs[texture] = image;
        return 0;
    }

    if (gltf->images.v[image].mime_type != NULL &&
        strcmp(gltf->images.v[image].mime_type, "image/png") != 0) {
        yf_seterr(YF_ERR_UNSUP, __func__);
        return -1;
    }

    const int_t view = gltf->images.v[image].buffer_view;
    yf_texdt_t data;
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

    const int_t sampler = gltf->textures.v[texture].sampler;
    if (sampler == YF_INT_MIN) {
        data.splr = cont->dfl_splr;
    } else {
        if (load_sampler(gltf, cont, sampler) != 0)
            return -1;
        data.splr = cont->splrs[sampler];
    }

    /* XXX: Default 'UVSET' value. */
    data.uvset = YF_UVSET_0;

    cont->imgs[image] = yf_texture_init(&data);
    free(data.data);
    if (cont->imgs[image] == NULL)
        return -1;

    cont->texs[texture] = image;
    return 0;
}

/* Loads a single skin from glTF contents. */
static int load_skin(const gltf_t *gltf, fdata_t *fdata, cont_t *cont,
                     int_t skin)
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
    if (cont->skins[skin] != NULL)
        return 0;

    const size_t jnt_n = gltf->skins.v[skin].joint_n;
    assert(jnt_n > 0);

    int_t *jnt_hier = calloc(gltf->nodes.n, sizeof *jnt_hier);
    yf_joint_t *jnts = malloc(jnt_n * sizeof *jnts);
    if (jnt_hier == NULL || jnts == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        free(jnt_hier);
        free(jnts);
        return -1;
    }

    for (size_t i = 0; i < jnt_n; i++) {
        const int_t node = gltf->skins.v[skin].joints[i];

        /* joint transform */
        const unsigned mask = gltf->nodes.v[node].xform_mask;
        if (mask != YF_GLTF_XFORM_NONE) {
            if (mask & YF_GLTF_XFORM_M) {
                yf_mat4_copy(jnts[i].xform, gltf->nodes.v[node].matrix);
            } else {
                if (mask & YF_GLTF_XFORM_T) {
                    const num_t *t = gltf->nodes.v[node].trs.t;
                    yf_mat4_xlate(jnts[i].xform, t[0], t[1], t[2]);
                } else {
                    yf_mat4_iden(jnts[i].xform);
                }
                if (mask & YF_GLTF_XFORM_R) {
                    const num_t *r = gltf->nodes.v[node].trs.r;
                    yf_mat4_t mr, rot;
                    yf_mat4_rotq(mr, r);
                    yf_mat4_mul(rot, jnts[i].xform, mr);
                    yf_mat4_copy(jnts[i].xform, rot);
                }
                if (mask & YF_GLTF_XFORM_S) {
                    const num_t *s = gltf->nodes.v[node].trs.s;
                    yf_mat4_t ms, scl;
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
        const int_t node = gltf->skins.v[skin].joints[i];
        /* negative 'pnt_i' means no parent */
        jnts[i].pnt_i = jnt_hier[node] - 1;
    }

    /* ibm data */
    /* TODO: Endianness. */
    const int_t acc = gltf->skins.v[skin].inv_bind_matrices;
    if (acc != YF_INT_MIN) {
        const int_t view = gltf->accessors.v[acc].buffer_view;
        const int_t buf = gltf->bufferviews.v[view].buffer;
        const size_t off = gltf->accessors.v[acc].byte_off +
            gltf->bufferviews.v[view].byte_off;

        assert(gltf->accessors.v[acc].count == (int_t)jnt_n);
        assert(gltf->accessors.v[acc].comp_type == YF_GLTF_COMP_FLOAT);
        assert(gltf->accessors.v[acc].type == YF_GLTF_TYPE_MAT4);
        assert(gltf->buffers.v[buf].byte_len - off >= sizeof(yf_mat4_t));

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
static int load_material(const gltf_t *gltf, fdata_t *fdata, cont_t *cont,
                         int_t material)
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
    if (cont->matls[material] != NULL)
        return 0;

    yf_matlprop_t prop = {0};
    const textureinfo_t *infos[5];
    yf_texref_t *refs[5];

    if (!gltf->materials.v[material].ext.unlit) {
        if (gltf->materials.v[material].ext.pbrsg != NULL) {
            /* spec-gloss (extension) */
            const pbrspecgloss_t *pbrsg = gltf->materials.v[material].ext.pbrsg;

            prop.pbr = YF_PBR_SPECGLOSS;
            memcpy(prop.pbrsg.diffuse_fac, pbrsg->diffuse_fac,
                   sizeof prop.pbrsg.diffuse_fac);
            memcpy(prop.pbrsg.specular_fac, pbrsg->specular_fac,
                   sizeof prop.pbrsg.specular_fac);
            prop.pbrsg.glossiness_fac = pbrsg->glossiness_fac;

            infos[0] = &pbrsg->diffuse_tex;
            refs[0] = &prop.pbrsg.diffuse_tex;
            infos[1] = &pbrsg->spec_gloss_tex;
            refs[1] = &prop.pbrsg.spec_gloss_tex;

        } else {
            /* metal-rough */
            const pbrmetalrough_t *pbrmr = &gltf->materials.v[material].pbrmr;

            prop.pbr = YF_PBR_METALROUGH;
            memcpy(prop.pbrmr.color_fac, pbrmr->base_clr_fac,
                   sizeof prop.pbrmr.color_fac);
            prop.pbrmr.metallic_fac = pbrmr->metallic_fac;
            prop.pbrmr.roughness_fac = pbrmr->roughness_fac;

            infos[0] = &pbrmr->base_clr_tex;
            refs[0] = &prop.pbrmr.color_tex;
            infos[1] = &pbrmr->metal_rough_tex;
            refs[1] = &prop.pbrmr.metal_rough_tex;
        }

        prop.normal.scale = gltf->materials.v[material].normal_tex.scale;

        prop.occlusion.strength =
            gltf->materials.v[material].occlusion_tex.strength;

        memcpy(prop.emissive.factor, gltf->materials.v[material].emissive_fac,
               sizeof prop.emissive.factor);

        infos[2] = &gltf->materials.v[material].normal_tex;
        refs[2] = &prop.normal.tex;
        infos[3] = &gltf->materials.v[material].occlusion_tex;
        refs[3] = &prop.occlusion.tex;
        infos[4] = &gltf->materials.v[material].emissive_tex;
        refs[4] = &prop.emissive.tex;

    } else {
        /* unlit (extension) */
        prop.pbr = YF_PBR_NONE;
        memcpy(prop.nopbr.color_fac,
               gltf->materials.v[material].pbrmr.base_clr_fac,
               sizeof prop.nopbr.color_fac);

        infos[0] = &gltf->materials.v[material].pbrmr.base_clr_tex;
        refs[0] = &prop.nopbr.color_tex;
        infos[1] = NULL;
        refs[1] = NULL;

        prop.normal.scale = 1.0f;
        prop.occlusion.strength = 1.0f;

        infos[2] = NULL;
        refs[2] = NULL;
        infos[3] = NULL;
        refs[3] = NULL;
        infos[4] = NULL;
        refs[4] = NULL;
    }

    switch (gltf->materials.v[material].alpha_mode) {
    case YF_GLTF_ALPHA_OPAQUE:
        prop.alphamode = YF_ALPHAMODE_OPAQUE;
        break;
    case YF_GLTF_ALPHA_MASK:
        prop.alphamode = YF_ALPHAMODE_MASK;
        prop.alpha_cutoff = gltf->materials.v[material].alpha_cutoff;
        break;
    case YF_GLTF_ALPHA_BLEND:
        prop.alphamode = YF_ALPHAMODE_BLEND;
        break;
    default:
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
    }

    prop.double_sided = gltf->materials.v[material].double_sided;

    for (size_t i = 0; i < (sizeof infos / sizeof *infos); i++) {
        if (infos[i] == NULL || infos[i]->index == YF_INT_MIN)
            continue;

        const int_t texture = infos[i]->index;
        if (load_texture(gltf, fdata, cont, texture) != 0)
            return -1;
        refs[i]->tex = cont->imgs[cont->texs[texture]];

        const int_t sampler = gltf->textures.v[texture].sampler;
        if (sampler == YF_INT_MIN) {
            refs[i]->splr = cont->dfl_splr;
        } else {
            if (load_sampler(gltf, cont, sampler) != 0)
                return -1;
            refs[i]->splr = cont->splrs[sampler];
        }

        switch (infos[i]->tex_coord) {
        case 0:
            refs[i]->uvset = YF_UVSET_0;
            break;
        case 1:
            refs[i]->uvset = YF_UVSET_1;
            break;
        default:
            yf_seterr(YF_ERR_UNSUP, __func__);
            return -1;
        }
    }

    cont->matls[material] = yf_material_init(&prop);
    return cont->matls[material] == NULL ? -1 : 0;
}

static int load_skeleton(const gltf_t *, fdata_t *, cont_t *, int_t);

/* Loads a single node from glTF contents. */
static int load_node(const gltf_t *gltf, fdata_t *fdata, cont_t *cont,
                     int_t node)
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
    if (cont->nodes[node] != NULL)
        return 0;

    /* node object */
    const int_t mesh = gltf->nodes.v[node].mesh;
    const int_t punctual = gltf->nodes.v[node].ext.light;
    if (mesh != YF_INT_MIN) {
        /* model */
        yf_model_t *mdl = yf_model_init();
        if (mdl == NULL)
            return -1;

        cont->nodes[node] = yf_model_getnode(mdl);

        if (load_mesh(gltf, fdata, cont, mesh) != 0) {
            yf_model_deinit(mdl);
            cont->nodes[node] = NULL;
            return -1;
        }
        yf_model_setmesh(mdl, cont->meshes[mesh]);

        const primitives_t *primitives = &gltf->meshes.v[mesh].primitives;
        for (size_t i = 0; i < primitives->n; i++) {
            const int_t material = primitives->v[i].material;
            if (material != YF_INT_MIN) {
                if (load_material(gltf, fdata, cont, material) != 0) {
                    yf_model_deinit(mdl);
                    cont->nodes[node] = NULL;
                    return -1;
                }
                yf_mesh_setmatl(cont->meshes[mesh], i, cont->matls[material]);
            }
        }

        const int_t skin = gltf->nodes.v[node].skin;
        if (skin != YF_INT_MIN) {
            yf_skeleton_t *skel = NULL;
            if (load_skeleton(gltf, fdata, cont, skin) != 0) {
                yf_model_deinit(mdl);
                cont->nodes[node] = NULL;
                return -1;
            }
            skel = yf_skin_newest(cont->skins[skin]);
            yf_model_setskin(mdl, cont->skins[skin], skel);
        }

    } else if (punctual != YF_INT_MIN) {
        /* light */
        int lightt;
        switch (gltf->ext.lights.v[punctual].type) {
        case YF_GLTF_LIGHT_POINT:
            lightt = YF_LIGHTT_POINT;
            break;
        case YF_GLTF_LIGHT_SPOT:
            lightt = YF_LIGHTT_SPOT;
            break;
        case YF_GLTF_LIGHT_DIRECT:
            lightt = YF_LIGHTT_DIRECT;
            break;
        default:
            yf_seterr(YF_ERR_INVFILE, __func__);
            return -1;
        }
        const float *color = gltf->ext.lights.v[punctual].color;
        float intensity = gltf->ext.lights.v[punctual].intensity;
        float range  = gltf->ext.lights.v[punctual].range;
        float inner_angle = gltf->ext.lights.v[punctual].spot.inner_cone_angle;
        float outer_angle = gltf->ext.lights.v[punctual].spot.outer_cone_angle;

        yf_light_t *light = yf_light_init(lightt, color, intensity, range,
                                          inner_angle, outer_angle);
        if (light == NULL)
            return -1;

        cont->nodes[node] = yf_light_getnode(light);

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
            yf_mat4_t *m = yf_node_getxform(cont->nodes[node]);
            if (mask & YF_GLTF_XFORM_T) {
                const num_t *t = gltf->nodes.v[node].trs.t;
                yf_mat4_xlate(*m, t[0], t[1], t[2]);
            }
            if (mask & YF_GLTF_XFORM_R) {
                const num_t *r = gltf->nodes.v[node].trs.r;
                yf_mat4_t mr, rot;
                yf_mat4_rotq(rot, r);
                yf_mat4_mul(mr, *m, rot);
                yf_mat4_copy(*m, mr);
            }
            if (mask & YF_GLTF_XFORM_S) {
                const num_t *s = gltf->nodes.v[node].trs.s;
                yf_mat4_t ms, scl;
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

/* Loads a single skeleton from glTF contents. */
static int load_skeleton(const gltf_t *gltf, fdata_t *fdata, cont_t *cont,
                         int_t skin)
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
    if (cont->skins[skin] == NULL) {
        if (load_skin(gltf, fdata, cont, skin) != 0)
            return -1;
    } else {
        if (yf_skin_newest(cont->skins[skin]) != NULL)
            /* skeleton already instantiated */
            return 0;
    }

    unsigned jnt_n;
    const yf_joint_t *jnts = yf_skin_getjnts(cont->skins[skin], &jnt_n);

    union { int_t i, *is; } unparented;
    unsigned unparented_n = 0;

    for (size_t i = 0; i < jnt_n; i++) {
        if (jnts[i].pnt_i >= 0)
            continue;

        /* XXX: 'yf_joint_t' array matches 'gltf.skins.v[].joints'. */
        switch (unparented_n) {
        case 0:
            unparented.i = gltf->skins.v[skin].joints[i];
            break;

        case 1: {
            int_t *tmp = malloc(jnt_n * sizeof *tmp);
            if (tmp == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                return -1;
            }
            tmp[0] = unparented.i;
            tmp[1] = gltf->skins.v[skin].joints[i];
            unparented.is = tmp;
        } break;

        default:
            unparented.is[unparented_n] = gltf->skins.v[skin].joints[i];
        }

        unparented_n++;
    }

    assert(unparented_n != 0);

    int_t root = YF_INT_MIN;
    if (unparented_n > 1) {
        /* need to find common root of unparented joints */
        int_t *hier = calloc(gltf->nodes.n, sizeof *hier);
        if (hier == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            free(unparented.is);
            return -1;
        }

        /* get ancestors */
        for (size_t i = 0; i < gltf->nodes.n; i++) {
            for (size_t j = 0; j < gltf->nodes.v[i].child_n; j++)
                /* zero means no parent */
                hier[gltf->nodes.v[i].children[j]] = i + 1;
        }

        /* go up the hierarchy chain to find the common root */
        for (int_t i = hier[unparented.is[0]] - 1; i >= 0; i = hier[i] - 1) {
            unsigned count = 1;
            for (size_t j = 1; j < unparented_n; j++) {
                for (int_t k = hier[unparented.is[j]] - 1; k >= 0;
                     k = hier[k] - 1) {
                    if (k == i) {
                        count++;
                        break;
                    }
                }
            }
            if (count == unparented_n) {
                root = i;
                break;
            }
        }
        free(unparented.is);
        free(hier);
        assert(root != YF_INT_MIN);

    } else {
        /* need to find unparented joint's parent, if it exists */
        for (size_t i = 0; i < gltf->nodes.n; i++) {
            for (size_t j = 0; j < gltf->nodes.v[i].child_n; j++) {
                if (gltf->nodes.v[i].children[j] == unparented.i) {
                    root = i;
                    i = gltf->nodes.n;
                    break;
                }
            }
        }
        if (root == YF_INT_MIN)
            /* skeleton root is the joint itself */
            root = unparented.i;
    }

    /* XXX: The skeleton instantiated here is unmanaged. Its nodes must be
       added to a collection since 'skin_unmkskel()' will not touch them. */
    yf_node_t **nodes = malloc((jnt_n + 1) * sizeof *nodes);
    if (nodes == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
    }

    for (size_t i = 0; i < jnt_n; i++) {
        const int_t joint = gltf->skins.v[skin].joints[i];
        if (load_node(gltf, fdata, cont, joint) != 0) {
            free(nodes);
            return -1;
        }
        nodes[i] = cont->nodes[joint];
    }
    if (load_node(gltf, fdata, cont, root) != 0) {
        free(nodes);
        return -1;
    }
    nodes[jnt_n] = cont->nodes[root];

    for (size_t i = 0; i < jnt_n; i++) {
        if (jnts[i].pnt_i < 0)
            yf_node_insert(nodes[jnt_n], nodes[i]);
        else
            yf_node_insert(nodes[jnts[i].pnt_i], nodes[i]);
    }

    yf_skeleton_t *skel = yf_skin_makeskel(cont->skins[skin], nodes);
    free(nodes);
    return skel == NULL ? -1 : 0;
}

/* Loads a single animation from glTF contents. */
static int load_animation(const gltf_t *gltf, fdata_t *fdata, cont_t *cont,
                          int_t animation)
{
    assert(gltf != NULL);
    assert(fdata != NULL);
    assert(cont != NULL);
    assert(animation >= 0);

    if (gltf->animations.n <= (size_t)animation) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    assert(cont->anims != NULL);
    if (cont->anims[animation] != NULL)
        return 0;

    const channels_t *channels = &gltf->animations.v[animation].channels;
    const size_t channel_n = gltf->animations.v[animation].channels.n;
    const asamplers_t *samplers = &gltf->animations.v[animation].samplers;
    const size_t sampler_n = gltf->animations.v[animation].samplers.n;

    yf_kfin_t *ins = malloc(sampler_n * sizeof *ins);
    yf_kfout_t *outs = malloc(sampler_n * sizeof *outs);
    yf_kfact_t *acts = malloc(channel_n * sizeof *acts);
    if (ins == NULL || outs == NULL || acts == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        free(ins);
        free(outs);
        free(acts);
        return -1;
    }
    unsigned in_n = 0;
    unsigned out_n = 0;

#define YF_DEALLOCKF() do { \
    if (ins != NULL) { \
        for (unsigned i = 0; i < in_n; i++) \
            free(ins[i].timeline); \
        free(ins); \
    } \
    if (outs != NULL) { \
        for (unsigned i = 0; i < out_n; i++) { \
            switch (outs[i].kfprop) { \
            case YF_KFPROP_T: \
                free(outs[i].t); \
                break; \
            case YF_KFPROP_R: \
                free(outs[i].r); \
                break; \
            case YF_KFPROP_S: \
                free(outs[i].s); \
                break; \
            } \
        } \
        free(outs); \
    } \
    free(acts); } while (0)

    /* mapping between sampler indices and input/output indices */
    struct { unsigned in, out; } *s_map = malloc(sampler_n * sizeof *s_map);
    if (s_map == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        free(ins);
        free(outs);
        free(acts);
        return -1;
    }

    for (size_t i = 0; i < sampler_n; i++) {
        int_t input = samplers->v[i].input;
        int_t output = samplers->v[i].output;

        for (size_t j = 0; j < i; j++) {
            if (input == samplers->v[j].input) {
                input = YF_INT_MIN;
                s_map[i].in = s_map[j].in;
                break;
            }
        }
        for (size_t j = 0; j < i; j++) {
            if (output == samplers->v[j].output) {
                output = YF_INT_MIN;
                s_map[i].out = s_map[j].out;
                break;
            }
        }

        /* create new input only when necessary */
        /* TODO: Endianness. */
        if (input != YF_INT_MIN) {
            assert(gltf->accessors.v[input].comp_type == YF_GLTF_COMP_FLOAT);
            assert(gltf->accessors.v[input].type == YF_GLTF_TYPE_SCALAR);

            FILE *file = seek_data(gltf, fdata, input, YF_INT_MIN);
            if (file == NULL) {
                YF_DEALLOCKF();
                free(s_map);
                return -1;
            }

            ins[in_n].n = gltf->accessors.v[input].count;
            ins[in_n].timeline = malloc(ins[in_n].n * sizeof(float));
            if (ins[in_n].timeline == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                YF_DEALLOCKF();
                free(s_map);
                return -1;
            }

            if (fread(ins[in_n].timeline, sizeof(float),
                      ins[in_n].n, file) < ins[in_n].n) {
                yf_seterr(YF_ERR_INVFILE, __func__);
                YF_DEALLOCKF();
                /* XXX: 'in_n' not incremented yet. */
                free(ins[in_n].timeline);
                free(s_map);
                return -1;
            }

            s_map[i].in = in_n++;
        }

        /* create new output only when necessary */
        /* TODO: Endianness. */
        if (output != YF_INT_MIN) {
            for (size_t j = 0; j < channel_n; j++) {
                if (channels->v[j].sampler != (int_t)i)
                    continue;
                switch (channels->v[j].target.path) {
                case YF_GLTF_PATH_XLATE:
                    outs[out_n].kfprop = YF_KFPROP_T;
                    break;
                case YF_GLTF_PATH_ROTATE:
                    outs[out_n].kfprop = YF_KFPROP_R;
                    break;
                case YF_GLTF_PATH_SCALE:
                    outs[out_n].kfprop = YF_KFPROP_S;
                    break;
                default:
                    yf_seterr(YF_ERR_UNSUP, __func__);
                    YF_DEALLOCKF();
                    free(s_map);
                    return -1;
                }
                break;
            }

            size_t elem_sz;
            switch (gltf->accessors.v[output].comp_type) {
            case YF_GLTF_COMP_FLOAT:
                elem_sz = sizeof(float);
                break;
            default:
                yf_seterr(YF_ERR_UNSUP, __func__);
                YF_DEALLOCKF();
                free(s_map);
                return -1;
            }
            switch (gltf->accessors.v[output].type) {
            case YF_GLTF_TYPE_VEC3:
                elem_sz *= 3;
                break;
            case YF_GLTF_TYPE_VEC4:
                elem_sz *= 4;
                break;
            default:
                yf_seterr(YF_ERR_UNSUP, __func__);
                YF_DEALLOCKF();
                free(s_map);
                return -1;
            }

            FILE *file = seek_data(gltf, fdata, output, YF_INT_MIN);
            if (file == NULL) {
                YF_DEALLOCKF();
                free(s_map);
                return -1;
            }

            outs[out_n].n = gltf->accessors.v[output].count;
            void *data = malloc(elem_sz * outs[out_n].n);
            if (data == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                YF_DEALLOCKF();
                free(s_map);
                return -1;
            }

            if (fread(data, elem_sz, outs[out_n].n, file) < outs[out_n].n) {
                yf_seterr(YF_ERR_INVFILE, __func__);
                YF_DEALLOCKF();
                free(s_map);
                free(data);
                return -1;
            }

            switch (outs[out_n].kfprop) {
            case YF_KFPROP_T:
                outs[out_n].t = data;
                break;
            case YF_KFPROP_R:
                outs[out_n].r = data;
                break;
            case YF_KFPROP_S:
                outs[out_n].s = data;
                break;
            }

            s_map[i].out = out_n++;
        }
    }

    /* each channel corresponds to a keyframe act */
    for (size_t i = 0; i < channel_n; i++) {
        const int_t sampler = channels->v[i].sampler;
        switch (samplers->v[sampler].interpolation) {
        case YF_GLTF_ERP_LINEAR:
            acts[i].kferp = YF_KFERP_LINEAR;
            break;
        case YF_GLTF_ERP_STEP:
            acts[i].kferp = YF_KFERP_STEP;
            break;
        default:
            yf_seterr(YF_ERR_UNSUP, __func__);
            YF_DEALLOCKF();
            free(s_map);
            return -1;
        }
        acts[i].in_i = s_map[sampler].in;
        acts[i].out_i = s_map[sampler].out;
    }

    cont->anims[animation] = yf_kfanim_init(ins, in_n, outs, out_n,
                                            acts, channel_n);
    YF_DEALLOCKF();
    free(s_map);
    if (cont->anims[animation] == NULL)
        return -1;

#undef YF_DEALLOCKF

    for (size_t i = 0; i < channel_n; i++) {
        const int_t node = channels->v[i].target.node;
        if (node == YF_INT_MIN)
            continue;

        if (load_node(gltf, fdata, cont, node) != 0)
            return -1;

        /* XXX: Caller is responsible for creating the node hierarchy. */
        yf_kfanim_settarget(cont->anims[animation], i, cont->nodes[node]);
    }

    return 0;
}

/* Loads a node graph from glTF contents. */
static int load_graph(const gltf_t *gltf, fdata_t *fdata, cont_t *cont,
                      int_t node)
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
        if (load_graph(gltf, fdata, cont,
                       gltf->nodes.v[node].children[i]) != 0)
            return -1;
    }

    if (load_node(gltf, fdata, cont, node) != 0)
        return -1;

    for (size_t i = 0; i < gltf->nodes.v[node].child_n; i++)
        yf_node_insert(cont->nodes[node],
                       cont->nodes[gltf->nodes.v[node].children[i]]);

    return 0;
}

/* Loads an entire scene from glTF contents. */
static int load_scene(const gltf_t *gltf, fdata_t *fdata, cont_t *cont,
                      int_t scene)
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
    if (cont->scns[scene] != NULL)
        return 0;

    cont->scns[scene] = yf_scene_init();
    if (cont->scns[scene] == NULL)
        return -1;

    yf_node_t *node = yf_scene_getnode(cont->scns[scene]);
    yf_node_setname(node, gltf->scenes.v[scene].name);
    for (size_t i = 0; i < gltf->scenes.v[scene].node_n; i++) {
        if (load_graph(gltf, fdata, cont,
                       gltf->scenes.v[scene].nodes[i]) != 0) {
            yf_scene_deinit(cont->scns[scene]);
            cont->scns[scene] = NULL;
            return -1;
        }
        yf_node_insert(node, cont->nodes[gltf->scenes.v[scene].nodes[i]]);
    }

    return 0;
}

/* Loads glTF contents. */
static int load_contents(const gltf_t *gltf, fdata_t *fdata, cont_t *cont)
{
    assert(gltf != NULL);
    assert(fdata != NULL);
    assert(cont != NULL);

    /* mesh creation */
    assert(gltf->meshes.n == 0 || cont->meshes != NULL);
    for (size_t i = 0; i < gltf->meshes.n; i++) {
        if (load_mesh(gltf, fdata, cont, i) != 0)
            return -1;
    }

    /* texture creation */
    assert(gltf->textures.n == 0 || cont->texs != NULL);
    for (size_t i = 0; i < gltf->textures.n; i++) {
        if (load_texture(gltf, fdata, cont, i) != 0)
            return -1;
    }

    /* skin creation */
    assert(gltf->skins.n == 0 || cont->skins != NULL);
    for (size_t i = 0; i < gltf->skins.n; i++) {
        if (load_skin(gltf, fdata, cont, i) != 0)
            return -1;
    }

    /* material creation */
    assert(gltf->materials.n == 0 || cont->matls != NULL);
    for (size_t i = 0; i < gltf->materials.n; i++) {
        if (load_material(gltf, fdata, cont, i) != 0)
            return -1;
    }

    /* node creation */
    assert(gltf->nodes.n == 0 || cont->nodes != NULL);
    for (size_t i = 0; i < gltf->nodes.n; i++) {
        if (load_node(gltf, fdata, cont, i) != 0)
            return -1;
    }

    /* skeleton creation */
    assert(gltf->skins.n == 0 || gltf->nodes.n > 0);
    for (size_t i = 0; i < gltf->skins.n; i++) {
        if (load_skeleton(gltf, fdata, cont, i) != 0)
            return -1;
    }

    /* animation creation */
    assert(gltf->animations.n == 0 || cont->anims != NULL);
    for (size_t i = 0; i < gltf->animations.n; i++) {
        if (load_animation(gltf, fdata, cont, i) != 0)
            return -1;
    }

    /* scene creation */
    assert(gltf->scenes.n == 0 || cont->scns != NULL);
    for (size_t i = 0; i < gltf->scenes.n; i++) {
        if (load_scene(gltf, fdata, cont, i) != 0)
            return -1;
    }

    return 0;
}

/* Manages created contents. */
static int manage_contents(const gltf_t *gltf, cont_t *cont, yf_collec_t *coll)
{
    assert(gltf != NULL);
    assert(cont != NULL);
    assert(coll != NULL);

    /* created scenes */
    if (cont->scns != NULL) {
        for (size_t i = 0; i < gltf->scenes.n; i++) {
            yf_scene_t *scn = cont->scns[i];
            if (scn == NULL)
                continue;

            const char *name = gltf->scenes.v[i].name;
            if (yf_collec_manage(coll, YF_CITEM_SCENE, name, scn) != 0) {
                if (yf_geterr() != YF_ERR_EXIST ||
                    yf_collec_manage(coll, YF_CITEM_SCENE, NULL, scn) != 0)
                    return -1;
            }
        }
    }

    /* created nodes */
    if (cont->nodes != NULL) {
        for (size_t i = 0; i < gltf->nodes.n; i++) {
            yf_node_t *node = cont->nodes[i];
            if (node == NULL)
                continue;

            const char *name = gltf->nodes.v[i].name;
            if (yf_collec_manage(coll, YF_CITEM_NODE, name, node) != 0) {
                if (yf_geterr() != YF_ERR_EXIST ||
                    yf_collec_manage(coll, YF_CITEM_NODE, NULL, node) != 0)
                    return -1;
            }
        }
    }

    /* created meshes */
    if (cont->meshes != NULL) {
        for (size_t i = 0; i < gltf->meshes.n; i++) {
            yf_mesh_t *mesh = cont->meshes[i];
            if (mesh == NULL)
                continue;

            const char *name = gltf->meshes.v[i].name;
            if (yf_collec_manage(coll, YF_CITEM_MESH, name, mesh) != 0) {
                if (yf_geterr() != YF_ERR_EXIST ||
                    yf_collec_manage(coll, YF_CITEM_MESH, NULL, mesh) != 0)
                    return -1;
            }
        }
    }

    /* created textures */
    if (cont->imgs != NULL) {
        for (size_t i = 0; i < gltf->images.n; i++) {
            yf_texture_t *tex = cont->imgs[i];
            if (tex == NULL)
                continue;

            const char *name = gltf->images.v[i].name;
            if (name == NULL)
                name = gltf->images.v[i].uri;

            if (yf_collec_manage(coll, YF_CITEM_TEXTURE, name, tex) != 0) {
                if (yf_geterr() != YF_ERR_EXIST ||
                    yf_collec_manage(coll, YF_CITEM_TEXTURE, NULL, tex) != 0)
                    return -1;
            }
        }
    }

    /* created skins */
    if (cont->skins != NULL) {
        for (size_t i = 0; i < gltf->skins.n; i++) {
            yf_skin_t *skin = cont->skins[i];
            if (skin == NULL)
                continue;

            const char *name = gltf->skins.v[i].name;
            if (yf_collec_manage(coll, YF_CITEM_SKIN, name, skin) != 0) {
                if (yf_geterr() != YF_ERR_EXIST ||
                    yf_collec_manage(coll, YF_CITEM_SKIN, NULL, skin) != 0)
                    return -1;
            }
        }
    }

    /* created materials */
    if (cont->matls != NULL) {
        for (size_t i = 0; i < gltf->materials.n; i++) {
            yf_material_t *matl = cont->matls[i];
            if (matl == NULL)
                continue;

            const char *name = gltf->materials.v[i].name;
            if (yf_collec_manage(coll, YF_CITEM_MATERIAL, name, matl) != 0) {
                if (yf_geterr() != YF_ERR_EXIST ||
                    yf_collec_manage(coll, YF_CITEM_MATERIAL, NULL, matl) != 0)
                    return -1;
            }
        }
    }

    /* created animations */
    if (cont->anims != NULL) {
        for (size_t i = 0; i < gltf->animations.n; i++) {
            yf_kfanim_t *anim = cont->anims[i];
            if (anim == NULL)
                continue;

            const char *name = gltf->animations.v[i].name;
            if (yf_collec_manage(coll, YF_CITEM_KFANIM, name, anim) != 0) {
                if (yf_geterr() != YF_ERR_EXIST ||
                    yf_collec_manage(coll, YF_CITEM_KFANIM, NULL, anim) != 0)
                    return -1;
            }
        }
    }

    return 0;
}

int yf_loadgltf(const char *pathname, size_t index, yf_datac_t *datac)
{
    assert(datac != NULL);
    assert(datac->coll != NULL);

    if (pathname == NULL) {
        yf_seterr(YF_ERR_INVARG, __func__);
        return -1;
    }

    FILE *file = fopen(pathname, "r");
    if (file == NULL) {
        yf_seterr(YF_ERR_NOFILE, __func__);
        return -1;
    }

    gltf_t gltf = {0};
    fdata_t fdata = {0};
    cont_t cont = {0};
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
    switch (datac->datac) {
    case YF_DATAC_COLL:
        r = load_contents(&gltf, &fdata, &cont);
        break;
    case YF_DATAC_SCN:
        if ((r = load_scene(&gltf, &fdata, &cont, index)) == 0)
            datac->scn = cont.scns[index];
        break;
    case YF_DATAC_NODE:
        if ((r = load_node(&gltf, &fdata, &cont, index)) == 0)
            datac->node = cont.nodes[index];
        break;
    case YF_DATAC_MESH:
        if ((r = load_mesh(&gltf, &fdata, &cont, index)) == 0)
            datac->mesh = cont.meshes[index];
        break;
    case YF_DATAC_SKIN:
        if ((r = load_skin(&gltf, &fdata, &cont, index)) == 0)
            datac->skin = cont.skins[index];
        break;
    case YF_DATAC_MATL:
        if ((r = load_material(&gltf, &fdata, &cont, index)) == 0)
            datac->matl = cont.matls[index];
        break;
    case YF_DATAC_TEX:
        if ((r = load_texture(&gltf, &fdata, &cont, index)) == 0)
            datac->tex = cont.imgs[cont.texs[index]];
        break;
    case YF_DATAC_ANIM:
        if ((r = load_animation(&gltf, &fdata, &cont, index)) == 0)
            datac->anim = cont.anims[index];
        break;
    default:
        yf_seterr(YF_ERR_INVARG, __func__);
        r = -1;
    }

    if (r == 0)
        r = manage_contents(&gltf, &cont, datac->coll);

    cont.deinit = r;
    deinit_gltf(&gltf, &fdata, &cont);
    fclose(file);

    return r;
}

int yf_loadgltf2(FILE *file, size_t index, yf_datac_t *datac)
{
    assert(file != NULL && !feof(file));
    assert(datac != NULL);
    assert(datac->coll != NULL);

    gltf_t gltf = {0};
    fdata_t fdata = {0};
    cont_t cont = {0};
    if (init_gltf(file, &gltf, &fdata, &cont) != 0)
        return -1;

    /* XXX: This function does not set 'fdata.path' - it is assumed that
       the whole data is embedded. */

    int r;
    switch (datac->datac) {
    case YF_DATAC_COLL:
        r = load_contents(&gltf, &fdata, &cont);
        break;
    case YF_DATAC_SCN:
        if ((r = load_scene(&gltf, &fdata, &cont, index)) == 0)
            datac->scn = cont.scns[index];
        break;
    case YF_DATAC_NODE:
        if ((r = load_node(&gltf, &fdata, &cont, index)) == 0)
            datac->node = cont.nodes[index];
        break;
    case YF_DATAC_MESH:
        if ((r = load_mesh(&gltf, &fdata, &cont, index)) == 0)
            datac->mesh = cont.meshes[index];
        break;
    case YF_DATAC_SKIN:
        if ((r = load_skin(&gltf, &fdata, &cont, index)) == 0)
            datac->skin = cont.skins[index];
        break;
    case YF_DATAC_MATL:
        if ((r = load_material(&gltf, &fdata, &cont, index)) == 0)
            datac->matl = cont.matls[index];
        break;
    case YF_DATAC_TEX:
        if ((r = load_texture(&gltf, &fdata, &cont, index)) == 0)
            datac->tex = cont.imgs[cont.texs[index]];
        break;
    case YF_DATAC_ANIM:
        if ((r = load_animation(&gltf, &fdata, &cont, index)) == 0)
            datac->anim = cont.anims[index];
        break;
    default:
        yf_seterr(YF_ERR_INVARG, __func__);
        r = -1;
    }

    if (r == 0)
        r = manage_contents(&gltf, &cont, datac->coll);

    cont.deinit = r;
    deinit_gltf(&gltf, &fdata, &cont);

    return r;
}

/*
 * DEVEL
 */

#if defined(YF_DEVEL) && defined(YF_PRINT)

static void print_gltf(const gltf_t *gltf)
{
    printf("\n[YF] OUTPUT (%s):\n glTF:\n", __func__);

    /* extensions used/required */
    printf("  extensions used (%zu):\n", gltf->ext_used_n);
    for (size_t i = 0; i < gltf->ext_used_n; i++)
        printf("   '%s'\n", gltf->ext_used[i]);
    printf("  extensions required (%zu):\n", gltf->ext_req_n);
    for (size_t i = 0; i < gltf->ext_req_n; i++)
        printf("   '%s'\n", gltf->ext_req[i]);

    /* asset */
    printf("  asset:\n"
           "   copyright: %s\n"
           "   generator: %s\n"
           "   version: %s\n"
           "   minVersion: %s\n",
           gltf->asset.copyright, gltf->asset.generator, gltf->asset.version,
           gltf->asset.min_version);

    /* scenes */
    printf("  scene: %lld\n"
           "  scenes (%zu):\n",
           gltf->scene, gltf->scenes.n);
    for (size_t i = 0; i < gltf->scenes.n; i++) {
        printf("   scene '%s':\n"
               "    nodes: [ ",
               gltf->scenes.v[i].name);
        for (size_t j = 0; j < gltf->scenes.v[i].node_n; j++)
            printf("%lld ", gltf->scenes.v[i].nodes[j]);
        puts("]");
    }

    /* nodes */
    printf("  nodes (%zu):\n", gltf->nodes.n);
    for (size_t i = 0; i < gltf->nodes.n; i++) {
        printf("   node '%s':\n"
               "    children: [ ",
               gltf->nodes.v[i].name);
        for (size_t j = 0; j < gltf->nodes.v[i].child_n; j++)
            printf("%lld ", gltf->nodes.v[i].children[j]);
        puts("]");

        printf("    camera: %lld\n"
               "    mesh: %lld\n",
               gltf->nodes.v[i].camera, gltf->nodes.v[i].mesh);

        if (gltf->nodes.v[i].xform_mask == YF_GLTF_XFORM_NONE) {
            puts("    (no transform)");
        } else if (gltf->nodes.v[i].xform_mask == YF_GLTF_XFORM_M) {
            puts("    matrix:");
            for (size_t j = 0; j < 4; j++) {
                for (size_t k = 0; k < 4; k++)
                    printf("     %.4f", gltf->nodes.v[i].matrix[j+k*4]);
                puts("");
            }
        } else {
            printf("    xlation: [ ");
            for (size_t j = 0; j < 3; j++)
                printf("%.4f ", gltf->nodes.v[i].trs.t[j]);
            puts("]");
            printf("    rotation: [ ");
            for (size_t j = 0; j < 4; j++)
                printf("%.4f ", gltf->nodes.v[i].trs.r[j]);
            puts("]");
            printf("    scale: [ ");
            for (size_t j = 0; j < 3; j++)
                printf("%.4f ", gltf->nodes.v[i].trs.s[j]);
            puts("]");
        }

        printf("    skin: %lld\n"
               "    weights: [ ",
               gltf->nodes.v[i].skin);
        for (size_t j = 0; j < gltf->nodes.v[i].weight_n; j++)
            printf("%.6f ", gltf->nodes.v[i].weights[j]);
        puts("]");

        printf("    extensions:\n"
               "     light: %lld\n",
               gltf->nodes.v[i].ext.light);
    }

    /* cameras */
    printf("  cameras (%zu):\n", gltf->cameras.n);
    for (size_t i = 0; i < gltf->cameras.n; i++) {
        printf("   camera '%s':\n", gltf->cameras.v[i].name);
        if (gltf->cameras.v[i].type == YF_GLTF_CAMERA_PERSP)
            printf("    perspective:\n"
                   "     yfov: %.6f\n"
                   "     aspect: %.6f\n"
                   "     znear: %.6f\n"
                   "     zfar: %.6f\n",
                   gltf->cameras.v[i].persp.yfov,
                   gltf->cameras.v[i].persp.aspect_ratio,
                   gltf->cameras.v[i].persp.znear,
                   gltf->cameras.v[i].persp.zfar);
        else
            printf("    orthographic:\n"
                   "     xmag: %.6f\n"
                   "     ymag: %.6f\n"
                   "     znear: %.6f\n"
                   "     zfar: %.6f\n",
                   gltf->cameras.v[i].ortho.xmag,
                   gltf->cameras.v[i].ortho.ymag,
                   gltf->cameras.v[i].ortho.znear,
                   gltf->cameras.v[i].ortho.zfar);
    }

    /* meshes */
    printf("  meshes (%zu):\n", gltf->meshes.n);
    for (size_t i = 0; i < gltf->meshes.n; i++) {
        printf("   mesh '%s':\n"
               "    primitives (%zu):\n",
               gltf->meshes.v[i].name, gltf->meshes.v[i].primitives.n);

        const primitives_t *prims = &gltf->meshes.v[i].primitives;
        for (size_t j = 0; j < gltf->meshes.v[i].primitives.n; j++) {
            printf("     primitive [%zu]:\n"
                   "      attributes:\n"
                   "       POSITION: %lld\n"
                   "       NORMAL: %lld\n"
                   "       TANGENT: %lld\n"
                   "       TEXCOORD_0: %lld\n"
                   "       TEXCOORD_1: %lld\n"
                   "       COLOR_0: %lld\n"
                   "       JOINTS_0: %lld\n"
                   "       WEIGHTS_0: %lld\n"
                   "      indices: %lld\n"
                   "      material: %lld\n"
                   "      mode: %lld\n",
                   j, prims->v[j].attributes[YF_GLTF_ATTR_POS],
                   prims->v[j].attributes[YF_GLTF_ATTR_NORM],
                   prims->v[j].attributes[YF_GLTF_ATTR_TGNT],
                   prims->v[j].attributes[YF_GLTF_ATTR_TC0],
                   prims->v[j].attributes[YF_GLTF_ATTR_TC1],
                   prims->v[j].attributes[YF_GLTF_ATTR_CLR0],
                   prims->v[j].attributes[YF_GLTF_ATTR_JNT0],
                   prims->v[j].attributes[YF_GLTF_ATTR_WGT0],
                   prims->v[j].indices, prims->v[j].material,
                   prims->v[j].mode);

            if (prims->v[j].targets.n == 0) {
                puts("      (no targets)");
            } else {
                const size_t target_n = prims->v[j].targets.n;
                for (size_t k = 0; k < target_n; k++)
                    printf("      targets [%zu]:\n"
                           "       POSITION: %lld\n"
                           "       NORMAL: %lld\n"
                           "       TANGENT: %lld\n",
                           k, prims->v[j].targets.v[k].position,
                           prims->v[j].targets.v[k].normal,
                           prims->v[j].targets.v[k].tangent);
            }
        }

        printf("    weights: [ ");
        for (size_t j = 0; j < gltf->meshes.v[i].weight_n; j++)
            printf("%.6f ", gltf->meshes.v[i].weights[j]);
        puts("]");
    }

    /* skins */
    printf("  skins (%zu):\n", gltf->skins.n);
    for (size_t i = 0; i < gltf->skins.n; i++) {
        printf("   skin '%s':\n"
               "    inverseBindMatrices: %lld\n"
               "    skeleton: %lld\n"
               "    joints: [ ",
               gltf->skins.v[i].name, gltf->skins.v[i].inv_bind_matrices,
               gltf->skins.v[i].skeleton);
        for (size_t j = 0; j < gltf->skins.v[i].joint_n; j++)
            printf("%lld ", gltf->skins.v[i].joints[j]);
        puts("]");
    }

    /* materials */
    printf("  materials (%zu):\n", gltf->materials.n);
    for (size_t i = 0; i < gltf->materials.n; i++) {
        printf("   material '%s':\n", gltf->materials.v[i].name);

        printf("    pbrMetallicRoughness:\n"
               "     baseColorFactor: [%.6f, %.6f, %.6f, %.6f]\n"
               "     baseColorTexture:\n"
               "      index: %lld\n"
               "      texCoord: %lld\n"
               "     metallicFactor: %.6f\n"
               "     roughnessFactor: %.6f\n"
               "     metallicRoughnessTexture:\n"
               "      index: %lld\n"
               "      texCoord: %lld\n",
               gltf->materials.v[i].pbrmr.base_clr_fac[0],
               gltf->materials.v[i].pbrmr.base_clr_fac[1],
               gltf->materials.v[i].pbrmr.base_clr_fac[2],
               gltf->materials.v[i].pbrmr.base_clr_fac[3],
               gltf->materials.v[i].pbrmr.base_clr_tex.index,
               gltf->materials.v[i].pbrmr.base_clr_tex.tex_coord,
               gltf->materials.v[i].pbrmr.metallic_fac,
               gltf->materials.v[i].pbrmr.roughness_fac,
               gltf->materials.v[i].pbrmr.metal_rough_tex.index,
               gltf->materials.v[i].pbrmr.metal_rough_tex.tex_coord);

        printf("    normalTexture:\n"
               "     index: %lld\n"
               "     texCoord: %lld\n"
               "     scale: %.6f\n",
               gltf->materials.v[i].normal_tex.index,
               gltf->materials.v[i].normal_tex.tex_coord,
               gltf->materials.v[i].normal_tex.scale);

        printf("    occlusionTexture:\n"
               "     index: %lld\n"
               "     texCoord: %lld\n"
               "     strength: %.6f\n",
               gltf->materials.v[i].occlusion_tex.index,
               gltf->materials.v[i].occlusion_tex.tex_coord,
               gltf->materials.v[i].occlusion_tex.strength);

        printf("    emissiveFactor: [%.6f, %.6f, %.6f]\n"
               "    emissiveTexture:\n"
               "     index: %lld\n"
               "     texCoord: %lld\n",
               gltf->materials.v[i].emissive_fac[0],
               gltf->materials.v[i].emissive_fac[1],
               gltf->materials.v[i].emissive_fac[2],
               gltf->materials.v[i].emissive_tex.index,
               gltf->materials.v[i].emissive_tex.tex_coord);

        printf("    alphaMode: %d\n"
               "    alphaCutoff: %.6f\n"
               "    doubleSided: %s\n",
               gltf->materials.v[i].alpha_mode,
               gltf->materials.v[i].alpha_cutoff,
               (gltf->materials.v[i].double_sided ? "true" : "false"));

        printf("    extensions:\n"
               "     pbrSpecularGlossiness:");

        if (gltf->materials.v[i].ext.pbrsg != NULL)
            printf("\n"
                   "      diffuseFactor: [%.6f, %.6f, %.6f, %.6f]\n"
                   "      diffuseTexture:\n"
                   "       index: %lld\n"
                   "       texCoord: %lld\n"
                   "      specularFactor: [%.6f, %.6f, %.6f]\n"
                   "      glossinessFactor: %.6f\n"
                   "      specularGlossinessTexture:\n"
                   "       index: %lld\n"
                   "       texCoord: %lld\n"
                   "     unlit: %s\n",
                   gltf->materials.v[i].ext.pbrsg->diffuse_fac[0],
                   gltf->materials.v[i].ext.pbrsg->diffuse_fac[1],
                   gltf->materials.v[i].ext.pbrsg->diffuse_fac[2],
                   gltf->materials.v[i].ext.pbrsg->diffuse_fac[3],
                   gltf->materials.v[i].ext.pbrsg->diffuse_tex.index,
                   gltf->materials.v[i].ext.pbrsg->diffuse_tex.tex_coord,
                   gltf->materials.v[i].ext.pbrsg->specular_fac[0],
                   gltf->materials.v[i].ext.pbrsg->specular_fac[1],
                   gltf->materials.v[i].ext.pbrsg->specular_fac[2],
                   gltf->materials.v[i].ext.pbrsg->glossiness_fac,
                   gltf->materials.v[i].ext.pbrsg->spec_gloss_tex.index,
                   gltf->materials.v[i].ext.pbrsg->spec_gloss_tex.tex_coord,
                   (gltf->materials.v[i].ext.unlit ? "!!!" : "no"));
        else
            printf(" no\n"
                   "     unlit: %s\n",
                   (gltf->materials.v[i].ext.unlit ? "yes" : "no"));

    }

    /* animations */
    printf("  animations (%zu):\n", gltf->animations.n);
    for (size_t i = 0; i < gltf->animations.n; i++) {
        printf("   animation '%s':\n"
               "    channels (%zu):\n",
               gltf->animations.v[i].name, gltf->animations.v[i].channels.n);

        for (size_t j = 0; j < gltf->animations.v[i].channels.n; j++)
            printf("     channel [%zu]:\n"
                   "      sampler: %lld\n"
                   "      target:\n"
                   "       node: %lld\n"
                   "       path: %d\n",
                   j, gltf->animations.v[i].channels.v[j].sampler,
                   gltf->animations.v[i].channels.v[j].target.node,
                   gltf->animations.v[i].channels.v[j].target.path);

        printf("    samplers (%zu):\n", gltf->animations.v[i].samplers.n);
        for (size_t j = 0; j < gltf->animations.v[i].samplers.n; j++)
            printf("     sampler [%zu]:\n"
                   "      input: %lld\n"
                   "      output: %lld\n"
                   "      interpolation: %d\n",
                   j, gltf->animations.v[i].samplers.v[j].input,
                   gltf->animations.v[i].samplers.v[j].output,
                   gltf->animations.v[i].samplers.v[j].interpolation);
    }

    /* accessors */
    printf("  accessors (%zu):\n", gltf->accessors.n);
    for (size_t i = 0; i < gltf->accessors.n; i++) {
        printf("   accessor '%s':\n"
               "    bufferView: %lld\n"
               "    byteOffset: %lld\n"
               "    count: %lld\n"
               "    componenType: %lld\n"
               "    type: %d\n",
               gltf->accessors.v[i].name, gltf->accessors.v[i].buffer_view,
               gltf->accessors.v[i].byte_off, gltf->accessors.v[i].count,
               gltf->accessors.v[i].comp_type, gltf->accessors.v[i].type);

        size_t comp_n, col_n = 1;
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
            comp_n = 4;
            break;
        case YF_GLTF_TYPE_MAT2:
            comp_n = 4;
            col_n = 2;
            break;
        case YF_GLTF_TYPE_MAT3:
            comp_n = 9;
            col_n = 3;
            break;
        case YF_GLTF_TYPE_MAT4:
            comp_n = 16;
            col_n = 4;
            break;
        }

        if (col_n > 1) {
            puts("    min:");
            for (size_t j = 0; j < col_n; j++) {
                for (size_t k = 0; k < col_n; k++)
                    printf("     %.4f", gltf->accessors.v[i].min.m4[j+k*col_n]);
                puts("");
            }
            puts("    max:");
            for (size_t j = 0; j < col_n; j++) {
                for (size_t k = 0; k < col_n; k++)
                    printf("     %.4f", gltf->accessors.v[i].max.m4[j+k*col_n]);
                puts("");
            }
        } else if (comp_n > 1) {
            printf("    min: [ ");
            for (size_t j = 0; j < comp_n; j++)
                printf("%.6f ", gltf->accessors.v[i].min.m4[j]);
            printf("]\n    max: [ ");
            for (size_t j = 0; j < comp_n; j++)
                printf("%.6f ", gltf->accessors.v[i].max.m4[j]);
            puts("]");
        } else {
            printf("    min: %.6f\n"
                   "    max: %.6f\n",
                   gltf->accessors.v[i].min.s, gltf->accessors.v[i].max.s);
        }

        printf("    normalized: %s\n"
               "    sparse:\n"
               "     indices:\n"
               "      bufferView: %lld\n"
               "      byteOffset: %lld\n"
               "      componenType: %lld\n"
               "     values:\n"
               "      bufferView: %lld\n"
               "      byteOffset: %lld\n",
               (gltf->accessors.v[i].normalized ? "true" : "false"),
               gltf->accessors.v[i].sparse.indices.buffer_view,
               gltf->accessors.v[i].sparse.indices.byte_off,
               gltf->accessors.v[i].sparse.indices.comp_type,
               gltf->accessors.v[i].sparse.values.buffer_view,
               gltf->accessors.v[i].sparse.values.byte_off);
    }

    /* buffer views */
    printf("  bufferViews (%zu):\n", gltf->bufferviews.n);
    for (size_t i = 0; i < gltf->bufferviews.n; i++)
        printf("   view '%s':\n"
               "    buffer: %lld\n"
               "    byteOffset: %lld\n"
               "    byteLength: %lld\n"
               "    byteStride: %lld\n"
               "    target: %lld\n",
               gltf->bufferviews.v[i].name, gltf->bufferviews.v[i].buffer,
               gltf->bufferviews.v[i].byte_off, gltf->bufferviews.v[i].byte_len,
               gltf->bufferviews.v[i].byte_strd, gltf->bufferviews.v[i].target);

    /* buffers */
    printf("  buffers (%zu):\n", gltf->buffers.n);
    for (size_t i = 0; i < gltf->buffers.n; i++)
        printf("   buffer '%s':\n"
               "    byteLength: %lld\n"
               "    uri: %s\n",
               gltf->buffers.v[i].name, gltf->buffers.v[i].byte_len,
               gltf->buffers.v[i].uri);

    /* textures */
    printf("  textures (%zu):\n", gltf->textures.n);
    for (size_t i = 0; i < gltf->textures.n; i++)
        printf("   texture '%s':\n"
               "    sampler: %lld\n"
               "    source: %lld\n",
               gltf->textures.v[i].name, gltf->textures.v[i].sampler,
               gltf->textures.v[i].source);

    /* images */
    printf("  images (%zu):\n", gltf->images.n);
    for (size_t i = 0; i < gltf->images.n; i++)
        printf("   image '%s':\n"
               "    uri: %s\n"
               "    mimeType: %s\n"
               "    bufferView: %lld\n",
               gltf->images.v[i].name, gltf->images.v[i].uri,
               gltf->images.v[i].mime_type, gltf->images.v[i].buffer_view);

    /* samplers */
    printf("  samplers (%zu):\n", gltf->samplers.n);
    for (size_t i = 0; i < gltf->samplers.n; i++)
        printf("   sampler '%s':\n"
               "    wrapS: %lld\n"
               "    wrapT: %lld\n"
               "    magFilter: %lld\n"
               "    minFilter: %lld\n",
               gltf->samplers.v[i].name, gltf->samplers.v[i].wrap_s,
               gltf->samplers.v[i].wrap_t, gltf->samplers.v[i].mag_filter,
               gltf->samplers.v[i].min_filter);

    /* extensions */
    puts("  extensions:");

    /* lights punctual */
    printf("   lights (%zu):\n", gltf->ext.lights.n);
    for (size_t i = 0; i < gltf->ext.lights.n; i++) {
        char *type = "";
        switch (gltf->ext.lights.v[i].type) {
        case YF_GLTF_LIGHT_POINT:
            type = "point";
            break;
        case YF_GLTF_LIGHT_SPOT:
            type = "spot";
            break;
        case YF_GLTF_LIGHT_DIRECT:
            type = "directional";
        }
        printf("    light '%s':\n"
               "     type: %s\n"
               "     color: [%.6f, %.6f, %.6f]\n"
               "     intensity: %.6f\n"
               "     range: %.6f\n"
               "     spot:\n"
               "      innerConeAngle: %.6f\n"
               "      outerConeAngle: %.6f\n",
               gltf->ext.lights.v[i].name, type, gltf->ext.lights.v[i].color[0],
               gltf->ext.lights.v[i].color[1], gltf->ext.lights.v[i].color[2],
               gltf->ext.lights.v[i].intensity, gltf->ext.lights.v[i].range,
               gltf->ext.lights.v[i].spot.inner_cone_angle,
               gltf->ext.lights.v[i].spot.outer_cone_angle);
    }

    puts("");
}

#endif
