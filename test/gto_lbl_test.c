/**
 * @file gto_lbl_test.c
 * @brief Confirms the real HP-41 GTO/LBL keycodes found this session
 *        (see test/hp41_raw_keys.h and CLAUDE.md's "Real HP-41
 *        keycodes found" section) perform a genuine, working program
 *        branch - not just a display-name coincidence.
 *
 * Records, via real PRGM-mode keystrokes, the 5-line program:
 *   line01: "1"        (X=1)
 *   line02: GTO 13     (HP41_KEY_GTO's own auto-committed default
 *                        local-label target - not chosen by this test,
 *                        confirmed reproducible)
 *   line03: "5"
 *   line04: "+"        (if reached: X = 1+5 = 6)
 *   line05: LBL 13     (SHIFT+STO - the matching target)
 *
 * then backs up to the true start (4x BST - this program has exactly
 * 5 real lines plus the implicit line00 header, a 6-position cycle;
 * the pointer sits at line05 right after recording it, so 4 backward
 * steps reaches line01) and runs once via R/S.
 *
 * If GTO genuinely branches to the matching LBL, execution jumps
 * straight from line02 to line05, skipping the "5,+" arithmetic
 * entirely, and reaching the implicit .END. right after immediately
 * halts - final X stays 1. If GTO were a no-op (or this whole
 * discovery were wrong), the arithmetic would execute normally,
 * leaving X=6. A companion, real negative control already exists
 * in this session's own investigation (not repeated here as an
 * assertion, but worth knowing): the exact same "GTO 13" with NO
 * matching LBL anywhere in the program produces a real "NONEXISTENT"
 * error when executed - i.e., GTO's behavior is genuinely sensitive to
 * whether a target exists, not merely a fixed no-op that happens to
 * look like success.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GLOBAL extern
#include "nutcpu.h"
#include "display.h"
#include "nut_rom_cx.h"
#include "hp41_key_bridge.h"
#include "hp41_raw_keys.h"

static int pump(int iters) { int ret=0; for(int i=0;i<iters;i++){ret=executeNUT(1000); if(ret!=0)break;} return ret; }
static bool asleep_state=false;
static int type_byte(int c){ if(asleep_state){flagKey=0;regPC=0;asleep_state=false;} hp41_key_bridge_feed_byte(c); int ret=pump(200000); if(ret==1)asleep_state=true; return ret; }
static void type_str(const char*s){for(;*s;s++)type_byte((unsigned char)*s);}
static int inject_raw(unsigned char code) {
    if (asleep_state) { flagKey = 0; regPC = 0; asleep_state = false; }
    if (lgkeybuf < 8) keybuffer[lgkeybuf++] = (char)code;
    int ret = pump(200000);
    if (ret == 1) asleep_state = true;
    return ret;
}

static int parse_display_int(const char *d) {
    char buf[32]; int j = 0;
    for (int i = 0; d[i] && j < 31; i++) if (d[i] != ' ') buf[j++] = d[i];
    buf[j] = 0;
    return atoi(buf);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    nut_boot_cx();
    int ret = pump(300000);
    if (ret == 1) asleep_state = true;
    char dispbuf[32];

    inject_raw(0xc5); /* PRGM on */
    type_str("1");                          /* line01 */
    inject_raw(HP41_KEY_GTO);                /* line02: "GTO 13" */
    type_str("5");                          /* line03 */
    type_byte('+');                         /* line04 */
    inject_raw(0x12); inject_raw(HP41_KEY_STO); /* SHIFT+STO -> LBL prompt */
    type_str("13");                         /* line05: "LBL 13" */
    inject_raw(0xc5); /* PRGM off - pointer left at line05 */

    for (int i = 0; i < 4; i++) { inject_raw(0x12); inject_raw(0xc2); } /* BST x4 -> line01 */
    inject_raw(0x87); /* R/S - one-shot run */

    const char *d = display_to_buf(dispbuf);
    int got = parse_display_int(d);
    printf("after R/S: disp=\"%s\" got=%d (expect 1, meaning GTO 13 "
           "skipped the 5,+ arithmetic and jumped to LBL 13) %s\n",
           d, got, got == 1 ? "OK" : "MISMATCH");

    if (got == 1) {
        printf("PASS: real GTO/LBL branch works end to end.\n");
        return 0;
    }
    printf("FAIL: expected X=1, got %d.\n", got);
    return 1;
}
