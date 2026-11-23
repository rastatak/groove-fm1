/* SPDX-License-Identifier: GPL-3.0-only
 * Groove fork */
/* Groove's bank kept through power-off: the 8 rhythms and 8 melodies of SAVE, and where new rhythms / melodies come
 * from. One flash object, storage.c's A/B pair of project slot 4's lock bank (OBJ_LOCKS0 + 3): Hortator reads a lock
 * bank there only when its length is a lock bank's (3208 B), so this shorter object is nothing to it; saving Hortator's
 * project 4 with parameter locks writes its own bank over this one (the Groove bank is then empty at the next power-on).
 * Written from the main loop only, and only while stopped (gv_store_poll: a second after the last change). */
#define GV_ST_MAGIC 0x31425647u                    /* "GVB1" */
typedef struct {
    uint32_t magic;
    uint8_t rbu, mbu, rsrc, msrc;
    uint16_t rb[8][8];
    gv_step_t mb[8][16];
} gv_store_t;
_Static_assert(sizeof(gv_store_t) <= 3584u && sizeof(gv_store_t) != 3208u, "the Groove bank: under a sector's +0xF00, not a lock bank's length");
static gv_store_t gv_stb __attribute__((section(".pool")));
static uint32_t gv_st_crc, gv_st_ms;
static uint8_t gv_st_ok;                           /* loaded (or found empty) once: polls may write */

static void gv_store_fill(void)
{
    memset(&gv_stb, 0, sizeof gv_stb);
    gv_stb.magic = GV_ST_MAGIC;
    gv_stb.rbu = gu.rbu; gv_stb.mbu = gu.mbu; gv_stb.rsrc = gv.rsrc; gv_stb.msrc = gv.msrc;
    memcpy(gv_stb.rb, gu.rb, sizeof gv_stb.rb);
    memcpy(gv_stb.mb, gu.mb, sizeof gv_stb.mb);
}
#if FELUCCA_FLASH
static void gv_store_load(void)
{
    if (st_load(OBJ_LOCKS0 + 3u, &gv_stb, sizeof gv_stb) == (int)sizeof gv_stb && gv_stb.magic == GV_ST_MAGIC) {
        gu.rbu = gv_stb.rbu; gu.mbu = gv_stb.mbu;
        gv.rsrc = gv_stb.rsrc > 3u ? 0u : gv_stb.rsrc; gv.msrc = gv_stb.msrc > 1u ? 0u : gv_stb.msrc;
        memcpy(gu.rb, gv_stb.rb, sizeof gu.rb);
        memcpy(gu.mb, gv_stb.mb, sizeof gu.mb);
    }
    gv_store_fill();
    gv_st_crc = st_crc32(&gv_stb, sizeof gv_stb);
    gv_st_ok = 1;
}
static void gv_store_poll(void)                    /* the bank changed and the sequencer is stopped: written, once */
{
    uint32_t c;
    if (!gv_st_ok || song.playing || fm1_ms - gv_st_ms < 1000u) return;
    gv_st_ms = fm1_ms;
    gv_store_fill();
    c = st_crc32(&gv_stb, sizeof gv_stb);
    if (c == gv_st_crc) return;
    if (st_save(OBJ_LOCKS0 + 3u, &gv_stb, sizeof gv_stb) == 0) { gv_st_crc = c; gv_say("BANQUE ENREGISTREE"); }
    else { gv_st_crc = c; gv_say("BANQUE : ECRITURE IMPOSSIBLE"); }   /* (not tried again until the next change) */
}
#else
static void gv_store_load(void) { gv_st_ok = 1; }
static void gv_store_poll(void) { }
#endif
