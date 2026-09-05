/**
 * @file cv82180a_local_slot_test.c
 * @brief The real-hardware-target milestone, continued: local-variable
 *        read/write (unlike LCLS/LCLX's pure arithmetic, this
 *        genuinely touches XM) verified against the real
 *        HP-41CV+82180A architecture, using the real SAVEX/GETX
 *        primitives directly - NOT MultiFOCAL's now-retired LSTO/LRCL
 *        wrapper functions, which this exact test originally exposed
 *        as broken here (see src/frames.s's "PHASE 3, then RETIRED"
 *        comment and CLAUDE.md's "Real HP-41CV+82180A boot config"
 *        section for the full story: LSTO/LRCL hardcoded a CX-
 *        mainframe-specific address for SAVEX/GETX that doesn't exist
 *        on this configuration - `gosub`ing there silently executed
 *        empty ROM instead of the real routine).
 *
 * A representative (not exhaustive - see local_slot_access_test.c for
 * the full 8-frame depth-ceiling version, already verified on CX)
 * round trip: push one frame, store 2 distinct values into its slots
 * directly via SAVEX, read them back via GETX, pop the frame. Passing
 * confirms the retirement fix actually works on the real target
 * architecture, not just the CX.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define GLOBAL extern
#include "nutcpu.h"
#include "display.h"
#include "nut_rom.h"
#include "nut_rom_82180a.h"
#include "hp41_key_bridge.h"
extern const uint16_t rom_frames_p0[4096];

static int pump(int iters) { int ret=0; for(int i=0;i<iters;i++){ret=executeNUT(1000); if(ret!=0)break;} return ret; }
static bool asleep_state=false;
static int type_byte(int c){ if(asleep_state){flagKey=0;regPC=0;asleep_state=false;} hp41_key_bridge_feed_byte(c); int ret=pump(200000); if(ret==1)asleep_state=true; return ret; }
static void type_str(const char*s){for(;*s;s++)type_byte((unsigned char)*s);}
static void xeq(const char*n){type_byte(0x18);type_byte(0x01);type_str(n);type_byte(0x01);}

static int parse_display_int(const char *d) {
    char buf[32]; int j = 0;
    for (int i = 0; d[i] && j < 31; i++) if (d[i] != ' ') buf[j++] = d[i];
    buf[j] = 0;
    return atoi(buf);
}

int main(void) {
    char dispbuf[32];
    int fails = 0;
    setvbuf(stdout, NULL, _IONBF, 0);

    nut_boot();
    nut_rom_wire_82180a();
    tabpage[6] = (short *)(const void *)rom_frames_p0;
    typmod[6] = 1;
    int ret = pump(300000);
    if (ret == 1) asleep_state = true;
    printf("cold boot: disp=\"%s\"\n", display_to_buf(dispbuf));

    /* MFSTK at size 35 (34 usable + 1 padding - see CLAUDE.md's Phase
     * 3 section for why the padding register is required). */
    type_byte(0x01); type_str("MFSTK"); type_byte(0x01);
    type_str("35");
    xeq("CRFLD");
    printf("setup CRFLD(MFSTK,35): disp=\"%s\"\n", display_to_buf(dispbuf));

    /* Push one frame: 2 -> 6. Frame occupies registers 3-6. */
    type_str("2"); xeq("LCLS");
    int size = parse_display_int(display_to_buf(dispbuf));
    printf("after LCLS: size=%d (expect 6)\n", size);
    if (size != 6) fails++;

    /* Store 2 distinct values into slots 0-1 (registers 3-4), directly
     * via the real SAVEX - the replacement for the retired LSTO. */
    type_byte(0x01); type_str("MFSTK"); type_byte(0x01);
    type_str("3");
    xeq("SEEKPTA");
    type_str("77"); xeq("SAVEX");
    type_str("88"); xeq("SAVEX");
    printf("SAVEX(77), SAVEX(88) done\n");

    /* Read them back, directly via the real GETX. */
    type_byte(0x01); type_str("MFSTK"); type_byte(0x01);
    type_str("3");
    xeq("SEEKPTA");
    xeq("GETX");
    int got1 = parse_display_int(display_to_buf(dispbuf));
    printf("GETX #1 -> got=%d (expect 77) %s\n", got1, got1 == 77 ? "OK" : "MISMATCH");
    if (got1 != 77) fails++;
    xeq("GETX");
    int got2 = parse_display_int(display_to_buf(dispbuf));
    printf("GETX #2 -> got=%d (expect 88) %s\n", got2, got2 == 88 ? "OK" : "MISMATCH");
    if (got2 != 88) fails++;

    /* Pop the frame back down. */
    type_str("6"); xeq("LCLX");
    size = parse_display_int(display_to_buf(dispbuf));
    printf("after LCLX: size=%d (expect 2)\n", size);
    if (size != 2) fails++;

    if (fails == 0) {
        printf("PASS: direct SAVEX/GETX local-variable read/write works "
               "correctly against the real 82180A module - the retirement "
               "of LSTO/LRCL's hardcoded-CX-address gosub calls fixes the "
               "real-hardware-target compatibility this exact test "
               "originally found broken.\n");
        return 0;
    }
    printf("FAIL: %d check(s) failed.\n", fails);
    return 1;
}
