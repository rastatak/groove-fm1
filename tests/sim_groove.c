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
    check("the DJ filter takes the highs off (quieter)", r1 < r0 * 0.9);
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
    tap_btn(B_SAVE); check("SON, SAVE: the sound kept, the page stays", gu.page == GP_SON && gv_sndcur[0] >= GV_NSND);
    memset(gv_usr, 0, sizeof gv_usr); gv_sndcur[0] = 0;
    tap_btn(B_HOME); tap_btn(B_SAVE); check("SAVE: BANQUE", gu.page == GP_SAVE);
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
    {   /* GLO: PRESETS up/down, ALGORITHM left/right, SELECT the value, OCT- validates */
        int16_t sw; uint8_t al;
        tap_btn(B_HOME); tap_btn(B_GLO); gu.gcur = 0;
        check("GLO: starts on its first page", gu.page == GP_GLO);
        sim_enc(EN_ALGO, 1); run(FS / 20u); check("GLO: ALGORITHM right = SWING", gu.gcur == 1u && gu.page == GP_GLO);
        sw = song.g[G_SWING]; sim_enc(EN_SELECT, 1); run(FS / 20u); check("GLO: SELECT turns the SWING", song.g[G_SWING] != sw);
        sim_enc(EN_PRESET, 1); run(FS / 20u); sim_enc(EN_PRESET, 1); run(FS / 20u); sim_enc(EN_ALGO, -1); run(FS / 20u);
        check("GLO: PRESETS down twice, ALGORITHM left = ALGORITHM option", gu.gcur == 4u);
        al = gv.algo; sim_enc(EN_SELECT, 1); run(FS / 20u); check("GLO: SELECT changes what ALGORITHM does", gv.algo == al + 1u);
        shot("glo_opt");
        gv.algo = al;
        sim_enc(EN_PRESET, 1); run(FS / 20u); sim_enc(EN_PRESET, 1); run(FS / 20u); check("GLO: the cursor stops on the last row", gu.gcur == 6u);
        tap_btn(B_OCTDN); check("GLO: OCT- on MIDI opens the MIDI page", gu.page == GP_MIDI);
        tap_btn(B_HOME); gu.gcur = 0;
    }
    tap_btn(B_GLO); tap_btn(B_GLO); check("GLO x2: MIDI", gu.page == GP_MIDI); shot("midi");
    tap_btn(B_SAVE); tap_btn(B_SAVE); check("SAVE x2: RYTHMES", gu.page == GP_BKR); tap_key(STEP_KEY[2]); check("RYTHMES key 3: saved", gu.rbu & 4u); shot("bkr");
    tap_btn(B_SAVE); check("SAVE x3: MELODIES", gu.page == GP_BKM); gv.sel = 8; tap_key(STEP_KEY[1]); check("MELODIES key 2: saved", gu.mbu & 2u); shot("bkm");
    /* SELECT past the last track: TOUT, TOUT RYTHME, TOUTE MELODIE (volume and effects of the group) */
    tap_btn(B_HOME); while (gu.page != GP_LIVE) tap_btn(B_HOME);
    gv.sel = 13; gu.grp = 0; sim_enc(EN_SELECT, 2); run(FS / 20u); check("LIVE SELECT past FX: TOUT RYTHME", gu.grp == 2);
    { int16_t a = trk[GV_ROWTRK[0]].p[P_LEVEL], b2 = trk[GV_ROWTRK[3]].p[P_LEVEL]; uint8_t m = gv.t[0].vol;
      sim_enc(EN_K1, -5); run(FS / 20u);
      check("group volume: every drum row down, the melody kept", trk[GV_ROWTRK[0]].p[P_LEVEL] < a && trk[GV_ROWTRK[3]].p[P_LEVEL] < b2 && gv.t[0].vol == m); }
    shot("grp");
    sim_enc(EN_SELECT, 1); run(FS / 20u); check("then TOUTE MELODIE", gu.grp == 3);
    tap_btn(B_LFO); while (gu.page != GP_MIX) tap_btn(B_LFO);
    { uint8_t r0 = gv.t[0].rev, r4 = gv.t[4].rev; sim_enc(EN_K4, 4); run(FS / 20u); check("MIX, the melody group: reverb up on every track", gv.t[0].rev > r0 && gv.t[4].rev >= r4); } shot("grpmix");
    sim_enc(EN_SELECT, 1); run(FS / 20u); check("SELECT again: back to the first track", gu.grp == 0 && gv.sel == 0);
    /* ENV pages 5-8: FX LIVE's 16 effects on the shown track */
    gv.sel = 8; tap_btn(B_ENV); while (gu.page != GP_EFA) tap_btn(B_ENV);
    { uint8_t c0 = gv.t[0].cut, c1 = gv.t[1].cut; sim_enc(EN_K1, -10); run(FS / 20u); check("ENV FX A on the bass: its filter only", gv.t[0].cut < c0 && gv.t[1].cut == c1); } shot("efa");
    tap_btn(B_ENV); tap_btn(B_ENV); check("ENV x7: FX C", gu.page == GP_EFC);
    gv.sel = 2; gu.dirty = 1; run(FS / 20u); gv.rrl[2] = 0; gv.rrl[0] = 0; sim_enc(EN_K2, 1); run(FS / 20u);
    check("FX C on the CLAP: roll on that row only", gv.rrl[2] == 2 && gv.rrl[0] == 0); shot("efc"); gv.rrl[2] = 0;
    tap_btn(B_ENV); tap_btn(B_ENV); check("ENV x9: back to ENV", gu.page == GP_ENV); gv.sel = 8;
    /* MIDI PISTES: a melodic track's channel and output; its notes go out on USB MIDI */
    gv.sel = 8; tap_btn(B_EDIT); while (gu.page != GP_EMIDI) tap_btn(B_EDIT);
    sim_enc(EN_K1, 4); sim_enc(EN_K2, 1); run(FS / 20u);
    check("MIDI PISTES on the bass: channel 5, MIDI only", gv.t[0].mch == 5 && gv.t[0].mo == 1);
    if (!sim_playing()) tap_btn(B_PLAY);
    { uint32_t w0 = mo_w, k, on = 0, off = 0, chok = 1; usb.config = 1; run(FS * 2u);
      for (k = w0; k != mo_w; k++) { uint32_t pk = midi_out_q[k % MQ], st = (pk >> 8) & 0xF0u; if (st == 0x90u) { on++; if (((pk >> 8) & 15u) != 4u) chok = 0; } else if (st == 0x80u) off++; }
      printf("info  MIDI out: %u note on, %u note off\n", (unsigned)on, (unsigned)off);
      check("the bass's notes leave on USB MIDI, channel 5, each with its note off", on > 2u && chok && off + 1u >= on); }
    shot("emidi2"); gv.t[0].mo = 0;
    /* REC: a knob's moves are written step by step, then replay in a loop */
    gv.sel = 8; tap_btn(B_ENV); while (gu.page != GP_FLT) tap_btn(B_ENV);
    if (!sim_playing()) tap_btn(B_PLAY);
    gv.t[0].cut = 100; if (!gv_recq) tap_btn(B_REC);
    { uint32_t k; for (k = 0; k < 40u; k++) { sim_enc(EN_K1, -1); run(FS / 30u); } }
    check("REC + a knob turned: a lane for it", gv_am_find(KN_CUT, 0) != 0);
    tap_btn(B_REC);
    { uint8_t mn = 255, mx = 0; uint32_t k; for (k = 0; k < 80u; k++) { run(FS / 10u); if (gv.t[0].cut < mn) mn = gv.t[0].cut; if (gv.t[0].cut > mx) mx = gv.t[0].cut; }
      printf("info  replayed cutoff %u..%u\n", (unsigned)mn, (unsigned)mx);
      check("REC off: the cutoff replays its move in a loop", mx - mn > 15); }
    shot("amrec");
    gv_am_clear(); check("EFFACER: no lane left", !gv_am_any());
    /* HOME x5: the mixer and the general equaliser */
    tap_btn(B_HOME); while (gu.page != GP_MIXR) tap_btn(B_HOME);
    check("HOME x5: MIXEUR", gu.page == GP_MIXR);
    if (!sim_playing()) tap_btn(B_PLAY);
    { double fl, cut; run(FS); fl = rms_of(FS * 2u); sim_enc(EN_K2, -64); sim_enc(EN_K3, -64); run(FS / 2u); cut = rms_of(FS * 2u);
      printf("info  rms flat %.4f, lows and mids cut %.4f\n", fl, cut);
      check("EQ: lows and mids cut = quieter", gv.eq[0] == 0 && cut < fl * 0.93); shot("mixr");
      gv.eq[0] = gv.eq[1] = 64; }
    gv.sel = 0; gu.grp = 0; { int16_t l = trk[GV_ROWTRK[0]].p[P_LEVEL]; sim_enc(EN_K1, -6); run(FS / 20u); check("MIXEUR knob 1: the kick's fader", trk[GV_ROWTRK[0]].p[P_LEVEL] < l); }
    /* GLO MIDI: the list (SELECT the row, PRESETS the value) */
    tap_btn(B_GLO); while (gu.page != GP_MIDI) tap_btn(B_GLO);
    gu.mrow = 0; sim_enc(EN_SELECT, 6); run(FS / 20u); check("MIDI: SELECT goes down the list", gu.mrow == 6);
    { uint8_t c = gv.t[1].mch; sim_enc(EN_ALGO, 1); run(FS / 20u); check("row SYNTHE CANAL, ALGORITHM: its channel", gv.t[1].mch == c + 1 && gu.page == GP_MIDI); gv.t[1].mch = c; }
    sim_enc(EN_PRESET, 1); run(FS / 20u); check("MIDI: PRESETS goes down a row", gu.mrow == 7);
    sim_enc(EN_PRESET, -1); run(FS / 20u); check("MIDI: PRESETS goes up a row", gu.mrow == 6);
    shot("midi2");
    /* REC on the page shown: MIX's delay knob too */
    gv.sel = 9; gu.grp = 0; tap_btn(B_LFO); while (gu.page != GP_MIX) tap_btn(B_LFO);
    if (!gv_recq) tap_btn(B_REC); sim_enc(EN_K3, 3); run(FS / 4u); check("REC on MIX: the delay knob gets its lane", gv_am_find(KN_DLY, 0) != 0); tap_btn(B_REC); gv_am_clear();
    /* EDIT SON: SELECT picks in the list, the keyboard plays the sound */
    gv.sel = 2; gu.grp = 0; tap_btn(B_EDIT); while (gu.page != GP_SON) tap_btn(B_EDIT);
    { uint16_t pr = trk[GV_ROWTRK[2]].pre; sim_enc(EN_SELECT, 1); run(FS / 20u); check("SON on the CLAP: SELECT loads the next clap", trk[GV_ROWTRK[2]].pre != pr && gv_infam(2, (uint32_t)trk[GV_ROWTRK[2]].p[P_MODEL])); }
    if (sim_playing()) tap_btn(B_PLAY);
    run(FS * 2u); { double q0 = rms_of(FS / 4u), q1; sim_key(5, 1); q1 = rms_of(FS / 4u); sim_key(5, 0); printf("info  SON: silence %.4f, a key %.4f\n", q0, q1); check("SON: any key plays the row's sound", q1 > q0 + 0.003); }
    shot("sond2"); gv.sel = 8; sim_enc(EN_SELECT, 1); run(FS / 20u); shot("sonm2");
    { double q1; sim_key(STEP_KEY[4], 1); q1 = rms_of(FS / 4u); sim_key(STEP_KEY[4], 0); check("SON on the bass: a white key plays it", q1 > 0.003); }
    /* OCT- = keep, OCT+ = back (EDIT SON, STYLES) */
    tap_btn(B_HOME); gv.sel = 2; tap_btn(B_EDIT); while (gu.page != GP_SON) tap_btn(B_EDIT); run(FS / 20u);
    { uint16_t pr = trk[GV_ROWTRK[2]].pre; int16_t md = trk[GV_ROWTRK[2]].p[P_MODEL];
      sim_enc(EN_SELECT, 3); run(FS / 20u); check("SON: browsing loads another clap", trk[GV_ROWTRK[2]].pre != pr);
      tap_btn(B_OCTUP); check("OCT+: the sound it had is back, the page before too", trk[GV_ROWTRK[2]].pre == pr && trk[GV_ROWTRK[2]].p[P_MODEL] == md && gu.page != GP_SON);
      tap_btn(B_EDIT); while (gu.page != GP_SON) tap_btn(B_EDIT); run(FS / 20u); sim_enc(EN_SELECT, 2); run(FS / 20u); pr = trk[GV_ROWTRK[2]].pre;
      tap_btn(B_OCTDN); tap_btn(B_OCTUP); check("OCT- then OCT+: the validated sound stays", trk[GV_ROWTRK[2]].pre == pr); }
    tap_btn(B_ARP); while (gu.page != GP_STY) tap_btn(B_ARP); { uint8_t st = gv.style; sim_enc(EN_SELECT, 2); run(FS / 20u); tap_btn(B_OCTDN); check("STYLES: OCT- takes the style shown", gv.style != st && gv.style == gu.sty); }
    {   /* the PIANO, and SAVE on the SON page: the sound kept */
        uint16_t ids[64]; uint32_t cur, n, k, found = 0; uint8_t cut; double r1, r2;
        tap_btn(B_HOME); gv.sel = 9; tap_btn(B_EDIT); while (gu.page != GP_SON) tap_btn(B_EDIT); run(FS / 20u);
        n = gv_snd_ids(ids, &cur);
        for (k = 0; k < n + 2u && !found; k++) { sim_enc(EN_SELECT, 1); run(FS / 20u); if (gv.t[1].wave == GW_PIANO) found = 1; }
        check("SYNTHE: a PIANO in its sounds", found);
        shot("son_piano");
        if (song.playing) { sim_btn(B_PLAY, 1); run(FS / 50u); sim_btn(B_PLAY, 0); run(FS / 2u); }
        sim_key(STEP_KEY[4], 1); r1 = rms_of(FS / 4u); run(FS); r2 = rms_of(FS / 4u); sim_key(STEP_KEY[4], 0); run(FS / 4u);
        printf("info  PIANO held: first 1/4 s %.4f, after 1 s %.4f\n", r1, r2);
        check("PIANO: sounds, and the held note dies away", r1 > 0.003 && r2 < r1 * 0.7);
        gv.t[1].cut = 33; cut = gv.t[1].cut;
        tap_btn(B_SAVE); check("SON, SAVE: stays on the page, the sound is one of MES SONS", gu.page == GP_SON && gv_sndcur[1] >= GV_NSND);
        sim_enc(EN_SELECT, 1); run(FS / 20u); check("another sound loaded", gv.t[1].cut != cut || gv_sndcur[1] < GV_NSND);
        n = gv_snd_ids(ids, &cur);
        for (k = 0; k < n + 2u && gv_sndcur[1] < GV_NSND; k++) { sim_enc(EN_SELECT, 1); run(FS / 20u); }
        check("MON SON 1 back from the list: its cutoff too", gv_sndcur[1] >= GV_NSND && gv.t[1].cut == 33 && gv.t[1].wave == GW_PIANO);
        shot("son_usr");
        tap_btn(B_OCTDN);
        gv.sel = 2; run(FS / 20u); n = gv_snd_ids(ids, &cur);
        tap_btn(B_SAVE); check("CLAP, SAVE: kept in the user bank", trk[GV_ROWTRK[2]].pre >= PRE_NFACT && gv_snd_ids(ids, &cur) == n + 1u);
        shot("son_usr_drum");
        tap_btn(B_HOME);
    }
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
