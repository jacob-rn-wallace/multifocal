/**
 * @file frames_lrst_test.c
 * @brief Phase 4: LRST manual recovery from an orphaned frame.
 *
 * LCLS/LCLX are pure X-in/X-out functions - the "current logical stack
 * size" has to be held between calls by the calling FOCAL program
 * itself (by convention, in a dedicated variable, recommended name
 * "MFSZ" - see src/frames.s's Phase 4 header comment). If a subroutine
 * that pushed a frame never reaches its matching LCLX (an error abort,
 * a GTO out, a manual stop), that persistent size value is left stuck
 * at the elevated size - not corrupted data, just a permanently
 * "lost" frame, since every future LCLS silently builds on top of the
 * stale size. LRST is the manual recovery tool: unconditionally resets
 * X to 2 (empty stack) regardless of X's prior value, touching no XM
 * state at all (MFSTK's own contents are never physically reclaimed or
 * reinitialized).
 *
 * This test: (1) pushes 3 real nested frames via LCLS (2->6->10->14),
 * simulating an in-progress, legitimately-nested call chain; (2)
 * simulates the abrupt-exit scenario by simply never popping and
 * instead calling LRST directly with X still at 14 - confirms it comes
 * back 2, not something derived from 14; (3) confirms LRST is
 * idempotent by calling it again from the already-empty state (X=2 in,
 * X=2 out); (4) confirms normal operation resumes correctly after
 * reset by doing a fresh LCLS from the post-reset state (2->6); (5)
 * confirms LRST ignores X entirely even at the opposite extreme by
 * calling it with X=34 (the depth ceiling) and checking it still
 * returns 2.
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
    printf("%-22s X=%d -> XEQ %-4s -> disp=\"%s\" got=%d (expect %d) %s\n",
           label, arg, xeqname, d, got, expected, got == expected ? "OK" : "MISMATCH");
    if (got != expected) fails++;
}

int main(void) {
    char dispbuf[32];
    setvbuf(stdout, NULL, _IONBF, 0);
    nut_boot_cx();
    tabpage[8] = (short *)(const void *)rom_frames_p0;
    typmod[8] = 1;
    int ret = pump(300000);
    if (ret == 1) asleep_state = true;

    /* One-time setup: MFSTK at size 35 - LRST itself never touches XM,
     * but LCLS/LCLX (used here to build a realistic nested state first)
     * still require it, same as every other test in this suite. */
    type_byte(0x01); type_str("MFSTK"); type_byte(0x01);
    type_str("35");
    xeq("CRFLD");
    printf("setup CRFLD(MFSTK,35): disp=\"%s\"\n", display_to_buf(dispbuf));

    /* Build a real nested state: 3 pushes, 2 -> 6 -> 10 -> 14. */
    check_step("LCLS #1", "LCLS", 2, 6);
    check_step("LCLS #2", "LCLS", 6, 10);
    check_step("LCLS #3", "LCLS", 10, 14);

    /* Simulate abrupt exit: never pop. Call LRST directly with X still
     * at 14 (the orphaned size) - must come back 2, not 10 (14-4) or
     * anything derived from the input. */
    check_step("LRST after orphan", "LRST", 14, 2);

    /* Idempotent: calling LRST again from the already-empty state. */
    check_step("LRST idempotent", "LRST", 2, 2);

    /* Normal operation resumes correctly post-reset. */
    check_step("LCLS post-reset", "LCLS", 2, 6);
    check_step("LCLX post-reset", "LCLX", 6, 2);

    /* LRST ignores X entirely, even at the opposite extreme (the depth
     * ceiling itself). */
    check_step("LRST at ceiling", "LRST", 34, 2);

    if (fails == 0) {
        printf("PASS: LRST correctly resets to 2 regardless of X's prior "
               "value (orphaned mid-stack, already-empty, and at the "
               "depth ceiling), and normal LCLS/LCLX operation resumes "
               "correctly afterward.\n");
        return 0;
    }
    printf("FAIL: %d checks mismatched.\n", fails);
    return 1;
}
