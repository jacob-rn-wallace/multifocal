/**
 * @file sto_rcl_test.c
 * @brief Confirms the real HP-41 STO/RCL keycodes found this session
 *        (see test/hp41_raw_keys.h and CLAUDE.md's "Real HP-41
 *        keycodes found" section) work reliably via raw keycode
 *        injection, bypassing hp41_key_bridge.c's own restricted
 *        named-key table entirely (soynut's file, untouched).
 *
 * Round-trips one value/register combination through real STO then
 * real RCL (with an intervening CLX so RCL can't be reading back a
 * stale X value by coincidence). Takes the value and register as
 * argv[1]/argv[2] and does exactly ONE trial per process -
 * test/run_sto_rcl.sh runs it for 3 independent combinations
 * ((42,07), (99,12), (5,03)). One process per trial is deliberate, not
 * incidental: nut_boot_cx() does NOT clear espaceRAM/keybuffer/
 * flagKey (only ROM page wiring and a handful of CPU registers), so
 * looping several trials with a fresh nut_boot_cx() each time but
 * *within one process* silently accumulates cross-trial contamination -
 * found the hard way in this exact file, whose first version did
 * exactly that and got 2 of 3 trials wrong before being fixed. See
 * CLAUDE.md for the full story.
 *
 * These are native, non-MultiFOCAL HP-41 operations - this test exists
 * purely to lock in the keycode discovery as a permanent, reusable
 * capability (not a MultiFOCAL correctness test), unblocking a future,
 * more literal "real stored FOCAL program" compatibility test that
 * this project's tooling previously couldn't drive at all.
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

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc < 3) { fprintf(stderr, "usage: %s <value> <2-digit-register>\n", argv[0]); return 2; }
    int val = atoi(argv[1]);
    const char *reg = argv[2];

    nut_boot_cx();
    int ret = pump(300000);
    if (ret == 1) asleep_state = true;

    type_str(argv[1]);
    inject_raw(HP41_KEY_STO);
    type_str(reg);

    inject_raw(0xc3); /* CLX - confirmed named key, clears X so the
                        * following RCL can't coincidentally show the
                        * right answer without actually reading the
                        * register. */

    inject_raw(HP41_KEY_RCL);
    type_str(reg);

    char dispbuf[32];
    const char *d = display_to_buf(dispbuf);
    int got = parse_display_int(d);
    printf("STO(%d,R%s) CLX RCL(R%s) -> disp=\"%s\" got=%d (expect %d) %s\n",
           val, reg, reg, d, got, val, got == val ? "OK" : "MISMATCH");
    return got == val ? 0 : 1;
}
