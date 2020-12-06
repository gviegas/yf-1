/*
 * YF
 * filetype-obj.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <yf/com/yf-error.h>

#include "filetype-obj.h"
#include "vertex.h"
#include "hashset.h"

#undef YF_INITCAP
#define YF_INITCAP 1024

#if !defined(_POSIX_C_SOURCE) || (_POSIX_C_SOURCE < 200809L)
/* TODO: Provide an implementation of `getline`. */
# error "Invalid platform"
#endif

/* Type holding key & value for use in the vertex map. */
typedef struct {
  unsigned key[3];
  unsigned value;
} L_kv;

/* Hashes a `L_kv`. */
static size_t hash_kv(const void *x);

/* Compares a `L_kv` to another. */
static int cmp_kv(const void *a, const void *b);

/* Deallocates a `L_kv`. */
static int dealloc_kv(void *val, void *arg);

int yf_filetype_obj_load(const char *pathname, YF_meshdt *data) {
  if (pathname == NULL) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
  }
  assert(data != NULL);

  YF_vec3 *poss = NULL;
  YF_vec2 *texs = NULL;
  YF_vec3 *norms = NULL;
  YF_vmdl *verts = NULL;
  unsigned *inds = NULL;
  size_t pos_n = 0;
  size_t tex_n = 0;
  size_t norm_n = 0;
  size_t vtx_n = 0;
  size_t idx_n = 0;
  size_t pos_cap = 0;
  size_t tex_cap = 0;
  size_t norm_cap = 0;
  size_t vtx_cap = 0;
  size_t idx_cap = 0;

  const char *fmt_v = "v %f %f %f";
  const char *fmt_vt = "vt %f %f";
  const char *fmt_vn = "vn %f %f %f";
  const char *fmt_f1 = "f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d";
  const char *fmt_f2 = "f %d/%d %d/%d %d/%d %d/%d";
  const char *fmt_f3 = "f %d//%d %d//%d %d//%d %d//%d";
  const char *fmt_f4 = "f %d %d %d %d";
  float v[3];
  float vt[2];
  float vn[3];
  int f[12];

  YF_hashset map = yf_hashset_init(hash_kv, cmp_kv);
  if (map == NULL)
    return -1;

  FILE *file = fopen(pathname, "r");
  if (file == NULL) {
    yf_seterr(YF_ERR_NOFILE, __func__);
    return -1;
  }

  char *line = NULL;
  size_t line_sz = 0;
  int r = -1;
  while (getline(&line, &line_sz, file) != -1) {
    if (sscanf(line, fmt_v, v, v+1, v+2) == 3) {
      /* position */
      if (pos_n == pos_cap) {
        pos_cap = pos_n == 0 ? YF_INITCAP : pos_cap*2;
        YF_vec3 *tmp = realloc(poss, pos_cap * sizeof *poss);
        if (tmp == NULL) {
          yf_seterr(YF_ERR_NOMEM, __func__);
          goto dealloc;
        }
        poss = tmp;
      }
      memcpy(poss+pos_n, v, sizeof v);
      ++pos_n;
    } else if (sscanf(line, fmt_vt, vt, vt+1) == 2) {
      /* tex. coord. */
      if (tex_n == tex_cap) {
        tex_cap = tex_n == 0 ? YF_INITCAP : tex_cap*2;
        YF_vec2 *tmp = realloc(texs, tex_cap * sizeof *texs);
        if (tmp == NULL) {
          yf_seterr(YF_ERR_NOMEM, __func__);
          goto dealloc;
        }
        texs = tmp;
      }
      memcpy(texs+tex_n, vt, sizeof vt);
      ++tex_n;
    } else if (sscanf(line, fmt_vn, vn, vn+1, vn+2) == 3) {
      /* normal */
      if (norm_n == norm_cap) {
        norm_cap = norm_n == 0 ? YF_INITCAP : norm_cap*2;
        YF_vec3 *tmp = realloc(norms, norm_cap * sizeof *norms);
        if (tmp == NULL) {
          yf_seterr(YF_ERR_NOMEM, __func__);
          goto dealloc;
        }
        norms = tmp;
      }
      memcpy(norms+norm_n, vn, sizeof vn);
      ++norm_n;
    } else {
      /* face or ignored */
      int n;
      int fmt;
      n = sscanf(
        line,
        fmt_f1,
        f, f+1, f+2, f+3, f+4, f+5, f+6, f+7, f+8, f+9, f+10, f+11);
      if (n == 9 || n == 12) {
        fmt = 1;
      } else {
        n = sscanf(line, fmt_f2, f, f+1, f+2, f+3, f+4, f+5, f+6, f+7);
        if (n == 6 || n == 8) {
          fmt = 2;
        } else {
          n = sscanf(line, fmt_f3, f, f+1, f+2, f+3, f+4, f+5, f+6, f+7);
          if (n == 6 || n == 8) {
            fmt = 3;
          } else {
            n = sscanf(line, fmt_f4, f, f+1, f+2, f+3);
            if (n == 3 || n == 4) {
              fmt = 4;
            } else {
              continue;
            }
          }
        }
      }
      /* quads will need two triangles and thus, six indices */
      if (idx_n + 6 > idx_cap) {
        idx_cap = idx_n == 0 ? YF_INITCAP : idx_cap*2;
        unsigned *tmp = realloc(inds, idx_cap * sizeof *inds);
        if (tmp == NULL) {
          yf_seterr(YF_ERR_NOMEM, __func__);
          goto dealloc;
        }
        inds = tmp;
      }
      L_kv kv = {0};
      L_kv *stored_val;
      unsigned face[4];
      switch (fmt) {
        case 1:
          /* pos/texc/norm triangle or quad */
          for (int i = 0; i < n/3; ++i) {
            kv.key[0] = f[3*i] - 1;
            kv.key[1] = f[3*i+1] - 1;
            kv.key[2] = f[3*i+2] - 1;
            if ((stored_val = yf_hashset_search(map, &kv)) != NULL) {
              face[i] = stored_val->value;
            } else {
              if (vtx_n == vtx_cap) {
                vtx_cap = vtx_n == 0 ? YF_INITCAP : vtx_cap*2;
                YF_vmdl *tmp = realloc(verts, vtx_cap * sizeof *verts);
                if (tmp == NULL) {
                  yf_seterr(YF_ERR_NOMEM, __func__);
                  goto dealloc;
                }
                verts = tmp;
              }
              yf_vec3_copy(verts[vtx_n].pos, poss[kv.key[0]]);
              yf_vec2_copy(verts[vtx_n].tc, texs[kv.key[1]]);
              yf_vec3_copy(verts[vtx_n].norm, norms[kv.key[2]]);
              L_kv *new_val = malloc(sizeof kv);
              if (new_val == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                goto dealloc;
              }
              *new_val = kv;
              new_val->value = vtx_n++;
              yf_hashset_insert(map, new_val);
              face[i] = new_val->value;
            }
          }
          inds[idx_n++] = face[0];
          inds[idx_n++] = face[1];
          inds[idx_n++] = face[2];
          if (n == 12) {
            inds[idx_n++] = face[0];
            inds[idx_n++] = face[2];
            inds[idx_n++] = face[3];
          }
          break;
        case 2:
          /* pos/texc triangle or quad */
          for (int i = 0; i < n/2; ++i) {
            kv.key[0] = f[2*i] - 1;
            kv.key[1] = f[2*i+1] - 1;
            if ((stored_val = yf_hashset_search(map, &kv)) != NULL) {
              face[i] = stored_val->value;
            } else {
              if (vtx_n == vtx_cap) {
                vtx_cap = vtx_n == 0 ? YF_INITCAP : vtx_cap*2;
                YF_vmdl *tmp = realloc(verts, vtx_cap * sizeof *verts);
                if (tmp == NULL) {
                  yf_seterr(YF_ERR_NOMEM, __func__);
                  goto dealloc;
                }
                verts = tmp;
              }
              yf_vec3_copy(verts[vtx_n].pos, poss[kv.key[0]]);
              yf_vec2_copy(verts[vtx_n].tc, texs[kv.key[1]]);
              yf_vec3_set(verts[vtx_n].norm, 0.0);
              L_kv *new_val = malloc(sizeof kv);
              if (new_val == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                goto dealloc;
              }
              *new_val = kv;
              new_val->value = vtx_n++;
              yf_hashset_insert(map, new_val);
              face[i] = new_val->value;
            }
          }
          inds[idx_n++] = face[0];
          inds[idx_n++] = face[1];
          inds[idx_n++] = face[2];
          if (n == 8) {
            inds[idx_n++] = face[0];
            inds[idx_n++] = face[2];
            inds[idx_n++] = face[3];
          }
          break;
        case 3:
          /* pos/norm triangle or quad */
          for (int i = 0; i < n/2; ++i) {
            kv.key[0] = f[2*i] - 1;
            kv.key[2] = f[2*i+1] - 1;
            if ((stored_val = yf_hashset_search(map, &kv)) != NULL) {
              face[i] = stored_val->value;
            } else {
              if (vtx_n == vtx_cap) {
                vtx_cap = vtx_n == 0 ? YF_INITCAP : vtx_cap*2;
                YF_vmdl *tmp = realloc(verts, vtx_cap * sizeof *verts);
                if (tmp == NULL) {
                  yf_seterr(YF_ERR_NOMEM, __func__);
                  goto dealloc;
                }
                verts = tmp;
              }
              yf_vec3_copy(verts[vtx_n].pos, poss[kv.key[0]]);
              yf_vec2_set(verts[vtx_n].tc, 0.0);
              yf_vec3_copy(verts[vtx_n].norm, norms[kv.key[2]]);
              L_kv *new_val = malloc(sizeof kv);
              if (new_val == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                goto dealloc;
              }
              *new_val = kv;
              new_val->value = vtx_n++;
              yf_hashset_insert(map, new_val);
              face[i] = new_val->value;
            }
          }
          inds[idx_n++] = face[0];
          inds[idx_n++] = face[1];
          inds[idx_n++] = face[2];
          if (n == 8) {
            inds[idx_n++] = face[0];
            inds[idx_n++] = face[2];
            inds[idx_n++] = face[3];
          }
          break;
        case 4:
          /* position only triangle or quad */
          for (int i = 0; i < n; ++i) {
            kv.key[0] = f[i] - 1;
            if ((stored_val = yf_hashset_search(map, &kv)) != NULL) {
              face[i] = stored_val->value;
            } else {
              if (vtx_n == vtx_cap) {
                vtx_cap = vtx_n == 0 ? YF_INITCAP : vtx_cap*2;
                YF_vmdl *tmp = realloc(verts, vtx_cap * sizeof *verts);
                if (tmp == NULL) {
                  yf_seterr(YF_ERR_NOMEM, __func__);
                  goto dealloc;
                }
                verts = tmp;
              }
              yf_vec3_copy(verts[vtx_n].pos, poss[kv.key[0]]);
              yf_vec2_set(verts[vtx_n].tc, 0.0);
              yf_vec3_set(verts[vtx_n].norm, 0.0);
              L_kv *new_val = malloc(sizeof kv);
              if (new_val == NULL) {
                yf_seterr(YF_ERR_NOMEM, __func__);
                goto dealloc;
              }
              *new_val = kv;
              new_val->value = vtx_n++;
              yf_hashset_insert(map, new_val);
              face[i] = new_val->value;
            }
          }
          inds[idx_n++] = face[0];
          inds[idx_n++] = face[1];
          inds[idx_n++] = face[2];
          if (n == 4) {
            inds[idx_n++] = face[0];
            inds[idx_n++] = face[2];
            inds[idx_n++] = face[3];
          }
          break;
      }
    }
  }

  if (vtx_n < vtx_cap) {
    YF_vmdl *tmp = realloc(verts, vtx_n * sizeof *verts);
    if (tmp != NULL)
      verts = tmp;
  }
  data->v.vtype = YF_VTYPE_MDL;
  data->v.data = verts;
  data->v.n = vtx_n;
  verts = NULL;
  if (vtx_n > 65536) {
    if (idx_n < idx_cap) {
      unsigned *tmp = realloc(inds, idx_n * sizeof *inds);
      if (tmp != NULL)
        inds = tmp;
    }
    data->i.data = inds;
    data->i.stride = sizeof *inds;
  } else {
    unsigned short *new_inds = malloc(idx_n * sizeof(unsigned short));
    if (new_inds != NULL) {
      for (unsigned i = 0; i < idx_n; ++i)
        new_inds[i] = inds[i];
      free(inds);
      data->i.data = new_inds;
      data->i.stride = sizeof *new_inds;
    } else {
      data->i.data = inds;
      data->i.stride = sizeof *inds;
    }
  }
  data->i.n = idx_n;
  inds = NULL;

  r = 0;
  dealloc:
  free(poss);
  free(texs);
  free(norms);
  free(verts);
  free(inds);
  yf_hashset_each(map, dealloc_kv, NULL);
  yf_hashset_deinit(map);
  fclose(file);
  free(line);
  return r;
}

static size_t hash_kv(const void *x) {
  const unsigned *k = x;
  return (k[0] ^ k[1] ^ k[2] ^ 4271934599);
}

static int cmp_kv(const void *a, const void *b) {
  const unsigned *k1 = a;
  const unsigned *k2 = b;
  return !((k1[0] == k2[0]) && (k1[1] == k2[1]) && (k1[2] == k2[2]));
}

static int dealloc_kv(void *val, void *arg) {
  free(val);
  return 0;
}
