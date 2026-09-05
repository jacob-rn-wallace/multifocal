/**
 * @file cv82180a_lrst_test.c
 * @brief LRST verified against the real HP-41CV+82180A architecture,
 *        closing the one honest gap left after retiring LSTO/LRCL (see
 *        CLAUDE.md's "LSTO/LRCL retired" section): LRST shares LCLS/
 *        LCLX's exact code pattern (pure X-register arithmetic, no
 *        SAVEX/GETX/any mainframe-fixed address at all), so it was
 *        never exposed to the hardcoded-CX-address bug that broke
 *        LSTO/LRCL - but that was an inference from the code, not a
 *        test result, until now.
 *
 * Reproduces frames_lrst_test.c's own core check (LRST resets X to 2
 * regardless of its prior value, including from a real orphaned-
 * mid-stack value) against this real target architecture instead of
 * the CX.
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

static int fails = 0;

static void check_step(const char *label, const char *xeqname, int arg, int expected) {
    char dispbuf[32];
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", arg);
    type_str(buf);
    xeq(xeqname);
    const char *d = display_to_buf(dispbuf);
    int got = parse_display_int(d);
    printf("%-18s X=%d -> XEQ %-4s -> disp=\"%s\" got=%d (expect %d) %s\n",
           label, arg, xeqname, d, got, expected, got == expected ? "OK" : "MISMATCH");
    if (got != expected) fails++;
}

int main(void) {
    char dispbuf[32];
    setvbuf(stdout, NULL, _IONBF, 0);

    nut_boot();
    nut_rom_wire_82180a();
    tabpage[6] = (short *)(const void *)rom_frames_p0;
    typmod[6] = 1;
    int ret = pump(300000);
    if (ret == 1) asleep_state = true;
    printf("cold boot: disp=\"%s\"\n", display_to_buf(dispbuf));

    type_byte(0x01); type_str("MFSTK"); type_byte(0x01);
    type_str("35");
    xeq("CRFLD");
    printf("setup CRFLD(MFSTK,35): disp=\"%s\"\n", display_to_buf(dispbuf));

    /* Build a real nested state: 3 pushes, 2 -> 6 -> 10 -> 14. */
    check_step("LCLS #1", "LCLS", 2, 6);
    check_step("LCLS #2", "LCLS", 6, 10);
    check_step("LCLS #3", "LCLS", 10, 14);

    /* Simulate an orphaned frame: LRST must reset to 2 regardless. */
    check_step("LRST after orphan", "LRST", 14, 2);
    check_step("LRST idempotent", "LRST", 2, 2);
    check_step("LCLS post-reset", "LCLS", 2, 6);
    check_step("LCLX post-reset", "LCLX", 6, 2);
    check_step("LRST at ceiling", "LRST", 34, 2);

    if (fails == 0) {
        printf("PASS: LRST correctly resets to 2 regardless of X's prior "
               "value on the real CV+82180A target architecture, exactly "
               "as it does on the CX.\n");
        return 0;
    }
    printf("FAIL: %d check(s) mismatched.\n", fails);
    return 1;
}
