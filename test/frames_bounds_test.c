/**
 * @file frames_bounds_test.c
 * @brief Phase 2: LCLS/LCLX recursion-depth-ceiling boundary test.
 *
 * Confirms the bounds check added to src/frames.s: LCLS refuses to
 * grow past the depth ceiling (34 = 8 frames x 4 + header 2) and LCLX
 * refuses to shrink past empty (2, header only) - both by leaving X
 * unchanged rather than doing the arithmetic. Also confirms the two
 * calls immediately adjacent to each boundary still work normally.
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
static int pump(int iters){int ret=0;for(int i=0;i<iters;i++){ret=executeNUT(1000);if(ret!=0)break;}return ret;}
static bool asleep=false;
static int type_byte(int c){if(asleep){flagKey=0;regPC=0;asleep=false;}hp41_key_bridge_feed_byte(c);int r=pump(200000);if(r==1)asleep=true;return r;}
static void type_str(const char*s){for(;*s;s++)type_byte((unsigned char)*s);}
static void xeq(const char*n){type_byte(0x18);type_byte(0x01);type_str(n);type_byte(0x01);}
static int parse_display_int(const char *d) {
    char buf[32]; int j = 0;
    for (int i = 0; d[i] && j < 31; i++) if (d[i] != ' ') buf[j++] = d[i];
    buf[j] = 0;
    return atoi(buf);
}
int main(void){
    char d[32];
    int fails = 0;
    nut_boot_cx();
    tabpage[8]=(short*)(const void*)rom_frames_p0;
    typmod[8]=1;
    int ret=pump(300000);
    if(ret==1) asleep=true;
    type_byte(0x01); type_str("MFSTK"); type_byte(0x01);
    type_str("34");
    xeq("CRFLD");
    printf("setup CRFLD(MFSTK,34): disp=\"%s\"\n", display_to_buf(d));

    /* Already at the depth ceiling - LCLS should refuse and leave X=34. */
    type_str("34");
    xeq("LCLS");
    { int got = parse_display_int(display_to_buf(d)); printf("LCLS(34) [at ceiling] -> got=%d (expect 34, refused) %s\n", got, got==34?"OK":"MISMATCH"); if (got!=34) fails++; }

    /* Already empty - LCLX should refuse and leave X=2. */
    type_str("2");
    xeq("LCLX");
    { int got = parse_display_int(display_to_buf(d)); printf("LCLX(2) [empty] -> got=%d (expect 2, refused) %s\n", got, got==2?"OK":"MISMATCH"); if (got!=2) fails++; }

    /* One below the ceiling should still succeed normally. */
    type_str("30");
    xeq("LCLS");
    { int got = parse_display_int(display_to_buf(d)); printf("LCLS(30) [one below ceiling] -> got=%d (expect 34) %s\n", got, got==34?"OK":"MISMATCH"); if (got!=34) fails++; }

    /* One above empty should still succeed normally. */
    type_str("6");
    xeq("LCLX");
    { int got = parse_display_int(display_to_buf(d)); printf("LCLX(6) [one above empty] -> got=%d (expect 2) %s\n", got, got==2?"OK":"MISMATCH"); if (got!=2) fails++; }

    if (fails == 0) { printf("PASS: all depth-ceiling boundary checks correct.\n"); return 0; }
    printf("FAIL: %d boundary checks mismatched.\n", fails);
    return 1;
}
