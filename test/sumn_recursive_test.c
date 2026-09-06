/**
 * @file sumn_recursive_test.c
 * @brief The real showcase: a genuinely SELF-recursive FOCAL program
 * (LBL "SUMN" calling XEQ "SUMN" on itself) where each recursion level
 * has its own MultiFOCAL local frame holding its own "n" - the exact
 * thing native flat-register FOCAL cannot do cleanly (every level would
 * otherwise share the same numbered register, stomping on each other).
 * MFDEMO/INNER (demo_local_scoping_test.c) proved two DIFFERENT
 * subroutines' frames don't collide; this proves the harder, more
 * compelling case - N simultaneously-active frames of the SAME
 * subroutine, at genuine call depth, correctly isolated.
 *
 *   LBL "SUMN"
 *     STO 00                     ; stash n (about to clobber X via LCLS)
 *     0
 *     STO 02                     ; R02 = 0 (base-case partial sum default)
 *     RCL 01  XEQ "LCLS"  STO 01 ; push THIS level's own frame
 *     "MFSTK" RCL 01 3 -  XEQ "SEEKPTA"
 *     RCL 00  XEQ "SAVEX"        ; store n in THIS frame's own slot
 *     RCL 00
 *     X=0?
 *     GTO 13                     ; n=0 (TRUE, no skip): jump straight to
 *                                ; the join point, using R02's default 0
 *     RCL 00  1  -               ; n-1
 *     XEQ "SUMN"                 ; recurse - pushes/uses/pops its OWN
 *                                ; frame while THIS level's frame stays
 *                                ; alive underneath it
 *     STO 02                     ; R02 = real partial sum (only reached
 *                                ; when n != 0 - the X=0? skip is what
 *                                ; keeps this line from ever running for
 *                                ; the n=0 base case)
 *   LBL 13
 *     "MFSTK" RCL 01 3 - XEQ "SEEKPTA"
 *     XEQ "GETX"                 ; recall THIS level's own n - the whole
 *                                ; point: must still be intact after the
 *                                ; recursive call's entire frame
 *                                ; lifecycle just happened
 *     RCL 02  +                  ; n + partial_sum
 *     STO 03                     ; stash result (about to clobber X via LCLX)
 *     RCL 01  XEQ "LCLX"  STO 01 ; pop THIS level's own frame
 *     RCL 03
 *     RTN
 *
 * Only one GTO/LBL pair is used anywhere, by design: raw keycode
 * injection commits GTO immediately to a fixed default target ("GTO
 * 13") rather than an editable one - confirmed empirically in the
 * original GTO/LBL keycode-discovery session (see CLAUDE.md) - so this
 * program is deliberately structured (R02 pre-seeded to the base-case
 * value 0 before the branch, GTO 13 as the sole jump) to need no
 * second distinct target.
 *
 * SUMN(4) = 4+3+2+1+0 = 10, via 5 simultaneously-nested self-recursive
 * calls (n=4,3,2,1,0), each with its own independent local "n" -
 * MFSZ reaches 22 (5 frames deep) at the bottom of the recursion,
 * well within the depth-8/34-register ceiling.
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
static void lbl_num(const char *n) { inject_raw(0x12); inject_raw(HP41_KEY_STO); type_str(n); }
static void rtn(void) { inject_raw(0x12); inject_raw(HP41_KEY_RTN); }
static void sto(const char *r) { inject_raw(HP41_KEY_STO); type_str(r); }
static void rcl(const char *r) { inject_raw(HP41_KEY_RCL); type_str(r); }
static void x_eq_0(void) { inject_raw(0x12); inject_raw(HP41_KEY_X_EQ_0); }
static void gto13(void) { inject_raw(HP41_KEY_GTO); }

static void show(const char *label) {
    char dispbuf[32];
    printf("%-24s disp=\"%s\"\n", label, display_to_buf(dispbuf));
}

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
    if (ret == 1) asleep_state = true;
    show("cold boot");

    /* One-time setup, live. */
    alpha("MFSTK"); type_str("35"); xeq("CRFLD");
    alpha("MFSTK"); type_str("1"); xeq("SEEKPTA");
    type_str("2"); sto("01"); /* MFSZ starts at 2 (empty stack) - a
        register's cold-boot default is 0, not 2, and frame_base's own
        "MFSZ - 3" arithmetic depends on this exact starting value. */
    show("MFSTK created + seeked + MFSZ init");

    inject_raw(0xc5); /* PRGM on */
    lbl_alpha("SUMN");
    sto("00");
    type_str("0");
    sto("02");
    rcl("01"); xeq("LCLS"); sto("01");
    alpha("MFSTK"); rcl("01"); type_str("3"); type_byte('-'); xeq("SEEKPTA");
    rcl("00"); xeq("SAVEX");
    rcl("00");
    x_eq_0();
    gto13();
    rcl("00"); type_str("1"); type_byte('-');
    xeq("SUMN");
    sto("02");
    lbl_num("13");
    alpha("MFSTK"); rcl("01"); type_str("3"); type_byte('-'); xeq("SEEKPTA");
    xeq("GETX");
    rcl("02"); type_byte('+');
    sto("03");
    rcl("01"); xeq("LCLX"); sto("01");
    rcl("03");
    rtn();
    inject_raw(0xc5); /* PRGM off */
    show("recording done");

    /* Run it: SUMN(4). */
    type_str("4");
    xeq("SUMN");
    const char *d = display_to_buf(dispbuf);
    int result = parse_display_int(d);
    printf("SUMN(4) -> disp=\"%s\" got=%d (expect 10) %s\n",
           d, result, result == 10 ? "OK" : "MISMATCH");

    /* Independent verification: MFSZ (R01) must have fully unwound. */
    inject_raw(0xc3); rcl("01");
    d = display_to_buf(dispbuf);
    int mfsz = parse_display_int(d);
    printf("RCL 01 (MFSZ, post-run) -> disp=\"%s\" got=%d (expect 2) %s\n",
           d, mfsz, mfsz == 2 ? "OK" : "MISMATCH");

    /* A second call, different input, proves no leftover state from
     * the first recursive run corrupts a fresh one. SUMN(3)=3+2+1+0=6. */
    type_str("3");
    xeq("SUMN");
    d = display_to_buf(dispbuf);
    int result2 = parse_display_int(d);
    printf("SUMN(3) -> disp=\"%s\" got=%d (expect 6) %s\n",
           d, result2, result2 == 6 ? "OK" : "MISMATCH");

    if (result == 10 && mfsz == 2 && result2 == 6) {
        printf("PASS: genuine self-recursion (LBL \"SUMN\" calling XEQ "
               "\"SUMN\" on itself, 5 simultaneously-active local frames "
               "at the deepest point) computes the correct sum, fully "
               "unwinds, and a second independent call still works "
               "correctly afterward.\n");
        return 0;
    }
    printf("FAIL.\n");
    return 1;
}
