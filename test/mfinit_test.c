/**
 * @file mfinit_test.c
 * @brief MFINIT: a real, hand-authored FOCAL subroutine that collapses
 * the whole one-time MFSTK setup sequence (documented in CLAUDE.md's
 * "local-scoping demo" walkthrough) into a single `XEQ ALPHA "MFINIT"
 * ALPHA`:
 *
 *   LBL "MFINIT"
 *     "MFSTK" 35 XEQ "CRFLD"     ; create the frame-stack file
 *     "MFSTK" 1 XEQ "SEEKPTA"    ; the mandatory post-CRFLD seek (a
 *                                ; fresh file's pointer isn't ready at
 *                                ; register 1 by default - see
 *                                ; CLAUDE.md's compatibility-testing
 *                                ; section)
 *     RTN
 *
 * This is NOT new MCODE - it's an ordinary stored FOCAL program, for a
 * hard architectural reason documented in CLAUDE.md's Phase 3 section:
 * CRFLD/SEEKPTA abandon their caller (jump to idle instead of
 * returning) when called via `gosub` from MCODE, so MultiFOCAL's own
 * module can never dispatch them internally - only real FOCAL-level
 * XEQ, which is exactly what a stored program does.
 *
 * Verifies MFINIT genuinely replaces the four-keystroke-group manual
 * setup, not just that it runs without error: after `XEQ "MFINIT"`,
 * does a real LCLS -> SAVEX -> GETX -> LCLX round trip and confirms it
 * all works correctly - proof the file MFINIT created is immediately
 * usable, not just present.
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
#include "hp41_raw_keys.h"
extern const uint16_t rom_frames_p0[4096];

static int pump(int iters) { int ret=0; for(int i=0;i<iters;i++){ret=executeNUT(1000); if(ret!=0)break;} return ret; }
static bool asleep_state=false;
static int type_byte(int c){ if(asleep_state){flagKey=0;regPC=0;asleep_state=false;} hp41_key_bridge_feed_byte(c); int ret=pump(200000); if(ret==1)asleep_state=true; return ret; }
static void type_str(const char*s){for(;*s;s++)type_byte((unsigned char)*s);}
static void xeq(const char*n){type_byte(0x18);type_byte(0x01);type_str(n);type_byte(0x01);}
static int inject_raw(unsigned char code) {
    if (asleep_state) { flagKey = 0; regPC = 0; asleep_state = false; }
    if (lgkeybuf < 8) keybuffer[lgkeybuf++] = (char)code;
    int ret = pump(200000);
    if (ret == 1) asleep_state = true;
    return ret;
}
static void alpha(const char *s) { type_byte(0x01); type_str(s); type_byte(0x01); }
static void lbl_alpha(const char *s) { inject_raw(0x12); inject_raw(HP41_KEY_STO); type_byte(0x01); type_str(s); type_byte(0x01); }
static void rtn(void) { inject_raw(0x12); inject_raw(HP41_KEY_RTN); }

static void show(const char *label) {
    char dispbuf[32];
    printf("%-28s disp=\"%s\"\n", label, display_to_buf(dispbuf));
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
    show("cold boot");

    /* Record MFINIT - the ONLY thing the user does by hand, once. */
    inject_raw(0xc5); /* PRGM on */
    lbl_alpha("MFINIT");       show("LBL \"MFINIT\"");
    alpha("MFSTK");            show("ALPHA MFSTK");
    type_str("35");            show("'35'");
    xeq("CRFLD");              show("XEQ CRFLD");
    alpha("MFSTK");            show("ALPHA MFSTK");
    type_str("1");             show("'1'");
    xeq("SEEKPTA");            show("XEQ SEEKPTA");
    rtn();                     show("RTN");
    inject_raw(0xc5); /* PRGM off */

    /* The one keystroke sequence a user actually needs going forward. */
    xeq("MFINIT");
    show("after XEQ MFINIT");

    /* Prove the storage MFINIT created is immediately, fully usable -
     * a real LCLS -> SAVEX -> GETX -> LCLX round trip, live (not
     * recorded), exactly like a user would do next. */
    type_str("2"); xeq("LCLS");
    int size = parse_display_int(display_to_buf(dispbuf));
    printf("LCLS(2) -> size=%d (expect 6) %s\n", size, size == 6 ? "OK" : "MISMATCH");
    if (size != 6) fails++;

    alpha("MFSTK"); type_str("3"); xeq("SEEKPTA");
    type_str("77"); xeq("SAVEX");
    alpha("MFSTK"); type_str("3"); xeq("SEEKPTA");
    xeq("GETX");
    int got = parse_display_int(display_to_buf(dispbuf));
    printf("SAVEX(77)/GETX -> got=%d (expect 77) %s\n", got, got == 77 ? "OK" : "MISMATCH");
    if (got != 77) fails++;

    type_str("6"); xeq("LCLX");
    size = parse_display_int(display_to_buf(dispbuf));
    printf("LCLX(6) -> size=%d (expect 2) %s\n", size, size == 2 ? "OK" : "MISMATCH");
    if (size != 2) fails++;

    if (fails == 0) {
        printf("PASS: XEQ \"MFINIT\" alone (one XEQ, not four separate "
               "keystroke groups) fully and correctly sets up MFSTK - "
               "LCLS/SAVEX/GETX/LCLX all work immediately afterward.\n");
        return 0;
    }
    printf("FAIL: %d check(s) mismatched.\n", fails);
    return 1;
}
