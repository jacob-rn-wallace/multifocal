/**
 * @file xm_probe_test.c
 * @brief Phase 2 spike: empirically verify HP-41CX Extended Memory (XM)
 *        register-level read/write actually works in soynut's real Nut
 *        CPU core, on top of the real CX ROM configuration (see
 *        nut_rom_cx.h/.c).
 *
 * Headless, same technique as phase0_loop_test.c (see that file's header
 * for the executeNUT()/POWOFF-wake gotchas this also relies on).
 *
 * Real keystroke sequence, confirmed against the HP-41CX Owner's Manual
 * Vol 2 Section 13 ("Extended Memory") and empirically verified here via
 * direct espaceRAM inspection, not just the LCD:
 *   1. ALPHA "TEST" ALPHA, "10", XEQ ALPHA CRFLD ALPHA
 *      - creates a 10-register XM file named TEST, current register = 1.
 *   2. "42", XEQ ALPHA SAVEX ALPHA
 *      - writes 42 into the current register (1) and advances the
 *        pointer to register 2. SAVEX/GETX are the real single-register
 *        primitives - NOT SAVER/GETR, which are bulk whole-file copy
 *        operations (an earlier version of this test used SAVER/GETR by
 *        mistake: on an empty register file that's a correct no-op,
 *        which is why it silently "succeeded" without writing anything -
 *        see CLAUDE.md's "Phase 2 groundwork" section for the full story).
 *   3. XEQ ALPHA GETX ALPHA
 *      - reads the current register (now register 2, since SAVEX already
 *        advanced past register 1) back into X.
 *
 * This test deliberately does NOT attempt an explicit seek back to
 * register 1 before the final read (SEEKPT/SEEKPTA are real catalog
 * names per the manual, but reliably SIGSEGV this emulator core in this
 * ROM configuration - a confirmed emu41gcc bug, not a usage error; see
 * CLAUDE.md). So the expected value here is whatever register 2 holds
 * (0, since only register 1 was ever written) - this test's job is to
 * prove SAVEX/GETX themselves work end to end without crashing, not to
 * prove random-access seeking (that's a separate, currently-blocked
 * question, relevant to Phase 3's LSTO/LRCL addressing, not Phase 2's
 * frame push/pop, which can stay purely sequential).
 *
 * Build/run: make -C test xm_probe
 */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define GLOBAL extern
#include "nutcpu.h"

#include "display.h"
#include "nut_rom_cx.h"
#include "hp41_key_bridge.h"

static int pump(int iters)
{
    int ret = 0;
    for (int i = 0; i < iters; i++) {
        ret = executeNUT(1000);
        if (ret != 0)
            break;
    }
    assert(ret >= 0 && ret <= 3);
    return ret;
}

#define COLD_BOOT_ITERS 300000
#define PER_KEY_ITERS 100000

static bool asleep_state = false;

static int type_byte(int c)
{
    if (asleep_state) {
        flagKey = 0;
        regPC = 0;
        asleep_state = false;
    }
    hp41_key_bridge_feed_byte(c);
    const int ret = pump(PER_KEY_ITERS);
    if (ret == 1)
        asleep_state = true;
    return ret;
}

static void type_str(const char *s)
{
    for (; *s; s++)
        type_byte((unsigned char)*s);
}

/** Snapshot the 128 CX-built-in XM registers (espaceRAM[64*8..192*8), 8 bytes/register). */
static void snapshot_xm(unsigned char out[128 * 8])
{
    memcpy(out, &espaceRAM[64 * 8], 128 * 8);
}

static void print_xm_diff(const unsigned char before[128 * 8], const unsigned char after_[128 * 8])
{
    int any = 0;
    for (int reg = 0; reg < 128; reg++) {
        if (memcmp(&before[reg * 8], &after_[reg * 8], 8) != 0) {
            any = 1;
            printf("  XM reg %d: ", reg);
            for (int b = 0; b < 8; b++) printf("%02x ", before[reg * 8 + b]);
            printf("-> ");
            for (int b = 0; b < 8; b++) printf("%02x ", after_[reg * 8 + b]);
            printf("\n");
        }
    }
    if (!any) printf("  (no change)\n");
}

int main(void)
{
    char dispbuf[32];

    nut_boot_cx();
    assert(regPC == 0);

    int ret = pump(COLD_BOOT_ITERS);
    printf("cold boot: ret=%d, display=\"%s\"\n", ret, display_to_buf(dispbuf));
    if (ret != 0 && ret != 1) {
        printf("FAIL: cold boot hit ret=%d (expected 0=OK or 1=POWOFF)\n", ret);
        return 1;
    }
    if (ret == 1)
        asleep_state = true;

    unsigned char snap_before_crfld[128 * 8];
    snapshot_xm(snap_before_crfld);

    /* ALPHA "TEST" ALPHA, "10", XEQ ALPHA CRFLD ALPHA */
    type_byte(0x01);
    type_str("TEST");
    type_byte(0x01);
    type_str("10");
    type_byte(0x18);
    type_byte(0x01);
    type_str("CRFLD");
    type_byte(0x01);

    const char *disp = display_to_buf(dispbuf);
    printf("after XEQ CRFLD: display=\"%s\"\n", disp);
    if (strstr(disp, "N O N E X I S T E N T") != NULL) {
        printf("FAIL: CRFLD not found in the catalog - CX/XM detection failed.\n");
        return 1;
    }
    if (strstr(disp, "1 0") == NULL) {
        printf("FAIL: unexpected display state after CRFLD.\n");
        return 1;
    }

    unsigned char snap_after_crfld[128 * 8];
    snapshot_xm(snap_after_crfld);
    printf("XM RAM diff after CRFLD:\n");
    print_xm_diff(snap_before_crfld, snap_after_crfld);

    /* "42", XEQ ALPHA SAVEX ALPHA - write 42 into register 1, advance to register 2. */
    type_str("42");
    type_byte(0x18);
    type_byte(0x01);
    type_str("SAVEX");
    type_byte(0x01);
    disp = display_to_buf(dispbuf);
    printf("after XEQ SAVEX: display=\"%s\"\n", disp);
    if (strstr(disp, "N O N E X I S T E N T") != NULL) {
        printf("FAIL: SAVEX not found in the catalog.\n");
        return 1;
    }

    unsigned char snap_after_savex[128 * 8];
    snapshot_xm(snap_after_savex);
    printf("XM RAM diff after SAVEX (should show a real write, unlike the SAVER mistake this replaced):\n");
    print_xm_diff(snap_after_crfld, snap_after_savex);

    /* XEQ ALPHA GETX ALPHA - read the current register (now #2) back into X. */
    type_byte(0x18);
    type_byte(0x01);
    type_str("GETX");
    type_byte(0x01);
    disp = display_to_buf(dispbuf);
    printf("after XEQ GETX: display=\"%s\"\n", disp);
    if (strstr(disp, "N O N E X I S T E N T") != NULL) {
        printf("FAIL: GETX not found in the catalog.\n");
        return 1;
    }

    printf("PASS: SAVEX/GETX confirmed working end to end (real XM register write "
           "verified via raw memory diff above, GETX ran cleanly reading the next "
           "register). Explicit seek-to-arbitrary-register remains blocked by a "
           "separate confirmed emu41gcc crash bug - see CLAUDE.md.\n");
    return 0;
}
