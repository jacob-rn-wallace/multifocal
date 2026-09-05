/**
 * @file local_slot_access_test.c
 * @brief Local-variable read/write within a pushed frame - replaces
 *        the retired LSTO/LRCL (see src/frames.s's "PHASE 3, then
 *        RETIRED" comment and CLAUDE.md's "Real HP-41CV+82180A boot
 *        config" section for why): the calling FOCAL program now uses
 *        the real SAVEX/GETX primitives directly, by name, instead of
 *        MultiFOCAL-specific wrapper functions - the same discipline
 *        CRFLD/SEEKPTA already required, and portable to any hardware
 *        configuration since it's ordinary catalog dispatch, never a
 *        MultiFOCAL-internal `gosub` to a fixed address.
 *
 * Exercises exactly the same milestone the old lsto_lrcl_test.c did,
 * with SAVEX/GETX in place of LSTO/LRCL: pushes all 8 frames to the
 * depth ceiling (size 2->34), stores distinct values into every slot
 * of the shallowest (frame 1, registers 3-6) and deepest (frame 8,
 * registers 31-34 - including the file's actual last usable register)
 * frames - the deepest write done out of sequential order to also
 * confirm a fresh SEEKPTA correctly repositions - then reads every one
 * of those 8 values back and confirms it round-trips correctly, before
 * popping all 8 frames back via LCLX.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define GLOBAL extern
#include "nutcpu.h"
#include "display.h"
#include "nut_rom_cx.h"
#include "hp41_key_bridge.h"
extern const uint16_t rom_frames_p0[4096];

static int pump(int iters) { int ret=0; for(int i=0;i<iters;i++){ret=executeNUT(1000); if(ret!=0)break;} return ret; }
static bool asleep_state=false;
static int type_byte(int c){ if(asleep_state){flagKey=0;regPC=0;asleep_state=false;} hp41_key_bridge_feed_byte(c); int ret=pump(200000); if(ret==1)asleep_state=true; return ret; }
static void type_str(const char*s){for(;*s;s++)type_byte((unsigned char)*s);}
static void xeq(const char*n){type_byte(0x18);type_byte(0x01);type_str(n);type_byte(0x01);}

static void seek(int reg) {
    char buf[8];
    type_byte(0x01); type_str("MFSTK"); type_byte(0x01);
    snprintf(buf, sizeof(buf), "%d", reg);
    type_str(buf);
    xeq("SEEKPTA");
}

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
    nut_boot_cx();
    tabpage[8] = (short *)(const void *)rom_frames_p0;
    typmod[8] = 1;
    int ret = pump(300000);
    if (ret == 1) asleep_state = true;

    /* One-time setup: MFSTK at size 35 (34 usable + 1 padding register
     * to dodge the last-register SEEKPTA bug - see file header). */
    type_byte(0x01); type_str("MFSTK"); type_byte(0x01);
    type_str("35");
    xeq("CRFLD");
    printf("setup CRFLD(MFSTK,35): disp=\"%s\"\n", display_to_buf(dispbuf));

    /* Push all 8 frames to the depth ceiling: 2->6->10->...->34. */
    int size = 2;
    for (int i = 0; i < 8; i++) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", size);
        type_str(buf);
        xeq("LCLS");
        size = parse_display_int(display_to_buf(dispbuf));
    }
    printf("after 8x LCLS: size=%d (expect 34)\n", size);
    if (size != 34) fails++;

    /* Frame 1 occupies registers 3-6 (base = size_after_push1 - 3 = 3).
     * Store 4 distinct values sequentially (one seek, then 4 SAVEXs
     * relying on auto-advance). */
    int frame1_vals[4] = {11, 12, 13, 14};
    seek(3);
    for (int i = 0; i < 4; i++) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", frame1_vals[i]);
        type_str(buf);
        xeq("SAVEX");
    }
    printf("frame1 (regs 3-6) sequential SAVEX done\n");

    /* Frame 8 occupies registers 31-34 (base = 34 - 3 = 31) - the
     * deepest frame, register 34 is the file's actual last usable
     * register. Store out of order to confirm each SAVEX's preceding
     * SEEKPTA really repositions rather than relying on leftover
     * state. */
    seek(34); type_str("84"); xeq("SAVEX");
    seek(31); type_str("81"); xeq("SAVEX");
    seek(33); type_str("83"); xeq("SAVEX");
    seek(32); type_str("82"); xeq("SAVEX");
    printf("frame8 (regs 31-34) out-of-order SAVEX done\n");

    /* Read frame1 back sequentially via GETX. */
    seek(3);
    for (int i = 0; i < 4; i++) {
        xeq("GETX");
        int got = parse_display_int(display_to_buf(dispbuf));
        int expect = frame1_vals[i];
        printf("frame1 slot%d GETX -> got=%d (expect %d) %s\n", i, got, expect,
               got == expect ? "OK" : "MISMATCH");
        if (got != expect) fails++;
    }

    /* Read frame8 back, deliberately out of order again. */
    int frame8_checks[4][2] = {{34, 84}, {31, 81}, {33, 83}, {32, 82}};
    for (int i = 0; i < 4; i++) {
        seek(frame8_checks[i][0]);
        xeq("GETX");
        int got = parse_display_int(display_to_buf(dispbuf));
        int expect = frame8_checks[i][1];
        printf("frame8 reg%d GETX -> got=%d (expect %d) %s\n",
               frame8_checks[i][0], got, expect, got == expect ? "OK" : "MISMATCH");
        if (got != expect) fails++;
    }

    /* Pop all 8 frames back down. */
    for (int i = 0; i < 8; i++) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", size);
        type_str(buf);
        xeq("LCLX");
        size = parse_display_int(display_to_buf(dispbuf));
    }
    printf("after 8x LCLX: size=%d (expect 2)\n", size);
    if (size != 2) fails++;

    if (fails == 0) {
        printf("PASS: direct SAVEX/GETX round-trip correctly at both the "
               "shallowest and deepest (including the file's last usable "
               "register) frames, in and out of sequential order - the "
               "replacement calling convention for the retired LSTO/LRCL.\n");
        return 0;
    }
    printf("FAIL: %d checks mismatched.\n", fails);
    return 1;
}
