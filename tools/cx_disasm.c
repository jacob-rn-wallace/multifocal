/* Throwaway research tool - disassemble real CXFUNS ROM routines to
 * verify the calling convention for HP-41CX Extended Memory OS entry
 * points (SEEKPTA, GETRX, SAVERX, CRFLD, RESZFL, EMROOM, EMDIR, RCLPTA)
 * before writing MultiFOCAL's own MCODE against them. Not part of the
 * shipped project - scratch/ is gitignored.
 *
 * Adapted from soynut's tools/nut_disasm.c (same technique, extended to
 * also wire CXFUNS0/1 at pages 3/4 instead of just the base OS).
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

extern const uint16_t rom_nut0[4096];
extern const uint16_t rom_nut1[4096];
extern const uint16_t rom_nut2[4096];
extern const uint16_t rom_cxfuns0[4096];
extern const uint16_t rom_cxfuns1[4096];

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

extern int desas(int adr, int *codes, char *ligne);

static int fetch_word(int addr) {
    int page = (addr >> 12) & 0xF;
    int off = addr & 0xFFF;
    switch (page) {
        case 0: return rom_nut0[off];
        case 1: return rom_nut1[off];
        case 2: return rom_nut2[off];
        case 3: return rom_cxfuns0[off];
        case 4: return rom_cxfuns1[off];
        default: return 0;
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <start_addr_hex> <num_instructions>\n", argv[0]);
        return 1;
    }
    int addr = (int)strtol(argv[1], NULL, 16);
    int count = atoi(argv[2]);
    for (int i = 0; i < count; i++) {
        int codes[2];
        codes[0] = fetch_word(addr);
        codes[1] = fetch_word(addr + 1);
        char ligne[128] = {0};
        int n = desas(addr, codes, ligne);
        printf("%04X: %04X", addr, codes[0]);
        if (n == 2) printf(" %04X", codes[1]); else printf("     ");
        printf("  %s\n", ligne);
        addr += n;
    }
    return 0;
}
