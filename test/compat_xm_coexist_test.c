/**
 * @file compat_xm_coexist_test.c
 * @brief Compatibility testing, Test 2: MultiFOCAL's own XM usage must
 *        not corrupt a native FOCAL program's INDEPENDENT XM file.
 *
 * Presence-only invariance (compat_presence_test.c) covers the case
 * where MultiFOCAL's functions are never invoked at all. The other
 * real compatibility risk, specific to this project's own design, is
 * the one Phase 2 groundwork flagged from the start (see CLAUDE.md):
 * "current file + pointer" is global HP-41CX OS state, not scoped to
 * any one caller. LSTO/LRCL (unlike the pure-arithmetic LCLS/LCLX) DO
 * touch XM via SAVEX/REALGETX against whatever file is currently
 * seeked. If a native FOCAL program has its own XM file open and a
 * subroutine call in between uses LSTO/LRCL (making MFSTK briefly
 * "current"), does the native program's own file survive completely
 * intact once it reseeks back to its own file?
 *
 * Deliberately does NOT assert specific numeric values read back from
 * XM (a real, currently-unexplained SAVEX/GETX pointer-arithmetic
 * anomaly was found while building this test suite - see
 * compat_presence_test.c and CLAUDE.md's compatibility-testing
 * section). Instead this test is self-referential: it reads the native
 * file's contents via a SEEKPTA+GETX sequence TWICE - once right after
 * writing it (the "baseline", whatever its actual values turn out to
 * be), and once again after a full round of MultiFOCAL activity (CRFLD
 * MFSTK, 3x LCLS, a seek, 2x LSTO, 3x LCLX) has made MFSTK "current" in
 * between. Whatever the true pointer semantics are, both read passes
 * go through the identical sequence, so if MultiFOCAL's own operations
 * left the native file untouched, the two passes must produce
 * byte-for-byte identical results - this needs no independent
 * assumption about what "correct" values should be.
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
extern const uint16_t rom_frames_p0[4096];

static int pump(int iters) { int ret=0; for(int i=0;i<iters;i++){ret=executeNUT(1000); if(ret!=0)break;} return ret; }
static bool asleep_state=false;
static int type_byte(int c){ if(asleep_state){flagKey=0;regPC=0;asleep_state=false;} hp41_key_bridge_feed_byte(c); int ret=pump(200000); if(ret==1)asleep_state=true; return ret; }
static void type_str(const char*s){for(;*s;s++)type_byte((unsigned char)*s);}
static void xeq(const char*n){type_byte(0x18);type_byte(0x01);type_str(n);type_byte(0x01);}

static void seek_file(const char *name, int reg) {
    char buf[8];
    type_byte(0x01); type_str(name); type_byte(0x01);
    snprintf(buf, sizeof(buf), "%d", reg);
    type_str(buf);
    xeq("SEEKPTA");
}

static int parse_display_int(const char *d) {
    char buf[32]; int j = 0;
    for (int i = 0; d[i] && j < 31; i++) if (d[i] != ' ') buf[j++] = d[i];
    buf[j] = 0;
    return atoi(buf);
}

/* Reads 3 consecutive values from NATIVE (starting at register 1) via
 * real catalog GETX, capturing whatever the display shows each time -
 * see the file header on why no specific values are assumed. */
static void read_native_triplet(int out[3]) {
    char dispbuf[32];
    seek_file("NATIVE", 1);
    for (int i = 0; i < 3; i++) {
        xeq("GETX");
        out[i] = parse_display_int(display_to_buf(dispbuf));
    }
}

static void push_lcls(int *size) {
    char dispbuf[32], buf[8];
    snprintf(buf, sizeof(buf), "%d", *size);
    type_str(buf);
    xeq("LCLS");
    *size = parse_display_int(display_to_buf(dispbuf));
}

static void pop_lclx(int *size) {
    char dispbuf[32], buf[8];
    snprintf(buf, sizeof(buf), "%d", *size);
    type_str(buf);
    xeq("LCLX");
    *size = parse_display_int(display_to_buf(dispbuf));
}

int main(void) {
    char dispbuf[32];
    setvbuf(stdout, NULL, _IONBF, 0);
    nut_boot_cx();
    tabpage[8] = (short *)(const void *)rom_frames_p0;
    typmod[8] = 1;
    int ret = pump(300000);
    if (ret == 1) asleep_state = true;

    /* Native program creates and populates its own XM file - no
     * MultiFOCAL involvement at all yet. */
    type_byte(0x01); type_str("NATIVE"); type_byte(0x01);
    type_str("8");
    xeq("CRFLD");
    printf("setup CRFLD(NATIVE,8): disp=\"%s\"\n", display_to_buf(dispbuf));
    type_str("91"); xeq("SAVEX");
    type_str("92"); xeq("SAVEX");
    type_str("93"); xeq("SAVEX");

    int baseline[3];
    read_native_triplet(baseline);
    printf("baseline read (immediately after writing): %d, %d, %d\n",
           baseline[0], baseline[1], baseline[2]);

    /* A full round of MultiFOCAL activity: create/use its own file,
     * which becomes "current" the moment CRFLD runs, displacing
     * NATIVE - exactly the shared-global-state risk this test targets. */
    type_byte(0x01); type_str("MFSTK"); type_byte(0x01);
    type_str("35");
    xeq("CRFLD");
    printf("MultiFOCAL setup CRFLD(MFSTK,35): disp=\"%s\"\n", display_to_buf(dispbuf));

    int size = 2;
    push_lcls(&size); push_lcls(&size); push_lcls(&size);
    printf("after 3x LCLS: size=%d (expect 14)\n", size);

    seek_file("MFSTK", 3);
    type_str("55"); xeq("LSTO");
    type_str("56"); xeq("LSTO");
    printf("LSTO(55), LSTO(56) into MFSTK frame1 done\n");

    pop_lclx(&size); pop_lclx(&size); pop_lclx(&size);
    printf("after 3x LCLX: size=%d (expect 2)\n", size);

    /* Back to NATIVE - does it still read exactly as it did before any
     * of the above happened? */
    int after[3];
    read_native_triplet(after);
    printf("post-MultiFOCAL read (same seek+GETX sequence): %d, %d, %d\n",
           after[0], after[1], after[2]);

    int fails = 0;
    if (size != 2) { printf("FAIL: MFSTK size did not return to 2 (got %d)\n", size); fails++; }
    for (int i = 0; i < 3; i++) {
        if (after[i] != baseline[i]) {
            printf("MISMATCH at slot %d: baseline=%d after=%d\n", i, baseline[i], after[i]);
            fails++;
        }
    }

    if (fails == 0) {
        printf("PASS: NATIVE's own XM file reads back identically before and "
               "after a full round of MultiFOCAL activity (CRFLD/LCLS/LSTO/"
               "LCLX) made MFSTK the OS's \"current\" file in between - "
               "MultiFOCAL's XM usage does not corrupt an independent "
               "native file.\n");
        return 0;
    }
    printf("FAIL: %d check(s) failed.\n", fails);
    return 1;
}
