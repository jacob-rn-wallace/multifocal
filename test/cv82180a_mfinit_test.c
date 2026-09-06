/**
 * @file cv82180a_mfinit_test.c
 * @brief MFINIT verified against the real HP-41CV+82180A architecture -
 * see mfinit_test.c (the CX version) for the full rationale. MFINIT is
 * pure FOCAL (LBL/ALPHA/XEQ steps only, no MCODE of its own), so its
 * correctness shouldn't depend on which hardware provides XM - this
 * confirms that inference with an actual test result on the real
 * target architecture, the same discipline this project has applied to
 * every other function.
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

    nut_boot();
    nut_rom_wire_82180a();
    tabpage[6] = (short *)(const void *)rom_frames_p0;
    typmod[6] = 1;
    int ret = pump(300000);
    if (ret == 1) asleep_state = true;
    show("cold boot");

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

    xeq("MFINIT");
    show("after XEQ MFINIT");

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
        printf("PASS: MFINIT works correctly on the real CV+82180A target "
               "architecture, exactly as it does on the CX - expected, "
               "since MFINIT is pure FOCAL with no MCODE of its own, but "
               "now confirmed rather than assumed.\n");
        return 0;
    }
    printf("FAIL: %d check(s) mismatched.\n", fails);
    return 1;
}
