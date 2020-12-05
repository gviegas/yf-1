/*
 * YF
 * filetype-otf.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "filetype-otf.h"
#include "error.h"

#ifdef _DEFAULT_SOURCE
#include <endian.h>
#else
/* TODO */
# error "Invalid platform"
#endif

#define YF_OTF_MAKETAG(c1, c2, c3, c4) \
  (be32toh((c1 << 24) | (c2 << 16) | (c3 << 8) | c4))

/* -- Common -- */

/* Font directory. */
#define YF_OTF_TTF be32toh(0x00010000)
typedef struct {
  uint32_t version;
  uint16_t tab_n;
  uint16_t search_rng;
  uint16_t entry_sel;
  uint16_t rng_shf;
} L_diro;
#define YF_OTF_DIROSZ 12
static_assert(offsetof(L_diro, rng_shf) == YF_OTF_DIROSZ-2, "!offsetof");
typedef struct {
  uint32_t tag;
  uint32_t chsum;
  uint32_t offs;
  uint32_t len;
} L_dire;
#define YF_OTF_DIRESZ 16
static_assert(sizeof(L_dire) == YF_OTF_DIRESZ, "!sizeof");
typedef struct {
  L_diro diro;
  L_dire *dires;
} L_dir;
#ifdef YF_DEBUG
# define YF_OTF_DIRO_PRINT(do_p) do { \
   printf("\n-- Filetype-otf (debug) --"); \
   printf("\ndiro - version: 0x%.8x", be32toh((do_p)->version)); \
   printf("\ndiro - tab_n: %hu", be16toh((do_p)->tab_n)); \
   printf("\ndiro - search_rng: %hu", be16toh((do_p)->search_rng)); \
   printf("\ndiro - entry_sel: %hu", be16toh((do_p)->entry_sel)); \
   printf("\ndiro - rng_shf: %hu", be16toh((do_p)->rng_shf)); \
   printf("\n--\n"); } while (0)
# define YF_OTF_DIRE_PRINT(de_p) do { \
   printf("\n-- Filetype-otf (debug) --"); \
   printf("\ndire - tag: %c%c%c%c", \
    (de_p)->tag, (de_p)->tag >> 8, (de_p)->tag >> 16, (de_p)->tag >> 24); \
   printf("\ndire - chsum: %u", be32toh((de_p)->chsum)); \
   printf("\ndire - offs: %u", be32toh((de_p)->offs)); \
   printf("\ndire - len: %u", be32toh((de_p)->len)); \
   printf("\n--\n"); } while (0)
#endif

/* Character to glyph mapping. */
#define YF_OTF_CMAPTAG YF_OTF_MAKETAG('c', 'm', 'a', 'p')
typedef struct {
  uint16_t version;
  uint16_t tab_n;
} L_cmaph;
#define YF_OTF_CMAPHSZ 4
static_assert(offsetof(L_cmaph, tab_n) == YF_OTF_CMAPHSZ-2, "!offsetof");
typedef struct {
  uint16_t platform;
  uint16_t encoding;
  uint32_t offs;
} L_cmape;
#define YF_OTF_CMAPESZ 8
static_assert(sizeof(L_cmape) == YF_OTF_CMAPESZ, "!sizeof");
typedef struct {
  L_cmaph cmaph;
  L_cmape *cmapes;
} L_cmap;
#ifdef YF_DEBUG
# define YF_OTF_CMAPH_PRINT(ch_p) do { \
   printf("\n-- Filetype-otf (debug) --"); \
   printf("\ncmaph - version: %hu", (ch_p)->version); \
   printf("\ncmaph - tab_n: %hu", be16toh((ch_p)->tab_n)); \
   printf("\n--\n"); } while (0)
# define YF_OTF_CMAPE_PRINT(ce_p) do { \
   printf("\n-- Filetype-otf (debug) --"); \
   printf("\ncmape - platform: %hu", be16toh((ce_p)->platform)); \
   printf("\ncmape - encoding: %hu", be16toh((ce_p)->encoding)); \
   printf("\ncmape - offset: %u", be32toh((ce_p)->offs)); \
   printf("\n--\n"); } while (0)
#endif

/* Font header. */
#define YF_OTF_HEADTAG YF_OTF_MAKETAG('h', 'e', 'a', 'd')
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
#define YF_OTF_HEADSZ 54
static_assert(offsetof(L_head, glyph_fmt) == YF_OTF_HEADSZ-2, "!offsetof");
#ifdef YF_DEBUG
# define YF_OTF_HEAD_PRINT(hd_p) do { \
   printf("\n-- Filetype-otf (debug) --"); \
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
#endif

/* Horizontal layout header. */
#define YF_OTF_HHEATAG YF_OTF_MAKETAG('h', 'h', 'e', 'a')
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
  int16_t caret_offs;
  int16_t reserved[4];
  int16_t metric_fmt;
  uint16_t hmetric_n;
} L_hhea;
#define YF_OTF_HHEASZ 36
static_assert(offsetof(L_hhea, hmetric_n) == YF_OTF_HHEASZ-2, "!offsetof");
#ifdef YF_DEBUG
# define YF_OTF_HHEA_PRINT(hh_p) do { \
   printf("\n-- Filetype-ttf (debug) --"); \
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
   printf("\nhhea - caret_offs: %hd", be16toh((hh_p)->caret_offs)); \
   printf("\nhhea - metric_fmt: %hd", be16toh((hh_p)->metric_fmt)); \
   printf("\nhhea - hmetric_n: %hu", be16toh((hh_p)->hmetric_n)); \
   printf("\n--\n"); } while (0)
#endif

/* Horizontal layout metrics. */
#define YF_OTF_HMTXTAG YF_OTF_MAKETAG('h', 'm', 't', 'x')
typedef struct {
  uint16_t adv_wdt;
  int16_t lbear;
} L_hmtxe;
#define YF_OTF_HMTXESZ 4
static_assert(sizeof(L_hmtxe) == YF_OTF_HMTXESZ, "!sizeof");
typedef struct {
  L_hmtxe *hmtxes;
  int16_t *lbears;
} L_hmtx;
#ifdef YF_DEBUG
# define YF_OTF_HMTXE_PRINT(hm_p) do { \
   printf("\n-- Filetype-otf (debug) --"); \
   printf("\nhmtxe - adv_wdt: %hu", be16toh((hm_p)->adv_wdt)); \
   printf("\nhmtxe - lbear: %hd", be16toh((hm_p)->lbear)); \
   printf("\n--\n"); } while (0)
#endif

/* Maximum profile. */
#define YF_OTF_MAXPTAG YF_OTF_MAKETAG('m', 'a', 'x', 'p')
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
#define YF_OTF_MAXPSZ 32
static_assert(offsetof(L_maxp, comp_dep_max) == YF_OTF_MAXPSZ-2, "!offsetof");
#ifdef YF_DEBUG
# define YF_OTF_MAXP_PRINT(mp_p) do { \
   printf("\n-- Filetype-otf (debug) --"); \
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
#endif

/* Naming. */
#define YF_OTF_NAMETAG YF_OTF_MAKETAG('n', 'a', 'm', 'e')
typedef struct {
  uint16_t format;
  uint16_t count;
  uint16_t str_offs;
} L_nameh;
#define YF_OTF_NAMEHSZ 6
static_assert(offsetof(L_nameh, str_offs) == YF_OTF_NAMEHSZ-2, "!offsetof");
typedef struct {
  uint16_t platform;
  uint16_t encoding;
  uint16_t language;
  uint16_t name;
  uint16_t len;
  uint16_t offs;
} L_namee;
#define YF_OTF_NAMEESZ 12
static_assert(sizeof(L_namee) == YF_OTF_NAMEESZ, "!sizeof");
typedef struct {
  uint16_t len;
  uint16_t offs;
} L_namel;
#define YF_OTF_NAMELSZ 4
static_assert(sizeof(L_namel) == YF_OTF_NAMELSZ, "!sizeof");
typedef struct {
  L_nameh nameh;
  L_namee *namees;
  /* naming table format 1 */
  uint16_t lang_n;
  L_namel *namels;
} L_name;
#ifdef YF_DEBUG
# define YF_OTF_NAMEH_PRINT(nh_p) do { \
   printf("\n-- Filetype-otf (debug) --"); \
   printf("\nnameh - format: %hu", be16toh((nh_p)->format)); \
   printf("\nnameh - count: %hu", be16toh((nh_p)->count)); \
   printf("\nnameh - str_offs: %hu", be16toh((nh_p)->str_offs)); \
   printf("\n--\n"); } while (0)
# define YF_OTF_NAMEE_PRINT(ne_p) do { \
   printf("\n-- Filetype-otf (debug) --"); \
   printf("\nnamee - platform: %hu", be16toh((ne_p)->platform)); \
   printf("\nnamee - encoding: %hu", be16toh((ne_p)->encoding)); \
   printf("\nnamee - language: %hu", be16toh((ne_p)->language)); \
   printf("\nnamee - name: %hu", be16toh((ne_p)->name)); \
   printf("\nnamee - len: %hu", be16toh((ne_p)->len)); \
   printf("\nnamee - offs: %hu", be16toh((ne_p)->offs)); \
   printf("\n--\n"); } while (0)
#endif

/* OS/2 & Windows metrics. */
#define YF_OTF_OS2TAG YF_OTF_MAKETAG('O', 'S', '/', '2')
typedef struct {
  /* v0 */
  uint16_t version;
  int16_t x_avg_char_wdt;
  uint16_t wgt_class;
  uint16_t wdt_class;
  uint16_t type;
  int16_t y_subsc_x_sz;
  int16_t y_subsc_y_sz;
  int16_t y_subsc_x_offs;
  int16_t y_subsc_y_offs;
  int16_t y_supersc_x_sz;
  int16_t y_supersc_y_sz;
  int16_t y_supersc_x_offs;
  int16_t y_supersc_y_offs;
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
#define YF_OTF_OS2V0  78
#define YF_OTF_OS2SZ 100
static_assert(offsetof(L_os2, up_optical_pt_sz) == YF_OTF_OS2SZ-2, "!offsetof");
#ifdef YF_DEBUG
# define YF_OTF_OS2_PRINT(o2_p) do { \
   printf("\n-- Filetype-otf (debug) --"); \
   printf("\nos2 - version: %hu", be16toh((o2_p)->version)); \
   printf("\nos2 - x_avg_char_wdt: %hd", be16toh((o2_p)->x_avg_char_wdt)); \
   printf("\nos2 - wgt_class: %hu", be16toh((o2_p)->wgt_class)); \
   printf("\nos2 - wdt_class: %hu", be16toh((o2_p)->wdt_class)); \
   printf("\nos2 - type: 0x%.4hx", be16toh((o2_p)->type)); \
   printf("\nos2 - y_subsc_x_sz: %hd", be16toh((o2_p)->y_subsc_x_sz)); \
   printf("\nos2 - y_subsc_y_sz: %hd", be16toh((o2_p)->y_subsc_y_sz)); \
   printf("\nos2 - y_subsc_x_offs: %hd", be16toh((o2_p)->y_subsc_x_offs)); \
   printf("\nos2 - y_subsc_y_offs: %hd", be16toh((o2_p)->y_subsc_y_offs)); \
   printf("\nos2 - y_supersc_x_sz: %hd", be16toh((o2_p)->y_supersc_x_sz)); \
   printf("\nos2 - y_supersc_y_sz: %hd", be16toh((o2_p)->y_supersc_y_sz)); \
   printf("\nos2 - y_supersc_x_offs: %hd", be16toh((o2_p)->y_supersc_x_offs)); \
   printf("\nos2 - y_supersc_y_offs: %hd", be16toh((o2_p)->y_supersc_y_offs)); \
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
#endif

/* PostScript info. */
#define YF_OTF_POSTTAG YF_OTF_MAKETAG('p', 'o', 's', 't')
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
#define YF_OTF_POSTHSZ 32
static_assert(offsetof(L_posth, mem1_max) == YF_OTF_POSTHSZ-4, "!offsetof");
typedef struct {
  L_posth posth;
  /* XXX: Glyph names won't see any use. */
} L_post;
#ifdef YF_DEBUG
# define YF_OTF_POSTH_PRINT(ps_p) do { \
   printf("\n-- Filetype-otf (debug) --"); \
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
#endif

/* -- TrueType -- */

/* Control values. */
#define YF_OTF_CVTTAG YF_OTF_MAKETAG('c', 'v', 't', ' ')
typedef struct {
  int16_t *ctrl_vals;
} L_cvt;
#ifdef YF_DEBUG
# define YF_OTF_CVT_PRINT(cv_p, len) do { \
   printf("\n-- Filetype-otf (debug) --"); \
   printf("\ncvt len is %u", len); \
   printf("\ncvt @%p", (void *)(cv_p)->ctrl_vals); \
   printf("\n--\n"); } while (0)
#endif

/* Font program (executed once). */
#define YF_OTF_FPGMTAG YF_OTF_MAKETAG('f', 'p', 'g', 'm')
typedef struct {
  uint8_t *instrs;
} L_fpgm;
#ifdef YF_DEBUG
# define YF_OTF_FPGM_PRINT(fp_p, len) do { \
   printf("\n-- Filetype-otf (debug) --"); \
   printf("\nfpgm len is %u", len); \
   printf("\nfpgm @%p", (void *)(fp_p)->instrs); \
   printf("\n--\n"); } while (0)
#endif

/* Grayscale device rendering. */
#define YF_OTF_GASPTAG YF_OTF_MAKETAG('g', 'a', 's', 'p')
typedef struct {
  uint16_t version;
  uint16_t rng_n;
} L_gasph;
#define YF_OTF_GASPHSZ 4
static_assert(offsetof(L_gasph, rng_n) == YF_OTF_GASPHSZ-2, "!offsetof");
typedef struct {
  uint16_t rng_ppem_max;
  uint16_t rng_behav;
} L_gaspe;
#define YF_OTF_GASPESZ 4
static_assert(sizeof(L_gaspe) == YF_OTF_GASPESZ, "!sizeof");
typedef struct {
  L_gasph gasph;
  L_gaspe *gaspes;
} L_gasp;
#ifdef YF_DEBUG
# define YF_OTF_GASPH_PRINT(gh_p) do { \
   printf("\n-- Filetype-otf (debug) --"); \
   printf("\ngasph - version: 0x%.4hx", be16toh((gh_p)->version)); \
   printf("\ngasph - rng_n: %hu", be16toh((gh_p)->rng_n)); \
   printf("\n--\n"); } while (0)
# define YF_OTF_GASPE_PRINT(ge_p) do { \
   printf("\n-- Filetype-otf (debug) --"); \
   printf("\ngaspe - rng_ppem_max: %hu", be16toh((ge_p)->rng_ppem_max)); \
   printf("\ngaspe - rng_behav: %hu", be16toh((ge_p)->rng_behav)); \
   printf("\n--\n"); } while (0)
#endif

/* Glyph data. */
#define YF_OTF_GLYFTAG YF_OTF_MAKETAG('g', 'l', 'y', 'f')
typedef struct {
  int16_t contour_n;
  int16_t x_min;
  int16_t y_min;
  int16_t x_max;
  int16_t y_max;
  uint8_t data[];
} L_glyfd;
#define YF_OTF_GLYFSZ 10
static_assert(offsetof(L_glyfd, data) == YF_OTF_GLYFSZ, "!offsetof");
typedef struct {
  uint8_t *glyphs;
} L_glyf;
#ifdef YF_DEBUG
# define YF_OTF_GLYFD_PRINT(gd_p) do { \
   printf("\n-- Filetype-otf (debug) --"); \
   printf("\nglyfd - contour_n: %hd", be16toh((gd_p)->contour_n)); \
   printf("\nglyfd - x_min: %hd", be16toh((gd_p)->x_min)); \
   printf("\nglyfd - y_min: %hd", be16toh((gd_p)->y_min)); \
   printf("\nglyfd - x_max: %hd", be16toh((gd_p)->x_max)); \
   printf("\nglyfd - y_max: %hd", be16toh((gd_p)->y_max)); \
   printf("\n--\n"); } while (0)
# define YF_OTF_GLYF_PRINT(gf_p, len) do { \
   printf("\n-- Filetype-otf (debug) --"); \
   printf("\nglyf len is %u", len); \
   printf("\nglyf @%p", (void *)(gf_p)->glyphs); \
   printf("\n--\n"); } while (0)
#endif

/* Glyph location offsets. */
#define YF_OTF_LOCATAG YF_OTF_MAKETAG('l', 'o', 'c', 'a')
typedef struct {
  union {
    uint16_t *offs16;
    uint32_t *offs32;
  };
} L_loca;
#ifdef YF_DEBUG
# define YF_OTF_LOCA_PRINT(lc_p, len, strd) do { \
   printf("\n-- Filetype-otf (debug) --"); \
   printf("\nloca len is %u", len); \
   printf("\nloca @%p", (void *)lc_p); \
   for (uint16_t i = 0; i < len / strd; ++i) { \
    if (i % 16 == 0) printf("\n[%d-%d]:", i, i+15); \
    if (strd == 2) printf(" %hu", be16toh((lc_p)->offs16[i])); \
    else printf(" %u", be32toh((lc_p)->offs32[i])); \
   } \
   printf("\n--\n"); } while (0)
#endif

/* Control value program (executed on font changes). */
#define YF_OTF_PREPTAG YF_OTF_MAKETAG('p', 'r', 'e', 'p')
typedef struct {
  uint8_t *program;
} L_prep;
#ifdef YF_DEBUG
# define YF_OTF_PREP_PRINT(pr_p, len) do { \
   printf("\n-- Filetype-otf (debug) --"); \
   printf("\nprep len is %u", len); \
   printf("\nprep @%p", (void *)(pr_p)->program); \
   printf("\n--\n"); } while (0)
#endif

/* OpenType tables. */
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
} L_otf;

/* Font mapping. */
typedef struct {
  /* TODO */
  uint16_t first_code;
  uint16_t entry_n;
  uint16_t *glyph_ids;
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

/* Loads a font containing TrueType outline. */
static int load_ttf(L_otf *otf, FILE *file);

/* Deinitializes font data. */
static void deinit_font(L_otf *otf);

/* Sets mapping of character codes to glyph indices. */
static int set_mapping(
  const L_cmap *cmap,
  FILE *file,
  uint32_t offs,
  L_fontmap *fmap);

/* Fills font strings. */
static int fill_str(
  const L_name *name,
  FILE *file,
  uint32_t str_offs,
  L_fontstr *fstr);


////////////////
static L_otf l_otf = {0};
static L_fontmap l_fm = {0};
////////////////


int yf_filetype_otf_load(const char *pathname) {
  if (pathname == NULL) {
    yf_seterr(YF_ERR_INVARG, __func__);
    return -1;
  }

  L_otf *otf = calloc(1, sizeof(L_otf));
  if (otf == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  FILE *file = fopen(pathname, "r");
  if (file == NULL) {
    yf_seterr(YF_ERR_NOFILE, __func__);
    free(otf);
    return -1;
  }

  /* font directory */
  otf->dir = calloc(1, sizeof(L_dir));
  if (otf->dir == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }

  /* offset subtable */
  if (fread(&otf->dir->diro, YF_OTF_DIROSZ, 1, file) < 1) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
  if (otf->dir->diro.version != YF_OTF_TTF) {
    yf_seterr(YF_ERR_UNSUP, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
  const uint16_t tab_n = be16toh(otf->dir->diro.tab_n);
#ifdef YF_DEBUG
  YF_OTF_DIRO_PRINT(&otf->dir->diro);
#endif

  /* directory entries */
  otf->dir->dires = malloc(tab_n * sizeof(L_dire));
  if (otf->dir->dires == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
  if (fread(otf->dir->dires, sizeof(L_dire), tab_n, file) < tab_n) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }

  const uint32_t cmap_tag = YF_OTF_CMAPTAG;
  uint32_t cmap_offs = 0;
  uint32_t cmap_len = 0;
  const uint32_t head_tag = YF_OTF_HEADTAG;
  uint32_t head_offs = 0;
  uint32_t head_len = 0;
  const uint32_t hhea_tag = YF_OTF_HHEATAG;
  uint32_t hhea_offs = 0;
  uint32_t hhea_len = 0;
  const uint32_t hmtx_tag = YF_OTF_HMTXTAG;
  uint32_t hmtx_offs = 0;
  uint32_t hmtx_len = 0;
  const uint32_t maxp_tag = YF_OTF_MAXPTAG;
  uint32_t maxp_offs = 0;
  uint32_t maxp_len = 0;
  const uint32_t name_tag = YF_OTF_NAMETAG;
  uint32_t name_offs = 0;
  uint32_t name_len = 0;
  const uint32_t os2_tag = YF_OTF_OS2TAG;
  uint32_t os2_offs = 0;
  uint32_t os2_len = 0;
  const uint32_t post_tag = YF_OTF_POSTTAG;
  uint32_t post_offs = 0;
  uint32_t post_len = 0;

  for (uint16_t i = 0; i < tab_n; ++i) {
    if (otf->dir->dires[i].tag == cmap_tag) {
      cmap_offs = be32toh(otf->dir->dires[i].offs);
      cmap_len = be32toh(otf->dir->dires[i].len);
    } else if (otf->dir->dires[i].tag == head_tag) {
      head_offs = be32toh(otf->dir->dires[i].offs);
      head_len = be32toh(otf->dir->dires[i].len);
    } else if (otf->dir->dires[i].tag == hhea_tag) {
      hhea_offs = be32toh(otf->dir->dires[i].offs);
      hhea_len = be32toh(otf->dir->dires[i].len);
    } else if (otf->dir->dires[i].tag == hmtx_tag) {
      hmtx_offs = be32toh(otf->dir->dires[i].offs);
      hmtx_len = be32toh(otf->dir->dires[i].len);
    } else if (otf->dir->dires[i].tag == maxp_tag) {
      maxp_offs = be32toh(otf->dir->dires[i].offs);
      maxp_len = be32toh(otf->dir->dires[i].len);
    } else if (otf->dir->dires[i].tag == name_tag) {
      name_offs = be32toh(otf->dir->dires[i].offs);
      name_len = be32toh(otf->dir->dires[i].len);
    } else if (otf->dir->dires[i].tag == os2_tag) {
      os2_offs = be32toh(otf->dir->dires[i].offs);
      os2_len = be32toh(otf->dir->dires[i].len);
    } else if (otf->dir->dires[i].tag == post_tag) {
      post_offs = be32toh(otf->dir->dires[i].offs);
      post_len = be32toh(otf->dir->dires[i].len);
    }
#ifdef YF_DEBUG
    YF_OTF_DIRE_PRINT(otf->dir->dires+i);
#endif
  }
  if (cmap_offs == 0 ||
    head_offs == 0 ||
    hhea_offs == 0 ||
    hmtx_offs == 0 ||
    maxp_offs == 0 ||
    name_offs == 0 ||
    os2_offs == 0 ||
    post_offs == 0 ||
    cmap_len < YF_OTF_CMAPHSZ ||
    head_len != YF_OTF_HEADSZ ||
    hhea_len != YF_OTF_HHEASZ ||
    hmtx_len < YF_OTF_HMTXESZ ||
    maxp_len != YF_OTF_MAXPSZ ||
    name_len < YF_OTF_NAMEHSZ ||
    os2_len < YF_OTF_OS2V0 ||
    post_len < YF_OTF_POSTHSZ)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }

  /* head table */
  otf->head = calloc(1, sizeof(L_head));
  if (otf->head == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
  if (fseek(file, head_offs, SEEK_SET) != 0 ||
    fread(otf->head, head_len, 1, file) < 1)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
#ifdef YF_DEBUG
  YF_OTF_HEAD_PRINT(otf->head);
#endif

  /* hhea table */
  otf->hhea = calloc(1, sizeof(L_hhea));
  if (otf->hhea == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
  if (fseek(file, hhea_offs, SEEK_SET) != 0 ||
    fread(otf->hhea, hhea_len, 1, file) < 1)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
  const uint16_t hmetric_n = be16toh(otf->hhea->hmetric_n);
#ifdef YF_DEBUG
  YF_OTF_HHEA_PRINT(otf->hhea);
#endif

  /* maxp table */
  otf->maxp = calloc(1, sizeof(L_maxp));
  if (otf->maxp == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
  if (fseek(file, maxp_offs, SEEK_SET) != 0 ||
    fread(otf->maxp, maxp_len, 1, file) < 1)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
  const uint16_t glyph_n = be16toh(otf->maxp->glyph_n);
#ifdef YF_DEBUG
  YF_OTF_MAXP_PRINT(otf->maxp);
#endif

  /* hmtx table */
  otf->hmtx = calloc(1, sizeof(L_hmtx));
  if (otf->hmtx == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }

  /* hmtx entries */
  otf->hmtx->hmtxes = malloc(hmetric_n * sizeof(L_hmtxe));
  if (otf->hmtx->hmtxes == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
  if (fseek(file, hmtx_offs, SEEK_SET) != 0 ||
    fread(otf->hmtx->hmtxes, sizeof(L_hmtxe), hmetric_n, file) < hmetric_n)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
  if (hmetric_n < glyph_n) {
    const uint16_t n = glyph_n - hmetric_n;
    otf->hmtx->lbears = malloc(n * sizeof(int16_t));
    if (otf->hmtx->lbears == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      deinit_font(otf);
      fclose(file);
      return -1;
    }
    if (fread(otf->hmtx->lbears, sizeof(int16_t), n, file) < n) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      deinit_font(otf);
      fclose(file);
      return -1;
    }
  }
#ifdef YF_DEBUG
  for (uint16_t i = 0; i < hmetric_n; ++i) {
    YF_OTF_HMTXE_PRINT(otf->hmtx->hmtxes+i);
    if (i == 3) {
      printf("\n... (#%hu hmtx entries)\n", hmetric_n);
      break;
    }
  }
#endif

  /* os2 table */
  otf->os2 = calloc(1, sizeof(L_os2));
  if (otf->os2 == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
  if (fseek(file, os2_offs, SEEK_SET) != 0 ||
    fread(otf->os2, os2_len, 1, file) < 1)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
#ifdef YF_DEBUG
  YF_OTF_OS2_PRINT(otf->os2);
#endif

  /* post table */
  otf->post = calloc(1, sizeof(L_post));
  if (otf->post == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
  if (fseek(file, post_offs, SEEK_SET) != 0 ||
    fread(&otf->post->posth, YF_OTF_POSTHSZ, 1, file) < 1)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
#ifdef YF_DEBUG
  YF_OTF_POSTH_PRINT(&otf->post->posth);
#endif

  /* cmap table */
  otf->cmap = calloc(1, sizeof(L_cmap));
  if (otf->cmap == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
  if (fseek(file, cmap_offs, SEEK_SET) != 0 ||
    fread(&otf->cmap->cmaph, YF_OTF_CMAPHSZ, 1, file) < 1)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
  const uint16_t cmap_n = be16toh(otf->cmap->cmaph.tab_n);
#ifdef YF_DEBUG
  YF_OTF_CMAPH_PRINT(&otf->cmap->cmaph);
#endif

  /* cmap entries */
  otf->cmap->cmapes = malloc(cmap_n * sizeof(L_cmape));
  if (otf->cmap->cmapes == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
  if (fread(otf->cmap->cmapes, sizeof(L_cmape), cmap_n, file) < cmap_n) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
#ifdef YF_DEBUG
  for (uint16_t i = 0; i < cmap_n; ++i) {
    YF_OTF_CMAPE_PRINT(otf->cmap->cmapes+i);
    uint16_t fmt;
    uint32_t offs = cmap_offs+be32toh(otf->cmap->cmapes[i].offs);
    if (fseek(file, offs, SEEK_SET) != 0 ||
      fread(&fmt, sizeof fmt, 1, file) < 1)
    { assert(0); }
    printf("\tcmap format: %hu\n", be16toh(fmt));
  }
#endif

  /* TODO */
  L_fontmap fmap = {0};
  if (set_mapping(otf->cmap, file, cmap_offs, &fmap) != 0)
    /* TODO */
    assert(0);

  /* name table */
  otf->name = calloc(1, sizeof(L_name));
  if (otf->name == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
  if (fseek(file, name_offs, SEEK_SET) != 0 ||
    fread(&otf->name->nameh, YF_OTF_NAMEHSZ, 1, file) < 1)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
  const uint16_t name_n = be16toh(otf->name->nameh.count);
#ifdef YF_DEBUG
  YF_OTF_NAMEH_PRINT(&otf->name->nameh);
#endif

  /* name entries */
  otf->name->namees = malloc(name_n * sizeof(L_namee));
  if (otf->name->namees == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
  if (fread(otf->name->namees, sizeof(L_namee), name_n, file) < name_n) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    deinit_font(otf);
    fclose(file);
    return -1;
  }
#ifdef YF_DEBUG
  for (uint16_t i = 0; i < name_n; ++i)
    YF_OTF_NAMEE_PRINT(otf->name->namees+i);
#endif
  if (otf->name->nameh.format != 0) {
    if (fread(&otf->name->lang_n, sizeof(uint16_t), 1, file) < 1) {
      yf_seterr(YF_ERR_INVFILE, __func__);
      deinit_font(otf);
      fclose(file);
      return -1;
    }
    const uint16_t n = be16toh(otf->name->lang_n);
    otf->name->namels = malloc(n * sizeof(L_namel));
    if (otf->name->namels == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      deinit_font(otf);
      fclose(file);
      return -1;
    }
    if (fread(otf->name->namels, sizeof(L_namel), n, file) < n) {
      yf_seterr(YF_ERR_INVFILE, __func__);
      deinit_font(otf);
      fclose(file);
      return -1;
    }
  }

  /* TODO */
  L_fontstr fstr = {0};
  const uint32_t str_offs = name_offs + be16toh(otf->name->nameh.str_offs);
  if (fill_str(otf->name, file, str_offs, &fstr) != 0)
    /* TODO */
    assert(0);

  if (load_ttf(otf, file) != 0) {
    deinit_font(otf);
    fclose(file);
    return -1;
  }

  /* TODO... */


  ////////////////
  memcpy(&l_otf, otf, sizeof l_otf);
  memcpy(&l_fm, &fmap, sizeof l_fm);
  ////////////////


  fclose(file);
  return 0;
}

static int load_ttf(L_otf *otf, FILE *file) {
  assert(otf != NULL);
  assert(!feof(file));

  const uint16_t tab_n = be16toh(otf->dir->diro.tab_n);

  const uint32_t cvt_tag = YF_OTF_CVTTAG;
  uint32_t cvt_offs = 0;
  uint32_t cvt_len = 0;
  const uint32_t fpgm_tag = YF_OTF_FPGMTAG;
  uint32_t fpgm_offs = 0;
  uint32_t fpgm_len = 0;
  const uint32_t gasp_tag = YF_OTF_GASPTAG;
  uint32_t gasp_offs = 0;
  uint32_t gasp_len = 0;
  const uint32_t glyf_tag = YF_OTF_GLYFTAG;
  uint32_t glyf_offs = 0;
  uint32_t glyf_len = 0;
  const uint32_t loca_tag = YF_OTF_LOCATAG;
  uint32_t loca_offs = 0;
  uint32_t loca_len = 0;
  const uint32_t prep_tag = YF_OTF_PREPTAG;
  uint32_t prep_offs = 0;
  uint32_t prep_len = 0;

  for (uint16_t i = 0; i < tab_n; ++i) {
    if (otf->dir->dires[i].tag == cvt_tag) {
      cvt_offs = be32toh(otf->dir->dires[i].offs);
      cvt_len = be32toh(otf->dir->dires[i].len);
    } else if (otf->dir->dires[i].tag == fpgm_tag) {
      fpgm_offs = be32toh(otf->dir->dires[i].offs);
      fpgm_len = be32toh(otf->dir->dires[i].len);
    } else if (otf->dir->dires[i].tag == gasp_tag) {
      gasp_offs = be32toh(otf->dir->dires[i].offs);
      gasp_len = be32toh(otf->dir->dires[i].len);
    } else if (otf->dir->dires[i].tag == glyf_tag) {
      glyf_offs = be32toh(otf->dir->dires[i].offs);
      glyf_len = be32toh(otf->dir->dires[i].len);
    } else if (otf->dir->dires[i].tag == loca_tag) {
      loca_offs = be32toh(otf->dir->dires[i].offs);
      loca_len = be32toh(otf->dir->dires[i].len);
    } else if (otf->dir->dires[i].tag == prep_tag) {
      prep_offs = be32toh(otf->dir->dires[i].offs);
      prep_len = be32toh(otf->dir->dires[i].len);
    }
  }
  if (glyf_offs == 0 || loca_offs == 0) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

  /* cvt table (optional) */
  if (cvt_offs != 0) {
    otf->ttf.cvt = calloc(1, sizeof(L_cvt));
    if (otf->ttf.cvt == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    otf->ttf.cvt->ctrl_vals = malloc(cvt_len);
    if (otf->ttf.cvt->ctrl_vals == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    if (fseek(file, cvt_offs, SEEK_SET) != 0 ||
      fread(otf->ttf.cvt->ctrl_vals, cvt_len, 1, file) < 1)
    {
      yf_seterr(YF_ERR_INVFILE, __func__);
      return -1;
    }
#ifdef YF_DEBUG
    YF_OTF_CVT_PRINT(otf->ttf.cvt, cvt_len);
#endif
  }

  /* fpgm table (optional) */
  if (fpgm_offs != 0) {
    otf->ttf.fpgm = calloc(1, sizeof(L_fpgm));
    if (otf->ttf.fpgm == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    otf->ttf.fpgm->instrs = malloc(fpgm_len);
    if (otf->ttf.fpgm->instrs == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    if (fseek(file, fpgm_offs, SEEK_SET) != 0 ||
      fread(otf->ttf.fpgm->instrs, fpgm_len, 1, file) < 1)
    {
      yf_seterr(YF_ERR_INVFILE, __func__);
      return -1;
    }
#ifdef YF_DEBUG
    YF_OTF_FPGM_PRINT(otf->ttf.fpgm, fpgm_len);
#endif
  }

  /* prep table (optional) */
  if (prep_offs != 0) {
    otf->ttf.prep = calloc(1, sizeof(L_prep));
    if (otf->ttf.prep == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    otf->ttf.prep->program = malloc(prep_len);
    if (otf->ttf.prep->program == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    if (fseek(file, prep_offs, SEEK_SET) != 0 ||
      fread(otf->ttf.prep->program, prep_len, 1, file) < 1)
    {
      yf_seterr(YF_ERR_INVFILE, __func__);
      return -1;
    }
#ifdef YF_DEBUG
    YF_OTF_PREP_PRINT(otf->ttf.prep, prep_len);
#endif
  }

  /* gasp table (optional) */
  if (gasp_offs != 0 && gasp_len >= YF_OTF_GASPHSZ) {
    otf->ttf.gasp = calloc(1, sizeof(L_gasp));
    if (otf->ttf.gasp == NULL) {
      yf_seterr(YF_ERR_NOMEM, __func__);
      return -1;
    }
    if (fseek(file, gasp_offs, SEEK_SET) != 0 ||
      fread(&otf->ttf.gasp->gasph, YF_OTF_GASPHSZ, 1, file) < 1)
    {
      yf_seterr(YF_ERR_INVFILE, __func__);
      return -1;
    }
    const uint16_t rng_n = be16toh(otf->ttf.gasp->gasph.rng_n);
    if (rng_n > 0) {
      otf->ttf.gasp->gaspes = malloc(rng_n * sizeof(L_gaspe));
      if (otf->ttf.gasp->gaspes == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        return -1;
      }
      if (fread(otf->ttf.gasp->gaspes, sizeof(L_gaspe), rng_n, file) < rng_n) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
      }
    }
#ifdef YF_DEBUG
    YF_OTF_GASPH_PRINT(&otf->ttf.gasp->gasph);
    for (uint16_t i = 0; i < rng_n; ++i)
      YF_OTF_GASPE_PRINT(otf->ttf.gasp->gaspes+i);
#endif
  }

  /* loca table */
  otf->ttf.loca = calloc(1, sizeof(L_loca));
  if (otf->ttf.loca == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  void *loca_data = malloc(loca_len);
  if (loca_data == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  if (fseek(file, loca_offs, SEEK_SET) != 0 ||
    fread(loca_data, loca_len, 1, file) < 1)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    free(loca_data);
    return -1;
  }
  if (otf->head->loca_fmt == 0)
    otf->ttf.loca->offs16 = loca_data;
  else
    otf->ttf.loca->offs32 = loca_data;
#ifdef YF_DEBUG
  YF_OTF_LOCA_PRINT(otf->ttf.loca, loca_len, (otf->head->loca_fmt ? 4 : 2));
#endif

  /* glyf table */
  otf->ttf.glyf = calloc(1, sizeof(L_glyf));
  if (otf->ttf.glyf == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  otf->ttf.glyf->glyphs = malloc(glyf_len);
  if (otf->ttf.glyf->glyphs == NULL) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return -1;
  }
  if (fseek(file, glyf_offs, SEEK_SET) != 0 ||
    fread(otf->ttf.glyf->glyphs, glyf_offs, 1, file) < 1)
  {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }
#ifdef YF_DEBUG
  uint32_t cmap_offs = 0;
  for (uint16_t i = 0; i < tab_n; ++i) {
    if (otf->dir->dires[i].tag == YF_OTF_CMAPTAG) {
      cmap_offs = be32toh(otf->dir->dires[i].offs);
      break;
    }
  }
  assert(cmap_offs != 0);
  L_fontmap fmap;
  if (set_mapping(otf->cmap, file, cmap_offs, &fmap) != 0)
    assert(0);
  YF_OTF_GLYF_PRINT(otf->ttf.glyf, glyf_len);
  for (uint16_t i = 0; i < fmap.entry_n; ++i) {
    uint16_t code = fmap.first_code+i;
    if (code < 32) continue;
    else if (code > 127) break;
    uint16_t id = fmap.glyph_ids[i];
    L_glyfd *gd;
    if (otf->head->loca_fmt == 0) {
      uint32_t offs = 2 * be16toh(otf->ttf.loca->offs16[id]);
      assert((offs % _Alignof(L_glyfd)) == 0);
      gd = (L_glyfd *)(otf->ttf.glyf->glyphs+offs);
    } else {
      uint32_t offs = be32toh(otf->ttf.loca->offs32[id]);
      assert((offs % _Alignof(L_glyfd)) == 0);
      gd = (L_glyfd *)(otf->ttf.glyf->glyphs+offs);
    }
    printf("\n[glyf #%hu (%c)]", id, code);
    YF_OTF_GLYFD_PRINT(gd);
  }
#endif

  return 0;
}

static void deinit_font(L_otf *otf) {
  if (otf == NULL)
    return;

  if (otf->dir != NULL) {
    free(otf->dir->dires);
    free(otf->dir);
  }
  if (otf->cmap != NULL) {
    free(otf->cmap->cmapes);
    free(otf->cmap);
  }
  free(otf->head);
  free(otf->hhea);
  if (otf->hmtx != NULL) {
    free(otf->hmtx->hmtxes);
    free(otf->hmtx->lbears);
    free(otf->hmtx);
  }
  free(otf->maxp);
  if (otf->name != NULL) {
    free(otf->name->namees);
    free(otf->name->namels);
    free(otf->name);
  }
  free(otf->os2);
  free(otf->post);
  if (otf->ttf.cvt != NULL) {
    free(otf->ttf.cvt->ctrl_vals);
    free(otf->ttf.cvt);
  }
  if (otf->ttf.fpgm != NULL) {
    free(otf->ttf.fpgm->instrs);
    free(otf->ttf.fpgm);
  }
  if (otf->ttf.gasp != NULL) {
    free(otf->ttf.gasp->gaspes);
    free(otf->ttf.gasp);
  }
  if (otf->ttf.glyf != NULL) {
    free(otf->ttf.glyf->glyphs);
    free(otf->ttf.glyf);
  }
  if (otf->ttf.loca != NULL) {
    free(otf->ttf.loca->offs16);
    free(otf->ttf.loca);
  }
  if (otf->ttf.prep != NULL) {
    free(otf->ttf.prep->program);
    free(otf->ttf.prep);
  }
  free(otf);
}

static int set_mapping(
  const L_cmap *cmap,
  FILE *file,
  uint32_t offs,
  L_fontmap *fmap)
{
  assert(cmap != NULL);
  assert(!feof(file));
  assert(fmap != NULL);

  const uint16_t platf[3] = {htobe16(1), htobe16(3), 0};
  const uint16_t encod[3] = {0,          htobe16(1), htobe16(3)};
  const uint16_t fmt  [3] = {htobe16(6), htobe16(4), htobe16(4)};

  const uint16_t encod_n = be16toh(cmap->cmaph.tab_n);
  uint16_t encod_i = UINT16_MAX;

  uint32_t sub_offs = 0;
  struct { uint16_t fmt, len, lang; } sub_hdr;
  static_assert(sizeof sub_hdr == 6, "!sizeof");

  for (uint16_t i = 0; i < sizeof fmt / sizeof fmt[0]; ++i) {
    for (uint16_t j = 0; j < encod_n; ++j) {
      if (cmap->cmapes[j].platform != platf[i] ||
        cmap->cmapes[j].encoding != encod[i])
      { continue; }

      sub_offs = offs + be32toh(cmap->cmapes[j].offs);
      if (fseek(file, sub_offs, SEEK_SET) != 0 ||
        fread(&sub_hdr, sizeof sub_hdr, 1, file) < 1)
      {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
      }

      if (sub_hdr.fmt == fmt[i]) {
        encod_i = j;
        i = UINT16_MAX-1;
        break;
      }
    }
  }

  if (encod_i == UINT16_MAX) {
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

    case 4:
      /* TODO */
      assert(0);
      return -1;

    case 6: {
      uint16_t first_code;
      uint16_t entry_n;
      if (fread(&first_code, sizeof first_code, 1, file) < 1 ||
        fread(&entry_n, sizeof entry_n, 1, file) < 1)
      {
        yf_seterr(YF_ERR_INVFILE, __func__);
        return -1;
      }
      first_code = be16toh(first_code);
      entry_n = be16toh(entry_n);
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
      /* TODO */
      fmap->first_code = first_code;
      fmap->entry_n = entry_n;
      for (uint16_t i = 0; i < entry_n; ++i)
        glyph_ids[i] = be16toh(glyph_ids[i]);
      fmap->glyph_ids = glyph_ids;
#ifdef YF_DEBUG
      printf("\n-- Font mapping (debug) --");
      printf("\nsub (6) - first_code: %hu", first_code);
      printf("\nsub (6) - entry_n: %hu", entry_n);
      for (uint16_t i = 0; i < entry_n; ++i) { \
        if (i % 16 == 0) printf("\n[%d-%d]:", i, i+15); \
        printf(" %hu", glyph_ids[i]); \
      } \
      printf("\n--\n");
#endif
    } break;

    default:
      assert(0);
      return -1;
  }

  /* TODO */

  return 0;
}

static int fill_str(
  const L_name *name,
  FILE *file,
  uint32_t str_offs,
  L_fontstr *fstr)
{
  assert(name != NULL);
  assert(!feof(file));
  assert(fstr != NULL);

  /* TODO: Select correct platform-encoding-language combination. */
  const uint16_t platf = htobe16(1);
  const uint16_t encod = 0;
  const uint16_t lang = 0;

  const uint16_t name_n = be16toh(name->nameh.count);
  uint16_t name_i = 0;

  for (; name_i < name_n; ++name_i) {
    if (name->namees[name_i].platform == platf) break;
  }
  for (; name_i < name_n; ++name_i) {
    if (name->namees[name_i].encoding == encod) break;
  }
  for (; name_i < name_n; ++name_i) {
    if (name->namees[name_i].language == lang) break;
  }

  if (name_i >= name_n) {
    yf_seterr(YF_ERR_INVFILE, __func__);
    return -1;
  }

  uint16_t nm, len, offs;
  char **str_p;

  for (; name_i < name_n; ++name_i) {
    if (name->namees[name_i].platform != platf ||
      name->namees[name_i].encoding != encod ||
      name->namees[name_i].language != lang)
    { break; }

    nm = be16toh(name->namees[name_i].name);
    len = be16toh(name->namees[name_i].len);
    offs = be16toh(name->namees[name_i].offs);

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
    if (fseek(file, str_offs+offs, SEEK_SET) != 0 ||
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


////////////////

YF_coord2 *trace_glyph(char c, uint16_t *coord_n) {
  assert(c >= l_fm.first_code && c-l_fm.first_code < l_fm.entry_n);

  YF_coord2 *coords = NULL;

  printf("\n-- trace_glyph() [%c] --", c);
  const uint16_t gid = l_fm.glyph_ids[c-l_fm.first_code];
  printf("\n> gid is %hu", gid);

  uint32_t goffs, gsz;
  if (l_otf.head->loca_fmt == 0) {
    goffs = be16toh(l_otf.ttf.loca->offs16[gid]) * 2;
    gsz = be16toh(l_otf.ttf.loca->offs16[gid+1]) * 2 - goffs;
  } else {
    goffs = be32toh(l_otf.ttf.loca->offs32[gid]);
    gsz = be32toh(l_otf.ttf.loca->offs32[gid+1]) - goffs;
  }
  printf("\n> goffs is %u", goffs);
  printf("\n> gsz is %u", gsz);

  assert((goffs % _Alignof(L_glyfd)) == 0);
  L_glyfd *gd = (L_glyfd *)(l_otf.ttf.glyf->glyphs+goffs);
  puts("");
  YF_OTF_GLYFD_PRINT(gd);

  const int16_t contr_n = be16toh(gd->contour_n);
  if (contr_n == 0) {
    printf("\n> simple glyph (no contour)");

  } else if (contr_n > 0) {
    printf("\n> simple glyph (%hd contour(s))", contr_n);

    uint16_t endpt_i[contr_n];
    for (uint16_t i = 0; i < contr_n; ++i) {
      endpt_i[i] = be16toh(((uint16_t *)gd->data)[i]);
      printf("\n> endpt_i #%d is %hu", i, endpt_i[i]);
    }

    const uint16_t instr_len = be16toh(((uint16_t *)gd->data)[contr_n]);
    const uint32_t flag_offs = sizeof endpt_i + sizeof instr_len + instr_len;
    printf("\n> instr_len is %hu", instr_len);
    printf("\n> flag_offs is %u", flag_offs);

    uint8_t flag = 0;
    uint8_t repeat_n = 0;
    uint16_t flag_i = 0;
    uint16_t x_i = 0;
    uint16_t y_i = 0;
    uint16_t pt_i = 0;
    uint32_t x_len = 0;
    uint32_t y_len = 0;

    struct {
      uint8_t flag;
      uint16_t x_i, y_i;
    } pts[endpt_i[contr_n-1]];

    do {
      flag = gd->data[flag_offs+flag_i++];
      printf("\nflag=0x%hhx", flag);

      if (flag & 8) {
        /* next byte specifies number of times to repeat this flag set */
        repeat_n = gd->data[flag_offs+flag_i++];
        printf("\n> repeat_n set to %hhu", repeat_n);
      }

      do {
        pts[pt_i].flag = flag;
        pts[pt_i].x_i = x_i;
        pts[pt_i].y_i = y_i;

        if (flag & 2) {
          /* x-coord is 1 byte long */
          ++x_i;
          ++x_len;
        } else if (!(flag & 16)) {
          /* x-coord is 2 bytes long and is not the same as prev x */
          ++x_i;
          x_len += 2;
        }

        if (flag & 4) {
          /* y-coord is 1 byte long */
          ++y_i;
          ++y_len;
        } else if (!(flag & 32)) {
          /* y-coord is 2 bytes long and is not the same as prev y */
          ++y_i;
          y_len += 2;
        }

        ++pt_i;
        if (repeat_n == 0)
          break;
        --repeat_n;
      } while (1);
    } while (pt_i <= endpt_i[contr_n-1]);

    printf("\n> flag_i is %hu", flag_i);
    printf("\n> x_i is %hu", x_i);
    printf("\n> y_i is %hu", y_i);
    printf("\n> pt_i is %hu", pt_i);
    printf("\n> x_len is %hu", x_len);
    printf("\n> y_len is %hu", y_len);
    for (uint16_t i = 0; i < pt_i; ++i)
      printf("\n> pt #%hd: \tflag=0x%.2hhx \tx_i=%.3hu \ty_i=%.3hu",
        i, pts[i].flag, pts[i].x_i, pts[i].y_i);

    coords = malloc(pt_i * sizeof *coords);
    assert(coords != NULL);
    uint8_t *x_p = gd->data+flag_offs+flag_i;
    uint8_t *y_p = x_p+x_len;
    YF_coord2 coord = {0};

    for (uint16_t i = 0; i < pt_i; ++i) {
      if (pts[i].flag & 1)
        /* pt is on the curve; off the curve otherwise */
        ;

      if (pts[i].flag & 2) {
        /* x-coord is 1 byte long */
        if (pts[i].flag & 16)
          /* is positive */
          coords[i].x = coord.x + *x_p;
        else
          /* is negative */
          coords[i].x = coord.x + -1.0 * *x_p;
        ++x_p;
      } else {
        /* x-coord is 2 bytes long */
        if (pts[i].flag & 16) {
          /* is same as prev x */
          coords[i].x = coords[i-1].x;
        } else {
          /* is not the same as prev x */
          int16_t x = be16toh((int16_t)(*(x_p+1)) << 8 | (int16_t)(*x_p));
          coords[i].x = coord.x + x;
          x_p += 2;
        }
      }

      if (pts[i].flag & 4) {
        /* y-coord is 1 byte long */
        if (pts[i].flag & 32)
          /* is positive */
          coords[i].y = coord.y + *y_p;
        else
          /* is negative */
          coords[i].y = coord.y + -1.0 * *y_p;
        ++y_p;
      } else {
        /* y-coord is 2 bytes long */
        if (pts[i].flag & 32) {
          /* is same as prev y */
          coords[i].y = coords[i-1].y;
        } else {
          /* is not the same as prev y */
          int16_t y = be16toh((int16_t)(*(y_p+1)) << 8 | (int16_t)(*y_p));
          coords[i].y = coord.y + y;
          y_p += 2;
        }
      }

      coord.x = coords[i].x;
      coord.y = coords[i].y;
    }

    *coord_n = pt_i;

  } else {
    printf("\n> compound glyph (%hd contour(s))", contr_n);
  }

  for (uint16_t i = 0; i < *coord_n; ++i)
    printf("\n> coord #%hu: \tx=%.4f \ty=%.4f", i, coords[i].x, coords[i].y);

  printf("\n--\n");

  return coords;
}

////////////////
