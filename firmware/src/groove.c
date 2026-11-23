/* SPDX-License-Identifier: GPL-3.0-only
 * Groove (FM-1 Groove Studio) on Hortator: 2026, the Groove authors. Hortator: DEADACTIVE, on Felucca by Leo Kuroshita.
 * The engine: 6 melodic tracks (BASSE, SYNTHE, BASSE 2, SYNTHE 2, NAPPE, FX) played by small subtractive voices,
 * their 16-step patterns, and the web app's generators (styles, scales, chord progressions, melodies), on top of
 * Hortator's 8 drum tracks. Same rules as the web app (data.js / app.js), so a groove sounds and behaves the same.
 * Runs in the audio ISR (gv_tick, gv_render, gv_keys_isr) and in the main loop (gv_gen_*, the UI: groove_ui.c). */
#include <stdarg.h>
#include "gv_data.h"
static int gv_abs(int v) { return v < 0 ? -v : v; }
/* a tiny printf for the screen: %s %d %+d %% */
#define GV_FMT_N 44                /* gv_fmt writes at most this, the 0 included: every buffer given to it is this long */
static void gv_fmt(char *b, const char *f, ...)
{
    va_list ap;
    char *o = b, *e = b + GV_FMT_N - 1;
    va_start(ap, f);
    while (*f && o < e) {
        if (*f != '%') { *o++ = *f++; continue; }
        f++;
        if (*f == '%') { *o++ = '%'; f++; continue; }
        if (*f == 's') { const char *s = va_arg(ap, const char *); while (*s && o < e) *o++ = *s++; f++; continue; }
        {
            int plus = *f == '+', v, n = 0;
            char d[12];
            unsigned u;
            if (plus) f++;
            v = va_arg(ap, int);
            if (v < 0) { if (o < e) *o++ = '-'; u = (unsigned)(-v); } else { if (plus && o < e) *o++ = '+'; u = (unsigned)v; }
            do { d[n++] = (char)('0' + u % 10u); u /= 10u; } while (u);
            while (n && o < e) *o++ = d[--n];
            f++;
        }
    }
    *o = 0;
    va_end(ap);
}

#define GV_NT 6                    /* melodic tracks */
#define GV_FS 44100u
enum { GT_BASS, GT_SYNTH, GT_PAD, GT_FX };
enum { GW_SAW, GW_SQR, GW_TRI, GW_PIANO };
enum { GF_LP, GF_HP, GF_BP };
/* the web app's rows of drums on Hortator's 8 tracks (power-on kit: kick snare clap hat-closed hat-open tom rim cymbal;
 * Groove puts a conga on track 8 for PERC) */
static const uint8_t GV_ROWTRK[8] = {0, 1, 2, 3, 4, 7, 5, 6};
static const char *const GV_ROWN[8] = {"KICK", "SNARE", "CLAP", "HAT F.", "HAT O.", "PERC", "LOW", "RIM"};
static const char *const GV_TRN[GV_NT] = {"BASSE", "SYNTHE", "BASSE 2", "SYNTHE 2", "NAPPE", "FX"};
static const uint8_t GV_TTYPE[GV_NT] = {GT_BASS, GT_SYNTH, GT_BASS, GT_SYNTH, GT_PAD, GT_FX};
static const uint8_t GV_AUS[7] = {0, 4, 8, 16, 32, 64, 128};

#define GV_MAXS 64u                 /* a track's steps at most (the drums': Hortator's NSTEP) */
typedef struct {                   /* one step (the web app's {n, a, s, o, l}) */
    int8_t n;                      /* scale degree, -1 none */
    uint8_t a : 1, s : 1, o4 : 2;  /* accent, slide, offset in quarter steps */
    uint8_t l4;                    /* length in quarter steps (4 = one step) */
} gv_step_t;

typedef struct {                   /* a melodic track; knobs 0..127 like Hortator's */
    uint8_t mute, wave, oct, ftype, lfw, lfs, ar, bw;
    int8_t po;                     /* pitch, whole octaves -3..3 */
    uint8_t vol, cut, res, env, att, dec, gl, lfr, lfd, dist, dly, rev;
    uint8_t mo, mch;               /* MIDI out (USB): 0 the inner sound, 1 MIDI only, 2 both; its channel 1..16 */
    uint8_t ap, vb, rm, wf, sat, fo;  /* FX LIVE pages F G: auto-pan, vibrato, ring modulation, wavefolder, saturation, the
                                    * vowel filter (0 off, 1..127 A E I O U) */
    int8_t ps;                     /* pitch shift, semitones -12..12 */
    uint8_t pan, sc, tr, ts;       /* FX LIVE page E: pan (64 the middle), sidechain (the kick ducks it), tremolo (an 1/8
                                    * note), tape stop (the track slows down to a halt) */
    uint8_t cho, rl, sk, rv, lp, cr, gt, hf;   /* FX LIVE pages C D: chorus, roll x2-4, skipped steps, reverse, loop
                                    * 1-8 steps, bitcrush, gate, half tempo */
    gv_step_t st[GV_MAXS];         /* its steps: len of them play (16, 32 or 64: 1, 2 or 4 bars) */
    uint8_t len;
} gv_trk_t;

typedef struct {                   /* everything Groove keeps (gv_save: flash) */
    uint32_t magic;
    uint8_t root, scale, prog, style, autob, oct;
    int8_t sel;                    /* the track shown: 0..7 drum rows, 8..13 melodic tracks */
    uint8_t gmute;                 /* bit 0 all the rhythm, bit 1 all the melody */
    uint16_t solo;                 /* bit per track 0..13 */
    uint8_t tpl;                   /* the template the drums came from (+1), 0 hand made */
    uint8_t rowar[8];              /* auto-random per drum row, bars */
    uint8_t rrl[8], rsk[8], rrv[8], rlp[8], rhf[8];   /* FX LIVE on the drum rows: roll, skipped steps, reverse, loop,
                                    * half tempo (seq.c asks gv_drum_step) */
    uint8_t eq[3];                 /* the general equaliser: lows, mids, highs (64 flat, 0 cut, 127 +6 dB) */
    uint8_t mf, hp, mq, xs, xd, xr;  /* the whole mix (web: FILTRE DJ): low-pass (127 open), high-pass (0 open), resonance,
                                    * disto, echo and reverb sends */
    uint8_t dtr, dap, drm, dwf, dsat, dfo;   /* the drums' own (all 8 rows together, on their mix): tremolo, auto-pan, ring
                                    * modulation, wavefolder, saturation, vowel */
    uint8_t rlen;                  /* the rhythm's steps (16, 32 or 64), the 8 rows alike */
    uint8_t rsrc, msrc;            /* where a new rhythm / a new melody comes from: 0 the generator, then the built-in
                                    * base (the style's genres, every genre) and the bank (SAVE) - gv_src_rhythm, gv_src_mel */
    uint8_t libk;                  /* the base's last motif + 1 (0: none) */
    uint8_t nr;                    /* the noise riser (one for the whole mix): louder and brighter as it goes up */
    uint8_t algo;                  /* ALGORITHM off the LIVE pages: 0 pages 1 BPM 2 FILTRE DJ 3 VOLUME 4 RYTHMES 5 SWING */
    gv_trk_t t[GV_NT];
} gv_state_t;
#define GV_MAGIC 0x47525631u       /* "GRV1" */

typedef struct {                   /* a voice, ISR only */
    uint32_t ph[3], inc[3], tinc[3];
    int32_t amp, atgt;             /* amplitude Q24 and where it goes */
    int32_t fenv;                  /* filter envelope Q15 */
    int32_t lp, bp;                /* SVF */
    int32_t dlp;                   /* DIST tone */
    uint32_t gate;                 /* samples left of the note, 0 = released */
    uint32_t delay;                /* samples before it starts (an offset step) */
    uint8_t acc, slide, on, nosc, fxs;
    uint32_t lph;                  /* LFO phase */
    uint32_t pinc[3];              /* the note waiting for its delay */
    uint32_t pgate;
    uint8_t pacc, pslide, pn;
    uint8_t pnote, mnote, mon, mchn; /* MIDI out: the note waiting, the one held on the other machine, its channel */
    uint32_t rph;                  /* ring modulation: the carrier's phase */
    int32_t f1l, f1b, f2l, f2b;    /* the vowel filter: two band-passes */
    uint32_t tsl;                  /* tape stop: how far it has slowed, Q16 (0 full speed .. 127 << 16 stopped) */
    int32_t crs; uint32_t crh, rlc;  /* bitcrush: the held sample, its countdown; roll: samples to the next hit */
} gv_voice_t;

static gv_state_t gv;
static uint32_t gv_step_len(uint32_t pos);
static int gv_track_heard(uint32_t k);
static void midi_out_event(uint32_t pkt);        /* usb_app.c: a packet into the USB MIDI out ring (from the audio ISR) */
static volatile uint32_t gv_on_ms[6];           /* when each melodic track's last note began (the ENV page's dot) */
static int16_t gv_scope[128];                   /* the shown track's last samples, 1 in 8 (the OSC page) */
static volatile uint8_t gv_scw, gv_scope_trk, gv_scdec;
/* FX pads held (web: LOOP 1-8, ROLL x2-4, REVERSE) on a target: 0 all, 1 the rhythm, 2 the melody */
static volatile uint8_t gv_plp, gv_prl, gv_prv, gv_ptg;
static int gv_pon(int drum) { return gv_ptg == 0u || gv_ptg == (drum ? 1u : 2u); }

static int gv_ui_input(uint32_t pressed, uint32_t notes, uint32_t now);   /* groove_ui.c */
static int gv_ui_draw(void);
static int gv_ui_leds(void);
static volatile uint8_t gv_kp;                  /* JEU on a LIVE page: every key plays the shown track */
static volatile uint8_t gv_recq;                /* REC on: keys write into the pattern */
static gv_voice_t gvv[GV_NT];
#ifndef GV_ON_DEFAULT
#define GV_ON_DEFAULT 1                 /* the device: Groove at power-on (the simulator: sim_init decides) */
#endif
static volatile uint8_t gv_on = GV_ON_DEFAULT;             /* the Groove UI owns the panel (else Hortator's own) */
static volatile uint8_t gv_pos, gv_bar, gv_cix; /* step 0..15, bars since PLAY, the chord playing (0..3) */
static uint32_t gv_acc_s, gv_stepn;            /* samples into the step, its length */
static volatile uint32_t gv_hit_req;            /* main -> ISR: drum rows to hit now (bit per row) */
static volatile int16_t gv_live_deg = -1000;    /* main -> ISR: the degree a black key plays on the shown track */
static volatile uint8_t gv_live_trk = 0xFF, gv_live_off;
static volatile uint8_t gv_regen_due;           /* ISR -> main: bit 0 a style auto-random, bits 1.. track auto-randoms */
static const uint32_t GV_SEMI[25] = {32768, 34716, 36781, 38968, 41285, 43740, 46341, 49097, 52016, 55109, 58386, 61858, 65536, 69433, 73562, 77936, 82570, 87480, 92682, 98193, 104032, 110218, 116772, 123715, 131072};   /* 2^(k/12) Q16, k -12..12 */
static const uint8_t GV_F1[5] = {68, 52, 45, 55, 47}, GV_F2[5] = {77, 88, 91, 68, 65};   /* A E I O U: the two formants, as cutoffs */
static uint32_t gv_vbp, gv_nzs = 0x1234567u;    /* the vibrato's phase (~6 Hz), the noise's state */
static int32_t gv_nzl, gv_df1l, gv_df1b, gv_df2l, gv_df2b;   /* (the drums' vowel filter) */
static uint32_t gv_drph;                        /* (the drums' ring modulation: its carrier, 150 Hz) */
static int32_t gv_sck;                         /* sidechain: the kick's envelope, Q15 (32767 at the hit, then back to 0) */
static uint8_t gv_scauto;                      /* the sidechain was set by the generator (TRAP, DRILL): another style clears it */
static volatile uint32_t gv_hit_t[14];          /* last hit per track (song.tick), the screen's flashes */
static const uint32_t gv_inc_tab[128] = {   /* MIDI note -> phase increment at 44.1 kHz (no float on the device) */
        796254u,     843601u,     893765u,     946911u,    1003217u,    1062871u,    1126073u,    1193033u,
       1263974u,    1339134u,    1418763u,    1503127u,    1592507u,    1687203u,    1787529u,    1893821u,
       2006434u,    2125742u,    2252146u,    2386065u,    2527948u,    2678268u,    2837526u,    3006254u,
       3185015u,    3374406u,    3575058u,    3787642u,    4012867u,    4251485u,    4504291u,    4772130u,
       5055896u,    5356535u,    5675051u,    6012507u,    6370030u,    6748811u,    7150117u,    7575285u,
       8025735u,    8502970u,    9008582u,    9544261u,   10111792u,   10713070u,   11350103u,   12025015u,
      12740059u,   13497623u,   14300233u,   15150569u,   16051469u,   17005939u,   18017165u,   19088521u,
      20223584u,   21426141u,   22700205u,   24050030u,   25480119u,   26995246u,   28600467u,   30301139u,
      32102938u,   34011878u,   36034330u,   38177043u,   40447168u,   42852281u,   45400411u,   48100060u,
      50960238u,   53990491u,   57200933u,   60602278u,   64205876u,   68023757u,   72068660u,   76354085u,
      80894335u,   85704563u,   90800821u,   96200119u,  101920476u,  107980983u,  114401866u,  121204555u,
     128411753u,  136047513u,  144137319u,  152708170u,  161788671u,  171409126u,  181601643u,  192400238u,
     203840952u,  215961966u,  228803732u,  242409110u,  256823506u,  272095026u,  288274639u,  305416341u,
     323577341u,  342818251u,  363203285u,  384800477u,  407681904u,  431923931u,  457607465u,  484818220u,
     513647012u,  544190053u,  576549277u,  610832681u,  647154683u,  685636503u,  726406571u,  769600953u,
     815363807u,  863847862u,  915214929u,  969636441u, 1027294024u, 1088380105u, 1153098554u, 1221665363u};
static uint32_t gv_rng = 0x2545F491u;

static uint32_t gv_rand(void) { gv_rng ^= gv_rng << 13; gv_rng ^= gv_rng >> 17; gv_rng ^= gv_rng << 5; return gv_rng; }
static uint32_t gv_rnd(uint32_t n) { return n ? gv_rand() % n : 0u; }
static int gv_chance(uint32_t pct) { return gv_rnd(100) < pct; }
#define GV_PICK(a) ((a)[gv_rnd(sizeof(a) / sizeof((a)[0]))])
static int32_t gv_clamp(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }

static uint32_t gv_L(void) { return GV_SCL[gv.scale % GV_NSC]; }
static int gv_t3(void) { return GV_SCP[gv.scale % GV_NSC][1]; }
static int gv_t5(void) { return GV_SCP[gv.scale % GV_NSC][2]; }
static const uint8_t *gv_progp(void) { return gv.prog && gv.prog <= GV_NPROG[gv.scale % GV_NSC] ? GV_PROG[gv.scale % GV_NSC][gv.prog - 1u] : 0; }
/* the chord of the bar, as a degree offset (high chords played under the root, as the web app's chOff) */
static int gv_choff(void)
{
    const uint8_t *p = gv_progp();
    int r, L = (int)gv_L();
    if (!p || !song.playing)
        return 0;
    r = p[gv_cix & 3u];
    return r > L / 2 ? r - L : r;
}
/* degree -> MIDI note on a track (the web app's d2m, + its pitch) */
static int gv_d2m(const gv_trk_t *t, int d)
{
    int L = (int)gv_L(), sc = gv.scale % GV_NSC, k = ((d % L) + L) % L, o = (d - k) / L;
    return gv_clamp((int)gv.root + 12 * (t->oct + 1) + GV_SC[sc][k] + 12 * o + 12 * t->po, 0, 127);
}

/* ------------------------------------------------------------------ the generators (main loop) --- */
static void gv_mel_clear(gv_trk_t *t)
{
    uint32_t c;
    for (c = 0; c < 16u; c++) {
        t->st[c].n = -1; t->st[c].a = t->st[c].s = 0; t->st[c].o4 = 0; t->st[c].l4 = 4;
    }
}
static uint8_t gv_l4(uint32_t q) { return (uint8_t)(q < 1u ? 1u : q > 64u ? 64u : q); }
static uint32_t gv_lenok(uint32_t n) { return n >= 64u ? 64u : n >= 32u ? 32u : 16u; }
static uint32_t gv_tlen(uint32_t i) { return gv_lenok(gv.t[i % GV_NT].len); }   /* a melodic track's steps */
static uint32_t gv_rlen(void) { return gv_lenok(gv.rlen); }                    /* the rhythm's */
/* a new melody for track i: rhythm on the grid (straight or syncopated as the scale likes), chord notes on the beats,
 * passing notes between, the half bar repeated, an ending that resolves; arpeggios on synths; pads hold the chord */
static int gv_src_rhythm(void);                /* (groove_ui.c: the base, the bank; 0 = the generator's turn) */
static int gv_src_mel(uint32_t i);
static void gv_vary_bar(uint32_t i, uint32_t bar);
static void gv_gen_bar(uint32_t i);
/* the whole track: its first bar generated; 2 bars: the second is the first, varied; 4 bars: A A' B A' (B another one) */
static void gv_mel_fill(uint32_t i)                /* bars 2.. from the first */
{
    gv_trk_t *t = &gv.t[i];
    uint32_t n = gv_tlen(i) / 16u;
    if (n >= 2u) { memcpy(&t->st[16], &t->st[0], 16u * sizeof t->st[0]); gv_vary_bar(i, 1); }
    if (n >= 4u) {
        gv_step_t a[16];
        memcpy(a, &t->st[0], sizeof a);
        gv_gen_bar(i);
        memcpy(&t->st[32], &t->st[0], sizeof a);
        memcpy(&t->st[0], a, sizeof a);
        memcpy(&t->st[48], &t->st[16], sizeof a);
    }
}
static void gv_gen_mel(uint32_t i)
{
    if (gv_src_mel(i)) return;                     /* (the source is the base or the bank) */
    gv_gen_bar(i);
    gv_mel_fill(i);
}
static void gv_gen_bar(uint32_t i)                 /* (into the track's first 16 steps) */
{
    gv_trk_t *t = &gv.t[i];
    int L = (int)gv_L(), t3 = gv_t3(), t5 = gv_t5(), ty = GV_TTYPE[i], bass = ty == GT_BASS;
    int psy = bass && gv.style == 3u;              /* PSYTRANCE: the rolling bass */
    uint32_t c;
    gv_mel_clear(t);
    if (ty == GT_PAD) {
        if (gv_chance(35)) { t->st[0].n = 0; t->st[0].l4 = 32; t->st[8].n = (int8_t)(gv_chance(50) ? 0 : t3); t->st[8].l4 = 32; }
        else { t->st[0].n = 0; t->st[0].l4 = 64; }
        return;
    }
    if (ty == GT_FX) {
        static const uint8_t POS[6] = {0, 4, 8, 12, 6, 14};
        uint8_t p[6];
        uint32_t k, n = 1u + gv_rnd(3);
        for (k = 0; k < 6u; k++) p[k] = POS[k];
        for (k = 5; k > 0; k--) { uint32_t j = gv_rnd(k + 1u); uint8_t x = p[k]; p[k] = p[j]; p[j] = x; }
        for (k = 0; k < n; k++) {
            int ch[4] = {L, L + t3, L + t5, t5};
            static const uint8_t LN[3] = {4, 8, 8};
            t->st[p[k]].n = (int8_t)ch[gv_rnd(4)];
            t->st[p[k]].l4 = GV_PICK(LN);
        }
        return;
    }
    {
        uint16_t rh;
        int hits[16], nh = 0, k, last, d, arp = -1, al;
        static const int8_t ARPS[6][6] = {{0, 3, 5, 7, -1, -1}, {0, 5, 7, 5, -1, -1}, {7, 5, 3, 0, -1, -1},
                                          {0, 3, 5, 7, 5, 3}, {0, 7, 5, 7, 3, 7}, {0, 5, 10, 7, -1, -1}};
        static const uint8_t ARPN[6] = {4, 4, 4, 6, 6, 4};
        static const uint8_t AL[3] = {2, 3, 4};
        int ct[6] = {0, t3, t5, L, L + t3, L + t5};
        uint32_t sy = GV_SCP[gv.scale % GV_NSC][0] * (bass ? 10u : 16u) / 10u;
        int syn = gv_chance(sy > 70u ? 70u : sy);
        if (psy)
            rh = 0x7777;
        else if (bass)
            rh = syn ? GV_PICK(GV_RH_BASS_B) : GV_PICK(GV_RH_BASS_A);
        else
            rh = syn ? GV_PICK(GV_RH_SYNTH_B) : GV_PICK(GV_RH_SYNTH_A);
        for (c = 0; c < 16u; c++)
            if ((rh >> (15u - c)) & 1u)
                hits[nh++] = (int)c;
        last = nh - 1;
        if (!bass && gv_chance(50))
            arp = (int)gv_rnd(6);
        al = GV_PICK(AL);
        { int d0[3] = {t3, t5, L}; d = d0[gv_rnd(3)]; }
        for (k = 0; k < nh; k++) {
            int cc = hits[k], f = (k < last ? hits[k + 1] : 16) - cc;
            gv_step_t *s = &t->st[cc];
            if (bass) {
                if (psy)
                    s->n = (int8_t)(gv_chance(90) ? 0 : L);
                else if (!k || cc % 8 == 0)
                    s->n = 0;
                else { int b7[7] = {0, 0, 0, 0, L, L, t5}; s->n = (int8_t)b7[gv_rnd(7)]; }
            } else if (arp >= 0) {
                int a = ARPS[arp][k % ARPN[arp]];   /* the arpeggio's chord tones (0 root, 3 third, 5 fifth, 7 octave) */
                s->n = (int8_t)(a == 0 ? 0 : a == 3 ? t3 : a == 5 ? t5 : a == 7 ? L : a == 10 ? L + t3 : 0);
            } else {
                int near, bd = 99, j;
                if (k == last || cc % 4 == 0) {
                    int x = k == last ? d : d + (int)gv_rnd(3) - 1;
                    near = ct[0];
                    for (j = 0; j < 6; j++) if (gv_abs(ct[j] - x) < bd) { bd = gv_abs(ct[j] - x); near = ct[j]; }
                    if (k != last && cc >= 8 && t->st[cc - 8].n >= 0) near = t->st[cc - 8].n;
                    d = near;
                } else if (cc >= 8 && t->st[cc - 8].n >= 0) {
                    d = t->st[cc - 8].n;
                } else {
                    static const int8_t MV[6] = {-2, -1, -1, 1, 1, 2};
                    d = gv_clamp(d + GV_PICK(MV), 0, L + t5);
                }
                s->n = (int8_t)d;
            }
            s->a = (cc % 4 == 0 && gv_chance(40)) ? 1u : 0u;
            {
                uint32_t l4;
                static const uint8_t SH[3] = {2, 3, 4}, LG[4] = {4, 8, 8, 12}, PS[2] = {2, 3};
                if (psy) l4 = GV_PICK(PS);
                else if (arp >= 0) l4 = (uint32_t)al;
                else if (bass) l4 = gv_chance(30) ? (uint32_t)f * 4u : GV_PICK(SH);
                else if (k == last) l4 = 16;
                else l4 = gv_chance(40) ? GV_PICK(LG) : GV_PICK(SH);
                if (l4 > (uint32_t)f * 4u) l4 = (uint32_t)f * 4u;
                s->l4 = gv_l4(l4);
            }
            if (f == 1 && k < last && gv_chance(bass ? 18u : 8u))
                s->s = 1;
        }
    }
}
/* VARIER for a melody: 3 or 4 touches in the scale, the motif stays (one bar of it) */
static void gv_vary_bar(uint32_t i, uint32_t bar)
{
    gv_step_t *p = &gv.t[i].st[(bar * 16u) % GV_MAXS];
    int on[16], n = 0, k, mx = 2 * (int)gv_L();
    for (k = 0; k < 16; k++) if (p[k].n >= 0) on[n++] = k;
    if (!n) return;
    for (k = 0; k < 3 + (int)gv_rnd(2); k++) {
        gv_step_t *s = &p[on[gv_rnd((uint32_t)n)]];
        uint32_t r = gv_rnd(100);
        static const int8_t MV[4] = {-2, -1, 1, 2};
        if (r < 60) s->n = (int8_t)gv_clamp(s->n + GV_PICK(MV), 0, mx);
        else if (r < 80) { int c = (int)(s - p), e; for (e = c + 1; e < 16; e++) if (p[e].n < 0 && !(e & 1)) { p[e].n = s->n; p[e].l4 = 4; break; } }
        else s->a ^= 1u;
    }
}
static void gv_vary_mel(uint32_t i)
{
    uint32_t k, any = 0, n = gv_tlen(i);
    for (k = 0; k < n; k++) any |= gv.t[i].st[k].n >= 0;
    if (!any) { gv_gen_mel(i); return; }
    gv_vary_bar(i, gv_rnd(n / 16u));
}
static uint16_t gv_rs_acc;                       /* (the accents of the row being set: the base's motifs have some) */
static void gv_row_setb(uint32_t row, uint32_t bar, uint16_t bits)   /* one bar of a row (its accents: gv_rs_acc) */
{
    track_t *t = &trk[GV_ROWTRK[row & 7u]];
    uint32_t c, o = (bar * 16u) % NSTEP;
    for (c = 0; c < 16u; c++) {
        step_t *s = &t->step[o + c];
        s->on = (bits >> (15u - c)) & 1u;
        s->acc = (gv_rs_acc >> (15u - c)) & 1u;
        s->slide = s->tie = 0; s->rat = 0; s->cond = 0; s->nt = 0; s->micro = 0;
    }
    t->p[P_SLEN] = (int16_t)gv_rlen();
}
static void gv_rhythm_row_set(uint32_t row, uint16_t bits)           /* the same in every bar */
{
    uint32_t b;
    for (b = 0; b < gv_rlen() / 16u; b++) gv_row_setb(row, b, bits);
}
static uint16_t gv_row_getb(uint32_t row, uint32_t bar)
{
    const track_t *t = &trk[GV_ROWTRK[row & 7u]];
    uint32_t c, o = (bar * 16u) % NSTEP;
    uint16_t b = 0;
    for (c = 0; c < 16u; c++)
        if (t->step[o + c].on) b |= (uint16_t)(1u << (15u - c));
    return b;
}
static uint16_t gv_rhythm_row_get(uint32_t row) { return gv_row_getb(row, 0); }
/* the lengths: longer = what was there, repeated (nothing changes to the ear until something is generated or edited) */
static void gv_set_rlen(uint32_t n)
{
    uint32_t r, c, o = gv_rlen();
    n = gv_lenok(n);
    for (r = 0; r < 8u; r++) {
        track_t *t = &trk[GV_ROWTRK[r]];
        for (c = o; c < n; c++) t->step[c] = t->step[c % o];
        t->p[P_SLEN] = (int16_t)n;
    }
    gv.rlen = (uint8_t)n;
}
static void gv_set_tlen(uint32_t i, uint32_t n)
{
    gv_trk_t *t = &gv.t[i % GV_NT];
    uint32_t c, o = gv_tlen(i);
    n = gv_lenok(n);
    for (c = o; c < n; c++) t->st[c] = t->st[c % o];
    t->len = (uint8_t)n;
}
/* a new rhythm in the style: one of its templates, not the one playing */
static void gv_gen_rhythm(void)
{
    uint32_t st = gv.style % GV_NSTY, n = GV_NTP[st], k, r;
    if (gv_src_rhythm()) { gv.tpl = 0; return; }   /* (the source is the base or the bank) */
    gv.libk = 0;
    do k = gv_rnd(n); while (n > 1u && gv.tpl == k + 1u);
    for (r = 0; r < 8u; r++) gv_rhythm_row_set(r, GV_TPL[st][k][r]);
    if (n > 1u && gv_rlen() > 16u) {               /* 2 bars: A B; 4 bars: A B A C (other templates of the style) */
        uint32_t b, o1 = (k + 1u + gv_rnd(n - 1u)) % n, o2 = n > 2u ? (k + 1u + gv_rnd(n - 1u)) % n : o1;
        for (b = 1; b < gv_rlen() / 16u; b += 2u)
            for (r = 0; r < 8u; r++) gv_row_setb(r, b, GV_TPL[st][b == 1u ? o1 : o2][r]);
    }
    gv.tpl = (uint8_t)(k + 1u);
}
/* one drum row from another template (the style's if it can) */
static void gv_gen_row(uint32_t row)
{
    uint16_t now = gv_rhythm_row_get(row), got = 0;
    uint32_t n = 0, s, k, st = gv.style % GV_NSTY, pass;
    for (pass = 0; pass < 2u && !n; pass++)        /* the style's templates first, then every style's (one kept at random
                                                    * as they pass: no list on the stack) */
        for (s = pass ? 0u : st; s < (pass ? (uint32_t)GV_NSTY : st + 1u); s++)
            for (k = 0; k < GV_NTP[s]; k++) {
                uint16_t v = GV_TPL[s][k][row];
                if (v && v != now && gv_rnd(++n) == 0u) got = v;
            }
    if (n) gv_rhythm_row_set(row, got);
    gv.tpl = 0;
}
static void gv_vary_rhythm(void)
{
    uint32_t k;
    for (k = 0; k < 4u; k++) {
        uint32_t r = 3u + gv_rnd(5), c = gv_rnd(gv_rlen());
        step_t *s = &trk[GV_ROWTRK[r]].step[c];
        s->on ^= 1u;
    }
    gv.tpl = 0;
}
/* a style: its tempo, its scale (notes moved onto the nearest of the new scale), a new rhythm */
static void gv_set_scale(uint32_t sc)
{
    uint32_t i, c, a = gv.scale % GV_NSC, b = sc % GV_NSC, La = GV_SCL[a], Lb = GV_SCL[b];
    if (a == b) return;
    for (i = 0; i < GV_NT; i++)
        for (c = 0; c < GV_MAXS; c++) {
            gv_step_t *s = &gv.t[i].st[c];
            int sem, d, best = 0, bd = 99;
            if (s->n < 0) continue;
            sem = GV_SC[a][s->n % La] + 12 * (s->n / (int)La);
            for (d = 0; d <= 2 * (int)Lb; d++) { int x = gv_abs(GV_SC[b][d % Lb] + 12 * (d / (int)Lb) - sem); if (x < bd) { bd = x; best = d; } }
            s->n = (int8_t)best;
        }
    gv.scale = (uint8_t)b;
    if (gv.prog > GV_NPROG[b]) gv.prog = GV_NPROG[b];
}
static void gv_set_style(uint32_t st)
{
    gv.style = (uint8_t)(st % GV_NSTY);
    song.g[G_BPM] = GV_STBPM[gv.style];
    if (GV_STSC[gv.style] >= 0) gv_set_scale((uint32_t)GV_STSC[gv.style]);
}
static void gv_gen_all(void)                   /* the style's rhythm + a melody on every track (ARP held) */
{
    uint32_t i;
    gv_gen_rhythm();
    for (i = 0; i < GV_NT; i++) gv_gen_mel(i);
    if (gv.style == 8u || gv.style == 9u) {        /* TRAP, DRILL: the kick ducks the basses a lot, the rest a little */
        for (i = 0; i < GV_NT; i++) gv.t[i].sc = GV_TTYPE[i] == GT_BASS ? 100 : 55;
        gv_scauto = 1;
    } else if (gv_scauto) { for (i = 0; i < GV_NT; i++) gv.t[i].sc = 0; gv_scauto = 0; }
}

/* ------------------------------------------------------------------ voices (audio ISR) --- */
static void gv_note_on(uint32_t i, int deg, uint32_t gate, int acc, int slide, uint32_t delay)
{
    gv_voice_t *v = &gvv[i];
    const gv_trk_t *t = &gv.t[i];
    uint32_t k, n = GV_TTYPE[i] == GT_PAD ? 3u : 1u;
    int off[3] = {0, gv_t3(), gv_t5()};
    for (k = 0; k < n; k++) v->pinc[k] = gv_inc_tab[gv_d2m(t, deg + off[k])];
    v->pnote = (uint8_t)gv_d2m(t, deg);
    v->pgate = gate ? gate : 0xFFFFFFFFu;
    v->pacc = (uint8_t)acc; v->pslide = (uint8_t)slide; v->pn = (uint8_t)n;
    v->delay = delay ? delay : 1u;
    gv_hit_t[8u + i] = song.tick + (delay >> 8);
}
static void gv_midi_off(gv_voice_t *v)           /* the note held on the other machine: let go */
{
    if (!v->mon) return;
    midi_out_event(0x08u | (0x80u | v->mchn) << 8 | (uint32_t)v->mnote << 16);
    v->mon = 0;
}
static void gv_note_start(uint32_t i)
{
    gv_voice_t *v = &gvv[i];
    gv_on_ms[i] = fm1_ms;
    const gv_trk_t *t = &gv.t[i];
    uint32_t k, legato = v->slide && v->on && v->gate;
    v->nosc = v->pn;
    for (k = 0; k < v->nosc; k++) {
        v->tinc[k] = v->pinc[k];
        if (!legato && !t->gl) v->inc[k] = v->pinc[k];
        else if (!v->inc[k]) v->inc[k] = v->pinc[k];
    }
    if (GV_TTYPE[i] == GT_FX) { v->inc[0] = v->tinc[0] * 4u > v->tinc[0] ? v->tinc[0] * 4u : v->tinc[0]; v->fxs = 1; }
    v->gate = v->pgate;
    v->acc = v->pacc;
    v->slide = v->pslide;
    v->atgt = (v->acc ? 1 << 24 : (int32_t)(0.72f * (1 << 24))) ;
    if (!legato) v->fenv = 32767;
    v->on = 1;
    if (t->mo && gv_track_heard(8u + i)) {              /* MIDI out: the new note, then the old one off (legato there too) */
        uint8_t was = v->mon, wn = v->mnote, wc = v->mchn;
        v->mchn = (uint8_t)((t->mch ? t->mch - 1u : i) & 15u); v->mnote = v->pnote & 127u; v->mon = 1;
        midi_out_event(0x09u | (0x90u | v->mchn) << 8 | (uint32_t)v->mnote << 16 | (v->acc ? 120u : 96u) << 24);
        if (was) midi_out_event(0x08u | (0x80u | wc) << 8 | (uint32_t)wn << 16);
    } else gv_midi_off(v);
}
static void gv_note_off(uint32_t i) { gvv[i].gate = 0; gvv[i].delay = 0; gv_midi_off(&gvv[i]); }

static void gv_init_tables(void) {}
/* cutoff (0..127 << 8, + envelope and LFO) -> SVF coefficient Q15 ~ 2 pi fc / fs, fc 40 Hz .. ~12 kHz, exponential */
static int32_t gv_fcoef(int32_t c7)
{
    int32_t c = gv_clamp(c7, 0, 127 << 8), o = c * 82 / 1280;   /* octaves above 40 Hz, x256 */
    int32_t fr = (int32_t)(40u << (o >> 8)) * (256 + (o & 255) * 181 / 256) / 256, k;
    k = fr * (int32_t)(205887u * 1024u / GV_FS) >> 10;         /* 2 pi x 32768 / FS (no 64-bit division in the ISR) */
    return k > 30000 ? 30000 : k < 60 ? 60 : k;
}
static int32_t gv_osc(uint32_t ph, uint32_t w)
{
    int32_t x = (int32_t)(ph >> 16) - 32768;       /* saw */
    if (w == GW_SQR) return ph < 0x80000000u ? 26000 : -26000;
    if (w == GW_TRI) return x < 0 ? 2 * x + 32767 : 32767 - 2 * x;
    if (w == GW_PIANO) {                           /* a struck string: the fundamental, its octave, a little third harmonic */
        uint32_t p2 = ph * 2u, p3 = ph * 3u;
        int32_t a = x < 0 ? 2 * x + 32767 : 32767 - 2 * x, x2 = (int32_t)(p2 >> 16) - 32768, x3 = (int32_t)(p3 >> 16) - 32768;
        return (a * 5 + (x2 < 0 ? 2 * x2 + 32767 : 32767 - 2 * x2) * 2 + x3) >> 3;
    }
    return x;
}
static int32_t gv_lfo(gv_voice_t *v, const gv_trk_t *t, uint32_t n)
{
    static const uint8_t DIVQ[5] = {64, 32, 16, 8, 4};     /* 1 bar .. 1/16, in 1/16 steps x4 */
    uint32_t hz256;                                       /* rate, Hz * 256 */
    int32_t ph, y;
    if (t->lfs) hz256 = (uint32_t)song.g[G_BPM] * 256u * 16u / (60u * DIVQ[(t->lfr * 4u + 63u) / 127u > 4u ? 4u : (t->lfr * 4u + 63u) / 127u]);
    else hz256 = 26u + (uint32_t)t->lfr * t->lfr * 5120u / 16129u;   /* 0.1 .. 20 Hz */
    v->lph += hz256 * (16777216u / GV_FS) * n;           /* 2^32 / (FS x 256) a Hz/256, per sample */
    ph = (int32_t)(v->lph >> 16) - 32768;
    switch (t->lfw) {
    case 1: y = ph < 0 ? 2 * ph + 32767 : 32767 - 2 * ph; break;
    case 2: y = ph < 0 ? 32767 : -32767; break;
    case 3: y = ph; break;
    default: { int32_t x = ph, a = x < 0 ? -x : x; y = (int32_t)(((int64_t)x * (65536 - a)) >> 14); y = gv_clamp(y, -32767, 32767); }
    }
    return y;                                            /* -32767..32767 */
}
static volatile uint8_t gv_live_hit;            /* a key's hit (drum_hit from here): never cut */
static int gv_track_heard(uint32_t k);
static int gv_drum_cut(const track_t *t)
{
    uint32_t r;
    if (gv.magic != GV_MAGIC || (!(gv.gmute & 1u) && !gv.solo)) return 0;   /* (P_MUTE: Hortator's own) */
    for (r = 0; r < 8u && &trk[GV_ROWTRK[r]] != t; r++) {}
    if (r == 8u) return 0;
    return (gv.gmute & 1u) || !((gv.solo >> r) & 1u);
}
static int gv_track_heard(uint32_t k)                     /* 0..7 rows, 8..13 tracks: solo / mute / group mute */
{
    if (k >= 8u && (gv.gmute & 2u)) return 0;
    if (k < 8u && (gv.gmute & 1u)) return 0;
    if (gv.solo) return (gv.solo >> k) & 1u;
    return k < 8u ? !trk[GV_ROWTRK[k]].p[P_MUTE] : !gv.t[k - 8u].mute;
}
/* the 6 tracks into the mix (fx.c mix_block: after the drums), sends to the delay and the reverb */
static void gv_render(int32_t *ml, int32_t *mr, int32_t *sc, int32_t *sd, int32_t *sr, uint32_t n)
{
    uint32_t i, j, k, sn = gv_stepn ? gv_stepn : gv_step_len(gv_pos);
    uint32_t trinc = 0x80000000u / (sn ? sn : 1u), trp0 = ((gv_pos & 1u) * sn + gv_acc_s) * trinc;   /* TREMOLO: one turn an 1/8 note */
    int32_t sck0 = gv_sck, aptri, vbtri;
    if (gv.magic != GV_MAGIC) return;
    { uint32_t a = ((gv_pos & 7u) * sn + gv_acc_s) * (0x20000000u / (sn ? sn : 1u));
      aptri = (int32_t)((a < 0x80000000u ? a : ~a) >> 15) - 32768;     /* a triangle, -32768..32767 */
      gv_vbp += n * 584318u; vbtri = (int32_t)((gv_vbp < 0x80000000u ? gv_vbp : ~gv_vbp) >> 15) - 32768; }
    if (gv.dtr | gv.dap | gv.drm | gv.dwf | gv.dsat | gv.dfo) {   /* the drums' effects: the mix holds only them yet */
        int32_t pan = gv_clamp(64 + ((aptri * gv.dap) >> 16), 0, 127), pl = pan > 64 ? (127 - pan) * 2 : 128, pr = pan < 64 ? pan * 2 : 128;
        int32_t tra = gv.dtr * 258, rmk = gv.drm * 256, wfg = 8 + gv.dwf, stg = 32 + gv.dsat;
        uint32_t trp = ((gv_pos & 1u) * sn + gv_acc_s) * trinc, f1k = 0, f2k = 0;
        if (gv.dfo) {
            uint32_t ps8 = ((uint32_t)gv.dfo - 1u) * 4u * 256u / 126u, ix = ps8 >> 8 > 3u ? 3u : ps8 >> 8, fr = ps8 - (ix << 8);
            f1k = (uint32_t)gv_fcoef((int32_t)(GV_F1[ix] * (256u - fr) + GV_F1[ix + 1u] * fr));
            f2k = (uint32_t)gv_fcoef((int32_t)(GV_F2[ix] * (256u - fr) + GV_F2[ix + 1u] * fr));
        }
        for (j = 0; j < n; j++) {
            int32_t x[2], c2;
            x[0] = gv_clamp(ml[j], -200000, 200000); x[1] = gv_clamp(mr[j], -200000, 200000);   /* (a loud drum: about 30000) */
            if (f1k) {                                 /* (the vowel: on the two sides together) */
                int32_t in = gv_clamp((x[0] + x[1]) >> 1, -60000, 60000) >> 3, h;
                gv_df1l += ((int32_t)f1k * gv_df1b) >> 15; h = in - gv_df1l - (gv_df1b >> 3); gv_df1b = gv_clamp(gv_df1b + (((int32_t)f1k * h) >> 15), -150000, 150000);
                gv_df2l += ((int32_t)f2k * gv_df2b) >> 15; h = in - gv_df2l - (gv_df2b >> 3); gv_df2b = gv_clamp(gv_df2b + (((int32_t)f2k * h) >> 15), -150000, 150000);
                gv_df1l = gv_clamp(gv_df1l, -150000, 150000); gv_df2l = gv_clamp(gv_df2l, -150000, 150000);
                x[0] = x[1] = gv_df1b + gv_df2b;
            }
            gv_drph += 14608550u;
            c2 = (int32_t)(gv_drph >> 16) - 32768; c2 = c2 < 0 ? 2 * c2 + 32767 : 32767 - 2 * c2;
            for (k = 0; k < 2u; k++) {
                int32_t y = x[k];
                if (rmk) y += (int32_t)(((int64_t)((int32_t)(((int64_t)y * c2) >> 15) - y) * rmk) >> 15);
                if (gv.dwf) { uint32_t u = (uint32_t)(((y * wfg) >> 3) + 32768) & 0x1FFFFu; y = u < 65536u ? (int32_t)u - 32768 : 98304 - (int32_t)u; }
                if (gv.dsat) {
                    int32_t a;
                    y = (y * stg) >> 5; a = y < 0 ? -y : y;
                    if (a > 12000) a = 12000 + ((a - 12000) >> 1);
                    if (a > 24000) a = 24000 + ((a - 24000) >> 2);
                    if (a > 30000) a = 30000 + ((a - 30000) >> 4);
                    y = ((y < 0 ? -a : a) * (256 - gv.dsat)) >> 8;
                }
                if (tra) { uint32_t tp = trp + j * trinc; y = (int32_t)(((int64_t)y * (32768 - ((int32_t)((tp < 0x80000000u ? tp : ~tp) >> 16) * tra >> 15))) >> 15); }
                x[k] = y;
            }
            ml[j] = (x[0] * pl) >> 7; mr[j] = (x[1] * pr) >> 7;
        }
    }
    if (gv.nr) {                                   /* NOISE RISER: one for the mix; up = louder and brighter, a little reverb */
        int32_t k = 300 + gv.nr * gv.nr * 2, lv = gv.nr * gv.nr;
        for (j = 0; j < n; j++) {
            int32_t z, o;
            gv_nzs = gv_nzs * 1664525u + 1013904223u; z = (int32_t)(gv_nzs >> 16) - 32768;
            gv_nzl += ((z - gv_nzl) * k) >> 15;
            o = (gv_nzl * lv) >> 13;
            ml[j] += o; mr[j] += o; sr[j] += o >> 1;
        }
    }
    for (j = 0; j < n; j++) { gv_sck -= (gv_sck >> 12) + 1; if (gv_sck < 0) gv_sck = 0; }   /* SIDECHAIN: the kick's envelope goes back */
    for (i = 0; i < GV_NT; i++) {
        gv_voice_t *v = &gvv[i];
        const gv_trk_t *t = &gv.t[i];
        int32_t lfo, cbase, q, fk = 1000, vol, dly, rev, drive, cho, gtv;
        uint32_t trl = gv_prl && gv_pon(0) ? gv_prl : t->rl, rper = trl > 1u ? sn / trl : 0u, crk = 1u + t->cr / 16u, crb = t->cr / 11u;
        uint32_t ty = GV_TTYPE[i], tst = (uint32_t)t->ts << 16, trp = trp0, pf, f1k = 0, f2k = 0;
        int32_t pan = t->pan, pl, pr, sca = t->sc * 258, tra = t->tr * 258, sce = sck0, rmk = t->rm * 256, wfg = 8 + t->wf, stg = 32 + t->sat;
        if (!v->on && !v->delay) continue;
        if (t->ap) pan = gv_clamp(pan + ((aptri * t->ap) >> 16), 0, 127);   /* AUTO-PAN: left and right over two beats */
        pl = pan > 64 ? (127 - pan) * 2 : 128; pr = pan < 64 ? pan * 2 : 128;
        pf = GV_SEMI[gv_clamp(t->ps, -12, 12) + 12];   /* PITCH, then VIBRATO (+-50 cents at most): one factor, Q16 */
        if (t->vb) pf = (uint32_t)(((uint64_t)pf * (uint32_t)(65536 + (((vbtri >> 8) * t->vb) >> 3))) >> 16);
        if (t->fo) {                                  /* the vowel: where between A E I O U, the two band-passes there */
            uint32_t ps8 = ((uint32_t)t->fo - 1u) * 4u * 256u / 126u, ix = ps8 >> 8 > 3u ? 3u : ps8 >> 8, fr = ps8 - (ix << 8);
            f1k = (uint32_t)gv_fcoef((int32_t)(GV_F1[ix] * (256u - fr) + GV_F1[ix + 1u] * fr));
            f2k = (uint32_t)gv_fcoef((int32_t)(GV_F2[ix] * (256u - fr) + GV_F2[ix + 1u] * fr));
        }
        lfo = t->lfd ? gv_lfo(v, t, n) : 0;
        q = 32767 - t->res * 240;                     /* damping: less = more resonance */
        vol = gv_track_heard(8u + i) && t->mo != 1u ? t->vol * 200 : 0;   /* (MIDI only: silent here) */   /* x VOICE_FS-ish level */
        dly = t->dly * 258; rev = t->rev * 258;
        drive = t->dist;
        cho = t->cho * 258; gtv = 127 - t->gt;
        if (rper && (!v->rlc || v->rlc > rper)) v->rlc = rper - gv_acc_s % rper;
        for (j = 0; j < n; j++) {
            int32_t x = 0, s, hp;
            if (v->delay && !--v->delay) gv_note_start(i);
            if (!v->on) continue;
            for (k = 0; k < v->nosc; k++) {
                if (v->inc[k] != v->tinc[k]) {               /* glide (or a slide) */
                    int32_t d = (int32_t)(v->tinc[k] - v->inc[k]);
                    uint32_t g = v->fxs ? 900u + t->dec * 20u : (v->slide ? 600u : 40u + (uint32_t)t->gl * 40u);
                    v->inc[k] += (uint32_t)(d / (int32_t)g + (d > 0 ? 1 : d < 0 ? -1 : 0));
                }
                {   /* the step: PITCH and VIBRATO (pf), TAPE STOP (slower) */
                    uint32_t st2 = v->inc[k];
                    if (pf != 65536u) st2 = (uint32_t)(((uint64_t)st2 * pf) >> 16);
                    if (v->tsl) st2 = (uint32_t)(((uint64_t)st2 * ((128u << 16) - v->tsl)) >> 23);
                    v->ph[k] += st2;
                    if (!k && rmk) v->rph += st2 + (st2 >> 1);      /* RING MOD: its carrier a fifth above */
                }
                x += gv_osc(v->ph[k] + (k ? k * 0x1234567u : 0u), t->wave) / (int32_t)(v->nosc == 3u ? 3 : 1);
            }
            if (rmk) {                                   /* RING MOD: the sound times a triangle */
                int32_t c = (int32_t)(v->rph >> 16) - 32768; c = c < 0 ? 2 * c + 32767 : 32767 - 2 * c;
                x += ((((x * c) >> 15) - x) * rmk) >> 15;
            }
            if (t->wf) {                                 /* WAVEFOLDER: louder, and what goes over the top folds back */
                uint32_t u = (uint32_t)(((x * wfg) >> 3) + 32768) & 0x1FFFFu;
                x = u < 65536u ? (int32_t)u - 32768 : 98304 - (int32_t)u;
            }
            if (v->gate) { if (v->gate != 0xFFFFFFFFu) v->gate--; }
            else { v->atgt = 0; if (v->mon) gv_midi_off(v); }
            {   /* amplitude: attack to the target, release when the gate closes */
                int32_t r = v->atgt > v->amp ? (ty == GT_PAD ? 9 + t->att / 12 : 4 + t->att / 14) : (ty == GT_PAD ? 12 : 9);
                v->amp += (v->atgt - v->amp) >> r;
            }
            v->fenv -= v->fenv >> (7 + t->dec / 18);
            if ((j & 15u) == 0u) {
                if (t->wave == GW_PIANO) v->atgt -= v->atgt >> (9 + t->dec / 40);   /* PIANO: the held note dies away */
                cbase = (t->cut << 8) + ((v->fenv * t->env * (v->acc ? 3 : 2)) >> 15) * 64 + ((lfo * t->lfd) >> 15) * 64;
                fk = gv_fcoef(cbase);
            }
            /* Chamberlin SVF */
            v->lp += (fk * v->bp) >> 15;
            hp = x - v->lp - ((q * v->bp) >> 15);
            v->bp += (fk * hp) >> 15;
            v->lp = gv_clamp(v->lp, -200000, 200000); v->bp = gv_clamp(v->bp, -200000, 200000);
            s = t->ftype == GF_HP ? hp : t->ftype == GF_BP ? v->bp : v->lp;
            if (drive) { s = s * (1 + drive / 16); s = s > 30000 ? 30000 + (s - 30000) / 8 : s < -30000 ? -30000 + (s + 30000) / 8 : s; }
            if (t->sat) {                                /* SATURATION: a soft knee in three slopes, then back down */
                int32_t a;
                s = (s * stg) >> 5; a = s < 0 ? -s : s;
                if (a > 12000) a = 12000 + ((a - 12000) >> 1);
                if (a > 24000) a = 24000 + ((a - 24000) >> 2);
                if (a > 30000) a = 30000 + ((a - 30000) >> 4);
                s = ((s < 0 ? -a : a) * (256 - t->sat)) >> 8;
            }
            if (f1k) {                                   /* the vowel: two band-passes side by side */
                int32_t in = gv_clamp(s, -60000, 60000) >> 3, h;   /* (/8: the band-passes ring 8 times louder) */
                v->f1l += ((int32_t)f1k * v->f1b) >> 15; h = in - v->f1l - (v->f1b >> 3); v->f1b += ((int32_t)f1k * h) >> 15;
                v->f2l += ((int32_t)f2k * v->f2b) >> 15; h = in - v->f2l - (v->f2b >> 3); v->f2b += ((int32_t)f2k * h) >> 15;
                v->f1b = gv_clamp(v->f1b, -150000, 150000); v->f2b = gv_clamp(v->f2b, -150000, 150000);
                v->f1l = gv_clamp(v->f1l, -150000, 150000); v->f2l = gv_clamp(v->f2l, -150000, 150000);
                s = v->f1b + v->f2b;
            }
            s = (int32_t)(((int64_t)s * (v->amp >> 9)) >> 15);   /* amp Q24 -> Q15 */
            if (t->cr) {                                 /* BITCRUSH: fewer bits, a held sample */
                if (!v->crh--) { v->crh = crk - 1u; v->crs = (s >> crb) << crb; }
                s = v->crs;
            }
            if (t->gt && (gv_acc_s + j) * 2u > sn) s = (s * gtv) >> 7;   /* GATE: the step's second half down */
            if (rper && !--v->rlc) { v->rlc = rper; if (v->gate) { v->amp >>= 3; v->fenv = 32767; } }   /* ROLL: hit again */
            s = (s * vol) >> 12;
            if (v->tsl != tst) {                         /* TAPE STOP: the speed glides to where the knob is (~0.5 s) */
                int32_t d = (int32_t)tst - (int32_t)v->tsl;
                v->tsl = d > -4096 && d < 4096 ? tst : (uint32_t)((int32_t)v->tsl + (d >> 12));
            }
            if (v->tsl >= (96u << 16)) s = (s * (int32_t)(127u - (v->tsl >> 16))) >> 5;   /* the last quarter: it fades too */
            if (sca | tra) {                             /* SIDECHAIN and TREMOLO: one gain, Q15 */
                int32_t g = 32768;
                if (sca) { g -= (sce * sca) >> 15; sce -= (sce >> 12) + 1; if (sce < 0) sce = 0; }
                if (tra) { uint32_t tp = trp + j * trinc; g = (g * (32768 - ((int32_t)((tp < 0x80000000u ? tp : ~tp) >> 16) * tra >> 15))) >> 15; }
                s = (int32_t)(((int64_t)s * g) >> 15);
            }
            ml[j] += (s * pl) >> 7; mr[j] += (s * pr) >> 7;
            if (i == gv_scope_trk && !(gv_scdec++ & 7u)) gv_scope[gv_scw++ & 127u] = (int16_t)gv_clamp(s >> 3, -32767, 32767);   /* 1 in 8: ~23 ms */
            if (cho) sc[j] += (s * cho) >> 15;
            if (dly) sd[j] += (s * dly) >> 15;
            if (rev) sr[j] += (s * rev) >> 15;
            if (!v->gate && v->amp < 64) { v->on = 0; v->amp = 0; v->fxs = 0; }
        }
    }
}

/* the whole mix (fx.c, after every track): disto, the DJ filter (low-pass then high-pass, with resonance), then the
 * echo and reverb sends of everything (the web app's master effects). Integer SVFs, one per side. */
static int32_t gvm_lp[2], gvm_bp[2], gvm_hl[2], gvm_hb[2], gvm_e1[2], gvm_e2[2];
static void gv_master(int32_t *ml, int32_t *mr, int32_t *sd, int32_t *sr, uint32_t n)   /* before the buses: disto, sends */
{
    uint32_t j, c;
    int32_t xs, xd, xr;
    if (gv.magic != GV_MAGIC || (!gv.xs && !gv.xd && !gv.xr)) return;
    xs = gv.xs; xd = gv.xd * 200; xr = gv.xr * 200;
    for (c = 0; c < 2u; c++) {
        int32_t *m = c ? mr : ml;
        for (j = 0; j < n; j++) {
            int32_t x = m[j] >> 6;                     /* the mix runs ~2^21: down to a voice's range */
            if (xs) { x = x * (1 + xs / 12); x = x > 24000 ? 24000 + (x - 24000) / 6 : x < -24000 ? -24000 + (x + 24000) / 6 : x; }
            x = gv_clamp(x, -65000, 65000);
            m[j] = x << 6;
            if (xd) sd[j] += (x * xd) >> 11;
            if (xr) sr[j] += (x * xr) >> 11;
        }
    }
}
/* after the buses (fx.c): the echo and the reverb join the mix, then the DJ filter and the 3-band equaliser on all of it */
static void gv_master_post(int32_t *ml, int32_t *mr, int32_t *wet, uint32_t n)
{
    uint32_t j, c;
    int32_t fl, fh, q;
    int eqon = gv.eq[0] != 64u || gv.eq[1] != 64u || gv.eq[2] != 64u;
    if (gv.magic != GV_MAGIC || (gv.mf >= 127u && !gv.hp && !eqon)) return;
    fl = gv.mf >= 127u ? 0 : gv_fcoef((int32_t)gv.mf << 8);
    fh = gv.hp ? gv_fcoef((int32_t)gv.hp << 8) : 0;
    q = 32767 - gv.mq * 230;
    for (c = 0; c < 2u; c++) {
        int32_t *m = c ? mr : ml;
        for (j = 0; j < n; j++) {
            int32_t x = (m[j] + wet[j]) >> 6, hp;
            if (fl) {
                gvm_lp[c] += (fl * gvm_bp[c]) >> 15;
                hp = x - gvm_lp[c] - ((q * gvm_bp[c]) >> 15);
                gvm_bp[c] += (fl * hp) >> 15;
                gvm_lp[c] = gv_clamp(gvm_lp[c], -65000, 65000); gvm_bp[c] = gv_clamp(gvm_bp[c], -65000, 65000);
                x = gvm_lp[c];
            }
            if (fh) {
                gvm_hl[c] += (fh * gvm_hb[c]) >> 15;
                hp = x - gvm_hl[c] - ((q * gvm_hb[c]) >> 15);
                gvm_hb[c] += (fh * hp) >> 15;
                gvm_hl[c] = gv_clamp(gvm_hl[c], -65000, 65000); gvm_hb[c] = gv_clamp(gvm_hb[c], -65000, 65000);
                x = hp;
            }
            if (eqon) {                                /* 3 bands: under ~250 Hz, over ~3 kHz, what is between */
                int32_t lo, hi, mid;
                gvm_e1[c] += (1167 * (x - gvm_e1[c])) >> 15;
                gvm_e2[c] += (11400 * (x - gvm_e2[c])) >> 15;
                lo = gvm_e1[c]; hi = x - gvm_e2[c]; mid = x - lo - hi;
                x = (lo * gv.eq[0] + mid * gv.eq[1] + hi * gv.eq[2]) >> 6;
            }
            m[j] = gv_clamp(x, -70000, 70000) << 6;       /* not 65000: -65000 << 6 looks like a chip ROM address to the build check */
        }
    }
    for (j = 0; j < n; j++) wet[j] = 0;               /* (it is in the mix now) */
}

/* ------------------------------------------------------------------ sequencer (audio ISR) --- */
/* the step playing, of the rhythm's and of a melodic track's own length */
static uint32_t gv_rstep(void) { return ((uint32_t)(gv_bar ? gv_bar - 1u : 0u) * 16u + (gv_pos & 15u)) % gv_rlen(); }
static uint32_t gv_tstep(uint32_t i) { return ((uint32_t)(gv_bar ? gv_bar - 1u : 0u) * 16u + (gv_pos & 15u)) % gv_tlen(i); }
static void gv_start(void) { gv_pos = 0; gv_bar = 0; gv_cix = 0; gv_acc_s = 0; gv_stepn = 0; }
static void gv_stop(void) { uint32_t i; for (i = 0; i < GV_NT; i++) gv_note_off(i); }
static uint32_t gv_step_len(uint32_t pos)
{
    uint32_t bpm = (uint32_t)gv_clamp(song.g[G_BPM], 40, 240), base = GV_FS * 60u / (bpm * 4u);
    uint32_t sw = (uint32_t)gv_clamp(song.g[G_SWING], 0, 100) * base / 400u;   /* swing: the odd steps late */
    return pos & 1u ? base - sw : base + sw;
}
static uint8_t gv_lpa[14] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, gv_lpc[14];
static uint32_t gv_loop(uint8_t *a, uint8_t *k, uint32_t lp, uint32_t c)   /* LOOP: the lp steps from where it began */
{
    if (!lp) { *a = 0xFF; return c; }
    if (*a == 0xFF) { *a = (uint8_t)c; *k = 0; }
    return (*a + (*k)++ % lp) & 15u;
}
static void gv_play_step(uint32_t pos)
{
    uint32_t i, sl = gv_step_len(pos);
    int co = gv_choff();
    if (pos == 0u) {
        gv_bar++;
        gv_cix = (uint8_t)((gv_bar - 1u) & 3u);
        if (gv.autob && gv_bar > 1u && (gv_bar - 1u) % gv.autob == 0u) gv_regen_due |= 1u;
        for (i = 0; i < GV_NT; i++) if (gv.t[i].ar && gv_bar > 1u && (gv_bar - 1u) % gv.t[i].ar == 0u) gv_regen_due |= 2u << i;
        for (i = 0; i < 8u; i++) if (gv.rowar[i] && gv_bar > 1u && (gv_bar - 1u) % gv.rowar[i] == 0u) gv_regen_due |= 0x100u << i;
        co = gv_choff();
    }
    for (i = 0; i < 8u; i++) if (trk[GV_ROWTRK[i]].step[gv_rstep()].on) gv_hit_t[i] = song.tick;
    if (trk[GV_ROWTRK[0]].step[gv_rstep()].on && gv_track_heard(0)) gv_sck = 32767;   /* the kick: the sidechain ducks */
    for (i = 0; i < GV_NT; i++) {
        const gv_trk_t *t = &gv.t[i];
        uint32_t c;
        const gv_step_t *s;
        if (t->hf && (pos & 1u)) continue;            /* HALF: one step in two, the pattern over 2 bars */
        c = t->hf ? ((pos >> 1) + ((gv_bar & 1u) ? 0u : 8u)) & 15u : pos;
        if (t->bw || t->rv || (gv_prv && gv_pon(0))) c = 15u - c;
        c = gv_loop(&gv_lpa[8u + i], &gv_lpc[8u + i], gv_plp && gv_pon(0) ? gv_plp : t->lp, c);
        s = &t->st[(((uint32_t)(gv_bar ? gv_bar - 1u : 0u) >> (t->hf ? 1u : 0u)) * 16u + (c & 15u)) % gv_tlen(i)];   /* (the bar playing, of its 1, 2 or 4) */
        if (s->n >= 0 && t->sk && gv_rnd(127u) < t->sk) continue;
        if (gv_live_trk == 8u + i) continue;           /* a key held: the live note wins */
        if (s->n < 0) {
            continue;
        }
        gv_note_on(i, s->n + co, (uint32_t)s->l4 * sl / 4u - (s->s ? 0u : sl / 16u), s->a, s->s, 1u + (uint32_t)s->o4 * sl / 4u);
    }
}
/* FX LIVE on the drum rows: seq.c, a step coming up (idx, its index) -> the one to play; 0 = none */
static int gv_drum_step(const track_t *t, uint16_t *idx, uint32_t cnt)
{
    uint32_t r, i = *idx;
    for (r = 0; r < 8u && &trk[GV_ROWTRK[r]] != t; r++) {}
    if (r == 8u) return 1;
    if (gv.magic == GV_MAGIC && !gv_track_heard(r)) return 0;   /* RYTHME cut, a solo elsewhere */
    {   /* (the effects turn inside the bar playing: i = its 16 steps, bb = where the bar begins) */
        uint32_t bb = i & ~15u;
        i &= 15u;
        if (gv.rhf[r]) { if (i & 1u) return 0; i = ((i >> 1) + (((cnt >> 4) & 1u) ? 8u : 0u)) & 15u; }
        if (gv.rrv[r] || (gv_prv && gv_pon(1))) i = 15u - (i & 15u);
        i = gv_loop(&gv_lpa[r], &gv_lpc[r], gv_plp && gv_pon(1) ? gv_plp : gv.rlp[r], i);
        if (gv.rsk[r] && gv_rnd(127u) < gv.rsk[r]) return 0;
        i = bb + (i & 15u);
    }
    *idx = (uint16_t)i;
    return 1;
}
static void gv_drum_roll(track_t *t)              /* after its hit: ROLL = 2..4 hits in the step */
{
    uint32_t r;
    for (r = 0; r < 8u && &trk[GV_ROWTRK[r]] != t; r++) {}
    uint8_t rl = r < 8u ? (gv_prl && gv_pon(1) ? gv_prl : gv.rrl[r]) : 0u;
    if (rl > 1u && t->rat_n) { t->rat_n = rl; t->rat_k = 1; }
}
/* the block's advance (seq.c events_block: n samples of the drums' clock) */
static void gv_tick(uint32_t n)
{
    if (gv.magic != GV_MAGIC) return;              /* not set up yet (gv_boot): its tracks would play degree 0 */
    if (!song.playing) { gv_acc_s = 0; gv_stepn = 0; return; }
    while (n) {
        uint32_t left;
        if (!gv_stepn) { gv_stepn = gv_step_len(gv_pos); gv_play_step(gv_pos); }
        left = gv_stepn - gv_acc_s;
        if (n < left) { gv_acc_s += n; return; }
        n -= left; gv_acc_s = 0; gv_stepn = 0;
        gv_pos = (uint8_t)((gv_pos + 1u) & 15u);
    }
}
/* the keys (seq.c keyboard_block, ISR) while Groove owns the panel: a black key plays the shown track; the white keys
 * are the UI's (groove_ui.c reads their edges in the main loop) */
static const uint8_t GV_BLACK[11] = {1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25};
static const uint8_t GV_WKEY[16] = {0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26};   /* = STEP_KEY (seq.c) */
static uint32_t gv_kp_held;
static void gv_rec_note(uint32_t i, int d)       /* REC: the note into the step playing (its degree, the chord taken off) */
{
    gv_step_t *s = &gv.t[i].st[gv_tstep(i)];
    if (!gv_recq || !song.playing) return;
    s->n = (int8_t)gv_clamp(d, 0, 2 * (int)gv_L()); s->l4 = 4; s->o4 = 0; s->a = 0; s->s = 0;
}
static void gv_play_key(uint32_t key, int down)  /* JEU: key 0..26 from the left = the scale going up */
{
    uint32_t i;
    if (gv.sel < 8) {
        uint32_t w;
        for (w = 0; w < 16u && GV_WKEY[w] != key; w++) {}
        if (!down) return;
        if (w < 16u || gv_kp == 2u) { uint32_t r = gv_kp == 2u ? (uint32_t)gv.sel & 7u : w & 7u; { gv_live_hit = 1; drum_hit(&trk[GV_ROWTRK[r]], w >= 8u ? 127 : 100); gv_live_hit = 0; } gv_hit_t[r] = song.tick;
            if (gv_recq && song.playing) trk[GV_ROWTRK[r]].step[gv_rstep()].on = 1; }
        return;
    }
    i = (uint32_t)gv.sel - 8u;
    if (down) {
        int d = (int)key + (int)gv.oct * (int)gv_L();
        gv_kp_held |= 1u << key;
        gv_live_trk = (uint8_t)(8u + i);
        gv_note_on(i, d + gv_choff(), 0, 0, gv_kp_held & ~(1u << key) ? 1 : 0, 1u);
        gv_rec_note(i, d);
    } else {
        gv_kp_held &= ~(1u << key);
        if (!gv_kp_held) { gv_note_off(i); gv_live_trk = 0xFF; }
    }
}
static void gv_keys_isr(uint32_t cur, uint32_t ch)
{
    uint32_t k, req = gv_hit_req;
    gv_hit_req = 0;
    for (k = 0; k < 8u; k++)
        if ((req >> k) & 1u) { track_t *t = &trk[GV_ROWTRK[k]]; { gv_live_hit = 1; drum_hit(t, 110); gv_live_hit = 0; } gv_hit_t[k] = song.tick; }
    if (gv_kp) {
        for (k = 0; k < 27u; k++) if ((ch >> k) & 1u) gv_play_key(k, (int)((cur >> k) & 1u));
        return;
    }
    for (k = 0; k < 11u; k++) {
        uint32_t bit = 1u << GV_BLACK[k];
        if (!(ch & bit)) continue;
        if (gv.sel < 8) {
            if ((cur & bit) && k < 8u) { { gv_live_hit = 1; drum_hit(&trk[GV_ROWTRK[k]], 110); gv_live_hit = 0; } gv_hit_t[k] = song.tick; gv.sel = (int8_t)k;
                if (gv_recq && song.playing) trk[GV_ROWTRK[k]].step[gv_rstep()].on = 1; }
        } else {
            uint32_t i = (uint32_t)gv.sel - 8u;
            if (cur & bit) {
                int d = (int)k + (int)gv.oct * (int)gv_L();
                gv_live_trk = (uint8_t)(8u + i);
                gv_live_deg = (int16_t)d;
                gv_note_on(i, d + gv_choff(), 0, 0, 0, 1u);
                gv_rec_note(i, d);
            } else if (gv_live_trk == 8u + i && !(cur & 0x2A52D2Au)) {   /* the last black key let go */
                gv_note_off(i);
                gv_live_trk = 0xFF;
            }
        }
    }
}
