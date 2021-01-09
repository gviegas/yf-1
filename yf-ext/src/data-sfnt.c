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

#include <yf/com/yf-hashset.h>
#include <yf/com/yf-error.h>

#include "data-sfnt.h"

#ifdef _DEFAULT_SOURCE
#include <endian.h>
#else
/* TODO */
# error "Invalid platform"
#endif

#ifndef YF_MIN
# define YF_MIN(a, b) (a < b ? a : b)
#endif
#ifndef YF_MAX
# define YF_MAX(a, b) (a > b ? a : b)
#endif

#ifdef YF_DEBUG
# define YF_SFNT_DIRO_PRINT(do_p) do { \
   printf("\n-- SFNT (debug) --"); \
   printf("\ndiro - version: 0x%.8x", be32toh((do_p)->version)); \
   printf("\ndiro - tab_n: %hu", be16toh((do_p)->tab_n)); \
   printf("\ndiro - search_rng: %hu", be16toh((do_p)->search_rng)); \
   printf("\ndiro - entry_sel: %hu", be16toh((do_p)->entry_sel)); \
   printf("\ndiro - rng_shf: %hu", be16toh((do_p)->rng_shf)); \
   printf("\n--\n"); } while (0)

# define YF_SFNT_DIRE_PRINT(de_p) do { \
   printf("\n-- SFNT (debug) --"); \
   printf("\ndire - tag: %c%c%c%c", \
    (de_p)->tag, (de_p)->tag >> 8, (de_p)->tag >> 16, (de_p)->tag >> 24); \
   printf("\ndire - chsum: %u", be32toh((de_p)->chsum)); \
   printf("\ndire - off: %u", be32toh((de_p)->off)); \
   printf("\ndire - len: %u", be32toh((de_p)->len)); \
   printf("\n--\n"); } while (0)

# define YF_SFNT_CMAPH_PRINT(ch_p) do { \
   printf("\n-- SFNT (debug) --"); \
   printf("\ncmaph - version: %hu", (ch_p)->version); \
   printf("\ncmaph - tab_n: %hu", be16toh((ch_p)->tab_n)); \
   printf("\n--\n"); } while (0)

# define YF_SFNT_CMAPE_PRINT(ce_p) do { \
   printf("\n-- SFNT (debug) --"); \
   printf("\ncmape - platform: %hu", be16toh((ce_p)->platform)); \
   printf("\ncmape - encoding: %hu", be16toh((ce_p)->encoding)); \
   printf("\ncmape - off: %u", be32toh((ce_p)->off)); \
   printf("\n--\n"); } while (0)

# define YF_SFNT_HEAD_PRINT(hd_p) do { \
   printf("\n-- SFNT (debug) --"); \
   printf("\nhead - major: %hu", be16toh((hd_p)->major)); \
   printf("\nhead - minor: %hu", be16toh((hd_p)->minor)); \
   printf("\nhead - revision: %u", (hd_p)->revision); \
   printf("\nhead - chsum_adj: %u", (hd_p)->chsum_adj); \
   printf("\nhead - magic: 0x%.8x", be32toh((hd_p)->magic)); \
   printf("\nhead - flags: %hx", be16toh((hd_p)->flags)); \
   printf("\nhead - upem: %hu", be16toh((hd_p)->upem)); \
   printf("\nhead - created: %ld", \
    ((int64_t)(hd_p)->created[0] << 32) | (int64_t)(hd_p)->created[1]); \
   printf("\nhead - modified: %ld", \
    ((int64_t)(hd_p)->modified[0] << 32) | (int64_t)(hd_p)->modified[1]); \
   printf("\nhead - x_min: %hd", be16toh((hd_p)->x_min)); \
   printf("\nhead - y_min: %hd", be16toh((hd_p)->y_min)); \
   printf("\nhead - x_max: %hd", be16toh((hd_p)->x_max)); \
   printf("\nhead - y_max: %hd", be16toh((hd_p)->y_max)); \
   printf("\nhead - style: %hx", be16toh((hd_p)->style)); \
   printf("\nhead - low_rec_ppm: %hu", be16toh((hd_p)->low_rec_ppem)); \
   printf("\nhead - dir_hint: %hd", be16toh((hd_p)->dir_hint)); \
   printf("\nhead - loca_fmt: %hd", be16toh((hd_p)->loca_fmt)); \
   printf("\nhead - glyph_fmt: %hd", be16toh((hd_p)->glyph_fmt)); \
   printf("\n--\n"); } while (0)

# define YF_SFNT_HHEA_PRINT(hh_p) do { \
   printf("\n-- SFNT (debug) --"); \
   printf("\nhhea - major: %hu", be16toh((hh_p)->major)); \
   printf("\nhhea - minor: %hu", be16toh((hh_p)->minor)); \
   printf("\nhhea - ascender: %hd", be16toh((hh_p)->ascender)); \
   printf("\nhhea - descender: %hd", be16toh((hh_p)->descender)); \
   printf("\nhhea - line_gap: %hd", be16toh((hh_p)->line_gap)); \
   printf("\nhhea - adv_wdt_max: %hu", be16toh((hh_p)->adv_wdt_max)); \
   printf("\nhhea - lbear_min: %hd", be16toh((hh_p)->lbear_min)); \
   printf("\nhhea - rbear_min: %hd", be16toh((hh_p)->rbear_min)); \
   printf("\nhhea - x_extent_max: %hd", be16toh((hh_p)->x_extent_max)); \
   printf("\nhhea - caret_slp_rise: %hd", be16toh((hh_p)->caret_slp_rise)); \
   printf("\nhhea - caret_slp_run: %hd", be16toh((hh_p)->caret_slp_run)); \
   printf("\nhhea - caret_off: %hd", be16toh((hh_p)->caret_off)); \
   printf("\nhhea - metric_fmt: %hd", be16toh((hh_p)->metric_fmt)); \
   printf("\nhhea - hmetric_n: %hu", be16toh((hh_p)->hmetric_n)); \
   printf("\n--\n"); } while (0)

# define YF_SFNT_HMTXE_PRINT(hm_p) do { \
   printf("\n-- SFNT (debug) --"); \
   printf("\nhmtxe - adv_wdt: %hu", be16toh((hm_p)->adv_wdt)); \
   printf("\nhmtxe - lbear: %hd", be16toh((hm_p)->lbear)); \
   printf("\n--\n"); } while (0)

# define YF_SFNT_MAXP_PRINT(mp_p) do { \
   printf("\n-- SFNT (debug) --"); \
   printf("\nmaxp - version: 0x%.8x", be32toh((mp_p)->version)); \
   printf("\nmaxp - glyph_n: %hu", be16toh((mp_p)->glyph_n)); \
   printf("\nmaxp - pt_max: %hu", be16toh((mp_p)->pt_max)); \
   printf("\nmaxp - contr_max: %hu", be16toh((mp_p)->contr_max)); \
   printf("\nmaxp - comp_pt_max: %hu", be16toh((mp_p)->comp_pt_max)); \
   printf("\nmaxp - comp_contr_max: %hu", be16toh((mp_p)->comp_contr_max)); \
   printf("\nmaxp - zone_max: %hu", be16toh((mp_p)->zone_max)); \
   printf("\nmaxp - z0_pt_max: %hu", be16toh((mp_p)->z0_pt_max)); \
   printf("\nmaxp - storage_max: %hu", be16toh((mp_p)->storage_max)); \
   printf("\nmaxp - fdef_max: %hu", be16toh((mp_p)->fdef_max)); \
   printf("\nmaxp - idef_max: %hu", be16toh((mp_p)->idef_max)); \
   printf("\nmaxp - stack_elem_max: %hu", be16toh((mp_p)->stack_elem_max)); \
   printf("\nmaxp - hint_sz_max: %hu", be16toh((mp_p)->hint_sz_max)); \
   printf("\nmaxp - comp_elem_max: %hu", be16toh((mp_p)->comp_elem_max)); \
   printf("\nmaxp - comp_dep_max: %hu", be16toh((mp_p)->comp_dep_max)); \
   printf("\n--\n"); } while (0)

# define YF_SFNT_NAMEH_PRINT(nh_p) do { \
   printf("\n-- SFNT (debug) --"); \
   printf("\nnameh - format: %hu", be16toh((nh_p)->format)); \
   printf("\nnameh - count: %hu", be16toh((nh_p)->count)); \
   printf("\nnameh - str_off: %hu", be16toh((nh_p)->str_off)); \
   printf("\n--\n"); } while (0)

# define YF_SFNT_NAMEE_PRINT(ne_p) do { \
   printf("\n-- SFNT (debug) --"); \
   printf("\nnamee - platform: %hu", be16toh((ne_p)->platform)); \
   printf("\nnamee - encoding: %hu", be16toh((ne_p)->encoding)); \
   printf("\nnamee - language: %hu", be16toh((ne_p)->language)); \
   printf("\nnamee - name: %hu", be16toh((ne_p)->name)); \
   printf("\nnamee - len: %hu", be16toh((ne_p)->len)); \
   printf("\nnamee - off: %hu", be16toh((ne_p)->off)); \
   printf("\n--\n"); } while (0)

# define YF_SFNT_OS2_PRINT(o2_p) do { \
   printf("\n-- SFNT (debug) --"); \
   printf("\nos2 - version: %hu", be16toh((o2_p)->version)); \
   printf("\nos2 - x_avg_char_wdt: %hd", be16toh((o2_p)->x_avg_char_wdt)); \
   printf("\nos2 - wgt_class: %hu", be16toh((o2_p)->wgt_class)); \
   printf("\nos2 - wdt_class: %hu", be16toh((o2_p)->wdt_class)); \
   printf("\nos2 - type: 0x%.4hx", be16toh((o2_p)->type)); \
   printf("\nos2 - y_subsc_x_sz: %hd", be16toh((o2_p)->y_subsc_x_sz)); \
   printf("\nos2 - y_subsc_y_sz: %hd", be16toh((o2_p)->y_subsc_y_sz)); \
   printf("\nos2 - y_subsc_x_off: %hd", be16toh((o2_p)->y_subsc_x_off)); \
   printf("\nos2 - y_subsc_y_off: %hd", be16toh((o2_p)->y_subsc_y_off)); \
   printf("\nos2 - y_supersc_x_sz: %hd", be16toh((o2_p)->y_supersc_x_sz)); \
   printf("\nos2 - y_supersc_y_sz: %hd", be16toh((o2_p)->y_supersc_y_sz)); \
   printf("\nos2 - y_supersc_x_off: %hd", be16toh((o2_p)->y_supersc_x_off)); \
   printf("\nos2 - y_supersc_y_off: %hd", be16toh((o2_p)->y_supersc_y_off)); \
   printf("\nos2 - y_strikeout_sz: %hd", be16toh((o2_p)->y_strikeout_sz)); \
   printf("\nos2 - y_strikeout_pos: %hd", be16toh((o2_p)->y_strikeout_pos)); \
   printf("\nos2 - family_class: %hd", be16toh((o2_p)->family_class)); \
   printf("\nos2 - panose: %u %u %u %u %u %u %u %u %u %u", \
    (o2_p)->panose[0], (o2_p)->panose[1], (o2_p)->panose[2], \
    (o2_p)->panose[3], (o2_p)->panose[4], (o2_p)->panose[5], \
    (o2_p)->panose[6], (o2_p)->panose[7], (o2_p)->panose[8], \
    (o2_p)->panose[9]); \
   printf("\nos2 - unicode_rng: 0x%.8x 0x%.8x 0x%.8x 0x%.8x", \
    (be32toh((o2_p)->unicode_rng1[0]) << 16) | \
     be32toh((o2_p)->unicode_rng1[1]), \
    (be32toh((o2_p)->unicode_rng2[0]) << 16) | \
     be32toh((o2_p)->unicode_rng2[1]), \
    (be32toh((o2_p)->unicode_rng3[0]) << 16) | \
     be32toh((o2_p)->unicode_rng3[1]), \
    (be32toh((o2_p)->unicode_rng4[0]) << 16) | \
     be32toh((o2_p)->unicode_rng4[1])); \
   printf("\nos2 - ach_vend: %c%c%c%c", \
    (o2_p)->ach_vend[0], (o2_p)->ach_vend[1], \
    (o2_p)->ach_vend[2], (o2_p)->ach_vend[3]); \
   printf("\nos2 - sel: 0x%.4hx", be16toh((o2_p)->sel)); \
   printf("\nos2 - first_char_i: %hu", be16toh((o2_p)->first_char_i)); \
   printf("\nos2 - last_char_i: %hu", be16toh((o2_p)->last_char_i)); \
   printf("\nos2 - typo_ascender: %hd", be16toh((o2_p)->typo_ascender)); \
   printf("\nos2 - typo_descender: %hd", be16toh((o2_p)->typo_descender)); \
   printf("\nos2 - typo_line_gap: %hd", be16toh((o2_p)->typo_line_gap)); \
   printf("\nos2 - win_ascent: %hu", be16toh((o2_p)->win_ascent)); \
   printf("\nos2 - win_descent: %hu", be16toh((o2_p)->win_descent)); \
   printf("\nos2 - code_page_rng: 0x%.8x 0x%.8x", \
    (be32toh((o2_p)->code_page_rng1[0]) << 16) | \
     be32toh((o2_p)->code_page_rng1[1]), \
    (be32toh((o2_p)->code_page_rng2[0]) << 16) | \
     be32toh((o2_p)->code_page_rng2[1])); \
   printf("\nos2 - x_hgt: %hd", be16toh((o2_p)->x_hgt)); \
   printf("\nos2 - cap_hgt: %hd", be16toh((o2_p)->cap_hgt)); \
   printf("\nos2 - deft_char: %hu", be16toh((o2_p)->deft_char)); \
   printf("\nos2 - break_char: %hu", be16toh((o2_p)->break_char)); \
   printf("\nos2 - max_ctx: %hu", be16toh((o2_p)->max_ctx)); \
   printf("\nos2 - lo_optical_pt_sz: %hu", be16toh((o2_p)->lo_optical_pt_sz)); \
   printf("\nos2 - up_optical_pt_sz: %hu", be16toh((o2_p)->up_optical_pt_sz)); \
   printf("\n--\n"); } while (0)

# define YF_SFNT_POSTH_PRINT(ps_p) do { \
   printf("\n-- SFNT (debug) --"); \
   printf("\nposth - version: 0x%.8x", be32toh((ps_p)->version)); \
   printf("\nposth - italic_ang: %u", be32toh((ps_p)->italic_ang)); \
   printf("\nposth - undln_pos: %hd", be16toh((ps_p)->undln_pos)); \
   printf("\nposth - undln_thick: %hd", be16toh((ps_p)->undln_thick)); \
   printf("\nposth - fixed_pitch: %u", be32toh((ps_p)->fixed_pitch)); \
   printf("\nposth - mem42_min: %u", be32toh((ps_p)->mem42_min)); \
   printf("\nposth - mem42_max: %u", be32toh((ps_p)->mem42_max)); \
   printf("\nposth - mem1_min: %u", be32toh((ps_p)->mem1_min)); \
   printf("\nposth - mem1_max: %u", be32toh((ps_p)->mem1_max)); \
   printf("\n--\n"); } while (0)

# define YF_SFNT_CVT_PRINT(cv_p, len) do { \
   printf("\n-- SFNT (debug) --"); \
   printf("\ncvt len is %u", len); \
   printf("\ncvt @%p", (void *)(cv_p)->ctrl_vals); \
   printf("\n--\n"); } while (0)

# define YF_SFNT_FPGM_PRINT(fp_p, len) do { \
   printf("\n-- SFNT (debug) --"); \
   printf("\nfpgm len is %u", len); \
   printf("\nfpgm @%p", (void *)(fp_p)->instrs); \
   printf("\n--\n"); } while (0)

# define YF_SFNT_GASPH_PRINT(gh_p) do { \
   printf("\n-- SFNT (debug) --"); \
   printf("\ngasph - version: 0x%.4hx", be16toh((gh_p)->version)); \
   printf("\ngasph - rng_n: %hu", be16toh((gh_p)->rng_n)); \
   printf("\n--\n"); } while (0)

# define YF_SFNT_GASPE_PRINT(ge_p) do { \
   printf("\n-- SFNT (debug) --"); \
   printf("\ngaspe - rng_ppem_max: %hu", be16toh((ge_p)->rng_ppem_max)); \
   printf("\ngaspe - rng_behav: %hu", be16toh((ge_p)->rng_behav)); \
   printf("\n--\n"); } while (0)

# define YF_SFNT_GLYFD_PRINT(gd_p) do { \
   printf("\n-- SFNT (debug) --"); \
   printf("\nglyfd - contr_n: %hd", be16toh((gd_p)->contr_n)); \
   printf("\nglyfd - x_min: %hd", be16toh((gd_p)->x_min)); \
   printf("\nglyfd - y_min: %hd", be16toh((gd_p)->y_min)); \
   printf("\nglyfd - x_max: %hd", be16toh((gd_p)->x_max)); \
   printf("\nglyfd - y_max: %hd", be16toh((gd_p)->y_max)); \
   printf("\n--\n"); } while (0)

# define YF_SFNT_GLYF_PRINT(gf_p, len) do { \
   printf("\n-- SFNT (debug) --"); \
   printf("\nglyf len is %u", len); \
   printf("\nglyf @%p", (void *)(gf_p)->glyphs); \
   printf("\n--\n"); } while (0)

# define YF_SFNT_LOCA_PRINT(lc_p, len, strd) do { \
   printf("\n-- SFNT (debug) --"); \
   printf("\nloca len is %u", len); \
   printf("\nloca @%p", (void *)lc_p); \
   for (uint16_t i = 0; i < len / strd; ++i) { \
    if (i % 16 == 0) printf("\n[%d-%d]:", i, i+15); \
    if (strd == 2) printf(" %hu", be16toh((lc_p)->off16[i])); \
    else printf(" %u", be32toh((lc_p)->off32[i])); \
   } \
   printf("\n--\n"); } while (0)

# define YF_SFNT_PREP_PRINT(pr_p, len) do { \
   printf("\n-- SFNT (debug) --"); \
   printf("\nprep len is %u", len); \
   printf("\nprep @%p", (void *)(pr_p)->program); \
   printf("\n--\n"); } while (0)
#endif /* YF_DEBUG */

/*
 * XXX: Ongoing devel.
 */

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
  int16_t ascender;
  int16_t descender;
  int16_t line_gap;
  uint16_t adv_wdt_max;
  int16_t lbear_min;
  int16_t rbear_min;
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
  int16_t lbear;
} L_hmtxe;
#define YF_SFNT_HMTXESZ 4
static_assert(sizeof(L_hmtxe) == YF_SFNT_HMTXESZ, "!sizeof");

typedef struct {
  L_hmtxe *hmtxes;
  int16_t *lbears;
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
  int16_t typo_ascender;
  int16_t typo_descender;
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

/* Grayscale device rendering. */
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
  uint16_t upem;
  int16_t x_min;
  int16_t y_min;
  int16_t x_max;
  int16_t y_max;
  uint16_t glyph_n;
  uint16_t pt_max;
  uint16_t contr_max;
  uint16_t comp_pt_max;
  uint16_t comp_contr_max;
  uint16_t comp_elem_max;
  int16_t ascender;
  int16_t descender;
  int16_t line_gap;
  uint16_t adv_wdt_max;
  int16_t lbear_min;
  int16_t rbear_min;
  int16_t x_extent_max;
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

/* Deinitializes font data. */
static void deinit_font(void *font);

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

/* Gets a glyph. */
static int get_glyph(void *font, wchar_t code, uint16_t pts, uint16_t dpi,
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
#ifdef YF_DEBUG
  YF_SFNT_DIRO_PRINT(&sfnt.dir->diro);
#endif

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
#ifdef YF_DEBUG
    YF_SFNT_DIRE_PRINT(sfnt.dir->dires+i);
#endif
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
#ifdef YF_DEBUG
  YF_SFNT_HEAD_PRINT(sfnt.head);
#endif

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
#ifdef YF_DEBUG
  YF_SFNT_HHEA_PRINT(sfnt.hhea);
#endif

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
#ifdef YF_DEBUG
  YF_SFNT_MAXP_PRINT(sfnt.maxp);
#endif

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
    sfnt.hmtx->lbears = malloc(n * sizeof(int16_t));
    if (sfnt.hmtx->lbears == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      deinit_tables(&sfnt);
      fclose(file);
      return -1;
    }
    if (fread(sfnt.hmtx->lbears, sizeof(int16_t), n, file) < n) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      deinit_tables(&sfnt);
      fclose(file);
      return -1;
    }
  }
#ifdef YF_DEBUG
  for (uint16_t i = 0; i < hmetric_n; ++i) {
    YF_SFNT_HMTXE_PRINT(sfnt.hmtx->hmtxes+i);
    if (i == 3) {
      printf("\n... (#%hu hmtx entries)\n", hmetric_n);
      break;
    }
  }
#endif

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
#ifdef YF_DEBUG
  YF_SFNT_OS2_PRINT(sfnt.os2);
#endif

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
#ifdef YF_DEBUG
  YF_SFNT_POSTH_PRINT(&sfnt.post->posth);
#endif

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
#ifdef YF_DEBUG
  YF_SFNT_CMAPH_PRINT(&sfnt.cmap->cmaph);
#endif

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
#ifdef YF_DEBUG
  for (uint16_t i = 0; i < cmap_n; ++i) {
    YF_SFNT_CMAPE_PRINT(sfnt.cmap->cmapes+i);
    uint16_t fmt;
    uint32_t off = cmap_off+be32toh(sfnt.cmap->cmapes[i].off);
    if (fseek(file, off, SEEK_SET) != 0 ||
        fread(&fmt, sizeof fmt, 1, file) < 1)
      assert(0);
    printf("\tcmap format: %hu\n", be16toh(fmt));
  }
#endif

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
#ifdef YF_DEBUG
  YF_SFNT_NAMEH_PRINT(&sfnt.name->nameh);
#endif

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
#ifdef YF_DEBUG
  for (uint16_t i = 0; i < name_n; ++i)
    YF_SFNT_NAMEE_PRINT(sfnt.name->namees+i);
#endif
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

  font->upem = be16toh(sfnt.head->upem);
  font->x_min = be16toh(sfnt.head->x_min);
  font->y_min = be16toh(sfnt.head->y_min);
  font->x_max = be16toh(sfnt.head->x_max);
  font->y_max = be16toh(sfnt.head->y_max);
  font->glyph_n = be16toh(sfnt.maxp->glyph_n);
  font->pt_max = be16toh(sfnt.maxp->pt_max);
  font->contr_max = be16toh(sfnt.maxp->contr_max);
  font->comp_pt_max = be16toh(sfnt.maxp->comp_pt_max);
  font->comp_contr_max = be16toh(sfnt.maxp->comp_contr_max);
  font->comp_elem_max = be16toh(sfnt.maxp->comp_elem_max);
  font->ascender = be16toh(sfnt.hhea->ascender);
  font->descender = be16toh(sfnt.hhea->descender);
  font->line_gap = be16toh(sfnt.hhea->line_gap);
  font->adv_wdt_max = be16toh(sfnt.hhea->adv_wdt_max);
  font->lbear_min = be16toh(sfnt.hhea->lbear_min);
  font->rbear_min = be16toh(sfnt.hhea->rbear_min);
  font->x_extent_max = be16toh(sfnt.hhea->x_extent_max);

  if (set_mapping(sfnt.cmap, file, cmap_off, &font->map) != 0) {
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
#ifdef YF_DEBUG
    YF_SFNT_CVT_PRINT(sfnt->ttf.cvt, cvt_len);
#endif
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
#ifdef YF_DEBUG
    YF_SFNT_FPGM_PRINT(sfnt->ttf.fpgm, fpgm_len);
#endif
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
#ifdef YF_DEBUG
    YF_SFNT_PREP_PRINT(sfnt->ttf.prep, prep_len);
#endif
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
#ifdef YF_DEBUG
    YF_SFNT_GASPH_PRINT(&sfnt->ttf.gasp->gasph);
    for (uint16_t i = 0; i < rng_n; ++i)
      YF_SFNT_GASPE_PRINT(sfnt->ttf.gasp->gaspes+i);
#endif
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
#ifdef YF_DEBUG
  YF_SFNT_LOCA_PRINT(sfnt->ttf.loca, loca_len, (sfnt->head->loca_fmt ? 4 : 2));
#endif

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
#ifdef YF_DEBUG
  uint32_t cmap_off = 0;
  for (uint16_t i = 0; i < tab_n; ++i) {
    if (be32toh(sfnt->dir->dires[i].tag) == YF_SFNT_CMAPTAG) {
      cmap_off = be32toh(sfnt->dir->dires[i].off);
      break;
    }
  }
  assert(cmap_off != 0);
  L_fontmap fmap;
  if (set_mapping(sfnt->cmap, file, cmap_off, &fmap) != 0)
    assert(0);
  YF_SFNT_GLYF_PRINT(sfnt->ttf.glyf, glyf_len);
  if (fmap.map == YF_SFNT_MAP_TRIMMED) {
    printf("\n[glyf - trimmed format (#%hu)]", fmap.trimmed.entry_n);
    for (uint16_t i = 0; i < fmap.trimmed.entry_n; ++i) {
      uint16_t code = fmap.trimmed.first_code+i;
      if (code < 32) continue;
      else if (code > 127) break;
      uint16_t id = fmap.trimmed.glyph_ids[i];
      uint32_t off;
      if (sfnt->head->loca_fmt == 0)
        off = be16toh(sfnt->ttf.loca->off16[id]) << 1;
      else
        off = be32toh(sfnt->ttf.loca->off32[id]);
      assert((off % _Alignof(L_glyfd)) == 0);
      L_glyfd *gd = (L_glyfd *)(sfnt->ttf.glyf->glyphs+off);
      printf("\n[glyf #%hu (%c)]", id, code);
      YF_SFNT_GLYFD_PRINT(gd);
    }
  } else {
    YF_hashset hset = fmap.sparse.glyph_ids;
    printf("\n[glyf - sparse format (#%lu)]", yf_hashset_getlen(hset));
    const uint16_t codes[] = {'a', 'b', 'C', 'D', '0', '1', '@', '!', ' ', '%'};
    int n = sizeof codes / sizeof codes[0];
    while (yf_hashset_getlen(hset) != 0 && n != 0) {
      /*uintptr_t kv = (uintptr_t)yf_hashset_extract(hset, NULL);*/
      uintptr_t kv = (uintptr_t)yf_hashset_search(hset, codes[--n]);
      uint16_t code = kv;
      uint16_t id = kv >> 16;
      uint32_t off;
      if (sfnt->head->loca_fmt == 0)
        off = be16toh(sfnt->ttf.loca->off16[id]) << 1;
      else
        off = be32toh(sfnt->ttf.loca->off32[id]);
      assert((off % _Alignof(L_glyfd)) == 0);
      L_glyfd *gd = (L_glyfd *)(sfnt->ttf.glyf->glyphs+off);
      printf("\n[glyf #%hu (%c)]", id, code);
      YF_SFNT_GLYFD_PRINT(gd);
    }
  }
#endif

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
    free(sfnt->hmtx->lbears);
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

  free(fnt->ttf.loca);
  free(fnt->ttf.glyf);

  free(font);
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
#ifdef YF_DEBUG
  printf("\n-- Font mapping (debug) --");
  printf("\nsub_hdr - fmt: %hu", be16toh(sub_hdr.fmt));
  printf("\nsub_hdr - len: %hu", be16toh(sub_hdr.len));
  printf("\nsub_hdr - lang: %hu", be16toh(sub_hdr.lang));
  printf("\n--\n");
#endif

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

#ifdef YF_DEBUG
  printf("\n-- Font strings (debug) --");
  printf("\n- copyright:\n%s\n", fstr->copyright);
  printf("\n- family:\n%s\n", fstr->family);
  printf("\n- subfamily:\n%s\n", fstr->subfamily);
  printf("\n- uid:\n%s\n", fstr->uid);
  printf("\n- name:\n%s\n", fstr->name);
  printf("\n- version:\n%s\n", fstr->version);
  printf("\n- trademark:\n%s\n", fstr->trademark);
  printf("\n- manufacturer:\n%s\n", fstr->manufacturer);
  printf("\n- designer:\n%s\n", fstr->designer);
  printf("\n- description:\n%s\n", fstr->description);
  printf("\n- license:\n%s\n", fstr->license);
  printf("\n- typographic_family:\n%s\n", fstr->typographic_family);
  printf("\n- typographic_subfamily:\n%s\n", fstr->typographic_subfamily);
  printf("\n- sample_text:\n%s\n", fstr->sample_text);
  printf("\n--\n");
#endif
  return 0;
}

static size_t hash_fmap(const void *x) {
  return ((uintptr_t)x & 0xffff) ^ 0xa993;
}

static int cmp_fmap(const void *a, const void *b) {
  /*return (uint16_t)a - (uint16_t)b;*/
  return ((uintptr_t)a & 0xffff) - ((uintptr_t)b & 0xffff);
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
  uint16_t upem;
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
static int scale_outline(L_outline *outln, uint16_t pts, uint16_t dpi);

/* Rasterizes an outline to produce a glyph. */
static int rasterize(L_outline *outln, YF_glyph *glyph);

static int get_glyph(void *font, wchar_t code, uint16_t pts, uint16_t dpi,
    YF_glyph *glyph)
{
  assert(font != NULL);
  assert(pts != 0 && dpi != 0);
  assert(glyph != NULL);

  int r = 0;
  L_outline outln = {0};

  if (fetch_glyph(font, code, &outln) != 0 ||
      scale_outline(&outln, pts, dpi) != 0 ||
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

  outln->upem = font->upem;

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
  const uint32_t len = font->ttf.loca[id+1] - off;
  assert((off % _Alignof(L_glyfd)) == 0);

  const L_glyfd *gd = (L_glyfd *)(font->ttf.glyf+off);
#ifdef YF_DEBUG
  YF_SFNT_GLYFD_PRINT(gd);
  printf("[id: %hu, off: %u, len: %u, is comp: %d]\n", id, off, len,
      YF_SFNT_ISCOMPND(font, id));
#endif

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

#ifdef YF_DEBUG
  printf("\n-- Outline (debug) --");
  for (uint16_t i = 0; i < outln->comp_n; ++i) {
    printf("\n--\n[Component #%hd]", i);

    printf("\nend indices (#%hu): ", outln->comps[i].end_n);
    for (uint16_t j = 0; j < outln->comps[i].end_n; ++j)
      printf("%hu ", outln->comps[i].ends[j]);

    printf("\npoints (#%hu): ", outln->comps[i].pt_n);
    for (uint16_t j = 0; j < outln->comps[i].pt_n; ++j)
      printf("\n[%s] (%hd %hd)", outln->comps[i].pts[j].on_curve ? "y" : "n",
          outln->comps[i].pts[j].x, outln->comps[i].pts[j].y);
  }
  printf("\n--\n");
#endif

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

static int scale_outline(L_outline *outln, uint16_t pts, uint16_t dpi) {
  assert(outln != NULL);
  assert(outln->comps != NULL);
  assert(pts > 0 && dpi > 0);

  const float fac = (float)(pts*dpi) / (float)(outln->upem*72);

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
  (seg).wind = y1 < y2 ? YF_SFNT_WIND_ON : \
    (y1 > y2 ? YF_SFNT_WIND_OFF : YF_SFNT_WIND_NONE); \
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

#ifdef YF_DEBUG
  printf("\n-- Segments (debug) --");
  printf("\n#%hu (#%hu max)", seg_i, seg_max);
  for (uint32_t i = 0; i < seg_i; ++i)
    printf("\n[w:%d] (%d,%d)--(%d,%d)", segs[i].wind,
        segs[i].p1.x, segs[i].p1.y, segs[i].p2.x, segs[i].p2.y);
  printf("\n--\n");
#endif

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
#define YF_INTERSECTS(res, seg, p1, p2) do { \
  const int32_t d1 = YF_DIRECTION(p1, p2, (seg).p1); \
  const int32_t d2 = YF_DIRECTION(p1, p2, (seg).p2); \
  const int32_t d3 = YF_DIRECTION((seg).p1, (seg).p2, p1); \
  const int32_t d4 = YF_DIRECTION((seg).p1, (seg).p2, p2); \
  if (((d1 < 0 && d2 > 0) || (d1 > 0 && d2 < 0)) && \
      ((d3 < 0 && d4 > 0) || (d3 > 0 && d4 < 0))) \
    res = 1; \
  else if (d1 == 0 && YF_ONPOINT(p1, p2, (seg).p1)) \
    res = 1; \
  else if (d2 == 0 && YF_ONPOINT(p1, p2, (seg).p2)) \
    res = 1; \
  else if (d3 == 0 && YF_ONPOINT((seg).p1, (seg).p2, p1)) \
    res = 1; \
  else if (d4 == 0 && YF_ONPOINT((seg).p1, (seg).p2, p2)) \
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
      L_point p2 = {p1.x+0xffff, p1.y};
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

  /* TODO... */

  glyph->width = YF_SFNT_FIXTOINT(w);
  glyph->height = YF_SFNT_FIXTOINT(h);
  glyph->bpp = 8;
  glyph->bitmap.u8 = bitmap;

  free(segs);
  return 0;
}
