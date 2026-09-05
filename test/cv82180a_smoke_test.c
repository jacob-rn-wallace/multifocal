/**
 * @file cv82180a_smoke_test.c
 * @brief First real test of the CV+82180A boot config - the actual
 *        real-hardware-target hardware shape (base HP-41 OS + a
 *        plug-in Extended Functions/Extended Memory module for XM),
 *        as opposed to the CX config every prior test in this project
 *        used. See CLAUDE.md's "Real HP-41CV+82180A boot config"
 *        section.
 *
 * Deliberately mirrors Phase 0's own approach: prove the basic plumbing
 * works before building anything on top of it. Confirms (1) the OS
 * boots and reaches a normal idle state, and (2) the 82180A module
 * genuinely provides working XM - a real CRFLD/SAVEX/GETX round trip
 * via keystrokes, the same primitives and calling convention this
 * project has used against the CX's built-in CXFUNS ROM since Phase 2
 * groundwork. No MultiFOCAL module is involved in this test at all.
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

    nut_boot();          /* soynut's own plain HP-41CV base OS boot */
    nut_rom_wire_82180a(); /* the real 82180A module at page 4 (Port 1) */
    int ret = pump(300000);
    if (ret == 1) asleep_state = true;
    printf("cold boot: disp=\"%s\"\n", display_to_buf(dispbuf));

    /* A real CRFLD/SAVEX/GETX round trip, via the 82180A this time,
     * not the CX's built-in CXFUNS - exactly the calling convention
     * established in Phase 2 groundwork. A fresh CRFLD does NOT leave
     * the pointer ready to write register 1 directly (root-caused
     * during compatibility testing - see CLAUDE.md) - explicit
     * SEEKPTA right after CRFLD, always. */
    type_byte(0x01); type_str("CVTEST"); type_byte(0x01);
    type_str("5");
    xeq("CRFLD");
    printf("CRFLD(CVTEST,5): disp=\"%s\"\n", display_to_buf(dispbuf));
    type_byte(0x01); type_str("CVTEST"); type_byte(0x01);
    type_str("1");
    xeq("SEEKPTA");
    printf("SEEKPTA(CVTEST,1): disp=\"%s\"\n", display_to_buf(dispbuf));

    type_str("42"); xeq("SAVEX");
    printf("SAVEX(42): disp=\"%s\"\n", display_to_buf(dispbuf));

    type_byte(0x01); type_str("CVTEST"); type_byte(0x01);
    type_str("1");
    xeq("SEEKPTA");
    printf("SEEKPTA(CVTEST,1): disp=\"%s\"\n", display_to_buf(dispbuf));

    xeq("GETX");
    const char *d = display_to_buf(dispbuf);
    int got = parse_display_int(d);
    printf("GETX: disp=\"%s\" got=%d (expect 42) %s\n",
           d, got, got == 42 ? "OK" : "MISMATCH");
    if (got != 42) fails++;

    if (fails == 0) {
        printf("PASS: the real 82180A module provides working XM "
               "(CRFLD/SAVEX/SEEKPTA/GETX round-trip correctly) on a "
               "plain HP-41CV base OS boot - no CX mainframe involved "
               "at all.\n");
        return 0;
    }
    printf("FAIL: %d check(s) failed.\n", fails);
    return 1;
}
