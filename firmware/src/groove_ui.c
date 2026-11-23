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
       GP_SCL, GP_ENV, GP_FLT, GP_LFO, GP_OSC, GP_MIX, GP_LRR, GP_SON, GP_GLO, GP_SAVE,
       GP_SONR, GP_SEDIT, GP_EMIDI, GP_MIDI, GP_BKR, GP_BKM, GP_EFA, GP_EFB, GP_EFC, GP_EFD, GP_MIXR, GP_EFE, GP_EFF, GP_EFG, GP_LIBR, GP_GSET, GP_N };   /* (added last: the ranges above stay) */
static const char *const GP_T[GP_N] = {"LIVE", "LIVE RYT", "LIVE MEL", "FX LIVE", "RYTHME", "MELODIE", "STYLES", "VISU", "GEN RYT",
                                       "GEN MEL", "FX RECAP", "FX FILTRE", "FX ESPACE", "FX BATT", "FX PERF", "GAMME", "ENV", "FILTRE",
                                       "LFO", "OSC", "MIX", "LOOP ROLL", "SON", "TEMPO", "BANQUE", "SON REGL.", "EDITER", "MIDI PISTES",
                                       "MIDI", "RYTHMES", "MELODIES", "FX A", "FX B", "FX C", "FX D", "MIXEUR", "FX E", "FX F", "FX G", "BASE RYT", "REGLAGES"};
#define GV_EFP(p) (((p) >= GP_EFA && (p) <= GP_EFD) || ((p) >= GP_EFE && (p) <= GP_EFG))   /* ENV pages 5-11: FX LIVE's 28 effects, on the shown track */
/* LFO: its two pages, then the same 28 effects (HOME: FX LIVE comes last, and HOME again goes through its pages A..G) */
static const uint8_t GV_LFOP[9] = {GP_MIX, GP_LRR, GP_EFA, GP_EFB, GP_EFC, GP_EFD, GP_EFE, GP_EFF, GP_EFG};
/* EDIT: the sound, then the rhythm base, the kept melodies and the generator's settings */
static const uint8_t GV_EDITP[7] = {GP_SON, GP_SONR, GP_SEDIT, GP_EMIDI, GP_LIBR, GP_BKM, GP_GSET};
static void gv_store_load(void);                      /* (groove_store.c: the bank in flash) */
static void gv_store_poll(void);
static const uint8_t GV_ENVP[11] = {GP_ENV, GP_FLT, GP_LFO, GP_OSC, GP_EFA, GP_EFB, GP_EFC, GP_EFD, GP_EFE, GP_EFF, GP_EFG};
#define GV_PERFP(p) ((p) >= GP_FXA && (p) <= GP_FXP)   /* the FX pages with the pads (LOOP ROLL REVERSE held) */
/* button -> its pages (B_* of panel.c); 0xFF = none */
static const uint8_t GV_GRP[NB][5] = {
    [B_HOME] = {GP_LIVE, GP_LRYT, GP_LMEL, GP_MIXR, GP_LFX}, [B_SEQ] = {GP_RYT, GP_MEL, 0xFF, 0xFF, 0xFF},
    [B_ARP] = {GP_STY, GP_VIS, GP_GRYT, GP_GMEL, 0xFF},  [B_FX] = {GP_FXR, GP_FXA, GP_FXB, GP_FXD, GP_FXP},
    [B_SCL] = {GP_SCL, 0xFF, 0xFF, 0xFF, 0xFF},          [B_ENV] = {GP_ENV, GP_FLT, GP_LFO, GP_OSC, 0xFF},
    [B_LFO] = {GP_MIX, GP_LRR, 0xFF, 0xFF, 0xFF},        [B_EDIT] = {GP_SON, GP_SONR, GP_SEDIT, GP_EMIDI, 0xFF},   /* (EDIT: GV_EDITP) */
    [B_GLO] = {GP_GLO, GP_MIDI, 0xFF, 0xFF, 0xFF},       [B_SAVE] = {GP_SAVE, GP_BKR, GP_BKM, GP_LIBR, 0xFF},
    [B_PLAY] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, [B_REC] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    [B_OCTDN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, [B_OCTUP] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

static struct {
    uint8_t page, btn, cur, sty, kp, dirty, init;
    uint8_t hold_b;                 /* the button held for a long press (NB none) */
    uint32_t hold_t0, last_draw, flash, toast_t;
    uint32_t intro_t;               /* the power-on intro began then (0: over) */
    uint8_t hold_done;
    char toast[32];
    int8_t deg;                     /* the last degree played (a new melody step takes it) */
    uint8_t fxt, fxpg;              /* FX LIVE: the target (0 rhythm, 1 all the melody, 2..7 a track), the page A..D */
    int8_t fxd;
    uint32_t fxa;
    int8_t byp[8][28];             /* FX LIVE: the value an effect had before its key cut it (-1 = not cut) */
    uint8_t mrow;                   /* GLO MIDI: the list's cursor */
    uint8_t lcur;                   /* BASE RYT: the motif under the cursor (the base's, then the bank's 8) */
    uint8_t gcur;                   /* GLO: the option under the cursor (PRESETS up/down, ALGORITHM left/right) */
    uint8_t ppage, pbtn, snp, sns;  /* the page before this button's (OCT+ = back); EDIT SON's snapshot: its page / track */
    uint8_t grp;                    /* SELECT past the 14 tracks (LIVE, MIX): 1 everything, 2 all the rhythm, 3 all the melody */
    uint8_t lock;                   /* FX pads: VERROU (a pad stays on when let go) */
    uint32_t tap[4];                /* TAP TEMPO: the last taps */
    uint32_t wprev;                 /* the white keys held (their release: the pads) */
    uint16_t rb[8][8];              /* SAVE: 8 rhythms (8 rows of 16 steps), in RAM until power-off */
    gv_step_t mb[8][16];            /* SAVE: 8 melodies (one track's 16 steps) */
    uint8_t rbu, mbu, rcur, mcur, bmode;   /* used slots (bits), the last loaded, CHARGER / ENREGISTRER / EFFACER */
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
       KN_DVOL, KN_DDIST, KN_DDLY, KN_DREV, KN_BMODE, KN_KOCT,
       KN_E2, KN_E3, KN_E4, KN_E5, KN_GCH, KN_GCLK, KN_GSYNC,
       KN_AVOL, KN_ADIST, KN_ADLY, KN_AREV, KN_MCH, KN_MOUT, KN_EQL, KN_EQM, KN_EQH, KN_RSRC, KN_MSRC, KN_N };
static const char *const KN_L[KN_N] = {"", "VOLUME", "VOLUME", "SWING", "AUTO", "AUTO", "STYLE", "RYTHME", "PITCH", "BPM",
    "AUTO RYT", "ACCORDS", "TONIQUE", "GAMME", "CUTOFF", "RESO", "TYPE", "ENV", "ATTAQUE", "DECAY", "GLIDE", "VITESSE",
    "PROFOND.", "FORME", "CALAGE", "ONDE", "OCTAVE", "DISTO", "DELAY", "REVERB", "DLY MIX", "DLY TEMPS", "REV TAILLE",
    "REV DAMP", "NOTE", "DUREE", "ACC SLIDE", "DECALAGE", "LIGNE", "MODELE", "ACCORD", "DECAY", "DISTO", "REVERB", "", "", "", "",
    "FILTRE DJ", "COUPE-HAUT", "COUPE-BAS", "RESO", "DISTO", "ECHO", "REVERB", "CIBLE", "VERROU", "LOOP", "ROLL", "LOOP", "ROLL",
    "SENS", "VOLUME", "DISTO", "DELAY", "REVERB", "MODE", "OCT CLAV", "", "", "", "", "CANAL BATT", "HORLOGE", "ENVOI HORL", "VOLUME", "DISTO", "DELAY", "REVERB", "CANAL", "SORTIE", "GRAVES", "MEDIUMS", "AIGUS", "SRC RYTHME", "SRC MELOD."};
static gv_step_t *gv_stp(void) { return &gv_tr()->st[gu.cur & 15u]; }
static track_t *gv_rt(void) { return &trk[GV_ROWTRK[gv.sel & 7]]; }
/* ------------------------------------------------------------- FX LIVE: 16 effects on 4 pages, on a target --- */
enum { FE_CUT, FE_RES, FE_DIST, FE_DLY, FE_REV, FE_LFD, FE_LFR, FE_VOL, FE_CHO, FE_RL, FE_SK, FE_RV, FE_LP, FE_CR, FE_GT, FE_HF, FE_PAN, FE_SC, FE_TR, FE_TS, FE_AP, FE_VB, FE_PS, FE_RM, FE_WF, FE_ST, FE_FO, FE_NR, FE_N };
static const char *const FE_L[FE_N] = {"FILTRE", "RESO", "DISTO", "ECHO", "REVERB", "WOBBLE", "VITESSE", "VOLUME",
                                       "CHORUS", "ROLL", "PAS SAUTES", "REVERSE", "LOOP", "BITCRUSH", "GATE", "DEMI-TEMPO",
                                       "PAN", "SIDECHAIN", "TREMOLO", "TAPE STOP",
                                       "AUTO-PAN", "VIBRATO", "PITCH", "RING MOD", "WAVEFOLD", "SATURATION", "VOYELLE", "NOISE"};
static const char *const FE_LD[FE_N] = {0, 0, 0, 0, 0, "ENV FILT", "DEC FILT", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};   /* the drums' */
static const int16_t FE_OFF[FE_N] = {127, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 64, 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0};   /* cut = this; -1 no cut */
static const uint8_t FE_RLO[4] = {0, 2, 3, 4}, FE_LPO[5] = {0, 1, 2, 4, 8};
static const char *const FXT_N[8] = {"RYTHME", "MELODIE", "BASSE", "BASSE 2", "SYNTHE", "SYNTHE 2", "NAPPE", "FX"};
static const uint8_t FXT_TRK[8] = {0, 0, 0, 2, 1, 3, 4, 5};   /* targets 2..7 -> gv.t[] */
static const char *const FXPN[7] = {"A", "B", "C", "D", "E", "F", "G"};
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
    case FE_LP: return &t->lp; case FE_CR: return &t->cr; case FE_GT: return &t->gt; case FE_PAN: return &t->pan;
    case FE_SC: return &t->sc; case FE_TR: return &t->tr; case FE_TS: return &t->ts; case FE_AP: return &t->ap;
    case FE_VB: return &t->vb; case FE_RM: return &t->rm; case FE_WF: return &t->wf; case FE_ST: return &t->sat;
    case FE_FO: return &t->fo; default: return &t->hf;
    }
}
static int32_t gv_tget(gv_trk_t *t, uint32_t e) { return e == FE_PS ? t->ps + 12 : *gv_tfld(t, e); }   /* (PITCH: -12..12 kept, 0..24 shown) */
static void gv_tset(gv_trk_t *t, uint32_t e, int32_t v) { if (e == FE_PS) t->ps = (int8_t)(v - 12); else *gv_tfld(t, e) = (uint8_t)v; }
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
    case FE_PAN: return gv_clamp(t->p[P_PAN] + 64, 0, 127);
    case FE_PS: return 12;
    case FE_TR: return gv.dtr; case FE_AP: return gv.dap; case FE_RM: return gv.drm; case FE_WF: return gv.dwf;   /* (the 8 rows together) */
    case FE_ST: return gv.dsat; case FE_FO: return gv.dfo;
    case FE_SC: case FE_TS: case FE_VB: return 0;   /* (the melody's only: the kick is what ducks, the drums are not slowed) */
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
    case FE_PAN: t->p[P_PAN] = (int16_t)gv_clamp(v - 64, -64, 63); break;
    case FE_TR: gv.dtr = (uint8_t)v; break; case FE_AP: gv.dap = (uint8_t)v; break; case FE_RM: gv.drm = (uint8_t)v; break;
    case FE_WF: gv.dwf = (uint8_t)v; break; case FE_ST: gv.dsat = (uint8_t)v; break; case FE_FO: gv.dfo = (uint8_t)v; break;
    case FE_SC: case FE_TS: case FE_VB: case FE_PS: break;
    case FE_LP: gv.rlp[r] = (uint8_t)v; break; default: gv.rhf[r] = (uint8_t)v; break;
    }
}
static int8_t gv_fxrow = -1;                         /* the ENV FX pages on a drum row: that row only (-1: all the rhythm) */
static int32_t gv_fxget(uint32_t e)                   /* the target's value: the mean of its tracks */
{
    uint32_t k, n = 0;
    int32_t a = 0;
    if (e == FE_NR) return gv.nr;                     /* (one riser for the whole mix, whatever the target) */
    if (gu.fxt == 0u && gv_fxrow >= 0) { a = gv_dget((uint32_t)gv_fxrow, e); n = 1; }
    else if (gu.fxt == 0u) { for (k = 0; k < 8u; k++) a += gv_dget(k, e); n = 8; }
    else if (gu.fxt == 1u) { for (k = 0; k < GV_NT; k++) a += gv_tget(&gv.t[k], e); n = GV_NT; }
    else { a = gv_tget(&gv.t[FXT_TRK[gu.fxt]], e); n = 1; }
    return (a + (int32_t)n / 2) / (int32_t)n;
}
static void gv_fxset(uint32_t e, int32_t v)
{
    uint32_t k;
    if (e == FE_NR) { gv.nr = (uint8_t)v; return; }
    if (gu.fxt == 0u && gv_fxrow >= 0) gv_dset((uint32_t)gv_fxrow, e, v);
    else if (gu.fxt == 0u) for (k = 0; k < 8u; k++) gv_dset(k, e, v);
    else if (gu.fxt == 1u) for (k = 0; k < GV_NT; k++) gv_tset(&gv.t[k], e, v);
    else gv_tset(&gv.t[FXT_TRK[gu.fxt]], e, v);
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
    int8_t *b = &gu.byp[gu.fxt][e];
    char m[32];
    if (e >= FE_N) return;
    if (FE_OFF[e] < 0) { gv_say("REGLE-LE AU KNOB"); return; }
    if (*b >= 0) { gv_fxset(e, *b); *b = -1; gv_fmt(m, "%s RETABLI", FE_L[e]); }
    else { *b = (int8_t)gv_fxget(e); gv_fxset(e, FE_OFF[e]); gv_fmt(m, "%s COUPE", FE_L[e]); }
    gv_say(m);
}
/* the groups (SELECT past the last track): a knob moves every member by the same amount, its value = their mean */
static int gv_grp_on(void) { return gu.grp && (gu.page == GP_LIVE || gu.page == GP_MIX || gu.page == GP_MIXR); }
static const char *const GV_GRPN[4] = {"", "TOUT", "TOUT RYTHME", "TOUTE MELODIE"};
static uint16_t gv_grpc(void) { return gu.grp == 2u ? GC_C : gu.grp == 3u ? GC_A : GC_T; }
static int gv_grp_has(int p) { return gu.grp == 1u || (gu.grp == 2u ? p < 8 : p >= 8); }
static int32_t gv_grp_val(int p, uint32_t k)          /* k: 0 volume 1 disto 2 delay 3 reverb */
{
    static const uint8_t PP[4] = {P_LEVEL, P_DIST, P_DLY, P_REV};
    const gv_trk_t *t = &gv.t[(p - 8) & 7];
    if (p < 8) return trk[GV_ROWTRK[p]].p[PP[k]];
    return k == 0u ? t->vol : k == 1u ? t->dist : k == 2u ? t->dly : t->rev;
}
static int32_t gv_grp_get(uint32_t k) { int32_t a = 0, n = 0; int p; for (p = 0; p < 14; p++) if (gv_grp_has(p)) { a += gv_grp_val(p, k); n++; } return n ? (a + n / 2) / n : 0; }
static void gv_grp_add(uint32_t k, int32_t dv)
{
    static const uint8_t PP[4] = {P_LEVEL, P_DIST, P_DLY, P_REV};
    int p;
    for (p = 0; p < 14; p++) {
        int32_t v;
        if (!gv_grp_has(p)) continue;
        v = gv_clamp(gv_grp_val(p, k) + dv, 0, 127);
        if (p < 8) trk[GV_ROWTRK[p]].p[PP[k]] = (int16_t)v;
        else { gv_trk_t *t = &gv.t[p - 8]; if (k == 0u) t->vol = (uint8_t)v; else if (k == 1u) t->dist = (uint8_t)v; else if (k == 2u) t->dly = (uint8_t)v; else t->rev = (uint8_t)v; }
    }
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
    case KN_FTYPE: *mx = 2; break;
    case KN_WAVE: *mx = 3; break;
    case KN_LFW: case KN_SACC: case KN_SOFF: *mx = 3; break;
    case KN_OCT: *mn = 1; *mx = 6; break;
    case KN_SNOTE: *mx = 2 * (int32_t)gv_L(); break;
    case KN_SLEN: *mn = 1; *mx = 64; break;
    case KN_ROW: *mx = 7; break;
    case KN_MODEL: *mx = NMODELS - 1; break;
    case KN_FX0: case KN_FX1: case KN_FX2: case KN_FX3: { uint32_t e = gu.fxpg * 4u + id - KN_FX0, c = gv_fxn(e); *mx = c ? (int32_t)c - 1 : e == FE_PS ? 24 : 127; break; }
    case KN_PTG: case KN_BMODE: *mx = 2; break;
    case KN_KOCT: case KN_RSRC: *mx = 3; break;
    case KN_MSRC: *mx = 1; break;
    case KN_MCH: *mn = 1; *mx = 16; break;
    case KN_MOUT: *mx = 2; break;
    case KN_E2: case KN_E3: case KN_E4: case KN_E5: { const param_desc_t *dd = track_desc(gv_rt(), P_E2 + id - KN_E2); *mn = dd->min; *mx = dd->max; break; }
    case KN_GCH: *mn = GP[G_DRCH].min; *mx = GP[G_DRCH].max; break;
    case KN_GCLK: *mx = GP[G_CLOCK].max; break;
    case KN_GSYNC: *mx = GP[G_SYNC].max; break;
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
    case KN_XD: return gv.xd; case KN_XR: return gv.xr; case KN_PTG: return gv_ptg; case KN_LOCK: return gu.lock; case KN_BMODE: return gu.bmode; case KN_KOCT: return gv.oct;
    case KN_E2: case KN_E3: case KN_E4: case KN_E5: return r->p[P_E2 + id - KN_E2];
    case KN_AVOL: case KN_ADIST: case KN_ADLY: case KN_AREV: return gv_grp_get(id - KN_AVOL);
    case KN_EQL: case KN_EQM: case KN_EQH: return gv.eq[id - KN_EQL];
    case KN_RSRC: return gv.rsrc; case KN_MSRC: return gv.msrc;
    case KN_MCH: return t->mch; case KN_MOUT: return t->mo;
    case KN_GCH: return song.g[G_DRCH]; case KN_GCLK: return song.g[G_CLOCK]; case KN_GSYNC: return song.g[G_SYNC];
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
    case KN_PTG: gv_ptg = (uint8_t)v; break; case KN_LOCK: gu.lock = (uint8_t)v; break; case KN_BMODE: gu.bmode = (uint8_t)v; break; case KN_KOCT: gv.oct = (uint8_t)v; break;
    case KN_E2: case KN_E3: case KN_E4: case KN_E5: r->p[P_E2 + id - KN_E2] = (int16_t)v; break;
    case KN_AVOL: case KN_ADIST: case KN_ADLY: case KN_AREV: gv_grp_add(id - KN_AVOL, v - gv_grp_get(id - KN_AVOL)); break;
    case KN_EQL: case KN_EQM: case KN_EQH: gv.eq[id - KN_EQL] = (uint8_t)v; break;
    case KN_RSRC: gv.rsrc = (uint8_t)v; break; case KN_MSRC: gv.msrc = (uint8_t)v; break;
    case KN_MCH: t->mch = (uint8_t)v; break; case KN_MOUT: t->mo = (uint8_t)v; break;
    case KN_GCH: song.g[G_DRCH] = (int16_t)v; break; case KN_GCLK: song.g[G_CLOCK] = (int16_t)v; break; case KN_GSYNC: song.g[G_SYNC] = (int16_t)v; break;
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
    static const char *const FT[3] = {"PASSE-BAS", "PASSE-HAUT", "BANDE"}, *const WV[4] = {"SCIE", "CARRE", "TRI", "PIANO"},
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
    case KN_WAVE: gv_fmt(b, "%s", WV[v & 3]); return;
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
    case KN_E2: case KN_E3: case KN_E4: case KN_E5: case KN_GCLK: case KN_GSYNC: case KN_GCH: {
        const param_desc_t *dd = id == KN_GCLK ? &GP[G_CLOCK] : id == KN_GSYNC ? &GP[G_SYNC] : id == KN_GCH ? &GP[G_DRCH] : track_desc(gv_rt(), P_E2 + id - KN_E2);
        if (dd->fmt == F_ENUM && dd->names && v >= 0 && v <= dd->max) gv_fmt(b, "%s", dd->names[v]); else gv_fmt(b, "%d", (int)v);
        return; }
    case KN_RSRC: { static const char *const N[4] = {"GENER.", "BASE", "BASE+", "BANQUE"}; gv_fmt(b, "%s", N[v & 3]); return; }
    case KN_MSRC: gv_fmt(b, "%s", v ? "BANQUE" : "GENER."); return;
    case KN_EQL: case KN_EQM: case KN_EQH: if (v == 64) { gv_fmt(b, "0"); } else gv_fmt(b, "%+d", (int)((v - 64) * 12 / 64)); return;
    case KN_MCH: gv_fmt(b, "CH %d", (int)v); return;
    case KN_MOUT: { static const char *const O[3] = {"INTERNE", "MIDI USB", "LES 2"}; gv_fmt(b, "%s", O[v % 3]); return; }
    case KN_DJ: if (v == 64) { gv_fmt(b, "OUVERT"); } else if (v < 64) { gv_fmt(b, "GRAVE %d", (int)(64 - v)); } else gv_fmt(b, "AIGU %d", (int)(v - 64)); return;
    case KN_MF: if (v >= 127) { gv_fmt(b, "OUVERT"); } else gv_fmt(b, "%d%%", (int)(v * 100 / 127)); return;
    case KN_HP: if (!v) { gv_fmt(b, "OUVERT"); } else gv_fmt(b, "%d%%", (int)(v * 100 / 127)); return;
    case KN_PTG: { static const char *const T[3] = {"TOUT", "RYTHME", "MELODIE"}; gv_fmt(b, "%s", T[v % 3]); return; }
    case KN_BMODE: { static const char *const T[3] = {"CHARGER", "ENREGISTR.", "EFFACER"}; gv_fmt(b, "%s", T[v % 3]); return; }
    case KN_LOCK: case KN_TBW: gv_fmt(b, id == KN_TBW ? (v ? "ARRIERE" : "AVANT") : (v ? "ON" : "OFF")); return;
    case KN_RLP: case KN_TLP: if (v) { gv_fmt(b, "%d PAS", (int)GV_LPV[v % 7]); } else gv_fmt(b, "OFF"); return;
    case KN_RRL: case KN_TRL: if (v) { gv_fmt(b, "X%d", (int)GV_RLV[v & 3]); } else gv_fmt(b, "OFF"); return;
    case KN_FX0: case KN_FX1: case KN_FX2: case KN_FX3: {
        uint32_t e = gu.fxpg * 4u + id - KN_FX0;
        if (e == FE_RL) { if (v) { gv_fmt(b, "X%d", (int)FE_RLO[v & 3]); } else gv_fmt(b, "OFF"); return; }
        if (e == FE_LP) { if (v) { gv_fmt(b, "%d PAS", (int)FE_LPO[v % 5]); } else gv_fmt(b, "OFF"); return; }
        if (e == FE_RV || e == FE_HF) { gv_fmt(b, v ? "ON" : "OFF"); return; }
        if (e == FE_CUT && v >= 127) { gv_fmt(b, "OUVERT"); return; }
        if (e == FE_PAN) { if (v == 64) gv_fmt(b, "CENTRE"); else gv_fmt(b, v < 64 ? "G %d" : "D %d", (int)gv_abs(v - 64)); return; }
        if (gu.fxt == 0u && (e == FE_SC || e == FE_TS || e == FE_VB || e == FE_PS)) { gv_fmt(b, "MELODIE"); return; }   /* (they need the note: not for the drums) */
        if (e == FE_PS) { gv_fmt(b, "%+d", (int)v - 12); return; }
        if (e == FE_FO) { static const char *const VW[5] = {"A", "E", "I", "O", "U"}; gv_fmt(b, "%s", v ? VW[gv_clamp((v - 1) * 5 / 127, 0, 4)] : "OFF"); return; }
        gv_fmt(b, "%d%%", (int)(v * 100 / 127)); return; }
    default: gv_fmt(b, "%d%%", (int)(v * 100 / 127)); return;
    }
}
/* the page's 4 knobs */
static void gv_knobs(uint8_t *k)
{
    int d = gv_isd();
    if (GV_EFP(gu.page)) {                             /* the FX of the shown track: FX LIVE's page and target follow */
        static const uint8_t TT[GV_NT] = {2, 4, 3, 5, 6, 7};
        gu.fxpg = (uint8_t)(gu.page >= GP_EFE ? 4u + gu.page - GP_EFE : gu.page - GP_EFA);
        if (d) { gu.fxt = 0; gv_fxrow = (int8_t)(gv.sel & 7); } else { gu.fxt = TT[(gv.sel - 8) % GV_NT]; gv_fxrow = -1; }
    } else gv_fxrow = -1;
    k[0] = k[1] = k[2] = k[3] = KN_NONE;
#define K4(a, b, c, e) do { k[0] = a; k[1] = b; k[2] = c; k[3] = e; } while (0)
    switch (gu.page) {                                 /* = the web app's pages (fm1.js PAGES) */
    case GP_MIXR: K4(gu.grp ? KN_AVOL : d ? KN_RVOL : KN_TVOL, KN_EQL, KN_EQM, KN_EQH); break;
    case GP_LIVE: if (gu.grp) { K4(KN_AVOL, KN_DJ, KN_SWING, KN_AREV); break; }
        K4(d ? KN_RVOL : KN_TVOL, KN_DJ, KN_SWING, gu.kp ? KN_KOCT : d ? KN_RAR : KN_TAR); break;
    case GP_LRYT: K4(KN_RVOL, KN_RAR, KN_STYLE, gu.kp ? KN_KOCT : KN_GMR); break;
    case GP_LMEL: K4(KN_TVOL, KN_TAR, KN_PITCH, gu.kp ? KN_KOCT : KN_WAVE); break;
    case GP_LFX: case GP_EFA: case GP_EFB: case GP_EFC: case GP_EFD: case GP_EFE: case GP_EFF: case GP_EFG: K4(KN_FX0, KN_FX1, KN_FX2, KN_FX3); break;
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
    case GP_MIX: if (gu.grp) { K4(KN_AVOL, KN_ADIST, KN_ADLY, KN_AREV); break; }
        if (d) K4(KN_RVOL, KN_RAR, KN_DVOL, KN_DREV); else K4(KN_TVOL, KN_DIST, KN_DLY, KN_REV); break;
    case GP_LRR: if (d) K4(KN_RLP, KN_RRL, KN_ROW, KN_RAR); else K4(KN_TLP, KN_TRL, KN_TBW, KN_TAR); break;
    case GP_SON: if (d) K4(KN_MODEL, KN_RTUNE, KN_RDEC, KN_RVOL); else K4(KN_WAVE, KN_OCT, KN_CUT, KN_DEC); break;
    case GP_GLO: K4(KN_BPM, KN_SWING, KN_STYLE, KN_AUTO); break;
    case GP_SAVE: case GP_BKR: case GP_BKM: K4(KN_BMODE, KN_STYLE, KN_AUTO, KN_NONE); break;
    case GP_LIBR: K4(KN_RSRC, KN_STYLE, KN_AUTO, KN_BPM); break;
    case GP_GSET: K4(KN_RSRC, KN_AUTO, KN_MSRC, KN_TAR); break;
    case GP_SONR: if (d) K4(KN_MODEL, KN_RTUNE, KN_RDEC, KN_RVOL); else K4(KN_WAVE, KN_OCT, KN_CUT, KN_DEC); break;
    case GP_SEDIT: if (d) K4(KN_E2, KN_E3, KN_E4, KN_E5); else K4(KN_RES, KN_ENV, KN_DIST, KN_LFD); break;
    case GP_EMIDI: if (!d) { K4(KN_MCH, KN_MOUT, KN_GCLK, KN_GSYNC); break; }   /* a melodic track: its channel, where it plays */
        /* fall through */
    case GP_MIDI: K4(KN_GCH, KN_GCLK, KN_GSYNC, KN_NONE); break;
    default: break;
    }
    if (!d && gu.page == GP_RYT) gv.sel = 0;
    if (d && (gu.page == GP_MEL || gu.page == GP_LMEL || gu.page == GP_GMEL || gu.page == GP_ENV || gu.page == GP_FLT ||
              gu.page == GP_LFO || gu.page == GP_OSC))
        gv.sel = 8;
}

/* ------------------------------------------------------------- REC: the knobs' moves, replayed in a loop --- */
/* REC + PLAY: a knob turned (any page: ENV, LFO, FX, MIX...) is written step by step into a lane; let go of REC and
 * the lanes replay over 4 bars. AM_N knobs at once; FX PERF: REJOUE (pause), EFFACER. In RAM until power-off. */
#define AM_N 6
#define AM_LEN 64
#define AM_NONE (-128)
typedef struct { uint8_t id, sel, fxt, fxpg, grp, touch; int8_t fxrow; int8_t v[AM_LEN]; } gv_am_t;   /* (a knob's value: -127..127) */
static gv_am_t gv_am[AM_N];
static uint8_t gv_am_on = 1, gv_am_last = 0xFF;
static int gv_am_ok(uint32_t id)                      /* the knobs worth recording: the sound's and the mix's */
{
    switch (id) {
    case KN_NONE: case KN_SNOTE: case KN_SLEN: case KN_SACC: case KN_SOFF: case KN_ROW: case KN_STYLE: case KN_SCALE: case KN_ROOT:
    case KN_PROG: case KN_BMODE: case KN_PTG: case KN_LOCK: case KN_GCH: case KN_GCLK: case KN_GSYNC: case KN_MCH: case KN_MOUT:
    case KN_MODEL: case KN_TAR: case KN_RAR: case KN_AUTO: case KN_KOCT: case KN_GMR: case KN_BPM: return 0;
    default: return 1;
    }
}
static int gv_am_any(void) { uint32_t k; for (k = 0; k < AM_N; k++) if (gv_am[k].id) return 1; return 0; }
static void gv_am_clear(void) { memset(gv_am, 0, sizeof gv_am); }
static gv_am_t *gv_am_find(uint32_t id, int make)     /* the lane of this knob as shown now (its track, its FX target) */
{
    uint32_t k;
    int fx = id >= KN_FX0 && id <= KN_FX3, ag = id >= KN_AVOL && id <= KN_AREV;
    for (k = 0; k < AM_N; k++) {
        gv_am_t *a = &gv_am[k];
        if (a->id == id && (fx ? a->fxt == gu.fxt && a->fxpg == gu.fxpg && a->fxrow == gv_fxrow : ag ? a->grp == gu.grp : a->sel == (uint8_t)gv.sel)) return a;
    }
    if (!make) return 0;
    for (k = 0; k < AM_N; k++) if (!gv_am[k].id) {
        gv_am_t *a = &gv_am[k];
        uint32_t i;
        a->id = (uint8_t)id; a->sel = (uint8_t)gv.sel; a->fxt = gu.fxt; a->fxpg = gu.fxpg; a->grp = gu.grp; a->fxrow = gv_fxrow; a->touch = 0;
        for (i = 0; i < AM_LEN; i++) a->v[i] = AM_NONE;
        return a;
    }
    return 0;
}
static int32_t gv_am_do(const gv_am_t *a, int set, int32_t v)   /* the lane's knob read / set as it was when recorded */
{
    int8_t sel = gv.sel, fr = gv_fxrow;
    uint8_t ft = gu.fxt, fp = gu.fxpg, gr = gu.grp;
    int32_t r = 0, mn, mx;
    fm1_irq_off();                                    /* (the keys' ISR reads gv.sel) */
    gv.sel = (int8_t)a->sel; gu.fxt = a->fxt; gu.fxpg = a->fxpg; gu.grp = a->grp; gv_fxrow = a->fxrow;
    if (set) { gv_krange(a->id, &mn, &mx); gv_kset(a->id, gv_clamp(v, mn, mx)); } else r = gv_kget(a->id);
    gv.sel = sel; gu.fxt = ft; gu.fxpg = fp; gu.grp = gr; gv_fxrow = fr;
    fm1_irq_on();
    return r;
}
static void gv_am_touch(uint32_t id)                  /* a knob turned by hand */
{
    gv_am_t *a;
    if (!gv_recq || !song.playing || !gv_am_ok(id)) { a = gv_am_find(id, 0); if (a) a->touch = 2; return; }   /* (2: the hand holds it this step) */
    a = gv_am_find(id, 1);
    if (!a) { gv_say("MOUVEMENTS PLEINS (FX PERF : EFFACER)"); return; }
    a->touch = 1;
}
static void gv_am_step(uint32_t p)                    /* a new step: write the knobs being recorded, replay the others */
{
    uint32_t k;
    for (k = 0; k < AM_N; k++) {
        gv_am_t *a = &gv_am[k];
        if (!a->id) continue;
        if (a->touch == 1 && gv_recq) a->v[p % AM_LEN] = (int8_t)gv_clamp(gv_am_do(a, 0, 0), -127, 127);
        else if (a->touch == 2) a->touch = 0;
        else if (gv_am_on && a->v[p % AM_LEN] != AM_NONE && gv_am_do(a, 0, 0) != a->v[p % AM_LEN]) { gv_am_do(a, 1, a->v[p % AM_LEN]); gu.dirty = 1; }
    }
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
    case 12: return gv_trk_any(FE_REV); case 13: return gv_trk_any(FE_LFD); case 14: return gv_am_any(); case 15: { uint32_t k; for (k = 0; k < 14u; k++) if (gv_fxr_on(k)) return 1; return 0; }
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
    case 14: gv_am_clear(); break;
    case 15: for (i = 0; i < 15u; i++) gv_fxr_cut(i); break;
    default: break;
    }
}
static int gv_perf_on(uint32_t j)
{
    if (j < 4u) return gv_plp == K_PLP[j];
    if (j < 7u) return gv_prl == j - 2u;
    if (j == 7u) return gv_prv != 0u;
    if (j < 11u) return gv_ptg == j - 8u;
    return j == 11u ? gu.lock : j == 12u ? gv_recq : j == 13u ? gv_am_on && gv_am_any() : 0;
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
    else if (j == 12u) { gv_recq ^= 1u; gv_say(gv_recq ? "REC : NOTES ET KNOBS S'ENREGISTRENT" : "REC COUPE"); }
    else if (j == 13u) { gv_am_on ^= 1u; gv_say(gv_am_on ? "MOUVEMENTS : LECTURE" : "MOUVEMENTS : PAUSE"); }
    else if (j == 14u) { gv_am_clear(); gv_say("MOUVEMENTS EFFACES"); }
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

/* ------------------------------------------------------------- PRESETS on LIVE: the row's family of sounds --- */
/* drums: Hortator's factory presets whose voice is of the row's family (the web app's folders: KICK, SNARE, CLAP...) */
static const uint8_t GV_FAM[8][8] = {
    {DM_KTRX, DM_KPUNC, DM_KBOOM, DM_K909, DM_K808, DM_BDEFM, DM_B2TRX, 0xFF},   /* KICK */
    {DM_SDTRX, DM_SSNAP, DM_S909, DM_S808, DM_SCRAK, DM_SDEFM, 0xFF, 0xFF},       /* SNARE */
    {DM_CPTRX, DM_CPEFM, DM_C909, DM_C808, 0xFF, 0xFF, 0xFF, 0xFF},               /* CLAP */
    {DM_HATC, DM_CHTRX, DM_HHEFM, DM_HNOIS, DM_HMETL, 0xFF, 0xFF, 0xFF},          /* HAT F. */
    {DM_HATO, DM_OHTRX, DM_CYMB, DM_CYTRX, DM_CYEFM, DM_HMETL, 0xFF, 0xFF},       /* HAT O. (and the cymbals) */
    {DM_CONGA, DM_CLAVE, DM_COWB, DM_CBTRX, DM_CBEFM, DM_MEMB, DM_FM, DM_XCTRX},  /* PERC */
    {DM_TOM, DM_XTTRX, DM_XTEFM, DM_MEMB, DM_ACID, 0xFF, 0xFF, 0xFF},             /* LOW */
    {DM_RIM, DM_RSTRX, DM_RSEFM, DM_CLAVE, 0xFF, 0xFF, 0xFF, 0xFF}};              /* RIM */
static int gv_infam(uint32_t row, uint32_t model) { uint32_t k; for (k = 0; k < 8u; k++) if (GV_FAM[row][k] == model) return 1; return 0; }
/* melody: the synth's sounds, each for the tracks it suits (bit GT_*) */
typedef struct { const char *n; uint8_t ty, wave, oct, cut, res, env, att, dec, lfd, lfr, dist; } gv_snd_t;
static const gv_snd_t GV_SND[] = {
    {"SUB", 1u << GT_BASS, GW_TRI, 2, 40, 10, 20, 0, 50, 0, 38, 0},
    {"ACID", 1u << GT_BASS, GW_SAW, 2, 45, 100, 100, 0, 30, 0, 38, 20},
    {"REESE", 1u << GT_BASS, GW_SAW, 2, 55, 30, 30, 0, 70, 40, 20, 10},
    {"PLUCK BASS", 1u << GT_BASS, GW_SQR, 2, 50, 60, 80, 0, 20, 0, 38, 0},
    {"DUB", 1u << GT_BASS, GW_TRI, 2, 35, 20, 40, 10, 90, 20, 10, 30},
    {"LEAD SCIE", 1u << GT_SYNTH, GW_SAW, 4, 80, 40, 60, 0, 50, 0, 38, 10},
    {"CARRE", 1u << GT_SYNTH, GW_SQR, 4, 70, 30, 50, 0, 40, 0, 38, 0},
    {"PLUCK", (1u << GT_SYNTH) | (1u << GT_FX), GW_SAW, 5, 60, 70, 100, 0, 15, 0, 38, 0},
    {"WOBBLE", (1u << GT_SYNTH) | (1u << GT_BASS), GW_SAW, 3, 50, 90, 30, 0, 60, 100, 70, 20},
    {"CLOCHE", (1u << GT_SYNTH) | (1u << GT_FX), GW_TRI, 5, 110, 20, 40, 0, 25, 0, 38, 0},
    {"PAD", 1u << GT_PAD, GW_SAW, 4, 64, 45, 30, 90, 110, 30, 15, 0},
    {"CORDES", 1u << GT_PAD, GW_SAW, 4, 75, 20, 20, 70, 100, 20, 30, 0},
    {"PAD SOMBRE", 1u << GT_PAD, GW_TRI, 3, 40, 60, 20, 100, 120, 40, 10, 0},
    {"AIR", (1u << GT_PAD) | (1u << GT_FX), GW_SQR, 5, 100, 80, 10, 110, 127, 60, 20, 0},
    {"LASER", 1u << GT_FX, GW_SQR, 5, 90, 110, 127, 0, 20, 0, 38, 40},
    {"SIRENE", 1u << GT_FX, GW_SAW, 4, 70, 70, 20, 30, 80, 127, 50, 0},
    {"PIANO", (1u << GT_SYNTH) | (1u << GT_PAD) | (1u << GT_FX), GW_PIANO, 4, 62, 5, 55, 0, 70, 0, 38, 0},
    {"PIANO GRAVE", 1u << GT_BASS, GW_PIANO, 3, 55, 5, 50, 0, 80, 0, 38, 0}};
#define GV_NSND (sizeof GV_SND / sizeof GV_SND[0])
#define GV_NUSR 8u                                    /* MES SONS: the melodic sounds kept by hand (RAM: until power-off) */
static gv_snd_t gv_usr[GV_NUSR];                      /* (ty 0: free) */
static uint8_t gv_usrn;                               /* where the next one goes once all are taken */
static const gv_snd_t *gv_snd_get(uint32_t id) { return id < GV_NSND ? &GV_SND[id] : &gv_usr[(id - GV_NSND) % GV_NUSR]; }
static int preset_user_save(uint32_t slot, const track_t *t);
static uint8_t gv_sndcur[GV_NT];
static uint32_t gv_snd_ids(uint16_t *ids, uint32_t *cur)   /* the shown track's sounds (<= 64), which one it has (0xFFFF none) */
{
    uint32_t n = 0, k;
    *cur = 0xFFFFu;
    if (gv_isd()) {
        uint32_t r = (uint32_t)gv.sel & 7u, pre = preset_of(&trk[GV_ROWTRK[r]]);
        for (k = 0; k < PRE_NFACT && n < 56u; k++) if (gv_infam(r, PRE_FACTORY[k].model)) { if (k == pre) *cur = n; ids[n++] = (uint16_t)k; }
        for (k = 0; k < PRE_NUSER && n < 64u; k++)    /* then the ones kept by hand (Hortator's user bank: in flash) */
            if (upr_used(k) && gv_infam(r, upr.s[k].model)) { if (PRE_NFACT + k == pre) *cur = n; ids[n++] = (uint16_t)(PRE_NFACT + k); }
    } else {
        uint32_t i = (uint32_t)gv.sel - 8u, ty = GV_TTYPE[i];
        for (k = 0; k < GV_NSND; k++) if ((GV_SND[k].ty >> ty) & 1u) { if (k == gv_sndcur[i]) *cur = n; ids[n++] = (uint16_t)k; }
        for (k = 0; k < GV_NUSR; k++) if (gv_usr[k].ty) { if (GV_NSND + k == gv_sndcur[i]) *cur = n; ids[n++] = (uint16_t)(GV_NSND + k); }
    }
    return n;
}
static void gv_snd_name(uint32_t id, char *out)
{
    if (gv_isd()) preset_name_of(id, out);
    else if (id >= GV_NSND) gv_fmt(out, "MON SON %d", (int)(id - GV_NSND) + 1);
    else gv_fmt(out, "%s", GV_SND[id].n);
}
static void gv_preset_turn(int32_t s)                  /* PRESETS (LIVE) / SELECT (EDIT SON): the next / previous sound of the family */
{
    char m[32], nm[20];
    uint16_t ids[64];
    uint32_t cur, n = gv_snd_ids(ids, &cur), k;
    if (!n) return;
    k = cur == 0xFFFFu ? (s > 0 ? 0u : n - 1u) : (uint32_t)(((int32_t)cur + s) % (int32_t)n + (int32_t)n) % n;
    if (gv_isd()) {
        track_t *tr = &trk[GV_ROWTRK[gv.sel & 7]];
        preset_load(tr, ids[k]);
        drum_hit(tr, 100);
    } else {
        uint32_t i = (uint32_t)gv.sel - 8u;
        gv_trk_t *t = &gv.t[i];
        const gv_snd_t *d = gv_snd_get(ids[k]);
        gv_sndcur[i] = (uint8_t)ids[k];
        t->wave = d->wave; t->oct = d->oct; t->cut = d->cut; t->res = d->res; t->env = d->env; t->att = d->att; t->dec = d->dec;
        t->lfd = d->lfd; t->lfr = d->lfr; t->dist = d->dist;
    }
    gv_snd_name(ids[k], nm);
    gv_fmt(m, "%s %d/%d : %s", gv_name(gv.sel), (int)k + 1, (int)n, nm);
    gv_say(m);
}

/* SAVE on the SON pages: the sound as it is now, kept (a drum row: Hortator's user bank, in flash; a melodic track: MES SONS) */
static void gv_snd_keep(void)
{
    char m[32];
    if (gv_isd()) {
        track_t *tr = gv_rt();
        int sl = tr->pre >= PRE_NFACT && tr->pre < PRE_NID ? (int)(tr->pre - PRE_NFACT) : preset_user_free();
        if (sl < 0 || preset_user_save((uint32_t)sl, tr)) { gv_say("PLUS DE PLACE"); return; }
        tr->pre = (uint16_t)(PRE_NFACT + (uint32_t)sl);
        gv_fmt(m, "ENREGISTRE : %s", gv_name(gv.sel));
    } else {
        uint32_t i = (uint32_t)gv.sel - 8u, k;
        const gv_trk_t *t = &gv.t[i];
        gv_snd_t *d;
        if (gv_sndcur[i] >= GV_NSND) k = gv_sndcur[i] - GV_NSND;     /* one of MES SONS, changed: over itself */
        else { for (k = 0; k < GV_NUSR && gv_usr[k].ty; k++) { } if (k >= GV_NUSR) { k = gv_usrn; gv_usrn = (uint8_t)((gv_usrn + 1u) % GV_NUSR); } }
        d = &gv_usr[k % GV_NUSR];
        d->n = 0; d->ty = 0xFF; d->wave = t->wave; d->oct = t->oct; d->cut = t->cut; d->res = t->res; d->env = t->env; d->att = t->att;
        d->dec = t->dec; d->lfd = t->lfd; d->lfr = t->lfr; d->dist = t->dist;
        gv_sndcur[i] = (uint8_t)(GV_NSND + k);
        gv_fmt(m, "ENREGISTRE : MON SON %d", (int)k + 1);
    }
    gv_say(m);
}

/* EDIT SON: the sound the track had when the page opened (OCT- keeps the one heard, OCT+ puts this one back) */
static struct { int16_t p[P_COUNT]; uint16_t pre; gv_trk_t t; uint8_t snd; } gv_snap;
static void gv_snap_take(void)
{
    if (gv_isd()) { const track_t *tr = gv_rt(); memcpy(gv_snap.p, tr->p, sizeof gv_snap.p); gv_snap.pre = tr->pre; }
    else { gv_snap.t = *gv_tr(); gv_snap.snd = gv_sndcur[(gv.sel - 8) % GV_NT]; }
}
static void gv_snap_back(void)
{
    if (gv_isd()) {
        track_t *tr = gv_rt();
        if (tr->p[P_MODEL] != gv_snap.p[P_MODEL]) drum_set_model(tr, (uint32_t)gv_snap.p[P_MODEL]);
        memcpy(tr->p, gv_snap.p, sizeof gv_snap.p); tr->pre = gv_snap.pre;
    } else {
        gv_trk_t *t = gv_tr();
        gv_step_t st[16];
        memcpy(st, t->st, sizeof st);                 /* (the melody stays: only the sound goes back) */
        *t = gv_snap.t; memcpy(t->st, st, sizeof st);
        gv_sndcur[(gv.sel - 8) % GV_NT] = gv_snap.snd;
    }
}

/* ------------------------------------------------------------- GLO MIDI: a list (SELECT the row, PRESETS its value) --- */
#define GV_MROWS 15u                                  /* HORLOGE, ENVOI, CANAL BATTERIE, then each melodic track's SORTIE and CANAL */
static void gv_midi_row(uint32_t r, int32_t dv, char *lab, char *val)
{
    static const char *const O[3] = {"INTERNE", "MIDI USB", "LES 2"};
    if (r == 0u) { if (dv) song.g[G_CLOCK] = (int16_t)gv_clamp(song.g[G_CLOCK] + dv, 0, GP[G_CLOCK].max); gv_fmt(lab, "HORLOGE"); gv_fmt(val, "%s", GP[G_CLOCK].names[song.g[G_CLOCK]]); }
    else if (r == 1u) { if (dv) song.g[G_SYNC] = (int16_t)gv_clamp(song.g[G_SYNC] + dv, 0, GP[G_SYNC].max); gv_fmt(lab, "ENVOI HORLOGE"); gv_fmt(val, "%s", GP[G_SYNC].names[song.g[G_SYNC]]); }
    else if (r == 2u) { if (dv) song.g[G_DRCH] = (int16_t)gv_clamp(song.g[G_DRCH] + dv, 1, 16); gv_fmt(lab, "CANAL BATTERIE"); gv_fmt(val, "CH %d", (int)song.g[G_DRCH]); }
    else {
        gv_trk_t *t = &gv.t[((r - 3u) / 2u) % GV_NT];
        if ((r - 3u) & 1u) { if (dv) t->mch = (uint8_t)gv_clamp(t->mch + dv, 1, 16); gv_fmt(lab, "%s CANAL", GV_TRN[((r - 3u) / 2u) % GV_NT]); gv_fmt(val, "CH %d", (int)t->mch); }
        else { if (dv) t->mo = (uint8_t)gv_clamp(t->mo + dv, 0, 2); gv_fmt(lab, "%s SORTIE", GV_TRN[((r - 3u) / 2u) % GV_NT]); gv_fmt(val, "%s", O[t->mo % 3u]); }
    }
}

/* ------------------------------------------------------------- where a new rhythm / melody comes from (PRESETS on LIVE RYTHME / MELODIE) --- */
static const char *const GV_RSRC[4] = {"SOURCE : GENERATION", "SOURCE : BASE DU STYLE", "SOURCE : BASE, TOUS GENRES", "SOURCE : BANQUE (SAVE)"};
static const char *const GV_MSRC[2] = {"SOURCE : GENERATION", "SOURCE : BANQUE (SAVE)"};
static const char *const GV_RSRS[4] = {"GENER.", "BASE", "BASE+", "BANQUE"};
static int gv_src_rhythm(void)                        /* 1: a rhythm from the base / the bank is on; 0: the generator's turn */
{
    uint32_t r, k, n = 0, got = 0;
    char m[40];
    if (gv.rsrc == 1u || gv.rsrc == 2u) {             /* the base: one of the style's genres (or of every genre), another than now */
        for (k = 0; k < GV_NLIB; k++)
            if ((gv.rsrc == 2u || ((GV_LIB[k].sm >> (gv.style % GV_NSTY)) & 1u)) && k + 1u != gv.libk && gv_rnd(++n) == 0u) got = k;
        if (!n) { if (!gv.libk) return 0; got = gv.libk - 1u; }   /* (one motif only: it stays; none for this style: the generator) */
        for (r = 0; r < 8u; r++) { gv_rs_acc = GV_LIB[got].ac[r]; gv_rhythm_row_set(r, GV_LIB[got].on[r]); }
        gv_rs_acc = 0;
        gv.libk = (uint8_t)(got + 1u);
        gv_fmt(m, "BASE : %s %d", GV_LGN[GV_LIB[got].g % GV_NLG], (int)GV_LIB[got].bpm);
        gv_say(m);
        return 1;
    }
    if (gv.rsrc == 3u && gu.rbu) {                    /* the bank: its next kept rhythm, in turn */
        for (k = 1; k <= 8u; k++) if ((gu.rbu >> ((gu.rcur + k) & 7u)) & 1u) break;
        k = (gu.rcur + k) & 7u;
        for (r = 0; r < 8u; r++) gv_rhythm_row_set(r, gu.rb[k][r]);
        gu.rcur = (uint8_t)k;
        gv.libk = 0;
        gv_fmt(m, "BANQUE : RYTHME %d", (int)k + 1);
        gv_say(m);
        return 1;
    }
    return 0;
}
#define GV_LCN (GV_NLIB + 8u)                         /* BASE RYT's list: the base's motifs, then the bank's 8 rhythms */
static int gv_lib_fits(uint32_t k) { return (int)((GV_LIB[k % GV_NLIB].sm >> (gv.style % GV_NSTY)) & 1u); }
static void gv_lib_apply(uint32_t k)                  /* the list's entry k on the 8 rows */
{
    uint32_t r;
    char m[40];
    if (k < GV_NLIB) {
        for (r = 0; r < 8u; r++) { gv_rs_acc = GV_LIB[k].ac[r]; gv_rhythm_row_set(r, GV_LIB[k].on[r]); }
        gv_rs_acc = 0;
        gv.libk = (uint8_t)(k + 1u);
        gv_fmt(m, "BASE : %s %d", GV_LGN[GV_LIB[k].g % GV_NLG], (int)GV_LIB[k].bpm);
    } else {
        uint32_t b = (k - GV_NLIB) & 7u;
        if (!((gu.rbu >> b) & 1u)) { gv_say("EMPLACEMENT VIDE"); return; }
        for (r = 0; r < 8u; r++) gv_rhythm_row_set(r, gu.rb[b][r]);
        gu.rcur = (uint8_t)b;
        gv.libk = 0;
        gv_fmt(m, "BANQUE : RYTHME %d", (int)b + 1);
    }
    gv.tpl = 0;
    gv_say(m);
}
static void gv_lib_step(int32_t d)                    /* SEQ RYTHME, OCT- / OCT+: the style's previous / next motif of the base */
{
    uint32_t k, n = 0, any = 0, cur = gv.libk ? gv.libk - 1u : (d > 0 ? GV_NLIB - 1u : 0u);
    for (k = 0; k < GV_NLIB; k++) any |= (uint32_t)gv_lib_fits(k);
    for (k = 1; k <= GV_NLIB; k++) {
        uint32_t j = (cur + GV_NLIB + (d > 0 ? k : GV_NLIB - k)) % GV_NLIB;
        if (!any || gv_lib_fits(j)) { n = j + 1u; break; }
    }
    if (n) gv_lib_apply(n - 1u);
}
static void gv_bank_mel_step(int32_t d)               /* SEQ MELODIE, OCT- / OCT+: the bank's previous / next melody on the track */
{
    uint32_t k;
    char m[32];
    if (!gu.mbu || gv_isd()) { gv_say("BANQUE DE MELODIES VIDE (SAVE)"); return; }
    for (k = 1; k <= 8u; k++) if ((gu.mbu >> ((gu.mcur + (d > 0 ? k : 8u - k)) & 7u)) & 1u) break;
    k = (gu.mcur + (d > 0 ? k : 8u - k)) & 7u;
    memcpy(gv.t[(gv.sel - 8) % GV_NT].st, gu.mb[k], sizeof gu.mb[k]);
    gu.mcur = (uint8_t)k;
    gv_fmt(m, "BANQUE : MELODIE %d", (int)k + 1);
    gv_say(m);
}
static void gv_bank_keep(void)                        /* SAVE held: what plays now into the bank's next free slot */
{
    uint32_t k, r, mel = gu.page == GP_BKM || (!gv_isd() && gu.page != GP_LIBR && gu.page != GP_BKR);
    char m[40];
    for (k = 0; k < 8u && (((mel ? gu.mbu : gu.rbu) >> k) & 1u); k++) { }
    if (k == 8u) { gv_say("BANQUE PLEINE : EFFACE UN EMPLACEMENT"); return; }
    if (mel) { memcpy(gu.mb[k], gv.t[gv_isd() ? 0 : (gv.sel - 8) % GV_NT].st, sizeof gu.mb[k]); gu.mbu |= (uint8_t)(1u << k); gu.mcur = (uint8_t)k; }
    else { for (r = 0; r < 8u; r++) gu.rb[k][r] = gv_rhythm_row_get(r); gu.rbu |= (uint8_t)(1u << k); gu.rcur = (uint8_t)k; }
    gv_fmt(m, "%s GARDE : BANQUE %d", mel ? "MELODIE" : "RYTHME", (int)k + 1);
    gv_say(m);
}
static int gv_src_mel(uint32_t i)                     /* 1: the bank's next kept melody is on track i */
{
    uint32_t k;
    if (!gv.msrc || !gu.mbu) return 0;
    for (k = 1; k <= 8u; k++) if ((gu.mbu >> ((gu.mcur + k) & 7u)) & 1u) break;
    k = (gu.mcur + k) & 7u;
    memcpy(gv.t[i % GV_NT].st, gu.mb[k], sizeof gu.mb[k]);
    gu.mcur = (uint8_t)k;
    return 1;
}

/* ------------------------------------------------------------- GLO: 8 options, 2 across (PRESETS up/down, ALGORITHM left/right) --- */
static const char *const GV_GOPT[8] = {"BPM", "SWING", "STYLE", "AUTO RYTHME", "ALGORITHM", "TAP TEMPO", "MIDI", "HORTATOR"};
static const char *const GV_GAL[6] = {"PAGES", "BPM", "FILTRE DJ", "VOLUME", "RYTHMES", "SWING"};
static void gv_tap(void);
static void gv_glo_val(uint32_t o, char *v)
{
    if (o == 0u) gv_fmt(v, "%d", (int)song.g[G_BPM]);
    else if (o == 1u) gv_fmt(v, "%d%%", (int)song.g[G_SWING]);
    else if (o == 2u) gv_fmt(v, "%s", GV_STN[gv.style]);
    else if (o == 3u) gv_kfmt(KN_AUTO, gv_kget(KN_AUTO), v);
    else if (o == 4u) gv_fmt(v, "%s", GV_GAL[gv.algo % 6u]);
    else gv_fmt(v, "OCT-");
}
static void gv_glo_turn(int32_t s)                    /* SELECT: the value of the option under the cursor */
{
    static const uint8_t K[4] = {KN_BPM, KN_SWING, KN_STYLE, KN_AUTO};
    uint32_t o = gu.gcur & 7u;
    if (o < 4u) { int32_t mn, mx; gv_krange(K[o], &mn, &mx); gv_kset(K[o], gv_clamp(gv_kget(K[o]) + s, mn, mx)); gv_am_touch(K[o]); }
    else if (o == 4u) gv.algo = (uint8_t)gv_clamp((int32_t)gv.algo + (s > 0 ? 1 : -1), 0, 5);
    else gv_say("OCT- = VALIDER");
}
static void gv_glo_ok(void)                           /* OCT-: do what the option says */
{
    uint32_t o = gu.gcur & 7u;
    if (o == 3u) { int32_t mn, mx; gv_krange(KN_AUTO, &mn, &mx); gv_kset(KN_AUTO, gv_kget(KN_AUTO) > mn ? mn : mx); gv_say("AUTO RYTHME"); }
    else if (o == 5u) gv_tap();
    else if (o == 6u) { gu.page = GP_MIDI; gu.btn = B_GLO; }
    else if (o == 7u) { gv_on = 0; ui.force = 1; }
    else gv_say("SELECT = CHANGER LA VALEUR");
}

/* ------------------------------------------------------------- pages: the white keys --- */
static int gv_live_page(void) { return gu.page <= GP_LMEL; }
static int gv_play_page(void) { return gu.page <= GP_LMEL || gu.page == GP_RYT || gu.page == GP_MEL; }   /* (SEQ too: ALGORITHM = EDITION / JEU) */
static void gv_white(uint32_t j, int down)
{
    gv_trk_t *t = gv_tr();
    if (GV_PERFP(gu.page)) { gv_perf_key(j, down); gu.dirty = 1; return; }
    if (!down) return;
    if ((gu.kp && gv_play_page()) || gu.page == GP_SON) {   /* JEU: the white keys play too (ISR: gv_keys_isr) */
        return;
    }
    switch (gu.page) {
    case GP_MIXR:
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
    case GP_EFA: case GP_EFB: case GP_EFC: case GP_EFD: case GP_EFE: case GP_EFF: case GP_EFG:
        if (j >= 8u) gv_fxbyp((gu.fxpg >> 1) * 8u + j - 8u);
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
    case GP_SAVE: case GP_BKR: case GP_BKM: {         /* keys 1-8 the rhythms, 9-16 the melodies (of the shown track) */
        uint32_t k = j & 7u, mel = gu.page == GP_BKM || (gu.page == GP_SAVE && j >= 8u), used = ((mel ? gu.mbu : gu.rbu) >> k) & 1u, r;
        char m[28];
        if (gu.page != GP_SAVE && j >= 8u) break;
        if (gu.bmode == 2u) { if (mel) gu.mbu &= (uint8_t)~(1u << k); else gu.rbu &= (uint8_t)~(1u << k); gv_say("EMPLACEMENT EFFACE"); }
        else if (gu.bmode == 1u || !used) {           /* save (an empty slot saves even in CHARGER) */
            if (mel) { memcpy(gu.mb[k], gv_isd() ? gv.t[0].st : t->st, sizeof gu.mb[k]); gu.mbu |= (uint8_t)(1u << k); gu.mcur = (uint8_t)k; }
            else { for (r = 0; r < 8u; r++) gu.rb[k][r] = gv_rhythm_row_get(r); gu.rbu |= (uint8_t)(1u << k); gu.rcur = (uint8_t)k; }
            gv_fmt(m, "%s %d ENREGISTRE", mel ? "MELODIE" : "RYTHME", (int)k + 1); gv_say(m);
        } else {                                      /* load */
            if (mel) { memcpy(gv_isd() ? gv.t[0].st : t->st, gu.mb[k], sizeof gu.mb[k]); gu.mcur = (uint8_t)k; }
            else { for (r = 0; r < 8u; r++) gv_rhythm_row_set(r, gu.rb[k][r]); gu.rcur = (uint8_t)k; gv.tpl = 0; }
            gv_fmt(m, "%s %d CHARGE", mel ? "MELODIE" : "RYTHME", (int)k + 1); gv_say(m);
        }
        break; }
    case GP_FXR: gv_fxr_cut(j); break;
    case GP_LIBR: {                                    /* keys 1-8: the 8 motifs shown; 9: the one under the cursor */
        int32_t top = gv_clamp((int32_t)gu.lcur - 3, 0, (int32_t)GV_LCN - 8);
        if (j < 8u) { gu.lcur = (uint8_t)(top + (int32_t)j); gv_lib_apply(gu.lcur); } else if (j == 8u) gv_lib_apply(gu.lcur);
        break; }
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
    case GP_SON: gv_preset_turn(s > 0 ? 1 : -1); break;
    case GP_MIDI: gu.mrow = (uint8_t)gv_clamp((int32_t)gu.mrow + s, 0, (int32_t)GV_MROWS - 1); break;
    case GP_STY: gu.sty = (uint8_t)((gu.sty + GV_NSTY * 4 + s) % GV_NSTY); break;
    case GP_LIBR: gu.lcur = (uint8_t)gv_clamp((int32_t)gu.lcur + s, 0, (int32_t)GV_LCN - 1); break;
    case GP_VIS: gu.sty = (uint8_t)((gu.sty + GV_NSTY * 4 + s) % GV_NSTY); gv_say(gu.sty == gv.style ? GV_STN[gu.sty] : "ARP MAINTENU = VALIDER"); break;
    case GP_SCL: gv_set_scale((uint32_t)((gv.scale + GV_NSC * 4 + s) % GV_NSC)); break;
    case GP_GLO: gv_glo_turn(s); break;
    case GP_LMEL: case GP_GMEL: case GP_ENV: case GP_FLT: case GP_LFO: case GP_OSC:
        gv.sel = (int8_t)(8 + (((gv.sel - 8) + s) % 6 + 6) % 6); break;
    case GP_LRYT: case GP_GRYT:
        gv.sel = (int8_t)((((gv.sel & 7) + s) % 8 + 8) % 8); break;
    default:
        if (gu.page == GP_LIVE || gu.page == GP_MIX || gu.page == GP_MIXR) {
            int32_t pos = (((gu.grp ? 13 + gu.grp : gv.sel) + s) % 17 + 17) % 17;
            if (pos > 13) { gu.grp = (uint8_t)(pos - 13); gv_say(GV_GRPN[gu.grp]); } else { gu.grp = 0; gv.sel = (int8_t)pos; }
        } else gv.sel = (int8_t)(((gv.sel + s) % 14 + 14) % 14);
        break;
    }
}
static void gv_go(uint32_t b)
{
    uint32_t k, n = 0;
    const uint8_t *L = b == B_ENV ? GV_ENVP : b == B_LFO ? GV_LFOP : b == B_EDIT ? GV_EDITP : GV_GRP[b];
    for (k = 0; k < (b == B_ENV ? 11u : b == B_LFO ? 9u : b == B_EDIT ? 7u : 5u) && L[k] != 0xFF; k++) n++;
    if (!n) return;
    if (b == B_HOME && gu.btn == b && gu.page == GP_LFX && gu.fxpg < 6u) { gu.fxpg++; gu.dirty = 1; return; }   /* FX LIVE: its next page */
    if (gu.btn == b) {
        for (k = 0; k < n && L[k] != gu.page; k++) {}
        gu.page = L[(k + 1u) % n];
    } else {
        gu.ppage = gu.page; gu.pbtn = gu.btn;
        gu.btn = (uint8_t)b; gu.page = L[0];
    }
    if (gu.page == GP_STY || gu.page == GP_VIS) gu.sty = gv.style;
    if (gu.page == GP_LFX && b == B_HOME) gu.fxpg = 0;
    gu.dirty = 1;
}
static void gv_long(uint32_t b)
{
    if (b == B_HOME) {
        if (gv_isd()) gv_gen_row((uint32_t)gv.sel); else gv_gen_mel((uint32_t)gv.sel - 8u);
        gv_say(gv_isd() ? "NOUVEAU MOTIF" : "NOUVELLE MELODIE");
    } else if (b == B_SAVE) {                         /* SAVE held: the rhythm (the melody on its pages) kept in the bank */
        gv_bank_keep();
    } else if (b == B_SEQ) {                          /* SEQ held: the shown row / track again, alone */
        if (gv_isd()) gv_gen_row((uint32_t)gv.sel); else gv_gen_mel((uint32_t)gv.sel - 8u);
        gv_say(gv_isd() ? "NOUVEAU MOTIF" : "NOUVELLE MELODIE");
    } else if (b == B_ENV || b == B_LFO) {            /* ENV, LFO held: every effect of the shown track back to zero */
        static const uint8_t TT[GV_NT] = {2, 4, 3, 5, 6, 7};
        uint32_t e;
        if (gv_isd()) { gu.fxt = 0; gv_fxrow = (int8_t)(gv.sel & 7); } else { gu.fxt = TT[(gv.sel - 8) % GV_NT]; gv_fxrow = -1; }
        for (e = 0; e < FE_N; e++) if (FE_OFF[e] >= 0) { gv_fxset(e, FE_OFF[e]); gu.byp[gu.fxt][e] = -1; }
        gv_say("TOUS LES EFFETS A ZERO");
    } else if (b == B_ARP) {
        gv_set_style(gu.page == GP_STY || gu.page == GP_VIS ? gu.sty : gv.style);
        gv_gen_all();
        gv_say("STYLE + TOUT REGENERE");
    } else if (b == B_FX && (gu.page == GP_LFX || GV_EFP(gu.page))) {
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
    gv_store_poll();                                  /* (the bank: written when it changed and nothing plays) */
    if (gu.intro_t && (int32_t)(fm1_ms - gu.intro_t) < 300) return 1;   /* (its first 0.3 s: what the power-on left in the keys is not a press) */
    if (gu.intro_t && (pressed || notes)) { gu.intro_t = 0; gu.dirty = 1; return 1; }   /* the intro: any key ends it, and does nothing else */
    if (!gv_on) {                                     /* Hortator's pages: ENV held + ARP pressed = back to Groove */
        if (((held >> panel.btn[B_ENV]) & 1u) && ((pressed >> panel.btn[B_ARP]) & 1u)) { gv_on = 1; gu.dirty = 1; }
        else return 0;
    }
    if (!gu.init) { gu.init = 1; gu.page = GP_LIVE; gu.btn = B_HOME; gu.deg = 0; gu.dirty = 1; gv_boot(); }   /* (only once Groove
                                                     * is on: it sets the tempo and the drums' steps) */
    if (gu.page == GP_SON && (gu.snp != GP_SON || gu.sns != (uint8_t)gv.sel)) gv_snap_take();   /* EDIT SON just opened */
    gu.snp = gu.page; gu.sns = (uint8_t)gv.sel;
    gv_kp = (uint8_t)(gu.page == GP_SON ? 2u : gu.kp && gv_play_page());   /* EDIT SON: every key plays the track (to hear the sounds) */
    if (song.playing) {                               /* REC's knob lanes: once a step */
        uint32_t p = (((uint32_t)(gv_bar ? gv_bar - 1u : 0u)) & 3u) * 16u + (gv_pos & 15u);
        if (p != gv_am_last) { gv_am_last = (uint8_t)p; gv_am_step(p); }
    } else gv_am_last = 0xFF;
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
        int lp = b == B_HOME || b == B_ARP || b == B_FX || b == B_ENV || b == B_LFO || b == B_SEQ ||
                 (b == B_SAVE && gu.page != GP_SON && gu.page != GP_SONR && gu.page != GP_SEDIT);   /* (held 2 s: gv_long) */
        if (pressed & bit) {
            gu.dirty = 1;
            if (lp) { gu.hold_b = (uint8_t)b; gu.hold_t0 = fm1_ms; gu.hold_done = 0; continue; }
            if (b == B_PLAY) play_press();
            else if (b == B_REC) { uint32_t k; gv_recq ^= 1u; for (k = 0; k < AM_N; k++) gv_am[k].touch = 0;
                gv_say(gv_recq ? "REC : NOTES ET KNOBS S'ENREGISTRENT" : gv_am_any() ? "REC COUPE : LES KNOBS REJOUENT" : "REC COUPE"); }
            else if ((b == B_OCTDN || b == B_OCTUP) && (gv_live_page() || gu.page == GP_LFX || gu.page == GP_MIXR)) {   /* LIVE: OCT- the rhythm, OCT+ the melody */
                gv.gmute ^= b == B_OCTDN ? 1u : 2u;
                gv_say(b == B_OCTDN ? ((gv.gmute & 1u) ? "RYTHME COUPE" : "RYTHME RETABLI") : ((gv.gmute & 2u) ? "MELODIE COUPEE" : "MELODIE RETABLIE"));
            }
            else if ((b == B_OCTDN || b == B_OCTUP) && (gu.page == GP_SON || gu.page == GP_STY || gu.page == GP_VIS)) {   /* OCT- = keep, OCT+ = back */
                if (b == B_OCTDN) {
                    if (gu.page == GP_SON) { gv_snap_take(); gv_say("SON VALIDE"); }
                    else gv_long(B_ARP);              /* the style shown: taken, all regenerated */
                } else {
                    if (gu.page == GP_SON) gv_snap_back(); else gu.sty = gv.style;
                    gu.page = gu.ppage; gu.btn = gu.pbtn; gv_say("RETOUR");
                }
            }
            else if (b == B_OCTDN && gu.page == GP_GLO) gv_glo_ok();
            else if (b == B_SAVE && (gu.page == GP_SON || gu.page == GP_SONR || gu.page == GP_SEDIT)) gv_snd_keep();   /* SAVE: keep this sound */
            else if ((b == B_OCTDN || b == B_OCTUP) && gu.page == GP_RYT) gv_lib_step(b == B_OCTUP ? 1 : -1);        /* SEQ RYTHME: the style's motifs of the base */
            else if ((b == B_OCTDN || b == B_OCTUP) && gu.page == GP_MEL) gv_bank_mel_step(b == B_OCTUP ? 1 : -1);   /* SEQ MELODIE: the bank's melodies */
            else if (b == B_OCTDN && gu.page == GP_LIBR) gv_lib_apply(gu.lcur);                                  /* BASE RYT: OCT- = play it */
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
            gv_am_touch(kn[j]);
            gu.dirty = 1;
        }
    if ((s = panel_enc(EN_SELECT)) != 0) { gv_select(s); gu.dirty = 1; }
    if ((s = panel_enc(EN_PRESET)) != 0) {
        if (gu.page == GP_MIDI) gu.mrow = (uint8_t)gv_clamp((int32_t)gu.mrow + (s > 0 ? 1 : -1), 0, (int32_t)GV_MROWS - 1);   /* up / down */
        else if (gu.page == GP_LIBR) {                 /* BASE RYT: the next / previous genre's first motif */
            uint32_t k = gu.lcur < GV_NLIB ? gu.lcur : GV_NLIB - 1u, g0 = GV_LIB[k].g;
            if (s > 0) { while (k < GV_NLIB && GV_LIB[k].g == g0) k++; }
            else { while (k > 0u && GV_LIB[k].g == g0) k--; g0 = GV_LIB[k].g; while (k > 0u && GV_LIB[k - 1u].g == g0) k--; }
            gu.lcur = (uint8_t)(k >= GV_NLIB ? (gu.lcur >= GV_NLIB ? gu.lcur : GV_NLIB) : k);
        }
        else if (gu.page == GP_GLO) gu.gcur = (uint8_t)(gv_clamp((int32_t)(gu.gcur >> 1) + (s > 0 ? 1 : -1), 0, 3) * 2 + (gu.gcur & 1u));
        else if (gu.page == GP_LRYT) {                 /* LIVE RYTHME: where a new rhythm comes from (the auto too) */
            gv.rsrc = (uint8_t)gv_clamp((int32_t)gv.rsrc + (s > 0 ? 1 : -1), 0, 3);
            gv_say(gv.rsrc == 3u && !gu.rbu ? "SOURCE BANQUE : VIDE (SAVE POUR Y GARDER)" : GV_RSRC[gv.rsrc]);
        } else if (gu.page == GP_LMEL) {               /* LIVE MELODIE: where a new melody comes from */
            gv.msrc = (uint8_t)gv_clamp((int32_t)gv.msrc + (s > 0 ? 1 : -1), 0, 1);
            gv_say(gv.msrc && !gu.mbu ? "SOURCE BANQUE : VIDE (SAVE POUR Y GARDER)" : GV_MSRC[gv.msrc]);
        }
        else if (gv_live_page() || gu.page == GP_SON) gv_preset_turn(s > 0 ? 1 : -1);
        else if (gu.page == GP_RYT || gu.page == GP_LRYT || gu.page == GP_GRYT) gv.sel = (int8_t)(((gv.sel & 7) + s + 8) % 8);
        else if (gu.page == GP_MEL || gu.page == GP_LMEL || gu.page == GP_GMEL) gv.sel = (int8_t)(8 + ((gv.sel - 8 + s) % 6 + 6) % 6);
        else { gv.sel = (int8_t)(((gv.sel + s) % 14 + 14) % 14); gu.grp = 0; }
        gu.dirty = 1;
    }
    if ((s = panel_enc(EN_ALGO)) != 0) {
        if (gu.page == GP_LFX) {
            int32_t o = gu.fxpg;
            gu.fxpg = (uint8_t)gv_clamp(o + (s > 0 ? 1 : -1), 0, 6);
            gu.fxd = (int8_t)(gu.fxpg >= o ? 1 : -1); gu.fxa = fm1_ms;
            { static const char *const PT[7] = {"A : FILTRE RESO DISTO ECHO", "B : REVERB WOBBLE VITESSE VOL", "C : CHORUS ROLL SAUTS REVERSE",
                                                "D : LOOP CRUSH GATE DEMI", "E : PAN SIDECHAIN TREMOLO TAPE", "F : AUTO-PAN VIBRATO PITCH RING",
                                                "G : WAVEFOLD SATUR. VOYELLE NOISE"}; gv_say(PT[gu.fxpg]); }
        } else if (gu.page == GP_MIDI) { char l[32], v[20]; gv_midi_row(gu.mrow, s > 0 ? 1 : -1, l, v); }   /* left / right: the value */
        else if (gu.page == GP_GLO) { gu.gcur = (uint8_t)((gu.gcur & 6u) | (s > 0 ? 1u : 0u)); }
        else if (gv_play_page()) { gu.kp = s > 0; gv_say(gu.kp ? "MODE JEU : LE CLAVIER JOUE" : gv_live_page() ? "MODE COUPURES" : "MODE EDITION : LES PAS"); }
        else if (gv.algo == 0u) {                     /* ALGORITHM: every page in turn (GLO: or a setting) */
            uint32_t k, n = GP_N;
            gu.page = (uint8_t)((gu.page + n + (uint32_t)(s > 0 ? 1 : n - 1)) % n);
            for (b = 0; b < NB; b++) for (k = 0; k < 5u; k++) if (GV_GRP[b][k] == gu.page) gu.btn = (uint8_t)b;
            if (GV_EFP(gu.page)) gu.btn = B_ENV;
            if (gu.page == GP_STY || gu.page == GP_VIS) gu.sty = gv.style;
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
    if (gv_grp_on() ? gv_grp_has(p) : p == gv.sel) { G_F(x - 1, y - 1, w + 2, h + 2, GC_T); G_F(x, y, w, h, GC_T); }
}
static void gv_draw_head(void)
{
    char b[24];
    int32_t w = G_TW(2, GP_T[gu.page]) + 8;
    G_R(2, 2, w, 15, GC_A);
    G_T(6, 5, 2, GP_T[gu.page], GC_BG);
    G_F(w + 6, 2, 100, 15, gv_grp_on() ? gv_grpc() : gv_col(gv.sel));
    G_T(w + 10, 5, G_TW(2, gv_grp_on() ? GV_GRPN[gu.grp] : gv_name(gv.sel)) > 92 ? 1u : 2u, gv_grp_on() ? GV_GRPN[gu.grp] : gv_name(gv.sel), gv_grp_on() ? gv_grpc() : gv_col(gv.sel));
    gv_fmt(b, "%s%d", song.playing ? ">" : "", (int)song.g[G_BPM]);
    G_TR(236, 5, 2, b, song.playing ? GC_Y : GC_M);
    gv_fmt(b, "%s  %s %s", GV_STN[gv.style], GV_NOTE[gv.root], GV_SCN[gv.scale % GV_NSC]);
    G_T(4, 22, 1, b, GC_M);
    if (gu.kp && gv_play_page()) { G_R(170, 20, 30, 9, GC_Y); G_T(174, 22, 1, "JEU", GC_BG); }
    else if (gu.page == GP_LRYT) G_TR(200, 22, 1, GV_RSRS[gv.rsrc & 3u], gv.rsrc ? GC_C : GC_M);   /* (PRESETS: the source) */
    else if (gu.page == GP_LMEL) G_TR(200, 22, 1, gv.msrc ? "BANQUE" : "GENER.", gv.msrc ? GC_C : GC_M);
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
        if (gv_am_find(kn[i], 0)) G_R(cx + 12, 188, 4, 4, gv_recq ? GC_R : GC_C);   /* a recorded knob */
        cv_knob(cx, 200 - gv_oy, 9, mx > mn ? (v - mn) * 1000 / (mx - mn) : 0, -1, i & 1u ? GC_C : GC_A);
        G_TC(cx, 216, 1, kn[i] >= KN_FX0 && kn[i] <= KN_FX3 ? (gu.fxt == 0u && FE_LD[gu.fxpg * 4u + i] ? FE_LD[gu.fxpg * 4u + i] : FE_L[gu.fxpg * 4u + i]) : kn[i] >= KN_E2 && kn[i] <= KN_E5 ? track_desc(gv_rt(), P_E2 + kn[i] - KN_E2)->label : KN_L[kn[i]], GC_M);
        gv_kfmt(kn[i], v, b);
        G_TC(cx, G_TW(2, b) > 58 ? 228 : 226, G_TW(2, b) > 58 ? 1u : 2u, b, GC_T);
    }
}
/* ------------------------------------------------------------- the web app's page pictures, in pixels --- */
static void G_L(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c) { cv_line(x0, y0 - gv_oy, x1, y1 - gv_oy, c); }
static void G_L2(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c) { G_L(x0, y0, x1, y1, c); G_L(x0, y0 + 1, x1, y1 + 1, c); }
static void G_C(int32_t cx, int32_t cy, int32_t r, uint16_t c) { if (r > 0) cv_ring(cx, cy - gv_oy, r, c); }
static int32_t gv_isqrt(int32_t v) { int32_t r = 0; while ((r + 1) * (r + 1) <= v) r++; return r; }
static void G_D(int32_t cx, int32_t cy, int32_t r, uint16_t c)           /* a disc */
{
    int32_t dy;
    for (dy = -r; dy <= r; dy++) { int32_t w = gv_isqrt(r * r - dy * dy); G_R(cx - w, cy + dy, 2 * w + 1, 1, c); }
}
static void G_TRI(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t c)   /* a filled triangle */
{
    int32_t y, mn = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2), mx = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);
    const int32_t X[3] = {x0, x1, x2}, Y[3] = {y0, y1, y2};
    for (y = mn; y <= mx; y++) {
        int32_t a = 9999, b = -9999, k;
        for (k = 0; k < 3; k++) {
            int32_t xa = X[k], ya = Y[k], xb = X[(k + 1) % 3], yb = Y[(k + 1) % 3];
            if ((y >= ya && y <= yb) || (y >= yb && y <= ya)) {
                int32_t x = ya == yb ? xa : xa + (xb - xa) * (y - ya) / (yb - ya);
                if (x < a) a = x;
                if (x > b) b = x;
                if (ya == yb) { if (xb < a) a = xb; if (xb > b) b = xb; }
            }
        }
        if (b >= a) G_R(a, y, b - a + 1, 1, c);
    }
}
static uint16_t gv_mix565(uint16_t a, uint16_t b, int32_t k)              /* k 0..16: a -> b */
{
    int32_t r = ((a >> 11) * (16 - k) + (b >> 11) * k) / 16, g = (((a >> 5) & 63) * (16 - k) + ((b >> 5) & 63) * k) / 16,
            bl = ((a & 31) * (16 - k) + (b & 31) * k) / 16;
    return (uint16_t)((r << 11) | (g << 5) | bl);
}
/* (RGB() swaps for the panel: the sky mixes plain 565 then the same swap as RGB) */
#define P565(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
static uint16_t gv_sw(uint16_t c) { return RGB(((c >> 11) << 3), (((c >> 5) & 63) << 2), ((c & 31) << 3)); }
static void G_SKY(int32_t y0, int32_t h, uint16_t top, uint16_t bot)     /* a vertical gradient (8 bands) */
{
    int32_t k;
    for (k = 0; k < 8; k++) G_R(0, y0 + h * k / 8, 240, h / 8 + 1, gv_sw(gv_mix565(top, bot, k * 16 / 7)));
}
static uint32_t gv_beatq(void)                       /* 0..255 within the beat (the tempo when stopped too) */
{
    uint32_t bpm = (uint32_t)gv_clamp(song.g[G_BPM], 40, 240), per = 60000u / bpm;
    return (fm1_ms % per) * 256u / per;
}
/* the A4E monster (3 legs, 2 tentacle hands, the eye, the headphones), u = pixel size; it dances on the beat */
static void gv_mon(int32_t cx, int32_t by, int32_t u)
{
    uint32_t q = gv_beatq(), bt = song.playing ? (uint32_t)(gv_pos >> 2) & 3u : (fm1_ms / 600u) & 3u;
    int32_t hop = (int32_t)(q < 128u ? (128 - (int32_t)q) * 3 * u / 128 : 0), y = by - hop, k;
    uint16_t o = RGB(0xf0, 0x80, 0x30), od = RGB(0xb8, 0x56, 0x1c), ink = RGB(0x11, 0x11, 0x11);
    G_R(cx - 9 * u, by + 1, 18 * u, u, RGB(0x10, 0x10, 0x10));              /* shadow */
    for (k = -1; k <= 1; k++) {                                              /* legs: one lifts after the other */
        int32_t lift = (int32_t)((bt + 4u + (uint32_t)k) % 3u) == 0 ? 2 * u : 0;
        G_R(cx + k * 4 * u - u - u / 2, y - 6 * u - lift, 3 * u, 6 * u, ink);
        G_R(cx + k * 4 * u - u, y - 6 * u - lift, 2 * u, 6 * u - u / 2, o);
    }
    {   /* hands: the outer tentacles, up and down with the beat */
        int32_t l = bt == 1u || bt == 3u ? 1 : 0, r = bt == 2u || bt == 3u ? 1 : 0;
        G_R(cx - 10 * u, y - (l ? 16 : 9) * u, 3 * u, 7 * u, ink); G_R(cx - 10 * u + u / 2, y - (l ? 16 : 9) * u + u / 2, 2 * u, 6 * u, o);
        G_R(cx + 7 * u, y - (r ? 16 : 9) * u, 3 * u, 7 * u, ink); G_R(cx + 7 * u + u / 2, y - (r ? 16 : 9) * u + u / 2, 2 * u, 6 * u, o);
    }
    G_R(cx - 7 * u - 1, y - 20 * u - 1, 14 * u + 2, 15 * u + 2, ink);       /* body */
    G_D(cx, y - 20 * u, 7 * u + 1, ink);
    G_R(cx - 7 * u, y - 20 * u, 14 * u, 15 * u, o);
    G_D(cx, y - 20 * u, 7 * u, o);
    G_R(cx + 4 * u, y - 22 * u, 3 * u, 16 * u, od);                          /* the shaded side */
    G_D(cx, y - 19 * u, 4 * u, ink); G_D(cx, y - 19 * u, 3 * u + u / 2, RGB(0xff, 0xff, 0xff));   /* the eye */
    G_D(cx, y - 19 * u, 2 * u, RGB(0x7c, 0xc2, 0x42)); G_D(cx, y - 19 * u, u, ink); G_R(cx + u / 2, y - 20 * u, u > 1 ? u / 2 + 1 : 1, u > 1 ? u / 2 + 1 : 1, RGB(0xff, 0xff, 0xff));
    G_R(cx - 4 * u, y - 13 * u, 8 * u, 2 * u + (q < 96u && song.playing ? u : 0), ink);   /* the mouth opens on the beat */
    G_R(cx - 3 * u, y - 13 * u, u, u, RGB(0xff, 0xff, 0xff)); G_R(cx + 2 * u, y - 13 * u, u, u, RGB(0xff, 0xff, 0xff));
    G_R(cx - 8 * u, y - 23 * u, 2 * u, 5 * u, RGB(0x8a, 0x0d, 0x1e)); G_R(cx + 6 * u, y - 23 * u, 2 * u, 5 * u, RGB(0x8a, 0x0d, 0x1e));   /* headphones */
    G_R(cx - 6 * u, y - 28 * u, 12 * u, u, GC_L); G_R(cx - 7 * u, y - 27 * u, u, 4 * u, GC_L); G_R(cx + 6 * u, y - 27 * u, u, 4 * u, GC_L);
}
/* the 6 scales' scenes (the web app's scenes.js), y0..y0+h */
static void gv_scene(uint32_t sc, int32_t y0, int32_t h)
{
    int32_t k, ms = (int32_t)fm1_ms, q = (int32_t)gv_beatq();
    switch (sc % GV_NSC) {
    case 0:   /* MINEUR: the city at night, the moon, lit windows */
        G_SKY(y0, h, P565(0x0b, 0x10, 0x20), P565(0x1c, 0x21, 0x40));
        G_D(196, y0 + 20, 10, RGB(0xe8, 0xe6, 0xd8)); G_D(201, y0 + 17, 9, RGB(0x10, 0x14, 0x26));
        { static const int8_t B[11][2] = {{0, 50}, {22, 70}, {40, 42}, {62, 82}, {86, 58}, {108, 90}, {128, 48}, {150, 74}, {172, 62}, {196, 86}, {218, 54}};
          for (k = 0; k < 11; k++) {
              int32_t H = B[k][1] * h / 110, x = B[k][0], wy;
              G_R(x, y0 + h - H, 24, H, k & 1 ? RGB(0x15, 0x1a, 0x2c) : RGB(0x1a, 0x20, 0x36));
              for (wy = y0 + h - H + 4; wy < y0 + h - 4; wy += 7) {
                  int32_t wx;
                  for (wx = x + 3; wx < x + 21; wx += 6) if (((wx * 7 + wy * 3 + ms / 900) % 5) < 2) G_R(wx, wy, 3, 3, RGB(0xff, 0xd2, 0x3f));
              }
          } }
        break;
    case 1:   /* PHRYGIEN: the desert, the pyramids, the sun going down */
        G_SKY(y0, h, P565(0xff, 0x8a, 0x1f), P565(0xe8, 0xb4, 0x5a));
        G_D(120, y0 + h * 62 / 100, 20, RGB(0xff, 0xf3, 0xc4));
        { int32_t gy = y0 + h * 82 / 100;
          G_TRI(48 - 44, gy, 48, gy - 42, 48 + 44, gy, RGB(0xc9, 0x8a, 0x3a)); G_TRI(48, gy - 42, 92, gy, 58, gy, RGB(0xa0, 0x6a, 0x2a));
          G_TRI(100 - 30, gy, 100, gy - 28, 130, gy, RGB(0xb9, 0x7a, 0x2e));
          G_TRI(206 - 38, gy, 206, gy - 36, 244, gy, RGB(0xc9, 0x8a, 0x3a)); G_TRI(206, gy - 36, 244, gy, 214, gy, RGB(0xa0, 0x6a, 0x2a));
          G_R(0, gy, 240, y0 + h - gy, RGB(0xd9, 0xa0, 0x4e));
          for (k = 0; k < 5; k++) G_R((k * 53 + ms / 40) % 240, gy + 4 + (k % 3) * 4, 10, 1, RGB(0xf0, 0xc0, 0x70)); }
        break;
    case 2:   /* DORIEN: the club, the spot lights, the mirror ball */
        G_R(0, y0, 240, h, RGB(0x16, 0x0b, 0x26));
        for (k = 0; k < 5; k++) {
            int32_t sw = gv_sin((uint32_t)(ms / 12 + k * 50) & 255u) * 40 / 127, x = 24 + k * 48;
            uint16_t c = k % 3 == 0 ? RGB(0x6a, 0x3a, 0x20) : k % 3 == 1 ? RGB(0x1a, 0x5a, 0x50) : RGB(0x6a, 0x40, 0x5a);
            G_TRI(x, y0, x + sw - 14, y0 + h, x + sw + 14, y0 + h, c);
        }
        G_L(120, y0, 120, y0 + 8, GC_M);
        { int32_t i, j;
          for (i = -3; i <= 3; i++) for (j = -3; j <= 3; j++) {
              if (i * i + j * j > 10) continue;
              G_R(120 + i * 4 - 2, y0 + 20 + j * 4 - 2, 3, 3, gv_sin((uint32_t)(i * 37 + j * 23 + ms / 6) & 255u) > 40 ? GC_T : RGB(0x8a, 0x94, 0xa6));
          } }
        G_R(0, y0 + h - 10, 240, 10, RGB(0x26, 0x14, 0x3a));
        for (k = 0; k < 12; k++) G_R(k * 20, y0 + h - 10, 18, 9, ((k + (ms / 250)) & 1) ? RGB(0xff, 0x46, 0x9a) : RGB(0x35, 0xe3, 0xc4));
        break;
    case 3:   /* MINEUR HARMONIQUE: the stars, the hills, the moon */
        G_SKY(y0, h, P565(0x1b, 0x12, 0x38), P565(0x6b, 0x3a, 0x5a));
        for (k = 0; k < 30; k++) if (gv_sin((uint32_t)(ms / 8 + k * 40) & 255u) > -40) G_R((k * 67) % 240, y0 + (k * 29) % (h * 55 / 100), 1, 1, GC_T);
        G_D(43, y0 + h * 22 / 100, 10, RGB(0xff, 0xe9, 0xa6)); G_D(47, y0 + h * 22 / 100 - 2, 9, RGB(0x1f, 0x14, 0x3c));
        for (k = 0; k < 240; k += 4) { int32_t hy = y0 + h * 78 / 100 + gv_sin((uint32_t)(k * 2) & 255u) * 5 / 127; G_R(k, hy, 4, y0 + h - hy, RGB(0x3a, 0x21, 0x40)); }
        break;
    case 4:   /* PENTA MINEURE: the pink sky, the red sun, the mountain */
        G_SKY(y0, h, P565(0xff, 0xb3, 0xdc), P565(0xff, 0xe9, 0xd6));
        G_D(173, y0 + h * 30 / 100, 14, RGB(0xff, 0x46, 0x46));
        { int32_t gy = y0 + h * 82 / 100, ty = y0 + h * 28 / 100;
          G_TRI(19, gy, 96, ty, 115, ty, RGB(0x5b, 0x6b, 0x8c)); G_TRI(19, gy, 115, ty, 197, gy, RGB(0x5b, 0x6b, 0x8c));
          G_TRI(80, ty + 14, 96, ty, 115, ty, GC_T); G_TRI(96, ty, 115, ty, 134, ty + 15, GC_T);
          G_R(0, gy, 240, y0 + h - gy, RGB(0x8a, 0xc0, 0x7a));
          for (k = 0; k < 6; k++) { int32_t px = (k * 47 + ms / 30) % 260 - 10, py = y0 + 10 + (k * 13) % 40 + gv_sin((uint32_t)(ms / 6 + k * 60) & 255u) * 4 / 127; G_R(px, py, 3, 2, RGB(0xff, 0x8a, 0xc8)); } }
        break;
    default:  /* MAJEUR: the blue sky, the sun smiling, the clouds */
        G_SKY(y0, h, P565(0x5e, 0xc8, 0xff), P565(0xbf, 0xea, 0xff));
        { int32_t sx = 187, sy = y0 + h * 24 / 100, r = 14 + (q < 64 && song.playing ? 2 : 0);
          for (k = 0; k < 12; k++) { int32_t a = k * 21 + ms / 30; G_L2(sx + gv_sin((uint32_t)a & 255u) * (r + 4) / 127, sy + gv_sin((uint32_t)(a + 64) & 255u) * (r + 4) / 127,
                                                                       sx + gv_sin((uint32_t)a & 255u) * (r + 10) / 127, sy + gv_sin((uint32_t)(a + 64) & 255u) * (r + 10) / 127, GC_Y); }
          G_D(sx, sy, r, GC_Y); G_R(sx - 7, sy - 3, 5, 3, RGB(0x11, 0x11, 0x11)); G_R(sx + 2, sy - 3, 5, 3, RGB(0x11, 0x11, 0x11)); G_R(sx - 4, sy + 4, 8, 2, RGB(0x11, 0x11, 0x11)); }
        for (k = 0; k < 2; k++) { int32_t cx = (k * 120 + ms / 50) % 280 - 30, cy = y0 + 20 + k * 22; G_D(cx, cy, 8, GC_T); G_D(cx + 10, cy - 3, 10, GC_T); G_D(cx + 21, cy, 8, GC_T); }
        G_R(0, y0 + h - 14, 240, 14, RGB(0x6c, 0xc0, 0x4a));
        break;
    }
}
static int32_t gs_x, gs_y, gs_w, gs_h;               /* the stage's rectangle: its lines stay inside */
static void G_LS(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c)   /* a line clipped to it (Liang-Barsky) */
{
    int32_t dx = x1 - x0, dy = y1 - y0, t0 = 0, t1 = 1024, k;
    int32_t pp[4], qq[4];
    pp[0] = -dx; pp[1] = dx; pp[2] = -dy; pp[3] = dy;
    qq[0] = x0 - gs_x; qq[1] = gs_x + gs_w - 1 - x0; qq[2] = y0 - gs_y; qq[3] = gs_y + gs_h - 1 - y0;
    for (k = 0; k < 4; k++) {
        if (!pp[k]) { if (qq[k] < 0) return; }
        else {
            int32_t t = qq[k] * 1024 / pp[k];
            if (pp[k] < 0) { if (t > t1) return; if (t > t0) t0 = t; }
            else { if (t < t0) return; if (t < t1) t1 = t; }
        }
    }
    G_L(x0 + dx * t0 / 1024, y0 + dy * t0 / 1024, x0 + dx * t1 / 1024, y0 + dy * t1 / 1024, c);
}
static void gv_stage(uint32_t st, int32_t x0, int32_t y0, int32_t w, int32_t h, int32_t u)   /* a style's scene + the monster */
{
    static const uint32_t TOP[GV_NSTY] = {0x0b0b14, 0x140808, 0x10103a, 0x090014, 0x14100a, 0x101214, 0x0a1a10, 0x141014, 0x0c0c18, 0x101010, 0x1a0808,
                                          0x0a1408, 0x2a0a3a, 0x1a0a30, 0x200000, 0x101010, 0x1a1008,
                                          /* (the 15 styles of the rhythm base) */
                                          0x140a0a, 0x08101c, 0x1a1408, 0x081418, 0x100a20, 0x0a0a1a, 0x200a1a, 0x0a1020, 0x0a0a0a, 0x0a140a, 0x14100a, 0x10081c, 0x1c0c08, 0x14100c, 0x0c1014},
                          BOT[GV_NSTY] = {0x1a1a30, 0x301010, 0x7ab8ff, 0x3a0a50, 0x405010, 0x202830, 0x20401a, 0x303040, 0x302040, 0x202030, 0x400a0a,
                                          0x30500a, 0x8a3a6a, 0x5a2a7a, 0x600a0a, 0x303030, 0x604020,
                                          0x501010, 0x204870, 0x705020, 0x1a5060, 0x4a2a80, 0x2a2a70, 0x902a6a, 0x2a50a0, 0x303a40, 0x2a6030, 0x604a20, 0x503080, 0x803a1a, 0x5a4830, 0x3a4a5a},
                          ACC[GV_NSTY] = {0xff4646, 0xff8a1f, 0xffffff, 0x35e3c4, 0xc6f432, 0x939ba5, 0x3ddc84, 0xffd23f, 0xa78bfa, 0x4f8cff, 0xff4646,
                                          0xc6f432, 0xff6fb5, 0xffd23f, 0xff4646, 0xff8a1f, 0xff9e6b,
                                          0xff3030, 0x6fd0ff, 0xffb347, 0x35e3c4, 0xb48cff, 0x7a8cff, 0xff6fb5, 0x4fd0ff, 0xc6f432, 0x3ddc84, 0xffd23f, 0xd08cff, 0xff8a1f, 0xe8c070, 0x9fc0d8};
    static const uint8_t KIND[GV_NSTY] = {0, 3, 1, 2, 4, 5, 6, 7, 7, 7, 3, 4, 8, 8, 0, 3, 9,
                                          0, 2, 8, 6, 1, 5, 8, 1, 3, 9, 7, 5, 9, 7, 6};   /* lasers rays spiral wall smiley grid dub city disco fire */
    uint32_t k, s = st % GV_NSTY, ms = fm1_ms, q = gv_beatq();
    uint16_t top = (uint16_t)P565(TOP[s] >> 16, (TOP[s] >> 8) & 255u, TOP[s] & 255u), bot = (uint16_t)P565(BOT[s] >> 16, (BOT[s] >> 8) & 255u, BOT[s] & 255u),
             acc = RGB(ACC[s] >> 16, (ACC[s] >> 8) & 255u, ACC[s] & 255u);
    int32_t cx = x0 + w / 2, fl = y0 + h - 6;
    gs_x = x0; gs_y = y0; gs_w = w; gs_h = h;
    for (k = 0; k < 8u; k++) G_R(x0, y0 + h * (int32_t)k / 8, w, h / 8 + 1, gv_sw(gv_mix565(top, bot, (int32_t)k * 16 / 7)));
    switch (KIND[s]) {
    case 0:   /* lasers from the stage */
        for (k = 0; k < 7u; k++) { int32_t a = gv_sin((ms / 10u + k * 37u) & 255u) * w / 2 / 127; G_LS(cx, fl - 10, cx + a + ((int32_t)k - 3) * w / 9, y0, acc); }
        break;
    case 1:   /* rays turning round the light */
        for (k = 0; k < 12u; k++) { uint32_t a = (k * 21u + ms / 40u) & 255u; G_LS(cx, y0 + h * 6 / 10, cx + gv_sin(a) * w / 127, y0 + h * 6 / 10 + gv_sin((a + 64u) & 255u) * w / 127, gv_mix565(acc, top, 8)); }
        G_D(cx, y0 + h * 6 / 10, 6 + (q < 64u ? 3 : 0), acc);
        break;
    case 2:   /* the psychedelic spiral */
        for (k = 0; k < 40u; k++) { uint32_t a = (k * 13u + ms / 15u) & 255u; int32_t r = (int32_t)k * h / 90; G_R(cx + gv_sin(a) * r / 127, y0 + h / 2 + gv_sin((a + 64u) & 255u) * r / 127, 2, 2, k & 1u ? acc : RGB(0xff, 0x6f, 0xb5)); }
        break;
    case 3:   /* the wall of speakers (free party) */
        for (k = 0; k < 8u; k++) { int32_t sx = x0 + 4 + (int32_t)(k % 4u) * (w - 8) / 4 + ((int32_t)k / 4) * w / 16, sy = fl - 18 - (int32_t)(k / 4u) * 20, r = 6 + (q < 64u ? 2 : 0);
            G_R(sx, sy, w / 6, 18, RGB(0x1a, 0x1a, 0x1e)); G_D(sx + w / 12, sy + 9, r > w / 14 ? w / 14 : r, RGB(0x05, 0x05, 0x05)); G_C(sx + w / 12, sy + 9, r > w / 14 ? w / 14 : r, acc); }
        break;
    case 4: { /* the acid smiley */
        int32_t sy = y0 + h / 2 - 6 + (q < 64u ? -3 : 0), r = h / 5;
        G_D(cx, sy, r, RGB(0xff, 0xd2, 0x3f)); G_R(cx - r / 2, sy - r / 3, 3, 4, RGB(0x11, 0x11, 0x11)); G_R(cx + r / 2 - 3, sy - r / 3, 3, 4, RGB(0x11, 0x11, 0x11));
        for (k = 0; k < 9u; k++) G_R(cx - r / 2 + (int32_t)k * r / 8, sy + r / 3 + gv_abs((int32_t)k - 4) * -1 + 3, 2, 2, RGB(0x11, 0x11, 0x11));
        break; }
    case 5:   /* minimal: the grid, one cell on the beat */
        for (k = 0; k < 16u; k++) G_R(x0 + 8 + (int32_t)(k % 8u) * (w - 16) / 8, y0 + h / 3 + (int32_t)(k / 8u) * 14, (w - 16) / 8 - 3, 10, k == (song.playing ? gv_pos : (ms / 200u) & 15u) ? acc : RGB(0x30, 0x34, 0x3a));
        break;
    case 6:   /* dub: the big speaker breathing, the smoke */
        G_R(cx - h / 4, y0 + 8, h / 2, h - 16, RGB(0x10, 0x18, 0x10)); G_D(cx, y0 + h / 2 - 4, h / 6 + (q < 96u ? 2 : 0), RGB(0x05, 0x05, 0x05)); G_C(cx, y0 + h / 2 - 4, h / 6 + 2, acc);
        for (k = 0; k < 5u; k++) G_C(x0 + 20 + (int32_t)k * w / 5, y0 + 12 + gv_sin((ms / 20u + k * 50u) & 255u) * 6 / 127, 5, RGB(0x60, 0x70, 0x60));
        break;
    case 7:   /* the city, the street lights */
        for (k = 0; k < 9u; k++) { int32_t bh = 20 + (int32_t)((k * 37u) % 40u), x = x0 + (int32_t)k * w / 9; G_R(x, fl - bh, w / 9 - 2, bh, RGB(0x18, 0x18, 0x24));
            if (((k * 7u + ms / 700u) % 3u) == 0u) G_R(x + 3, fl - bh + 5, 3, 3, acc); }
        break;
    case 8: { /* house: the disco ball, the spots */
        int32_t i, j;
        for (k = 0; k < 4u; k++) { int32_t sw = gv_sin((ms / 14u + k * 60u) & 255u) * 30 / 127, x = x0 + 20 + (int32_t)k * (w - 40) / 3; G_TRI(x, y0, x + sw - 10, fl, x + sw + 10, fl, gv_mix565(top, acc, 5)); }
        for (i = -2; i <= 2; i++) for (j = -2; j <= 2; j++) if (i * i + j * j <= 5) G_R(cx + i * 4 - 2, y0 + 16 + j * 4 - 2, 3, 3, gv_sin((uint32_t)(i * 40 + j * 23 + (int32_t)ms / 6) & 255u) > 40 ? GC_T : RGB(0x8a, 0x94, 0xa6));
        break; }
    default:  /* tribe: the fire */
        for (k = 0; k < 14u; k++) { int32_t fx = cx - 18 + (int32_t)k * 3, fh = 10 + gv_abs(gv_sin((ms / 5u + k * 41u) & 255u)) * 14 / 127; G_R(fx, fl - fh, 3, fh, k & 1u ? acc : RGB(0xff, 0xd2, 0x3f)); }
        break;
    }
    G_R(x0, fl, w, y0 + h - fl, RGB(0x10, 0x10, 0x14));
    if (u) gv_mon(KIND[s] == 1u ? cx : x0 + w * 82 / 100, fl, u);
}
static void gv_perfmini(int32_t y)                   /* the 16 pads, small (the web's perfMini) */
{
    uint32_t j;
    for (j = 0; j < 16u; j++) G_R(4 + (int32_t)j * 146 / 10, y, 12, 10, gv_perf_on(j) ? (j < 8u ? GC_C : GC_Y) : GC_P2);
    G_TC(120, y + 14, 1, "PADS : LOOP ROLL REVERSE CIBLE VERROU", GC_M);
}
static void gv_cell(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t kind, int32_t v, uint16_t cc, int hk)   /* the MIX cells */
{
    int32_t k, ms = (int32_t)fm1_ms, beat = 60000 / gv_clamp(song.g[G_BPM], 40, 240);
    G_R(x, y, w, h, GC_P);
    if (kind == 0u) {                                  /* a level meter that jumps with the hits */
        int32_t pulse = hk ? 256 : 64 + gv_sin((uint32_t)(ms * 256 / beat) & 255u) * 32 / 127 + 32, lv = v * (90 + 166 * pulse / 256) / 256;
        for (k = 0; k < 12; k++) G_R(x + 14, y + h - 10 - k * 7, w - 28, 5, k * 127 / 12 < lv ? (k > 9 ? GC_R : k > 7 ? GC_Y : cc) : GC_P2);
    } else if (kind == 1u) {                           /* the disto: the wave clips */
        int32_t i, py = 0;
        for (i = 0; i <= w - 4; i++) { int32_t a = gv_sin((uint32_t)(i * 512 / w - ms / 6) & 255u), yy = gv_clamp(a * (1 + v / 9), -127, 127);
            yy = y + h / 2 - yy * (h * 32 / 100) / 127; if (i) G_L(x + 2 + i - 1, py, x + 2 + i, yy, GC_Y); py = yy; }
    } else if (kind == 2u) {                           /* the echo: dots going away */
        int32_t per = beat * 3 / 4, ph = (ms % per) * 256 / per, a = 256;
        for (k = 0; k < 6 && v; k++) { int32_t cx = x + 8 + (k * 256 + ph) * (w - 14) / (6 * 256); if (a > 40) G_D(cx, y + h / 2, (5 * a / 256) + 1, a > 128 ? cc : gv_mix565(GC_P, cc, a / 16)); a = a * 62 / 100; }
    } else if (kind == 3u) {                           /* the reverb: rings growing */
        int32_t per = beat * 2;
        for (k = 0; k < 4; k++) { int32_t ph = ((ms % per) * 256 / per + k * 64) & 255; if (v) G_C(x + w / 2, y + h / 2, 4 + ph * (10 + 38 * v / 127) / 256, ph < 160 ? GC_C : GC_L); }
    } else {                                           /* AUTO: the dice, turning when on */
        int32_t sp = v ? ms / 120 : 0, dx = gv_sin((uint32_t)(sp * 8) & 255u) * 3 / 127;
        G_R(x + w / 2 - 14 + dx, y + h / 2 - 14, 28, 28, v ? GC_C : GC_P2);
        G_R(x + w / 2 - 8 + dx, y + h / 2 - 8, 4, 4, GC_BG); G_R(x + w / 2 - 2 + dx, y + h / 2 - 2, 4, 4, GC_BG); G_R(x + w / 2 + 4 + dx, y + h / 2 + 4, 4, 4, GC_BG);
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
    case GP_MIXR: {                                    /* the mixer: 14 faders, the general equaliser's curve */
        static const char *const AB[14] = {"KIC", "SNA", "CLA", "HHF", "HHO", "PER", "LOW", "RIM", "BAS", "SYN", "BA2", "SY2", "NAP", "FX"};
        int32_t py = 0, k;
        G_T(4, 35, 1, "EQ", GC_M);
        G_R(20, 34, 216, 20, GC_P); G_R(20, 44, 216, 1, GC_P2);
        for (k = 0; k <= 215; k++) {                   /* lows | mids | highs, smoothed between */
            int32_t a = k < 54 ? gv.eq[0] : k < 108 ? gv.eq[0] + (gv.eq[1] - gv.eq[0]) * (k - 54) / 54 : k < 162 ? gv.eq[1] + (gv.eq[2] - gv.eq[1]) * (k - 108) / 54 : gv.eq[2],
                    yy = 44 - (a - 64) * 9 / 64;
            if (k) G_L2(20 + k - 1, py, 20 + k, yy, GC_Y);
            py = yy;
        }
        for (i = 0; i < 14u; i++) {
            int32_t x = 3 + (int32_t)i * 17, lv = i < 8u ? trk[GV_ROWTRK[i]].p[P_LEVEL] : gv.t[i - 8u].vol, h = lv * 96 / 127,
                    au = gv_heard((int)i), sel = gv_grp_on() ? gv_grp_has((int)i) : (int32_t)i == gv.sel;
            uint16_t c = gv_col((int)i);
            G_R(x, 58, 14, 98, GC_P);
            G_R(x + 4, 59 + 96 - h, 6, h, au ? c : GC_L);
            if (au && gv_flash((int)i)) G_R(x + 1, 59 + 96 - h, 12, h, c);      /* its hit: the whole fader lights */
            G_R(x, 59 + 96 - h - 2, 14, 4, au ? GC_T : GC_L);                     /* the fader's cap */
            if (sel) G_F(x - 1, 57, 16, 100, GC_T);
            G_TC(x + 7, 160, 1, AB[i], sel ? GC_T : au ? c : GC_L);
        }
        G_TC(120, 172, 1, "SELECT : PISTE OU GROUPE  TOUCHES : COUPER", GC_M);
        break; }
    case GP_LRYT:
        for (i = 0; i < 8u; i++) gv_tile((int)i, 3 + (int32_t)(i % 4u) * 59, 36 + (int32_t)(i / 4u) * 72, 56, 66, 1);
        break;
    case GP_LMEL:
        for (i = 0; i < 6u; i++) gv_tile((int)(8u + i), 3 + (int32_t)(i % 3u) * 79, 36 + (int32_t)(i / 3u) * 72, 76, 66, 1);
        break;
    case GP_EFA: case GP_EFB: case GP_EFC: case GP_EFD: case GP_EFE: case GP_EFF: case GP_EFG:
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
        G_T(4, 50, 1, GV_EFP(gu.page) ? gv_name(gv.sel) : FXT_N[gu.fxt], GV_EFP(gu.page) ? gv_col(gv.sel) : tc);
        gv_fmt(b, "PAGE %s", FXPN[gu.fxpg]);
        G_TR(194, 50, 1, b, GC_Y);
        for (i = 0; i < 7u; i++) G_R(198 + (int32_t)i * 6, 50, 4, 4, i == gu.fxpg ? GC_Y : GC_P2);
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
            case FE_PAN: {                                /* two speakers, the sound between them */
                int32_t px = x + 8 + v * (w - 16) / 127, bob = gv_sin((ms / 3u) & 255u) * 3 / 127;
                G_R(x + 4, cy - 12, 5, 24, v <= 64 ? ink : GC_P2); G_R(x + w - 9, cy - 12, 5, 24, v >= 64 ? ink : GC_P2);
                G_R(x + 10, cy, w - 20, 1, GC_L); G_R(cx, cy - 3, 1, 7, GC_L);
                cv_ring(px, cy + bob - gv_oy, 4, GC_Y); G_R(px - 1, cy + bob - 1, 3, 3, GC_Y);
                break; }
            case FE_SC: {                                 /* the level ducks on the kick, then comes back */
                uint32_t age = song.playing ? song.tick - gv_hit_t[0] : 99u;
                int32_t py = 0;
                for (q = 0; q < (uint32_t)w - 8u; q++) {
                    int32_t a = (int32_t)q + (int32_t)(age > 40u ? 40u : age), dk = a < 44 ? (44 - a) * v / 127 * 30 / 44 : 0, yy = cy - 14 + dk;
                    if (q) cv_line(x + 3 + (int32_t)q, py - gv_oy, x + 4 + (int32_t)q, yy - gv_oy, ink);
                    py = yy;
                }
                G_R(x + 4, cy + 20, 8, 8, gv_flash(0) ? GC_Y : GC_P2);   /* the kick */
                break; }
            case FE_TR: {                                 /* a wave whose height breathes an 1/8 note */
                int32_t py = 0, dp = 127 - (int32_t)((ph * 2u & 255u) < 128u ? (ph * 2u & 255u) : 255u - (ph * 2u & 255u)) * v / 127;
                for (q = 0; q < (uint32_t)w - 8u; q++) {
                    int32_t yy = cy - gv_sin((q * 20u + ms / 3u) & 255u) * dp / 127 * 20 / 127;
                    if (q) cv_line(x + 3 + (int32_t)q, py - gv_oy, x + 4 + (int32_t)q, yy - gv_oy, ink);
                    py = yy;
                }
                break; }
            case FE_TS: {                                 /* a reel: slower and slower, then still */
                static uint32_t ang, last;
                int32_t k2;
                ang += (ms - last) * (uint32_t)(127 - v) / 127u; last = ms;
                cv_ring(cx, cy - gv_oy, 16, ink); cv_ring(cx, cy - gv_oy, 4, v ? GC_Y : ink);
                for (q = 0; q < 3u; q++) {
                    k2 = (int32_t)((ang / 3u + q * 85u) & 255u);
                    cv_line(cx, cy - gv_oy, cx + gv_sin((uint32_t)k2) * 15 / 127, cy - gv_sin((uint32_t)(k2 + 64) & 255u) * 15 / 127 - gv_oy, v ? GC_Y : ink);
                }
                if (v >= 120) G_TC(cx, y + 6, 1, "STOP", GC_R);
                break; }
            case FE_AP: {                                 /* the sound swings between the two speakers */
                int32_t px = cx + gv_sin((ms * 64u / beat) & 255u) * v / 127 * (w / 2 - 10) / 127;
                G_R(x + 4, cy - 12, 5, 24, ink); G_R(x + w - 9, cy - 12, 5, 24, ink);
                G_R(x + 10, cy, w - 20, 1, GC_L);
                cv_ring(px, cy - gv_oy, 4, GC_Y); G_R(px - 1, cy - 1, 3, 3, GC_Y);
                break; }
            case FE_VB: case FE_RM: case FE_WF: case FE_ST: {   /* a wave: wobbling, times another, folded, rounded */
                int32_t py = 0;
                for (q = 0; q < (uint32_t)w - 8u; q++) {
                    uint32_t ang = (q * 9u + ms / 4u) & 255u;
                    int32_t yy;
                    if (e == FE_VB) ang = (ang + (uint32_t)(gv_sin((ms / 2u + q * 3u) & 255u) * v / 500)) & 255u;
                    yy = gv_sin(ang);
                    if (e == FE_RM) yy = yy * (127 - v + v * gv_sin((q * 40u + ms / 2u) & 255u) / 127) / 127;
                    if (e == FE_WF && v) { yy = yy * (8 + v / 2) / 8; yy = ((yy + 127) & 511); yy = yy < 255 ? yy - 127 : 383 - yy; }
                    if (e == FE_ST && v) { yy = yy * (32 + v) / 32; yy = gv_abs(yy) > 70 ? (yy < 0 ? -1 : 1) * (70 + (gv_abs(yy) - 70) / 4) : yy; yy = gv_clamp(yy, -110, 110); }
                    yy = cy - yy * 22 / 127;
                    if (q) cv_line(x + 3 + (int32_t)q, py - gv_oy, x + 4 + (int32_t)q, yy - gv_oy, e == FE_ST && v ? GC_Y : ink);
                    py = yy;
                }
                break; }
            case FE_PS: {                                 /* a scale of semitones, the mark on the one chosen */
                int32_t sm = v - 12;
                for (q = 0; q < 25u; q += 3u) G_R(cx - (q == 12u ? 12 : 6), cy + 24 - (int32_t)q * 2, q == 12u ? 24 : 12, 1, GC_L);
                G_R(cx - 14, cy + 24 - v * 2 - 1, 28, 3, sm ? GC_Y : ink);
                gv_fmt(b, "%+d", (int)sm); G_TC(cx, y + 6, 1, b, sm ? GC_Y : GC_M);
                break; }
            case FE_FO: {                                 /* a mouth: wide for A, round for O, its letter */
                static const uint8_t MW[5] = {18, 16, 20, 9, 6}, MH[5] = {12, 6, 3, 10, 5};
                static const char *const VW[5] = {"A", "E", "I", "O", "U"};
                int32_t k2 = v ? gv_clamp((v - 1) * 5 / 127, 0, 4) : 1, mw = MW[k2], mh = v ? MH[k2] + (int32_t)((ms / 120u) & 1u) : 1;
                G_F(cx - mw, cy - mh, mw * 2, mh * 2 + 1, ink);
                cv_ring(cx - 9, cy - 22 - gv_oy, 2, ink); cv_ring(cx + 9, cy - 22 - gv_oy, 2, ink);
                if (v) G_TC(cx, y + 6, 1, VW[k2], GC_Y);
                break; }
            case FE_NR:                                   /* noise climbing: more and higher as it goes up */
                for (q = 0; q < 24u; q++) {
                    uint32_t rnd = (q * 2654435761u + (ms / 60u) * 40503u) >> 8;
                    int32_t hh = v * (int32_t)(4u + q) / 127 * (int32_t)(40u + (rnd & 31u)) / 60;
                    if (hh > 0) G_R(x + 4 + (int32_t)q * 2, cy + 22 - hh, 1, hh, q > 17u ? GC_Y : ink);
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
    case GP_STY: {                                     /* the list, the style's scene (ARP held 2 s = take it) */
        int32_t top = gv_clamp((int32_t)gu.sty - 3, 0, GV_NSTY - 8), k;
        for (k = 0; k < 8; k++) {
            int32_t j = top + k, y = 36 + k * 17;
            if (j == gu.sty) { G_R(2, y, 112, 16, GC_P2); G_F(2, y, 112, 16, GC_T); }
            G_T(7, y + 5, 1, GV_STN[j], j == gv.style ? GC_Y : j == gu.sty ? GC_T : GC_M);
            if (j == gv.style) G_R(104, y + 6, 4, 4, GC_Y);
        }
        gv_stage(gu.sty, 118, 36, 120, 108, 1);
        G_F(118, 36, 120, 108, GC_L);
        gv_fmt(b, "%d BPM  %s", (int)GV_STBPM[gu.sty], GV_STSC[gu.sty] >= 0 ? GV_SCN[GV_STSC[gu.sty]] : "MINEUR");
        G_TC(178, 150, 1, b, GC_T);
        G_TC(178, 164, 1, gu.sty == gv.style ? "EN COURS" : "OCT- OU ARP 2 S = VALIDER", gu.sty == gv.style ? GC_Y : GC_C);
        gv_fmt(b, "%d/%d", (int)gu.sty + 1, GV_NSTY);
        G_TC(58, 174, 1, b, GC_M);
        break; }
    case GP_GRYT: {                                    /* the style, the 8 rows of the rhythm */
        uint32_t r, c;
        G_TC(120, 38, 3, GV_STN[gv.style], GC_C);
        gv_fmt(b, "%d BPM  SWING %d%%  AUTO %s", (int)song.g[G_BPM], (int)song.g[G_SWING], gv.autob ? "ON" : "OFF");
        G_TC(120, 64, 1, b, GC_M);
        for (r = 0; r < 8u; r++) {
            int32_t y = 76 + (int32_t)r * 12, sel = (int32_t)r == (gv.sel & 7);
            const track_t *tr = &trk[GV_ROWTRK[r]];
            G_T(4, y + 2, 1, GV_ROWN[r], sel ? GV_ROWC[r] : GC_L);
            for (c = 0; c < 16u; c++) G_R(40 + (int32_t)c * 12, y, 11, 10, tr->step[c].on ? GV_ROWC[r] : (c % 4u ? GC_P : GC_P2));
            if (sel) G_F(39, y - 1, 194, 12, GC_T);
        }
        if (song.playing) G_F(40 + gv_pos * 12 - 1, 74, 13, 100, GC_C);
        G_TC(120, 176, 1, gv.tpl ? "MOTIF DU STYLE" : "MOTIF A LA MAIN", GC_T);
        break; }
    case GP_GMEL: {                                    /* the chords, then the 6 tracks' melodies */
        const uint8_t *p = gv_progp();
        uint32_t k, c;
        if (p) for (i = 0; i < 4u; i++) {
            int32_t x = 14 + (int32_t)i * 54;
            int on = song.playing && gv_cix == i;
            G_R(x, 36, 50, 20, on ? GC_Y : GC_P2);
            if (!on) G_F(x, 36, 50, 20, GC_L);
            G_TC(x + 25, 42, 2, gv_degname(p[i]), on ? GC_BG : GC_T);
        } else G_TC(120, 42, 1, "ACCORDS OFF  TONIQUE FIXE", GC_M);
        for (k = 0; k < GV_NT; k++) {
            int32_t y = 62 + (int32_t)k * 19, sel = (int32_t)k + 8 == gv.sel, L = (int32_t)gv_L();
            G_R(36, y, 200, 17, sel ? GC_P2 : GC_P);
            G_T(2, y + 6, 1, GV_TRN[k], sel ? GV_TRC[k] : GC_L);
            for (c = 0; c < 16u; c++) { const gv_step_t *st = &gv.t[k].st[c]; if (st->n >= 0) G_R(37 + (int32_t)c * 12, y + 14 - st->n * 13 / (2 * L), 10, 3, gv.t[k].mute ? GC_L : GV_TRC[k]); }
            if (sel) G_F(36, y, 200, 17, GC_T);
        }
        if (song.playing) G_F(37 + gv_pos * 12 - 1, 60, 12, 116, GC_C);
        G_TC(120, 178, 1, gu.mbu ? "BANQUE : SAVE (TOUCHES 9-16)" : "BANQUE VIDE  SAVE = ENREGISTRER", GC_A);
        break; }
    case GP_ENV: {                                     /* the envelope's curve; the dot runs it when the track plays */
        uint16_t cc = gv_col(gv.sel);
        int32_t X = 12, Y = 40, W = 216, H = 92, x1 = X + 4 + t->att * 60 / 127, x2 = x1 + 20 + t->dec * 70 / 127, x3 = X + W - 30,
                ys = Y + H * 45 / 100 + 10, yb = Y + H - 8, el = (int32_t)(fm1_ms - gv_on_ms[(gv.sel - 8) & 7]);
        G_R(X, Y, W, H, GC_P);
        G_L2(X + 4, yb, x1, Y + 10, cc); G_L2(x1, Y + 10, x2, ys, cc); G_L2(x2, ys, x3, ys, cc); G_L2(x3, ys, X + W - 4, yb, cc);
        if (el >= 0 && el < 1200) {
            int32_t px = X + 4 + el * (W - 8) / 1200, py;
            py = px < x1 ? yb - (px - X - 4) * (yb - Y - 10) / (x1 - X - 3) : px < x2 ? Y + 10 + (px - x1) * (ys - Y - 10) / (x2 - x1 + 1) : px < x3 ? ys : ys + (px - x3) * (yb - ys) / (X + W - 3 - x3);
            G_D(px, py, 4, GC_T);
        }
        G_TC((X + 4 + x1) / 2, Y + H - 12, 1, "A", GC_M); G_TC((x1 + x2) / 2, Y + H - 12, 1, "D", GC_M);
        G_TC((x2 + x3) / 2, Y + H - 12, 1, "S", GC_M); G_TC((x3 + X + W) / 2, Y + H - 12, 1, "R", GC_M);
        G_T(X + 4, Y + H + 10, 1, "GLIDE", GC_M);
        { int32_t gl = t->gl * 80 / 127, i, py = 0; for (i = 0; i <= 90; i++) { int32_t yy = Y + H + 14 - (i < gl ? i * 8 / (gl + 1) : 8); if (i) G_L(X + 40 + i - 1, py, X + 40 + i, yy, GC_Y); py = yy; } }
        { static const char *const WV[4] = {"SCIE", "CARRE", "TRI", "PIANO"}; G_TR(X + W - 4, Y + H + 10, 1, WV[t->wave & 3u], cc); }
        break; }
    case GP_FLT: {                                     /* the filter's curve, the cutoff (yellow), the sound going through */
        uint16_t cc = gv_col(gv.sel);
        int32_t X = 10, Y = 40, W = 220, H = 100, i, py = 0, lfo = t->lfd ? gv_sin((uint32_t)(fm1_ms * (uint32_t)(26 + t->lfr * t->lfr * 5120 / 16129) / 1000u) & 255u) * t->lfd / 127 * 15 / 127 : 0,
                fc = gv_clamp(t->cut + lfo, 0, 127), q = t->res;
        G_R(X, Y, W, H, GC_P);
        for (i = 1; i < 5; i++) G_R(X + i * W / 5, Y, 1, H, GC_P2);
        for (i = 0; i <= W; i++) {
            int32_t f = i * 127 / W, d = f - fc, pk = gv_abs(d) < 8 ? q * (8 - gv_abs(d)) / 8 : 0, r;
            r = t->ftype == 1u ? (d > 0 ? 100 : gv_clamp(100 + d * 5, 0, 100)) : t->ftype == 2u ? gv_clamp(100 - gv_abs(d) * (4 + q / 12), 0, 100) : (d < 0 ? 100 : gv_clamp(100 - d * 5, 0, 100));
            r += pk * 90 / 127;
            { int32_t yy = Y + H - 8 - gv_clamp(r, 0, 190) * H * 42 / 10000; if (i) G_L2(X + i - 1, py, X + i, yy, cc); py = yy; }
        }
        for (i = 0; i < 26; i++) {
            int32_t f = (int32_t)((i * 35 + fm1_ms / 16u) % 256u), fx = f * 127 / 255, d = fx - fc,
                    a = t->ftype == 1u ? (d > 0 ? 100 : gv_clamp(100 + d * 5, 0, 100)) : t->ftype == 2u ? gv_clamp(100 - gv_abs(d) * 4, 0, 100) : (d < 0 ? 100 : gv_clamp(100 - d * 5, 0, 100));
            G_R(X + f * W / 256, Y + H - 10 - ((i * 37) % 60) * a / 100, 2, 2, a > 50 ? GC_T : GC_L);
        }
        for (i = Y + 4; i < Y + H - 4; i += 6) G_R(X + fc * W / 127, i, 1, 3, GC_Y);
        { static const char *const FT[3] = {"PASSE-BAS", "PASSE-HAUT", "BANDE"}; gv_fmt(b, "%s%s", FT[t->ftype % 3u], t->lfd ? "  LFO ACTIF" : ""); G_TC(120, Y + H + 6, 1, b, cc); }
        break; }
    case GP_LFO: {                                     /* the LFO's wave, moving at its speed; the cutoff it moves */
        uint16_t cc = gv_col(gv.sel);
        int32_t X = 10, Y = 40, W = 220, H = 80, i, py = 0, dep = 32 + t->lfd * 95 / 127,
                hz256 = t->lfs ? (int32_t)((uint32_t)song.g[G_BPM] * 256u / 60u) : 26 + t->lfr * t->lfr * 5120 / 16129,
                cyc = gv_clamp(hz256 * 12 / 2560, 1, 6), ph0 = (int32_t)(fm1_ms * (uint32_t)hz256 / 1000u) & 255, v = 0;
        G_R(X, Y, W, H, GC_P); G_R(X, Y + H / 2, W, 1, GC_P2);
        for (i = 0; i <= W; i++) {
            int32_t ph = (ph0 + i * 256 * cyc / W) & 255, w = t->lfw == 0u ? gv_sin((uint32_t)ph) : t->lfw == 1u ? 127 - gv_abs(ph - 128) * 254 / 128 : t->lfw == 2u ? (ph < 128 ? 127 : -127) : 127 - ph,
                    yy = Y + H / 2 - w * (H * 40 / 100) * dep / (127 * 127);
            if (i) G_L2(X + i - 1, py, X + i, yy, cc);
            py = yy; v = w;
        }
        G_D(X + W - 3, py, 4, GC_T);
        if (t->lfs) G_T(X + 6, Y + 6, 1, "CALE SUR LE TEMPO", GC_M);
        else { gv_fmt(b, "%d,%d HZ", (int)(hz256 / 256), (int)(hz256 % 256 * 10 / 256)); G_T(X + 6, Y + 6, 1, b, GC_M); }
        G_T(X, Y + H + 10, 1, "COUPURE", GC_M);
        { int32_t fc = gv_clamp(t->cut + v * t->lfd / 127 * 15 / 127, 0, 127); G_R(X + 50, Y + H + 9, 170, 7, GC_P2); G_R(X + 50, Y + H + 9, 170 * fc / 127, 7, cc); G_R(X + 50 + 170 * t->cut / 127 - 1, Y + H + 6, 2, 13, GC_T); }
        break; }
    case GP_OSC: {                                     /* the oscilloscope: the track's own sound */
        uint16_t cc = gv_col(gv.sel);
        int32_t X = 8, Y = 38, W = 224, H = 104, i, z = 0, pk = 1, py = 0;
        uint8_t w0 = (uint8_t)(gv_scw - 127u);
        gv_scope_trk = (uint8_t)((gv.sel - 8) & 7);
        G_R(X, Y, W, H, RGB(0x0b, 0x0d, 0x10));
        for (i = 1; i < 8; i++) G_R(X + i * W / 8, Y, 1, H, RGB(0x1d, 0x22, 0x28));
        G_R(X, Y + H / 2, W, 1, RGB(0x1d, 0x22, 0x28));
        for (i = 0; i < 127; i++) pk = gv_abs(gv_scope[(w0 + i) & 127]) > pk ? gv_abs(gv_scope[(w0 + i) & 127]) : pk;
        for (i = 1; i < 60; i++) if (gv_scope[(w0 + i - 1) & 127] < 0 && gv_scope[(w0 + i) & 127] >= 0) { z = i; break; }
        if (pk > 30 && gvv[(gv.sel - 8) & 7].on) {
            for (i = 0; i < W; i++) {
                int32_t smp = gv_scope[(w0 + z + i * (125 - z) / W) & 127], yy = Y + H / 2 - smp * (H * 42 / 100) / pk;
                if (i) G_L2(X + i - 1, py, X + i, yy, cc);
                py = yy;
            }
        } else G_TC(120, Y + H / 2 - 4, 1, "JOUE UNE NOTE OU PLAY", GC_M);
        { static const char *const WV[4] = {"SCIE", "CARRE", "TRI", "PIANO"}; gv_fmt(b, "%s  OCT %d  PITCH %+d", WV[t->wave & 3u], (int)t->oct, (int)t->po); G_TC(120, Y + H + 6, 1, b, cc); }
        break; }
    case GP_SON: {                                     /* the track's sounds: SELECT moves and loads, every key plays it */
        uint16_t ids[64];
        uint32_t cur, n = gv_snd_ids(ids, &cur);
        int32_t c = cur == 0xFFFFu ? 0 : (int32_t)cur, top = gv_clamp(c - 3, 0, (int32_t)n > 7 ? (int32_t)n - 7 : 0), k;
        gv_fmt(b, "SONS  %s", gv_name(gv.sel));
        G_T(6, 36, 2, b, gv_col(gv.sel));
        gv_fmt(b, "%d/%d", cur == 0xFFFFu ? 0 : c + 1, (int)n);
        G_TR(234, 38, 1, b, GC_M);
        for (k = 0; k < 7 && top + k < (int32_t)n; k++) {
            char nm[20];
            int32_t j = top + k, y = 54 + k * 17, on = cur != 0xFFFFu && j == c;
            gv_snd_name(ids[j], nm);
            if (on) { G_R(3, y, 234, 16, RGB(0x2c, 0x24, 0x1a)); G_F(3, y, 234, 16, GC_A); }
            G_R(5, y + 2, 3, 12, gv_col(gv.sel));
            G_T(14, y + 5, 1, nm, on ? GC_T : GC_M);
            if (gv_isd()) G_T(110, y + 5, 1, DMODELS[(ids[j] < PRE_NFACT ? PRE_FACTORY[ids[j]].model : upr.s[(ids[j] - PRE_NFACT) % PRE_NUSER].model) % NMODELS].name, GC_L);
            else if (ids[j] >= GV_NSND) G_T(110, y + 5, 1, "MES SONS", GC_L);
            if (on) G_TR(230, y + 5, 1, "EN COURS", GC_Y);
        }
        G_TC(120, 176, 1, "SELECT ECOUTER  OCT- VALIDER  OCT+ RETOUR  SAVE GARDER", GC_C);
        break; }
    case GP_FXD: case GP_SONR: case GP_SEDIT: {
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
        if (gu.page == GP_FXD) { uint32_t j; for (j = 0; j < 16u; j++) G_R(4 + (int32_t)j * 146 / 10, 172, 12, 8, gv_perf_on(j) ? (j < 8u ? GC_C : GC_Y) : GC_P2); }
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
    case GP_SCL: {                                     /* the scale's scene and the monster; its notes; the chords */
        int32_t k;
        gv_scene(gv.scale, 31, 110);
        gv_mon(200, 138, 2);
        gv_fmt(b, "%s %s", GV_NOTE[gv.root], GV_SCN[gv.scale % GV_NSC]);
        G_R(4, 35, G_TW(1, b) + 10, 12, RGB(0x10, 0x10, 0x14)); G_T(9, 38, 1, b, GC_T);
        for (k = 0; k < 12; k++) {
            int in = 0, j;
            for (j = 0; j < (int)gv_L(); j++) if (GV_SC[gv.scale % GV_NSC][j] == k) in = 1;
            G_R(1 + k * 20, 143, 18, 14, k == 0 ? GC_Y : in ? GC_A : GC_P2);
            G_TC(10 + k * 20, 147, 1, GV_NOTE[(gv.root + (uint32_t)k) % 12u], in || !k ? GC_BG : GC_L);
        }
        {
            const uint8_t *p = gv_progp();
            if (p) for (i = 0; i < 4u; i++) G_TC(36 + (int32_t)i * 56, 166, 2, gv_degname(p[i]), song.playing && gv_cix == i ? GC_Y : GC_A);
            else G_TC(120, 168, 1, "ACCORDS OFF  SELECT = CHANGER DE GAMME", GC_M);
        }
        break; }
    case GP_GLO: {                                     /* the tempo, then 8 options: PRESETS up/down, ALGORITHM left/right, SELECT the value */
        gv_fmt(b, "%d", (int)song.g[G_BPM]);
        G_TC(120, 38, 8, b, GC_Y);
        for (i = 0; i < 8u; i++) {
            char v[20];
            int32_t x = 4 + (int32_t)(i & 1u) * 117, y = 92 + (int32_t)(i >> 1) * 20, on = i == (gu.gcur & 7u);
            gv_glo_val(i, v);
            G_R(x, y, 115, 18, on ? GC_P2 : GC_P);
            if (on) G_F(x, y, 115, 18, GC_A);
            G_T(x + 4, y + 6, 1, GV_GOPT[i], on ? GC_T : GC_M);
            G_TR(x + 111, y + 6, 1, v, i >= 5u ? (on ? GC_A : GC_L) : on ? GC_Y : GC_T);
        }
        G_TC(120, 175, 1, "PRESETS HAUT/BAS  ALGO GAUCHE/DROITE  SELECT VALEUR", GC_M);
        break; }
    case GP_FXR: {                                     /* the list of the effects at work, each its bar (16 keys = cut it) */
        static const uint8_t AMT_K[16] = {0};
        int32_t act = 0;
        (void)AMT_K;
        for (i = 0; i < 15u; i++) act += gv_fxr_on(i);
        if (act) { gv_fmt(b, "%d EFFET(S) ACTIF(S)", (int)act); } else gv_fmt(b, "AUCUN EFFET ACTIF");
        G_T(6, 36, 1, b, act ? GC_Y : GC_M);
        G_TR(236, 36, 1, "TOUCHE = COUPER", GC_M);
        for (i = 0; i < 16u; i++) {
            int32_t x = 4 + (int32_t)(i % 2u) * 118, y = 46 + (int32_t)(i / 2u) * 17, on = gv_fxr_on(i), amt;
            static const char *const N[16] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16"};
            amt = i == 0u ? 127 - gv.mf : i == 1u ? gv.hp : i == 2u ? gv.mq : i == 3u ? gv.xd : i == 4u ? gv.xr : i == 5u ? gv.xs : on ? 90 : 0;
            G_R(x, y, 114, 15, on ? RGB(0x2c, 0x24, 0x1a) : GC_P);
            G_T(x + 3, y + 5, 1, N[i], GC_L);
            G_T(x + 16, y + 5, 1, K_FXR[i], on ? GC_T : GC_L);
            G_R(x + 80, y + 5, 30, 5, GC_P2);
            if (on) G_R(x + 80, y + 5, 30 * gv_clamp(amt, 8, 127) / 127, 5, i < 6u ? GC_A : i < 9u ? GC_C : GC_Y);
        }
        break; }
    case GP_FXA: {                                     /* the whole mix's filter: low-pass, high-pass, the resonance bumps */
        static const char *const TG[3] = {"TOUT", "RYTHME", "MELODIE"};
        int32_t X = 12, Y = 48, W = 216, H = 78, py = 0;
        gv_fmt(b, "FILTRE  PADS SUR : %s%s", TG[gv_ptg % 3u], gu.lock ? "  VERROU" : "");
        G_TC(120, 36, 1, b, GC_Y);
        G_R(X, Y, W, H, GC_P);
        for (i = 0; i <= (uint32_t)W; i++) {
            int32_t f = (int32_t)i * 127 / W, a = f > gv.mf ? (f - gv.mf) * 4 : 0, bb = f < gv.hp ? (gv.hp - f) * 4 : 0,
                    bump = (gv_abs(f - gv.mf) < 8 ? (8 - gv_abs(f - gv.mf)) : 0) + (gv.hp && gv_abs(f - gv.hp) < 8 ? (8 - gv_abs(f - gv.hp)) : 0),
                    yy = Y + 16 + gv_clamp((a + bb) * H / 127, 0, H - 20) - bump * gv.mq * 30 / (8 * 127);
            if (i) G_L2(X + (int32_t)i - 1, py, X + (int32_t)i, yy, GC_A);
            py = yy;
        }
        G_T(X + 4, Y + H - 10, 1, "COUPE-BAS", GC_M); G_TR(X + W - 4, Y + H - 10, 1, "COUPE-HAUT", GC_M);
        gv_perfmini(150);
        break; }
    case GP_FXB: {                                     /* echo repeats, the reverb's tail, the disto's curve */
        int32_t k, py = 0;
        G_TC(120, 36, 1, "ESPACE : ECHO  REVERB  DISTO", GC_Y);
        for (k = 0; k < 6; k++) { int32_t h = 60 * gv.xd / 127, kk; for (kk = 0; kk < k; kk++) h = h * 6 / 10; G_R(14 + k * 16, 118 - h, 10, h + 1, GC_C); }
        G_TC(44, 124, 1, "ECHO", GC_M);
        for (k = 0; k < 60; k++) { int32_t h = 50 * gv.xr / 127 * (60 - k) / 60 * (6 + 4 * (gv_abs(gv_sin((uint32_t)(k * 70) & 255u)) / 127)) / 10; G_R(112 + k, 118 - h, 1, h, GC_A); }
        G_TC(142, 124, 1, "REVERB", GC_M);
        for (k = 0; k <= 40; k++) { int32_t x = k * 254 / 40 - 127, a = 1 + gv.xs * 12 / 127, yy = gv_clamp(x * a, -127, 127); yy = 92 - yy * 24 / 127; if (k) G_L2(186 + k - 1, py, 186 + k, yy, GC_Y); py = yy; }
        G_TC(206, 124, 1, "DISTO", GC_M);
        gv_perfmini(150);
        break; }
    case GP_FXP: {                                     /* the 16 pads, big (8 x 2) */
        static const char *const W1[16] = {"LOOP", "LOOP", "LOOP", "LOOP", "ROLL", "ROLL", "ROLL", "REV", "CIBLE", "CIBLE", "CIBLE", "VERR.", "REC", "REJ.", "EFF.", "ZERO"},
                          *const W2[16] = {"1", "2", "4", "8", "X2", "X3", "X4", "<<", "TOUT", "RYT", "MEL", "", "", "", "", ""};
        for (i = 0; i < 16u; i++) {
            int32_t x = 4 + (int32_t)(i % 8u) * 29, y = 44 + (int32_t)(i / 8u) * 42, on = gv_perf_on(i);
            uint16_t c = i < 8u ? GC_C : GC_Y;
            G_R(x, y, 27, 36, on ? c : GC_P2);
            if (!on) G_F(x, y, 27, 36, GC_L);
            G_TC(x + 14, y + 10, 1, W1[i], on ? GC_BG : GC_T);
            G_TC(x + 14, y + 22, 1, W2[i], on ? GC_BG : GC_M);
        }
        G_TC(120, 140, 1, gv_recq ? "REC : TOURNE LES KNOBS, ILS S'ENREGISTRENT" : gv_am_any() ? (gv_am_on ? "MOUVEMENTS EN BOUCLE : LECTURE" : "MOUVEMENTS : PAUSE") : "MAINTIENS LES PADS  REC = ENREGISTRER", gv_recq ? GC_R : GC_M);
        break; }
    case GP_MIX: {                                     /* volume, disto, echo, reverb of the track (drums: + auto, the kit) */
        int d = gv_isd(), hk = gv_flash(gv.sel);
        uint16_t cc = gv_col(gv.sel);
        const track_t *r = gv_rt();
        static const char *const LM[4] = {"VOLUME", "DISTO", "DELAY", "REVERB"}, *const LD[4] = {"VOLUME", "AUTO", "VOL BATT", "REV BATT"};
        int32_t vals[4];
        uint32_t kinds[4];
        if (gv_grp_on()) cc = gv_grpc();
        G_TC(120, 38, 2, gv_grp_on() ? GV_GRPN[gu.grp] : gv_name(gv.sel), cc);
        if (gv_grp_on()) { d = 0; for (i = 0; i < 4u; i++) { vals[i] = gv_grp_get(i); kinds[i] = i; } }
        else if (d) { vals[0] = r->p[P_LEVEL]; vals[1] = gv.rowar[gv.sel & 7]; vals[2] = gv_kget(KN_DVOL); vals[3] = gv_kget(KN_DREV); kinds[0] = 0; kinds[1] = 4; kinds[2] = 0; kinds[3] = 3; }
        else { vals[0] = t->vol; vals[1] = t->dist; vals[2] = t->dly; vals[3] = t->rev; kinds[0] = 0; kinds[1] = 1; kinds[2] = 2; kinds[3] = 3; }
        for (i = 0; i < 4u; i++) {
            int32_t x = 4 + (int32_t)i * 59;
            gv_cell(x, 52, 56, 92, kinds[i], vals[i], cc, hk);
            G_TC(x + 28, 148, 1, d ? LD[i] : LM[i], GC_M);
        }
        break; }
    case GP_LRR: {                                     /* the loop going round, the roll repeating, the way */
        int d = gv_isd();
        uint8_t lp = d ? gv.rlp[gv.sel & 7] : t->lp, rl = d ? gv.rrl[gv.sel & 7] : t->rl, bw = d ? 0 : t->bw, rv = d ? gv.rrv[gv.sel & 7] : t->rv;
        uint32_t sd = 15000u / (uint32_t)gv_clamp(song.g[G_BPM], 40, 240), cs = song.playing ? gv_pos : (fm1_ms / sd) & 15u,
                 pos = lp ? (fm1_ms / sd) % lp : (bw || rv ? 15u - cs : cs);
        int32_t k;
        gv_fmt(b, "%s%s", gv_name(gv.sel), bw || rv ? "  < SENS INVERSE" : "");
        G_TC(120, 38, 1, b, GC_T);
        for (i = 0; i < 16u; i++) G_R(8 + (int32_t)i * 14, 52, 12, 26, i == pos ? (lp ? GC_C : GC_A) : lp && i < lp ? RGB(0x17, 0x4a, 0x41) : (i % 4u ? GC_P : GC_P2));
        if (lp) { int32_t x2 = 8 + lp * 14 - 2, a = (int32_t)((fm1_ms % (sd * lp)) * (uint32_t)(x2 - 8) / (sd * lp)); G_L(8, 86, x2, 86, GC_C); G_D(8 + a, 86, 3, GC_C); }
        gv_fmt(b, lp ? "LOOP %d PAS" : "LOOP OFF", (int)lp);
        G_T(8, 100, 2, b, lp ? GC_C : GC_M);
        for (k = 0; k < 4; k++) {
            int act = k < (rl ? rl : 1), lit = act && (int32_t)(((fm1_ms % sd) * (rl ? rl : 1)) / sd) == k;
            G_R(140 + k * 24, 96, 18, 18, lit ? GC_Y : act ? RGB(0x5a, 0x4a, 0x12) : GC_P2);
        }
        gv_fmt(b, rl > 1 ? "ROLL X%d" : "ROLL OFF", (int)rl);
        G_T(140, 120, 1, b, rl > 1 ? GC_Y : GC_M);
        break; }
    case GP_VIS: {                                     /* the style's scene, the monster dancing, the chords */
        const uint8_t *p = gv_progp();
        gv_stage(gu.sty, 0, 31, 240, 140, 3);
        G_R(4, 35, G_TW(2, GV_STN[gu.sty]) + 10, 17, RGB(0x10, 0x10, 0x14)); G_T(9, 39, 2, GV_STN[gu.sty], gu.sty == gv.style ? GC_T : GC_C);
        gv_fmt(b, "M%d", (int)(gv_bar & 3u) + 1); G_R(200, 35, 36, 17, RGB(0x10, 0x10, 0x14)); G_TC(218, 39, 2, b, GC_Y);
        if (p) for (i = 0; i < 4u; i++) G_TC(36 + (int32_t)i * 56, 174, 1, gv_degname(p[i]), song.playing && gv_cix == i ? GC_Y : GC_T);
        break; }
    case GP_LIBR: {                                    /* the rhythm base: its motifs by genre, the bank's 8; what the cursor's holds */
        int32_t top = gv_clamp((int32_t)gu.lcur - 3, 0, (int32_t)GV_LCN - 8), k, r, c, n = 0, of = 0;
        for (k = 0; k < 8; k++) {
            int32_t j = top + k, y = 36 + k * 17, used = j < (int32_t)GV_NLIB || ((gu.rbu >> (j - (int32_t)GV_NLIB)) & 1u);
            int32_t on = j < (int32_t)GV_NLIB ? (int32_t)gv.libk == j + 1 : used && gu.rcur == j - (int32_t)GV_NLIB && !gv.libk && !gv.tpl;
            if (j == gu.lcur) { G_R(2, y, 122, 16, GC_P2); G_F(2, y, 122, 16, GC_T); }
            if (j < (int32_t)GV_NLIB) {
                int32_t q, i2 = 1;
                for (q = 0; q < j; q++) if (GV_LIB[q].g == GV_LIB[j].g) i2++;
                gv_fmt(b, "%s %d", GV_LGN[GV_LIB[j].g % GV_NLG], (int)i2);
            } else gv_fmt(b, used ? "BANQUE %d" : "BANQUE %d (VIDE)", (int)(j - (int32_t)GV_NLIB) + 1);
            G_T(7, y + 5, 1, b, on ? GC_Y : j == gu.lcur ? GC_T : j < (int32_t)GV_NLIB && gv_lib_fits((uint32_t)j) ? GC_C : GC_M);
            if (on) G_R(116, y + 6, 4, 4, GC_Y);
        }
        G_R(128, 36, 110, 74, GC_P); G_F(128, 36, 110, 74, GC_L);
        for (r = 0; r < 8; r++) {
            uint16_t bits = gu.lcur < GV_NLIB ? GV_LIB[gu.lcur].on[r] : ((gu.rbu >> (gu.lcur - GV_NLIB)) & 1u) ? gu.rb[(gu.lcur - GV_NLIB) & 7u][r] : 0u,
                     ac = gu.lcur < GV_NLIB ? GV_LIB[gu.lcur].ac[r] : 0u;
            for (c = 0; c < 16; c++) {
                if ((bits >> (15 - c)) & 1u) G_R(132 + c * 6 + c / 4, 40 + r * 8, 5, (ac >> (15 - c)) & 1u ? 7 : 5, GV_ROWC[r]);
                else if (!(c & 3)) G_R(132 + c * 6 + c / 4, 44 + r * 8, 5, 1, GC_P2);
            }
        }
        for (k = 0; k < (int32_t)GV_NLIB; k++) if (gv_lib_fits((uint32_t)k)) { n++; if (k < (int32_t)gu.lcur) of++; }
        if (gu.lcur < GV_NLIB) { gv_fmt(b, "%s  %d BPM", GV_LGN[GV_LIB[gu.lcur].g % GV_NLG], (int)GV_LIB[gu.lcur].bpm); G_TC(183, 116, 1, b, GC_T); }
        else G_TC(183, 116, 1, "BANQUE (SAVE)", GC_T);
        gv_fmt(b, "%d POUR %s", (int)n, GV_STN[gv.style]); G_TC(183, 128, 1, b, n ? GC_C : GC_M);
        G_TC(183, 142, 1, "1-8 OU OCT- : JOUER", GC_Y);
        G_TC(183, 154, 1, "PRESETS : GENRE", GC_M);
        G_TC(183, 166, 1, "SAVE 2 S : GARDER", GC_M);
        gv_fmt(b, "%d/%d", (int)gu.lcur + 1, (int)GV_LCN); G_TC(62, 174, 1, b, GC_M);
        (void)of;
        break; }
    case GP_GSET: {                                    /* where new rhythms and melodies come from, and how often they change */
        static const char *const RS[4] = {"GENERATION : LES MODELES DU STYLE", "BASE : LES MOTIFS DU STYLE", "BASE+ : LES MOTIFS DE TOUS LES GENRES", "BANQUE : TES RYTHMES GARDES, A LA SUITE"};
        char v[20];
        G_T(6, 40, 1, "RYTHME", GC_Y);
        G_T(6, 52, 1, RS[gv.rsrc & 3u], GC_T);
        gv_kfmt(KN_AUTO, gv_kget(KN_AUTO), v); gv_fmt(b, "CHANGE TOUT SEUL : %s", v); G_T(6, 64, 1, b, gv.autob ? GC_C : GC_M);
        gv_fmt(b, "BASE : %d MOTIFS  BANQUE : %d/8", (int)GV_NLIB, (int)__builtin_popcount(gu.rbu)); G_T(6, 76, 1, b, GC_M);
        G_R(4, 90, 232, 1, GC_L);
        G_T(6, 98, 1, "MELODIE", GC_Y);
        G_T(6, 110, 1, gv.msrc ? "BANQUE : TES MELODIES GARDEES, A LA SUITE" : "GENERATION : DANS LA GAMME, SUR LES ACCORDS", GC_T);
        if (gv_isd()) gv_fmt(b, "AUTO : CHOISIS UNE PISTE MELODIQUE");
        else { gv_kfmt(KN_TAR, gv_kget(KN_TAR), v); gv_fmt(b, "%s CHANGE TOUT SEUL : %s", gv_name(gv.sel), v); }
        G_T(6, 122, 1, b, GC_M);
        gv_fmt(b, "BANQUE : %d/8", (int)__builtin_popcount(gu.mbu)); G_T(6, 134, 1, b, GC_M);
        G_R(4, 148, 232, 1, GC_L);
        G_TC(120, 156, 1, "LA BANQUE EST GARDEE A L'ARRET (STOP)", GC_M);
        G_TC(120, 168, 1, "SAVE 2 S = GARDER CE QUI JOUE", GC_M);
        break; }
    case GP_BKR: case GP_BKM: {                        /* 8 slots, each a small picture of what it holds */
        int mel = gu.page == GP_BKM;
        static const char *const MD[3] = {"TOUCHES 1-8 : CHARGER (VIDE : ENREGISTRER)", "TOUCHES 1-8 : ENREGISTRER", "TOUCHES 1-8 : EFFACER"};
        G_TC(120, 36, 1, MD[gu.bmode % 3u], gu.bmode == 2u ? GC_R : GC_Y);
        for (i = 0; i < 8u; i++) {
            int32_t x = 3 + (int32_t)(i % 4u) * 59, y = 48 + (int32_t)(i / 4u) * 62, used = (int32_t)(((mel ? gu.mbu : gu.rbu) >> i) & 1u),
                    cur = used && (int32_t)i == (mel ? gu.mcur : gu.rcur), c, r;
            G_R(x, y, 56, 58, used ? GC_P2 : GC_P);
            G_F(x, y, 56, 58, cur ? (mel ? GC_A : GC_Y) : GC_P2);
            gv_fmt(b, "%d", (int)i + 1);
            G_T(x + 4, y + 4, 2, b, cur ? GC_Y : GC_M);
            if (!used) { G_TC(x + 28, y + 34, 1, "VIDE", GC_L); continue; }
            if (mel) { for (c = 0; c < 16; c++) { const gv_step_t *st = &gu.mb[i][c]; if (st->n >= 0) G_R(x + 4 + c * 3, y + 52 - gv_clamp(st->n, 0, 14) * 2, 3, 2, gv_col(gv.sel < 8 ? 8 : gv.sel)); } }
            else for (r = 0; r < 8; r++) for (c = 0; c < 16; c++) if ((gu.rb[i][r] >> (15 - c)) & 1u) G_R(x + 4 + c * 3, y + 22 + r * 4, 2, 3, GV_ROWC[r]);
        }
        G_TC(120, 176, 1, mel ? "LES MELODIES DE LA PISTE CHOISIE" : "LES RYTHMES (8 LIGNES)", GC_M);
        break; }
    case GP_EMIDI: {                                   /* the tracks and where they play */
        gv_fmt(b, "HORLOGE %s", GP[G_CLOCK].names[song.g[G_CLOCK]]);
        G_T(6, 36, 1, b, GC_C);
        for (i = 0; i < 14u; i++) {
            int32_t x = 3 + (int32_t)(i / 7u) * 119, y = 48 + (int32_t)(i % 7u) * 18, on = (int32_t)i == gv.sel;
            if (on) { G_R(x, y, 115, 16, GC_P2); G_F(x, y, 115, 16, GC_T); }
            G_R(x + 2, y + 2, 3, 12, gv_col((int)i));
            G_T(x + 9, y + 5, 1, gv_name((int)i), gv_col((int)i));
            if (i < 8u) { gv_fmt(b, "CH %d", (int)song.g[G_DRCH]); G_TR(x + 111, y + 5, 1, b, GC_T); }
            else { const gv_trk_t *mt = &gv.t[i - 8u]; static const char *const O[3] = {"INTERNE", "CH %d", "CH %d + INT"}; gv_fmt(b, O[mt->mo % 3u], (int)mt->mch); G_TR(x + 111, y + 5, 1, b, mt->mo ? GC_Y : GC_M); }
        }
        G_TC(120, 177, 1, "SELECT : LA PISTE  KNOB 1 CANAL  KNOB 2 SORTIE", GC_M);
        break; }
    case GP_MIDI: {                                    /* the list: SELECT moves, PRESETS changes the value */
        int32_t top = gv_clamp((int32_t)gu.mrow - 2, 0, (int32_t)GV_MROWS - 5), k;
        G_T(6, 36, 1, "USB MIDI : PAR LE CABLE USB", GC_C);
        for (k = 0; k < 5; k++) {
            char l[32], v[20];
            int32_t r = top + k, y = 48 + k * 25, on = r == (int32_t)gu.mrow;
            gv_midi_row((uint32_t)r, 0, l, v);
            G_R(6, y, 220, 22, on ? GC_P2 : GC_P);
            if (on) G_F(6, y, 220, 22, GC_T);
            if (r >= 3) G_R(8, y + 2, 3, 18, GV_TRC[((r - 3) / 2) % GV_NT]);
            G_T(15, y + 8, 1, l, on ? GC_T : GC_M);
            G_TR(220, y + 5, 2, v, on ? GC_Y : GC_T);
        }
        G_R(230, 48, 4, 122, GC_P); G_R(230, 48 + (int32_t)gu.mrow * 110 / (int32_t)(GV_MROWS - 1u), 4, 12, GC_A);   /* where in the list */
        G_TC(120, 176, 1, "PRESETS : HAUT / BAS   ALGORITHM : VALEUR", GC_M);
        break; }
    case GP_SAVE: {                                    /* the bank: 8 rhythms, 8 melodies (RAM: until power-off) */
        static const char *const MD[3] = {"TOUCHE : CHARGER (VIDE : ENREGISTRER)", "TOUCHE : ENREGISTRER", "TOUCHE : EFFACER"};
        G_TC(120, 36, 1, MD[gu.bmode % 3u], gu.bmode == 2u ? GC_R : GC_Y);
        for (i = 0; i < 16u; i++) {
            int32_t x = 3 + (int32_t)(i % 4u) * 59, y = 48 + (int32_t)(i / 4u) * 32, mel = i >= 8u, k = (int32_t)(i & 7u),
                    used = (int32_t)(((mel ? gu.mbu : gu.rbu) >> k) & 1u), cur = used && k == (mel ? gu.mcur : gu.rcur);
            G_R(x, y, 56, 29, cur ? (mel ? GC_A : GC_Y) : used ? GC_P2 : GC_P);
            if (!used) G_F(x, y, 56, 29, GC_P2);
            gv_fmt(b, "%s%d", mel ? "M" : "R", k + 1);
            G_T(x + 4, y + 4, 1, b, cur ? GC_BG : GC_M);
            G_TC(x + 28, y + 16, 1, used ? (mel ? "MELODIE" : "RYTHME") : "VIDE", cur ? GC_BG : used ? GC_T : GC_L);
        }
        G_TC(120, 178, 1, "1-8 RYTHMES  9-16 MELODIES DE LA PISTE", GC_M);
        break; }
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
/* the power-on intro, 3.2 s (any key ends it): the spiral in two palettes that swap, rainbow rings growing from the
 * middle, the alien big and dancing, the name */
#define GV_INTRO_MS 3200u
static uint16_t gv_hue(uint32_t h)                    /* 0..255 round the colour wheel */
{
    uint32_t k = (h & 255u) * 6u, f = k & 255u, q = 255u - f;
    switch (k >> 8) {
    case 0: return RGB(255, f, 0); case 1: return RGB(q, 255, 0); case 2: return RGB(0, 255, f);
    case 3: return RGB(0, q, 255); case 4: return RGB(f, 0, 255); default: return RGB(255, 0, q);
    }
}
static void gv_draw_intro(void)
{
    uint32_t ms = (int32_t)(fm1_ms - gu.intro_t) > 0 ? fm1_ms - gu.intro_t : 0u, k;
    gv_stage((ms / 400u) & 1u ? 18u : 3u, 0, 0, 240, 240, 0);
    for (k = 0; k < 6u; k++) {
        int32_t r = (int32_t)((ms / 7u + k * 26u) % 156u);
        cv_ring(120, 112 - gv_oy, r, gv_hue(ms / 5u + k * 43u));
        cv_ring(120, 112 - gv_oy, r + 1, gv_hue(ms / 5u + k * 43u + 20u));
    }
    gv_mon(120, 186, ms < 600u ? 2 + (int32_t)(ms / 300u) : 4);
    G_R(20, 8, 200, 30, GC_BG); G_F(20, 8, 200, 30, gv_hue(ms / 4u));
    G_TC(120, 14, 3, "FM-1 GROOVE", gv_hue(ms / 4u + 128u));
    if (ms > 1200u && ((ms / 300u) & 1u)) G_TC(120, 226, 1, "UNE TOUCHE POUR COMMENCER", GC_T);
}
/* the whole screen (ui_draw.c, first thing): 1 = drawn by Groove */
static int gv_ui_draw(void)
{
    uint32_t s;
    if (!gv_on) return 0;
    if (gu.intro_t && (int32_t)(fm1_ms - gu.intro_t) >= (int32_t)GV_INTRO_MS) { gu.intro_t = 0; gu.dirty = 1; }
    if (!gu.dirty && fm1_ms - gu.last_draw < (gu.intro_t ? 50u : 100u)) return 1;   /* 10 frames/s for the playhead and the flashes (the intro: 20) */
    gu.dirty = 0;
    gu.last_draw = fm1_ms;
    for (s = 0; s < 2u; s++) {
        gv_oy = (int32_t)s * 120;
        cv_begin(240, 120, GC_BG);
        if (gu.intro_t) { gv_draw_intro(); cv_blit(0, (uint32_t)gv_oy); continue; }
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
        case GP_MIXR:
        case GP_LIVE: on = j < 14u ? gv_heard((int)j) : !((gv.gmute >> (j - 14u)) & 1u); break;
        case GP_EFA: case GP_EFB: case GP_EFC: case GP_EFD: case GP_EFE: case GP_EFF: case GP_EFG:
        case GP_LFX: on = j < 8u ? (j == gu.fxt && gu.page == GP_LFX) : (gu.fxpg >> 1) * 8u + j - 8u < FE_N && gv_fxon((gu.fxpg >> 1) * 8u + j - 8u) && FE_OFF[(gu.fxpg >> 1) * 8u + j - 8u] >= 0; break;
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
        case GP_SAVE: on = (int)((j < 8u ? gu.rbu : gu.mbu) >> (j & 7u)) & 1; break;
        case GP_BKR: on = j < 8u && ((gu.rbu >> j) & 1u); break;
        case GP_BKM: on = j < 8u && ((gu.mbu >> j) & 1u); break;
        case GP_LIBR: on = j <= 8u; break;
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
        t->pan = 64; t->vol = VOL[i]; t->oct = OCT[i]; t->wave = WAV[i]; t->cut = CUT[i]; t->res = RES[i];
        t->mch = (uint8_t)(i + 1u); t->mo = 0;
        t->env = 76; t->dec = 38; t->dly = DLY[i]; t->rev = REV[i]; t->lfr = 38;
        gv_mel_clear(t);
    }
    drum_set_model(&trk[7], DM_CONGA);
    memset(gu.byp, 0xFF, sizeof gu.byp);
    gv.mf = 127;                                      /* FILTRE DJ open */
    gv.eq[0] = gv.eq[1] = gv.eq[2] = 64;              /* the equaliser flat */
    gu.intro_t = fm1_ms ? fm1_ms : 1u;                /* the intro, once (never later than now: the test is now - then) */
    gv_store_load();                                  /* the bank and the sources kept in flash */
    gv_set_style(gv.style);
    gv_gen_rhythm();
    gv_gen_mel(0);
    gv_gen_mel(1);
}
