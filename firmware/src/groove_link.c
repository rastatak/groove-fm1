/* SPDX-License-Identifier: GPL-3.0-only
 * Groove: 2026 */
/* Groove link: the song, the rhythm and the melodies as the web Studio sends and reads them over USB (SysEx, in the
 * editor protocol's framing: editor_groove.c is the hook, web/studio/link.js the page's side). Here only bytes in and
 * out, every one 0..127, so the simulator checks it (tests/sim_groove.c) and the Studio's own test runs it (gx_link).
 *   song    style, bpm u14, root, scale, prog, rhythm bars (1 2 4), track shown, playing, 6 x a track's bars
 *   rhythm  bars, then per bar per row (8): its 16 steps on (3 bytes: bits 0-6, 7-13, 14-15; bit 15 = step 1), its
 *           accents the same
 *   melody  track 0..5, bars, then per step: degree (0..28, 127 none), quarter steps long (1..64),
 *           flags (bits 0-1 quarter steps late, bit 2 accent, bit 3 slide)
 * A setter gives 0, or 1 a bad length, 2 a value out of range (nothing changed then). Main loop only. */
#define GV_LK_VER 1u
#define GV_LK_SONG_N 15u
#define GV_LK_KEEP 0x7Fu            /* song: "leave it as it is" in a 7-bit field (bpm: 0x3FFF) */
static void gv_lk_p16(uint8_t *o, uint32_t v) { o[0] = (uint8_t)(v & 0x7Fu); o[1] = (uint8_t)((v >> 7) & 0x7Fu); o[2] = (uint8_t)((v >> 14) & 3u); }
static uint32_t gv_lk_g16(const uint8_t *a) { return (uint32_t)a[0] | (uint32_t)a[1] << 7 | ((uint32_t)a[2] & 3u) << 14; }
static int gv_lk_bars(uint32_t b) { return b == 1u || b == 2u || b == 4u; }

static uint32_t gv_lk_song_get(uint8_t *o)
{
    uint32_t i, bpm = (uint32_t)song.g[G_BPM];
    o[0] = gv.style; o[1] = (uint8_t)(bpm & 0x7Fu); o[2] = (uint8_t)((bpm >> 7) & 0x7Fu);
    o[3] = gv.root; o[4] = gv.scale; o[5] = gv.prog; o[6] = (uint8_t)(gv_rlen() / 16u);
    o[7] = (uint8_t)gv.sel; o[8] = song.playing ? 1u : 0u;
    for (i = 0; i < GV_NT; i++) o[9u + i] = (uint8_t)(gv_tlen(i) / 16u);
    return GV_LK_SONG_N;
}
/* a: style, bpm u14, root, scale, prog; GV_LK_KEEP (bpm 0x3FFF) leaves one as it is. The notes keep their place in
 * the scale (the Studio sends its own degrees after), so the scale is set as it is, not moved to (gv_set_scale) */
static int gv_lk_song_set(const uint8_t *a, uint32_t n)
{
    uint32_t bpm;
    if (n != 6u) return 1;
    bpm = (uint32_t)a[1] | (uint32_t)a[2] << 7;
    if ((a[0] != GV_LK_KEEP && a[0] >= GV_NSTY) || (bpm != 0x3FFFu && (bpm < 40u || bpm > 240u)) ||
        (a[3] != GV_LK_KEEP && a[3] >= 12u) || (a[4] != GV_LK_KEEP && a[4] >= GV_NSC)) return 2;
    if (a[5] != GV_LK_KEEP && a[5] > GV_NPROG[a[4] != GV_LK_KEEP ? a[4] : gv.scale % GV_NSC]) return 2;
    if (a[0] != GV_LK_KEEP) gv.style = a[0];
    if (bpm != 0x3FFFu) song.g[G_BPM] = (int16_t)bpm;
    if (a[3] != GV_LK_KEEP) gv.root = a[3];
    if (a[4] != GV_LK_KEEP) { gv.scale = a[4]; if (gv.prog > GV_NPROG[gv.scale]) gv.prog = GV_NPROG[gv.scale]; }
    if (a[5] != GV_LK_KEEP) gv.prog = a[5];
    return 0;
}

static uint32_t gv_lk_ryt_get(uint8_t *o)
{
    uint32_t bars = gv_rlen() / 16u, b, r, c, n = 1;
    o[0] = (uint8_t)bars;
    for (b = 0; b < bars; b++)
        for (r = 0; r < 8u; r++) {
            const track_t *t = &trk[GV_ROWTRK[r]];
            uint32_t ac = 0;
            for (c = 0; c < 16u; c++) if (t->step[b * 16u + c].on && t->step[b * 16u + c].acc) ac |= 1u << (15u - c);
            gv_lk_p16(o + n, gv_row_getb(r, b)); gv_lk_p16(o + n + 3u, ac); n += 6u;
        }
    return n;
}
static int gv_lk_ryt_set(const uint8_t *a, uint32_t n)
{
    uint32_t bars, b, r, k = 1;
    uint16_t keep = gv_rs_acc;
    if (n < 1u) return 1;
    bars = a[0];
    if (!gv_lk_bars(bars)) return 2;
    if (n != 1u + bars * 48u) return 1;
    for (b = 1; b < n; b++) if (a[b] > 0x7Fu || ((b - 1u) % 3u == 2u && a[b] > 3u)) return 2;
    gv_set_rlen(bars * 16u);
    for (b = 0; b < bars; b++)
        for (r = 0; r < 8u; r++, k += 6u) { gv_rs_acc = (uint16_t)gv_lk_g16(a + k + 3u); gv_row_setb(r, b, (uint16_t)gv_lk_g16(a + k)); }
    gv_rs_acc = keep;
    gv.tpl = 0;
    return 0;
}

static uint32_t gv_lk_mel_get(uint32_t i, uint8_t *o)
{
    uint32_t len, c;
    i %= GV_NT; len = gv_tlen(i);
    o[0] = (uint8_t)i; o[1] = (uint8_t)(len / 16u);
    for (c = 0; c < len; c++) {
        const gv_step_t *s = &gv.t[i].st[c];
        o[2u + c * 3u] = (uint8_t)(s->n < 0 ? 0x7F : s->n > 28 ? 28 : s->n);
        o[3u + c * 3u] = (uint8_t)(s->l4 < 1u ? 1u : s->l4 > 64u ? 64u : s->l4);
        o[4u + c * 3u] = (uint8_t)((s->o4 & 3u) | (s->a ? 4u : 0u) | (s->s ? 8u : 0u));
    }
    return 2u + len * 3u;
}
static int gv_lk_mel_set(const uint8_t *a, uint32_t n)
{
    uint32_t i, bars, len, c;
    if (n < 2u) return 1;
    i = a[0]; bars = a[1];
    if (i >= GV_NT || !gv_lk_bars(bars)) return 2;
    len = bars * 16u;
    if (n != 2u + len * 3u) return 1;
    for (c = 0; c < len; c++) {
        const uint8_t *q = a + 2u + c * 3u;
        if ((q[0] > 28u && q[0] != 0x7Fu) || q[1] < 1u || q[1] > 64u || q[2] > 15u) return 2;
    }
    fm1_irq_off();
    gv_mel_wipe(&gv.t[i]);
    gv.t[i].len = (uint8_t)len;
    for (c = 0; c < len; c++) {
        const uint8_t *q = a + 2u + c * 3u;
        gv_step_t *s = &gv.t[i].st[c];
        s->n = (int8_t)(q[0] == 0x7Fu ? -1 : q[0]); s->l4 = q[1]; s->o4 = q[2] & 3u; s->a = (q[2] >> 2) & 1u; s->s = (q[2] >> 3) & 1u;
    }
    fm1_irq_on();
    return 0;
}
