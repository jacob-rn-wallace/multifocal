/**
 * @file cv82180a_frames_test.c
 * @brief The real-hardware-target milestone: MultiFOCAL's own LCLS/
 *        LCLX frame push/pop, verified on the actual target
 *        architecture - a plain HP-41CV base OS with the real 82180A
 *        Extended Functions/Extended Memory module providing XM,
 *        exactly as the user's own physical machine would need it -
 *        not the CX every prior test in this project used.
 *
 * Two real HP-41 ROMs, two real ports: the 82180A at page 5 (Port 2,
 * NOT page 4/Port 1 - see nut_rom_82180a.h for why page 4 is avoided),
 * MultiFOCAL's own frames.mod at page 6 (Port 3) - a genuine two-
 * module physical configuration, confirmed clean at cold boot
 * (test/nut_rom_82180a.h's own header documents the verification).
 *
 * Reproduces frames_test.c's own milestone exactly (3 nested LCLS
 * pushes then 3 LCLX pops, 2 -> 6 -> 10 -> 14 -> 10 -> 6 -> 2) against
 * this real-target boot config instead of the CX one. If this passes,
 * MultiFOCAL's frame push/pop genuinely works on the actual hardware
 * shape it was always meant for - not just a CX standing in for it.
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
    setvbuf(stdout, NULL, _IONBF, 0);

    nut_boot();            /* real HP-41CV base OS */
    nut_rom_wire_82180a(); /* real 82180A at page 5 (Port 2) */
    tabpage[6] = (short *)(const void *)rom_frames_p0; /* MultiFOCAL at page 6 (Port 3) */
    typmod[6] = 1;

    int ret = pump(300000);
    if (ret == 1) asleep_state = true;
    printf("cold boot: disp=\"%s\"\n", display_to_buf(dispbuf));

    /* One-time setup: MFSTK at its full max size, exactly as
     * frames_test.c's own CX-based milestone does. */
    type_byte(0x01); type_str("MFSTK"); type_byte(0x01);
    type_str("34");
    xeq("CRFLD");
    printf("setup CRFLD(MFSTK,34): disp=\"%s\"\n", display_to_buf(dispbuf));

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
        printf("PASS: MultiFOCAL's own LCLS/LCLX frame push/pop works "
               "correctly on the real HP-41CV+82180A target "
               "architecture - not just the CX.\n");
        return 0;
    } else {
        printf("FAIL: %d of 6 steps mismatched.\n", fails);
        return 1;
    }
}
