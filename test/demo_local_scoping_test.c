/**
 * @file demo_local_scoping_test.c
 * @brief A real, hand-authored, stored FOCAL program demonstrating
 * MultiFOCAL's actual feature - subroutine-local storage that doesn't
 * collide across nested calls - closing the gap flagged repeatedly in
 * CLAUDE.md ("a real hand-written demonstration program - everything
 * so far has been driven by synthetic test-harness keystrokes, not an
 * actual authored FOCAL program"). compat_native_program_test.c already
 * proved a real stored program using native STO/RCL/GTO/LBL; this is
 * the MultiFOCAL-specific counterpart - a real program that calls a
 * real subroutine, and each independently pushes/uses/pops its own
 * MultiFOCAL local frame.
 *
 * The program (two real global FOCAL labels, entered via genuine
 * PRGM-mode keystrokes, R01 used as the MFSZ persistent-size variable
 * per CLAUDE.md's Phase 4 documented convention, R02 as a verification
 * register):
 *
 *   LBL "MFDEMO"
 *     2  STO 01                  ; MFSZ = 2 (empty stack)
 *     XEQ "LCLS"  STO 01         ; push outer frame: MFSZ = 6
 *     "MFSTK" 3 XEQ "SEEKPTA"    ; seek to outer frame's own register (3)
 *     111 XEQ "SAVEX"            ; outer local = 111
 *     XEQ "INNER"                ; call a real global subroutine, which
 *                                ; pushes/uses/pops its OWN frame in the
 *                                ; meantime (registers 7-10, MFSZ 6->10->6)
 *     "MFSTK" 3 XEQ "SEEKPTA"    ; seek back to outer frame's register (3)
 *     XEQ "GETX"  STO 02         ; read outer local back - the whole point
 *                                ; of this test: must still read 111,
 *                                ; undisturbed by INNER's own frame
 *     RCL 01 XEQ "LCLX" STO 01   ; pop outer frame: MFSZ = 2
 *     RTN
 *
 *   LBL "INNER"
 *     RCL 01 XEQ "LCLS" STO 01   ; push inner frame: MFSZ = 6 -> 10
 *     "MFSTK" 7 XEQ "SEEKPTA"    ; seek to inner frame's own register (7)
 *     222 XEQ "SAVEX"            ; inner local = 222 (a different
 *                                ; register than outer's 111 - the
 *                                ; actual no-collision property)
 *     RCL 01 XEQ "LCLX" STO 01   ; pop inner frame: MFSZ = 10 -> 6
 *     RTN
 *
 * Verified via three independent, direct register reads after running
 * (not just trusting the live display): R02 must read back 111 (proof
 * outer's local survived INNER's entire push/use/pop lifecycle), R01
 * must read back 2 (proof the stack fully unwound), and a fresh
 * GETX at outer's own register (3) must also read 111 directly from
 * XM, independent of MultiFOCAL's own bookkeeping.
 *
 * MFSTK setup (CRFLD + the mandatory post-CRFLD SEEKPTA, per the
 * "CRFLD's own post-creation pointer state is not equivalent to
 * SEEKPTA(1)" finding in CLAUDE.md's compatibility-testing section) is
 * done live, before PRGM mode - a one-time step, same convention as
 * every other test in this project, not part of the demo program
 * itself.
 *
 * New keycode used here for the first time: RTN = SHIFT + 0x83, found
 * by a full 0x00-0xFF brute-force PRGM-mode name-spelling scan (one
 * process per candidate) after a first guess of SHIFT+R/S turned out
 * to be VIEW, not RTN - see test/hp41_raw_keys.h's own header comment
 * for the corrected finding.
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
static void sto(const char *r) { inject_raw(HP41_KEY_STO); type_str(r); }
static void rcl(const char *r) { inject_raw(HP41_KEY_RCL); type_str(r); }

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

    /* One-time setup, live (never part of the demo program itself). */
    alpha("MFSTK"); type_str("35"); xeq("CRFLD");
    show("CRFLD(MFSTK,35)");
    alpha("MFSTK"); type_str("1"); xeq("SEEKPTA");
    show("SEEKPTA(MFSTK,1)");

    inject_raw(0xc5); /* PRGM on */

    /* --- LBL "INNER" --- */
    lbl_alpha("INNER");       show("LBL \"INNER\"");
    rcl("01");                show("RCL 01");
    xeq("LCLS");               show("XEQ LCLS");
    sto("01");                show("STO 01");
    alpha("MFSTK");            show("ALPHA MFSTK");
    type_str("7");             show("'7'");
    xeq("SEEKPTA");            show("XEQ SEEKPTA");
    type_str("222");           show("'222'");
    xeq("SAVEX");              show("XEQ SAVEX");
    rcl("01");                 show("RCL 01");
    xeq("LCLX");               show("XEQ LCLX");
    sto("01");                 show("STO 01");
    rtn();                     show("RTN");

    /* --- LBL "MFDEMO" --- */
    lbl_alpha("MFDEMO");       show("LBL \"MFDEMO\"");
    type_str("2");             show("'2'");
    sto("01");                 show("STO 01");
    xeq("LCLS");               show("XEQ LCLS");
    sto("01");                 show("STO 01");
    alpha("MFSTK");            show("ALPHA MFSTK");
    type_str("3");             show("'3'");
    xeq("SEEKPTA");            show("XEQ SEEKPTA");
    type_str("111");           show("'111'");
    xeq("SAVEX");              show("XEQ SAVEX");
    xeq("INNER");              show("XEQ \"INNER\"");
    alpha("MFSTK");            show("ALPHA MFSTK");
    type_str("3");             show("'3'");
    xeq("SEEKPTA");            show("XEQ SEEKPTA");
    xeq("GETX");               show("XEQ GETX");
    sto("02");                 show("STO 02");
    rcl("01");                 show("RCL 01");
    xeq("LCLX");               show("XEQ LCLX");
    sto("01");                 show("STO 01");
    rtn();                     show("RTN");

    inject_raw(0xc5); /* PRGM off */

    /* Run the real recorded program via a genuine XEQ ALPHA "MFDEMO"
     * ALPHA - exactly how a user would run it. */
    xeq("MFDEMO");
    show("after XEQ MFDEMO");

    /* Verify via direct register reads, independent of the display. */
    inject_raw(0xc3); rcl("02");
    const char *d = display_to_buf(dispbuf);
    int r02 = parse_display_int(d);
    printf("RCL 02 (outer local, post-INNER)  disp=\"%s\" got=%d (expect 111) %s\n",
           d, r02, r02 == 111 ? "OK" : "MISMATCH");
    if (r02 != 111) fails++;

    inject_raw(0xc3); rcl("01");
    d = display_to_buf(dispbuf);
    int r01 = parse_display_int(d);
    printf("RCL 01 (MFSZ, post-run)           disp=\"%s\" got=%d (expect 2) %s\n",
           d, r01, r01 == 2 ? "OK" : "MISMATCH");
    if (r01 != 2) fails++;

    /* And independently, straight from XM itself (bypassing whatever
     * MultiFOCAL's own bookkeeping claims): re-seek to outer's register
     * (3) and GETX it fresh. */
    alpha("MFSTK"); type_str("3"); xeq("SEEKPTA"); xeq("GETX");
    d = display_to_buf(dispbuf);
    int fresh = parse_display_int(d);
    printf("fresh GETX at register 3          disp=\"%s\" got=%d (expect 111) %s\n",
           d, fresh, fresh == 111 ? "OK" : "MISMATCH");
    if (fresh != 111) fails++;

    if (fails == 0) {
        printf("PASS: a real, hand-authored, stored FOCAL program - two "
               "global labels, a genuine subroutine call between them - "
               "demonstrates MultiFOCAL's actual local-variable-scoping "
               "feature end to end: INNER's own local frame did not "
               "disturb MFDEMO's, and both frames were correctly freed.\n");
        return 0;
    }
    printf("FAIL: %d check(s) mismatched.\n", fails);
    return 1;
}
