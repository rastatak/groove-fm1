/* SPDX-License-Identifier: GPL-3.0-only
 * Groove on the simulator (tools/build_sim.sh): boots into Groove, plays, the pages draw, the keys and knobs act.
 * Writes build/sim/groove_*.ppm (the screens) and build/sim/groove.wav (a few bars) to look at and listen to. */
#include "sim_core.c"
#include <stdio.h>
#include <math.h>

static int fails;
static int check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok) fails++;
    return ok;
}
static float wav[FS * 8 * 2];
static uint32_t wn;
static double rms_acc;
static void run(uint32_t frames)
{
    while (frames) {
        uint32_t n = frames < SIM_MAX_FRAMES ? frames : SIM_MAX_FRAMES, i;
        sim_render(n);
        for (i = 0; i < n * 2u; i++) {
            float v = sim_audio()[i];
            rms_acc += (double)v * v;
            if (wn < sizeof wav / sizeof wav[0]) wav[wn++] = v;
        }
        frames -= n;
    }
}
static double rms_of(uint32_t frames) { rms_acc = 0; run(frames); return sqrt(rms_acc / (frames * 2.0)); }
static void shot(const char *name)
{
    char p[128];
    FILE *f;
    uint32_t i;
    const uint16_t *fb_ = sim_fb();
    ui.force = 1;
    gu.dirty = 1;
    run(FS / 20u);
    snprintf(p, sizeof p, "build/sim/groove_%s.ppm", name);
    f = fopen(p, "wb");
    if (!f) return;
    fprintf(f, "P6 240 240 255\n");
    for (i = 0; i < 240u * 240u; i++) {
        uint16_t c = fb_[i];
        fputc((c >> 11) << 3, f); fputc(((c >> 5) & 63) << 2, f); fputc((c & 31) << 3, f);
    }
    fclose(f);
}
static void tap_btn(uint32_t b) { sim_btn(b, 1); run(FS / 50u); sim_btn(b, 0); run(FS / 50u); }
static void tap_key(uint32_t k) { sim_key(k, 1); run(FS / 50u); sim_key(k, 0); run(FS / 50u); }
static void wav_out(void)
{
    FILE *f = fopen("build/sim/groove.wav", "wb");
    uint32_t i, n = wn, sr = FS, br = FS * 4u, ds = n * 2u, rs = 36u + ds;
    if (!f) return;
    fwrite("RIFF", 1, 4, f); fwrite(&rs, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f);
    { uint32_t x = 16; uint16_t a = 1, c = 2, ba = 4, bits = 16; fwrite(&x, 4, 1, f); fwrite(&a, 2, 1, f); fwrite(&c, 2, 1, f);
      fwrite(&sr, 4, 1, f); fwrite(&br, 4, 1, f); fwrite(&ba, 2, 1, f); fwrite(&bits, 2, 1, f); }
    fwrite("data", 1, 4, f); fwrite(&ds, 4, 1, f);
    for (i = 0; i < n; i++) { float v = wav[i] * 32767.f; int16_t s = (int16_t)(v > 32767.f ? 32767 : v < -32768.f ? -32768 : v); fwrite(&s, 2, 1, f); }
    fclose(f);
}

int main(void)
{
    uint32_t i, t0, n, mel = 0;
    double r0, r1;
    sim_groove = 1;
    sim_init(1);
    run(FS / 10u);
    check("Groove owns the panel at power-on", gv_on == 1);
    check("a style, a rhythm and two melodies at power-on", gv.t[0].st[0].n >= -1 && gv.tpl > 0);
    for (i = 0; i < 16u; i++) mel += gv.t[0].st[i].n >= 0;
    check("the bass has notes", mel > 0);
    shot("live");
    check("the LIVE page draws (title strip orange)", sim_fb()[4 * 240 + 6] != sim_fb()[100 * 240 + 239]);
    wn = 0;
    r0 = rms_of(FS / 4u);
    tap_btn(B_PLAY);
    check("PLAY starts", sim_playing());
    r1 = rms_of(FS * 3u);
    printf("info  rms stopped %.4f, playing %.4f\n", r0, r1);
    check("the groove sounds (drums + melodic tracks)", r1 > 0.01 && r1 < 0.7);
    t0 = (uint32_t)gv_hit_t[8];
    run(FS);
    check("the bass plays on the clock", (uint32_t)gv_hit_t[8] != t0 || gv_track_heard(8));
    shot("live_play");
    /* every page draws without crashing */
    tap_btn(B_HOME); shot("lryt"); check("HOME again: LIVE RYTHME", gu.page == GP_LRYT);
    tap_btn(B_HOME); shot("lmel"); check("HOME again: LIVE MELODIE", gu.page == GP_LMEL);
    tap_btn(B_SEQ); shot("ryt"); check("SEQ: RYTHME", gu.page == GP_RYT);
    n = trk[GV_ROWTRK[gv.sel & 7]].step[2].on;
    tap_key(STEP_KEY[2]);
    check("RYTHME: white key 3 toggles step 3", trk[GV_ROWTRK[gv.sel & 7]].step[2].on != n);
    tap_btn(B_SEQ); shot("mel"); check("SEQ again: MELODIE", gu.page == GP_MEL);
    tap_btn(B_ARP); shot("sty");
    tap_btn(B_ARP); shot("gryt");
    tap_btn(B_ARP); shot("gmel");
    tap_btn(B_FX); shot("fx");
    tap_btn(B_SCL); shot("scl");
    tap_btn(B_ENV); shot("env"); tap_btn(B_ENV); shot("flt"); tap_btn(B_ENV); shot("lfo");
    tap_btn(B_LFO); shot("mix");
    tap_btn(B_EDIT); shot("son");
    tap_btn(B_GLO); shot("glo");
    /* knobs */
    tap_btn(B_ENV);
    n = gv_tr()->att;
    sim_enc(EN_K1, 5); run(FS / 20u);
    check("ENV: KNOB 1 turns the attack", gv_tr()->att != n);
    /* HOME held 2 s: a new melody */
    tap_btn(B_HOME);
    gv.sel = 8;
    { gv_step_t old[16]; memcpy(old, gv.t[0].st, sizeof old);
      sim_btn(B_HOME, 1); run(FS * 21u / 10u); sim_btn(B_HOME, 0); run(FS / 20u);
      check("HOME held 2 s: a new melody for the track", memcmp(old, gv.t[0].st, sizeof old) != 0); }
    /* ARP held: style + everything */
    { uint32_t tp = gv.tpl; uint8_t st = gv.style;
      tap_btn(B_ARP); sim_enc(EN_SELECT, 3); run(FS / 20u);
      sim_btn(B_ARP, 1); run(FS * 21u / 10u); sim_btn(B_ARP, 0); run(FS / 20u);
      check("ARP held 2 s: the shown style taken, all regenerated", gv.style != st || gv.tpl != tp); }
    /* JEU: ALGORITHM right on LIVE, every key plays */
    tap_btn(B_HOME); while (gu.page != GP_LIVE) tap_btn(B_HOME);
    sim_enc(EN_ALGO, 1); run(FS / 20u);
    check("ALGORITHM right on LIVE: JEU", gu.kp == 1 && gv_kp == 1);
    shot("jeu");
    sim_enc(EN_ALGO, -1); run(FS / 20u);
    check("ALGORITHM left: back to the mutes", gu.kp == 0);
    /* FX LIVE: 4 pages of 4 effects on a target */
    tap_btn(B_HOME); while (gu.page != GP_LFX) tap_btn(B_HOME);
    check("HOME x4: FX LIVE", gu.page == GP_LFX);
    sim_enc(EN_K1, -40); sim_enc(EN_K2, 30); sim_enc(EN_K3, 20); sim_enc(EN_K4, 10); run(FS / 2u);
    check("FX LIVE A on the rhythm: the drums' filter closes", trk[GV_ROWTRK[0]].p[P_FTYPE] == FT_LP && trk[GV_ROWTRK[0]].p[P_FCUT] < 127);
    shot("fxA");
    sim_enc(EN_ALGO, 1); run(FS / 2u); shot("fxB");
    sim_enc(EN_ALGO, 1); run(FS / 20u);
    check("ALGORITHM: page C", gu.fxpg == 2);
    sim_enc(EN_K1, 30); sim_enc(EN_K2, 1); sim_enc(EN_K3, 15); sim_enc(EN_K4, 1); run(FS / 2u);
    check("page C on the rhythm: roll x2, skips, reverse", gv.rrl[0] == 2 && gv.rsk[0] > 0 && gv.rrv[0] == 1);
    shot("fxC");
    { double r = rms_of(FS); check("the drums still sound with C's effects", r > 0.005); }
    sim_enc(EN_SELECT, 2); run(FS / 20u);
    check("SELECT: the target is the bass", gu.fxt == 2);
    sim_enc(EN_ALGO, 1); run(FS / 20u);
    sim_enc(EN_K1, 2); sim_enc(EN_K2, 30); sim_enc(EN_K3, 30); sim_enc(EN_K4, 1); run(FS / 2u);
    check("page D on the bass: loop, crush, gate, half tempo", gv.t[0].lp == 2 && gv.t[0].cr > 0 && gv.t[0].gt > 0 && gv.t[0].hf == 1 && gv.t[2].lp == 0);
    shot("fxD");
    { double r = rms_of(FS); check("the bass still sounds with D's effects", r > 0.005); }
    tap_key(STEP_KEY[13]);
    check("white key 14: the bass's bitcrush cut", gv.t[0].cr == 0 && gu.byp[2][FE_CR] > 0);
    tap_key(STEP_KEY[13]);
    check("again: back", gv.t[0].cr > 0);
    sim_btn(B_FX, 1); run(FS * 21u / 10u); sim_btn(B_FX, 0); run(FS / 20u);
    check("FX held on FX LIVE: the target's effects to zero", gv.t[0].lp == 0 && gv.t[0].hf == 0 && gv.t[0].cr == 0);
    /* the web app's pages: FILTRE DJ, FX x5, VISU, LOOP ROLL, TAP / ALGO */
    tap_btn(B_HOME); while (gu.page != GP_LIVE) tap_btn(B_HOME);
    check("LIVE knob 2 = FILTRE DJ, open at power-on", gv.mf == 127 && gv.hp == 0);
    r0 = rms_of(FS);
    sim_enc(EN_K2, -60); run(FS / 20u);
    check("FILTRE DJ left: the low-pass closes", gv.mf < 60 && gv.hp == 0);
    r1 = rms_of(FS);
    printf("info  rms open %.4f, DJ low-pass %.4f\n", r0, r1);
    check("the DJ filter takes the highs off (quieter)", r1 < r0);
    sim_enc(EN_K2, 120); run(FS / 20u);
    check("FILTRE DJ right: the high-pass opens", gv.hp > 0 && gv.mf == 127);
    sim_enc(EN_K2, -200); sim_enc(EN_K2, 64); run(FS / 20u);
    tap_btn(B_FX); check("FX: FX RECAP first", gu.page == GP_FXR); shot("fxr");
    tap_key(STEP_KEY[0]); check("RECAP key 1: the low-pass cut (open)", gv.mf == 127);
    tap_btn(B_FX); check("FX again: FX FILTRE", gu.page == GP_FXA);
    sim_key(STEP_KEY[1], 1); run(FS / 4u);
    check("FX pad LOOP 2 held: on", gv_plp == 2);
    shot("fxa");
    sim_key(STEP_KEY[1], 0); run(FS / 20u);
    check("let go: off", gv_plp == 0);
    tap_key(STEP_KEY[11]); tap_key(STEP_KEY[4]);
    check("VERROU, then ROLL x2: it stays", gu.lock && gv_prl == 2);
    tap_key(STEP_KEY[4]); check("pressed again: off", gv_prl == 0);
    tap_key(STEP_KEY[11]);
    tap_btn(B_FX); tap_btn(B_FX); check("FX x4: FX BATT", gu.page == GP_FXD); shot("fxd");
    tap_btn(B_FX); check("FX x5: FX PERF", gu.page == GP_FXP); shot("fxp");
    tap_btn(B_ARP); tap_btn(B_ARP); check("ARP x2: VISU", gu.page == GP_VIS); shot("vis");
    tap_btn(B_LFO); tap_btn(B_LFO); check("LFO x2: LOOP ROLL", gu.page == GP_LRR);
    gv.sel = 8; tap_key(STEP_KEY[4]); check("LOOP ROLL key 5: the bass loops 4 steps", gv.t[0].lp == 4); shot("lrr");
    tap_key(STEP_KEY[0]);
    tap_btn(B_GLO); tap_key(STEP_KEY[3]); check("GLO key 4: ALGORITHM = BPM", gv.algo == 1u);
    { int16_t bpm = song.g[G_BPM]; tap_btn(B_ENV); sim_enc(EN_ALGO, 3); run(FS / 20u); check("ALGORITHM turns the BPM now", song.g[G_BPM] == bpm + 3); }
    tap_btn(B_GLO); tap_key(STEP_KEY[2]); shot("glo2");
    tap_btn(B_PLAY); if (!sim_playing()) tap_btn(B_PLAY);
    gv.sel = 8; tap_btn(B_ENV); while (gu.page != GP_ENV) tap_btn(B_ENV); run(FS / 3u); shot("envp");
    tap_btn(B_ENV); run(FS / 5u); shot("fltp"); gv.t[0].lfd = 80; tap_btn(B_ENV); run(FS / 5u); shot("lfop");
    tap_btn(B_ENV); run(FS / 2u); shot("oscp");
    tap_btn(B_LFO); while (gu.page != GP_MIX) tap_btn(B_LFO); gv.t[0].dist = 60; gv.t[0].dly = 70; gv.t[0].rev = 90; run(FS / 5u); shot("mixp");
    tap_btn(B_LFO); gv.t[0].lp = 4; run(FS / 5u); shot("lrrp"); gv.t[0].lp = 0;
    tap_btn(B_SCL); { uint32_t k; for (k = 0; k < 6u; k++) { gv_set_scale(k); run(FS / 10u); { char n[16]; snprintf(n, sizeof n, "scl%u", (unsigned)k); shot(n); } } }
    gv.xd = 60; gv.xr = 80; gv.xs = 50; gv.mf = 90; gv.hp = 20; gv.mq = 70;
    tap_btn(B_FX); while (gu.page != GP_FXR) tap_btn(B_FX); shot("fxrp"); tap_btn(B_FX); shot("fxap"); tap_btn(B_FX); shot("fxbp"); tap_btn(B_FX); shot("fxdp"); tap_btn(B_FX); shot("fxpp");
    tap_btn(B_ARP); while (gu.page != GP_STY) tap_btn(B_ARP); run(FS / 5u); shot("styp");
    tap_btn(B_ARP); run(FS / 5u); shot("visp"); tap_btn(B_ARP); shot("grytp"); tap_btn(B_ARP); shot("gmelp");
    tap_btn(B_GLO); shot("glop");
    gv.sel = 8; tap_btn(B_EDIT); shot("sonm"); gv.sel = 0; gu.dirty = 1; shot("sond"); gv.sel = 8;
    tap_btn(B_SAVE); check("SAVE: BANQUE", gu.page == GP_SAVE);
    { uint16_t r0 = gv_rhythm_row_get(0);
      tap_key(STEP_KEY[0]); check("SAVE key 1 (empty): the rhythm saved", gu.rbu & 1u);
      gv_rhythm_row_set(0, 0x1234); tap_key(STEP_KEY[0]); check("key 1 again: loaded back", gv_rhythm_row_get(0) == r0);
      tap_key(STEP_KEY[8]); check("key 9: the bass's melody saved", gu.mbu & 1u);
      sim_enc(EN_K1, 2); run(FS / 20u); tap_key(STEP_KEY[8]); check("MODE EFFACER, key 9: gone", !(gu.mbu & 1u));
      sim_enc(EN_K1, -2); run(FS / 20u); }
    shot("savep");
    /* LIVE: OCT- / OCT+ cut all the rhythm / the melody; PRESETS: the row's family of sounds */
    tap_btn(B_HOME); while (gu.page != GP_LIVE) tap_btn(B_HOME);
    if (!sim_playing()) tap_btn(B_PLAY);
    tap_btn(B_OCTDN); check("LIVE OCT-: all the rhythm cut", gv.gmute & 1u);
    { uint32_t h0 = gv_hit_t[0]; double r; run(FS); r = rms_of(FS / 2u); (void)h0; printf("info  rms rhythm cut %.4f\n", r); }
    tap_btn(B_OCTDN); check("OCT- again: back", !(gv.gmute & 1u));
    tap_btn(B_OCTUP); check("LIVE OCT+: all the melody cut", gv.gmute & 2u); tap_btn(B_OCTUP);
    gv.sel = 2;
    { uint32_t m0 = (uint32_t)trk[GV_ROWTRK[2]].p[P_MODEL], k, ok = 1;
      for (k = 0; k < 5u; k++) { sim_enc(EN_PRESET, 1); run(FS / 10u); if (!gv_infam(2, (uint32_t)trk[GV_ROWTRK[2]].p[P_MODEL])) ok = 0; }
      check("PRESETS on the CLAP: only claps", ok);
      check("PRESETS: the sound changed", (uint32_t)trk[GV_ROWTRK[2]].p[P_MODEL] != m0 || trk[GV_ROWTRK[2]].pre != 0xFFFFu); }
    shot("presets");
    gv.sel = 8; { uint8_t c0 = gv.t[0].cut; sim_enc(EN_PRESET, 1); run(FS / 20u); sim_enc(EN_PRESET, 1); run(FS / 20u); check("PRESETS on the bass: a bass sound", gv.t[0].cut != c0 || gv_sndcur[0] != 0); }
    sim_enc(EN_ALGO, 1); run(FS / 20u); { uint8_t kn[4]; gv_knobs(kn); check("JEU: KNOB 4 = the keyboard's octave", kn[3] == KN_KOCT); } sim_enc(EN_ALGO, -1); run(FS / 20u);
    tap_btn(B_ARP); while (gu.page != GP_VIS) tap_btn(B_ARP);
    { uint8_t st = gv.style, k; sim_enc(EN_SELECT, 1); run(FS / 20u); check("VISU SELECT: another style shown, not yet taken", gu.sty != st && gv.style == st);
      sim_btn(B_ARP, 1); run(FS * 21u / 10u); sim_btn(B_ARP, 0); run(FS / 20u); check("ARP held on VISU: taken", gv.style == gu.sty);
      for (k = 0; k < GV_NSTY; k++) { char n[16]; gu.sty = k; snprintf(n, sizeof n, "st%u", (unsigned)k); shot(n); } gu.sty = gv.style; }
    gv.sel = 0; tap_btn(B_EDIT); while (gu.page != GP_SON) tap_btn(B_EDIT);
    tap_btn(B_EDIT); check("EDIT x2: SON REGLAGES", gu.page == GP_SONR); shot("sonr");
    tap_btn(B_EDIT); check("EDIT x3: EDITER", gu.page == GP_SEDIT); { int16_t e = trk[GV_ROWTRK[0]].p[P_E2]; sim_enc(EN_K1, 4); run(FS / 20u); check("EDITER knob 1: the voice's 3rd knob", trk[GV_ROWTRK[0]].p[P_E2] != e); } shot("sedit");
    tap_btn(B_EDIT); check("EDIT x4: MIDI PISTES", gu.page == GP_EMIDI); shot("emidi");
    tap_btn(B_GLO); tap_btn(B_GLO); check("GLO x2: MIDI", gu.page == GP_MIDI); shot("midi");
    tap_btn(B_SAVE); tap_btn(B_SAVE); check("SAVE x2: RYTHMES", gu.page == GP_BKR); tap_key(STEP_KEY[2]); check("RYTHMES key 3: saved", gu.rbu & 4u); shot("bkr");
    tap_btn(B_SAVE); check("SAVE x3: MELODIES", gu.page == GP_BKM); gv.sel = 8; tap_key(STEP_KEY[1]); check("MELODIES key 2: saved", gu.mbu & 2u); shot("bkm");
    /* GLO, key 16: Hortator; ENV+ARP: back */
    tap_btn(B_GLO); tap_key(STEP_KEY[15]);
    check("GLO key 16: Hortator's own pages", gv_on == 0);
    sim_btn(B_ENV, 1); run(FS / 50u); tap_btn(B_ARP); sim_btn(B_ENV, 0); run(FS / 50u);
    check("ENV held + ARP: back to Groove", gv_on == 1);
    run(FS);
    wav_out();
    printf("%s (%d failed)\n", fails ? "sim_groove: FAILED" : "sim_groove: all ok", fails);
    return fails != 0;
}
