/* SPDX-License-Identifier: GPL-3.0-only
 * Groove (FM-1 Groove Studio) on Hortator: the panel and the screen, the web app's way.
 * Same rules everywhere: PRESETS picks the track (8 drum rows + 6 melodic tracks), the 11 black keys play it, the 16
 * white keys change with the page (their LEDs show what they do), KNOB 1-4 = the 4 settings at the bottom of the
 * screen, SELECT moves on the page. A button pressed again shows its next page. Held 2 s: HOME = a new pattern for
 * the track, ARP = the style's rhythm + every melody, FX = the page's effect back to zero.
 * ALGORITHM on the LIVE pages: right = JEU (every key plays the track), left = the mutes.
 * GLO page, white key 16: Hortator's own pages (back to Groove: hold ENV and press ARP). */
static void play_press(void);
static void gv_boot(void);

enum { GP_LIVE, GP_LRYT, GP_LMEL, GP_LFX, GP_RYT, GP_MEL, GP_STY, GP_VIS, GP_GRYT, GP_GMEL, GP_FXR, GP_FXA, GP_FXB, GP_FXD, GP_FXP,
       GP_SCL, GP_ENV, GP_FLT, GP_LFO, GP_OSC, GP_MIX, GP_LRR, GP_SON, GP_GLO, GP_SAVE, GP_N };
static const char *const GP_T[GP_N] = {"LIVE", "LIVE RYT", "LIVE MEL", "FX LIVE", "RYTHME", "MELODIE", "STYLES", "VISU", "GEN RYT",
                                       "GEN MEL", "FX RECAP", "FX FILTRE", "FX ESPACE", "FX BATT", "FX PERF", "GAMME", "ENV", "FILTRE",
                                       "LFO", "OSC", "MIX", "LOOP ROLL", "SON", "TEMPO", "BANQUE"};
#define GV_PERFP(p) ((p) >= GP_FXA && (p) <= GP_FXP)   /* the FX pages with the pads (LOOP ROLL REVERSE held) */
/* button -> its pages (B_* of panel.c); 0xFF = none */
static const uint8_t GV_GRP[NB][5] = {
    [B_HOME] = {GP_LIVE, GP_LRYT, GP_LMEL, GP_LFX, 0xFF}, [B_SEQ] = {GP_RYT, GP_MEL, 0xFF, 0xFF, 0xFF},
    [B_ARP] = {GP_STY, GP_VIS, GP_GRYT, GP_GMEL, 0xFF},  [B_FX] = {GP_FXR, GP_FXA, GP_FXB, GP_FXD, GP_FXP},
    [B_SCL] = {GP_SCL, 0xFF, 0xFF, 0xFF, 0xFF},          [B_ENV] = {GP_ENV, GP_FLT, GP_LFO, GP_OSC, 0xFF},
    [B_LFO] = {GP_MIX, GP_LRR, 0xFF, 0xFF, 0xFF},        [B_EDIT] = {GP_SON, 0xFF, 0xFF, 0xFF, 0xFF},
    [B_GLO] = {GP_GLO, 0xFF, 0xFF, 0xFF, 0xFF},          [B_SAVE] = {GP_SAVE, 0xFF, 0xFF, 0xFF, 0xFF},
    [B_PLAY] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, [B_REC] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    [B_OCTDN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, [B_OCTUP] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

static struct {
    uint8_t page, btn, cur, sty, kp, dirty, init;
    uint8_t hold_b;                 /* the button held for a long press (NB none) */
    uint32_t hold_t0, last_draw, flash, toast_t;
    uint8_t hold_done;
    char toast[32];
    int8_t deg;                     /* the last degree played (a new melody step takes it) */
    uint8_t fxt, fxpg;              /* FX LIVE: the target (0 rhythm, 1 all the melody, 2..7 a track), the page A..D */
    int8_t fxd;
    uint32_t fxa;
    int16_t byp[8][16];             /* FX LIVE: the value an effect had before its key cut it (-1 = not cut) */
    uint8_t lock;                   /* FX pads: VERROU (a pad stays on when let go) */
    uint32_t tap[4];                /* TAP TEMPO: the last taps */
    uint32_t wprev;                 /* the white keys held (their release: the pads) */
} gu = {.hold_b = NB};

/* ------------------------------------------------------------- colours (the web app's palette, RGB565) --- */
#define GC_BG RGB(0x10, 0x12, 0x15)
#define GC_P RGB(0x1b, 0x1e, 0x22)
#define GC_P2 RGB(0x26, 0x2a, 0x30)
#define GC_L RGB(0x3a, 0x40, 0x48)
#define GC_T RGB(0xee, 0xf1, 0xf5)
#define GC_M RGB(0x93, 0x9b, 0xa5)
#define GC_A RGB(0xff, 0x8a, 0x1f)
#define GC_Y RGB(0xff, 0xd2, 0x3f)
#define GC_C RGB(0x35, 0xe3, 0xc4)
#define GC_R RGB(0xff, 0x46, 0x46)
static const uint16_t GV_ROWC[8] = {RGB(0xff, 0x5a, 0x5a), RGB(0xff, 0x8a, 0x1f), RGB(0xff, 0x6f, 0xb5), RGB(0xff, 0xd2, 0x3f),
                                    RGB(0xc6, 0xf4, 0x32), RGB(0x3d, 0xdc, 0x84), RGB(0xff, 0x9e, 0x6b), RGB(0x3d, 0xdc, 0x84)};
static const uint16_t GV_TRC[GV_NT] = {RGB(0x4f, 0x8c, 0xff), RGB(0x35, 0xe3, 0xc4), RGB(0x4f, 0x8c, 0xff), RGB(0x35, 0xe3, 0xc4),
                                       RGB(0xa7, 0x8b, 0xfa), RGB(0xe6, 0x6b, 0xff)};
static uint16_t gv_col(int p) { return p < 8 ? GV_ROWC[p & 7] : GV_TRC[(p - 8) % GV_NT]; }
static const char *gv_name(int p) { return p < 8 ? GV_ROWN[p & 7] : GV_TRN[(p - 8) % GV_NT]; }
static int gv_isd(void) { return gv.sel < 8; }
static gv_trk_t *gv_tr(void) { return &gv.t[gv_isd() ? 0 : gv.sel - 8]; }
static const char *const GV_NOTE[12] = {"DO", "DO#", "RE", "RE#", "MI", "FA", "FA#", "SOL", "SOL#", "LA", "LA#", "SI"};
static const char *gv_degname(int d)
{
    int L = (int)gv_L(), k = ((d % L) + L) % L;
    return GV_NOTE[(gv.root + GV_SC[gv.scale % GV_NSC][k]) % 12u];
}
static void gv_say(const char *s)
{
    uint32_t i;
    for (i = 0; i < sizeof gu.toast - 1u && s[i]; i++) gu.toast[i] = s[i];
    gu.toast[i] = 0;
    gu.toast_t = fm1_ms + 1500u;
    gu.dirty = 1;
}

/* ------------------------------------------------------------- the knobs: id -> label, range, value --- */
enum { KN_NONE, KN_TVOL, KN_RVOL, KN_SWING, KN_TAR, KN_RAR, KN_STYLE, KN_GMR, KN_PITCH, KN_BPM, KN_AUTO, KN_PROG, KN_ROOT,
       KN_SCALE, KN_CUT, KN_RES, KN_FTYPE, KN_ENV, KN_ATT, KN_DEC, KN_GL, KN_LFR, KN_LFD, KN_LFW, KN_LFS, KN_WAVE, KN_OCT,
       KN_DIST, KN_DLY, KN_REV, KN_DMIX, KN_DTIME, KN_RSIZE, KN_RDAMP, KN_SNOTE, KN_SLEN, KN_SACC, KN_SOFF, KN_ROW,
       KN_MODEL, KN_RTUNE, KN_RDEC, KN_RDIST, KN_RREV, KN_FX0, KN_FX1, KN_FX2, KN_FX3,
       KN_DJ, KN_MF, KN_HP, KN_MQ, KN_XS, KN_XD, KN_XR, KN_PTG, KN_LOCK, KN_RLP, KN_RRL, KN_TLP, KN_TRL, KN_TBW,
       KN_DVOL, KN_DDIST, KN_DDLY, KN_DREV, KN_N };
static const char *const KN_L[KN_N] = {"", "VOLUME", "VOLUME", "SWING", "AUTO", "AUTO", "STYLE", "RYTHME", "PITCH", "BPM",
    "AUTO RYT", "ACCORDS", "TONIQUE", "GAMME", "CUTOFF", "RESO", "TYPE", "ENV", "ATTAQUE", "DECAY", "GLIDE", "VITESSE",
    "PROFOND.", "FORME", "CALAGE", "ONDE", "OCTAVE", "DISTO", "DELAY", "REVERB", "DLY MIX", "DLY TEMPS", "REV TAILLE",
    "REV DAMP", "NOTE", "DUREE", "ACC SLIDE", "DECALAGE", "LIGNE", "MODELE", "ACCORD", "DECAY", "DISTO", "REVERB", "", "", "", "",
    "FILTRE DJ", "COUPE-HAUT", "COUPE-BAS", "RESO", "DISTO", "ECHO", "REVERB", "CIBLE", "VERROU", "LOOP", "ROLL", "LOOP", "ROLL",
    "SENS", "VOLUME", "DISTO", "DELAY", "REVERB"};
static gv_step_t *gv_stp(void) { return &gv_tr()->st[gu.cur & 15u]; }
static track_t *gv_rt(void) { return &trk[GV_ROWTRK[gv.sel & 7]]; }
/* ------------------------------------------------------------- FX LIVE: 16 effects on 4 pages, on a target --- */
enum { FE_CUT, FE_RES, FE_DIST, FE_DLY, FE_REV, FE_LFD, FE_LFR, FE_VOL, FE_CHO, FE_RL, FE_SK, FE_RV, FE_LP, FE_CR, FE_GT, FE_HF, FE_N };
static const char *const FE_L[FE_N] = {"FILTRE", "RESO", "DISTO", "ECHO", "REVERB", "WOBBLE", "VITESSE", "VOLUME",
                                       "CHORUS", "ROLL", "PAS SAUTES", "REVERSE", "LOOP", "BITCRUSH", "GATE", "DEMI-TEMPO"};
static const char *const FE_LD[FE_N] = {0, 0, 0, 0, 0, "ENV FILT", "DEC FILT", 0, 0, 0, 0, 0, 0, 0, 0, 0};   /* the drums' */
static const int16_t FE_OFF[FE_N] = {127, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0};   /* cut = this; -1 no cut */
static const uint8_t FE_RLO[4] = {0, 2, 3, 4}, FE_LPO[5] = {0, 1, 2, 4, 8};
static const char *const FXT_N[8] = {"RYTHME", "MELODIE", "BASSE", "BASSE 2", "SYNTHE", "SYNTHE 2", "NAPPE", "FX"};
static const uint8_t FXT_TRK[8] = {0, 0, 0, 2, 1, 3, 4, 5};   /* targets 2..7 -> gv.t[] */
static const char *const FXPN[4] = {"A", "B", "C", "D"};
static const int8_t GV_QSIN[65] = {0,3,6,9,12,16,19,22,25,28,31,34,37,40,43,46,49,51,54,57,60,63,65,68,71,73,76,78,81,83,85,88,90,92,94,96,98,100,102,104,106,107,109,111,112,113,115,116,117,118,120,121,122,122,123,124,125,125,126,126,126,127,127,127,127};
static int32_t gv_sin(uint32_t a)                     /* a 0..255 = one turn -> -127..127 */
{
    a &= 255u;
    return a < 64u ? GV_QSIN[a] : a < 128u ? GV_QSIN[128u - a] : a < 192u ? -GV_QSIN[a - 128u] : -GV_QSIN[256u - a];
}
static uint16_t gv_fxtc(uint32_t j) { return j == 0u ? GC_C : j == 1u ? GC_A : GV_TRC[FXT_TRK[j]]; }
static uint8_t *gv_tfld(gv_trk_t *t, uint32_t e)
{
    switch (e) {
    case FE_CUT: return &t->cut; case FE_RES: return &t->res; case FE_DIST: return &t->dist; case FE_DLY: return &t->dly;
    case FE_REV: return &t->rev; case FE_LFD: return &t->lfd; case FE_LFR: return &t->lfr; case FE_VOL: return &t->vol;
    case FE_CHO: return &t->cho; case FE_RL: return &t->rl; case FE_SK: return &t->sk; case FE_RV: return &t->rv;
    case FE_LP: return &t->lp; case FE_CR: return &t->cr; case FE_GT: return &t->gt; default: return &t->hf;
    }
}
static int32_t gv_dget(uint32_t r, uint32_t e)       /* a drum row's value of an effect, 0..127 (or its choice) */
{
    const track_t *t = &trk[GV_ROWTRK[r]];
    switch (e) {
    case FE_CUT: return t->p[P_FTYPE] == FT_LP ? t->p[P_FCUT] : 127; case FE_RES: return t->p[P_FRESO];
    case FE_DIST: return t->p[P_DIST]; case FE_DLY: return t->p[P_DLY]; case FE_REV: return t->p[P_REV];
    case FE_LFD: return t->p[P_FENV] > 0 ? t->p[P_FENV] * 2 : 0; case FE_LFR: return t->p[P_FDEC];
    case FE_VOL: return t->p[P_LEVEL]; case FE_CHO: return t->p[P_CHOR];
    case FE_CR: return t->p[P_ITYPE] == IT_CRUSH ? 127 - t->p[P_IA] : 0;
    case FE_GT: return t->p[P_SLCR] == SL_GATE ? t->p[P_SLDEPTH] : 0;
    case FE_RL: return gv.rrl[r]; case FE_SK: return gv.rsk[r]; case FE_RV: return gv.rrv[r];
    case FE_LP: return gv.rlp[r]; default: return gv.rhf[r];
    }
}
static void gv_dset(uint32_t r, uint32_t e, int32_t v)
{
    track_t *t = &trk[GV_ROWTRK[r]];
    switch (e) {
    case FE_CUT: t->p[P_FTYPE] = (int16_t)(v >= 127 ? FT_OFF : FT_LP); t->p[P_FCUT] = (int16_t)v; break;
    case FE_RES: t->p[P_FRESO] = (int16_t)v; if (t->p[P_FTYPE] == FT_OFF && v) t->p[P_FTYPE] = FT_LP; break;
    case FE_DIST: t->p[P_DIST] = (int16_t)v; break; case FE_DLY: t->p[P_DLY] = (int16_t)v; break;
    case FE_REV: t->p[P_REV] = (int16_t)v; break;
    case FE_LFD: t->p[P_FENV] = (int16_t)(v / 2); if (t->p[P_FTYPE] == FT_OFF && v) { t->p[P_FTYPE] = FT_LP; t->p[P_FCUT] = 40; } break;
    case FE_LFR: t->p[P_FDEC] = (int16_t)v; break; case FE_VOL: t->p[P_LEVEL] = (int16_t)v; break;
    case FE_CHO: t->p[P_CHOR] = (int16_t)v; break;
    case FE_CR: t->p[P_ITYPE] = (int16_t)(v ? IT_CRUSH : IT_OFF); t->p[P_IA] = (int16_t)(127 - v); t->p[P_IB] = (int16_t)(127 - v / 2); t->p[P_IMIX] = 127; break;
    case FE_GT: t->p[P_SLCR] = (int16_t)(v ? SL_GATE : SL_OFF); t->p[P_SLPAT] = 1; t->p[P_SLRATE] = 2; t->p[P_SLDEPTH] = (int16_t)v; break;
    case FE_RL: gv.rrl[r] = (uint8_t)v; break; case FE_SK: gv.rsk[r] = (uint8_t)v; break; case FE_RV: gv.rrv[r] = (uint8_t)v; break;
    case FE_LP: gv.rlp[r] = (uint8_t)v; break; default: gv.rhf[r] = (uint8_t)v; break;
    }
}
static int32_t gv_fxget(uint32_t e)                   /* the target's value: the mean of its tracks */
{
    uint32_t k, n = 0;
    int32_t a = 0;
    if (gu.fxt == 0u) { for (k = 0; k < 8u; k++) a += gv_dget(k, e); n = 8; }
    else if (gu.fxt == 1u) { for (k = 0; k < GV_NT; k++) a += *gv_tfld(&gv.t[k], e); n = GV_NT; }
    else { a = *gv_tfld(&gv.t[FXT_TRK[gu.fxt]], e); n = 1; }
    return (a + (int32_t)n / 2) / (int32_t)n;
}
static void gv_fxset(uint32_t e, int32_t v)
{
    uint32_t k;
    if (gu.fxt == 0u) for (k = 0; k < 8u; k++) gv_dset(k, e, v);
    else if (gu.fxt == 1u) for (k = 0; k < GV_NT; k++) *gv_tfld(&gv.t[k], e) = (uint8_t)v;
    else *gv_tfld(&gv.t[FXT_TRK[gu.fxt]], e) = (uint8_t)v;
}
static uint32_t gv_fxn(uint32_t e) { return e == FE_RL ? 4u : e == FE_LP ? 5u : e == FE_RV || e == FE_HF ? 2u : 0u; }   /* choices */
static int32_t gv_fxix(uint32_t e)                    /* a choice effect: the index of its value */
{
    int32_t v = gv_fxget(e), k, b = 0;
    if (e == FE_RL) { for (k = 0; k < 4; k++) if (gv_abs(FE_RLO[k] - v) < gv_abs(FE_RLO[b] - v)) b = k; return b; }
    if (e == FE_LP) { for (k = 0; k < 5; k++) if (gv_abs(FE_LPO[k] - v) < gv_abs(FE_LPO[b] - v)) b = k; return b; }
    return v > 0;
}
static int gv_fxon(uint32_t e) { return FE_OFF[e] < 0 || gv_fxget(e) != FE_OFF[e]; }
static void gv_fxbyp(uint32_t e)                       /* a white key 9-16: the effect cut / back */
{
    int16_t *b = &gu.byp[gu.fxt][e];
    char m[32];
    if (FE_OFF[e] < 0) { gv_say("REGLE-LE AU KNOB"); return; }
    if (*b >= 0) { gv_fxset(e, *b); *b = -1; gv_fmt(m, "%s RETABLI", FE_L[e]); }
    else { *b = (int16_t)gv_fxget(e); gv_fxset(e, FE_OFF[e]); gv_fmt(m, "%s COUPE", FE_L[e]); }
    gv_say(m);
}
static const uint8_t GV_LPV[7] = {0, 1, 2, 3, 4, 6, 8}, GV_RLV[4] = {0, 2, 3, 4};   /* LOOP / ROLL choices (web: LOOP ROLL) */
static int32_t gv_lpix(uint8_t v) { int32_t k; for (k = 0; k < 7; k++) if (GV_LPV[k] == v) return k; return 0; }
static int32_t gv_rlix(uint8_t v) { int32_t k; for (k = 0; k < 4; k++) if (GV_RLV[k] == v) return k; return 0; }
static void gv_krange(uint32_t id, int32_t *mn, int32_t *mx)
{
    *mn = 0; *mx = 127;
    switch (id) {
    case KN_SWING: *mx = 100; break;
    case KN_TAR: case KN_RAR: case KN_AUTO: *mx = 6; break;
    case KN_STYLE: *mx = GV_NSTY - 1; break;
    case KN_GMR: case KN_LFS: *mx = 1; break;
    case KN_PITCH: *mn = -3; *mx = 3; break;
    case KN_BPM: *mn = 40; *mx = 240; break;
    case KN_PROG: *mx = GV_NPROG[gv.scale % GV_NSC]; break;
    case KN_ROOT: *mx = 11; break;
    case KN_SCALE: *mx = GV_NSC - 1; break;
    case KN_FTYPE: case KN_WAVE: *mx = 2; break;
    case KN_LFW: case KN_SACC: case KN_SOFF: *mx = 3; break;
    case KN_OCT: *mn = 1; *mx = 6; break;
    case KN_SNOTE: *mx = 2 * (int32_t)gv_L(); break;
    case KN_SLEN: *mn = 1; *mx = 64; break;
    case KN_ROW: *mx = 7; break;
    case KN_MODEL: *mx = NMODELS - 1; break;
    case KN_FX0: case KN_FX1: case KN_FX2: case KN_FX3: { uint32_t e = gu.fxpg * 4u + id - KN_FX0, c = gv_fxn(e); *mx = c ? (int32_t)c - 1 : 127; break; }
    case KN_PTG: *mx = 2; break;
    case KN_LOCK: case KN_TBW: *mx = 1; break;
    case KN_RLP: case KN_TLP: *mx = 6; break;
    case KN_RRL: case KN_TRL: *mx = 3; break;
    default: break;
    }
}
static uint32_t gv_ar_idx(uint8_t v) { uint32_t k; for (k = 0; k < 7u; k++) if (GV_AUS[k] == v) return k; return 0; }
static int32_t gv_kget(uint32_t id)
{
    gv_trk_t *t = gv_tr();
    track_t *r = gv_rt();
    switch (id) {
    case KN_TVOL: return t->vol; case KN_RVOL: return r->p[P_LEVEL]; case KN_SWING: return song.g[G_SWING];
    case KN_TAR: return (int32_t)gv_ar_idx(t->ar); case KN_RAR: return (int32_t)gv_ar_idx(gv.rowar[gv.sel & 7]);
    case KN_STYLE: return gv.style; case KN_GMR: return !(gv.gmute & 1u); case KN_PITCH: return t->po;
    case KN_BPM: return song.g[G_BPM]; case KN_AUTO: return (int32_t)gv_ar_idx(gv.autob); case KN_PROG: return gv.prog;
    case KN_ROOT: return gv.root; case KN_SCALE: return gv.scale; case KN_CUT: return t->cut; case KN_RES: return t->res;
    case KN_FTYPE: return t->ftype; case KN_ENV: return t->env; case KN_ATT: return t->att; case KN_DEC: return t->dec;
    case KN_GL: return t->gl; case KN_LFR: return t->lfr; case KN_LFD: return t->lfd; case KN_LFW: return t->lfw;
    case KN_LFS: return t->lfs; case KN_WAVE: return t->wave; case KN_OCT: return t->oct; case KN_DIST: return t->dist;
    case KN_DLY: return t->dly; case KN_REV: return t->rev; case KN_DMIX: return song.g[G_DMIX];
    case KN_DTIME: return song.g[G_DTIME]; case KN_RSIZE: return song.g[G_RSIZE]; case KN_RDAMP: return song.g[G_RDAMP];
    case KN_SNOTE: return gv_stp()->n < 0 ? 0 : gv_stp()->n; case KN_SLEN: return gv_stp()->l4;
    case KN_SACC: return gv_stp()->a | (gv_stp()->s << 1); case KN_SOFF: return gv_stp()->o4;
    case KN_ROW: return gv.sel & 7; case KN_MODEL: return r->p[P_MODEL]; case KN_RTUNE: return r->p[P_E0];
    case KN_RDEC: return r->p[P_E1]; case KN_RDIST: return r->p[P_DIST]; case KN_RREV: return r->p[P_REV];
    case KN_FX0: case KN_FX1: case KN_FX2: case KN_FX3: { uint32_t e = gu.fxpg * 4u + id - KN_FX0; return gv_fxn(e) ? gv_fxix(e) : gv_fxget(e); }
    case KN_DJ: return gv.hp ? 64 + gv.hp / 2 : gv.mf >= 127u ? 64 : gv.mf / 2;
    case KN_MF: return gv.mf; case KN_HP: return gv.hp; case KN_MQ: return gv.mq; case KN_XS: return gv.xs;
    case KN_XD: return gv.xd; case KN_XR: return gv.xr; case KN_PTG: return gv_ptg; case KN_LOCK: return gu.lock;
    case KN_RLP: return gv_lpix(gv.rlp[gv.sel & 7]); case KN_RRL: return gv_rlix(gv.rrl[gv.sel & 7]);
    case KN_TLP: return gv_lpix(t->lp); case KN_TRL: return gv_rlix(t->rl); case KN_TBW: return t->bw;
    case KN_DVOL: case KN_DDIST: case KN_DDLY: case KN_DREV: {
        static const uint8_t PP[4] = {P_LEVEL, P_DIST, P_DLY, P_REV};
        int32_t a = 0; uint32_t k;
        for (k = 0; k < 8u; k++) a += trk[GV_ROWTRK[k]].p[PP[id - KN_DVOL]];
        return a / 8; }
    default: return 0;
    }
}
static void gv_kset(uint32_t id, int32_t v)
{
    gv_trk_t *t = gv_tr();
    track_t *r = gv_rt();
    gv_step_t *s;
    switch (id) {
    case KN_TVOL: t->vol = (uint8_t)v; break; case KN_RVOL: r->p[P_LEVEL] = (int16_t)v; break;
    case KN_SWING: song.g[G_SWING] = (int16_t)v; break; case KN_TAR: t->ar = GV_AUS[v]; break;
    case KN_RAR: gv.rowar[gv.sel & 7] = GV_AUS[v]; break; case KN_STYLE: gv_set_style((uint32_t)v); break;
    case KN_GMR: gv.gmute = (uint8_t)(v ? gv.gmute & ~1u : gv.gmute | 1u); break; case KN_PITCH: t->po = (int8_t)v; break;
    case KN_BPM: song.g[G_BPM] = (int16_t)v; break; case KN_AUTO: gv.autob = GV_AUS[v]; break;
    case KN_PROG: gv.prog = (uint8_t)v; break; case KN_ROOT: gv.root = (uint8_t)v; break;
    case KN_SCALE: gv_set_scale((uint32_t)v); break; case KN_CUT: t->cut = (uint8_t)v; break; case KN_RES: t->res = (uint8_t)v; break;
    case KN_FTYPE: t->ftype = (uint8_t)v; break; case KN_ENV: t->env = (uint8_t)v; break; case KN_ATT: t->att = (uint8_t)v; break;
    case KN_DEC: t->dec = (uint8_t)v; break; case KN_GL: t->gl = (uint8_t)v; break; case KN_LFR: t->lfr = (uint8_t)v; break;
    case KN_LFD: t->lfd = (uint8_t)v; break; case KN_LFW: t->lfw = (uint8_t)v; break; case KN_LFS: t->lfs = (uint8_t)v; break;
    case KN_WAVE: t->wave = (uint8_t)v; break; case KN_OCT: t->oct = (uint8_t)v; break; case KN_DIST: t->dist = (uint8_t)v; break;
    case KN_DLY: t->dly = (uint8_t)v; break; case KN_REV: t->rev = (uint8_t)v; break;
    case KN_DMIX: song.g[G_DMIX] = (int16_t)v; break; case KN_DTIME: song.g[G_DTIME] = (int16_t)v; break;
    case KN_RSIZE: song.g[G_RSIZE] = (int16_t)v; break; case KN_RDAMP: song.g[G_RDAMP] = (int16_t)v; break;
    case KN_SNOTE: s = gv_stp(); if (s->n < 0) { s->l4 = 4; s->o4 = 0; } s->n = (int8_t)v; gu.deg = (int8_t)v; break;
    case KN_SLEN: gv_stp()->l4 = (uint8_t)v; break;
    case KN_SACC: s = gv_stp(); s->a = (uint8_t)(v & 1); s->s = (uint8_t)(v >> 1); break;
    case KN_SOFF: gv_stp()->o4 = (uint8_t)v; break;
    case KN_ROW: gv.sel = (int8_t)v; break;
    case KN_MODEL: drum_set_model(r, (uint32_t)v); break;
    case KN_RTUNE: r->p[P_E0] = (int16_t)v; break; case KN_RDEC: r->p[P_E1] = (int16_t)v; break;
    case KN_RDIST: r->p[P_DIST] = (int16_t)v; break; case KN_RREV: r->p[P_REV] = (int16_t)v; break;
    case KN_FX0: case KN_FX1: case KN_FX2: case KN_FX3: {
        uint32_t e = gu.fxpg * 4u + id - KN_FX0;
        gv_fxset(e, e == FE_RL ? FE_RLO[v & 3] : e == FE_LP ? FE_LPO[v % 5] : v);
        gu.byp[gu.fxt][e] = -1;
        break; }
    case KN_DJ:                                       /* left: the low-pass closes; right: the high-pass opens */
        if (v < 64) { gv.mf = (uint8_t)(v * 2); gv.hp = 0; } else if (v > 64) { gv.mf = 127; gv.hp = (uint8_t)((v - 64) * 2); }
        else { gv.mf = 127; gv.hp = 0; }
        break;
    case KN_MF: gv.mf = (uint8_t)v; break; case KN_HP: gv.hp = (uint8_t)v; break; case KN_MQ: gv.mq = (uint8_t)v; break;
    case KN_XS: gv.xs = (uint8_t)v; break; case KN_XD: gv.xd = (uint8_t)v; break; case KN_XR: gv.xr = (uint8_t)v; break;
    case KN_PTG: gv_ptg = (uint8_t)v; break; case KN_LOCK: gu.lock = (uint8_t)v; break;
    case KN_RLP: gv.rlp[gv.sel & 7] = GV_LPV[v % 7]; break; case KN_RRL: gv.rrl[gv.sel & 7] = GV_RLV[v & 3]; break;
    case KN_TLP: t->lp = GV_LPV[v % 7]; break; case KN_TRL: t->rl = GV_RLV[v & 3]; break; case KN_TBW: t->bw = (uint8_t)v; break;
    case KN_DVOL: case KN_DDIST: case KN_DDLY: case KN_DREV: {
        static const uint8_t PP[4] = {P_LEVEL, P_DIST, P_DLY, P_REV};
        uint32_t k;
        for (k = 0; k < 8u; k++) trk[GV_ROWTRK[k]].p[PP[id - KN_DVOL]] = (int16_t)v;
        break; }
    default: break;
    }
}
static void gv_kfmt(uint32_t id, int32_t v, char *b)
{
    static const char *const FT[3] = {"PASSE-BAS", "PASSE-HAUT", "BANDE"}, *const WV[3] = {"SCIE", "CARRE", "TRI"},
                      *const LW[4] = {"SINUS", "TRIANGLE", "CARRE", "SCIE"};
    switch (id) {
    case KN_TAR: case KN_RAR: case KN_AUTO: if (v) { gv_fmt(b, "%d MES", (int)GV_AUS[v]); } else gv_fmt(b, "OFF"); return;
    case KN_STYLE: gv_fmt(b, "%s", GV_STN[v % GV_NSTY]); return;
    case KN_GMR: gv_fmt(b, v ? "ACTIF" : "COUPE"); return;
    case KN_PITCH: gv_fmt(b, "%+d OCT", (int)v); return;
    case KN_PROG: if (v) { gv_fmt(b, "SUITE %d", (int)v); } else gv_fmt(b, "OFF"); return;
    case KN_ROOT: gv_fmt(b, "%s", GV_NOTE[v % 12]); return;
    case KN_SCALE: gv_fmt(b, "%s", GV_SCN[v % GV_NSC]); return;
    case KN_FTYPE: gv_fmt(b, "%s", FT[v % 3]); return;
    case KN_WAVE: gv_fmt(b, "%s", WV[v % 3]); return;
    case KN_LFW: gv_fmt(b, "%s", LW[v % 4]); return;
    case KN_LFS: gv_fmt(b, v ? "TEMPO" : "LIBRE"); return;
    case KN_SNOTE: gv_fmt(b, "%s", gv_stp()->n < 0 ? "-" : gv_degname((int)v)); return;
    case KN_SLEN: gv_fmt(b, "%d.%d PAS", (int)(v / 4), (int)(v % 4 * 25)); return;
    case KN_SACC: { static const char *const A[4] = {"-", "ACCENT", "SLIDE", "ACC+SL"}; gv_fmt(b, "%s", A[v & 3]); return; }
    case KN_SOFF: gv_fmt(b, v ? "+%d/4" : "SUR LE PAS", (int)v); return;
    case KN_ROW: gv_fmt(b, "%s", GV_ROWN[v & 7]); return;
    case KN_MODEL: gv_fmt(b, "%s", DMODELS[(uint32_t)v % NMODELS].name); return;
    case KN_BPM: case KN_OCT: gv_fmt(b, "%d", (int)v); return;
    case KN_SWING: gv_fmt(b, "%d%%", (int)v); return;
    case KN_DJ: if (v == 64) { gv_fmt(b, "OUVERT"); } else if (v < 64) { gv_fmt(b, "GRAVE %d", (int)(64 - v)); } else gv_fmt(b, "AIGU %d", (int)(v - 64)); return;
    case KN_MF: if (v >= 127) { gv_fmt(b, "OUVERT"); return; } break;
    case KN_HP: if (!v) { gv_fmt(b, "OUVERT"); return; } break;
    case KN_PTG: { static const char *const T[3] = {"TOUT", "RYTHME", "MELODIE"}; gv_fmt(b, "%s", T[v % 3]); return; }
    case KN_LOCK: case KN_TBW: gv_fmt(b, id == KN_TBW ? (v ? "ARRIERE" : "AVANT") : (v ? "ON" : "OFF")); return;
    case KN_RLP: case KN_TLP: if (v) { gv_fmt(b, "%d PAS", (int)GV_LPV[v % 7]); } else gv_fmt(b, "OFF"); return;
    case KN_RRL: case KN_TRL: if (v) { gv_fmt(b, "X%d", (int)GV_RLV[v & 3]); } else gv_fmt(b, "OFF"); return;
    case KN_FX0: case KN_FX1: case KN_FX2: case KN_FX3: {
        uint32_t e = gu.fxpg * 4u + id - KN_FX0;
        if (e == FE_RL) { if (v) { gv_fmt(b, "X%d", (int)FE_RLO[v & 3]); } else gv_fmt(b, "OFF"); return; }
        if (e == FE_LP) { if (v) { gv_fmt(b, "%d PAS", (int)FE_LPO[v % 5]); } else gv_fmt(b, "OFF"); return; }
        if (e == FE_RV || e == FE_HF) { gv_fmt(b, v ? "ON" : "OFF"); return; }
        if (e == FE_CUT && v >= 127) { gv_fmt(b, "OUVERT"); return; }
        gv_fmt(b, "%d%%", (int)(v * 100 / 127)); return; }
    default: gv_fmt(b, "%d%%", (int)(v * 100 / 127)); return;
    }
}
/* the page's 4 knobs */
static void gv_knobs(uint8_t *k)
{
    int d = gv_isd();
    k[0] = k[1] = k[2] = k[3] = KN_NONE;
#define K4(a, b, c, e) do { k[0] = a; k[1] = b; k[2] = c; k[3] = e; } while (0)
    switch (gu.page) {                                 /* = the web app's pages (fm1.js PAGES) */
    case GP_LIVE: K4(d ? KN_RVOL : KN_TVOL, KN_DJ, KN_SWING, d ? KN_RAR : KN_TAR); break;
    case GP_LRYT: K4(KN_RVOL, KN_RAR, KN_STYLE, KN_GMR); break;
    case GP_LMEL: K4(KN_TVOL, KN_TAR, KN_PITCH, KN_WAVE); break;
    case GP_LFX: K4(KN_FX0, KN_FX1, KN_FX2, KN_FX3); break;
    case GP_RYT: K4(KN_ROW, KN_RVOL, KN_RLP, KN_RRL); break;
    case GP_MEL: K4(KN_SNOTE, KN_SLEN, KN_SACC, KN_SOFF); break;
    case GP_STY: K4(KN_STYLE, KN_BPM, KN_AUTO, KN_SWING); break;
    case GP_VIS: K4(KN_BPM, KN_DJ, KN_SWING, KN_AUTO); break;
    case GP_GRYT: K4(KN_STYLE, KN_AUTO, KN_RAR, KN_SWING); break;
    case GP_GMEL: K4(KN_PROG, KN_TAR, KN_SCALE, KN_ROOT); break;
    case GP_FXR: K4(KN_DJ, KN_XD, KN_XR, KN_XS); break;
    case GP_FXA: K4(KN_MF, KN_HP, KN_MQ, KN_PTG); break;
    case GP_FXB: K4(KN_XD, KN_XR, KN_XS, KN_NONE); break;
    case GP_FXD: K4(KN_DVOL, KN_DDIST, KN_DDLY, KN_DREV); break;
    case GP_FXP: K4(KN_PTG, KN_NONE, KN_LOCK, KN_NONE); break;
    case GP_SCL: K4(KN_ROOT, KN_SCALE, KN_PROG, KN_PITCH); break;
    case GP_ENV: K4(KN_ATT, KN_DEC, KN_ENV, KN_GL); break;
    case GP_FLT: K4(KN_CUT, KN_RES, KN_FTYPE, KN_ENV); break;
    case GP_LFO: K4(KN_LFR, KN_LFD, KN_LFW, KN_LFS); break;
    case GP_OSC: K4(KN_WAVE, KN_OCT, KN_PITCH, KN_TVOL); break;
    case GP_MIX: if (d) K4(KN_RVOL, KN_RAR, KN_DVOL, KN_DREV); else K4(KN_TVOL, KN_DIST, KN_DLY, KN_REV); break;
    case GP_LRR: if (d) K4(KN_RLP, KN_RRL, KN_ROW, KN_RAR); else K4(KN_TLP, KN_TRL, KN_TBW, KN_TAR); break;
    case GP_SON: if (d) K4(KN_MODEL, KN_RTUNE, KN_RDEC, KN_RVOL); else K4(KN_WAVE, KN_OCT, KN_CUT, KN_DEC); break;
    case GP_GLO: K4(KN_BPM, KN_SWING, KN_STYLE, KN_AUTO); break;
    default: break;
    }
    if (!d && gu.page == GP_RYT) gv.sel = 0;
    if (d && (gu.page == GP_MEL || gu.page == GP_LMEL || gu.page == GP_GMEL || gu.page == GP_ENV || gu.page == GP_FLT ||
              gu.page == GP_LFO || gu.page == GP_OSC))
        gv.sel = 8;
}

/* ------------------------------------------------------------- the web app's key pages --- */
static const char *const K_FXR[16] = {"COUPE-HAUT", "COUPE-BAS", "RESONANCE", "ECHO", "REVERB", "DISTO", "DISTO BAT", "DELAY BAT",
                                      "REVERB BAT", "LOOP ROLL", "DISTO PIST", "ECHO PIST", "REV PIST", "LFO PIST", "MOUVEMENTS", "TOUT COUPER"};
static const char *const K_PERF[16] = {"LOOP 1", "LOOP 2", "LOOP 4", "LOOP 8", "ROLL X2", "ROLL X3", "ROLL X4", "REVERSE",
                                       "TOUT", "RYTHME", "MELODIE", "VERROU", "REC FX", "REJOUE", "EFFACER", "ZERO"};
static const char *const K_LRR[16] = {"LOOP OFF", "LOOP 1", "LOOP 2", "LOOP 3", "LOOP 4", "LOOP 6", "LOOP 8", "",
                                      "ROLL OFF", "ROLL X2", "ROLL X3", "ROLL X4", "", "SENS <", "REVERSE", ""};
static const char *const K_VIS[16] = {"RYTHME ?", "MELODIES ?", "TOUT ?", "VARIER RYT", "VARIER MEL", "PLAY", "", "", "", "", "", "", "", "", "", ""};
static const char *const K_GLO[16] = {"TAP TEMPO", "", "ALGO PAGES", "ALGO BPM", "ALGO DJ", "ALGO VOLUME", "ALGO RYTHMES", "ALGO SWING",
                                      "", "", "", "", "", "", "", "HORTATOR"};
static const uint8_t K_PLP[4] = {1, 2, 4, 8};
static int gv_rows_any(uint32_t pp) { uint32_t k; for (k = 0; k < 8u; k++) if (trk[GV_ROWTRK[k]].p[pp]) return 1; return 0; }
static int gv_trk_any(uint32_t f) { uint32_t i; for (i = 0; i < GV_NT; i++) if (*gv_tfld(&gv.t[i], f)) return 1; return 0; }
static int gv_lr_any(void)
{
    uint32_t i;
    if (gv_plp || gv_prl) return 1;
    for (i = 0; i < 8u; i++) if (gv.rlp[i] || gv.rrl[i]) return 1;
    for (i = 0; i < GV_NT; i++) if (gv.t[i].lp || gv.t[i].rl) return 1;
    return 0;
}
static int gv_fxr_on(uint32_t j)                      /* FX RECAP: is this effect at work */
{
    switch (j) {
    case 0: return gv.mf < 127u; case 1: return gv.hp > 0u; case 2: return gv.mq > 0u; case 3: return gv.xd > 0u;
    case 4: return gv.xr > 0u; case 5: return gv.xs > 0u; case 6: return gv_rows_any(P_DIST); case 7: return gv_rows_any(P_DLY);
    case 8: return gv_rows_any(P_REV); case 9: return gv_lr_any(); case 10: return gv_trk_any(FE_DIST); case 11: return gv_trk_any(FE_DLY);
    case 12: return gv_trk_any(FE_REV); case 13: return gv_trk_any(FE_LFD); case 15: { uint32_t k; for (k = 0; k < 14u; k++) if (gv_fxr_on(k)) return 1; return 0; }
    default: return 0;
    }
}
static void gv_fxr_cut(uint32_t j)                    /* FX RECAP: a key cuts that effect */
{
    uint32_t i;
    switch (j) {
    case 0: gv.mf = 127; break; case 1: gv.hp = 0; break; case 2: gv.mq = 0; break; case 3: gv.xd = 0; break;
    case 4: gv.xr = 0; break; case 5: gv.xs = 0; break;
    case 6: case 7: case 8: { static const uint8_t PP[3] = {P_DIST, P_DLY, P_REV}; for (i = 0; i < 8u; i++) trk[GV_ROWTRK[i]].p[PP[j - 6u]] = 0; break; }
    case 9: gv_plp = gv_prl = 0; for (i = 0; i < 8u; i++) gv.rlp[i] = gv.rrl[i] = 0; for (i = 0; i < GV_NT; i++) gv.t[i].lp = gv.t[i].rl = 0; break;
    case 10: case 11: case 12: case 13: { static const uint8_t FF[4] = {FE_DIST, FE_DLY, FE_REV, FE_LFD}; for (i = 0; i < GV_NT; i++) *gv_tfld(&gv.t[i], FF[j - 10u]) = 0; break; }
    case 15: for (i = 0; i < 14u; i++) gv_fxr_cut(i); break;
    default: break;
    }
}
static int gv_perf_on(uint32_t j)
{
    if (j < 4u) return gv_plp == K_PLP[j];
    if (j < 7u) return gv_prl == j - 2u;
    if (j == 7u) return gv_prv != 0u;
    if (j < 11u) return gv_ptg == j - 8u;
    return j == 11u && gu.lock;
}
static void gv_perf_key(uint32_t j, int down)         /* the FX pads: on while held (VERROU: until pressed again) */
{
    if (j < 8u) {
        int on = gv_perf_on(j);
        if (down && gu.lock && on) on = -1;           /* latched and pressed again: off */
        else if (!down && gu.lock) return;
        if (down && on != -1) { if (j < 4u) gv_plp = K_PLP[j]; else if (j < 7u) gv_prl = (uint8_t)(j - 2u); else gv_prv = 1; }
        else { if (j < 4u) { if (on) gv_plp = 0; } else if (j < 7u) { if (on) gv_prl = 0; } else gv_prv = 0; }
        return;
    }
    if (!down) return;
    if (j < 11u) { gv_ptg = (uint8_t)(j - 8u); gv_say(K_PERF[j]); }
    else if (j == 11u) { gu.lock ^= 1u; if (!gu.lock) gv_plp = gv_prl = gv_prv = 0; gv_say(gu.lock ? "VERROU : LES PADS RESTENT" : "VERROU COUPE"); }
    else if (j < 15u) gv_say("MOUVEMENTS : VERSION WEB SEULEMENT");
    else { gv_plp = gv_prl = gv_prv = 0; gv.mf = 127; gv.hp = 0; gv.mq = 0; gv.xs = 0; gv.xd = 0; gv.xr = 0; gv_say("EFFETS A ZERO"); }
}
static int gv_lrr_on(uint32_t j)                      /* MIX / LOOP ROLL: the shown track's loop, roll, way */
{
    int d = gv_isd();
    uint8_t lp = d ? gv.rlp[gv.sel & 7] : gv_tr()->lp, rl = d ? gv.rrl[gv.sel & 7] : gv_tr()->rl;
    if (j < 7u) return lp == GV_LPV[j];
    if (j >= 8u && j < 12u) return rl == GV_RLV[j - 8u];
    if (j == 13u) return !d && gv_tr()->bw;
    if (j == 14u) return d ? gv.rrv[gv.sel & 7] != 0u : gv_tr()->rv != 0u;
    return 0;
}
static void gv_lrr_key(uint32_t j)
{
    int d = gv_isd();
    gv_trk_t *t = gv_tr();
    if (j < 7u) { if (d) gv.rlp[gv.sel & 7] = GV_LPV[j]; else t->lp = GV_LPV[j]; }
    else if (j >= 8u && j < 12u) { if (d) gv.rrl[gv.sel & 7] = GV_RLV[j - 8u]; else t->rl = GV_RLV[j - 8u]; }
    else if (j == 13u && !d) t->bw ^= 1u;
    else if (j == 14u) { if (d) gv.rrv[gv.sel & 7] ^= 1u; else t->rv ^= 1u; }
}
static void gv_tap(void)                               /* TAP TEMPO: the mean of the last taps (2 s apart at most) */
{
    uint32_t n = 0, k;
    char m[24];
    if (gu.tap[0] && fm1_ms - gu.tap[0] > 2000u) memset(gu.tap, 0, sizeof gu.tap);
    for (k = 3u; k > 0u; k--) gu.tap[k] = gu.tap[k - 1u];
    gu.tap[0] = fm1_ms;
    for (k = 0; k < 4u && gu.tap[k]; k++) n++;
    if (n >= 2u) {
        song.g[G_BPM] = (int16_t)gv_clamp((int32_t)(60000u * (n - 1u) / (gu.tap[0] - gu.tap[n - 1u] + 1u)), 40, 240);
        gv_fmt(m, "TAP %d BPM", (int)song.g[G_BPM]);
        gv_say(m);
    } else gv_say("TAP : ENCORE");
}

/* ------------------------------------------------------------- pages: the white keys --- */
static int gv_live_page(void) { return gu.page <= GP_LMEL; }
static void gv_white(uint32_t j, int down)
{
    gv_trk_t *t = gv_tr();
    if (GV_PERFP(gu.page)) { gv_perf_key(j, down); gu.dirty = 1; return; }
    if (!down) return;
    if (gu.kp && gv_live_page()) {                     /* JEU: the white keys play too (ISR: gv_keys_isr) */
        return;
    }
    switch (gu.page) {
    case GP_LIVE:
        if (j < 8u) trk[GV_ROWTRK[j]].p[P_MUTE] ^= 1;
        else if (j < 14u) gv.t[j - 8u].mute ^= 1u;
        else gv.gmute ^= j == 14u ? 1u : 2u;
        break;
    case GP_LRYT: if (j < 8u) trk[GV_ROWTRK[j]].p[P_MUTE] ^= 1; else gv.solo ^= (uint16_t)(1u << (j - 8u)); break;
    case GP_LMEL:
        if (j < 6u) gv.t[j].mute ^= 1u; else if (j == 6u) gv.gmute ^= 2u; else if (j == 7u) { gv_gen_mel((uint32_t)gv.sel - 8u); gv_say("NOUVELLE MELODIE"); }
        else if (j < 14u) gv.solo ^= (uint16_t)(1u << j); else if (j == 14u) { uint32_t i; for (i = 0; i < GV_NT; i++) gv_gen_mel(i); gv_say("TOUTES LES PISTES"); }
        else gv.solo = 0;
        break;
    case GP_LFX:
        if (j < 8u) { gu.fxt = (uint8_t)j; gv_say(FXT_N[j]); }
        else gv_fxbyp((gu.fxpg >> 1) * 8u + j - 8u);
        break;
    case GP_RYT: { step_t *s = &gv_rt()->step[j]; s->on ^= 1u; gu.cur = (uint8_t)j; gv.tpl = 0; break; }
    case GP_MEL: {
        gv_step_t *s = &t->st[j];
        gu.cur = (uint8_t)j;
        if (s->n >= 0) { s->n = -1; } else { s->n = gu.deg; s->l4 = 4; s->o4 = 0; s->a = s->s = 0; }
        break; }
    case GP_STY: if (j < GV_NSTY) { gu.sty = (uint8_t)j; gv_say(GV_STN[j]); } break;
    case GP_GRYT:
        if (j == 0u) { gv_gen_rhythm(); gv_say("NOUVEAU RYTHME"); } else if (j == 1u) { gv_vary_rhythm(); gv_say("RYTHME VARIE"); }
        else if (j == 2u) gv_gen_row((uint32_t)gv.sel & 7u);
        else if (j == 3u) gv_rhythm_row_set((uint32_t)gv.sel & 7u, 0);
        else if (j == 4u) { uint32_t r; for (r = 0; r < 8u; r++) gv_rhythm_row_set(r, 0); }
        break;
    case GP_GMEL:
        if (j == 0u) gv_gen_mel((uint32_t)gv.sel - 8u); else if (j == 1u) gv_vary_mel((uint32_t)gv.sel - 8u);
        else if (j == 2u) { uint32_t i; for (i = 0; i < GV_NT; i++) gv_gen_mel(i); }
        else if (j == 3u) gv_mel_clear(t);
        else if (j == 4u) { uint32_t i; for (i = 0; i < GV_NT; i++) gv_mel_clear(&gv.t[i]); }
        break;
    case GP_SCL: if (j < 12u) gv.root = (uint8_t)j; break;
    case GP_ENV: case GP_FLT: case GP_LFO: case GP_OSC:
        if (j < 3u) t->wave = (uint8_t)j; else if (j >= 4u && j < 10u) t->oct = (uint8_t)(j - 3u); else if (j >= 13u) t->ftype = (uint8_t)(j - 13u);
        break;
    case GP_MIX: case GP_LRR: gv_lrr_key(j); break;
    case GP_FXR: gv_fxr_cut(j); break;
    case GP_VIS:
        if (j == 0u) { gv_gen_rhythm(); gv_say("NOUVEAU RYTHME"); }
        else if (j == 1u) { uint32_t i; for (i = 0; i < GV_NT; i++) gv_gen_mel(i); gv_say("NOUVELLES MELODIES"); }
        else if (j == 2u) { gv_gen_all(); gv_say("TOUT REGENERE"); }
        else if (j == 3u) { gv_vary_rhythm(); gv_say("RYTHME VARIE"); }
        else if (j == 4u) { uint32_t i; for (i = 0; i < GV_NT; i++) gv_vary_mel(i); gv_say("MELODIES VARIEES"); }
        else if (j == 5u) play_press();
        break;
    case GP_GLO:
        if (j == 0u) gv_tap();
        else if (j >= 2u && j < 8u) { gv.algo = (uint8_t)(j - 2u); gv_say(K_GLO[j]); }
        else if (j == 15u) { gv_on = 0; ui.force = 1; }
        break;
    default: break;
    }
    gu.dirty = 1;
}
static void gv_select(int32_t s)
{
    switch (gu.page) {
    case GP_RYT: case GP_MEL: gu.cur = (uint8_t)((gu.cur + (uint32_t)s) & 15u); break;
    case GP_LFX: gu.fxt = (uint8_t)((gu.fxt + 8 + s % 8) % 8); gv_say(FXT_N[gu.fxt]); break;
    case GP_STY: gu.sty = (uint8_t)((gu.sty + GV_NSTY * 4 + s) % GV_NSTY); break;
    case GP_SCL: gv_set_scale((uint32_t)((gv.scale + GV_NSC * 4 + s) % GV_NSC)); break;
    case GP_GLO: song.g[G_BPM] = (int16_t)gv_clamp(song.g[G_BPM] + s, 40, 240); break;
    case GP_LMEL: case GP_GMEL: case GP_ENV: case GP_FLT: case GP_LFO: case GP_OSC:
        gv.sel = (int8_t)(8 + (((gv.sel - 8) + s) % 6 + 6) % 6); break;
    case GP_LRYT: case GP_GRYT:
        gv.sel = (int8_t)((((gv.sel & 7) + s) % 8 + 8) % 8); break;
    default: gv.sel = (int8_t)(((gv.sel + s) % 14 + 14) % 14); break;
    }
}
static void gv_go(uint32_t b)
{
    uint32_t k, n = 0;
    for (k = 0; k < 5u && GV_GRP[b][k] != 0xFF; k++) n++;
    if (!n) return;
    if (gu.btn == b) {
        for (k = 0; k < n && GV_GRP[b][k] != gu.page; k++) {}
        gu.page = GV_GRP[b][(k + 1u) % n];
    } else {
        gu.btn = (uint8_t)b; gu.page = GV_GRP[b][0];
    }
    if (gu.page == GP_STY) gu.sty = gv.style;
    gu.dirty = 1;
}
static void gv_long(uint32_t b)
{
    if (b == B_HOME) {
        if (gv_isd()) gv_gen_row((uint32_t)gv.sel); else gv_gen_mel((uint32_t)gv.sel - 8u);
        gv_say(gv_isd() ? "NOUVEAU MOTIF" : "NOUVELLE MELODIE");
    } else if (b == B_ARP) {
        gv_set_style(gu.page == GP_STY ? gu.sty : gv.style);
        gv_gen_all();
        gv_say("STYLE + TOUT REGENERE");
    } else if (b == B_FX && gu.page == GP_LFX) {
        uint32_t e;
        for (e = 0; e < FE_N; e++) if (FE_OFF[e] >= 0) { gv_fxset(e, FE_OFF[e]); gu.byp[gu.fxt][e] = -1; }
        gv_say("FX LIVE A ZERO");
    } else if (b == B_FX) {                           /* the web app's: the page's effects back to zero */
        if (gu.page == GP_FXD) { uint32_t k; for (k = 0; k < 8u; k++) { track_t *r = &trk[GV_ROWTRK[k]]; r->p[P_DIST] = 0; r->p[P_DLY] = 0; r->p[P_REV] = 0; } }
        else { gv.mf = 127; gv.hp = 0; gv.mq = 0; gv.xs = 0; gv.xd = 0; gv.xr = 0; gv_plp = gv_prl = gv_prv = 0; }
        gv_say("EFFETS A ZERO");
    }
    gu.flash = fm1_ms;
    gu.dirty = 1;
}

/* the panel while Groove owns it (ui_input.c, first thing): 1 = taken */
static int gv_ui_input(uint32_t pressed, uint32_t notes, uint32_t now)
{
    uint32_t b, j, held = fm1_in.buttons;
    int32_t s;
    uint8_t kn[4];
    (void)now;
    if (!gv_on) {                                     /* Hortator's pages: ENV held + ARP pressed = back to Groove */
        if (((held >> panel.btn[B_ENV]) & 1u) && ((pressed >> panel.btn[B_ARP]) & 1u)) { gv_on = 1; gu.dirty = 1; }
        else return 0;
    }
    if (!gu.init) { gu.init = 1; gu.page = GP_LIVE; gu.btn = B_HOME; gu.deg = 0; gu.dirty = 1; gv_boot(); }   /* (only once Groove
                                                     * is on: it sets the tempo and the drums' steps) */
    gv_kp = (uint8_t)(gu.kp && gv_live_page());
    if (gv_regen_due) {                               /* the auto-randoms the sequencer asked for (bars) */
        uint32_t r = gv_regen_due, i;
        gv_regen_due = 0;
        if (r & 1u) gv_gen_rhythm();
        for (i = 0; i < GV_NT; i++) if ((r >> (1u + i)) & 1u) gv_gen_mel(i);
        for (i = 0; i < 8u; i++) if ((r >> (8u + i)) & 1u) gv_gen_row(i);
        gu.dirty = 1;
    }
    for (b = 0; b < NB; b++) {
        uint32_t bit = 1u << panel.btn[b];
        int lp = b == B_HOME || b == B_ARP || b == B_FX;
        if (pressed & bit) {
            gu.dirty = 1;
            if (lp) { gu.hold_b = (uint8_t)b; gu.hold_t0 = fm1_ms; gu.hold_done = 0; continue; }
            if (b == B_PLAY) play_press();
            else if (b == B_REC) { gv_recq ^= 1u; gv_say(gv_recq ? "REC : LES TOUCHES NOIRES ECRIVENT" : "REC COUPE"); }
            else if (b == B_OCTDN) { if (gv.oct > 0) gv.oct--; }
            else if (b == B_OCTUP) { if (gv.oct < 3) gv.oct++; }
            else gv_go(b);
        }
    }
    if (gu.hold_b != NB) {
        uint32_t bit = 1u << panel.btn[gu.hold_b];
        if (held & bit) {
            if (!gu.hold_done && fm1_ms - gu.hold_t0 >= 2000u) { gu.hold_done = 1; gv_long(gu.hold_b); }
            gu.dirty = 1;
        } else {
            if (!gu.hold_done) gv_go(gu.hold_b);
            gu.hold_b = NB;
        }
    }
    for (j = 0; j < 16u; j++)
        if ((notes >> STEP_KEY[j]) & 1u) gv_white(j, 1);
    {   /* let go: the FX pads */
        uint32_t cur = fm1_in.notes, gone = gu.wprev & ~cur;
        for (j = 0; j < 16u; j++) if ((gone >> STEP_KEY[j]) & 1u) gv_white(j, 0);
        gu.wprev = cur;
    }
    gv_knobs(kn);
    for (j = 0; j < 4u; j++)
        if ((s = panel_enc(EN_K1 + j)) != 0 && kn[j] != KN_NONE) {
            int32_t mn, mx;
            gv_krange(kn[j], &mn, &mx);
            if (mx - mn > 40) s *= 2;
            gv_kset(kn[j], gv_clamp(gv_kget(kn[j]) + s, mn, mx));
            gu.dirty = 1;
        }
    if ((s = panel_enc(EN_SELECT)) != 0) { gv_select(s); gu.dirty = 1; }
    if ((s = panel_enc(EN_PRESET)) != 0) {
        if (gu.page == GP_RYT || gu.page == GP_LRYT || gu.page == GP_GRYT) gv.sel = (int8_t)(((gv.sel & 7) + s + 8) % 8);
        else if (gu.page == GP_MEL || gu.page == GP_LMEL || gu.page == GP_GMEL) gv.sel = (int8_t)(8 + ((gv.sel - 8 + s) % 6 + 6) % 6);
        else gv.sel = (int8_t)(((gv.sel + s) % 14 + 14) % 14);
        gu.dirty = 1;
    }
    if ((s = panel_enc(EN_ALGO)) != 0) {
        if (gu.page == GP_LFX) {
            int32_t o = gu.fxpg;
            gu.fxpg = (uint8_t)gv_clamp(o + (s > 0 ? 1 : -1), 0, 3);
            gu.fxd = (int8_t)(gu.fxpg >= o ? 1 : -1); gu.fxa = fm1_ms;
            { static const char *const PT[4] = {"A : FILTRE RESO DISTO ECHO", "B : REVERB WOBBLE VITESSE VOL", "C : CHORUS ROLL SAUTS REVERSE",
                                                "D : LOOP CRUSH GATE DEMI"}; gv_say(PT[gu.fxpg]); }
        } else if (gv_live_page()) { gu.kp = s > 0; gv_say(gu.kp ? "MODE JEU : LE CLAVIER JOUE" : "MODE COUPURES"); }
        else if (gv.algo == 0u) {                     /* ALGORITHM: every page in turn (GLO: or a setting) */
            uint32_t k, n = GP_N;
            gu.page = (uint8_t)((gu.page + n + (uint32_t)(s > 0 ? 1 : n - 1)) % n);
            for (b = 0; b < NB; b++) for (k = 0; k < 5u; k++) if (GV_GRP[b][k] == gu.page) gu.btn = (uint8_t)b;
            if (gu.page == GP_STY) gu.sty = gv.style;
        } else {
            static const uint8_t AK[6] = {KN_NONE, KN_BPM, KN_DJ, KN_TVOL, KN_NONE, KN_SWING};
            char m[32], v[20];
            uint32_t id = gv.algo == 3u && gv_isd() ? KN_RVOL : AK[gv.algo % 6u];
            if (gv.algo == 4u) { gv_gen_rhythm(); gv_say("NOUVEAU RYTHME"); }
            else {
                int32_t mn, mx;
                gv_krange(id, &mn, &mx);
                gv_kset(id, gv_clamp(gv_kget(id) + s, mn, mx));
                gv_kfmt(id, gv_kget(id), v);
                gv_fmt(m, "%s %s", KN_L[id], v);
                gv_say(m);
            }
        }
        gu.dirty = 1;
    }
    return 1;
}

/* ------------------------------------------------------------- the screen: 2 strips of 120 rows --- */
static int32_t gv_oy;                                    /* the strip's first row */
static void G_R(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c) { cv_rect(x, y - gv_oy, w, h, c); }
static void G_T(int32_t x, int32_t y, uint32_t sc, const char *s, uint16_t c) { md_text(x, y - gv_oy, &MD_F3, sc, s, c); }
static int32_t G_TW(uint32_t sc, const char *s) { return md_text_w(&MD_F3, sc, s); }
static void G_TC(int32_t cx, int32_t y, uint32_t sc, const char *s, uint16_t c) { G_T(cx - G_TW(sc, s) / 2, y, sc, s, c); }
static void G_TR(int32_t rx, int32_t y, uint32_t sc, const char *s, uint16_t c) { G_T(rx - G_TW(sc, s), y, sc, s, c); }
static void G_F(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)
{
    G_R(x, y, w, 1, c); G_R(x, y + h - 1, w, 1, c); G_R(x, y, 1, h, c); G_R(x + w - 1, y, 1, h, c);
}
static int gv_heard(int p) { return gv_track_heard((uint32_t)p); }
static int gv_flash(int p) { return song.playing && song.tick - gv_hit_t[p] < 20u; }

static void gv_tile(int p, int32_t x, int32_t y, int32_t w, int32_t h, int big)
{
    uint16_t c = gv_col(p);
    int au = gv_heard(p);
    G_R(x, y, w, h, gv_flash(p) && au ? c : GC_P2);
    G_R(x, y + 2, 3, h - 4, au ? c : GC_L);
    G_T(x + 6, y + 4, big ? 2u : 1u, gv_name(p), gv_flash(p) && au ? GC_BG : au ? c : GC_L);
    {
        int32_t lv = p < 8 ? trk[GV_ROWTRK[p]].p[P_LEVEL] : gv.t[p - 8].vol;
        G_R(x + 4, y + h - 6, w - 8, 3, GC_BG);
        G_R(x + 4, y + h - 6, (w - 8) * lv / 127, 3, au ? c : GC_L);
    }
    if (p < 8 ? trk[GV_ROWTRK[p]].p[P_MUTE] : gv.t[p - 8].mute) G_R(x + 4, y + h / 2, w - 8, 1, GC_L);
    if ((gv.solo >> p) & 1u) G_F(x, y, w, h, GC_Y);
    if (p == gv.sel) { G_F(x - 1, y - 1, w + 2, h + 2, GC_T); G_F(x, y, w, h, GC_T); }
}
static void gv_draw_head(void)
{
    char b[24];
    int32_t w = G_TW(2, GP_T[gu.page]) + 8;
    G_R(2, 2, w, 15, GC_A);
    G_T(6, 5, 2, GP_T[gu.page], GC_BG);
    G_F(w + 6, 2, 100, 15, gv_col(gv.sel));
    G_T(w + 10, 5, 2, gv_name(gv.sel), gv_col(gv.sel));
    gv_fmt(b, "%s%d", song.playing ? ">" : "", (int)song.g[G_BPM]);
    G_TR(236, 5, 2, b, song.playing ? GC_Y : GC_M);
    gv_fmt(b, "%s  %s %s", GV_STN[gv.style], GV_NOTE[gv.root], GV_SCN[gv.scale % GV_NSC]);
    G_T(4, 22, 1, b, GC_M);
    if (gu.kp && gv_live_page()) { G_R(170, 20, 30, 9, GC_Y); G_T(174, 22, 1, "JEU", GC_BG); }
    { uint32_t k; for (k = 0; k < 4u; k++) G_R(206 + (int32_t)k * 8, 21, 6, 6, song.playing && gv_pos / 4u == k ? GC_C : GC_P2); }
    G_R(0, 30, 240, 1, GC_L);
}
static void gv_draw_foot(void)
{
    uint8_t kn[4];
    uint32_t i;
    char b[20];
    gv_knobs(kn);
    G_R(0, 184, 240, 1, GC_L);
    for (i = 0; i < 4u; i++) {
        int32_t cx = 30 + 60 * (int32_t)i, mn, mx, v;
        if (kn[i] == KN_NONE) continue;
        gv_krange(kn[i], &mn, &mx);
        v = gv_kget(kn[i]);
        cv_knob(cx, 200 - gv_oy, 9, mx > mn ? (v - mn) * 1000 / (mx - mn) : 0, -1, i & 1u ? GC_C : GC_A);
        G_TC(cx, 216, 1, kn[i] >= KN_FX0 && kn[i] <= KN_FX3 ? (gu.fxt == 0u && FE_LD[gu.fxpg * 4u + i] ? FE_LD[gu.fxpg * 4u + i] : FE_L[gu.fxpg * 4u + i]) : KN_L[kn[i]], GC_M);
        gv_kfmt(kn[i], v, b);
        G_TC(cx, G_TW(2, b) > 58 ? 228 : 226, G_TW(2, b) > 58 ? 1u : 2u, b, GC_T);
    }
}
static void gv_grid16(const char *const *lab, int32_t y0, int32_t hh, uint16_t ink)   /* the 16 white keys, as the web's labels */
{
    uint32_t j;
    int32_t ch = hh / 4;
    for (j = 0; j < 16u; j++) {
        int on = 0, x = 2 + (int32_t)(j % 4u) * 59, y = y0 + (int32_t)(j / 4u) * ch;
        if (!lab[j][0]) continue;
        switch (gu.page) {
        case GP_FXR: on = gv_fxr_on(j); break;
        case GP_FXA: case GP_FXB: case GP_FXP: on = gv_perf_on(j); break;
        case GP_MIX: case GP_LRR: on = gv_lrr_on(j); break;
        case GP_GLO: on = (j >= 2u && j < 8u && gv.algo == j - 2u); break;
        default: break;
        }
        G_R(x, y, 56, ch - 3, on ? ink : GC_P2);
        G_TC(x + 28, y + ch / 2 - 4, 1, lab[j], on ? GC_BG : GC_M);
    }
}
static void gv_draw_body(void)
{
    uint32_t i, c;
    char b[24];
    gv_trk_t *t = gv_tr();
    switch (gu.page) {
    case GP_LIVE:
        for (i = 0; i < 14u; i++) gv_tile((int)i, 3 + (int32_t)(i % 4u) * 59, 34 + (int32_t)(i / 4u) * 37, 56, 34, 0);
        for (i = 0; i < 2u; i++) {
            int off = (gv.gmute >> i) & 1u;
            int32_t x = 3 + (int32_t)(2u + i) * 59, y = 34 + 3 * 37;
            G_R(x, y, 56, 34, off ? RGB(0x3a, 0x14, 0x14) : GC_P);
            G_TC(x + 28, y + 8, 1, i ? "MELODIE" : "RYTHME", off ? GC_R : GC_M);
            G_TC(x + 28, y + 20, 1, off ? "COUPE" : "ACTIF", off ? GC_R : GC_C);
        }
        break;
    case GP_LRYT:
        for (i = 0; i < 8u; i++) gv_tile((int)i, 3 + (int32_t)(i % 4u) * 59, 36 + (int32_t)(i / 4u) * 72, 56, 66, 1);
        break;
    case GP_LMEL:
        for (i = 0; i < 6u; i++) gv_tile((int)(8u + i), 3 + (int32_t)(i % 3u) * 79, 36 + (int32_t)(i / 3u) * 72, 76, 66, 1);
        break;
    case GP_LFX: {
        uint16_t tc = gv_fxtc(gu.fxt);
        int32_t sl = fm1_ms - gu.fxa < 250u ? (int32_t)(250u - (fm1_ms - gu.fxa)) * 40 / 250 * (gu.fxd ? gu.fxd : 1) : 0;
        uint32_t ms = fm1_ms, beat = 60000u / (uint32_t)gv_clamp(song.g[G_BPM], 40, 240), ph = (ms % (beat / 4u)) * 256u / (beat / 4u);
        for (i = 0; i < 8u; i++) {                        /* the targets */
            int32_t x = 2 + (int32_t)i * 30;
            int on = i == gu.fxt;
            G_R(x, 34, 28, 12, on ? gv_fxtc(i) : GC_P2);
            G_TC(x + 14, 37, 1, i == 1u ? "MEL" : i == 0u ? "RYT" : i == 3u ? "BAS2" : i == 5u ? "SYN2" : i == 4u ? "SYN" : i == 2u ? "BASS" : FXT_N[i], on ? GC_BG : gv_fxtc(i));
        }
        G_T(4, 50, 1, FXT_N[gu.fxt], tc);
        gv_fmt(b, "PAGE %s", FXPN[gu.fxpg]);
        G_TR(212, 50, 1, b, GC_Y);
        for (i = 0; i < 4u; i++) G_R(216 + (int32_t)i * 6, 50, 4, 4, i == gu.fxpg ? GC_Y : GC_P2);
        for (i = 0; i < 4u; i++) {                        /* the 4 effects, each its animation */
            uint32_t e = gu.fxpg * 4u + i, q;
            int32_t x = 4 + (int32_t)i * 59 + sl, y = 60, w = 56, h = 92, cx = x + w / 2, cy = y + 40, v = gv_fxget(e), on = gv_fxon(e);
            uint16_t ink = on ? tc : GC_L;
            if (x < 0 || x + w > 240) continue;
            G_R(x, y, w, h, GC_P);
            switch (e) {
            case FE_CUT: case FE_RES: {                   /* the filter's curve and its peak */
                int32_t cut = gv_fxget(FE_CUT) * w / 127, rs = gv_fxget(FE_RES), py = 0;
                for (q = 0; q < (uint32_t)w; q++) {
                    int32_t d = (int32_t)q - cut, yy = d <= 0 ? 0 : d * 3;
                    if (gv_abs(d) < 6) yy -= rs * (6 - gv_abs(d)) / 30;
                    if (e == FE_RES) yy = (int32_t)(gv_sin((q * 24u + ms / 3u) & 255u) * (rs / 8 + 2)) / 127;
                    yy = gv_clamp(cy + (e == FE_RES ? 0 : -10) + yy, y + 4, y + h - 16);
                    if (q) cv_line(x + (int32_t)q - 1, py - gv_oy, x + (int32_t)q, yy - gv_oy, ink);
                    py = yy;
                }
                if (e == FE_RES) for (q = 0; q < 3u; q++) cv_ring(cx, cy - gv_oy, 4 + (int32_t)(((ms / 8u + q * 85u) & 255u) * (uint32_t)(4 + rs / 6) / 256u), ink);
                break; }
            case FE_DIST: case FE_CR: case FE_CHO: case FE_LFD: case FE_LFR: {   /* a wave: clipped, stepped, doubled, moving */
                int32_t py = 0, py2 = 0;
                for (q = 0; q < (uint32_t)w; q++) {
                    uint32_t ang = (q * 9u + ms / (e == FE_LFR ? 2u + (127u - (uint32_t)v) / 16u : 4u)) & 255u, qq = q;
                    int32_t yy;
                    if (e == FE_CR && v) { uint32_t st = 2u + (uint32_t)v / 16u; qq = q / st * st; ang = (qq * 9u + ms / 4u) & 255u; }
                    yy = gv_sin(ang);
                    if (e == FE_DIST) yy = gv_clamp(yy * (1 + v / 12), -127, 127);
                    if (e == FE_CR && v) { int32_t lv = 2 + (127 - v) / 10; yy = yy * lv / 127 * 127 / lv; }
                    if (e == FE_LFD) yy = yy * v / 127;
                    yy = cy - yy * 22 / 127;
                    if (q) cv_line(x + (int32_t)q - 1, py - gv_oy, x + (int32_t)q, yy - gv_oy, e == FE_DIST ? GC_Y : ink);
                    if (e == FE_CHO && v) {
                        int32_t y2 = cy - gv_sin((ang + 20u + (uint32_t)(gv_sin((ms / 6u) & 255u) * v / 300)) & 255u) * 22 / 127;
                        if (q) cv_line(x + (int32_t)q - 1, py2 - gv_oy, x + (int32_t)q, y2 - gv_oy, GC_C);
                        py2 = y2;
                    }
                    py = yy;
                }
                break; }
            case FE_DLY: case FE_REV:                     /* echoes going away / rings growing */
                for (q = 0; q < 5u; q++) {
                    uint32_t p = ((ms * 256u / (beat * (e == FE_REV ? 2u : 1u))) + q * 51u) & 255u;
                    if (e == FE_DLY) { int32_t r = (6 - (int32_t)q) * (v ? 1 : 0); if (r > 0) { int32_t dx = x + 6 + (int32_t)((q * 256u + p) * (uint32_t)(w - 12) / 1280u); G_R(dx - r / 2, cy - r / 2, r, r, ink); } }
                    else cv_ring(cx, cy - gv_oy, 3 + (int32_t)(p * (uint32_t)(6 + v / 5) / 256u), q & 1u ? GC_C : ink);
                }
                break;
            case FE_VOL: {
                int32_t lv = v * 10 / 127;
                if (song.playing && gv_flash(gu.fxt == 0u ? 0 : 8 + FXT_TRK[gu.fxt])) lv = lv > 1 ? lv - 1 : lv;
                for (q = 0; q < 10u; q++) G_R(x + 14, y + h - 22 - (int32_t)q * 7, w - 28, 5, (int32_t)q < lv ? (q > 7u ? GC_R : tc) : GC_P2);
                break; }
            case FE_RL: {
                int32_t n2 = FE_RLO[gv_fxix(e) & 3];
                for (q = 0; q < 4u; q++) {
                    int lit = n2 ? (int32_t)q < n2 : q == 0u;
                    int32_t hh = n2 ? 30 - (int32_t)q * 5 : 30;
                    G_R(x + 6 + (int32_t)q * 12, cy + 16 - hh, 9, hh, lit ? (n2 && q == ph * (uint32_t)n2 / 256u ? GC_Y : ink) : GC_P2);
                }
                if (n2) { gv_fmt(b, "X%d", (int)n2); G_TC(cx, y + 6, 1, b, GC_Y); }
                break; }
            case FE_SK:
                for (q = 0; q < 8u; q++) {
                    int32_t bx = x + 4 + (int32_t)(q % 4u) * 13, by = cy - 18 + (int32_t)(q / 4u) * 20;
                    int skp = (int32_t)(((q * 7u + gv_pos * 3u) % 10u) * 127u / 10u) < v;
                    G_R(bx, by, 10, 14, skp ? GC_P2 : (q == (gv_pos & 7u) ? GC_Y : ink));
                    if (skp) cv_line(bx, by - gv_oy, bx + 9, by + 13 - gv_oy, GC_R);
                }
                break;
            case FE_RV: {
                int r2 = v > 0;
                int32_t ax = x + 6 + (int32_t)((r2 ? 255u - ph : ph) * (uint32_t)(w - 12) / 256u);
                G_R(x + 6, cy - 2, w - 12, 4, GC_P2);
                G_R(ax - 3, cy - 7, 7, 14, r2 ? GC_R : ink);
                G_TC(cx, y + 6, 1, r2 ? "<<" : ">", r2 ? GC_R : GC_M);
                break; }
            case FE_LP: {
                int32_t n2 = FE_LPO[gv_fxix(e) % 5];
                for (q = 0; q < 16u; q++) {
                    int in = !n2 || (int32_t)q < n2, cur = n2 ? (int32_t)q == (int32_t)(gv_pos % (uint32_t)n2) : q == gv_pos;
                    G_R(x + 4 + (int32_t)(q % 4u) * 12, cy - 22 + (int32_t)(q / 4u) * 11, 10, 9, cur && song.playing ? GC_Y : in ? ink : GC_P2);
                }
                if (n2) cv_ring(cx, y + h - 22 - gv_oy, 5 + (int32_t)((ms / 60u) & 1u), GC_Y);
                break; }
            case FE_GT:
                for (q = 0; q < 8u; q++) {
                    int32_t hh = (q & 1u) && v ? 30 * (127 - v) / 127 : 30;
                    G_R(x + 4 + (int32_t)q * 6, cy + 16 - hh, 5, hh, q == ph * 8u / 256u ? GC_Y : (q & 1u) && v ? GC_P2 : ink);
                }
                break;
            default: {                                    /* DEMI-TEMPO: two wheels, half as fast */
                uint32_t sp = v ? ms / 2u : ms;
                for (q = 0; q < 2u; q++) {
                    int32_t wx = cx + (q ? 13 : -13), k2;
                    cv_ring(wx, cy - gv_oy, 9, q ? GC_Y : ink);
                    k2 = (int32_t)((sp / 4u + q * 40u) & 255u);
                    cv_line(wx, cy - gv_oy, wx + gv_sin((uint32_t)k2) * 9 / 127, cy - gv_sin((uint32_t)(k2 + 64) & 255u) * 9 / 127 - gv_oy, q ? GC_Y : ink);
                }
                G_TC(cx, y + 6, 1, v ? "1/2" : "1X", v ? GC_Y : GC_M);
                break; }
            }
            G_TC(cx, y + h - 10, 1, gu.fxt == 0u && FE_LD[e] ? FE_LD[e] : FE_L[e], on ? GC_T : GC_L);
            if (gu.byp[gu.fxt][e] >= 0) G_F(x, y, w, h, GC_R);
        }
        break; }
    case GP_RYT:
        for (i = 0; i < 8u; i++) {
            int32_t y = 36 + (int32_t)i * 18;
            const track_t *r = &trk[GV_ROWTRK[i]];
            int sel = (int)i == (gv.sel & 7);
            if (sel) G_R(0, y - 1, 240, 17, GC_P2);
            G_T(3, y + 5, 1, GV_ROWN[i], r->p[P_MUTE] ? GC_L : GV_ROWC[i]);
            for (c = 0; c < 16u; c++) {
                int32_t x = 40 + (int32_t)c * 12;
                G_R(x, y, 11, 15, r->step[c].on ? GV_ROWC[i] : (c % 4u ? GC_P : GC_P2));
                if (sel && c == gu.cur) G_F(x - 1, y - 1, 13, 17, GC_T);
            }
        }
        if (song.playing) G_F(40 + gv_pos * 12 - 1, 34, 13, 147, GC_C);
        break;
    case GP_MEL: {
        int L = (int)gv_L(), n = 2 * L + 1, rh = 140 / n, d;
        int32_t y0 = 36 + (140 - n * rh) / 2;
        for (d = 0; d < n; d++) {
            int32_t y = y0 + (n - 1 - d) * rh;
            G_R(26, y, 208, rh - 1, d % L == 0 ? RGB(0x26, 0x20, 0x16) : d & 1 ? GC_P : RGB(0x16, 0x19, 0x1c));
            if (d % L == 0) G_TR(23, y + rh / 2 - 2, 1, gv_degname(d), GC_Y);
        }
        for (c = 0; c < 16u; c++) {
            const gv_step_t *s = &t->st[c];
            if (s->n >= 0 && s->n < n) {
                int32_t len = s->l4 * 13 / 4;
                if (26 + (int32_t)c * 13 + len > 234) len = 234 - 26 - (int32_t)c * 13;
                G_R(26 + (int32_t)c * 13 + s->o4 * 3, y0 + (n - 1 - s->n) * rh, len > 3 ? len - 1 : 3, rh - 1, gv_col(gv.sel));
                if (s->a) G_R(27 + (int32_t)c * 13, y0 + (n - 1 - s->n) * rh, 6, 2, GC_T);
            }
        }
        G_F(26 + gu.cur * 13, y0 - 2, 13, n * rh + 3, GC_T);
        if (song.playing) G_F(26 + gv_pos * 13, y0 - 2, 13, n * rh + 3, GC_C);
        break; }
    case GP_STY:
        for (i = 0; i < 9u; i++) {
            int k = ((int)gu.sty - 4 + (int)i + GV_NSTY * 2) % GV_NSTY;
            int32_t y = 36 + (int32_t)i * 15;
            if (k == gu.sty) G_R(2, y - 2, 236, 14, GC_P2);
            G_T(8, y + 1, 2, GV_STN[k], k == gv.style ? GC_Y : k == gu.sty ? GC_T : GC_M);
            gv_fmt(b, "%d BPM", (int)GV_STBPM[k]);
            G_TR(234, y + 3, 1, b, GC_M);
        }
        G_TC(120, 174, 1, "MAINTIENS ARP 2 S : VALIDER + TOUT REGENERER", GC_C);
        break;
    case GP_GRYT: case GP_GMEL: {
        static const char *const KR[8] = {"1 NOUVEAU RYTHME", "2 VARIER", "3 NOUVELLE LIGNE", "4 VIDER LIGNE", "5 VIDER TOUT", "", "", ""};
        static const char *const KM[8] = {"1 NOUVELLE MELODIE", "2 VARIER", "3 TOUTES LES PISTES", "4 VIDER PISTE", "5 VIDER TOUT", "", "", ""};
        G_TC(120, 38, 3, GV_STN[gv.style], GC_A);
        for (i = 0; i < 5u; i++) G_T(14, 66 + (int32_t)i * 14, 2, gu.page == GP_GRYT ? KR[i] : KM[i], GC_T);
        {
            const uint8_t *p = gv_progp();
            if (p) for (i = 0; i < 4u; i++) {
                int32_t x = 14 + (int32_t)i * 54;
                int on = song.playing && gv_cix == i;
                G_R(x, 142, 50, 18, on ? GC_Y : GC_P2);
                G_TC(x + 25, 147, 2, gv_degname(p[i]), on ? GC_BG : GC_T);
            }
        }
        break; }
    case GP_FXD: case GP_ENV: case GP_FLT: case GP_LFO: case GP_OSC: case GP_SON: {
        uint8_t kn[4];
        gv_knobs(kn);
        G_TC(120, 40, 2, gu.page == GP_FXD ? "TOUTE LA BATTERIE" : gv_name(gv.sel), gu.page == GP_FXD ? GC_C : gv_col(gv.sel));
        for (i = 0; i < 4u; i++) {
            int32_t mn, mx, v, x = 22 + (int32_t)i * 56, h = 110;
            if (kn[i] == KN_NONE) continue;
            gv_krange(kn[i], &mn, &mx);
            v = mx > mn ? (gv_kget(kn[i]) - mn) * h / (mx - mn) : 0;
            G_R(x, 56, 24, h, GC_P);
            G_R(x, 56 + h - v, 24, v, gu.page == GP_FXD ? GC_C : gv_col(gv.sel));
        }
        if (gu.page == GP_LFO && t->lfd) {             /* the LFO's wave, moving */
            int32_t px = 0, py = 0;
            for (c = 0; c < 200u; c++) {
                int32_t ph = (int32_t)((c * 3u + fm1_ms / 8u) & 255u), y;
                y = t->lfw == 2 ? (ph < 128 ? 20 : -20) : t->lfw == 3 ? 20 - ph * 40 / 255 : (ph < 128 ? ph * 40 / 128 - 20 : 20 - (ph - 128) * 40 / 128);
                if (c) cv_line(px, py - gv_oy, 20 + (int32_t)c, 110 - y - gv_oy, GC_Y);
                px = 20 + (int32_t)c; py = 110 - y;
            }
        }
        break; }
    case GP_SCL:
        for (i = 0; i < 12u; i++) {
            int in = 0, k;
            for (k = 0; k < (int)gv_L(); k++) if (GV_SC[gv.scale % GV_NSC][k] == i) in = 1;
            G_R(1 + (int32_t)i * 20, 40, 18, 50, i == 0u ? GC_Y : in ? GC_A : GC_P2);
            G_TC(10 + (int32_t)i * 20, 80, 1, GV_NOTE[(gv.root + i) % 12u], in || !i ? GC_BG : GC_L);
        }
        gv_fmt(b, "%s %s", GV_NOTE[gv.root], GV_SCN[gv.scale % GV_NSC]);
        G_TC(120, 104, 2, b, GC_T);
        {
            const uint8_t *p = gv_progp();
            if (p) for (i = 0; i < 4u; i++) G_TC(36 + (int32_t)i * 56, 130, 2, gv_degname(p[i]), song.playing && gv_cix == i ? GC_Y : GC_A);
            else G_TC(120, 130, 1, "ACCORDS OFF", GC_M);
        }
        break;
    case GP_GLO:
        gv_fmt(b, "%d", (int)song.g[G_BPM]);
        G_TC(120, 40, 6, b, GC_Y);
        G_TC(120, 84, 1, "BPM", GC_M);
        gv_grid16(K_GLO, 96, 80, GC_A);
        break;
    case GP_FXR:
        G_TC(120, 36, 1, "LES EFFETS EN ROUTE (TOUCHE : LE COUPER)", GC_M);
        gv_grid16(K_FXR, 48, 132, GC_C);
        break;
    case GP_FXA: case GP_FXB: case GP_FXP: {
        static const char *const TG[3] = {"TOUT", "RYTHME", "MELODIE"};
        gv_fmt(b, "PADS SUR : %s%s", TG[gv_ptg % 3u], gu.lock ? "  VERROU" : "");
        G_TC(120, 36, 1, b, GC_Y);
        gv_grid16(K_PERF, 48, 132, GC_A);
        break; }
    case GP_MIX: case GP_LRR:
        G_TC(120, 36, 1, gv_name(gv.sel), gv_col(gv.sel));
        gv_grid16(K_LRR, 48, 132, gv_col(gv.sel));
        break;
    case GP_VIS: {                                     /* the style, the beat, the little orange monster dancing */
        uint32_t bt = song.playing ? (uint32_t)(gv_pos & 3u) : 0u, ph = song.playing ? (uint32_t)(gv_acc_s * 256u / (gv_stepn ? gv_stepn : 1u)) : 0u;
        int32_t hop = song.playing && bt == 0u ? (int32_t)(256u - ph) * 10 / 256 : 0, mx = 120, my = 150 - hop, k;
        G_TC(120, 36, 3, GV_STN[gv.style], GC_A);
        for (i = 0; i < 14u; i++) {
            int32_t h = gv_flash((int)i) ? 30 : 6, x = 4 + (int32_t)i * 17;
            G_R(x, 176 - h, 13, h, gv_col((int)i));
        }
        for (k = -1; k <= 1; k++) G_R(mx + k * 9 - 3, my + 12 + (bt & 1u ? (k == 0 ? -3 : 0) : (k ? -3 : 0)), 7, 12, GC_A);   /* the 3 legs */
        G_R(mx - 26, my - 4 - (bt == 1u ? 12 : 0), 8, 18, GC_A);                    /* the hands */
        G_R(mx + 18, my - 4 - (bt == 3u ? 12 : 0), 8, 18, GC_A);
        G_R(mx - 18, my - 40, 36, 54, GC_A);                                       /* the body */
        G_R(mx - 14, my - 46, 28, 8, GC_A);
        G_R(mx - 8, my - 32, 16, 16, GC_T); G_R(mx - 5, my - 29, 10, 10, RGB(0x7c, 0xc2, 0x42)); G_R(mx - 2, my - 26, 4, 4, GC_BG);   /* the eye */
        G_R(mx - 9, my - 8, 18, 5 + (song.playing && bt == 0u ? 3 : 0), GC_BG);  /* the mouth */
        G_R(mx - 22, my - 46, 4, 18, RGB(0x8a, 0x0d, 0x1e)); G_R(mx + 18, my - 46, 4, 18, RGB(0x8a, 0x0d, 0x1e));   /* headphones */
        G_R(mx - 20, my - 50, 40, 3, GC_L);
        break; }
    case GP_SAVE:
        G_TC(120, 70, 2, "BANQUE", GC_A);
        G_TC(120, 100, 1, "BATTERIE : PROJETS HORTATOR (GLO 16 > SAVE)", GC_M);
        G_TC(120, 116, 1, "MELODIES : VERSION SUIVANTE", GC_M);
        break;
    default: break;
    }
    if (gu.hold_b != NB && !gu.hold_done && fm1_ms - gu.hold_t0 > 250u) {
        int32_t p = (int32_t)((fm1_ms - gu.hold_t0) * 220u / 2000u);
        G_R(6, 150, 228, 26, GC_P);
        G_F(6, 150, 228, 26, GC_Y);
        G_R(10, 168, p > 220 ? 220 : p, 5, GC_Y);
        G_TC(120, 155, 1, gu.hold_b == B_HOME ? "MAINTIENS : NOUVELLE GENERATION" : gu.hold_b == B_ARP ? "MAINTIENS : STYLE + TOUT REGENERER" : "MAINTIENS : EFFETS A ZERO", GC_Y);
    } else if (fm1_ms < gu.toast_t) {
        G_R(6, 152, 228, 20, RGB(6, 16, 14));
        G_F(6, 152, 228, 20, GC_C);
        G_TC(120, 159, 1, gu.toast, GC_C);
    }
}
/* the whole screen (ui_draw.c, first thing): 1 = drawn by Groove */
static int gv_ui_draw(void)
{
    uint32_t s;
    if (!gv_on) return 0;
    if (!gu.dirty && fm1_ms - gu.last_draw < 100u) return 1;   /* 10 frames/s for the playhead and the flashes */
    gu.dirty = 0;
    gu.last_draw = fm1_ms;
    for (s = 0; s < 2u; s++) {
        gv_oy = (int32_t)s * 120;
        cv_begin(240, 120, GC_BG);
        gv_draw_head();
        gv_draw_body();
        gv_draw_foot();
        if (gu.flash && fm1_ms - gu.flash < 400u) {                  /* a long press done: the screen's edge flashes */
            G_F(0, 0, 240, 240, GC_Y); G_F(1, 1, 238, 238, GC_Y); G_F(2, 2, 236, 236, GC_A);
        }
        cv_blit(0, (uint32_t)gv_oy);
    }
    return 1;
}
/* the LEDs: the page's button, PLAY, the white keys showing what is on */
static int gv_ui_leds(void)
{
    uint8_t nl[FM1_NCOL] = {0}, nb[FM1_NCOL] = {0};
    uint32_t j;
    if (!gv_on) return 0;
    led_put(nl, panel.btn[gu.btn], 1);
    led_put(nl, panel.btn[B_PLAY], song.playing && ((song.tick / 64u) & 1u) == 0u);
    led_put(nl, panel.btn[B_REC], gv_recq);
    for (j = 0; j < 16u; j++) {
        int on = 0;
        switch (gu.page) {
        case GP_LIVE: on = j < 14u ? gv_heard((int)j) : !((gv.gmute >> (j - 14u)) & 1u); break;
        case GP_LFX: on = j < 8u ? j == gu.fxt : gv_fxon((gu.fxpg >> 1) * 8u + j - 8u) && FE_OFF[(gu.fxpg >> 1) * 8u + j - 8u] >= 0; break;
        case GP_LRYT: on = j < 8u ? !trk[GV_ROWTRK[j]].p[P_MUTE] : (gv.solo >> (j - 8u)) & 1u; break;
        case GP_LMEL: on = j < 6u ? !gv.t[j].mute : j == 7u || j == 14u; break;
        case GP_RYT: on = gv_rt()->step[j].on || (song.playing && gv_pos == j); break;
        case GP_MEL: on = gv_tr()->st[j].n >= 0 || (song.playing && gv_pos == j); break;
        case GP_STY: on = j == gu.sty || j == gv.style; break;
        case GP_GRYT: case GP_GMEL: on = j < 5u; break;
        case GP_SCL: on = j == gv.root; break;
        case GP_GLO: on = j == 15u || j == 0u || (j >= 2u && j < 8u && gv.algo == j - 2u); break;
        case GP_FXR: on = gv_fxr_on(j); break;
        case GP_FXA: case GP_FXB: case GP_FXP: on = gv_perf_on(j); break;
        case GP_MIX: case GP_LRR: on = gv_lrr_on(j); break;
        case GP_VIS: on = j < 6u; break;
        default: break;
        }
        led_put(nl, 14u + STEP_KEY[j], on);
    }
    for (j = 0; j < 27u; j++) if ((fm1_in.notes >> j) & 1u) led_put(nl, 14u + j, 1);
    leds_out(nl, nb, 0);
    return 1;
}
/* the groove at boot: a style, its rhythm, a melody on every track (until saved grooves come back) */
static void gv_boot(void)
{
    uint32_t i;
    static const uint8_t VOL[GV_NT] = {100, 60, 70, 50, 80, 50}, OCT[GV_NT] = {2, 4, 3, 5, 4, 5}, WAV[GV_NT] = {GW_SAW, GW_SQR, GW_SQR, GW_SAW, GW_SAW, GW_TRI},
                         CUT[GV_NT] = {50, 70, 50, 64, 64, 80}, RES[GV_NT] = {83, 38, 83, 45, 13, 83}, DLY[GV_NT] = {13, 51, 13, 38, 13, 76},
                         REV[GV_NT] = {6, 25, 6, 32, 89, 64};
    gv_init_tables();
    gv_rng ^= fm1_ms * 2654435761u;
    gv.magic = GV_MAGIC;
    gv.root = 9; gv.scale = 0; gv.prog = 1; gv.style = 0; gv.sel = 8; gv.oct = 0;
    for (i = 0; i < GV_NT; i++) {
        gv_trk_t *t = &gv.t[i];
        t->vol = VOL[i]; t->oct = OCT[i]; t->wave = WAV[i]; t->cut = CUT[i]; t->res = RES[i];
        t->env = 76; t->dec = 38; t->dly = DLY[i]; t->rev = REV[i]; t->lfr = 38;
        gv_mel_clear(t);
    }
    drum_set_model(&trk[7], DM_CONGA);
    memset(gu.byp, 0xFF, sizeof gu.byp);
    gv.mf = 127;                                      /* FILTRE DJ open */
    gv_set_style(gv.style);
    gv_gen_rhythm();
    gv_gen_mel(0);
    gv_gen_mel(1);
}
