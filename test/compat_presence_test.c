/**
 * @file compat_presence_test.c
 * @brief Compatibility testing, Test 1: MultiFOCAL's module, merely
 *        PRESENT (never invoked by name), must not change the outcome
 *        of any native operation - the literal wording of CLAUDE.md's
 *        hard constraint ("native FOCAL programs must behave
 *        identically whether or not MultiFOCAL is present").
 *
 * Runs an identical sequence of real, native (non-MultiFOCAL) HP-41CX
 * operations - creating and using its OWN XM file via the real catalog
 * functions (CRFLD/SAVEX/GETX/RESZFL, exactly as Phase 2/3's own probes
 * used them), plus real RPN arithmetic (ENTER, +, -, *, /) - and prints
 * every intermediate display plus a final raw dump of the CX's 128
 * built-in XM registers (espaceRAM[64*8..192*8), the same region
 * xm_probe_test.c/resz_probe_test.c already inspect this way) to
 * stdout. None of these operations ever reference LCLS/LCLX/LSTO/LRCL/
 * LRST/MFSTK by name.
 *
 * Takes one argument: "with" loads MultiFOCAL's own module at page 8
 * exactly like every other test in this suite; any other value (or no
 * argument) leaves page 8 untouched, matching nut_boot_cx()'s own
 * default (confirmed by inspection: it never touches page 8 itself -
 * every existing MultiFOCAL test sets tabpage[8]/typmod[8] explicitly
 * after calling it). Run twice, once per argument, and diff the two
 * stdout captures (test/run_compat_presence.sh does this) - if
 * MultiFOCAL's module is truly additive-only, the two outputs must be
 * byte-for-byte identical.
 *
 * NOTE on the printed values below: a real, currently-unexplained
 * pointer-arithmetic anomaly was found while writing this test (real
 * catalog-dispatched GETX's first read after a fresh SEEKPTA does not
 * return the just-seeked-to register's value - see the compatibility-
 * testing section of CLAUDE.md for the full observation). It's flagged
 * there, not root-caused here, and it does NOT affect this test's own
 * conclusion: whatever the true semantics are, they're either present
 * or absent identically in both runs, which is the only thing this
 * test needs to establish. The "(expect N)" labels below describe what
 * a naive read-after-write model would predict, kept only so a human
 * skimming the output can see where that model breaks down - the test
 * itself makes no assertion on these values, only on whether the two
 * full runs match each other.
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
extern const uint16_t rom_frames_p0[4096];

static int pump(int iters) { int ret=0; for(int i=0;i<iters;i++){ret=executeNUT(1000); if(ret!=0)break;} return ret; }
static bool asleep_state=false;
static int type_byte(int c){ if(asleep_state){flagKey=0;regPC=0;asleep_state=false;} hp41_key_bridge_feed_byte(c); int ret=pump(200000); if(ret==1)asleep_state=true; return ret; }
static void type_str(const char*s){for(;*s;s++)type_byte((unsigned char)*s);}
static void xeq(const char*n){type_byte(0x18);type_byte(0x01);type_str(n);type_byte(0x01);}

static void show(const char *label) {
    char dispbuf[32];
    printf("%-28s disp=\"%s\"\n", label, display_to_buf(dispbuf));
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
    /* Goes to stderr, not stdout - stdout must be byte-identical
     * between the two runs, and this line necessarily isn't. */
    fprintf(stderr, "module=%s\n", with_module ? "present" : "absent");
    show("cold boot");

    /* Native file, native name, no MultiFOCAL involvement at all. */
    type_byte(0x01); type_str("NATIVE"); type_byte(0x01);
    type_str("5");
    xeq("CRFLD");
    show("CRFLD(NATIVE,5)");

    type_str("11"); xeq("SAVEX"); show("SAVEX(11)");
    type_str("22"); xeq("SAVEX"); show("SAVEX(22)");
    type_str("33"); xeq("SAVEX"); show("SAVEX(33)");

    /* Reseek to read them back. */
    type_byte(0x01); type_str("NATIVE"); type_byte(0x01);
    type_str("1");
    xeq("SEEKPTA");
    show("SEEKPTA(NATIVE,1)");
    xeq("GETX"); show("GETX #1 (expect 11)");
    xeq("GETX"); show("GETX #2 (expect 22)");
    xeq("GETX"); show("GETX #3 (expect 33)");

    /* Grow the native file - a real, non-MultiFOCAL RESZFL call. */
    type_str("8");
    xeq("RESZFL");
    show("RESZFL(NATIVE,8)");

    /* Real RPN arithmetic - ENTER, +, -, *, / all go through tabcode[],
     * exactly as a physical keypress would. */
    type_str("5"); type_byte(13);
    type_str("3"); type_byte('+');
    show("5 ENTER 3 + (expect 8)");
    type_str("2"); type_byte('-');
    show("2 - (expect 6)");
    type_str("4"); type_byte('*');
    show("4 * (expect 24)");
    type_str("3"); type_byte('/');
    show("3 / (expect 8)");

    /* Confirm the native file's own data is untouched by any of the
     * above. */
    type_byte(0x01); type_str("NATIVE"); type_byte(0x01);
    type_str("1");
    xeq("SEEKPTA");
    xeq("GETX"); show("post-arith GETX #1 (expect 11)");
    xeq("GETX"); show("post-arith GETX #2 (expect 22)");
    xeq("GETX"); show("post-arith GETX #3 (expect 33)");

    /* Final raw XM-region dump (the CX's 128 built-in XM registers). */
    unsigned char xm[128 * 8];
    memcpy(xm, &espaceRAM[64 * 8], sizeof(xm));
    unsigned long checksum = 0;
    for (size_t i = 0; i < sizeof(xm); i++) checksum = checksum * 31 + xm[i];
    printf("final XM region checksum: %lu\n", checksum);

    return 0;
}
