/**
 * @file phase0_loop_test.c
 * @brief MultiFOCAL Phase 0 milestone: assemble a trivial MCODE routine,
 *        load it in the emulator, call it from a FOCAL test program.
 *
 * Headless (no SDL/GUI) - boots the real HP-41 OS ROM plus the
 * MultiFOCAL Phase 0 test module (see nut_rom_multifocal.h) against
 * soynut's real Nut CPU core, types the keystroke sequence
 * "XEQ ALPHA MFTEST ALPHA" over the same USB-serial key-bridge protocol
 * soynut's own firmware/sim use, and checks that the display shows
 * "MFTEST WORKS" - proof that XEQ-by-name found the module's FAT entry
 * on page 8 and executed real MCODE out of it, not that the routine was
 * just jumped to directly.
 *
 * executeNUT(n) (emu41gcc/nutcpu.c) returns as soon as the display goes
 * dirty (fdsp), not after n instructions - the real idle loop redraws
 * the LCD almost every instruction, so each call nets roughly one
 * instruction while idle. This is why soynut's own real main loops
 * (firmware/main.c, sim/sim_main.c) call it once per outer-loop
 * iteration and rely on many iterations over wall-clock time, rather
 * than expecting one call to advance the CPU by n instructions. This
 * harness does the same: pump() runs many outer iterations rather than
 * requesting a large n.
 *
 * Build/run: make -C test run
 */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define GLOBAL extern
#include "nutcpu.h"

#include "display.h"
#include "nut_rom.h"
#include "nut_rom_multifocal.h"
#include "hp41_key_bridge.h"

/**
 * @brief Run the CPU for up to `iters` outer iterations (see file header
 *        for why iteration count, not instruction count, is what matters
 *        here), stopping early on POWOFF/invalid-opcode/breakpoint.
 * @return executeNUT()'s last status code (0=OK/limit, 1=POWOFF, 2=invalid opcode, 3=breakpoint).
 */
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

/** Outer-loop iterations to allow cold boot (memory clear + settle into idle) to finish. */
#define COLD_BOOT_ITERS 300000
/** Outer-loop iterations to allow one keystroke to be picked up and processed. */
#define PER_KEY_ITERS 20000

/* Whether the CPU is currently asleep (auto power-off). executeNUT()'s
 * POWOFF return (1) doesn't stop the CPU by itself - real real callers
 * (soynut's firmware/main.c, sim/sim_main.c) track this externally and,
 * on the next key, wake it by resetting regPC to the reset vector
 * (mirroring the real hardware: any keypress while off re-vectors the
 * CPU to address 0, same as a cold reset, but WITHOUT re-arming the
 * coldstart Carry flag - the OS tells warm-wake from cold-boot apart via
 * its own already-initialized RAM state, not by us re-signaling
 * coldstart). See sim/sim_main.c's sim_handle_sleep_state(). */
static bool asleep_state = false;

static void type_byte(int c)
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
}

int main(void)
{
    char dispbuf[32];

    nut_boot();
    nut_rom_wire_multifocal_test_module();
    assert(regPC == 0);

    int ret = pump(COLD_BOOT_ITERS);
    printf("cold boot: ret=%d, display=\"%s\"\n", ret, display_to_buf(dispbuf));
    if (ret != 0 && ret != 1) {
        printf("FAIL: cold boot hit ret=%d (expected 0=OK or 1=POWOFF)\n", ret);
        return 1;
    }
    if (ret == 1)
        asleep_state = true;

    /* "XEQ ALPHA MFTEST ALPHA" - ctrl-X = XEQ, ctrl-A = ALPHA (see
     * hp41_key_bridge.h's protocol doc), then the name, then ALPHA again
     * to close alpha entry and execute. */
    type_byte(0x18); /* XEQ */
    type_byte(0x01); /* ALPHA */
    type_byte('M');
    type_byte('F');
    type_byte('T');
    type_byte('E');
    type_byte('S');
    type_byte('T');
    type_byte(0x01); /* ALPHA - closes entry, executes */

    const char *disp = display_to_buf(dispbuf);
    printf("after XEQ MFTEST: display=\"%s\"\n", disp);

    /* display_to_buf() puts a delimiter between every display cell
     * (including within one word), so compare with spaces stripped from
     * both sides rather than expecting an exact literal match. */
    char packed[32];
    size_t j = 0;
    for (size_t i = 0; disp[i] != '\0' && j + 1 < sizeof(packed); i++)
        if (disp[i] != ' ')
            packed[j++] = disp[i];
    packed[j] = '\0';

    if (strstr(packed, "MFTESTWORKS") == NULL) {
        printf("FAIL: expected \"MFTEST WORKS\" on display, got \"%s\"\n", disp);
        return 1;
    }

    printf("PASS: MultiFOCAL Phase 0 loop confirmed - assembled MCODE, "
           "loaded into the emulator, found and executed by name from "
           "a FOCAL-level XEQ.\n");
    return 0;
}
