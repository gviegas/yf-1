/*
 * YF
 * data-sfnt.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include <yf/com/yf-util.h>
#include <yf/com/yf-hashset.h>
#include <yf/com/yf-error.h>

#include "data-sfnt.h"

#ifdef _DEFAULT_SOURCE
#include <endian.h>
#else
/* TODO */
# error "Invalid platform"
#endif

#define YF_SFNT_MAKETAG(c1, c2, c3, c4) \
  ((c1 << 24) | (c2 << 16) | (c3 << 8) | c4)

/*
 * Common
 */

/* Font directory. */
#define YF_SFNT_TTF be32toh(0x00010000)

typedef struct {
  uint32_t version;
  uint16_t tab_n;
  uint16_t search_rng;
  uint16_t entry_sel;
  uint16_t rng_shf;
} L_diro;
#define YF_SFNT_DIROSZ 12
static_assert(offsetof(L_diro, rng_shf) == YF_SFNT_DIROSZ-2, "!offsetof");

typedef struct {
  uint32_t tag;
  uint32_t chsum;
  uint32_t off;
  uint32_t len;
} L_dire;
#define YF_SFNT_DIRESZ 16
static_assert(sizeof(L_dire) == YF_SFNT_DIRESZ, "!sizeof");

typedef struct {
  L_diro diro;
  L_dire *dires;
} L_dir;

/* Character to glyph mapping. */
#define YF_SFNT_CMAPTAG YF_SFNT_MAKETAG('c', 'm', 'a', 'p')

typedef struct {
  uint16_t version;
  uint16_t tab_n;
} L_cmaph;
#define YF_SFNT_CMAPHSZ 4
static_assert(offsetof(L_cmaph, tab_n) == YF_SFNT_CMAPHSZ-2, "!offsetof");

typedef struct {
  uint16_t platform;
  uint16_t encoding;
  uint32_t off;
} L_cmape;
#define YF_SFNT_CMAPESZ 8
static_assert(sizeof(L_cmape) == YF_SFNT_CMAPESZ, "!sizeof");

typedef struct {
  L_cmaph cmaph;
  L_cmape *cmapes;
} L_cmap;

/* Font header. */
#define YF_SFNT_HEADTAG YF_SFNT_MAKETAG('h', 'e', 'a', 'd')

typedef struct {
  uint16_t major;
  uint16_t minor;
  uint32_t revision;
  uint32_t chsum_adj;
  uint32_t magic;
  uint16_t flags;
  uint16_t upem;
  int32_t created[2]; /* i64 */
  int32_t modified[2]; /* i64 */
  int16_t x_min;
  int16_t y_min;
  int16_t x_max;
  int16_t y_max;
  uint16_t style;
  uint16_t low_rec_ppem;
  int16_t dir_hint;
  int16_t loca_fmt;
  int16_t glyph_fmt;
} L_head;
#define YF_SFNT_HEADSZ 54
static_assert(offsetof(L_head, glyph_fmt) == YF_SFNT_HEADSZ-2, "!offsetof");

/* Horizontal layout header. */
#define YF_SFNT_HHEATAG YF_SFNT_MAKETAG('h', 'h', 'e', 'a')

typedef struct {
  uint16_t major;
  uint16_t minor;
  int16_t ascent;
  int16_t descent;
  int16_t line_gap;
  uint16_t adv_wdt_max;
  int16_t lsb_min;
  int16_t rsb_min;
  int16_t x_extent_max;
  int16_t caret_slp_rise;
  int16_t caret_slp_run;
  int16_t caret_off;
  int16_t reserved[4];
  int16_t metric_fmt;
  uint16_t hmetric_n;
} L_hhea;
#define YF_SFNT_HHEASZ 36
static_assert(offsetof(L_hhea, hmetric_n) == YF_SFNT_HHEASZ-2, "!offsetof");

/* Horizontal layout metrics. */
#define YF_SFNT_HMTXTAG YF_SFNT_MAKETAG('h', 'm', 't', 'x')

typedef struct {
  uint16_t adv_wdt;
  int16_t lsb;
} L_hmtxe;
#define YF_SFNT_HMTXESZ 4
static_assert(sizeof(L_hmtxe) == YF_SFNT_HMTXESZ, "!sizeof");

typedef struct {
  L_hmtxe *hmtxes;
  int16_t *lsbs;
} L_hmtx;

/* Maximum profile. */
#define YF_SFNT_MAXPTAG YF_SFNT_MAKETAG('m', 'a', 'x', 'p')

typedef struct {
  uint32_t version;
  uint16_t glyph_n;
  uint16_t pt_max;
  uint16_t contr_max;
  uint16_t comp_pt_max;
  uint16_t comp_contr_max;
  uint16_t zone_max;
  uint16_t z0_pt_max;
  uint16_t storage_max;
  uint16_t fdef_max;
  uint16_t idef_max;
  uint16_t stack_elem_max;
  uint16_t hint_sz_max;
  uint16_t comp_elem_max;
  uint16_t comp_dep_max;
} L_maxp;
#define YF_SFNT_MAXPSZ 32
static_assert(offsetof(L_maxp, comp_dep_max) == YF_SFNT_MAXPSZ-2, "!offsetof");

/* Naming. */
#define YF_SFNT_NAMETAG YF_SFNT_MAKETAG('n', 'a', 'm', 'e')

typedef struct {
  uint16_t format;
  uint16_t count;
  uint16_t str_off;
} L_nameh;
#define YF_SFNT_NAMEHSZ 6
static_assert(offsetof(L_nameh, str_off) == YF_SFNT_NAMEHSZ-2, "!offsetof");

typedef struct {
  uint16_t platform;
  uint16_t encoding;
  uint16_t language;
  uint16_t name;
  uint16_t len;
  uint16_t off;
} L_namee;
#define YF_SFNT_NAMEESZ 12
static_assert(sizeof(L_namee) == YF_SFNT_NAMEESZ, "!sizeof");

typedef struct {
  uint16_t len;
  uint16_t off;
} L_namel;
#define YF_SFNT_NAMELSZ 4
static_assert(sizeof(L_namel) == YF_SFNT_NAMELSZ, "!sizeof");

typedef struct {
  L_nameh nameh;
  L_namee *namees;
  /* naming table format 1 */
  uint16_t lang_n;
  L_namel *namels;
} L_name;

/* OS/2 & Windows metrics. */
#define YF_SFNT_OS2TAG YF_SFNT_MAKETAG('O', 'S', '/', '2')

typedef struct {
  /* v0 */
  uint16_t version;
  int16_t x_avg_char_wdt;
  uint16_t wgt_class;
  uint16_t wdt_class;
  uint16_t type;
  int16_t y_subsc_x_sz;
  int16_t y_subsc_y_sz;
  int16_t y_subsc_x_off;
  int16_t y_subsc_y_off;
  int16_t y_supersc_x_sz;
  int16_t y_supersc_y_sz;
  int16_t y_supersc_x_off;
  int16_t y_supersc_y_off;
  int16_t y_strikeout_sz;
  int16_t y_strikeout_pos;
  int16_t family_class;
  uint8_t panose[10];
  uint16_t unicode_rng1[2]; /* u32 */
  uint16_t unicode_rng2[2]; /* u32 */
  uint16_t unicode_rng3[2]; /* u32 */
  uint16_t unicode_rng4[2]; /* u32 */
  uint8_t ach_vend[4];
  uint16_t sel;
  uint16_t first_char_i;
  uint16_t last_char_i;
  int16_t typo_ascent;
  int16_t typo_descent;
  int16_t typo_line_gap;
  uint16_t win_ascent;
  uint16_t win_descent;
  /* v1 */
  uint16_t code_page_rng1[2]; /* u32 */
  uint16_t code_page_rng2[2]; /* u32 */
  /* v2, v3 & v4 */
  int16_t x_hgt;
  int16_t cap_hgt;
  uint16_t deft_char;
  uint16_t break_char;
  uint16_t max_ctx;
  /* v5 */
  uint16_t lo_optical_pt_sz;
  uint16_t up_optical_pt_sz;
} L_os2;
#define YF_SFNT_OS2V0  78
#define YF_SFNT_OS2SZ 100
static_assert(offsetof(L_os2, up_optical_pt_sz) == YF_SFNT_OS2SZ-2,
    "!offsetof");

/* PostScript info. */
#define YF_SFNT_POSTTAG YF_SFNT_MAKETAG('p', 'o', 's', 't')

typedef struct {
  uint32_t version;
  uint32_t italic_ang;
  int16_t undln_pos;
  int16_t undln_thick;
  uint32_t fixed_pitch;
  uint32_t mem42_min;
  uint32_t mem42_max;
  uint32_t mem1_min;
  uint32_t mem1_max;
} L_posth;
#define YF_SFNT_POSTHSZ 32
static_assert(offsetof(L_posth, mem1_max) == YF_SFNT_POSTHSZ-4, "!offsetof");

typedef struct {
  L_posth posth;
  /* XXX: Glyph names won't see any use. */
} L_post;

/*
 * TrueType
 */

/* Control values. */
#define YF_SFNT_CVTTAG YF_SFNT_MAKETAG('c', 'v', 't', ' ')

typedef struct {
  int16_t *ctrl_vals;
} L_cvt;

/* Font program (executed once). */
#define YF_SFNT_FPGMTAG YF_SFNT_MAKETAG('f', 'p', 'g', 'm')

typedef struct {
  uint8_t *instrs;
} L_fpgm;

/* Greyscale device rendering. */
#define YF_SFNT_GASPTAG YF_SFNT_MAKETAG('g', 'a', 's', 'p')

typedef struct {
  uint16_t version;
  uint16_t rng_n;
} L_gasph;
#define YF_SFNT_GASPHSZ 4
static_assert(offsetof(L_gasph, rng_n) == YF_SFNT_GASPHSZ-2, "!offsetof");

typedef struct {
  uint16_t rng_ppem_max;
  uint16_t rng_behav;
} L_gaspe;
#define YF_SFNT_GASPESZ 4
static_assert(sizeof(L_gaspe) == YF_SFNT_GASPESZ, "!sizeof");

typedef struct {
  L_gasph gasph;
  L_gaspe *gaspes;
} L_gasp;

/* Glyph data. */
#define YF_SFNT_GLYFTAG YF_SFNT_MAKETAG('g', 'l', 'y', 'f')

typedef struct {
  int16_t contr_n;
  int16_t x_min;
  int16_t y_min;
  int16_t x_max;
  int16_t y_max;
  uint8_t data[];
} L_glyfd;
#define YF_SFNT_GLYFSZ 10
static_assert(offsetof(L_glyfd, data) == YF_SFNT_GLYFSZ, "!offsetof");

typedef struct {
  uint8_t *glyphs;
} L_glyf;

/* Glyph location offsets. */
#define YF_SFNT_LOCATAG YF_SFNT_MAKETAG('l', 'o', 'c', 'a')

typedef struct {
  union {
    uint16_t *off16;
    uint32_t *off32;
  };
} L_loca;

/* Control value program (executed on font changes). */
#define YF_SFNT_PREPTAG YF_SFNT_MAKETAG('p', 'r', 'e', 'p')

typedef struct {
  uint8_t *program;
} L_prep;

/* SFNT tables. */
typedef struct {
  L_dir *dir;
  L_cmap *cmap;
  L_head *head;
  L_hhea *hhea;
  L_hmtx *hmtx;
  L_maxp *maxp;
  L_name *name;
  L_os2 *os2;
  L_post *post;
  struct {
    L_cvt *cvt;
    L_fpgm *fpgm;
    L_gasp *gasp;
    L_glyf *glyf;
    L_loca *loca;
    L_prep *prep;
  } ttf;
} L_sfnt;

/* Font metrics. */
typedef struct {
  uint16_t upem;
  int16_t x_min;
  int16_t y_min;
  int16_t x_max;
  int16_t y_max;
  int16_t ascent;
  int16_t descent;
  int16_t line_gap;
  uint16_t adv_wdt_max;
  int16_t lsb_min;
  int16_t rsb_min;
  struct {
    uint16_t adv_wdt;
    int16_t lsb;
    /* TODO: Other per-glyph metrics. */
  } *glyphs;
} L_fontmet;

/* Font mapping. */
typedef struct {
#define YF_SFNT_MAP_SPARSE  0
#define YF_SFNT_MAP_TRIMMED 1
  int map;
  union {
    struct {
      YF_hashset glyph_ids;
    } sparse;
    struct {
      uint16_t first_code;
      uint16_t entry_n;
      uint16_t *glyph_ids;
    } trimmed;
  };
} L_fontmap;

/* Font strings. */
typedef struct {
  char *copyright;
  char *family;
  char *subfamily;
  char *uid;
  char *name;
  char *version;
  char *trademark;
  char *manufacturer;
  char *designer;
  char *description;
  char *license;
  char *typographic_family;
  char *typographic_subfamily;
  char *sample_text;
} L_fontstr;

/* Font. */
typedef struct {
  uint16_t glyph_n;
  uint16_t pt_max;
  uint16_t contr_max;
  uint16_t comp_pt_max;
  uint16_t comp_contr_max;
  uint16_t comp_elem_max;
  L_fontmet met;
  L_fontmap map;
  L_fontstr str;
  struct {
    /* glyph offsets stored pre-multiplied/byte-swapped */
    uint32_t *loca;
    /* glyph descriptions stored as is */
    uint8_t *glyf;
  } ttf;
} L_font;

/* Verifies if file is valid. */
static int verify_file(FILE *file);

/* Loads a font containing TrueType outline. */
static int load_ttf(L_sfnt *sfnt, FILE *file);

/* Deinitializes SFNT tables. */
static void deinit_tables(L_sfnt *sfnt);

/* Gets font metrics. */
static int get_metrics(const L_sfnt *sfnt, L_fontmet *fmet);

/* Sets mapping of character codes to glyph indices. */
static int set_mapping(const L_cmap *cmap, FILE *file, uint32_t off,
    L_fontmap *fmap);

/* Fills font strings. */
static int fill_str(const L_name *name, FILE *file, uint32_t str_off,
    L_fontstr *fstr);

/* Hashes glyphs of a 'L_fontmap' sparse format. */
static size_t hash_fmap(const void *x);

/* Compares glyphs of a 'L_fontmap' sparse format. */
static int cmp_fmap(const void *a, const void *b);

/* Deinitializes font data. */
static void deinit_font(void *font);

/* Scales metrics. */
static void scale_metrics(void *font, uint16_t pt, uint16_t dpi,
    int16_t *x_min, int16_t *y_min, int16_t *x_max, int16_t *y_max);

/* Gets a glyph. */
static int get_glyph(void *font, wchar_t code, uint16_t pt, uint16_t dpi,
    YF_glyph *glyph);

int yf_loadsfnt(const char *pathname, YF_fontdt *data) {
  if (pathname == NULL) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
  }

  L_sfnt sfnt = {0};
  FILE *file = fopen(pathname, "r");
  if (file == NULL) {
    yf_seterr(YF_ERR_NOFILE, __func__);
    return -1;
  }
  if (verify_file(file) != 0) {
    fclose(file);
    return -1;
  }
  rewind(file);

  /* font directory */
  sfnt.dir = calloc(1, sizeof(L_dir));
  if (sfnt.dir == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }

  /* offset subtable */
  if (fread(&sfnt.dir->diro, YF_SFNT_DIROSZ, 1, file) < 1) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }
  if (sfnt.dir->diro.version != YF_SFNT_TTF) {
    yf_seterr(YF_ERR_UNSUP, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }
  const uint16_t tab_n = be16toh(sfnt.dir->diro.tab_n);

  /* directory entries */
  sfnt.dir->dires = malloc(tab_n * sizeof(L_dire));
  if (sfnt.dir->dires == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }
  if (fread(sfnt.dir->dires, sizeof(L_dire), tab_n, file) < tab_n) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }

  const uint32_t cmap_tag = YF_SFNT_CMAPTAG;
  uint32_t cmap_off = 0;
  uint32_t cmap_len = 0;
  const uint32_t head_tag = YF_SFNT_HEADTAG;
  uint32_t head_off = 0;
  uint32_t head_len = 0;
  const uint32_t hhea_tag = YF_SFNT_HHEATAG;
  uint32_t hhea_off = 0;
  uint32_t hhea_len = 0;
  const uint32_t hmtx_tag = YF_SFNT_HMTXTAG;
  uint32_t hmtx_off = 0;
  uint32_t hmtx_len = 0;
  const uint32_t maxp_tag = YF_SFNT_MAXPTAG;
  uint32_t maxp_off = 0;
  uint32_t maxp_len = 0;
  const uint32_t name_tag = YF_SFNT_NAMETAG;
  uint32_t name_off = 0;
  uint32_t name_len = 0;
  const uint32_t os2_tag = YF_SFNT_OS2TAG;
  uint32_t os2_off = 0;
  uint32_t os2_len = 0;
  const uint32_t post_tag = YF_SFNT_POSTTAG;
  uint32_t post_off = 0;
  uint32_t post_len = 0;

  for (uint16_t i = 0; i < tab_n; ++i) {
    const uint32_t tag = be32toh(sfnt.dir->dires[i].tag);
    if (tag == cmap_tag) {
      cmap_off = be32toh(sfnt.dir->dires[i].off);
      cmap_len = be32toh(sfnt.dir->dires[i].len);
    } else if (tag == head_tag) {
      head_off = be32toh(sfnt.dir->dires[i].off);
      head_len = be32toh(sfnt.dir->dires[i].len);
    } else if (tag == hhea_tag) {
      hhea_off = be32toh(sfnt.dir->dires[i].off);
      hhea_len = be32toh(sfnt.dir->dires[i].len);
    } else if (tag == hmtx_tag) {
      hmtx_off = be32toh(sfnt.dir->dires[i].off);
      hmtx_len = be32toh(sfnt.dir->dires[i].len);
    } else if (tag == maxp_tag) {
      maxp_off = be32toh(sfnt.dir->dires[i].off);
      maxp_len = be32toh(sfnt.dir->dires[i].len);
    } else if (tag == name_tag) {
      name_off = be32toh(sfnt.dir->dires[i].off);
      name_len = be32toh(sfnt.dir->dires[i].len);
    } else if (tag == os2_tag) {
      os2_off = be32toh(sfnt.dir->dires[i].off);
      os2_len = be32toh(sfnt.dir->dires[i].len);
    } else if (tag == post_tag) {
      post_off = be32toh(sfnt.dir->dires[i].off);
      post_len = be32toh(sfnt.dir->dires[i].len);
    }
  }
  if (cmap_off == 0 || head_off == 0 || hhea_off == 0 || hmtx_off == 0 ||
      maxp_off == 0 || name_off == 0 || os2_off == 0 || post_off == 0 ||
      cmap_len < YF_SFNT_CMAPHSZ || head_len != YF_SFNT_HEADSZ ||
      hhea_len != YF_SFNT_HHEASZ || hmtx_len < YF_SFNT_HMTXESZ ||
      maxp_len != YF_SFNT_MAXPSZ || name_len < YF_SFNT_NAMEHSZ ||
      os2_len < YF_SFNT_OS2V0 || post_len < YF_SFNT_POSTHSZ)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }

  /* head table */
  sfnt.head = calloc(1, sizeof(L_head));
  if (sfnt.head == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }
  if (fseek(file, head_off, SEEK_SET) != 0 ||
      fread(sfnt.head, head_len, 1, file) < 1)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }

  /* hhea table */
  sfnt.hhea = calloc(1, sizeof(L_hhea));
  if (sfnt.hhea == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }
  if (fseek(file, hhea_off, SEEK_SET) != 0 ||
      fread(sfnt.hhea, hhea_len, 1, file) < 1)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }
  const uint16_t hmetric_n = be16toh(sfnt.hhea->hmetric_n);

  /* maxp table */
  sfnt.maxp = calloc(1, sizeof(L_maxp));
  if (sfnt.maxp == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }
  if (fseek(file, maxp_off, SEEK_SET) != 0 ||
      fread(sfnt.maxp, maxp_len, 1, file) < 1)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }
  const uint16_t glyph_n = be16toh(sfnt.maxp->glyph_n);

  /* hmtx table */
  sfnt.hmtx = calloc(1, sizeof(L_hmtx));
  if (sfnt.hmtx == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }

  /* hmtx entries */
  sfnt.hmtx->hmtxes = malloc(hmetric_n * sizeof(L_hmtxe));
  if (sfnt.hmtx->hmtxes == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }
  if (fseek(file, hmtx_off, SEEK_SET) != 0 ||
      fread(sfnt.hmtx->hmtxes, sizeof(L_hmtxe), hmetric_n, file) < hmetric_n)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }
  if (hmetric_n < glyph_n) {
    const uint16_t n = glyph_n - hmetric_n;
    sfnt.hmtx->lsbs = malloc(n * sizeof(int16_t));
    if (sfnt.hmtx->lsbs == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      deinit_tables(&sfnt);
      fclose(file);
      return -1;
    }
    if (fread(sfnt.hmtx->lsbs, sizeof(int16_t), n, file) < n) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      deinit_tables(&sfnt);
      fclose(file);
      return -1;
    }
  }

  /* os2 table */
  sfnt.os2 = calloc(1, sizeof(L_os2));
  if (sfnt.os2 == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }
  if (fseek(file, os2_off, SEEK_SET) != 0 ||
      fread(sfnt.os2, os2_len, 1, file) < 1)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }

  /* post table */
  sfnt.post = calloc(1, sizeof(L_post));
  if (sfnt.post == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }
  if (fseek(file, post_off, SEEK_SET) != 0 ||
      fread(&sfnt.post->posth, YF_SFNT_POSTHSZ, 1, file) < 1)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }

  /* cmap table */
  sfnt.cmap = calloc(1, sizeof(L_cmap));
  if (sfnt.cmap == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }
  if (fseek(file, cmap_off, SEEK_SET) != 0 ||
      fread(&sfnt.cmap->cmaph, YF_SFNT_CMAPHSZ, 1, file) < 1)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }
  const uint16_t cmap_n = be16toh(sfnt.cmap->cmaph.tab_n);

  /* cmap entries */
  sfnt.cmap->cmapes = malloc(cmap_n * sizeof(L_cmape));
  if (sfnt.cmap->cmapes == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }
  if (fread(sfnt.cmap->cmapes, sizeof(L_cmape), cmap_n, file) < cmap_n) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }

  /* name table */
  sfnt.name = calloc(1, sizeof(L_name));
  if (sfnt.name == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }
  if (fseek(file, name_off, SEEK_SET) != 0 ||
      fread(&sfnt.name->nameh, YF_SFNT_NAMEHSZ, 1, file) < 1)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }
  const uint16_t name_n = be16toh(sfnt.name->nameh.count);

  /* name entries */
  sfnt.name->namees = malloc(name_n * sizeof(L_namee));
  if (sfnt.name->namees == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }
  if (fread(sfnt.name->namees, sizeof(L_namee), name_n, file) < name_n) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }
  if (sfnt.name->nameh.format != 0) {
    if (fread(&sfnt.name->lang_n, sizeof(uint16_t), 1, file) < 1) {
      yf_seterr(YF_ERR_INVFILE, __func__);
      deinit_tables(&sfnt);
      fclose(file);
      return -1;
    }
    const uint16_t n = be16toh(sfnt.name->lang_n);
    sfnt.name->namels = malloc(n * sizeof(L_namel));
    if (sfnt.name->namels == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      deinit_tables(&sfnt);
      fclose(file);
      return -1;
    }
    if (fread(sfnt.name->namels, sizeof(L_namel), n, file) < n) {
      yf_seterr(YF_ERR_INVFILE, __func__);
      deinit_tables(&sfnt);
      fclose(file);
      return -1;
    }
  }

  /* TODO: Check if this is a ttf file before this call. */
  if (load_ttf(&sfnt, file) != 0) {
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }

  L_font *font = calloc(1, sizeof *font);
  if (font == NULL) {
    deinit_tables(&sfnt);
    fclose(file);
    return -1;
  }
  font->glyph_n = be16toh(sfnt.maxp->glyph_n);
  font->pt_max = be16toh(sfnt.maxp->pt_max);
  font->contr_max = be16toh(sfnt.maxp->contr_max);
  font->comp_pt_max = be16toh(sfnt.maxp->comp_pt_max);
  font->comp_contr_max = be16toh(sfnt.maxp->comp_contr_max);
  font->comp_elem_max = be16toh(sfnt.maxp->comp_elem_max);

  if (get_metrics(&sfnt, &font->met) != 0 ||
      set_mapping(sfnt.cmap, file, cmap_off, &font->map) != 0)
  {
    deinit_tables(&sfnt);
    free(font);
    fclose(file);
    return -1;
  }

  /* TODO: This is unlikely to be of any use. */
  const uint32_t str_off = name_off + be16toh(sfnt.name->nameh.str_off);
  fill_str(sfnt.name, file, str_off, &font->str);

  if (sfnt.ttf.glyf != NULL && sfnt.ttf.loca != NULL) {
    if (sfnt.head->loca_fmt == 0) {
      /* 16-bit offsets: pre-multiply, byte-swap and copy to dw buffer */
      font->ttf.loca = malloc((font->glyph_n + 1) * sizeof(uint32_t));
      if (font->ttf.loca == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        deinit_font(font);
        deinit_tables(&sfnt);
        fclose(file);
        return -1;
      }
      for (uint32_t i = 0; i <= font->glyph_n; ++i)
        font->ttf.loca[i] = be16toh(sfnt.ttf.loca->off16[i]) << 1;
    } else {
      /* 32-bit offsets: take ownership and byte-swap */
      font->ttf.loca = sfnt.ttf.loca->off32;
      sfnt.ttf.loca->off32 = NULL;
      for (uint32_t i = 0; i <= font->glyph_n; ++i)
        font->ttf.loca[i] = be32toh(font->ttf.loca[i]);
    }
    /* take ownership of raw glyph data */
    font->ttf.glyf = sfnt.ttf.glyf->glyphs;
    sfnt.ttf.glyf->glyphs = NULL;
  } else {
    /* TODO */
    assert(0);
  }

  data->font = font;
  data->glyph = get_glyph;
  data->metrics = scale_metrics;
  data->deinit = deinit_font;

  deinit_tables(&sfnt);
  fclose(file);
  return 0;
}

static int verify_file(FILE *file) {
  assert(!feof(file));

  rewind(file);
  L_diro diro;
  if (fread(&diro, YF_SFNT_DIROSZ, 1, file) < 1) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

  const uint16_t tab_n = be16toh(diro.tab_n);
  L_dire dires[tab_n];
  if (fread(dires, YF_SFNT_DIRESZ, tab_n, file) < tab_n) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

  uint32_t chsum, off, dw_n, *buf = NULL;
  for (uint16_t i = 0; i < tab_n; ++i) {
    off = be32toh(dires[i].off);
    if (fseek(file, off, SEEK_SET) != 0) {
      yf_seterr(YF_ERR_INVFILE, __func__);
      return -1;
    }

    dw_n = (be32toh(dires[i].len) + 3) >> 2;
    buf = malloc(dw_n << 2);
    if (buf == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }

    if (fread(buf, 4, dw_n, file) < dw_n) {
      yf_seterr(YF_ERR_INVFILE, __func__);
      free(buf);
      return -1;
    }

    chsum = 0;
    for (uint32_t j = 0; j < dw_n; ++j)
      chsum += be32toh(buf[j]);
    free(buf);

    if (chsum != be32toh(dires[i].chsum) &&
        be32toh(dires[i].tag) != YF_SFNT_HEADTAG)
    {
      yf_seterr(YF_ERR_INVFILE, __func__);
      return -1;
    }
  }

  return 0;
}

static int load_ttf(L_sfnt *sfnt, FILE *file) {
  assert(sfnt != NULL);
  assert(!feof(file));

  const uint16_t tab_n = be16toh(sfnt->dir->diro.tab_n);

  const uint32_t cvt_tag = YF_SFNT_CVTTAG;
  uint32_t cvt_off = 0;
  uint32_t cvt_len = 0;
  const uint32_t fpgm_tag = YF_SFNT_FPGMTAG;
  uint32_t fpgm_off = 0;
  uint32_t fpgm_len = 0;
  const uint32_t gasp_tag = YF_SFNT_GASPTAG;
  uint32_t gasp_off = 0;
  uint32_t gasp_len = 0;
  const uint32_t glyf_tag = YF_SFNT_GLYFTAG;
  uint32_t glyf_off = 0;
  uint32_t glyf_len = 0;
  const uint32_t loca_tag = YF_SFNT_LOCATAG;
  uint32_t loca_off = 0;
  uint32_t loca_len = 0;
  const uint32_t prep_tag = YF_SFNT_PREPTAG;
  uint32_t prep_off = 0;
  uint32_t prep_len = 0;

  for (uint16_t i = 0; i < tab_n; ++i) {
    const uint32_t tag = be32toh(sfnt->dir->dires[i].tag);
    if (tag == cvt_tag) {
      cvt_off = be32toh(sfnt->dir->dires[i].off);
      cvt_len = be32toh(sfnt->dir->dires[i].len);
    } else if (tag == fpgm_tag) {
      fpgm_off = be32toh(sfnt->dir->dires[i].off);
      fpgm_len = be32toh(sfnt->dir->dires[i].len);
    } else if (tag == gasp_tag) {
      gasp_off = be32toh(sfnt->dir->dires[i].off);
      gasp_len = be32toh(sfnt->dir->dires[i].len);
    } else if (tag == glyf_tag) {
      glyf_off = be32toh(sfnt->dir->dires[i].off);
      glyf_len = be32toh(sfnt->dir->dires[i].len);
    } else if (tag == loca_tag) {
      loca_off = be32toh(sfnt->dir->dires[i].off);
      loca_len = be32toh(sfnt->dir->dires[i].len);
    } else if (tag == prep_tag) {
      prep_off = be32toh(sfnt->dir->dires[i].off);
      prep_len = be32toh(sfnt->dir->dires[i].len);
    }
  }
  if (glyf_off == 0 || loca_off == 0) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

  /* cvt table (optional) */
  if (cvt_off != 0) {
    sfnt->ttf.cvt = calloc(1, sizeof(L_cvt));
    if (sfnt->ttf.cvt == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    sfnt->ttf.cvt->ctrl_vals = malloc(cvt_len);
    if (sfnt->ttf.cvt->ctrl_vals == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    if (fseek(file, cvt_off, SEEK_SET) != 0 ||
        fread(sfnt->ttf.cvt->ctrl_vals, cvt_len, 1, file) < 1)
    {
      yf_seterr(YF_ERR_INVFILE, __func__);
      return -1;
    }
  }

  /* fpgm table (optional) */
  if (fpgm_off != 0) {
    sfnt->ttf.fpgm = calloc(1, sizeof(L_fpgm));
    if (sfnt->ttf.fpgm == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    sfnt->ttf.fpgm->instrs = malloc(fpgm_len);
    if (sfnt->ttf.fpgm->instrs == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    if (fseek(file, fpgm_off, SEEK_SET) != 0 ||
        fread(sfnt->ttf.fpgm->instrs, fpgm_len, 1, file) < 1)
    {
      yf_seterr(YF_ERR_INVFILE, __func__);
      return -1;
    }
  }

  /* prep table (optional) */
  if (prep_off != 0) {
    sfnt->ttf.prep = calloc(1, sizeof(L_prep));
    if (sfnt->ttf.prep == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    sfnt->ttf.prep->program = malloc(prep_len);
    if (sfnt->ttf.prep->program == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    if (fseek(file, prep_off, SEEK_SET) != 0 ||
        fread(sfnt->ttf.prep->program, prep_len, 1, file) < 1)
    {
      yf_seterr(YF_ERR_INVFILE, __func__);
      return -1;
    }
  }

  /* gasp table (optional) */
  if (gasp_off != 0 && gasp_len >= YF_SFNT_GASPHSZ) {
    sfnt->ttf.gasp = calloc(1, sizeof(L_gasp));
    if (sfnt->ttf.gasp == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    if (fseek(file, gasp_off, SEEK_SET) != 0 ||
        fread(&sfnt->ttf.gasp->gasph, YF_SFNT_GASPHSZ, 1, file) < 1)
    {
      yf_seterr(YF_ERR_INVFILE, __func__);
      return -1;
    }
    const uint16_t rng_n = be16toh(sfnt->ttf.gasp->gasph.rng_n);
    if (rng_n > 0) {
      sfnt->ttf.gasp->gaspes = malloc(rng_n * sizeof(L_gaspe));
      if (sfnt->ttf.gasp->gaspes == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
      }
      if (fread(sfnt->ttf.gasp->gaspes, sizeof(L_gaspe), rng_n, file) < rng_n) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
      }
    }
  }

  /* loca table */
  sfnt->ttf.loca = calloc(1, sizeof(L_loca));
  if (sfnt->ttf.loca == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  void *loca_data = malloc(loca_len);
  if (loca_data == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  if (fseek(file, loca_off, SEEK_SET) != 0 ||
      fread(loca_data, loca_len, 1, file) < 1)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    free(loca_data);
    return -1;
  }
  if (sfnt->head->loca_fmt == 0)
    sfnt->ttf.loca->off16 = loca_data;
  else
    sfnt->ttf.loca->off32 = loca_data;

  /* glyf table */
  sfnt->ttf.glyf = calloc(1, sizeof(L_glyf));
  if (sfnt->ttf.glyf == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  sfnt->ttf.glyf->glyphs = malloc(glyf_len);
  if (sfnt->ttf.glyf->glyphs == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  if (fseek(file, glyf_off, SEEK_SET) != 0 ||
      fread(sfnt->ttf.glyf->glyphs, glyf_len, 1, file) < 1)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

  return 0;
}

static void deinit_tables(L_sfnt *sfnt) {
  if (sfnt == NULL)
    return;

  if (sfnt->dir != NULL) {
    free(sfnt->dir->dires);
    free(sfnt->dir);
  }
  if (sfnt->cmap != NULL) {
    free(sfnt->cmap->cmapes);
    free(sfnt->cmap);
  }
  free(sfnt->head);
  free(sfnt->hhea);
  if (sfnt->hmtx != NULL) {
    free(sfnt->hmtx->hmtxes);
    free(sfnt->hmtx->lsbs);
    free(sfnt->hmtx);
  }
  free(sfnt->maxp);
  if (sfnt->name != NULL) {
    free(sfnt->name->namees);
    free(sfnt->name->namels);
    free(sfnt->name);
  }
  free(sfnt->os2);
  free(sfnt->post);

  if (sfnt->ttf.cvt != NULL) {
    free(sfnt->ttf.cvt->ctrl_vals);
    free(sfnt->ttf.cvt);
  }
  if (sfnt->ttf.fpgm != NULL) {
    free(sfnt->ttf.fpgm->instrs);
    free(sfnt->ttf.fpgm);
  }
  if (sfnt->ttf.gasp != NULL) {
    free(sfnt->ttf.gasp->gaspes);
    free(sfnt->ttf.gasp);
  }
  if (sfnt->ttf.glyf != NULL) {
    free(sfnt->ttf.glyf->glyphs);
    free(sfnt->ttf.glyf);
  }
  if (sfnt->ttf.loca != NULL) {
    free(sfnt->ttf.loca->off16);
    free(sfnt->ttf.loca);
  }
  if (sfnt->ttf.prep != NULL) {
    free(sfnt->ttf.prep->program);
    free(sfnt->ttf.prep);
  }

  /* XXX: The 'sfnt' ptr itself is not freed, since this structure is
     expected to be allocated from the stack. */
}

static int get_metrics(const L_sfnt *sfnt, L_fontmet *fmet) {
  assert(sfnt != NULL);
  assert(fmet != NULL);

  fmet->upem = be16toh(sfnt->head->upem);
  fmet->x_min = be16toh(sfnt->head->x_min);
  fmet->y_min = be16toh(sfnt->head->y_min);
  fmet->x_max = be16toh(sfnt->head->x_max);
  fmet->y_max = be16toh(sfnt->head->y_max);
  fmet->ascent = be16toh(sfnt->hhea->ascent);
  fmet->descent = be16toh(sfnt->hhea->descent);
  fmet->line_gap = be16toh(sfnt->hhea->line_gap);
  fmet->adv_wdt_max = be16toh(sfnt->hhea->adv_wdt_max);
  fmet->lsb_min = be16toh(sfnt->hhea->lsb_min);
  fmet->rsb_min = be16toh(sfnt->hhea->rsb_min);

  const uint16_t hmetric_n = be16toh(sfnt->hhea->hmetric_n);
  const uint16_t glyph_n = be16toh(sfnt->maxp->glyph_n);

  fmet->glyphs = malloc(glyph_n * sizeof *fmet->glyphs);
  if (fmet->glyphs == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }

  uint16_t i;
  for (i = 0; i < hmetric_n; ++i) {
    fmet->glyphs[i].adv_wdt = be16toh(sfnt->hmtx->hmtxes[i].adv_wdt);
    fmet->glyphs[i].lsb = be16toh(sfnt->hmtx->hmtxes[i].lsb);
  }
  for (; i < glyph_n; ++i) {
    fmet->glyphs[i].adv_wdt = fmet->glyphs[hmetric_n-1].adv_wdt;
    fmet->glyphs[i].lsb = be16toh(sfnt->hmtx->lsbs[i-hmetric_n]);
  }
  return 0;
}

static int set_mapping(const L_cmap *cmap, FILE *file, uint32_t off,
    L_fontmap *fmap)
{
  assert(cmap != NULL);
  assert(!feof(file));
  assert(fmap != NULL);

  /* encodings */
  const struct { uint16_t plat, encd, fmt, lang; } encds[] = {
    {0, 3, 4, 0}, /* Unicode (sparse) */
    {1, 0, 6, 0}, /* Macintosh (roman, trimmed) */
    {3, 1, 4, 0}  /* Windows (sparse) */
  };

  const uint16_t tab_n = be16toh(cmap->cmaph.tab_n);
  const uint16_t encd_n = sizeof encds / sizeof encds[0];
  uint16_t encd_i = UINT16_MAX;

  uint32_t sub_off = 0;
  struct { uint16_t fmt, len, lang; } sub_hdr;
  static_assert(sizeof sub_hdr == 6, "!sizeof");

  for (uint16_t i = 0; i < encd_n; ++i) {
    for (uint16_t j = 0; j < tab_n; ++j) {
      if (be16toh(cmap->cmapes[j].platform) != encds[i].plat ||
          be16toh(cmap->cmapes[j].encoding) != encds[i].encd)
        continue;

      sub_off = off + be32toh(cmap->cmapes[j].off);
      if (fseek(file, sub_off, SEEK_SET) != 0 ||
          fread(&sub_hdr, sizeof sub_hdr, 1, file) < 1)
      {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
      }

      if (be16toh(sub_hdr.fmt) == encds[i].fmt) {
        encd_i = j;
        i = UINT16_MAX-1;
        break;
      }
    }
  }

  if (encd_i == UINT16_MAX) {
    yf_seterr(YF_ERR_UNSUP, __func__);
    return -1;
  }

  switch (be16toh(sub_hdr.fmt)) {
    case 0:
      /* TODO */
      assert(0);
      return -1;

    case 4: {
      /* sparse format */
      struct { uint16_t seg_cnt_x2, search_rng, entry_sel, rng_shf; } sub_4;
      static_assert(sizeof sub_4 == 8, "!sizeof");

      if (fread(&sub_4, sizeof sub_4, 1, file) < 1) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
      }
      const uint16_t seg_cnt = be16toh(sub_4.seg_cnt_x2) >> 1;
      const size_t len = be16toh(sub_hdr.len) - (sizeof sub_hdr+sizeof sub_4);
      uint16_t *var = malloc(len);
      if (var == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
      }
      if (fread(var, len, 1, file) < 1) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        free(var);
        return -1;
      }

      YF_hashset glyph_ids = yf_hashset_init(hash_fmap, cmp_fmap);
      if (glyph_ids == NULL) {
        free(var);
        return -1;
      }
      uint16_t end_code, start_code, code, delta, rng_off, id;
      for (uint16_t i = 0; var[i] != 0xffff; ++i) {
        end_code = be16toh(var[i]);
        start_code = code = be16toh(var[seg_cnt+i+1]);
        delta = be16toh(var[2*seg_cnt+i+1]);
        rng_off = be16toh(var[3*seg_cnt+i+1]);
        if (rng_off != 0) {
          do {
            id = var[3*seg_cnt+i+1 + (rng_off>>1) + (code-start_code)];
            id = be16toh(id);
            yf_hashset_insert(glyph_ids,
                (const void *)((uintptr_t)code | (id << 16)));
          } while (code++ < end_code);
        } else {
          do {
            id = delta + code;
            yf_hashset_insert(glyph_ids,
                (const void *)((uintptr_t)code | (id << 16)));
          } while (code++ < end_code);
        }
      }
      free(var);
      fmap->map = YF_SFNT_MAP_SPARSE;
      fmap->sparse.glyph_ids = glyph_ids;
    } break;

    case 6: {
      /* trimmed format */
      struct { uint16_t first_code, entry_n; } sub_6;
      static_assert(sizeof sub_6 == 4, "!sizeof");

      if (fread(&sub_6, sizeof sub_6, 1, file) < 1) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
      }
      const uint16_t first_code = be16toh(sub_6.first_code);
      const uint16_t entry_n = be16toh(sub_6.entry_n);
      uint16_t *glyph_ids = malloc(entry_n * sizeof *glyph_ids);
      if (glyph_ids == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
      }
      if (fread(glyph_ids, sizeof *glyph_ids, entry_n, file) < entry_n) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        free(glyph_ids);
        return -1;
      }
      fmap->map = YF_SFNT_MAP_TRIMMED;
      fmap->trimmed.first_code = first_code;
      fmap->trimmed.entry_n = entry_n;
      for (uint16_t i = 0; i < entry_n; ++i)
        glyph_ids[i] = be16toh(glyph_ids[i]);
      fmap->trimmed.glyph_ids = glyph_ids;
    } break;

    default:
      assert(0);
      return -1;
  }

  return 0;
}

static int fill_str(const L_name *name, FILE *file, uint32_t str_off,
    L_fontstr *fstr)
{
  assert(name != NULL);
  assert(!feof(file));
  assert(fstr != NULL);

  /* TODO: Select correct platform-encoding-language combination. */
  const uint16_t plat = htobe16(1);
  const uint16_t encd = 0;
  const uint16_t lang = 0;

  const uint16_t name_n = be16toh(name->nameh.count);
  uint16_t name_i = 0;

  for (; name_i < name_n; ++name_i) {
    if (name->namees[name_i].platform == plat)
      break;
  }
  for (; name_i < name_n; ++name_i) {
    if (name->namees[name_i].encoding == encd)
      break;
  }
  for (; name_i < name_n; ++name_i) {
    if (name->namees[name_i].language == lang)
      break;
  }

  if (name_i >= name_n) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

  uint16_t nm, len, off;
  char **str_p;

  for (; name_i < name_n; ++name_i) {
    if (name->namees[name_i].platform != plat ||
        name->namees[name_i].encoding != encd ||
        name->namees[name_i].language != lang)
      break;

    nm = be16toh(name->namees[name_i].name);
    len = be16toh(name->namees[name_i].len);
    off = be16toh(name->namees[name_i].off);

    switch (nm) {
      case 0:
        str_p = &fstr->copyright;
        break;
      case 1:
        str_p = &fstr->family;
        break;
      case 2:
        str_p = &fstr->subfamily;
        break;
      case 3:
        str_p = &fstr->uid;
        break;
      case 4:
        str_p = &fstr->name;
        break;
      case 5:
        str_p = &fstr->version;
        break;
      case 7:
        str_p = &fstr->trademark;
        break;
      case 8:
        str_p = &fstr->manufacturer;
        break;
      case 9:
        str_p = &fstr->designer;
        break;
      case 10:
        str_p = &fstr->description;
        break;
      case 13:
        str_p = &fstr->license;
        break;
      case 16:
        str_p = &fstr->typographic_family;
        break;
      case 17:
        str_p = &fstr->typographic_subfamily;
        break;
      case 19:
        str_p = &fstr->sample_text;
        break;
      default:
        continue;
    }

    *str_p = malloc(len+1);
    if (*str_p == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    (*str_p)[len] = '\0';
    if (fseek(file, str_off+off, SEEK_SET) != 0 ||
        fread(*str_p, len, 1, file) < 1)
    {
      yf_seterr(YF_ERR_INVFILE, __func__);
      return -1;
    }
  }

  return 0;
}

static size_t hash_fmap(const void *x) {
  return ((uintptr_t)x & 0xffff) ^ 0xa993;
}

static int cmp_fmap(const void *a, const void *b) {
  /*return (uint16_t)a - (uint16_t)b;*/
  return ((uintptr_t)a & 0xffff) - ((uintptr_t)b & 0xffff);
}

static void deinit_font(void *font) {
  if (font == NULL)
    return;

  L_font *fnt = font;

  free(fnt->str.copyright);
  free(fnt->str.family);
  free(fnt->str.subfamily);
  free(fnt->str.uid);
  free(fnt->str.name);
  free(fnt->str.version);
  free(fnt->str.trademark);
  free(fnt->str.manufacturer);
  free(fnt->str.designer);
  free(fnt->str.description);
  free(fnt->str.license);
  free(fnt->str.typographic_family);
  free(fnt->str.typographic_subfamily);
  free(fnt->str.sample_text);

  switch (fnt->map.map) {
    case YF_SFNT_MAP_SPARSE:
      yf_hashset_deinit(fnt->map.sparse.glyph_ids);
      break;
    case YF_SFNT_MAP_TRIMMED:
      free(fnt->map.trimmed.glyph_ids);
      break;
  }

  free(fnt->met.glyphs);

  free(fnt->ttf.loca);
  free(fnt->ttf.glyf);

  free(font);
}

static void scale_metrics(void *font, uint16_t pt, uint16_t dpi,
    int16_t *x_min, int16_t *y_min, int16_t *x_max, int16_t *y_max)
{
  assert(font != NULL);

  L_font *fnt = font;
  const float scale = (float)(pt*dpi) / (float)(fnt->met.upem*72);

  if (x_min != NULL)
    *x_min = round(fnt->met.x_min*scale);
  if (y_min != NULL)
    *y_min = round(fnt->met.y_min*scale);
  if (x_max != NULL)
    *x_max = round(fnt->met.x_max*scale);
  if (y_max != NULL)
    *y_max = round(fnt->met.y_max*scale);
}

/* Checks whether a glyph is made of parts (compound/composite). */
#define YF_SFNT_ISCOMPND(fnt, id) \
  (be16toh(((L_glyfd *)((fnt)->ttf.glyf+((fnt)->ttf.loca[id])))->contr_n) \
  > 0x7fff)

/* Component of a simple glyph. */
typedef struct {
  /* indices of last points, one per contour */
  uint16_t *ends;
  uint16_t end_n;
  /* point list */
  struct { int on_curve; int32_t x, y; } *pts;
  uint16_t pt_n;
} L_component;

/* Complete outline of a glyph. */
typedef struct {
  /* (pt*dpi)/(upem*72) */
  float scale;
  /* metrics */
  uint16_t adv_wdt;
  int16_t lsb;
  /* boundaries */
  int32_t x_min;
  int32_t y_min;
  int32_t x_max;
  int32_t y_max;
  /* component list */
  L_component *comps;
  uint16_t comp_n;
} L_outline;

/* Fetches glyph data to produce an outline. */
static int fetch_glyph(L_font *font, wchar_t code, L_outline *outln);

/* Fetches a simple glyph. */
static int fetch_simple(L_font *font, uint16_t id, L_component *comp);

/* Fetches a compound glyph. */
static int fetch_compnd(L_font *font, uint16_t id, L_component *comps,
    uint16_t *comp_i);

/* Deinitializes an outline. */
static void deinit_outline(L_outline *outln);

/* Scales an outline. */
static int scale_outline(L_outline *outln);

/* Grid-fits a scaled outline. */
static int grid_fit(L_outline *outln);

/* Rasterizes an outline to produce a glyph. */
static int rasterize(L_outline *outln, YF_glyph *glyph);

static int get_glyph(void *font, wchar_t code, uint16_t pt, uint16_t dpi,
    YF_glyph *glyph)
{
  assert(font != NULL);
  assert(pt != 0 && dpi != 0);
  assert(glyph != NULL);

  int r = 0;
  L_outline outln = {0};
  outln.scale = (float)(pt*dpi) / (float)(((L_font *)font)->met.upem*72);

  if (fetch_glyph(font, code, &outln) != 0 ||
      scale_outline(&outln) != 0 ||
      grid_fit(&outln) != 0 ||
      rasterize(&outln, glyph) != 0)
    r = -1;

  deinit_outline(&outln);
  return r;
}

static int fetch_glyph(L_font *font, wchar_t code, L_outline *outln) {
  assert(font != NULL);
  assert(font->ttf.loca != NULL);
  assert(font->ttf.glyf != NULL);
  assert(outln != NULL);

  uint16_t id;
  if (font->map.map == YF_SFNT_MAP_SPARSE) {
    YF_hashset hset = font->map.sparse.glyph_ids;
    const void *key = (const void *)(uintptr_t)code;
    if (!yf_hashset_contains(hset, key)) {
      yf_seterr(YF_ERR_NOTFND, __func__);
      return -1;
    }
    /* the glyph ID is encoded in the 3rd and 4th bytes of the value ptr */
    id = (uintptr_t)yf_hashset_search(font->map.sparse.glyph_ids, key) >> 16;
  } else {
    const uint16_t first = font->map.trimmed.first_code;
    const uint16_t n = font->map.trimmed.entry_n;
    const uint16_t *ids = font->map.trimmed.glyph_ids;
    if (code < first || code >= first+n) {
      yf_seterr(YF_ERR_NOTFND, __func__);
      return -1;
    }
    id = ids[code-first];
  }

  const uint32_t off = font->ttf.loca[id];
  /*const uint32_t len = font->ttf.loca[id+1] - off;*/
  assert((off % _Alignof(L_glyfd)) == 0);
  const L_glyfd *gd = (L_glyfd *)(font->ttf.glyf+off);

  outln->x_min = (int16_t)be16toh(gd->x_min);
  outln->y_min = (int16_t)be16toh(gd->y_min);
  outln->x_max = (int16_t)be16toh(gd->x_max);
  outln->y_max = (int16_t)be16toh(gd->y_max);

  if (YF_SFNT_ISCOMPND(font, id)) {
    /* allocate max. components and let callee update the count */
    outln->comp_n = 0;
    outln->comps = calloc(font->comp_elem_max, sizeof *outln->comps);
    if (outln->comps == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    if (fetch_compnd(font, id, outln->comps, &outln->comp_n) != 0)
      return -1;
  } else {
    /* one component suffices */
    outln->comp_n = 1;
    outln->comps = calloc(outln->comp_n, sizeof *outln->comps);
    if (outln->comps == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    if (fetch_simple(font, id, outln->comps) != 0)
      return -1;
  }

  outln->adv_wdt = font->met.glyphs[id].adv_wdt;
  outln->lsb = font->met.glyphs[id].lsb;
  return 0;
}

static int fetch_simple(L_font *font, uint16_t id, L_component *comp) {
  assert(font != NULL);
  assert(comp != NULL);

  uint32_t off = font->ttf.loca[id];
  const L_glyfd *gd = (L_glyfd *)(font->ttf.glyf+off);

  const int16_t contr_n = be16toh(gd->contr_n);
  if (contr_n == 0) {
    /* TODO */
    assert(0);
  }

  comp->end_n = contr_n;
  comp->ends = malloc(comp->end_n * sizeof *comp->ends);
  if (comp->ends == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  for (uint16_t i = 0; i < comp->end_n; ++i)
    comp->ends[i] = be16toh(((uint16_t *)gd->data)[i]);

  comp->pt_n = comp->ends[contr_n-1] + 1;
  comp->pts = malloc(comp->pt_n * sizeof *comp->pts);
  if (comp->pts == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }

  const uint16_t instr_len = be16toh(((uint16_t *)gd->data)[contr_n]);

  off = contr_n*sizeof(uint16_t) + sizeof instr_len + instr_len;
  int32_t flag_n = comp->ends[contr_n-1];
  uint8_t flags = 0;
  uint8_t repeat_n = 0;
  uint32_t y_off = 0;

  /* compute offsets from flags array */
  do {
    flags = gd->data[off++];
    repeat_n = (flags & 8) ? gd->data[off++] : 0;
    flag_n -= repeat_n + 1;
    if (flags & 2)
      /* x is byte */
      y_off += repeat_n + 1;
    else if (!(flags & 16))
      /* x is word */
      y_off += (repeat_n + 1) << 1;
    /* x is same value otherwise */
  } while (flag_n >= 0);

  uint32_t x_off = off;
  y_off += off;
  off = contr_n*sizeof(uint16_t) + sizeof instr_len + instr_len;
  flag_n = comp->ends[contr_n-1];
  int on_curve;
  int16_t x = 0;
  int16_t y = 0;
  int16_t d;
  uint16_t pt_i = 0;

  /* fetch point data */
  do {
    flags = gd->data[off++];
    repeat_n = (flags & 8) ? gd->data[off++] : 0;
    flag_n -= repeat_n + 1;

    do {
      on_curve = flags & 1;

      if (flags & 2) {
        if (flags & 16)
          /* x is byte, positive sign */
          x += gd->data[x_off++];
        else
          /* x is byte, negative sign */
          x += -(gd->data[x_off++]);
      } else if (!(flags & 16)) {
        /* x is signed word */
        d = gd->data[x_off+1];
        d = (d << 8) | gd->data[x_off];
        x += be16toh(d);
        x_off += 2;
      }

      if (flags & 4) {
        if (flags & 32)
          /* y is byte, positive sign */
          y += gd->data[y_off++];
        else
          /* y is byte, negative sign */
          y += -(gd->data[y_off++]);
      } else if (!(flags & 32)) {
        /* y is signed word */
        d = gd->data[y_off+1];
        d = (d << 8) | gd->data[y_off];
        y += be16toh(d);
        y_off += 2;
      }

      comp->pts[pt_i].on_curve = on_curve;
      comp->pts[pt_i].x = x;
      comp->pts[pt_i].y = y;
      ++pt_i;
    } while (repeat_n-- > 0);

  } while (flag_n >= 0);
  return 0;
}

static int fetch_compnd(L_font *font, uint16_t id, L_component *comps,
    uint16_t *comp_i)
{
  assert(font != NULL);
  assert(comps != NULL);
  assert(comp_i != NULL);

#undef YF_GETWRD
#define YF_GETWRD(res, gd, off) do { \
  res = gd->data[off+1]; \
  res = ((res) << 8) | gd->data[off]; \
  res = be16toh(res); \
  off += 2; } while (0)

  uint32_t off = font->ttf.loca[id];
  const L_glyfd *gd = (L_glyfd *)(font->ttf.glyf+off);
  off = 0;

  uint16_t idx = *comp_i;
  uint16_t flags;
  uint16_t comp_id;
  uint16_t arg[2];
  int16_t v[4];

  do {
    YF_GETWRD(flags, gd, off);
    YF_GETWRD(comp_id, gd, off);

    if (YF_SFNT_ISCOMPND(font, comp_id)) {
      if (fetch_compnd(font, comp_id, comps, comp_i) != 0)
        return -1;
    } else {
      if (fetch_simple(font, comp_id, comps+((*comp_i)++)) != 0)
        return -1;
    }

    if (flags & 1) {
      /* arguments are words */
      YF_GETWRD(arg[0], gd, off);
      YF_GETWRD(arg[1], gd, off);
    } else {
      /* arguments are bytes */
      arg[0] = gd->data[off++];
      arg[1] = gd->data[off++];
    }

    if (flags & 2) {
      /* arguments are x,y values */
      for (uint16_t i = idx; i < *comp_i; ++i) {
        for (uint16_t j = 0; j < comps[i].pt_n; ++j) {
          comps[i].pts[j].x += arg[0];
          comps[i].pts[j].y += arg[1];
        }
      }
    } else {
      /* arguments are points */
      /* TODO */
      assert(0);
    }

    /* TODO: Transform points. */
    if (flags & 8) {
      /* simple scale */
      YF_GETWRD(v[0], gd, off);
      v[3] = v[0];
      v[1] = v[2] = 0;
    } else if (flags & 64) {
      /* different scales */
      YF_GETWRD(v[0], gd, off);
      YF_GETWRD(v[3], gd, off);
      v[1] = v[2] = 0;
    } else if (flags & 128) {
      /* 2x2 transformation */
      YF_GETWRD(v[0], gd, off);
      YF_GETWRD(v[1], gd, off);
      YF_GETWRD(v[2], gd, off);
      YF_GETWRD(v[3], gd, off);
    } else {
      /* as is */
      v[0] = v[3] = 1;
      v[1] = v[2] = 0;
    }

    ++idx;
  } while (flags & 32);
  return 0;
}

static void deinit_outline(L_outline *outln) {
  if (outln == NULL)
    return;

  if (outln->comps != NULL) {
    for (uint16_t i = 0; i < outln->comp_n; ++i) {
      free(outln->comps[i].ends);
      free(outln->comps[i].pts);
    }
    free(outln->comps);
  }
  /* XXX: 'outln' ptr not freed. */
}

/* 26.6 fixed-point arithmetic used for scaling. */
/* XXX: May require additional bounds check for large point values. */
#define YF_SFNT_Q 6

#define YF_SFNT_INTTOFIX(x) ((x)<<YF_SFNT_Q)
#define YF_SFNT_FIXTOINT(x) \
  (((x)>>YF_SFNT_Q)+((((x)&(1<<(YF_SFNT_Q-1)))>>(YF_SFNT_Q-1))))

#define YF_SFNT_FLTTOFIX(x) ((int32_t)round((float)(x)*(1<<YF_SFNT_Q)))
#define YF_SFNT_FIXTOFLT(x) ((float)(x)*(1.0f/(1<<YF_SFNT_Q)))

#define YF_SFNT_FIXMUL(x, y) ((((x)*(y))+(1<<(YF_SFNT_Q-1)))>>YF_SFNT_Q)
#define YF_SFNT_FIXDIV(x, y) \
  (((x)&(1<<31)) && ((y)&(1<<31)) ? \
    (((x)<<YF_SFNT_Q)+((y)>>1))/(y) : (((x)<<YF_SFNT_Q)-((y)>>1))/(y))

#define YF_SFNT_FIXROUND(x) \
  ((x)&(1<<31) ? \
    ((x)&(~((1<<YF_SFNT_Q)-1)))-(((x)&(1<<(YF_SFNT_Q-1)))<<1) : \
    ((x)&(~((1<<YF_SFNT_Q)-1)))+(((x)&(1<<(YF_SFNT_Q-1)))<<1))

static int scale_outline(L_outline *outln) {
  assert(outln != NULL);
  assert(outln->comps != NULL);

  const float fac = outln->scale;

  /* create scaled points for each contour of each component */
  for (uint16_t i = 0; i < outln->comp_n; ++i) {
    L_component comp;
    comp.ends = outln->comps[i].ends;
    comp.end_n = outln->comps[i].end_n;
    comp.pt_n = outln->comps[i].pt_n << 1;
    comp.pts = malloc(comp.pt_n * sizeof *comp.pts);
    if (comp.pts == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    uint16_t pt_i = 0;
    uint16_t begn = 0;
    uint16_t curr = 0;

    for (uint16_t j = 0; j < comp.end_n; ++j) {
      uint16_t end = comp.ends[j];
      do {
        if (outln->comps[i].pts[curr].on_curve) {
          comp.pts[pt_i].on_curve = 1;
          comp.pts[pt_i].x = YF_SFNT_FLTTOFIX(outln->comps[i].pts[curr].x*fac);
          comp.pts[pt_i].y = YF_SFNT_FLTTOFIX(outln->comps[i].pts[curr].y*fac);
          if (++pt_i == comp.pt_n) {
            comp.pt_n = YF_MIN(comp.pt_n<<1, 0xffff);
            void *tmp = realloc(comp.pts, comp.pt_n * sizeof *comp.pts);
            if (tmp == NULL) {
              yf_seterr(YF_ERR_NOMEM, __func__);
              return -1;
            }
            comp.pts = tmp;
          }
          continue;
        }

        const uint16_t p0_i = curr == begn ? end : curr-1;
        const uint16_t p1_i = curr;
        const uint16_t p2_i = curr == end ? begn : curr+1;

        struct { int on; int32_t x, y; } curve[3] = {
          {
            outln->comps[i].pts[p0_i].on_curve,
            outln->comps[i].pts[p0_i].x,
            outln->comps[i].pts[p0_i].y
          },
          {
            outln->comps[i].pts[p1_i].on_curve,
            outln->comps[i].pts[p1_i].x,
            outln->comps[i].pts[p1_i].y
          },
          {
            outln->comps[i].pts[p2_i].on_curve,
            outln->comps[i].pts[p2_i].x,
            outln->comps[i].pts[p2_i].y
          }
        };

        /* missing on-curve points created as needed */
        if (curve[0].on) {
          curve[0].x = YF_SFNT_FLTTOFIX(curve[0].x*fac);
          curve[0].y = YF_SFNT_FLTTOFIX(curve[0].y*fac);
        } else {
          curve[0].x = YF_SFNT_FLTTOFIX((curve[0].x+curve[1].x)*fac*0.5f);
          curve[0].y = YF_SFNT_FLTTOFIX((curve[0].y+curve[1].y)*fac*0.5f);
        }
        if (curve[2].on) {
          curve[2].x = YF_SFNT_FLTTOFIX(curve[2].x*fac);
          curve[2].y = YF_SFNT_FLTTOFIX(curve[2].y*fac);
        } else {
          curve[2].x = YF_SFNT_FLTTOFIX((curve[1].x+curve[2].x)*fac*0.5f);
          curve[2].y = YF_SFNT_FLTTOFIX((curve[1].y+curve[2].y)*fac*0.5f);
        }
        curve[1].x = YF_SFNT_FLTTOFIX(curve[1].x*fac);
        curve[1].y = YF_SFNT_FLTTOFIX(curve[1].y*fac);

        /* TODO: Dynamic range instead. */
        const int32_t ts = (1<<YF_SFNT_Q)>>2;
        int32_t t = 0;
        while ((t += ts) < (1<<YF_SFNT_Q)) {
          const int32_t diff = (1<<YF_SFNT_Q)-t;
          const int32_t dbl = YF_SFNT_FIXMUL(t, 2<<YF_SFNT_Q);
          const int32_t a = YF_SFNT_FIXMUL(diff, diff);
          const int32_t b = YF_SFNT_FIXMUL(dbl, diff);
          const int32_t c = YF_SFNT_FIXMUL(t, t);
          const int32_t x0 = curve[0].x;
          const int32_t y0 = curve[0].y;
          const int32_t x1 = curve[1].x;
          const int32_t y1 = curve[1].y;
          const int32_t x2 = curve[2].x;
          const int32_t y2 = curve[2].y;

          /* p(t) = (1-t)^2*p0 + 2*t*(1-t)*p1 + t^2*p2 */
          comp.pts[pt_i].x = YF_SFNT_FIXMUL(a, x0) + YF_SFNT_FIXMUL(b, x1) +
            YF_SFNT_FIXMUL(c, x2);
          comp.pts[pt_i].y = YF_SFNT_FIXMUL(a, y0) + YF_SFNT_FIXMUL(b, y1) +
            YF_SFNT_FIXMUL(c, y2);

          /* XXX: Careful with overflow here. */
          if (++pt_i == comp.pt_n) {
            comp.pt_n = YF_MIN(comp.pt_n<<1, 0xffff);
            void *tmp = realloc(comp.pts, comp.pt_n * sizeof *comp.pts);
            if (tmp == NULL) {
              yf_seterr(YF_ERR_NOMEM, __func__);
              return -1;
            }
            comp.pts = tmp;
          }
        }

      } while (curr++ != end);

      begn = end+1;
      comp.ends[j] = pt_i-1;
    }

    free(outln->comps[i].pts);
    if (comp.pt_n > pt_i) {
      comp.pt_n = pt_i;
      void *tmp = realloc(comp.pts, comp.pt_n * sizeof *comp.pts);
      if (tmp != NULL)
        comp.pts = tmp;
    }
    outln->comps[i].pts = comp.pts;
    outln->comps[i].pt_n = comp.pt_n;
  }

  outln->x_min = YF_SFNT_FLTTOFIX(outln->x_min*fac);
  outln->y_min = YF_SFNT_FLTTOFIX(outln->y_min*fac);
  outln->x_max = YF_SFNT_FLTTOFIX(outln->x_max*fac);
  outln->y_max = YF_SFNT_FLTTOFIX(outln->y_max*fac);
  return 0;
}

static int grid_fit(L_outline *outln) {
  assert(outln != NULL);

  /* TODO: Improve this. */

  outln->x_min = YF_SFNT_FIXROUND(outln->x_min);
  outln->y_min = YF_SFNT_FIXROUND(outln->y_min);
  outln->x_max = YF_SFNT_FIXROUND(outln->x_max);
  outln->y_max = YF_SFNT_FIXROUND(outln->y_max);

  for (uint16_t i = 0; i < outln->comp_n; ++i) {
    for (uint16_t j = 0; j < outln->comps[i].pt_n; ++j) {
      const int32_t x = outln->comps[i].pts[j].x;
      outln->comps[i].pts[j].x = YF_SFNT_FIXROUND(x);
      const int32_t y = outln->comps[i].pts[j].y;
      outln->comps[i].pts[j].y = YF_SFNT_FIXROUND(y);
    }
  }
  return 0;
}

/* Contour winding directions. */
#define YF_SFNT_WIND_NONE 0
#define YF_SFNT_WIND_ON   1
#define YF_SFNT_WIND_OFF -1

/* Contour point. */
typedef struct {
  int32_t x;
  int32_t y;
} L_point;

/* Contour Segment. */
typedef struct {
  int wind;
  L_point p1;
  L_point p2;
} L_segment;

static int rasterize(L_outline *outln, YF_glyph *glyph) {
  assert(outln != NULL);
  assert(outln->comps != NULL);
  assert(glyph != NULL);

#undef YF_NEWSEG
#define YF_NEWSEG(seg, comp, i, j) do { \
  const int32_t x1 = (comp).pts[i].x; \
  const int32_t y1 = (comp).pts[i].y; \
  const int32_t x2 = (comp).pts[j].x; \
  const int32_t y2 = (comp).pts[j].y; \
  if (y1 < y2) \
    (seg).wind = YF_SFNT_WIND_ON; \
  else if (y1 > y2) \
    (seg).wind = YF_SFNT_WIND_OFF; \
  else if (x1 < x2) \
    (seg).wind = YF_SFNT_WIND_ON; \
  else \
    (seg).wind = YF_SFNT_WIND_OFF; \
  (seg).p1 = (L_point){x1, y1}; \
  (seg).p2 = (L_point){x2, y2}; } while (0)

  uint32_t seg_max = 0;
  for (uint16_t i = 0; i < outln->comp_n; ++i)
    seg_max += outln->comps[i].ends[outln->comps[i].end_n-1] + 1;

  L_segment *segs = malloc(seg_max * sizeof *segs);
  if (segs == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }

  /* create segments for each contour of each component */
  uint32_t seg_i = 0;
  for (uint16_t i = 0; i < outln->comp_n; ++i) {
    uint16_t curr, end, begn = 0;
    for (uint16_t j = 0; j < outln->comps[i].end_n; ++j) {
      curr = begn;
      end = outln->comps[i].ends[j];
      while (curr++ < end) {
        YF_NEWSEG(segs[seg_i], outln->comps[i], curr-1, curr);
        ++seg_i;
      }
      /* close the contour */
      YF_NEWSEG(segs[seg_i], outln->comps[i], end, begn);
      ++seg_i;
      begn = end+1;
    }
  }

#undef YF_DIRECTION
#define YF_DIRECTION(p1, p2, p3) \
  (YF_SFNT_FIXMUL((p3).x-(p1).x, (p2).y-(p1).y) - \
    YF_SFNT_FIXMUL((p2).x-(p1).x, (p3).y-(p1).y))
/*
  (((p3).x-(p1).x) * ((p2).y-(p1).y) - ((p2).x-(p1).x) * ((p3).y-(p1).y))
*/

#undef YF_ONPOINT
#define YF_ONPOINT(p1, p2, p3) \
  (YF_MIN((p1).x, (p2).x) <= (p3).x && YF_MAX((p1).x, (p2).x) >= (p3).x && \
    YF_MIN((p1).y, (p2).y) <= (p3).y && YF_MAX((p1).y, (p2).y) >= (p3).y)

#undef YF_ONSEG
#define YF_ONSEG(seg, p) \
  (YF_DIRECTION((seg).p1, (seg).p2, (p)) != 0 ? 0 : \
    YF_ONPOINT((seg).p1, (seg).p2, (p)))

#undef YF_INTERSECTS
#define YF_INTERSECTS(res, seg, p1_, p2_) do { \
  const int32_t d1 = YF_DIRECTION(p1_, p2_, (seg).p1); \
  const int32_t d2 = YF_DIRECTION(p1_, p2_, (seg).p2); \
  const int32_t d3 = YF_DIRECTION((seg).p1, (seg).p2, p1_); \
  const int32_t d4 = YF_DIRECTION((seg).p1, (seg).p2, p2_); \
  if (((d1 < 0 && d2 > 0) || (d1 > 0 && d2 < 0)) && \
      ((d3 < 0 && d4 > 0) || (d3 > 0 && d4 < 0))) \
    res = 1; \
  else if (d1 == 0 && YF_ONPOINT(p1_, p2_, (seg).p1)) \
    res = 1; \
  else if (d2 == 0 && YF_ONPOINT(p1_, p2_, (seg).p2)) \
    res = 1; \
  else if (d3 == 0 && YF_ONPOINT((seg).p1, (seg).p2, p1_)) \
    res = 1; \
  else if (d4 == 0 && YF_ONPOINT((seg).p1, (seg).p2, p2_)) \
    res = 1; \
  else \
    res = 0; } while (0)

  /* rasterize */
  const uint32_t w = outln->x_max - outln->x_min;
  const uint32_t h = outln->y_max - outln->y_min;
  uint8_t *bitmap = malloc(YF_SFNT_FIXTOINT(w)*YF_SFNT_FIXTOINT(h));
  if (bitmap == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    free(segs);
    return -1;
  }
  const uint32_t half = 1<<(YF_SFNT_Q-1);
  const uint32_t one = half<<1;
  uint32_t idx = 0;
  for (uint32_t y = half; y < h; y += one) {
    for (uint32_t x = half; x < w; x += one) {
      L_point p1 = {x+outln->x_min, y+outln->y_min};
      L_point p2 = {one+outln->x_max, p1.y};
      int wind = YF_SFNT_WIND_NONE;
      for (uint32_t i = 0; i < seg_i; ++i) {
        if (YF_ONSEG(segs[i], p1)) {
          wind = YF_SFNT_WIND_ON;
          break;
        }
        int isect;
        YF_INTERSECTS(isect, segs[i], p1, p2);
        if (isect)
          wind += segs[i].wind;
      }
      bitmap[idx++] = wind != YF_SFNT_WIND_NONE ? 255 : 0;
    }
  }

  glyph->width = YF_SFNT_FIXTOINT(w);
  glyph->height = YF_SFNT_FIXTOINT(h);
  glyph->bpp = 8;
  glyph->bitmap.u8 = bitmap;
  glyph->base_h = YF_SFNT_FIXTOINT(outln->y_min);
  glyph->adv_wdt = round(outln->scale*outln->adv_wdt);
  glyph->lsb = round(outln->scale*outln->lsb);

  free(segs);
  return 0;
}
