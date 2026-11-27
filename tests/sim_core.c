/* SPDX-License-Identifier: GPL-3.0-only
 * Drum machine fork: 2026 DEADACTIVE */
/* The web simulator's core (docs/SIMULATOR.md): the firmware (tests/sim_host.h) behind a small API the page
 * drives. One instance = one power-on. The browser builds this file as wasm (tools/build_sim.sh) and runs it
 * in an AudioWorklet; tests/sim_test.c includes it. sim_render is the clock: it runs the audio ISR once per
 * half buffer and the main loop's UI frame every 15 ms of audio, as the device does. */
#include "sim_host.h"

#define SIM_MAX_FRAMES 1024u
#define SIM_FRAME_SAMPLES (FS * 15u / 1000u)             /* main.c: ~60 UI frames a second at most */

static float sim_out[2u * SIM_MAX_FRAMES];
static uint32_t sim_pos = HALF_FRAMES;                   /* frames of the current half already handed out */
static uint64_t sim_next_frame;
static int32_t sim_knob = 512 * 16;                       /* main.c's MASTER filter state */
static uint32_t sim_master_raw = 800;

static void sim_demo_install(void);
static void (*sim_half_hook)(void);                     /* after each audio half: the lab's probes (sim_lab.c) */

void sim_frame(void)                                     /* main.c's loop body, minus the hardware */
{
    int32_t a = (int32_t)sim_master_raw;
    uint32_t k10;
    song.batt_raw = 600;                                  /* a full battery */
    sim_knob += (a * 16 - sim_knob) / 8;
    k10 = (uint32_t)(sim_knob / 16);
    song.master_q12 = (k10 * k10) >> 8;
    felucca_dbg.ui_frames++;
    felucca_dbg.page = ui.page;
    felucca_dbg.home = ui.home;
    ui_input();
    ui_leds();
    ui_draw();
}

void sim_flash_reset(void)
{
    memset(hflash, 0xFF, sizeof hflash);
    memset(sim_dirty, 0, sizeof sim_dirty);
}

#ifndef SIM_GROOVE_DEFAULT
#define SIM_GROOVE_DEFAULT 0
#endif
int sim_groove = SIM_GROOVE_DEFAULT;
void sim_set_groove(int on) { sim_groove = on != 0; }        /* the page's machine (worklet.js): Groove; the tests: Hortator */                                      /* 1 = the Groove UI owns the panel, as on the device (Hortator's own tests: 0) */
void sim_init(int fresh)                                 /* main.c's boot order */
{
    gv_on = (uint8_t)sim_groove;
    if (fresh)
        sim_flash_reset();
    persist_boot();
    settings_init();
    memset(&felucca_dbg, 0, sizeof felucca_dbg);
    felucca_dbg.magic = DBG_MAGIC;
    felucca_dbg.boots = 1;
    panel_init();
    drum_boot_init();
    audio_init();
    sim_knob = (int32_t)sim_master_raw * 16;
    sim_frame();                                          /* settles MASTER and draws the first screen */
    lcd_fill(0, 0, 240, 240, C_BG);
    ui.force = 1;
    if (fresh)
        sim_demo_install();
    else if (!lastp.set && project_used(0))               /* a returning visitor: as on the device, the last project
                                                           * saved (drum_boot_init loaded it, or asks LOAD FAILED);
                                                           * none remembered: their slot 1, not the device's empty
                                                           * power-on pattern (PLAY should groove on a public demo) */
        project_load(0);
    sim_next_frame = sim_samples + SIM_FRAME_SAMPLES;
}

uint32_t sim_render(uint32_t frames)                     /* interleaved float stereo into sim_audio(); UI frames run */
{
    uint32_t i, ui_frames = 0;
    if (frames > SIM_MAX_FRAMES)
        frames = SIM_MAX_FRAMES;
    for (i = 0; i < frames; i++) {
        const int32_t *h;
        if (sim_pos == HALF_FRAMES) {
            if (sim_samples >= sim_next_frame) {          /* between halves, as the main loop runs between ISRs */
                sim_frame();
                ui_frames++;
                sim_next_frame += SIM_FRAME_SAMPLES;
            }
            sim_half ^= 1u;
            fm1_alnk0_irq();
            sim_samples += HALF_FRAMES;
            sim_ms_update();
            if (sim_half_hook)
                sim_half_hook();
            sim_pos = 0;
        }
        h = &abuf[sim_half * HALF_WORDS];
        sim_out[2u * i] = (float)h[2u * sim_pos] * (1.0f / 8388608.0f);   /* 24-bit */
        sim_out[2u * i + 1u] = (float)h[2u * sim_pos + 1u] * (1.0f / 8388608.0f);
        sim_pos++;
    }
    return ui_frames;
}

void sim_btn(uint32_t label, int down)                    /* label = B_FX .. B_OCTUP (panel.c) */
{
    uint32_t bit;
    if (label >= NB)
        return;
    bit = 1u << panel.btn[label];
    if (down) {
        fm1_in.pressed |= bit & ~fm1_in.buttons;
        fm1_in.buttons |= bit;
    } else
        fm1_in.buttons &= ~bit;
}

void sim_key(uint32_t n, int down)                        /* note key 0 (F3) .. 26 (G5) */
{
    uint32_t bit;
    if (n >= 27u)
        return;
    bit = 1u << n;
    if (down) {
        fm1_in.notes_pressed |= bit & ~fm1_in.notes;
        fm1_in.notes |= bit;
    } else
        fm1_in.notes &= ~bit;
}

void sim_enc(uint32_t role, int32_t steps)                /* role = EN_SELECT .. EN_K4, + = clockwise */
{
    if (role < NE)
        sim_enc_acc[panel.enc[role]] += steps * panel.dir[role];
}

void sim_master(uint32_t v) { sim_master_raw = v > 1023u ? 1023u : v; }

static int sim_led_lit(uint32_t id)
{
    uint32_t p, r;
    for (p = 0; p < FM1_NCOL; p++)
        for (r = 1; r < 5u; r++)
            if (FM1_KEYMAP[r][p] == (int8_t)id)
                return ((led_dim_on ? led_lay[p] | led_lay[p] >> 24 : fm1_led[p]) >> r) & 1u;   /* (the hook on:
                                                                                * the full layer and a breathing one, as
                                                                                * lit; no scan here composes fm1_led) */
    return 0;
}

uint32_t sim_leds(void)                                   /* bit b = button label b (B_FX .. B_OCTUP) */
{
    uint32_t b, m = 0;
    for (b = 0; b < NB; b++)
        m |= (uint32_t)sim_led_lit(panel.btn[b]) << b;
    return m;
}

uint32_t sim_key_leds(void)                               /* bit n = note key n (0 = F3 .. 26 = G5) */
{
    uint32_t n, m = 0;
    for (n = 0; n < 27u; n++)
        m |= (uint32_t)sim_led_lit(14u + n) << n;
    return m;
}

uint32_t sim_playing(void) { return song.playing != 0u; }
uint16_t *sim_fb(void) { return fb; }
float *sim_audio(void) { return sim_out; }
uint8_t *sim_flash(void) { return hflash; }
uint32_t *sim_flash_dirty(void) { return sim_dirty; }
void sim_flash_clean(void) { memset(sim_dirty, 0, sizeof sim_dirty); }

/* ---- the web app (web/studio/fw.js): the firmware's own generators, so the Studio makes what the FM-1 makes ----
 * gx_buf: 8 rows x 64 steps (bit 0 on, bit 1 accent), then one track's 64 steps x 5 (degree or 255, quarter steps
 * long, quarter steps late, accent, slide) */
static uint8_t gx_out[8 * 64 + 64 * 5];
uint8_t *gx_buf(void) { return gx_out; }
static void gx_rows(void)
{
    uint32_t r, c;
    for (r = 0; r < 8u; r++) for (c = 0; c < 64u; c++) { const step_t *q = &trk[GV_ROWTRK[r]].step[c % gv_rlen()]; gx_out[r * 64u + c] = (uint8_t)((q->on ? 1u : 0u) | (q->acc ? 2u : 0u)); }
}
static void gx_trk(uint32_t i)
{
    uint32_t c;
    for (c = 0; c < 64u; c++) { const gv_step_t *q = &gv.t[i].st[c % gv_tlen(i)]; uint8_t *o = &gx_out[512u + c * 5u];
        o[0] = (uint8_t)(q->n < 0 ? 255 : q->n); o[1] = q->l4; o[2] = q->o4; o[3] = q->a; o[4] = q->s; }
}
static void gx_trk_in(uint32_t i)
{
    uint32_t c;
    for (c = 0; c < 64u; c++) { gv_step_t *q = &gv.t[i].st[c]; const uint8_t *o = &gx_out[512u + c * 5u];
        q->n = (int8_t)(o[0] == 255u ? -1 : o[0] > 28u ? 28 : o[0]); q->l4 = (uint8_t)(o[1] ? o[1] > 64u ? 64u : o[1] : 1u); q->o4 = o[2] & 3u; q->a = o[3] & 1u; q->s = o[4] & 1u; }
}
uint32_t gx_boot(uint32_t seed)                         /* a machine of its own, Groove on, never played */
{
    uint32_t k;
    sim_groove = 1; sim_init(1);
    for (k = 0; k < 8u && !gu.init; k++) sim_frame();
    gv_rng = seed ? seed : 1u;
    return gu.init;
}
void gx_cfg(uint32_t style, uint32_t root, uint32_t scale, uint32_t prog, uint32_t rsrc, uint32_t msrc)
{
    gv.style = (uint8_t)(style % GV_NSTY); gv.root = (uint8_t)(root % 12u); gv.scale = (uint8_t)(scale % GV_NSC);
    gv.prog = (uint8_t)prog; gv.rsrc = (uint8_t)(rsrc > 2u ? 0u : rsrc); gv.msrc = (uint8_t)(msrc > 1u ? 0u : msrc);
}
uint32_t gx_rhythm(uint32_t bars)                       /* -> the template taken (1..), or 256 + the base's motif */
{
    gv_set_rlen(bars * 16u); gv_gen_rhythm(); gx_rows();
    return gv.tpl ? gv.tpl : 256u + gv.libk;
}
void gx_row(uint32_t r, uint32_t now)                   /* one row again; now = its first bar as it is (not to give it back) */
{
    r &= 7u; gv_rs_acc = 0; gv_rhythm_row_set(r, (uint16_t)now); gv_gen_row(r); gx_rows();
}
void gx_mel(uint32_t i, uint32_t bars) { i %= GV_NT; gv_set_tlen(i, bars * 16u); gv_gen_mel(i); gx_trk(i); }
void gx_vary(uint32_t i, uint32_t bars, uint32_t bar)   /* the track as the page has it (gx_buf), its bar `bar` varied */
{
    uint32_t k, any = 0;
    i %= GV_NT; gv.t[i].len = (uint8_t)gv_lenok(bars * 16u); gx_trk_in(i);
    bar %= gv_tlen(i) / 16u;
    for (k = 0; k < 16u; k++) any |= gv.t[i].st[bar * 16u + k].n >= 0;
    if (any) gv_vary_bar(i, bar); else gv_gen_mel(i);
    gx_trk(i);
}

/* the Studio's link (web/studio/link.js) checked against the firmware's own bytes (groove_link.c), no USB: the
 * request's arguments in gx_buf, n of them; the reply's data back in gx_buf: its length, or 4096 + the error */
uint32_t gx_link(uint32_t cmd, uint32_t n)
{
    static uint8_t in[2 + 64 * 3 + 8];
    int e = 0;
    if (n > sizeof in) return 4096u + 1u;
    memcpy(in, gx_out, n);
    switch (cmd) {
    case 0x30: gx_out[0] = 'G'; gx_out[1] = 'V'; gx_out[2] = GV_LK_VER; return 3u + gv_lk_song_get(gx_out + 3);
    case 0x31: e = gv_lk_song_set(in, n); return e ? 4096u + (uint32_t)e : gv_lk_song_get(gx_out);
    case 0x32: return gv_lk_ryt_get(gx_out);
    case 0x33: e = gv_lk_ryt_set(in, n); if (!e) gx_out[0] = in[0]; return e ? 4096u + (uint32_t)e : 1u;
    case 0x34: return n == 1u && in[0] < GV_NT ? gv_lk_mel_get(in[0], gx_out) : 4096u + 2u;
    case 0x35: e = gv_lk_mel_set(in, n); if (!e) { gx_out[0] = in[0]; gx_out[1] = in[1]; } return e ? 4096u + (uint32_t)e : 2u;
    default: return 4096u + 3u;
    }
}

#include "sim_demo.c"
#include "sim_lab.c"
