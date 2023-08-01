/*
 * YF
 * test-draw.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "yf/wsys/yf-event.h"
#include "yf/wsys/yf-keyboard.h"

#include "yf-core.h"

#define YF_WINW 512
#define YF_WINH 384
#define YF_WINT "Draw"

/* Shared variables. */
struct vars {
    yf_context_t *ctx;
    yf_wsi_t *wsi;
    yf_buffer_t *buf;
    yf_image_t *img;
    yf_dtable_t *dtb;
    yf_pass_t *pass;
    yf_target_t **tgts;
    yf_gstate_t *gst;

    yf_window_t *win;
    int key;
};
static struct vars vars_ = {0};

/* Vertex type. */
struct vertex {
    float pos[3];
    float clr[4];
};

/* Key event function. */
static void key_kb(int key, int state,
                   YF_UNUSED unsigned mod_mask, YF_UNUSED void *data)
{
    if (state == YF_KEYSTATE_PRESSED)
        vars_.key = key;
}

/* Initializes content. */
static void init(void)
{
    /* Context */
    yf_context_t *ctx = yf_context_init();
    assert(ctx != NULL);

    /* Buffer */
    yf_buffer_t *buf = yf_buffer_init(ctx, 2048);
    assert(buf != NULL);

    /* Stages */
    yf_shdid_t vshd, fshd;

    if (yf_loadshd(ctx, "tmp/vert", &vshd) != 0)
        assert(0);
    if (yf_loadshd(ctx, "tmp/frag", &fshd) != 0)
        assert(0);

    const yf_stage_t stgs[] = {
        {YF_STAGE_VERT, vshd, "main"},
        {YF_STAGE_FRAG, fshd, "main"}
    };

    /* DTable */
    const yf_dentry_t entry = {0, YF_DTYPE_UNIFORM, 1, NULL};
    yf_dtable_t *dtb = yf_dtable_init(ctx, &entry, 1);
    assert(dtb != NULL);

    const unsigned alloc_n = 1;
    if (yf_dtable_alloc(dtb, alloc_n) != 0)
        assert(0);

    /* VInput */
    const yf_vattr_t attrs[] = {
        {0, YF_VFMT_FLOAT3, 0},
        {1, YF_VFMT_FLOAT4, offsetof(struct vertex, clr)}
    };

    const yf_vinput_t vin = {
        attrs,
        sizeof attrs / sizeof attrs[0],
        sizeof(struct vertex),
        YF_VRATE_VERT
    };

    /* WSI */
    yf_window_t *win = yf_window_init(YF_WINW, YF_WINH, YF_WINT, 0);
    assert(win != NULL);

    yf_wsi_t *wsi = yf_wsi_init(ctx, win);
    assert(wsi != NULL);

    unsigned pres_img_n;
    yf_image_t *const *pres_imgs = yf_wsi_getimages(wsi, &pres_img_n);
    assert(pres_imgs != NULL && pres_img_n != 0);

    /* Image */
    const yf_dim3_t img_dim = {YF_WINW, YF_WINH, 1};
    yf_image_t *img = yf_image_init(ctx, YF_PIXFMT_D16UNORM, img_dim, 1, 1, 1);
    assert(img != NULL);

    /* Pass */
    int pres_fmt;
    unsigned pres_spl;
    yf_image_getval(pres_imgs[0], &pres_fmt, NULL, NULL, NULL, &pres_spl);

    const yf_colordsc_t clr_dsc = {
        pres_fmt,
        pres_spl,
        YF_LOADOP_UNDEF,
        YF_STOREOP_UNDEF
    };

    const yf_depthdsc_t dep_dsc = {
        YF_PIXFMT_D16UNORM,
        1,
        YF_LOADOP_UNDEF,
        YF_STOREOP_UNDEF,
        YF_LOADOP_UNDEF,
        YF_STOREOP_UNDEF
    };

    yf_pass_t *pass = yf_pass_init(ctx, &clr_dsc, 1, NULL, &dep_dsc);
    assert(pass != NULL);

    /* Targets */
    const yf_dim2_t tgt_dim = {YF_WINW, YF_WINH};
    const yf_attach_t dep_att = {img, 0};

    yf_attach_t *clr_atts = malloc(pres_img_n * sizeof(yf_attach_t));
    yf_target_t **tgts = malloc(pres_img_n * sizeof(yf_target_t *));
    assert(clr_atts != NULL && tgts != NULL);

    for (size_t i = 0; i < pres_img_n; i++) {
        clr_atts[i] = (yf_attach_t){pres_imgs[i], 0};
        tgts[i] = yf_pass_maketarget(pass, tgt_dim, 1, clr_atts+i, NULL,
                                     &dep_att);
        assert(tgts[i] != NULL);
    }
    free(clr_atts);

    /* Graphics state */
    const yf_gconf_t conf = {
        pass,
        stgs,
        sizeof stgs / sizeof stgs[0],
        &dtb,
        1,
        &vin,
        1,
        YF_TOPOLOGY_TRIANGLE,
        YF_POLYMODE_FILL,
        YF_CULLMODE_BACK,
        YF_WINDING_CCW
    };

    yf_gstate_t *gst = yf_gstate_init(ctx, &conf);
    assert(gst != NULL);

    /* Data copy */
    const float m[16] = {
        0.9f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.9f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.9f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    const struct vertex verts[3] = {
        {{-1.0f,  1.0f, 0.5f}, {0.525f, 0.305f, 0.483f, 1.0f}},
        {{ 1.0f,  1.0f, 0.5f}, {0.773f, 0.893f, 0.200f, 1.0f}},
        {{ 0.0f, -1.0f, 0.5f}, {0.912f, 0.450f, 0.335f, 1.0f}}
    };

    if (yf_buffer_copy(buf, 0, m, sizeof m) != 0)
        assert(0);
    if (yf_buffer_copy(buf, sizeof m, verts, sizeof verts) != 0)
        assert(0);

    const yf_slice_t elems = {0, 1};
    const size_t buf_off = 0;
    const size_t buf_sz = sizeof m;

    if (yf_dtable_copybuf(dtb, 0, 0, elems, &buf, &buf_off, &buf_sz) != 0)
        assert(0);

    vars_.ctx = ctx;
    vars_.wsi = wsi;
    vars_.buf = buf;
    vars_.img = img;
    vars_.dtb = dtb;
    vars_.pass = pass;
    vars_.tgts = tgts;
    vars_.gst = gst;

    vars_.win = win;
    vars_.key = YF_KEY_UNKNOWN;

    yf_evtfn_t fn = {.key_kb = key_kb};
    yf_setevtfn(YF_EVT_KEYKB, fn, NULL);
}

/* Updates content. */
static void update(void)
{
    /* Events */
    yf_pollevt(YF_EVT_KEYKB);

    /* Command buffer */
    static const yf_viewport_t vp = {0.0f, 0.0f, YF_WINW, YF_WINH, 0.0f, 1.0f};
    static const yf_rect_t sciss = {{0, 0}, {YF_WINW, YF_WINH}};

    int tgt_i = yf_wsi_next(vars_.wsi, 0);
    assert(tgt_i >= 0);

    yf_cmdbuf_t *cb = yf_cmdbuf_get(vars_.ctx, YF_CMDBUF_GRAPH);
    assert(cb != NULL);

    yf_cmdbuf_setgstate(cb, vars_.gst);
    yf_cmdbuf_settarget(cb, vars_.tgts[tgt_i]);
    yf_cmdbuf_setvport(cb, 0, &vp);
    yf_cmdbuf_setsciss(cb, 0, sciss);
    yf_cmdbuf_setdtable(cb, 0, 0);
    yf_cmdbuf_setvbuf(cb, 0, vars_.buf, sizeof(float[16]));
    yf_cmdbuf_clearcolor(cb, 0, YF_COLOR_BLACK);
    yf_cmdbuf_cleardepth(cb, 1.0f);
    yf_cmdbuf_draw(cb, 0, 3, 0, 1);

    if (yf_cmdbuf_end(cb) != 0)
        assert(0);
    if (yf_cmdbuf_exec(vars_.ctx) != 0)
        assert(0);
    if (yf_wsi_present(vars_.wsi, tgt_i) != 0)
        assert(0);
}

/* Runs the main loop. */
static int run(void)
{
    const long frame_tm = 1.0 / 60.0 * 1000000000.0;
    long dt;
    time_t sec;
    struct timespec before, now, idle;
    clock_gettime(CLOCK_MONOTONIC, &before);

    do {
        update();

        clock_gettime(CLOCK_MONOTONIC, &now);
        sec = now.tv_sec - before.tv_sec;
        dt = sec > 0 ? ((long)999999999 * sec) : 0;
        dt = dt - before.tv_nsec + now.tv_nsec;

        if (dt < frame_tm) {
            idle.tv_sec = 0;
            idle.tv_nsec = frame_tm - dt;
            clock_nanosleep(CLOCK_MONOTONIC, 0, &idle, NULL);
            clock_gettime(CLOCK_MONOTONIC, &now);
            dt = frame_tm;
        }

        before = now;

        printf("\n[time] %lds %ldns (%.0f fps)", now.tv_sec, now.tv_nsec,
               (double)999999999 / (double)dt);
    } while (vars_.key == YF_KEY_UNKNOWN);

    yf_gstate_deinit(vars_.gst);
    yf_pass_deinit(vars_.pass);
    yf_dtable_deinit(vars_.dtb);
    yf_image_deinit(vars_.img);
    yf_buffer_deinit(vars_.buf);
    yf_wsi_deinit(vars_.wsi);
    yf_window_deinit(vars_.win);
    yf_context_deinit(vars_.ctx);
    return 0;
}

/* Tests drawing. */
int yf_test_draw(void)
{
    init();
    return run();
}
