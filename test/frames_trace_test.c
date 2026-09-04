/**
 * @file frames_trace_test.c
 * @brief Instruction-level trace of LCLS's execution to find why it
 *        leaves the display showing program-listing-like text ("RTN",
 *        "STO 08") instead of completing cleanly, with zero visible
 *        change to XM (see frames_test.c).
 */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define GLOBAL extern
#include "nutcpu.h"

#include "display.h"
#include "nut_rom_cx.h"
#include "hp41_key_bridge.h"

extern const uint16_t rom_frames_p0[4096];

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

static void type_byte_traced(int c, int max_steps) {
    if (asleep_state) { flagKey = 0; regPC = 0; asleep_state = false; }
    hp41_key_bridge_feed_byte(c);
    int last_page = -1;
    for (int i = 0; i < max_steps; i++) {
        int pc = regPC;
        int page = (pc >> 12) & 0xF;
        if (page != last_page) {
            printf("-- page transition: now page %d (PC=%04X) at step %d --\n", page, pc, i);
            last_page = page;
        }
        if (page == 8) {
            int codes[2] = { fetch_word(pc), fetch_word(pc + 1) };
            char ligne[128] = {0};
            (void)desas(pc, codes, ligne);
            printf("[%5d] PC=%04X %-20s regData=%03X(dec %d) Carry=%d regPer=%02x ",
                   i, pc, ligne, regData, regData, Carry, regPer);
            print_reg("A", regA);
            print_reg("B", regB);
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

    setvbuf(stdout, NULL, _IONBF, 0);
    nut_boot_cx();
    tabpage[8] = (short *)(const void *)rom_frames_p0;
    typmod[8] = 1;

    int ret = pump(COLD_BOOT_ITERS);
    printf("cold boot: ret=%d, display=\"%s\"\n", ret, display_to_buf(dispbuf));
    if (ret == 1) asleep_state = true;

    type_byte(0x01); type_str("MFSTK"); type_byte(0x01);
    type_str("2");
    type_byte(0x18); type_byte(0x01); type_str("CRFLD"); type_byte(0x01);
    printf("setup CRFLD(MFSTK,2): display=\"%s\"\n", display_to_buf(dispbuf));

    type_byte(0x18); /* XEQ */
    type_byte(0x01); /* ALPHA */
    type_str("LCLS");
    printf("=== tracing closing ALPHA that executes LCLS ===\n");
    type_byte_traced(0x01, 200000);

    printf("after LCLS trace: display=\"%s\" regPC=0x%04x\n", display_to_buf(dispbuf), regPC);
    return 0;
}
