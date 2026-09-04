/**
 * @file frames_test.c
 * @brief Phase 2 milestone: nested LCLS/LCLX frame lifecycle test.
 *
 * LCLS/LCLX are pure functions over the FOCAL X register (see
 * src/frames.s's header comment for the full design rationale and the
 * decisive finding that led to it): the caller supplies the current
 * MFSTK stack size in X, and each call returns the new size in X after
 * growing/shrinking by 4 registers via RESZFL. No seeking, no header
 * register, no ALPHA touch inside LCLS/LCLX at all.
 *
 * This test creates MFSTK once (via real keystrokes, size 2), then
 * exercises 3 nested pushes and 3 pops, checking the returned size
 * after each XEQ matches what a real nested call chain should produce:
 * 2 -> 6 -> 10 -> 14 -> 10 -> 6 -> 2.
 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
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

static int parse_display_int(const char *d) {
    char buf[32]; int j = 0;
    for (int i = 0; d[i] && j < 31; i++) if (d[i] != ' ') buf[j++] = d[i];
    buf[j] = 0;
    return atoi(buf);
}

int main(void) {
    char dispbuf[32];
    setvbuf(stdout, NULL, _IONBF, 0);
    nut_boot_cx();
    tabpage[8] = (short *)(const void *)rom_frames_p0;
    typmod[8] = 1;
    int ret = pump(300000);
    printf("cold boot disp=%s\n", display_to_buf(dispbuf));
    if (ret == 1) asleep_state = true;

    /* One-time setup via keystrokes: create MFSTK at size 2 (LCLS/LCLX
     * never call CRFLD themselves - see frames.s's header comment on
     * why ALPHA-touching functions can't be gosub'd from MCODE). */
    type_byte(0x01); type_str("MFSTK"); type_byte(0x01);
    type_str("2");
    xeq("CRFLD");
    printf("setup CRFLD(MFSTK,2): disp=\"%s\"\n", display_to_buf(dispbuf));

    int fails = 0;
    int expect;

    #define CHECK_STEP(label, xeqname, arg, expected) \
        type_str(#arg); \
        xeq(xeqname); \
        expect = (expected); \
        { \
            const char *d = display_to_buf(dispbuf); \
            int got = parse_display_int(d); \
            printf("%-14s X=%d -> XEQ %-4s -> disp=\"%s\" got=%d (expect %d) %s\n", \
                   label, (arg), xeqname, d, got, expect, got == expect ? "OK" : "MISMATCH"); \
            if (got != expect) fails++; \
        }

    CHECK_STEP("LCLS #1", "LCLS", 2, 6);
    CHECK_STEP("LCLS #2", "LCLS", 6, 10);
    CHECK_STEP("LCLS #3", "LCLS", 10, 14);
    CHECK_STEP("LCLX #1", "LCLX", 14, 10);
    CHECK_STEP("LCLX #2", "LCLX", 10, 6);
    CHECK_STEP("LCLX #3", "LCLX", 6, 2);

    if (fails == 0) {
        printf("PASS: all 6 nested LCLS/LCLX steps returned the correct size.\n");
        return 0;
    } else {
        printf("FAIL: %d of 6 steps mismatched.\n", fails);
        return 1;
    }
}
