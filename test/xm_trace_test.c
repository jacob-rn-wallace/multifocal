/**
 * @file xm_trace_test.c
 * @brief Instruction-level trace of the SAVER->SEEKPT->GETR sequence to
 *        find why the write doesn't show up in espaceRAM (see
 *        xm_probe_test.c for the higher-level probe this follows up on).
 *
 * Single-steps executeNUT(1) through the SAVER call and logs regPC,
 * disassembly, regData, regA/B/C, and Carry every instruction for a
 * bounded window.
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

extern int desas(int adr, int *codes, char *ligne);
int ch_label(long adr, char *s) { (void)adr; (void)s; return 0; }
char char41(int v) {
    char c;
    v &= 0x13f;
    if (v <= 0x1f) c = v + '@';
    else if (v <= 0x3f) c = v;
    else if (v < 0x100) c = '.';
    else if (v <= 0x105) c = v - 0xa0;
    else if (v <= 0x10f) c = '*';
    else c = '.';
    return c;
}

static int fetch_word(int addr) {
    int page = (addr >> 12) & 0xF;
    int off = addr & 0xFFF;
    if (tabpage[page] == NULL) return 0;
    return (unsigned short)tabpage[page][off];
}

static void print_reg(const char *name, unsigned char *r) {
    printf("%s=", name);
    for (int i = 13; i >= 0; i--) printf("%x", r[i] & 0xF);
    printf(" ");
}

static int pump(int iters) {
    int ret = 0;
    for (int i = 0; i < iters; i++) {
        ret = executeNUT(1000);
        if (ret != 0) break;
    }
    return ret;
}

#define COLD_BOOT_ITERS 300000
#define PER_KEY_ITERS 100000

static bool asleep_state = false;

static int type_byte(int c) {
    if (asleep_state) { flagKey = 0; regPC = 0; asleep_state = false; }
    hp41_key_bridge_feed_byte(c);
    const int ret = pump(PER_KEY_ITERS);
    if (ret == 1) asleep_state = true;
    return ret;
}

static void type_str(const char *s) { for (; *s; s++) type_byte((unsigned char)*s); }

/* Feed one byte but only give the CPU a SMALL bounded number of real
 * instruction-steps, tracing each one - used for the final "close ALPHA
 * on SAVER" keystroke, where we want to watch what happens rather than
 * blast through it with a huge pump(). */
static void type_byte_traced(int c, int max_steps, int only_page3) {
    if (asleep_state) { flagKey = 0; regPC = 0; asleep_state = false; }
    hp41_key_bridge_feed_byte(c);
    int last_page = -1;
    for (int i = 0; i < max_steps; i++) {
        int pc = regPC;
        int page = (pc >> 12) & 0xF;
        int show = !only_page3 || page == 3 || page == 4;
        if (page != last_page) {
            printf("  -- page transition: now page %d (PC=%04X) at step %d --\n", page, pc, i);
            last_page = page;
        }
        if (show) {
            int codes[2] = { fetch_word(pc), fetch_word(pc + 1) };
            char ligne[128] = {0};
            (void)desas(pc, codes, ligne);
            printf("[%5d] PC=%04X %-20s regData=%04X(dec %d) Carry=%d flagKey=%d lgkeybuf=%d dspon=%d fdsp=%d ",
                   i, pc, ligne, regData, regData, Carry, flagKey, lgkeybuf, dspon, fdsp);
            print_reg("A", regA);
            print_reg("C", regC);
            printf("\n");
        }
        int ret = executeNUT(1);
        if (ret == 1) { printf("  -> POWOFF\n"); asleep_state = true; break; }
        if (ret == 2) { printf("  -> INVALID OPCODE at PC=%04X\n", pc); break; }
        if (ret == 3) { printf("  -> BREAKPOINT\n"); break; }
    }
}

int main(void) {
    char dispbuf[32];

    nut_boot_cx();
    int ret = pump(COLD_BOOT_ITERS);
    printf("cold boot: ret=%d, display=\"%s\"\n", ret, display_to_buf(dispbuf));
    if (ret == 1) asleep_state = true;

    type_byte(0x01); type_str("TEST"); type_byte(0x01);
    type_str("10");
    type_byte(0x18); type_byte(0x01); type_str("CRFLD"); type_byte(0x01);
    printf("after CRFLD: display=\"%s\"\n", display_to_buf(dispbuf));

    type_str("42");
    printf("after typing 42: display=\"%s\" regPC=0x%04x\n", display_to_buf(dispbuf), regPC);

    unsigned char before[128 * 8];
    memcpy(before, &espaceRAM[64 * 8], sizeof(before));

    type_byte(0x18);
    type_byte(0x01);
    type_str("SAVER");
    printf("=== tracing final ALPHA keystroke that closes+executes SAVER (page transitions + page 3/4 detail only) ===\n");
    type_byte_traced(0x01, 20000, 1);

    printf("after SAVER trace: display=\"%s\" regPC=0x%04x\n", display_to_buf(dispbuf), regPC);

    unsigned char after_saver[128 * 8];
    memcpy(after_saver, &espaceRAM[64 * 8], sizeof(after_saver));
    printf("XM RAM diff immediately after SAVER trace:\n");
    int any = 0;
    for (int reg = 0; reg < 128; reg++) {
        if (memcmp(&before[reg * 8], &after_saver[reg * 8], 8) != 0) {
            any = 1;
            printf("  XM reg %d: ", reg);
            for (int b = 0; b < 8; b++) printf("%02x ", before[reg * 8 + b]);
            printf("-> ");
            for (int b = 0; b < 8; b++) printf("%02x ", after_saver[reg * 8 + b]);
            printf("\n");
        }
    }
    if (!any) printf("  (no change anywhere in XM registers 64-191)\n");

    return 0;
}
