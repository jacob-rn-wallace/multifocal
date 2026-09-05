/**
 * @file compat_native_program_test.c
 * @brief Compatibility testing, Test 3: a literal, real STORED FOCAL
 *        PROGRAM (not calculator-mode keystrokes) must behave
 *        identically whether MultiFOCAL's module is present or not -
 *        the most literal reading of CLAUDE.md's hard constraint, and
 *        the piece compat_presence_test.c's own Test 1 explicitly
 *        could not attempt (no way to press STO/RCL/GTO/LBL at the
 *        time - see CLAUDE.md's "Real HP-41 keycodes found" section).
 *        Now that test/hp41_raw_keys.h has all four, this closes that
 *        gap.
 *
 * The program (9 real lines, entered via genuine PRGM-mode keystrokes,
 * verified independently in a throwaway probe before being locked in
 * here - see CLAUDE.md):
 *   line1: "5"          (X=5)
 *   line2: STO 01       (R01=5)
 *   line3: RCL 01       (X=5, redundant but a real RCL exercise)
 *   line4: "3"          (fresh entry: X=3, Y=5 via automatic stack lift)
 *   line5: GTO 13       (HP41_KEY_GTO's own auto-committed default
 *                        local-label target, not chosen by this test)
 *   line6: "999"        (a deliberately WRONG value - must be skipped)
 *   line7: LBL 13       (SHIFT+STO - the matching target)
 *   line8: "+"          (X = Y(5) + X(3) = 8, using the state from
 *                        before line6 - only reachable this way if
 *                        GTO genuinely skipped line6)
 *   line9: STO 02       (R02=8)
 *
 * Exercises digit entry, STO, RCL, real arithmetic, and a genuine
 * GTO/LBL branch - a representative native FOCAL program using no
 * MultiFOCAL function whatsoever. Takes one argument, "with" or any
 * other value, exactly like compat_presence_test.c - see that file's
 * header for why "module absent" is simply not touching page 8.
 * test/run_compat_native_program.sh runs both and diffs stdout - they
 * must be byte-for-byte identical.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
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
static int inject_raw(unsigned char code) {
    if (asleep_state) { flagKey = 0; regPC = 0; asleep_state = false; }
    if (lgkeybuf < 8) keybuffer[lgkeybuf++] = (char)code;
    int ret = pump(200000);
    if (ret == 1) asleep_state = true;
    return ret;
}

static void show(const char *label) {
    char dispbuf[32];
    printf("%-24s disp=\"%s\"\n", label, display_to_buf(dispbuf));
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    bool with_module = (argc > 1 && strcmp(argv[1], "with") == 0);

    nut_boot_cx();
    if (with_module) {
        tabpage[8] = (short *)(const void *)rom_frames_p0;
        typmod[8] = 1;
    }
    int ret = pump(300000);
    if (ret == 1) asleep_state = true;
    fprintf(stderr, "module=%s\n", with_module ? "present" : "absent");
    show("cold boot");

    inject_raw(0xc5); /* PRGM on */
    type_str("5");                                   /* line1 */
    show("line1 '5'");
    inject_raw(HP41_KEY_STO); type_str("01");         /* line2: STO 01 */
    show("line2 STO 01");
    inject_raw(HP41_KEY_RCL); type_str("01");         /* line3: RCL 01 */
    show("line3 RCL 01");
    type_str("3");                                   /* line4 */
    show("line4 '3'");
    inject_raw(HP41_KEY_GTO);                          /* line5: GTO 13 */
    show("line5 GTO");
    type_str("999");                                 /* line6: wrong, must be skipped */
    show("line6 '999'");
    inject_raw(0x12); inject_raw(HP41_KEY_STO); type_str("13"); /* line7: LBL 13 */
    show("line7 LBL 13");
    type_byte('+');                                  /* line8 */
    show("line8 '+'");
    inject_raw(HP41_KEY_STO); type_str("02");         /* line9: STO 02 */
    show("line9 STO 02");
    inject_raw(0xc5); /* PRGM off */

    for (int i = 0; i < 8; i++) { inject_raw(0x12); inject_raw(0xc2); } /* BST x8 -> line1 */
    show("back-stepped to line1");

    inject_raw(0x87); /* R/S - run the real program */
    show("after R/S");

    /* Verify both registers directly, independent of the running
     * display's own X value. */
    inject_raw(0xc3); /* CLX */
    inject_raw(HP41_KEY_RCL); type_str("01");
    show("RCL 01");
    inject_raw(0xc3);
    inject_raw(HP41_KEY_RCL); type_str("02");
    show("RCL 02");

    return 0;
}
