/* SPDX-License-Identifier: GPL-3.0-only
 * Groove: 2026 */
/* Groove link over USB: the web Studio (web/studio/link.js) reads and sends the song, the rhythm and the melodies in
 * the editor protocol's frames (editor_proto.h: F0 7D 48 54 cmd seq ... F7; a reply: cmd, seq, rc, then its data).
 * Included by felucca.c after the editor's other parts, before editor.c, which calls ed_groove_cmd first. The bytes
 * themselves: groove_link.c. Commands 0x30.. (the editor's own stop at 27, its pushes are 0x40..):
 *   0x30 GV_INFO      -            -> rc, 'G' 'V', version, the song (15 bytes)
 *   0x31 GV_SONG      6 bytes      -> rc, the song as it is now           (write)
 *   0x32 GV_RYT_GET   -            -> rc, the rhythm
 *   0x33 GV_RYT_SET   the rhythm   -> rc, its bars                        (write)
 *   0x34 GV_MEL_GET   track        -> rc, the melody
 *   0x35 GV_MEL_SET   the melody   -> rc, track, its bars                 (write)
 * Only while Groove runs (gv_on): else rc 2 "NOT IN GROOVE". Main loop only. */
#include "editor_proto.h"
#define ED_HAS_GROOVE 1
enum { ED_GV_INFO = 0x30, ED_GV_SONG, ED_GV_RYT_GET, ED_GV_RYT_SET, ED_GV_MEL_GET, ED_GV_MEL_SET, ED_GV_END };
static int ed_groove_write(uint32_t cmd) { return cmd == ED_GV_SONG || cmd == ED_GV_RYT_SET || cmd == ED_GV_MEL_SET; }
static uint8_t ed_gv_buf[2u + GV_MAXS * 3u + 8u] __attribute__((section(".pool")));   /* (the pool: RAM is short) */
static void ed_gv_out(uint32_t cmd, const uint8_t *p, uint32_t n)
{
    uint32_t k;
    ed_reply(cmd, ED_OK);
    for (k = 0; k < n; k++) ed_b(p[k]);
    ed_send();
}
static int ed_groove_cmd(uint32_t cmd, const uint8_t *a, uint32_t n)
{
    int e = 0;
    uint32_t k;
    if (cmd < ED_GV_INFO || cmd >= ED_GV_END)
        return 0;
    if (!gv_on) { ed_fail(cmd, ED_E_REFUSED, ED_D_BLOCKED, "NOT IN GROOVE"); return 1; }
    for (k = 0; k < n; k++) if (a[k] > 0x7Fu) e = 2;
    if (!e) switch (cmd) {
    case ED_GV_INFO:
        if (n) { e = 1; break; }
        ed_gv_buf[0] = 'G'; ed_gv_buf[1] = 'V'; ed_gv_buf[2] = GV_LK_VER;
        ed_gv_out(cmd, ed_gv_buf, 3u + gv_lk_song_get(ed_gv_buf + 3));
        return 1;
    case ED_GV_SONG:
        if ((e = gv_lk_song_set(a, n)) != 0) break;
        gv_say("STUDIO : MORCEAU RECU");
        ed_gv_out(cmd, ed_gv_buf, gv_lk_song_get(ed_gv_buf));
        return 1;
    case ED_GV_RYT_GET:
        if (n) { e = 1; break; }
        ed_gv_out(cmd, ed_gv_buf, gv_lk_ryt_get(ed_gv_buf));
        return 1;
    case ED_GV_RYT_SET:
        if ((e = gv_lk_ryt_set(a, n)) != 0) break;
        gv_say("STUDIO : RYTHME RECU");
        ed_gv_buf[0] = a[0];
        ed_gv_out(cmd, ed_gv_buf, 1);
        return 1;
    case ED_GV_MEL_GET:
        if (n != 1u) { e = 1; break; }
        if (a[0] >= GV_NT) { e = 2; break; }
        ed_gv_out(cmd, ed_gv_buf, gv_lk_mel_get(a[0], ed_gv_buf));
        return 1;
    default:                                       /* ED_GV_MEL_SET */
        if ((e = gv_lk_mel_set(a, n)) != 0) break;
        gv_say("STUDIO : MELODIE RECUE");
        ed_gv_buf[0] = a[0]; ed_gv_buf[1] = a[1];
        ed_gv_out(cmd, ed_gv_buf, 2);
        return 1;
    }
    if (e == 1) ed_fail(cmd, ED_E_BAD, ED_D_LENGTH, "BAD LENGTH");
    else ed_fail(cmd, ED_E_BAD, ED_D_RANGE, "OUT OF RANGE");
    return 1;
}
