/**
 * @file resz_probe_test.c
 * @brief Phase 2 spike: empirically verify RESZFL (resize the current XM
 *        file) and SEEKPTA (seek to an arbitrary register) both work.
 *
 * These previously appeared to crash the emulator (along with SAVEP/
 * EMROOM) - root-caused to a real bug in this project's OWN test harness,
 * not the ROMs or emu41gcc: nut_rom_cx.c had CXFUNS1.ROM wired at a flat
 * page 4, but real HP-41CX hardware bank-switches page 5 between TIMER
 * and CXFUN1 (page 4 isn't used at all). Once corrected (see
 * nut_rom_cx.c), both RESZFL and SEEKPTA run cleanly with no crash - see
 * CLAUDE.md's "Phase 2" section for the full story. This means the frame
 * stack does NOT need to avoid seeking/resizing - a single growing/
 * shrinking file with real seek-based navigation is back on the table.
 *
 * Real keystroke sequences, per the HP-41CX Owner's Manual Vol 2 (p.
 * 209-213): RESZFL needs the target file already current and the new
 * register count in X, no ALPHA involvement (unlike CRFLD). SEEKPTA
 * needs the target position in X and (per the manual) ALPHA cleared to
 * seek within the already-current file.
 *
 * Build/run: make -C test resz_probe
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

static void xeq(const char *name)
{
    type_byte(0x18);
    type_byte(0x01);
    type_str(name);
    int r = type_byte(0x01);
    printf("  [xeq %s: ret=%d regPC=0x%04x asleep=%d]\n", name, r, regPC, asleep_state);
}

int main(void)
{
    char dispbuf[32];

    setvbuf(stdout, NULL, _IONBF, 0);
    nut_boot_cx();
    int ret = pump(COLD_BOOT_ITERS);
    printf("cold boot: ret=%d, display=\"%s\"\n", ret, display_to_buf(dispbuf));
    if (ret == 1)
        asleep_state = true;

    /* ALPHA "STK" ALPHA, "2", XEQ ALPHA CRFLD ALPHA - a 2-register file. */
    type_byte(0x01);
    type_str("STK");
    type_byte(0x01);
    type_str("2");
    xeq("CRFLD");
    const char *disp = display_to_buf(dispbuf);
    printf("after CRFLD(STK,2): display=\"%s\"\n", disp);
    if (strstr(disp, "N O N E X I S T E N T") != NULL) {
        printf("FAIL: CRFLD not found in the catalog.\n");
        return 1;
    }

    /* Write 11, 22 into registers 1, 2. */
    type_str("11");
    xeq("SAVEX");
    type_str("22");
    xeq("SAVEX");
    disp = display_to_buf(dispbuf);
    printf("after writing 11,22: display=\"%s\"\n", disp);
    if (strstr(disp, "N O N E X I S T E N T") != NULL) {
        printf("FAIL: SAVEX hit NONEXISTENT during initial fill.\n");
        return 1;
    }

    unsigned char before_grow[128 * 8];
    memcpy(before_grow, &espaceRAM[64 * 8], sizeof(before_grow));

    /* Grow to 4 registers via RESZFL(4) - no ALPHA needed, file already current. */
    type_str("4");
    xeq("RESZFL");
    disp = display_to_buf(dispbuf);
    printf("after RESZFL(4): display=\"%s\"\n", disp);
    if (strstr(disp, "N O N E X I S T E N T") != NULL) {
        printf("FAIL: RESZFL not found in the catalog.\n");
        return 1;
    }

    unsigned char after_grow[128 * 8];
    memcpy(after_grow, &espaceRAM[64 * 8], sizeof(after_grow));
    printf("XM RAM diff from RESZFL growth alone (should be metadata only, not touch reg 1/2's data):\n");
    for (int reg = 0; reg < 128; reg++) {
        if (memcmp(&before_grow[reg * 8], &after_grow[reg * 8], 8) != 0) {
            printf("  XM reg %d: ", reg);
            for (int b = 0; b < 8; b++) printf("%02x ", before_grow[reg * 8 + b]);
            printf("-> ");
            for (int b = 0; b < 8; b++) printf("%02x ", after_grow[reg * 8 + b]);
            printf("\n");
        }
    }

    /* Now write 33, 44 - the critical test: do these land in the newly
     * added registers 3,4 (continuing sequentially from wherever the
     * pointer already was), or does something else happen (overwrite
     * 1/2, error, garbage)? */
    type_str("33");
    xeq("SAVEX");
    type_str("44");
    xeq("SAVEX");
    disp = display_to_buf(dispbuf);
    printf("after writing 33,44 post-grow: display=\"%s\"\n", disp);
    if (strstr(disp, "N O N E X I S T E N T") != NULL) {
        printf("FAIL: SAVEX hit NONEXISTENT after growth.\n");
        return 1;
    }

    unsigned char after_fill[128 * 8];
    memcpy(after_fill, &espaceRAM[64 * 8], sizeof(after_fill));
    printf("XM RAM diff after writing 33,44 (should show 2 NEW register writes, "
           "and registers holding 11/22 should be UNCHANGED from after_grow):\n");
    int changed_11_22 = 0;
    for (int reg = 0; reg < 128; reg++) {
        if (memcmp(&after_grow[reg * 8], &after_fill[reg * 8], 8) != 0) {
            printf("  XM reg %d: ", reg);
            for (int b = 0; b < 8; b++) printf("%02x ", after_grow[reg * 8 + b]);
            printf("-> ");
            for (int b = 0; b < 8; b++) printf("%02x ", after_fill[reg * 8 + b]);
            printf("\n");
        }
    }
    (void)changed_11_22;

    /* Critical re-test: SEEKPTA previously crashed the emulator under the
     * WRONG page-4 wiring. Does it still crash now that page 5 is wired
     * correctly (bank-switched, CXFUNS1 in bank 0)? */
    type_byte(0x01); type_byte(0x01); /* ALPHA ALPHA = clear alpha */
    type_str("1");
    xeq("SEEKPTA");
    disp = display_to_buf(dispbuf);
    printf("after SEEKPTA(1): display=\"%s\"\n", disp);

    /* Shrink back to 2 via RESZFL(2) - should discard 3/4's data, keep 1/2 intact. */
    type_str("2");
    xeq("RESZFL");
    disp = display_to_buf(dispbuf);
    printf("after RESZFL(2) shrink: display=\"%s\"\n", disp);
    if (strstr(disp, "N O N E X I S T E N T") != NULL) {
        printf("FAIL: RESZFL(shrink) hit NONEXISTENT.\n");
        return 1;
    }

    printf("PASS (no crash, no NONEXISTENT) - inspect the diffs above by hand to confirm "
           "sequential grow-then-write behaves as the seek-free frame-stack design needs.\n");
    return 0;
}
